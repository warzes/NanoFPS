#include "002TriangleSpinning.h"

bool Example_002::Setup()
{
	auto& device = GetRenderDevice();

	// Uniform buffer
	// add new example
	{
		BufferCreateInfo bufferCreateInfo              = {};
		bufferCreateInfo.size                          = MINIMUM_UNIFORM_BUFFER_SIZE;
		bufferCreateInfo.usageFlags.bits.uniformBuffer = true;
		bufferCreateInfo.memoryUsage                   = MEMORY_USAGE_CPU_TO_GPU;

		CHECKED_CALL(device.CreateBuffer(bufferCreateInfo, &mUniformBuffer));
	}

	// Descriptor
	// add new example
	{
		DescriptorPoolCreateInfo poolCreateInfo = {};
		poolCreateInfo.uniformBuffer = 1;
		CHECKED_CALL(device.CreateDescriptorPool(poolCreateInfo, &mDescriptorPool));

		DescriptorSetLayoutCreateInfo layoutCreateInfo = {};
		layoutCreateInfo.bindings.push_back(DescriptorBinding{ 0, DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, SHADER_STAGE_ALL_GRAPHICS });
		CHECKED_CALL(device.CreateDescriptorSetLayout(layoutCreateInfo, &mDescriptorSetLayout));

		CHECKED_CALL(device.AllocateDescriptorSet(mDescriptorPool, mDescriptorSetLayout, &mDescriptorSet));

		WriteDescriptor write = {};
		write.binding = 0;
		write.type = DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		write.bufferOffset = 0;
		write.bufferRange = WHOLE_SIZE;
		write.pBuffer = mUniformBuffer;
		CHECKED_CALL(mDescriptorSet->UpdateDescriptors(1, &write));
	}

	// Pipeline
	{
		CHECKED_CALL(device.CreateShader("basic/shaders", "VertexColors.vs", &mVS));
		CHECKED_CALL(device.CreateShader("basic/shaders", "VertexColors.ps", &mPS));

		PipelineInterfaceCreateInfo piCreateInfo = {};
		piCreateInfo.setCount                    = 1; // add new example
		piCreateInfo.sets[0].set                 = 0;
		piCreateInfo.sets[0].pLayout             = mDescriptorSetLayout; // add new example
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
		gpCreateInfo.depthReadEnable                    = false;
		gpCreateInfo.depthWriteEnable                   = false;
		gpCreateInfo.blendModes[0]                      = BLEND_MODE_NONE;
		gpCreateInfo.outputState.renderTargetCount      = 1;
		gpCreateInfo.outputState.renderTargetFormats[0] = GetRender().GetSwapChain().GetColorFormat();
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

	// Vertex buffer and geometry data
	{
		std::vector<float> vertexData =
		{
			// position           // vertex colors
			 0.0f,  0.5f, 0.0f,   1.0f, 0.0f, 0.0f,
			-0.5f, -0.5f, 0.0f,   0.0f, 1.0f, 0.0f,
			 0.5f, -0.5f, 0.0f,   0.0f, 0.0f, 1.0f,
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

void Example_002::Shutdown()
{
	mPerFrame.clear();
	mVS.Reset();
	mPS.Reset();
	mPipelineInterface.Reset();
	mPipeline.Reset();
	mVertexBuffer.Reset();
}

void Example_002::Update()
{
}

void Example_002::Render()
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
	// add new example
	{
		//float    t = GetElapsedSeconds();
		static float t = 0.0;
		t += 0.001f;
		float4x4 mat = glm::rotate(t, float3(0, 0, 1));

		void* pData = nullptr;
		CHECKED_CALL(mUniformBuffer->MapMemory(0, &pData));
		memcpy(pData, &mat, sizeof(mat));
		mUniformBuffer->UnmapMemory();
	}

	// Build command buffer
	CHECKED_CALL(frame.cmd->Begin());
	{
		RenderPassPtr renderPass = swapChain.GetRenderPass(imageIndex);
		ASSERT_MSG(!renderPass.IsNull(), "render pass object is null");

		RenderPassBeginInfo beginInfo = {};
		beginInfo.pRenderPass         = renderPass;
		beginInfo.renderArea          = renderPass->GetRenderArea();
		beginInfo.RTVClearCount       = 1;
		beginInfo.RTVClearValues[0]   = { {0.9, 0.8, 0.3, 1} };

		frame.cmd->TransitionImageLayout(renderPass->GetRenderTargetImage(0), ALL_SUBRESOURCES, RESOURCE_STATE_PRESENT, RESOURCE_STATE_RENDER_TARGET);
		frame.cmd->BeginRenderPass(&beginInfo);
		{
			frame.cmd->SetScissors(render.GetScissor());
			frame.cmd->SetViewports(render.GetViewport());
			frame.cmd->BindGraphicsDescriptorSets(mPipelineInterface, 1, &mDescriptorSet);
			frame.cmd->BindGraphicsPipeline(mPipeline);
			frame.cmd->BindVertexBuffers(1, &mVertexBuffer, &mVertexBinding.GetStride());
			frame.cmd->Draw(3, 1, 0, 0);

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