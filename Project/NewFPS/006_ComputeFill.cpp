#include "stdafx.h"
#include "006_ComputeFill.h"

bool Example_006::Setup()
{
	auto& device = GetRenderDevice();

	// Uniform buffer
	{
		vkr::BufferCreateInfo bufferCreateInfo = {};
		bufferCreateInfo.size = vkr::MINIMUM_UNIFORM_BUFFER_SIZE;
		bufferCreateInfo.usageFlags.bits.uniformBuffer = true;
		bufferCreateInfo.memoryUsage = vkr::MEMORY_USAGE_CPU_TO_GPU;

		CHECKED_CALL(device.CreateBuffer(bufferCreateInfo, &mUniformBuffer));
	}

	// vkr::Texture image, view, and sampler
	{
		vkr::vkrUtil::ImageOptions imageOptions = vkr::vkrUtil::ImageOptions().AdditionalUsage(vkr::IMAGE_USAGE_STORAGE).MipLevelCount(1);
		CHECKED_CALL(vkr::vkrUtil::CreateImageFromFile(device.GetGraphicsQueue(), "basic/textures/box_panel.jpg", &mImage, imageOptions, false));

		vkr::SampledImageViewCreateInfo sampledViewCreateInfo = vkr::SampledImageViewCreateInfo::GuessFromImage(mImage);
		CHECKED_CALL(device.CreateSampledImageView(sampledViewCreateInfo, &mSampledImageView));

		vkr::StorageImageViewCreateInfo storageViewCreateInfo = vkr::StorageImageViewCreateInfo::GuessFromImage(mImage);
		CHECKED_CALL(device.CreateStorageImageView(storageViewCreateInfo, &mStorageImageView));

		vkr::SamplerCreateInfo samplerCreateInfo = {};
		CHECKED_CALL(device.CreateSampler(samplerCreateInfo, &mSampler));
	}

	// Descriptor
	{
		vkr::DescriptorPoolCreateInfo poolCreateInfo = {};
		poolCreateInfo.uniformBuffer = 1;
		poolCreateInfo.sampledImage = 1;
		poolCreateInfo.sampler = 1;
		poolCreateInfo.storageImage = 1;
		CHECKED_CALL(device.CreateDescriptorPool(poolCreateInfo, &mDescriptorPool));

		// Compute
		{
			vkr::DescriptorSetLayoutCreateInfo layoutCreateInfo = {};
			layoutCreateInfo.bindings.push_back(vkr::DescriptorBinding(0, vkr::DESCRIPTOR_TYPE_STORAGE_IMAGE));
			CHECKED_CALL(device.CreateDescriptorSetLayout(layoutCreateInfo, &mComputeDescriptorSetLayout));

			CHECKED_CALL(device.AllocateDescriptorSet(mDescriptorPool, mComputeDescriptorSetLayout, &mComputeDescriptorSet));

			vkr::WriteDescriptor write = {};
			write.binding = 0;
			write.type = vkr::DESCRIPTOR_TYPE_STORAGE_IMAGE;
			write.pImageView = mStorageImageView;
			CHECKED_CALL(mComputeDescriptorSet->UpdateDescriptors(1, &write));
		}

		// Graphics
		{
			vkr::DescriptorSetLayoutCreateInfo layoutCreateInfo = {};
			layoutCreateInfo.bindings.push_back(vkr::DescriptorBinding(0, vkr::DESCRIPTOR_TYPE_UNIFORM_BUFFER));
			layoutCreateInfo.bindings.push_back(vkr::DescriptorBinding(1, vkr::DESCRIPTOR_TYPE_SAMPLED_IMAGE));
			layoutCreateInfo.bindings.push_back(vkr::DescriptorBinding(2, vkr::DESCRIPTOR_TYPE_SAMPLER));
			CHECKED_CALL(device.CreateDescriptorSetLayout(layoutCreateInfo, &mGraphicsDescriptorSetLayout));

			CHECKED_CALL(device.AllocateDescriptorSet(mDescriptorPool, mGraphicsDescriptorSetLayout, &mGraphicsDescriptorSet));

			vkr::WriteDescriptor write = {};
			write.binding = 0;
			write.type = vkr::DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			write.bufferOffset = 0;
			write.bufferRange = WHOLE_SIZE;
			write.pBuffer = mUniformBuffer;
			CHECKED_CALL(mGraphicsDescriptorSet->UpdateDescriptors(1, &write));

			write = {};
			write.binding = 1;
			write.type = vkr::DESCRIPTOR_TYPE_SAMPLED_IMAGE;
			write.pImageView = mSampledImageView;
			CHECKED_CALL(mGraphicsDescriptorSet->UpdateDescriptors(1, &write));

			write = {};
			write.binding = 2;
			write.type = vkr::DESCRIPTOR_TYPE_SAMPLER;
			write.pSampler = mSampler;
			CHECKED_CALL(mGraphicsDescriptorSet->UpdateDescriptors(1, &write));
		}
	}

	// Compute pipeline
	{
		CHECKED_CALL(device.CreateShader("basic/shaders", "ComputeFill.cs", &mCS));

		vkr::PipelineInterfaceCreateInfo piCreateInfo = {};
		piCreateInfo.setCount = 1;
		piCreateInfo.sets[0].set = 0;
		piCreateInfo.sets[0].pLayout = mComputeDescriptorSetLayout;
		CHECKED_CALL(device.CreatePipelineInterface(piCreateInfo, &mComputePipelineInterface));

		vkr::ComputePipelineCreateInfo cpCreateInfo = {};
		cpCreateInfo.CS = { mCS.Get(), "csmain" };
		cpCreateInfo.pPipelineInterface = mComputePipelineInterface;
		CHECKED_CALL(device.CreateComputePipeline(cpCreateInfo, &mComputePipeline));
	}

	// Graphics pipeline
	{
		CHECKED_CALL(device.CreateShader("basic/shaders", "vkr::Texture.vs", &mVS));
		CHECKED_CALL(device.CreateShader("basic/shaders", "vkr::Texture.ps", &mPS));

		vkr::PipelineInterfaceCreateInfo piCreateInfo = {};
		piCreateInfo.setCount = 1;
		piCreateInfo.sets[0].set = 0;
		piCreateInfo.sets[0].pLayout = mGraphicsDescriptorSetLayout;
		CHECKED_CALL(device.CreatePipelineInterface(piCreateInfo, &mGraphicsPipelineInterface));

		mVertexBinding.AppendAttribute({ "POSITION", 0, vkr::FORMAT_R32G32B32_FLOAT, 0, APPEND_OFFSET_ALIGNED, vkr::VERTEX_INPUT_RATE_VERTEX });
		mVertexBinding.AppendAttribute({ "TEXCOORD", 1, vkr::FORMAT_R32G32_FLOAT, 0, APPEND_OFFSET_ALIGNED, vkr::VERTEX_INPUT_RATE_VERTEX });

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
		gpCreateInfo.pPipelineInterface = mGraphicsPipelineInterface;
		CHECKED_CALL(device.CreateGraphicsPipeline(gpCreateInfo, &mGraphicsPipeline));
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
			// position           // tex coords
		   -0.5f,  0.5f, 0.0f,   0.0f, 0.0f,
		   -0.5f, -0.5f, 0.0f,   0.0f, 1.0f,
			0.5f, -0.5f, 0.0f,   1.0f, 1.0f,

		   -0.5f,  0.5f, 0.0f,   0.0f, 0.0f,
			0.5f, -0.5f, 0.0f,   1.0f, 1.0f,
			0.5f,  0.5f, 0.0f,   1.0f, 0.0f,
		};
		uint32_t dataSize = SizeInBytesU32(vertexData);

		vkr::BufferCreateInfo bufferCreateInfo = {};
		bufferCreateInfo.size = dataSize;
		bufferCreateInfo.usageFlags.bits.vertexBuffer = true;
		bufferCreateInfo.memoryUsage = vkr::MEMORY_USAGE_CPU_TO_GPU;
		CHECKED_CALL(device.CreateBuffer(bufferCreateInfo, &mVertexBuffer));

		void* pAddr = nullptr;
		CHECKED_CALL(mVertexBuffer->MapMemory(0, &pAddr));
		memcpy(pAddr, vertexData.data(), dataSize);
		mVertexBuffer->UnmapMemory();
	}

	return true;
}

