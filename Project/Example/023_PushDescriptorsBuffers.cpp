#include "stdafx.h"
#include "023_PushDescriptorsBuffers.h"

struct DrawParams
{
	float4x4 MVP;
	uint     TextureIndex;
};

EngineApplicationCreateInfo Example_023::Config() const
{
	EngineApplicationCreateInfo createInfo{};
	createInfo.render.swapChain.depthFormat = vkr::Format::D32_FLOAT;
	createInfo.render.showImgui = false;
	return createInfo;
}

bool Example_023::Setup()
{
	auto& device = GetRenderDevice();

	// Uniform buffers
	for (uint32_t i = 0; i < 3; ++i) {
		vkr::BufferCreateInfo createInfo = {};
		createInfo.size = 256;
		createInfo.usageFlags.bits.uniformBuffer = true;
		createInfo.memoryUsage = vkr::MemoryUsage::CPUToGPU;

		CHECKED_CALL(device.CreateBuffer(createInfo, &mUniformBuffers[i]));
	}

	// Texture image, view, and sampler
	{
		// Image 0
		{
			const uint32_t textureIndex = 0;

			vkr::vkrUtil::ImageOptions options = vkr::vkrUtil::ImageOptions().MipLevelCount(RemainingMipLevels);
			CHECKED_CALL(vkr::vkrUtil::CreateImageFromFile(device.GetGraphicsQueue(), "basic/textures/box_panel.jpg", &mImages[textureIndex], options, true));

			vkr::SampledImageViewCreateInfo viewCreateInfo = vkr::SampledImageViewCreateInfo::GuessFromImage(mImages[textureIndex]);
			CHECKED_CALL(device.CreateSampledImageView(viewCreateInfo, &mSampledImageViews[textureIndex]));
		}
		// Image 1
		{
			const uint32_t textureIndex = 1;

			vkr::vkrUtil::ImageOptions options = vkr::vkrUtil::ImageOptions().MipLevelCount(RemainingMipLevels);
			CHECKED_CALL(vkr::vkrUtil::CreateImageFromFile(device.GetGraphicsQueue(), "basic/textures/chinatown.jpg", &mImages[textureIndex], options, true));

			vkr::SampledImageViewCreateInfo viewCreateInfo = vkr::SampledImageViewCreateInfo::GuessFromImage(mImages[textureIndex]);
			CHECKED_CALL(device.CreateSampledImageView(viewCreateInfo, &mSampledImageViews[textureIndex]));
		}
		// Image 2
		{
			const uint32_t textureIndex = 2;

			vkr::vkrUtil::ImageOptions options = vkr::vkrUtil::ImageOptions().MipLevelCount(RemainingMipLevels);
			CHECKED_CALL(vkr::vkrUtil::CreateImageFromFile(device.GetGraphicsQueue(), "basic/textures/hanging_lights.jpg", &mImages[textureIndex], options, true));

			vkr::SampledImageViewCreateInfo viewCreateInfo = vkr::SampledImageViewCreateInfo::GuessFromImage(mImages[textureIndex]);
			CHECKED_CALL(device.CreateSampledImageView(viewCreateInfo, &mSampledImageViews[textureIndex]));
		}

		// Sampler
		vkr::SamplerCreateInfo samplerCreateInfo = {};
		samplerCreateInfo.magFilter = vkr::Filter::Linear;
		samplerCreateInfo.minFilter = vkr::Filter::Linear;
		samplerCreateInfo.mipmapMode = vkr::SamplerMipmapMode::Linear;
		samplerCreateInfo.minLod = 0;
		samplerCreateInfo.maxLod = FLT_MAX;
		CHECKED_CALL(device.CreateSampler(samplerCreateInfo, &mSampler));
	}

	// Descriptor
	{
		vkr::DescriptorPoolCreateInfo poolCreateInfo = {};
		poolCreateInfo.uniformBuffer = 8;
		poolCreateInfo.sampledImage = 8;
		poolCreateInfo.sampler = 8;
		CHECKED_CALL(device.CreateDescriptorPool(poolCreateInfo, &mDescriptorPool));

		// Descriptor set layout for buffers
		{
			vkr::DescriptorSetLayoutCreateInfo layoutCreateInfo = {};
			layoutCreateInfo.flags.bits.pushable = true;
			layoutCreateInfo.bindings.push_back(vkr::DescriptorBinding(0, vkr::DescriptorType::UniformBuffer));
			CHECKED_CALL(device.CreateDescriptorSetLayout(layoutCreateInfo, &mDescriptorSetLayoutBuffers));
		}

		// Descriptor set layout for textures and samplers
		{
			vkr::DescriptorSetLayoutCreateInfo layoutCreateInfo = {};
			layoutCreateInfo.bindings.push_back(vkr::DescriptorBinding(1, vkr::DescriptorType::SampledImage, 3));
			layoutCreateInfo.bindings.push_back(vkr::DescriptorBinding(4, vkr::DescriptorType::Sampler));
			CHECKED_CALL(device.CreateDescriptorSetLayout(layoutCreateInfo, &mDescriptorSetLayout));
		}

		// Allocate descriptor set and write descriptors
		{
			CHECKED_CALL(device.AllocateDescriptorSet(mDescriptorPool, mDescriptorSetLayout, &mDescriptorSet));

			vkr::WriteDescriptor write = {};
			// Texture[0]
			write = {};
			write.binding = 1;
			write.arrayIndex = 0;
			write.type = vkr::DescriptorType::SampledImage;
			write.imageView = mSampledImageViews[0];
			CHECKED_CALL(mDescriptorSet->UpdateDescriptors(1, &write));
			// Texture[1]
			write = {};
			write.binding = 1;
			write.arrayIndex = 1;
			write.type = vkr::DescriptorType::SampledImage;
			write.imageView = mSampledImageViews[1];
			CHECKED_CALL(mDescriptorSet->UpdateDescriptors(1, &write));
			// Texture[2]
			write = {};
			write.binding = 1;
			write.arrayIndex = 2;
			write.type = vkr::DescriptorType::SampledImage;
			write.imageView = mSampledImageViews[2];
			CHECKED_CALL(mDescriptorSet->UpdateDescriptors(1, &write));

			// Sampler
			write = {};
			write.binding = 4;
			write.type = vkr::DescriptorType::Sampler;
			write.sampler = mSampler;
			CHECKED_CALL(mDescriptorSet->UpdateDescriptors(1, &write));
		}
	}

	// Pipeline
	{
		CHECKED_CALL(device.CreateShader("basic/shaders", "PushDescriptorsBuffersTexture.vs", &mVS));
		CHECKED_CALL(device.CreateShader("basic/shaders", "PushDescriptorsBuffersTexture.ps", &mPS));

		vkr::PipelineInterfaceCreateInfo piCreateInfo = {};
		piCreateInfo.setCount = 2;
		piCreateInfo.sets[0].set = 0;
		piCreateInfo.sets[0].layout = mDescriptorSetLayout;
		piCreateInfo.sets[1].set = 1;
		piCreateInfo.sets[1].layout = mDescriptorSetLayoutBuffers;
		CHECKED_CALL(device.CreatePipelineInterface(piCreateInfo, &mPipelineInterface));

		mVertexBinding.AppendAttribute({ "POSITION", 0, vkr::Format::R32G32B32_FLOAT, 0, APPEND_OFFSET_ALIGNED, vkr::VertexInputRate::Vertex });
		mVertexBinding.AppendAttribute({ "TEXCOORD", 1, vkr::Format::R32G32_FLOAT, 0, APPEND_OFFSET_ALIGNED, vkr::VertexInputRate::Vertex });

		vkr::GraphicsPipelineCreateInfo2 gpCreateInfo = {};
		gpCreateInfo.VS = { mVS.Get(), "vsmain" };
		gpCreateInfo.PS = { mPS.Get(), "psmain" };
		gpCreateInfo.vertexInputState.bindingCount = 1;
		gpCreateInfo.vertexInputState.bindings[0] = mVertexBinding;
		gpCreateInfo.topology = vkr::PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		gpCreateInfo.polygonMode = vkr::POLYGON_MODE_FILL;
		gpCreateInfo.cullMode = vkr::CULL_MODE_NONE;
		gpCreateInfo.frontFace = vkr::FRONT_FACE_CCW;
		gpCreateInfo.depthReadEnable = true;
		gpCreateInfo.depthWriteEnable = true;
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

		mPerFrame.push_back(frame);
	}

	// Vertex buffer and geometry data
	{
		// clang-format off
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
		// clang-format on
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

void Example_023::Shutdown()
{
	mPerFrame.clear();
	// TODO: доделать очистку
}

void Example_023::Update()
{
}

void Example_023::Render()
{
	auto& render = GetRender();
	auto& device = GetRenderDevice();
	auto& swapChain = render.GetSwapChain();
	PerFrame& frame = mPerFrame[0];

	// Wait for and reset render complete fence
	CHECKED_CALL(frame.renderCompleteFence->WaitAndReset());

	uint32_t imageIndex = UINT32_MAX;
	CHECKED_CALL(swapChain.AcquireNextImage(UINT64_MAX, frame.imageAcquiredSemaphore, frame.imageAcquiredFence, &imageIndex));

	// Wait for and reset image acquired fence
	CHECKED_CALL(frame.imageAcquiredFence->WaitAndReset());

	// Build command buffer
	CHECKED_CALL(frame.cmd->Begin());
	{
		vkr::RenderPassPtr renderPass = swapChain.GetRenderPass(imageIndex);
		ASSERT_MSG(!renderPass.IsNull(), "render pass object is null");

		vkr::RenderPassBeginInfo beginInfo = {};
		beginInfo.pRenderPass = renderPass;
		beginInfo.renderArea = renderPass->GetRenderArea();
		beginInfo.RTVClearCount = 1;
		beginInfo.RTVClearValues[0] = {0, 0, 0, 0};
		beginInfo.DSVClearValue = { 1.0f, 0xFF };

		frame.cmd->TransitionImageLayout(renderPass->GetRenderTargetImage(0), ALL_SUBRESOURCES, vkr::ResourceState::Present, vkr::ResourceState::RenderTarget);
		frame.cmd->BeginRenderPass(&beginInfo);
		{
			frame.cmd->SetScissors(render.GetScissor());
			frame.cmd->SetViewports(render.GetViewport());
			frame.cmd->BindGraphicsPipeline(mPipeline);
			frame.cmd->BindVertexBuffers(1, &mVertexBuffer, &mVertexBinding.GetStride());

			// Bind descriptor set
			frame.cmd->BindGraphicsDescriptorSets(mPipelineInterface, 1, &mDescriptorSet);

			// Get elapsed seconds
			//float t = GetElapsedSeconds();
			static float t = 0.0;
			t += 0.001f;

			// Calculate perspective and view matrices
			float4x4 P = glm::perspective(glm::radians(60.0f), GetWindowAspect(), 0.001f, 10000.0f);
			float4x4 V = glm::lookAt(float3(0, 0, 3), float3(0, 0, 0), float3(0, 1, 0));

			// Push descriptor vars to make things easier to read
			const uint32_t pushDescriptorsSetNumber = 1;

			// Draw center cube
			{
				// Calculate MVP
				float4x4 T = glm::translate(float3(0, 0, -10 * (1 + sin(t / 2))));
				float4x4 R = glm::rotate(t / 4, float3(0, 0, 1)) * glm::rotate(t / 4, float3(0, 1, 0)) * glm::rotate(t / 4, float3(1, 0, 0));
				float4x4 M = T * R;
				float4x4 mat = P * V * M;
				uint32_t textureIndex = 0;

				DrawParams drawParams = { mat, textureIndex };

				// Get uniform buffer at index 0
				auto pUniformBuffer = mUniformBuffers[0].Get();
				// Copy draw params
				pUniformBuffer->CopyFromSource(sizeof(DrawParams), &drawParams);
				// Push uniform buffer
				frame.cmd->PushGraphicsUniformBuffer(mPipelineInterface.Get(), 0, pushDescriptorsSetNumber, 0, pUniformBuffer);
			}
			frame.cmd->Draw(36, 1, 0, 0);

			// Draw left cube
			{
				// Calculate MVP
				float4x4   T = glm::translate(float3(-4, 0, -10 * (1 + sin(t / 2))));
				float4x4   R = glm::rotate(t / 4, float3(0, 0, 1)) * glm::rotate(t / 2, float3(0, 1, 0)) * glm::rotate(t / 4, float3(1, 0, 0));
				float4x4   M = T * R;
				float4x4   mat = P * V * M;
				uint32_t   textureIndex = 1;
				DrawParams drawParams = { mat, textureIndex };

				// Get uniform buffer at index 1
				auto pUniformBuffer = mUniformBuffers[1].Get();
				// Copy draw params
				pUniformBuffer->CopyFromSource(sizeof(DrawParams), &drawParams);
				// Push uniform buffer
				frame.cmd->PushGraphicsUniformBuffer(mPipelineInterface.Get(), 0, pushDescriptorsSetNumber, 0, pUniformBuffer);
			}
			frame.cmd->Draw(36, 1, 0, 0);

			// Draw right cube
			{
				// Calculate MVP
				float4x4   T = glm::translate(float3(4, 0, -10 * (1 + sin(t / 2))));
				float4x4   R = glm::rotate(t / 4, float3(0, 0, 1)) * glm::rotate(t, float3(0, 1, 0)) * glm::rotate(t / 4, float3(1, 0, 0));
				float4x4   M = T * R;
				float4x4   mat = P * V * M;
				uint32_t   textureIndex = 2;
				DrawParams drawParams = { mat, textureIndex };

				// Get uniform buffer at index 2
				auto pUniformBuffer = mUniformBuffers[2].Get();
				// Copy draw params
				pUniformBuffer->CopyFromSource(sizeof(DrawParams), &drawParams);
				// Push uniform buffer
				frame.cmd->PushGraphicsUniformBuffer(mPipelineInterface.Get(), 0, pushDescriptorsSetNumber, 0, pUniformBuffer);
			}
			frame.cmd->Draw(36, 1, 0, 0);

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

	CHECKED_CALL(device.GetGraphicsQueue()->Submit(&submitInfo));

	CHECKED_CALL(swapChain.Present(imageIndex, 1, &frame.renderCompleteSemaphore));
}