#include "stdafx.h"
#include "011_CompressedTexture.h"

EngineApplicationCreateInfo Example_011::Config() const
{
	EngineApplicationCreateInfo createInfo{};
	createInfo.render.swapChain.depthFormat = vkr::FORMAT_D32_FLOAT;
	return createInfo;
}

bool Example_011::Setup()
{
	auto& device = GetRenderDevice();

	// Uniform buffer
	int id = 1;
	for (const auto& texture : textures)
	{
		TexturedShape shape = {};

		vkr::BufferCreateInfo bufferCreateInfo = {};
		bufferCreateInfo.size = vkr::MINIMUM_UNIFORM_BUFFER_SIZE;
		bufferCreateInfo.usageFlags.bits.uniformBuffer = true;
		bufferCreateInfo.memoryUsage = vkr::MEMORY_USAGE_CPU_TO_GPU;

		CHECKED_CALL(device.CreateBuffer(bufferCreateInfo, &shape.uniformBuffer));

		// vkr::Texture image, view, and sampler
		{
			vkr::vkrUtil::ImageOptions options = vkr::vkrUtil::ImageOptions().MipLevelCount(vkr::RemainingMipLevels);
			CHECKED_CALL(vkr::vkrUtil::CreateImageFromFile(device.GetGraphicsQueue(), texture.texturePath, &shape.image));

			vkr::SampledImageViewCreateInfo viewCreateInfo = vkr::SampledImageViewCreateInfo::GuessFromImage(shape.image);
			CHECKED_CALL(device.CreateSampledImageView(viewCreateInfo, &shape.sampledImageView));

			vkr::SamplerCreateInfo samplerCreateInfo = {};
			samplerCreateInfo.magFilter = vkr::FILTER_LINEAR;
			samplerCreateInfo.minFilter = vkr::FILTER_LINEAR;
			samplerCreateInfo.mipmapMode = vkr::SAMPLER_MIPMAP_MODE_LINEAR;
			samplerCreateInfo.minLod = 0;
			samplerCreateInfo.maxLod = FLT_MAX;
			CHECKED_CALL(device.CreateSampler(samplerCreateInfo, &shape.sampler));
		}

		shape.homeLoc = texture.homeLoc;
		shape.id = id++;
		mShapes.push_back(shape);
	}

	// Descriptor
	{
		vkr::DescriptorPoolCreateInfo poolCreateInfo = {};
		poolCreateInfo.uniformBuffer = static_cast<uint32_t>(textures.size());
		poolCreateInfo.sampledImage = static_cast<uint32_t>(textures.size());
		poolCreateInfo.sampler = static_cast<uint32_t>(textures.size());
		CHECKED_CALL(device.CreateDescriptorPool(poolCreateInfo, &mDescriptorPool));

		vkr::DescriptorSetLayoutCreateInfo layoutCreateInfo = {};
		layoutCreateInfo.bindings.push_back(vkr::DescriptorBinding(0, vkr::DESCRIPTOR_TYPE_UNIFORM_BUFFER));
		layoutCreateInfo.bindings.push_back(vkr::DescriptorBinding(1, vkr::DESCRIPTOR_TYPE_SAMPLED_IMAGE));
		layoutCreateInfo.bindings.push_back(vkr::DescriptorBinding(2, vkr::DESCRIPTOR_TYPE_SAMPLER));
		CHECKED_CALL(device.CreateDescriptorSetLayout(layoutCreateInfo, &mDescriptorSetLayout));

		for (auto& shape : mShapes)
		{
			CHECKED_CALL(device.AllocateDescriptorSet(mDescriptorPool, mDescriptorSetLayout, &shape.descriptorSet));

			vkr::WriteDescriptor write = {};
			write.binding = 0;
			write.type = vkr::DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			write.bufferOffset = 0;
			write.bufferRange = WHOLE_SIZE;
			write.pBuffer = shape.uniformBuffer;
			CHECKED_CALL(shape.descriptorSet->UpdateDescriptors(1, &write));

			write = {};
			write.binding = 1;
			write.type = vkr::DESCRIPTOR_TYPE_SAMPLED_IMAGE;
			write.pImageView = shape.sampledImageView;
			CHECKED_CALL(shape.descriptorSet->UpdateDescriptors(1, &write));

			write = {};
			write.binding = 2;
			write.type = vkr::DESCRIPTOR_TYPE_SAMPLER;
			write.pSampler = shape.sampler;
			CHECKED_CALL(shape.descriptorSet->UpdateDescriptors(1, &write));
		}
	}

	// Pipeline
	{
		CHECKED_CALL(device.CreateShader("basic/shaders", "vkr::Texture.vs", &mVS));
		CHECKED_CALL(device.CreateShader("basic/shaders", "vkr::Texture.ps", &mPS));

		vkr::PipelineInterfaceCreateInfo piCreateInfo = {};
		piCreateInfo.setCount = 1;
		piCreateInfo.sets[0].set = 0;
		piCreateInfo.sets[0].pLayout = mDescriptorSetLayout;
		CHECKED_CALL(device.CreatePipelineInterface(piCreateInfo, &mPipelineInterface));

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
		bufferCreateInfo.memoryUsage = vkr::MEMORY_USAGE_CPU_TO_GPU;

		CHECKED_CALL(device.CreateBuffer(bufferCreateInfo, &mVertexBuffer));

		void* pAddr = nullptr;
		CHECKED_CALL(mVertexBuffer->MapMemory(0, &pAddr));
		memcpy(pAddr, vertexData.data(), dataSize);
		mVertexBuffer->UnmapMemory();
	}

	return true;
}