void Example_006::Shutdown()
{
	mPerFrame.clear();
	mVS.Reset();
	mPS.Reset();
	mVertexBuffer.Reset();
	// TODO: доделать очистку
}

void Example_006::Update()
{
}

void Example_006::Render()
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
		float4x4 mat = glm::rotate(t, float3(0, 0, 1));

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

		// Fill image with red
		frame.cmd->TransitionImageLayout(mImage, ALL_SUBRESOURCES, vkr::RESOURCE_STATE_SHADER_RESOURCE, vkr::RESOURCE_STATE_UNORDERED_ACCESS);
		frame.cmd->BindComputeDescriptorSets(mComputePipelineInterface, 1, &mComputeDescriptorSet);
		frame.cmd->BindComputePipeline(mComputePipeline);
		frame.cmd->Dispatch(mImage->GetWidth(), mImage->GetHeight(), 1);
		frame.cmd->TransitionImageLayout(mImage, ALL_SUBRESOURCES, vkr::RESOURCE_STATE_UNORDERED_ACCESS, vkr::RESOURCE_STATE_SHADER_RESOURCE);

		frame.cmd->TransitionImageLayout(renderPass->GetRenderTargetImage(0), ALL_SUBRESOURCES, vkr::RESOURCE_STATE_PRESENT, vkr::RESOURCE_STATE_RENDER_TARGET);
		frame.cmd->BeginRenderPass(&beginInfo);
		{
			// Draw texture
			frame.cmd->SetScissors(render.GetScissor());
			frame.cmd->SetViewports(render.GetViewport());
			frame.cmd->BindGraphicsDescriptorSets(mGraphicsPipelineInterface, 1, &mGraphicsDescriptorSet);
			frame.cmd->BindGraphicsPipeline(mGraphicsPipeline);
			frame.cmd->BindVertexBuffers(1, &mVertexBuffer, &mVertexBinding.GetStride());
			frame.cmd->Draw(6, 1, 0, 0);

			// Draw ImGui
			render.DrawDebugInfo();
			render.DrawImGui(frame.cmd);
		}
		frame.cmd->EndRenderPass();
		frame.cmd->TransitionImageLayout(renderPass->GetRenderTargetImage(0), ALL_SUBRESOURCES, vkr::RESOURCE_STATE_RENDER_TARGET, vkr::RESOURCE_STATE_PRESENT);
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