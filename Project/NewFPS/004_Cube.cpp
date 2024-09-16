#include "004_Cube.h"

EngineApplicationCreateInfo Example_004::Config() const
{
	EngineApplicationCreateInfo createInfo{};
	createInfo.render.swapChain.depthFormat = FORMAT_D32_FLOAT;
	return createInfo;
}

bool Example_004::Setup()
{
	auto& device = GetRenderDevice();

	// Uniform buffer
	{
		BufferCreateInfo bufferCreateInfo              = {};
		bufferCreateInfo.size                          = MINIMUM_UNIFORM_BUFFER_SIZE;
		bufferCreateInfo.usageFlags.bits.uniformBuffer = true;
		bufferCreateInfo.memoryUsage                   = MEMORY_USAGE_CPU_TO_GPU;
		CHECKED_CALL(device.CreateBuffer(bufferCreateInfo, &mUniformBuffer));
	}

	// Descriptor
	{
		DescriptorPoolCreateInfo poolCreateInfo = {};
		poolCreateInfo.uniformBuffer            = 1;
		CHECKED_CALL(device.CreateDescriptorPool(poolCreateInfo, &mDescriptorPool));

		DescriptorSetLayoutCreateInfo layoutCreateInfo = {};
		layoutCreateInfo.bindings.push_back(DescriptorBinding{ 0, DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, SHADER_STAGE_ALL_GRAPHICS });
		CHECKED_CALL(device.CreateDescriptorSetLayout(layoutCreateInfo, &mDescriptorSetLayout));

		CHECKED_CALL(device.AllocateDescriptorSet(mDescriptorPool, mDescriptorSetLayout, &mDescriptorSet));

		WriteDescriptor write = {};
		write.binding         = 0;
		write.type            = DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		write.bufferOffset    = 0;
		write.bufferRange     = WHOLE_SIZE;
		write.pBuffer         = mUniformBuffer;
		CHECKED_CALL(mDescriptorSet->UpdateDescriptors(1, &write));
	}

	// Pipeline
	{
		CHECKED_CALL(device.CreateShader("basic/shaders", "VertexColors.vs", &mVS));
		CHECKED_CALL(device.CreateShader("basic/shaders", "VertexColors.ps", &mPS));

		PipelineInterfaceCreateInfo piCreateInfo = {};
		piCreateInfo.setCount                    = 1;
		piCreateInfo.sets[0].set                 = 0;
		piCreateInfo.sets[0].pLayout             = mDescriptorSetLayout;
		CHECKED_CALL(device.CreatePipelineInterface(piCreateInfo, &mPipelineInterface));

		mVertexBinding.AppendAttribute({ "POSITION", 0, FORMAT_R32G32B32_FLOAT, 0, APPEND_OFFSET_ALIGNED, VERTEX_INPUT_RATE_VERTEX });
		mVertexBinding.AppendAttribute({ "COLOR", 1, FORMAT_R32G32B32_FLOAT, 0, APPEND_OFFSET_ALIGNED, VERTEX_INPUT_RATE_VERTEX });

		GraphicsPipelineCreateInfo2 gpCreateInfo        = {};
		gpCreateInfo.VS                                 = { mVS.Get(), "vsmain" };
		gpCreateInfo.PS                                 = { mPS.Get(), "psmain" };
		gpCreateInfo.vertexInputState.bindingCount      = 1;
		gpCreateInfo.vertexInputState.bindings[0]       = mVertexBinding;
		gpCreateInfo.topology                           = PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		gpCreateInfo.polygonMode                        = POLYGON_MODE_FILL;
		gpCreateInfo.cullMode                           = CULL_MODE_NONE;
		gpCreateInfo.frontFace                          = FRONT_FACE_CCW;
		gpCreateInfo.depthReadEnable                    = true;
		gpCreateInfo.depthWriteEnable                   = true;
		gpCreateInfo.blendModes[0]                      = BLEND_MODE_NONE;
		gpCreateInfo.outputState.renderTargetCount      = 1;
		gpCreateInfo.outputState.renderTargetFormats[0] = GetRender().GetSwapChain().GetColorFormat();
		gpCreateInfo.outputState.depthStencilFormat     = GetRender().GetSwapChain().GetDepthFormat();
		gpCreateInfo.pPipelineInterface                 = mPipelineInterface;
		CHECKED_CALL(device.CreateGraphicsPipeline(gpCreateInfo, &mPipeline));
	}

	// Per frame data
	{
		PerFrame frame = {};

		CHECKED_CALL(device.GetGraphicsQueue()->CreateCommandBuffer(&frame.cmd));

		SemaphoreCreateInfo semaCreateInfo = {};
		CHECKED_CALL(device.CreateSemaphore(semaCreateInfo, &frame.imageAcquiredSemaphore));

		FenceCreateInfo fenceCreateInfo = {};
		CHECKED_CALL(device.CreateFence(fenceCreateInfo, &frame.imageAcquiredFence));

		CHECKED_CALL(device.CreateSemaphore(semaCreateInfo, &frame.renderCompleteSemaphore));

		fenceCreateInfo = { true }; // Create signaled
		CHECKED_CALL(device.CreateFence(fenceCreateInfo, &frame.renderCompleteFence));

		mPerFrame.push_back(frame);
	}

	// Render pass
	{
		for (uint32_t i = 0; i < GetRender().GetSwapChain().GetImageCount(); ++i)
		{
			auto pRenderTargetImage = GetRender().GetSwapChain().GetColorImage(i);
			auto pDepthStencilimage = GetRender().GetSwapChain().GetDepthImage(i);

			// Explicitly use OP_LOAD for all attachments
			RenderPassCreateInfo3 createInfo  = {};
			createInfo.width                  = pRenderTargetImage->GetWidth();
			createInfo.height                 = pRenderTargetImage->GetHeight();
			createInfo.renderTargetCount      = 1;
			createInfo.pRenderTargetImages[0] = pRenderTargetImage;
			createInfo.renderTargetLoadOps[0] = ATTACHMENT_LOAD_OP_LOAD;
			createInfo.pDepthStencilImage     = pDepthStencilimage;
			createInfo.depthLoadOp            = ATTACHMENT_LOAD_OP_LOAD;
			createInfo.stencilLoadOp          = ATTACHMENT_LOAD_OP_LOAD;

			RenderPassPtr renderPass = nullptr;
			CHECKED_CALL(device.CreateRenderPass(createInfo, &renderPass));

			mRenderPasses.push_back(renderPass);
		}
	}

	// Vertex buffer and geometry data
	{
		std::vector<float> vertexData = {
			// position          // vertex colors
			-1.0f,-1.0f,-1.0f,   1.0f, 0.0f, 0.0f,  // -Z side
			 1.0f, 1.0f,-1.0f,   1.0f, 0.0f, 0.0f,
			 1.0f,-1.0f,-1.0f,   1.0f, 0.0f, 0.0f,
			-1.0f,-1.0f,-1.0f,   1.0f, 0.0f, 0.0f,
			-1.0f, 1.0f,-1.0f,   1.0f, 0.0f, 0.0f,
			 1.0f, 1.0f,-1.0f,   1.0f, 0.0f, 0.0f,

			-1.0f, 1.0f, 1.0f,   0.0f, 1.0f, 0.0f,  // +Z side
			-1.0f,-1.0f, 1.0f,   0.0f, 1.0f, 0.0f,
			 1.0f, 1.0f, 1.0f,   0.0f, 1.0f, 0.0f,
			-1.0f,-1.0f, 1.0f,   0.0f, 1.0f, 0.0f,
			 1.0f,-1.0f, 1.0f,   0.0f, 1.0f, 0.0f,
			 1.0f, 1.0f, 1.0f,   0.0f, 1.0f, 0.0f,

			-1.0f,-1.0f,-1.0f,   0.0f, 0.0f, 1.0f,  // -X side
			-1.0f,-1.0f, 1.0f,   0.0f, 0.0f, 1.0f,
			-1.0f, 1.0f, 1.0f,   0.0f, 0.0f, 1.0f,
			-1.0f, 1.0f, 1.0f,   0.0f, 0.0f, 1.0f,
			-1.0f, 1.0f,-1.0f,   0.0f, 0.0f, 1.0f,
			-1.0f,-1.0f,-1.0f,   0.0f, 0.0f, 1.0f,

			 1.0f, 1.0f,-1.0f,   1.0f, 1.0f, 0.0f,  // +X side
			 1.0f, 1.0f, 1.0f,   1.0f, 1.0f, 0.0f,
			 1.0f,-1.0f, 1.0f,   1.0f, 1.0f, 0.0f,
			 1.0f,-1.0f, 1.0f,   1.0f, 1.0f, 0.0f,
			 1.0f,-1.0f,-1.0f,   1.0f, 1.0f, 0.0f,
			 1.0f, 1.0f,-1.0f,   1.0f, 1.0f, 0.0f,

			-1.0f,-1.0f,-1.0f,   1.0f, 0.0f, 1.0f,  // -Y side
			 1.0f,-1.0f,-1.0f,   1.0f, 0.0f, 1.0f,
			 1.0f,-1.0f, 1.0f,   1.0f, 0.0f, 1.0f,
			-1.0f,-1.0f,-1.0f,   1.0f, 0.0f, 1.0f,
			 1.0f,-1.0f, 1.0f,   1.0f, 0.0f, 1.0f,
			-1.0f,-1.0f, 1.0f,   1.0f, 0.0f, 1.0f,

			-1.0f, 1.0f,-1.0f,   0.0f, 1.0f, 1.0f,  // +Y side
			-1.0f, 1.0f, 1.0f,   0.0f, 1.0f, 1.0f,
			 1.0f, 1.0f, 1.0f,   0.0f, 1.0f, 1.0f,
			-1.0f, 1.0f,-1.0f,   0.0f, 1.0f, 1.0f,
			 1.0f, 1.0f, 1.0f,   0.0f, 1.0f, 1.0f,
			 1.0f, 1.0f,-1.0f,   0.0f, 1.0f, 1.0f,
		};
		uint32_t dataSize = SizeInBytesU32(vertexData);

		BufferCreateInfo bufferCreateInfo             = {};
		bufferCreateInfo.size                         = dataSize;
		bufferCreateInfo.usageFlags.bits.vertexBuffer = true;
		bufferCreateInfo.memoryUsage                  = MEMORY_USAGE_CPU_TO_GPU;
		CHECKED_CALL(device.CreateBuffer(bufferCreateInfo, &mVertexBuffer));

		void* pAddr = nullptr;
		CHECKED_CALL(mVertexBuffer->MapMemory(0, &pAddr));
		memcpy(pAddr, vertexData.data(), dataSize);
		mVertexBuffer->UnmapMemory();
	}

	return true;
}

