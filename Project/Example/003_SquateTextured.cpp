#include "stdafx.h"
#include "003_SquateTextured.h"

bool Example_003::Setup()
{
	auto& device = GetRenderDevice();

	// Uniform buffer
	{
		vkr::BufferCreateInfo bufferCreateInfo              = {};
		bufferCreateInfo.size                          = vkr::MINIMUM_UNIFORM_BUFFER_SIZE;
		bufferCreateInfo.usageFlags.bits.uniformBuffer = true;
		bufferCreateInfo.memoryUsage                   = vkr::MemoryUsage::CPUToGPU;
		CHECKED_CALL(device.CreateBuffer(bufferCreateInfo, &mUniformBuffer));
	}

	// vkr::Texture image, view, and sampler
	{
		vkr::vkrUtil::ImageOptions options = vkr::vkrUtil::ImageOptions().MipLevelCount(RemainingMipLevels);
		CHECKED_CALL(vkr::vkrUtil::CreateImageFromFile(device.GetGraphicsQueue(), "basic/textures/box_panel.jpg", &mImage, options, false));

		vkr::SampledImageViewCreateInfo viewCreateInfo = vkr::SampledImageViewCreateInfo::GuessFromImage(mImage);
		CHECKED_CALL(device.CreateSampledImageView(viewCreateInfo, &mSampledImageView));

		vkr::SamplerCreateInfo samplerCreateInfo = {};
		CHECKED_CALL(device.CreateSampler(samplerCreateInfo, &mSampler));
	}

	// Descriptor
	{
		vkr::DescriptorPoolCreateInfo poolCreateInfo = {};
		poolCreateInfo.uniformBuffer            = 1;
		poolCreateInfo.sampledImage             = 1;
		poolCreateInfo.sampler                  = 1;
		CHECKED_CALL(device.CreateDescriptorPool(poolCreateInfo, &mDescriptorPool));

		vkr::DescriptorSetLayoutCreateInfo layoutCreateInfo = {};
		layoutCreateInfo.bindings.push_back(vkr::DescriptorBinding{ 0, vkr::DescriptorType::UniformBuffer, 1, vkr::SHADER_STAGE_ALL_GRAPHICS });
		layoutCreateInfo.bindings.push_back(vkr::DescriptorBinding(1, vkr::DescriptorType::SampledImage));
		layoutCreateInfo.bindings.push_back(vkr::DescriptorBinding(2, vkr::DescriptorType::Sampler));
		CHECKED_CALL(device.CreateDescriptorSetLayout(layoutCreateInfo, &mDescriptorSetLayout));

		CHECKED_CALL(device.AllocateDescriptorSet(mDescriptorPool, mDescriptorSetLayout, &mDescriptorSet));

		vkr::WriteDescriptor write = {};
		write.binding         = 0;
		write.type            = vkr::DescriptorType::UniformBuffer;
		write.bufferOffset    = 0;
		write.bufferRange     = WHOLE_SIZE;
		write.buffer         = mUniformBuffer;
		CHECKED_CALL(mDescriptorSet->UpdateDescriptors(1, &write));

		write                 = {};
		write.binding         = 1;
		write.type            = vkr::DescriptorType::SampledImage;
		write.imageView      = mSampledImageView;
		CHECKED_CALL(mDescriptorSet->UpdateDescriptors(1, &write));

		write                 = {};
		write.binding         = 2;
		write.type            = vkr::DescriptorType::Sampler;
		write.sampler        = mSampler;
		CHECKED_CALL(mDescriptorSet->UpdateDescriptors(1, &write));
	}

	// Pipeline
	{
		CHECKED_CALL(device.CreateShader("basic/shaders", "Texture.vs", &mVS));
		CHECKED_CALL(device.CreateShader("basic/shaders", "Texture.ps", &mPS));

		vkr::PipelineInterfaceCreateInfo piCreateInfo = {};
		piCreateInfo.setCount                    = 1;
		piCreateInfo.sets[0].set                 = 0;
		piCreateInfo.sets[0].layout             = mDescriptorSetLayout;
		CHECKED_CALL(device.CreatePipelineInterface(piCreateInfo, &mPipelineInterface));

		mVertexBinding.AppendAttribute({ "POSITION", 0, vkr::Format::R32G32B32_FLOAT, 0, APPEND_OFFSET_ALIGNED, vkr::VertexInputRate::Vertex });
		mVertexBinding.AppendAttribute({ "TEXCOORD", 1, vkr::Format::R32G32_FLOAT, 0, APPEND_OFFSET_ALIGNED, vkr::VertexInputRate::Vertex });

		vkr::GraphicsPipelineCreateInfo2 gpCreateInfo        = {};
		gpCreateInfo.VS                                 = { mVS.Get(), "vsmain" };
		gpCreateInfo.PS                                 = { mPS.Get(), "psmain" };
		gpCreateInfo.vertexInputState.bindingCount      = 1;
		gpCreateInfo.vertexInputState.bindings[0]       = mVertexBinding;
		gpCreateInfo.topology                           = vkr::PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		gpCreateInfo.polygonMode                        = vkr::POLYGON_MODE_FILL;
		gpCreateInfo.cullMode                           = vkr::CULL_MODE_NONE;
		gpCreateInfo.frontFace                          = vkr::FRONT_FACE_CCW;
		gpCreateInfo.depthReadEnable                    = false;
		gpCreateInfo.depthWriteEnable                   = false;
		gpCreateInfo.blendModes[0]                      = vkr::BLEND_MODE_NONE;
		gpCreateInfo.outputState.renderTargetCount      = 1;
		gpCreateInfo.outputState.renderTargetFormats[0] = GetRender().GetSwapChain().GetColorFormat();
		gpCreateInfo.pipelineInterface                 = mPipelineInterface;
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
		std::vector<float> vertexData = 
		{
			// position           // tex coords
			-0.5f,  0.5f, 0.0f,   0.0f, 0.0f,
			-0.5f, -0.5f, 0.0f,   0.0f, 1.0f,
			 0.5f, -0.5f, 0.0f,   1.0f, 1.0f,

			-0.5f,  0.5f, 0.0f,   0.0f, 0.0f,
			 0.5f, -0.5f, 0.0f,   1.0f, 1.0f,
			 0.5f,  0.5f, 0.0f,   1.0f, 0.0f,
		};
		uint32_t dataSize = SizeInBytesU32(vertexData);

		vkr::BufferCreateInfo bufferCreateInfo             = {};
		bufferCreateInfo.size                         = dataSize;
		bufferCreateInfo.usageFlags.bits.vertexBuffer = true;
		bufferCreateInfo.memoryUsage                  = vkr::MemoryUsage::CPUToGPU;
		CHECKED_CALL(device.CreateBuffer(bufferCreateInfo, &mVertexBuffer));

		void* pAddr = nullptr;
		CHECKED_CALL(mVertexBuffer->MapMemory(0, &pAddr));
		memcpy(pAddr, vertexData.data(), dataSize);
		mVertexBuffer->UnmapMemory();
	}

	return true;
}