void Example_011::Shutdown()
{
	mPerFrame.clear();
	mVS.Reset();
	mPS.Reset();
	mPipelineInterface.Reset();

	// TODO: доделать очистку
}

void Example_011::Update()
{
}

void Example_011::Render()
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
	for (auto& shape : mShapes)
	{
		//float    t = GetElapsedSeconds();
		static float t = 0.0;
		t += 0.001f;
		float4x4 P = glm::perspective(glm::radians(60.0f), GetWindowAspect(), 0.001f, 10000.0f);
		float4x4 V = glm::lookAt(float3(0, 0, 3), float3(0, 0, 0), float3(0, 1, 0));
		float4x4 T = glm::translate(float3(shape.homeLoc[0], shape.homeLoc[1], -5 + sin(shape.id * t / 2))); // * sin(t / 2)));
		float4x4 R = glm::rotate(shape.id + t, float3(shape.id * t, 0, 0)) * glm::rotate(shape.id + t / 4, float3(0, shape.id * t, 0)) * glm::rotate(shape.id + t / 4, float3(0, 0, shape.id * t));
		float4x4 M = T * R;
		float4x4 mat = P * V * M;

		void* pData = nullptr;
		CHECKED_CALL(shape.uniformBuffer->MapMemory(0, &pData));
		memcpy(pData, &mat, sizeof(mat));
		shape.uniformBuffer->UnmapMemory();
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

		frame.cmd->TransitionImageLayout(renderPass->GetRenderTargetImage(0), ALL_SUBRESOURCES, vkr::RESOURCE_STATE_PRESENT, vkr::RESOURCE_STATE_RENDER_TARGET);
		frame.cmd->BeginRenderPass(&beginInfo);
		{
			frame.cmd->SetScissors(render.GetScissor());
			frame.cmd->SetViewports(render.GetViewport());

			frame.cmd->BindGraphicsPipeline(mPipeline);
			frame.cmd->BindVertexBuffers(1, &mVertexBuffer, &mVertexBinding.GetStride());

			for (auto& shape : mShapes)
			{
				frame.cmd->BindGraphicsDescriptorSets(mPipelineInterface, 1, &shape.descriptorSet);
				frame.cmd->Draw(36, 1, 0, 0);
			}

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