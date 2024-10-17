#include "stdafx.h"
#include "029_InstanceTriangles.h"

EngineApplicationCreateInfo Example_029::Config() const
{
	EngineApplicationCreateInfo createInfo{};
	//createInfo.render.swapChain.depthFormat = vkr::Format::D32_FLOAT;
	createInfo.render.showImgui = true;
	//createInfo.render.swapChain.imageCount = 1;
	return createInfo;
}

bool Example_029::Setup()
{
	auto& device = GetRenderDevice();

	// Pipeline
	{
		CHECKED_CALL(device.CreateShader("basic/shaders", "PassThroughPos.vs", &mVS));
		CHECKED_CALL(device.CreateShader("basic/shaders", "PassThroughPos.ps", &mPS));

		vkr::PipelineInterfaceCreateInfo piCreateInfo = {};
		CHECKED_CALL(device.CreatePipelineInterface(piCreateInfo, &mPipelineInterface));

		mVertexBinding.AppendAttribute({ "POSITION", 0, vkr::Format::R32G32B32A32_FLOAT, 0, APPEND_OFFSET_ALIGNED, vkr::VertexInputRate::Vertex });

		vkr::GraphicsPipelineCreateInfo2 gpCreateInfo = {};
		gpCreateInfo.VS = { mVS.Get(), "vsmain" };
		gpCreateInfo.PS = { mPS.Get(), "psmain" };
		gpCreateInfo.vertexInputState.bindingCount = 1;
		gpCreateInfo.vertexInputState.bindings[0] = mVertexBinding;
		gpCreateInfo.topology = vkr::PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		gpCreateInfo.polygonMode = vkr::POLYGON_MODE_FILL;
		gpCreateInfo.cullMode = vkr::CULL_MODE_NONE;
		gpCreateInfo.frontFace = vkr::FRONT_FACE_CCW;
		gpCreateInfo.depthReadEnable = false;
		gpCreateInfo.depthWriteEnable = false;
		gpCreateInfo.blendModes[0] = vkr::BLEND_MODE_NONE;
		gpCreateInfo.outputState.renderTargetCount = 1;
		gpCreateInfo.outputState.renderTargetFormats[0] = GetRender().GetSwapChain().GetColorFormat();
		gpCreateInfo.outputState.depthStencilFormat = GetRender().GetSwapChain().GetDepthFormat();
		gpCreateInfo.pipelineInterface = mPipelineInterface;
		CHECKED_CALL(device.CreateGraphicsPipeline(gpCreateInfo, &mPipeline));
	}

	// Per frame data
	{
		PerFrame frame = {};

		CHECKED_CALL(device.GetGraphicsQueue()->CreateCommandBuffer(&frame.cmd));

		vkr::SemaphoreCreateInfo semaCreateInfo = {};
		CHECKED_CALL(device.CreateSemaphore(semaCreateInfo, &frame.imageAcquiredSemaphore));

		vkr::FenceCreateInfo fenceCreateInfo = {};
		CHECKED_CALL(device.CreateFence(fenceCreateInfo, &frame.imageAcquiredFence));

		CHECKED_CALL(device.CreateSemaphore(semaCreateInfo, &frame.renderCompleteSemaphore));

		fenceCreateInfo = { true }; // Create signaled
		CHECKED_CALL(device.CreateFence(fenceCreateInfo, &frame.renderCompleteFence));

		vkr::QueryCreateInfo queryCreateInfo = {};
		queryCreateInfo.type = vkr::QueryType::Timestamp;
		queryCreateInfo.count = 2;
		CHECKED_CALL(device.CreateQuery(queryCreateInfo, &frame.timestampQuery));

		mPerFrame.push_back(frame);
	}

	mRenderTargetSize = uint2(GetWindowWidth(), GetWindowHeight());

	mViewport = { 0, 0, float(mRenderTargetSize.x), float(mRenderTargetSize.y), 0, 1 };
	mScissorRect = { 0, 0, mRenderTargetSize.x, mRenderTargetSize.y };

	// Vertex buffer for triangle.
	{
		// Size of half the triangle's side.
		// Use a very small triangle since we are not interested in anything
		// but draw call load.
		float triangleHalfSize = 0.5f;

		std::vector<float> vertexData = {
			 triangleHalfSize,  triangleHalfSize, 0.0f, 1.0f,
			-triangleHalfSize,  triangleHalfSize, 0.0f, 1.0f,
			-triangleHalfSize, -triangleHalfSize, 0.0f, 1.0f,
		};
		uint32_t dataSize = SizeInBytesU32(vertexData);

		vkr::BufferCreateInfo bufferCreateInfo = {};
		bufferCreateInfo.size = dataSize;
		bufferCreateInfo.usageFlags.bits.vertexBuffer = true;
		bufferCreateInfo.memoryUsage = vkr::MemoryUsage::CPUToGPU;
		CHECKED_CALL(device.CreateBuffer(bufferCreateInfo, &mVertexBuffer));

		void* pAddr = nullptr;
		CHECKED_CALL(mVertexBuffer->MapMemory(0, &pAddr));
		memcpy(pAddr, vertexData.data(), dataSize);
		mVertexBuffer->UnmapMemory();
	}

	return true;
}