void Example_004::Shutdown()
{
	mPerFrame.clear();
	mVS.Reset();
	mPS.Reset();
	mPipelineInterface.Reset();
	mPipeline.Reset();
	mVertexBuffer.Reset();
}

void Example_004::Update()
{
}

void Example_004::Render()
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
		float4x4 M = glm::rotate(t, float3(0, 0, 1)) * glm::rotate(t, float3(0, 1, 0)) * glm::rotate(t, float3(1, 0, 0));
		float4x4 mat = P * V * M;

		void* pData = nullptr;
		CHECKED_CALL(mUniformBuffer->MapMemory(0, &pData));
		memcpy(pData, &mat, sizeof(mat));
		mUniformBuffer->UnmapMemory();
	}

	// Build command buffer
	CHECKED_CALL(frame.cmd->Begin());
	{
		RenderPassPtr renderPass = mRenderPasses[imageIndex];
		ASSERT_MSG(!renderPass.IsNull(), "render pass object is null");

		RenderPassBeginInfo beginInfo = {};
		beginInfo.pRenderPass         = renderPass;
		beginInfo.renderArea          = renderPass->GetRenderArea();

		// Clear RTV to greyish blue
		RenderTargetClearValue rtvClearValue = { 0.23f, 0.23f, 0.33f, 0 };
		DepthStencilClearValue dsvClearValue = { 1.0f, 0xFF };

		frame.cmd->TransitionImageLayout(renderPass->GetRenderTargetImage(0), ALL_SUBRESOURCES, RESOURCE_STATE_PRESENT, RESOURCE_STATE_RENDER_TARGET);
		frame.cmd->BeginRenderPass(&beginInfo);
		{
			frame.cmd->ClearRenderTarget(renderPass->GetRenderTargetImage(0), rtvClearValue);
			frame.cmd->ClearDepthStencil(renderPass->GetDepthStencilImage(), dsvClearValue, CLEAR_FLAG_DEPTH);
			frame.cmd->SetScissors(render.GetScissor());
			frame.cmd->SetViewports(render.GetViewport());
			frame.cmd->BindGraphicsDescriptorSets(mPipelineInterface, 1, &mDescriptorSet);
			frame.cmd->BindGraphicsPipeline(mPipeline);
			frame.cmd->BindVertexBuffers(1, &mVertexBuffer, &mVertexBinding.GetStride());
			frame.cmd->Draw(36, 1, 0, 0);

			// Draw ImGui
			render.DrawDebugInfo();
			render.DrawImGui(frame.cmd);
		}
		frame.cmd->EndRenderPass();
		frame.cmd->TransitionImageLayout(renderPass->GetRenderTargetImage(0), ALL_SUBRESOURCES, RESOURCE_STATE_RENDER_TARGET, RESOURCE_STATE_PRESENT);
	}
	CHECKED_CALL(frame.cmd->End());

	SubmitInfo submitInfo           = {};
	submitInfo.commandBufferCount   = 1;
	submitInfo.ppCommandBuffers     = &frame.cmd;
	submitInfo.waitSemaphoreCount   = 1;
	submitInfo.ppWaitSemaphores     = &frame.imageAcquiredSemaphore;
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.ppSignalSemaphores   = &frame.renderCompleteSemaphore;
	submitInfo.pFence               = frame.renderCompleteFence;

	CHECKED_CALL(render.GetGraphicsQueue()->Submit(&submitInfo));

	CHECKED_CALL(swapChain.Present(imageIndex, 1, &frame.renderCompleteSemaphore));
}