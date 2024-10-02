#include "stdafx.h"
#include "027_Mipmap.h"

EngineApplicationCreateInfo Example_027::Config() const
{
	EngineApplicationCreateInfo createInfo{};
	createInfo.render.swapChain.depthFormat = vkr::Format::D32_FLOAT;
	createInfo.render.showImgui = true;
	return createInfo;
}

bool Example_027::Setup()
{
	auto& device = GetRenderDevice();

	// Uniform buffer
	for (uint32_t i = 0; i < 2; ++i) {
		vkr::BufferCreateInfo bufferCreateInfo = {};
		bufferCreateInfo.size = vkr::MINIMUM_UNIFORM_BUFFER_SIZE;
		bufferCreateInfo.usageFlags.bits.uniformBuffer = true;
		bufferCreateInfo.memoryUsage = vkr::MemoryUsage::CPUToGPU;

		CHECKED_CALL(device.CreateBuffer(bufferCreateInfo, &mUniformBuffer[i]));
	}

	// Texture image, view, and sampler
	{
		// std::vector<std::string> textureFiles = {"box_panel.jpg", "statue.jpg"};
		for (uint32_t i = 0; i < 2; ++i) {
			vkr::vkrUtil::ImageOptions options = vkr::vkrUtil::ImageOptions().MipLevelCount(RemainingMipLevels);
			CHECKED_CALL(vkr::vkrUtil::CreateImageFromFile(device.GetGraphicsQueue(), "basic/textures/hanging_lights.jpg", &mImage[i], options, i == 1));

			vkr::SampledImageViewCreateInfo viewCreateInfo = vkr::SampledImageViewCreateInfo::GuessFromImage(mImage[i]);
			CHECKED_CALL(device.CreateSampledImageView(viewCreateInfo, &mSampledImageView[i]));
		}

		// Query available mip levels from the images
		mMaxLevelLeft = mSampledImageView[0]->GetMipLevelCount() - 1; // Since first level is zero
		mMaxLevelRight = mSampledImageView[1]->GetMipLevelCount() - 1;
		mLevelLeft = 0;
		mLevelRight = 0;
		mLeftInGpu = false;
		mRightInGpu = true;
		// To better perceive the MipMap we disable the interpolation between them
		vkr::SamplerCreateInfo samplerCreateInfo = {};
		samplerCreateInfo.magFilter = vkr::Filter::Nearest;
		samplerCreateInfo.minFilter = vkr::Filter::Nearest;
		samplerCreateInfo.mipmapMode = vkr::SamplerMipmapMode::Nearest;
		samplerCreateInfo.minLod = 0;
		samplerCreateInfo.maxLod = FLT_MAX;
		CHECKED_CALL(device.CreateSampler(samplerCreateInfo, &mSampler));
	}

	// Descriptor
	{
		vkr::DescriptorPoolCreateInfo poolCreateInfo = {};
		poolCreateInfo.uniformBuffer = 2;
		poolCreateInfo.sampledImage = 2;
		poolCreateInfo.sampler = 2;
		CHECKED_CALL(device.CreateDescriptorPool(poolCreateInfo, &mDescriptorPool));

		vkr::DescriptorSetLayoutCreateInfo layoutCreateInfo = {};
		layoutCreateInfo.bindings.push_back(vkr::DescriptorBinding(0, vkr::DescriptorType::UniformBuffer));
		layoutCreateInfo.bindings.push_back(vkr::DescriptorBinding(1, vkr::DescriptorType::SampledImage));
		layoutCreateInfo.bindings.push_back(vkr::DescriptorBinding(2, vkr::DescriptorType::Sampler));
		CHECKED_CALL(device.CreateDescriptorSetLayout(layoutCreateInfo, &mDescriptorSetLayout));

		for (uint32_t i = 0; i < 2; ++i) {
			CHECKED_CALL(device.AllocateDescriptorSet(mDescriptorPool, mDescriptorSetLayout, &mDescriptorSet[i]));

			vkr::WriteDescriptor write = {};
			write.binding = 0;
			write.type = vkr::DescriptorType::UniformBuffer;
			write.bufferOffset = 0;
			write.bufferRange = WHOLE_SIZE;
			write.buffer = mUniformBuffer[i];
			CHECKED_CALL(mDescriptorSet[i]->UpdateDescriptors(1, &write));

			write = {};
			write.binding = 1;
			write.type = vkr::DescriptorType::SampledImage;
			write.imageView = mSampledImageView[i];
			CHECKED_CALL(mDescriptorSet[i]->UpdateDescriptors(1, &write));

			write = {};
			write.binding = 2;
			write.type = vkr::DescriptorType::Sampler;
			write.sampler = mSampler;
			CHECKED_CALL(mDescriptorSet[i]->UpdateDescriptors(1, &write));
		}
	}

	// Pipeline
	{
		CHECKED_CALL(device.CreateShader("basic/shaders", "TextureMip.vs", &mVS));
		CHECKED_CALL(device.CreateShader("basic/shaders", "TextureMip.ps", &mPS));

		vkr::PipelineInterfaceCreateInfo piCreateInfo = {};
		piCreateInfo.setCount = 1;
		piCreateInfo.sets[0].set = 0;
		piCreateInfo.sets[0].layout = mDescriptorSetLayout;
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

		-1.0f, 1.0f, 1.0f,   0.0f, 0.0f,  // +Z side
		-1.0f,-1.0f, 1.0f,   0.0f, 1.0f,
		1.0f, 1.0f, 1.0f,   1.0f, 0.0f,
		-1.0f,-1.0f, 1.0f,   0.0f, 1.0f,
		1.0f,-1.0f, 1.0f,   1.0f, 1.0f,
		1.0f, 1.0f, 1.0f,   1.0f, 0.0f,

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

void Example_027::Shutdown()
{
	mPerFrame.clear();
	// TODO: доделать очистку
}

void Example_027::Update()
{
}

void Example_027::Render()
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
	float4x4 P = glm::perspective(glm::radians(60.0f), GetWindowAspect(), 1.0f, 4.0f);
	float4x4 V = glm::lookAt(float3(0, 0, 3.1), float3(0, 0, 0), float3(0, 1, 0));
	struct alignas(16) InputData
	{
		float4x4 M;
		int      mipLevel;
	};
	{
		InputData uniBuffer;
		float4x4  M = glm::translate(float3(-1.05, 0, 0));
		uniBuffer.M = P * V * M;
		uniBuffer.mipLevel = mLevelLeft;

		void* pData = nullptr;
		CHECKED_CALL(mUniformBuffer[0]->MapMemory(0, &pData));
		memcpy(pData, &uniBuffer, sizeof(InputData));
		mUniformBuffer[0]->UnmapMemory();
	}

	// Update uniform buffer
	{
		InputData uniBuffer;
		float4x4  M = glm::translate(float3(1.05, 0, 0));
		uniBuffer.M = P * V * M;
		uniBuffer.mipLevel = mLevelRight;

		void* pData = nullptr;
		CHECKED_CALL(mUniformBuffer[1]->MapMemory(0, &pData));
		memcpy(pData, &uniBuffer, sizeof(InputData));
		mUniformBuffer[1]->UnmapMemory();
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
		beginInfo.RTVClearValues[0] = {0, 0, 0, 0};
		beginInfo.DSVClearValue = { 1.0f, 0xFF };

		frame.cmd->TransitionImageLayout(renderPass->GetRenderTargetImage(0), ALL_SUBRESOURCES, vkr::ResourceState::Present, vkr::ResourceState::RenderTarget);
		frame.cmd->BeginRenderPass(&beginInfo);
		{
			frame.cmd->SetScissors(render.GetScissor());
			frame.cmd->SetViewports(render.GetViewport());
			frame.cmd->BindGraphicsPipeline(mPipeline);
			frame.cmd->BindVertexBuffers(1, &mVertexBuffer, &mVertexBinding.GetStride());

			frame.cmd->BindGraphicsDescriptorSets(mPipelineInterface, 1, &mDescriptorSet[0]);
			frame.cmd->Draw(6, 1, 0, 0);

			frame.cmd->BindGraphicsDescriptorSets(mPipelineInterface, 1, &mDescriptorSet[1]);
			frame.cmd->Draw(6, 1, 0, 0);

			// Draw ImGui
			render.DrawDebugInfo();
			{
				ImGui::Separator();
				ImGui::Text("Left generated in:");
				ImGui::SameLine();
				const auto lightBlue = ImVec4(0.0f, 0.8f, 1.0f, 1.0f);
				ImGui::TextColored(lightBlue, mLeftInGpu ? "GPU" : "CPU");
				ImGui::Text("Right generated in:");
				ImGui::SameLine();
				ImGui::TextColored(lightBlue, mRightInGpu ? "GPU" : "CPU");

				ImGui::Text("Mip Map Level");
				ImGui::SliderInt("Left", &mLevelLeft, 0, mMaxLevelLeft);
				ImGui::SliderInt("Right", &mLevelRight, 0, mMaxLevelRight);

				static const char* currentFilter = mFilterNames[0];

				if (ImGui::BeginCombo("Filter", currentFilter)) {
					for (size_t i = 0; i < mFilterNames.size(); ++i) {
						bool isSelected = (currentFilter == mFilterNames[i]);
						if (ImGui::Selectable(mFilterNames[i], isSelected)) {
							currentFilter = mFilterNames[i];
							mFilterOption = static_cast<int>(i);
						}
						if (isSelected) {
							ImGui::SetItemDefaultFocus();
						}
					}
					ImGui::EndCombo();
				}
			}
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

	CHECKED_CALL(GetRenderDevice().GetGraphicsQueue()->Submit(&submitInfo));

	CHECKED_CALL(swapChain.Present(imageIndex, 1, &frame.renderCompleteSemaphore));
}