void Example_003::Shutdown()
{
	mPerFrame.clear();
	mVS.Reset();
	mPS.Reset();
	mPipelineInterface.Reset();
	mPipeline.Reset();
	mVertexBuffer.Reset();
	// TODO: доделать очистку
}

void Example_003::Update()
{
}

void Example_003::Render()
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
		beginInfo.pRenderPass         = renderPass;
		beginInfo.renderArea          = renderPass->GetRenderArea();
		beginInfo.RTVClearCount       = 1;
		beginInfo.RTVClearValues[0]   = {0.9, 0.8, 0.3, 1};

		frame.cmd->TransitionImageLayout(renderPass->GetRenderTargetImage(0), ALL_SUBRESOURCES, vkr::ResourceState::Present, vkr::ResourceState::RenderTarget);
		frame.cmd->BeginRenderPass(&beginInfo);
		{
			frame.cmd->SetScissors(render.GetScissor());
			frame.cmd->SetViewports(render.GetViewport());
			frame.cmd->BindGraphicsDescriptorSets(mPipelineInterface, 1, &mDescriptorSet);
			frame.cmd->BindGraphicsPipeline(mPipeline);
			frame.cmd->BindVertexBuffers(1, &mVertexBuffer, &mVertexBinding.GetStride());
			frame.cmd->Draw(6, 1, 0, 0);

			// Draw ImGui
			render.DrawDebugInfo();
			render.DrawImGui(frame.cmd);
		}
		frame.cmd->EndRenderPass();
		frame.cmd->TransitionImageLayout(renderPass->GetRenderTargetImage(0), ALL_SUBRESOURCES, vkr::ResourceState::RenderTarget, vkr::ResourceState::Present);
	}
	CHECKED_CALL(frame.cmd->End());

	vkr::SubmitInfo submitInfo           = {};
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