#include "stdafx.h"
#include "005_CubeTextured.h"

EngineApplicationCreateInfo Example_005::Config() const
{
	EngineApplicationCreateInfo createInfo{};
	createInfo.render.swapChain.depthFormat = FORMAT_D32_FLOAT;
	return createInfo;
}

bool Example_005::Setup()
{
	auto& device = GetRenderDevice();

	// Uniform buffer
	for (uint32_t i = 0; i < 3; ++i)
	{
		BufferCreateInfo bufferCreateInfo = {};
		bufferCreateInfo.size = MINIMUM_UNIFORM_BUFFER_SIZE;
		bufferCreateInfo.usageFlags.bits.uniformBuffer = true;
		bufferCreateInfo.memoryUsage = MEMORY_USAGE_CPU_TO_GPU;

		CHECKED_CALL(device.CreateBuffer(bufferCreateInfo, &mUniformBuffer[i]));
	}

	// Texture image, view, and sampler
	{
		grfx_util::ImageOptions options = grfx_util::ImageOptions().MipLevelCount(REMAINING_MIP_LEVELS);
		CHECKED_CALL(grfx_util::CreateImageFromFile(device.GetGraphicsQueue(), "basic/textures/box_panel.jpg", &mImage, options, true));

		SampledImageViewCreateInfo viewCreateInfo = SampledImageViewCreateInfo::GuessFromImage(mImage);
		CHECKED_CALL(device.CreateSampledImageView(viewCreateInfo, &mSampledImageView));

		SamplerCreateInfo samplerCreateInfo = {};
		samplerCreateInfo.magFilter = FILTER_LINEAR;
		samplerCreateInfo.minFilter = FILTER_LINEAR;
		samplerCreateInfo.mipmapMode = SAMPLER_MIPMAP_MODE_LINEAR;
		samplerCreateInfo.minLod = 0;
		samplerCreateInfo.maxLod = FLT_MAX;
		CHECKED_CALL(device.CreateSampler(samplerCreateInfo, &mSampler));
	}

	// Descriptor
	{
		DescriptorPoolCreateInfo poolCreateInfo = {};
		poolCreateInfo.uniformBuffer = 8;
		poolCreateInfo.sampledImage = 8;
		poolCreateInfo.sampler = 8;
		CHECKED_CALL(device.CreateDescriptorPool(poolCreateInfo, &mDescriptorPool));

		DescriptorSetLayoutCreateInfo layoutCreateInfo = {};
		layoutCreateInfo.bindings.push_back(DescriptorBinding(0, DESCRIPTOR_TYPE_UNIFORM_BUFFER));
		layoutCreateInfo.bindings.push_back(DescriptorBinding(1, DESCRIPTOR_TYPE_SAMPLED_IMAGE));
		layoutCreateInfo.bindings.push_back(DescriptorBinding(2, DESCRIPTOR_TYPE_SAMPLER));
		CHECKED_CALL(device.CreateDescriptorSetLayout(layoutCreateInfo, &mDescriptorSetLayout));

		for (uint32_t i = 0; i < 3; ++i)
		{
			CHECKED_CALL(device.AllocateDescriptorSet(mDescriptorPool, mDescriptorSetLayout, &mDescriptorSet[i]));

			WriteDescriptor write = {};
			write.binding = 0;
			write.type = DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			write.bufferOffset = 0;
			write.bufferRange = WHOLE_SIZE;
			write.pBuffer = mUniformBuffer[i];
			CHECKED_CALL(mDescriptorSet[i]->UpdateDescriptors(1, &write));

			write = {};
			write.binding = 1;
			write.type = DESCRIPTOR_TYPE_SAMPLED_IMAGE;
			write.pImageView = mSampledImageView;
			CHECKED_CALL(mDescriptorSet[i]->UpdateDescriptors(1, &write));

			write = {};
			write.binding = 2;
			write.type = DESCRIPTOR_TYPE_SAMPLER;
			write.pSampler = mSampler;
			CHECKED_CALL(mDescriptorSet[i]->UpdateDescriptors(1, &write));
		}
	}

	// Pipeline
	{
		CHECKED_CALL(device.CreateShader("basic/shaders", "Texture.vs", &mVS));
		CHECKED_CALL(device.CreateShader("basic/shaders", "Texture.ps", &mPS));

		PipelineInterfaceCreateInfo piCreateInfo = {};
		piCreateInfo.setCount = 1;
		piCreateInfo.sets[0].set = 0;
		piCreateInfo.sets[0].pLayout = mDescriptorSetLayout;
		CHECKED_CALL(device.CreatePipelineInterface(piCreateInfo, &mPipelineInterface));

		mVertexBinding.AppendAttribute({ "POSITION", 0, FORMAT_R32G32B32_FLOAT, 0, APPEND_OFFSET_ALIGNED, VERTEX_INPUT_RATE_VERTEX });
		mVertexBinding.AppendAttribute({ "TEXCOORD", 1, FORMAT_R32G32_FLOAT, 0, APPEND_OFFSET_ALIGNED, VERTEX_INPUT_RATE_VERTEX });

		GraphicsPipelineCreateInfo2 gpCreateInfo = {};
		gpCreateInfo.VS = { mVS.Get(), "vsmain" };
		gpCreateInfo.PS = { mPS.Get(), "psmain" };
		gpCreateInfo.vertexInputState.bindingCount = 1;
		gpCreateInfo.vertexInputState.bindings[0] = mVertexBinding;
		gpCreateInfo.topology = PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		gpCreateInfo.polygonMode = POLYGON_MODE_FILL;
		gpCreateInfo.cullMode = CULL_MODE_NONE;
		gpCreateInfo.frontFace = FRONT_FACE_CCW;
		gpCreateInfo.depthReadEnable = true;
		gpCreateInfo.depthWriteEnable = true;
		gpCreateInfo.blendModes[0] = BLEND_MODE_NONE;
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
		std::vector<float> vertexData = {
			 -1.0f,-1.0f,-1.0f,   1.0f, 1.0f,  // -Z side
			 1.0f, 1.0f,-1.0f,   0.0f, 0.0f,
			 1.0f,-1.0f,-1.0f,   0.0f, 1.0f,
			-1.0f,-1.0f,-1.0f,   1.0f, 1.0f,
			-1.0f, 1.0f,-1.0f,   1.0f, 0.0f,
			 1.0f, 1.0f,-1.0f,   0.0f, 0.0f,

			-1.0f, 1.0f, 1.0f,   0.0f, 0.0f,  // +Z side
			-1.0f,-1.0f, 1.0f,   0.0f, 1.0f,
			 1.0f, 1.0f, 1.0f,   1.0f, 0.0f,
			-1.0f,-1.0f, 1.0f,   0.0f, 1.0f,
			 1.0f,-1.0f, 1.0f,   1.0f, 1.0f,
			 1.0f, 1.0f, 1.0f,   1.0f, 0.0f,

			-1.0f,-1.0f,-1.0f,   0.0f, 1.0f,  // -X side
			-1.0f,-1.0f, 1.0f,   1.0f, 1.0f,
			-1.0f, 1.0f, 1.0f,   1.0f, 0.0f,
			-1.0f, 1.0f, 1.0f,   1.0f, 0.0f,
			-1.0f, 1.0f,-1.0f,   0.0f, 0.0f,
			-1.0f,-1.0f,-1.0f,   0.0f, 1.0f,

			 1.0f, 1.0f,-1.0f,   0.0f, 1.0f,  // +X side
			 1.0f, 1.0f, 1.0f,   1.0f, 1.0f,
			 1.0f,-1.0f, 1.0f,   1.0f, 0.0f,
			 1.0f,-1.0f, 1.0f,   1.0f, 0.0f,
			 1.0f,-1.0f,-1.0f,   0.0f, 0.0f,
			 1.0f, 1.0f,-1.0f,   0.0f, 1.0f,

			-1.0f,-1.0f,-1.0f,   1.0f, 0.0f,  // -Y side
			 1.0f,-1.0f,-1.0f,   1.0f, 1.0f,
			 1.0f,-1.0f, 1.0f,   0.0f, 1.0f,
			-1.0f,-1.0f,-1.0f,   1.0f, 0.0f,
			 1.0f,-1.0f, 1.0f,   0.0f, 1.0f,
			-1.0f,-1.0f, 1.0f,   0.0f, 0.0f,

			-1.0f, 1.0f,-1.0f,   1.0f, 0.0f,  // +Y side
			-1.0f, 1.0f, 1.0f,   0.0f, 0.0f,
			 1.0f, 1.0f, 1.0f,   0.0f, 1.0f,
			-1.0f, 1.0f,-1.0f,   1.0f, 0.0f,
			 1.0f, 1.0f, 1.0f,   0.0f, 1.0f,
			 1.0f, 1.0f,-1.0f,   1.0f, 1.0f,
		};
		uint32_t dataSize = SizeInBytesU32(vertexData);

		BufferCreateInfo bufferCreateInfo = {};
		bufferCreateInfo.size = dataSize;
		bufferCreateInfo.usageFlags.bits.vertexBuffer = true;
		bufferCreateInfo.memoryUsage = MEMORY_USAGE_CPU_TO_GPU;
		CHECKED_CALL(device.CreateBuffer(bufferCreateInfo, &mVertexBuffer));

		void* pAddr = nullptr;
		CHECKED_CALL(mVertexBuffer->MapMemory(0, &pAddr));
		memcpy(pAddr, vertexData.data(), dataSize);
		mVertexBuffer->UnmapMemory();
	}

	return true;
}