void Example_029::Shutdown()
{
	mPerFrame.clear();
	mVS.Reset();
	mPS.Reset();
	mPipelineInterface.Reset();
	mPipeline.Reset();
	mVertexBuffer.Reset();
}

void Example_029::Update()
{
}

void Example_029::Render()
{
	auto& render = GetRender();
	auto& swapChain = render.GetSwapChain();
	PerFrame& frame = mPerFrame[0];

	// Wait for and reset render complete fence
	CHECKED_CALL(frame.renderCompleteFence->WaitAndReset());

	uint32_t imageIndex = UINT32_MAX;
	CHECKED_CALL(swapChain.AcquireNextImage(UINT64_MAX, frame.imageAcquiredSemaphore, frame.imageAcquiredFence, &imageIndex));

	// Wait for and reset image acquired fence
	CHECKED_CALL(frame.imageAcquiredFence->WaitAndReset());

	// Read query results
	static bool firstFrame = true;
	if (!firstFrame)
	{
		uint64_t data[2] = { 0 };
		CHECKED_CALL(frame.timestampQuery->GetData(data, 2 * sizeof(uint64_t)));
		mGpuWorkDuration = data[1] - data[0];
	}

	// Reset queries
	frame.timestampQuery->Reset(0, 2);

	// Build command buffer
	CHECKED_CALL(frame.cmd->Begin());
	{
		vkr::RenderPassPtr renderPass = swapChain.GetRenderPass(imageIndex);
		ASSERT_MSG(!renderPass.IsNull(), "render pass object is null");

		frame.cmd->SetScissors(renderPass->GetScissor());
		frame.cmd->SetViewports(renderPass->GetViewport());

		frame.cmd->TransitionImageLayout(renderPass->GetRenderTargetImage(0), ALL_SUBRESOURCES, vkr::ResourceState::Present, vkr::ResourceState::RenderTarget);
		frame.cmd->BeginRenderPass(renderPass);
		{
			frame.cmd->WriteTimestamp(frame.timestampQuery, vkr::PIPELINE_STAGE_TOP_OF_PIPE_BIT, 0);
			frame.cmd->SetScissors(1, &mScissorRect);
			frame.cmd->SetViewports(1, &mViewport);
			frame.cmd->BindGraphicsPipeline(mPipeline);
			frame.cmd->BindVertexBuffers(1, &mVertexBuffer, &mVertexBinding.GetStride());
			if (mUseInstancedDraw) {
				frame.cmd->Draw(3, mNumTriangles, 0, 0);
			}
			else {
				for (uint32_t i = 0; i < mNumTriangles; ++i) {
					frame.cmd->Draw(3, 1, 0, 0);
				}
			}
			frame.cmd->WriteTimestamp(frame.timestampQuery, vkr::PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 1);

			// Draw ImGui
			render.DrawDebugInfo();
			render.DrawImGui(frame.cmd);
		}
		frame.cmd->EndRenderPass();
		frame.cmd->ResolveQueryData(frame.timestampQuery, 0, 2);
		frame.cmd->TransitionImageLayout(renderPass->GetRenderTargetImage(0), ALL_SUBRESOURCES, vkr::ResourceState::RenderTarget, vkr::ResourceState::Present);
	}
	CHECKED_CALL(frame.cmd->End());

	vkr::SubmitInfo submitInfo = {};
	submitInfo.commandBufferCount = 1;
	submitInfo.ppCommandBuffers = &frame.cmd;
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.ppWaitSemaphores = &frame.imageAcquiredSemaphore;
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.ppSignalSemaphores = &frame.renderCompleteSemaphore;
	submitInfo.pFence = frame.renderCompleteFence;

	CHECKED_CALL(render.GetGraphicsQueue()->Submit(&submitInfo));

	CHECKED_CALL(swapChain.Present(imageIndex, 1, &frame.renderCompleteSemaphore));

	if (firstFrame)
		firstFrame = false;
	else
	{
		uint64_t frequency = 0;
		render.GetGraphicsQueue()->GetTimestampFrequency(&frequency);
		const float      gpuWorkDuration = static_cast<float>(mGpuWorkDuration / static_cast<double>(frequency)) * 1000.0f;
		PerFrameRegister stats = {};
		//stats.frameNumber = GetFrameCount();
		stats.gpuWorkDuration = gpuWorkDuration;
		//stats.cpuFrameTime = GetPrevFrameTime();
		mFrameRegisters.push_back(stats);
	}
}
