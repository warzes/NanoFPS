#include "stdafx.h"
#include "007_DrawIndexed.h"

EngineApplicationCreateInfo Example_007::Config() const
{
	EngineApplicationCreateInfo createInfo{};
	createInfo.render.swapChain.depthFormat = vkr::FORMAT_D32_FLOAT;
	return createInfo;
}

bool Example_007::Setup()
{
	auto& device = GetRenderDevice();

	// Uniform buffer
	{
		vkr::BufferCreateInfo bufferCreateInfo = {};
		bufferCreateInfo.size = vkr::MINIMUM_UNIFORM_BUFFER_SIZE;
		bufferCreateInfo.usageFlags.bits.uniformBuffer = true;
		bufferCreateInfo.memoryUsage = vkr::MemoryUsage::CPUToGPU;

		CHECKED_CALL(device.CreateBuffer(bufferCreateInfo, &mUniformBuffer));
	}

	// Descriptor
	{
		vkr::DescriptorPoolCreateInfo poolCreateInfo = {};
		poolCreateInfo.uniformBuffer = 1;
		CHECKED_CALL(device.CreateDescriptorPool(poolCreateInfo, &mDescriptorPool));

		vkr::DescriptorSetLayoutCreateInfo layoutCreateInfo = {};
		layoutCreateInfo.bindings.push_back(vkr::DescriptorBinding{ 0, vkr::DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, vkr::SHADER_STAGE_ALL_GRAPHICS });
		CHECKED_CALL(device.CreateDescriptorSetLayout(layoutCreateInfo, &mDescriptorSetLayout));

		CHECKED_CALL(device.AllocateDescriptorSet(mDescriptorPool, mDescriptorSetLayout, &mDescriptorSet));

		vkr::WriteDescriptor write = {};
		write.binding = 0;
		write.type = vkr::DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		write.bufferOffset = 0;
		write.bufferRange = WHOLE_SIZE;
		write.pBuffer = mUniformBuffer;
		CHECKED_CALL(mDescriptorSet->UpdateDescriptors(1, &write));
	}

	// Pipeline
	{
		CHECKED_CALL(device.CreateShader("basic/shaders", "VertexColors.vs", &mVS));
		CHECKED_CALL(device.CreateShader("basic/shaders", "VertexColors.ps", &mPS));

		vkr::PipelineInterfaceCreateInfo piCreateInfo = {};
		piCreateInfo.setCount = 1;
		piCreateInfo.sets[0].set = 0;
		piCreateInfo.sets[0].pLayout = mDescriptorSetLayout;
		CHECKED_CALL(device.CreatePipelineInterface(piCreateInfo, &mPipelineInterface));

		mVertexBinding.AppendAttribute({ "POSITION", 0, vkr::FORMAT_R32G32B32_FLOAT, 0, APPEND_OFFSET_ALIGNED, vkr::VERTEX_INPUT_RATE_VERTEX });
		mVertexBinding.AppendAttribute({ "COLOR", 1, vkr::FORMAT_R32G32B32_FLOAT, 0, APPEND_OFFSET_ALIGNED, vkr::VERTEX_INPUT_RATE_VERTEX });

		vkr::GraphicsPipelineCreateInfo2 gpCreateInfo = {};
		gpCreateInfo.VS = { mVS.Get(), "vsmain" };
		gpCreateInfo.PS = { mPS.Get(), "psmain" };
		gpCreateInfo.vertexInputState.bindingCount = 1;
		gpCreateInfo.vertexInputState.bindings[0] = mVertexBinding;
		gpCreateInfo.topology = vkr::PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		gpCreateInfo.polygonMode = vkr::POLYGON_MODE_FILL;
		gpCreateInfo.cullMode = vkr::CULL_MODE_BACK;
		gpCreateInfo.frontFace = vkr::FRONT_FACE_CCW;
		gpCreateInfo.depthReadEnable = true;
		gpCreateInfo.depthWriteEnable = true;
		gpCreateInfo.blendModes[0] = vkr::BLEND_MODE_NONE;
		gpCreateInfo.outputState.renderTargetCount = 1;
		gpCreateInfo.outputState.renderTargetFormats[0] = GetRender().GetSwapChain().GetColorFormat();
		gpCreateInfo.outputState.depthStencilFormat = GetRender().GetSwapChain().GetDepthFormat();
		gpCreateInfo.pPipelineInterface = mPipelineInterface;
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

		mPerFrame.push_back(frame);
	}

	// Vertex buffer and geometry data
	{
		std::vector<float> vertexData = {
			// position          // vertex colors
			1.0f, 1.0f,-1.0f,   1.0f, 0.0f, 0.0f,  //  0  -Z side
			1.0f,-1.0f,-1.0f,   1.0f, 0.0f, 0.0f,  //  1
		   -1.0f,-1.0f,-1.0f,   1.0f, 0.0f, 0.0f,  //  2
		   -1.0f, 1.0f,-1.0f,   1.0f, 0.0f, 0.0f,  //  3

		   -1.0f, 1.0f, 1.0f,   0.0f, 1.0f, 0.0f,  //  4  +Z side
		   -1.0f,-1.0f, 1.0f,   0.0f, 1.0f, 0.0f,  //  5
			1.0f,-1.0f, 1.0f,   0.0f, 1.0f, 0.0f,  //  6
			1.0f, 1.0f, 1.0f,   0.0f, 1.0f, 0.0f,  //  7

		   -1.0f, 1.0f,-1.0f,   0.0f, 0.0f, 1.0f,  //  8  -X side
		   -1.0f,-1.0f,-1.0f,   0.0f, 0.0f, 1.0f,  //  9
		   -1.0f,-1.0f, 1.0f,   0.0f, 0.0f, 1.0f,  // 10
		   -1.0f, 1.0f, 1.0f,   0.0f, 0.0f, 1.0f,  // 11

			1.0f, 1.0f, 1.0f,   1.0f, 1.0f, 0.0f,  // 12  +X side
			1.0f,-1.0f, 1.0f,   1.0f, 1.0f, 0.0f,  // 13
			1.0f,-1.0f,-1.0f,   1.0f, 1.0f, 0.0f,  // 14
			1.0f, 1.0f,-1.0f,   1.0f, 1.0f, 0.0f,  // 15

		   -1.0f,-1.0f, 1.0f,   1.0f, 0.0f, 1.0f,  // 16  -Y side
		   -1.0f,-1.0f,-1.0f,   1.0f, 0.0f, 1.0f,  // 17
			1.0f,-1.0f,-1.0f,   1.0f, 0.0f, 1.0f,  // 18
			1.0f,-1.0f, 1.0f,   1.0f, 0.0f, 1.0f,  // 19

		   -1.0f, 1.0f,-1.0f,   0.0f, 1.0f, 1.0f,  // 20  +Y side
		   -1.0f, 1.0f, 1.0f,   0.0f, 1.0f, 1.0f,  // 21
			1.0f, 1.0f, 1.0f,   0.0f, 1.0f, 1.0f,  // 22
			1.0f, 1.0f,-1.0f,   0.0f, 1.0f, 1.0f,  // 23
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

	// Index buffer
	{
		// clang-format off
		std::vector<uint16_t> indexData = {
			0,  1,  2, // -Z side
			0,  2,  3,

			4,  5,  6, // +Z side
			4,  6,  7,

			8,  9, 10, // -X side
			8, 10, 11,

		   12, 13, 14, // +X side
		   12, 14, 15,

		   16, 17, 18, // -X side
		   16, 18, 19,

		   20, 21, 22, // +X side
		   20, 22, 23,
		};
		// clang-format on
		uint32_t dataSize = SizeInBytesU32(indexData);

		vkr::BufferCreateInfo bufferCreateInfo = {};
		bufferCreateInfo.size = dataSize;
		bufferCreateInfo.usageFlags.bits.indexBuffer = true;
		bufferCreateInfo.memoryUsage = vkr::MemoryUsage::CPUToGPU;

		CHECKED_CALL(device.CreateBuffer(bufferCreateInfo, &mIndexBuffer));

		void* pAddr = nullptr;
		CHECKED_CALL(mIndexBuffer->MapMemory(0, &pAddr));
		memcpy(pAddr, indexData.data(), dataSize);
		mIndexBuffer->UnmapMemory();
	}

	return true;
}

void Example_007::Shutdown()
{
	mPerFrame.clear();
	mVS.Reset();
	mPS.Reset();
	mPipelineInterface.Reset();
	mPipeline.Reset();
	mVertexBuffer.Reset();
	// TODO: доделать очистку
}

void Example_007::Update()
{
}

void Example_007::Render()
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

	// Update uniform buffer
	{
		//float    t = GetElapsedSeconds();
		static float t = 0.0;
		t += 0.001f;
		float4x4 P = glm::perspective(glm::radians(60.0f), GetWindowAspect(), 0.001f, 10000.0f);
		float4x4 V = glm::lookAt(float3(0, 0, 3), float3(0, 0, 0), float3(0, 1, 0));
		float4x4 M = glm::rotate(t, float3(0, 0, 1)) * glm::rotate(2 * t, float3(0, 1, 0)) * glm::rotate(t, float3(1, 0, 0));
		float4x4 mat = P * V * M;

		void* pData = nullptr;
		CHECKED_CALL(mUniformBuffer->MapMemory(0, &pData));
		memcpy(pData, &mat, sizeof(mat));
		mUniformBuffer->UnmapMemory();
	}
	
	// Build command buffer
	CHECKED_CALL(frame.cmd->Begin());
	{
		vkr::RenderPassPtr renderPass = swapChain.GetRenderPass(imageIndex);
		ASSERT_MSG(!renderPass.IsNull(), "render pass object is null");

		vkr::RenderPassBeginInfo beginInfo = {};
		beginInfo.pRenderPass = renderPass;
		beginInfo.renderArea = renderPass->GetRenderArea();
		beginInfo.RTVClearCount = 1;
		beginInfo.RTVClearValues[0] = { {0, 0, 0, 0} };
		beginInfo.DSVClearValue = { 1.0f, 0xFF };

		frame.cmd->TransitionImageLayout(renderPass->GetRenderTargetImage(0), ALL_SUBRESOURCES, vkr::ResourceState::Present, vkr::ResourceState::RenderTarget);
		frame.cmd->BeginRenderPass(&beginInfo);
		{
			frame.cmd->SetScissors(render.GetScissor());
			frame.cmd->SetViewports(render.GetViewport());
			frame.cmd->BindVertexBuffers(1, &mVertexBuffer, &mVertexBinding.GetStride());
			frame.cmd->BindGraphicsDescriptorSets(mPipelineInterface, 1, &mDescriptorSet);
			frame.cmd->BindGraphicsPipeline(mPipeline);
			frame.cmd->BindIndexBuffer(mIndexBuffer, vkr::IndexType::Uint16);
			frame.cmd->DrawIndexed(36, 1, 0, 0, 0);

			// Draw ImGui
			render.DrawDebugInfo();
			render.DrawImGui(frame.cmd);
		}
		frame.cmd->EndRenderPass();
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
}