void Example_005::Shutdown()
{
	mPerFrame.clear();
	mVS.Reset();
	mPS.Reset();
	mPipelineInterface.Reset();
	mPipeline.Reset();
	mVertexBuffer.Reset();
	// TODO: доделать очистку
}

void Example_005::Update()
{
}

void Example_005::Render()
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
		float4x4 T = glm::translate(float3(0, 0, -10 * (1 + sin(t / 2))));
		float4x4 R = glm::rotate(t / 4, float3(0, 0, 1)) * glm::rotate(t / 4, float3(0, 1, 0)) * glm::rotate(t / 4, float3(1, 0, 0));
		float4x4 M = T * R;
		float4x4 mat = P * V * M;

		void* pData = nullptr;
		CHECKED_CALL(mUniformBuffer[0]->MapMemory(0, &pData));
		memcpy(pData, &mat, sizeof(mat));
		mUniformBuffer[0]->UnmapMemory();
	}
	// Update uniform buffer
	{
		//float    t = GetElapsedSeconds();
		static float t = 0.0;
		t += 0.001f;
		float4x4 P = glm::perspective(glm::radians(60.0f), GetWindowAspect(), 0.001f, 10000.0f);
		float4x4 V = glm::lookAt(float3(0, 0, 3), float3(0, 0, 0), float3(0, 1, 0));
		float4x4 T = glm::translate(float3(-4, 0, -10 * (1 + sin(t / 2))));
		float4x4 R = glm::rotate(t / 4, float3(0, 0, 1)) * glm::rotate(t / 2, float3(0, 1, 0)) * glm::rotate(t / 4, float3(1, 0, 0));
		float4x4 M = T * R;
		float4x4 mat = P * V * M;

		void* pData = nullptr;
		CHECKED_CALL(mUniformBuffer[1]->MapMemory(0, &pData));
		memcpy(pData, &mat, sizeof(mat));
		mUniformBuffer[1]->UnmapMemory();
	}

	// Update uniform buffer
	{
		//float    t = GetElapsedSeconds();
		static float t = 0.0;
		t += 0.001f;
		float4x4 P = glm::perspective(glm::radians(60.0f), GetWindowAspect(), 0.001f, 10000.0f);
		float4x4 V = glm::lookAt(float3(0, 0, 3), float3(0, 0, 0), float3(0, 1, 0));
		float4x4 T = glm::translate(float3(4, 0, -10 * (1 + sin(t / 2))));
		float4x4 R = glm::rotate(t / 4, float3(0, 0, 1)) * glm::rotate(t, float3(0, 1, 0)) * glm::rotate(t / 4, float3(1, 0, 0));
		float4x4 M = T * R;
		float4x4 mat = P * V * M;

		void* pData = nullptr;
		CHECKED_CALL(mUniformBuffer[2]->MapMemory(0, &pData));
		memcpy(pData, &mat, sizeof(mat));
		mUniformBuffer[2]->UnmapMemory();
	}

	// Build command buffer
	CHECKED_CALL(frame.cmd->Begin());
	{
		RenderPassPtr renderPass = swapChain.GetRenderPass(imageIndex);
		ASSERT_MSG(!renderPass.IsNull(), "render pass object is null");

		RenderPassBeginInfo beginInfo = {};
		beginInfo.pRenderPass = renderPass;
		beginInfo.renderArea = renderPass->GetRenderArea();
		beginInfo.RTVClearCount = 1;
		beginInfo.RTVClearValues[0] = { {0, 0, 0, 0} };
		beginInfo.DSVClearValue = { 1.0f, 0xFF };

		frame.cmd->TransitionImageLayout(renderPass->GetRenderTargetImage(0), ALL_SUBRESOURCES, RESOURCE_STATE_PRESENT, RESOURCE_STATE_RENDER_TARGET);
		frame.cmd->BeginRenderPass(&beginInfo);
		{
			frame.cmd->SetScissors(render.GetScissor());
			frame.cmd->SetViewports(render.GetViewport());
			frame.cmd->BindGraphicsPipeline(mPipeline);
			frame.cmd->BindVertexBuffers(1, &mVertexBuffer, &mVertexBinding.GetStride());

			frame.cmd->BindGraphicsDescriptorSets(mPipelineInterface, 1, &mDescriptorSet[0]);
			frame.cmd->Draw(36, 1, 0, 0);

			frame.cmd->BindGraphicsDescriptorSets(mPipelineInterface, 1, &mDescriptorSet[1]);
			frame.cmd->Draw(36, 1, 0, 0);

			frame.cmd->BindGraphicsDescriptorSets(mPipelineInterface, 1, &mDescriptorSet[2]);
			frame.cmd->Draw(36, 1, 0, 0);

			// Draw ImGui
			render.DrawDebugInfo();
			render.DrawImGui(frame.cmd);
		}
		frame.cmd->EndRenderPass();
		frame.cmd->TransitionImageLayout(renderPass->GetRenderTargetImage(0), ALL_SUBRESOURCES, RESOURCE_STATE_RENDER_TARGET, RESOURCE_STATE_PRESENT);
	}
	CHECKED_CALL(frame.cmd->End());

	SubmitInfo submitInfo = {};
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