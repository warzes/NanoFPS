#include "stdafx.h"
#include "017_AsyncCompute.h"

EngineApplicationCreateInfo Example_017::Config() const
{
	EngineApplicationCreateInfo createInfo{};
	createInfo.render.swapChain.imageCount = mNumFramesInFlight;
	createInfo.render.showImgui = true;
	return createInfo;
}

bool Example_017::Setup()
{
	auto& device = GetRenderDevice();

	mAsyncComputeEnabled = true;
	mUseQueueFamilyTransfers = true;

	mCamera = PerspCamera(60.0f, GetWindowAspect());

	mGraphicsQueue = device.GetGraphicsQueue();
	mComputeQueue = mAsyncComputeEnabled ? device.GetComputeQueue() : mGraphicsQueue;

	// Per frame data
	for (uint32_t i = 0; i < mNumFramesInFlight; ++i)
	{
		PerFrame                  frame = {};
		vkr::SemaphoreCreateInfo semaCreateInfo = {};

		for (uint32_t d = 0; d < frame.renderData.size(); ++d) {
			CHECKED_CALL(mGraphicsQueue->CreateCommandBuffer(&frame.renderData[d].cmd));
			CHECKED_CALL(mGraphicsQueue->CreateCommandBuffer(&frame.composeData[d].cmd));
			CHECKED_CALL(mComputeQueue->CreateCommandBuffer(&frame.computeData[d].cmd));

			CHECKED_CALL(device.CreateSemaphore(semaCreateInfo, &frame.renderData[d].completeSemaphore));
			CHECKED_CALL(device.CreateSemaphore(semaCreateInfo, &frame.computeData[d].completeSemaphore));
			CHECKED_CALL(device.CreateSemaphore(semaCreateInfo, &frame.composeData[d].completeSemaphore));
		}

		// Use the graphics queue for drawing to the swapchain.
		CHECKED_CALL(mGraphicsQueue->CreateCommandBuffer(&frame.drawToSwapchainData.cmd));

		vkr::FenceCreateInfo fenceCreateInfo = {};
		CHECKED_CALL(device.CreateFence(fenceCreateInfo, &frame.imageAcquiredFence));
		fenceCreateInfo = { true }; // Create signaled
		CHECKED_CALL(device.CreateFence(fenceCreateInfo, &frame.renderCompleteFence));

		CHECKED_CALL(device.CreateSemaphore(semaCreateInfo, &frame.imageAcquiredSemaphore));
		CHECKED_CALL(device.CreateSemaphore(semaCreateInfo, &frame.renderCompleteSemaphore));

		mPerFrame.push_back(frame);
	}

	// Descriptor pool
	{
		vkr::DescriptorPoolCreateInfo createInfo = {};
		createInfo.sampler = 200;
		createInfo.sampledImage = 200;
		createInfo.uniformBuffer = 200;
		createInfo.storageImage = 200;

		CHECKED_CALL(device.CreateDescriptorPool(createInfo, &mDescriptorPool));
	}

	// Descriptor layout for graphics pipeline (vkr::Texture.hlsl)
	{
		vkr::DescriptorSetLayoutCreateInfo layoutCreateInfo = {};
		layoutCreateInfo.bindings.push_back(vkr::DescriptorBinding(0, vkr::DESCRIPTOR_TYPE_UNIFORM_BUFFER));
		layoutCreateInfo.bindings.push_back(vkr::DescriptorBinding(1, vkr::DESCRIPTOR_TYPE_SAMPLED_IMAGE));
		layoutCreateInfo.bindings.push_back(vkr::DescriptorBinding(2, vkr::DESCRIPTOR_TYPE_SAMPLER));
		CHECKED_CALL(device.CreateDescriptorSetLayout(layoutCreateInfo, &mRenderLayout));
	}

	// vkr::Mesh
	{
		vkr::Geometry geo;
		vkr::TriMesh  mesh = vkr::TriMesh::CreateFromOBJ("basic/models/altimeter/altimeter.obj", vkr::TriMeshOptions().Indices().TexCoords().Scale(float3(1.5f)));
		CHECKED_CALL(vkr::Geometry::Create(mesh, &geo));
		CHECKED_CALL(vkr::vkrUtil::CreateMeshFromGeometry(device.GetGraphicsQueue(), &geo, &mModelMesh));
	}

	// vkr::Texture.
	{
		vkr::vkrUtil::TextureOptions options = vkr::vkrUtil::TextureOptions().MipLevelCount(RemainingMipLevels);
		CHECKED_CALL(vkr::vkrUtil::CreateTextureFromFile(device.GetGraphicsQueue(), "materials/textures/altimeter/albedo.jpg", &mModelTexture, options));
	}

	// Samplers.
	{
		vkr::SamplerCreateInfo samplerCreateInfo = {};
		samplerCreateInfo.magFilter = vkr::Filter::Linear;
		samplerCreateInfo.minFilter = vkr::Filter::Linear;
		samplerCreateInfo.mipmapMode = vkr::SamplerMipmapMode::Linear;
		samplerCreateInfo.minLod = 0.0f;
		samplerCreateInfo.maxLod = FLT_MAX;
		CHECKED_CALL(device.CreateSampler(samplerCreateInfo, &mLinearSampler));

		samplerCreateInfo.magFilter = vkr::Filter::Nearest;
		samplerCreateInfo.minFilter = vkr::Filter::Nearest;
		samplerCreateInfo.mipmapMode = vkr::SamplerMipmapMode::Nearest;
		CHECKED_CALL(device.CreateSampler(samplerCreateInfo, &mNearestSampler));
	}

	// Pipeline for graphics rendering.
	{
		vkr::ShaderModulePtr VS;
		CHECKED_CALL(device.CreateShader("basic/shaders", "vkr::Texture.vs", &VS));
		vkr::ShaderModulePtr PS;
		CHECKED_CALL(device.CreateShader("basic/shaders", "vkr::Texture.ps", &PS));

		vkr::PipelineInterfaceCreateInfo piCreateInfo = {};
		piCreateInfo.setCount = 1;
		piCreateInfo.sets[0].set = 0;
		piCreateInfo.sets[0].pLayout = mRenderLayout;
		CHECKED_CALL(device.CreatePipelineInterface(piCreateInfo, &mRenderPipelineInterface));

		vkr::GraphicsPipelineCreateInfo2 gpCreateInfo = {};
		gpCreateInfo.VS = { VS.Get(), "vsmain" };
		gpCreateInfo.PS = { PS.Get(), "psmain" };
		gpCreateInfo.vertexInputState.bindingCount = 2;
		gpCreateInfo.vertexInputState.bindings[0] = mModelMesh->GetDerivedVertexBindings()[0];
		gpCreateInfo.vertexInputState.bindings[1] = mModelMesh->GetDerivedVertexBindings()[1];
		gpCreateInfo.topology = vkr::PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		gpCreateInfo.polygonMode = vkr::POLYGON_MODE_FILL;
		gpCreateInfo.cullMode = vkr::CULL_MODE_BACK;
		gpCreateInfo.frontFace = vkr::FRONT_FACE_CCW;
		gpCreateInfo.depthReadEnable = true;
		gpCreateInfo.depthWriteEnable = true;
		gpCreateInfo.blendModes[0] = vkr::BLEND_MODE_NONE;
		gpCreateInfo.outputState.renderTargetCount = 1;
		gpCreateInfo.outputState.renderTargetFormats[0] = GetRender().GetSwapChain().GetColorFormat();
		gpCreateInfo.outputState.depthStencilFormat = vkr::FORMAT_D32_FLOAT;
		gpCreateInfo.pPipelineInterface = mRenderPipelineInterface;
		CHECKED_CALL(device.CreateGraphicsPipeline(gpCreateInfo, &mRenderPipeline));

		device.DestroyShaderModule(VS);
		device.DestroyShaderModule(PS);
	}

	for (auto& frameData : mPerFrame)
	{
		for (auto& renderData : frameData.renderData)
		{
			// Descriptor set.
			{
				CHECKED_CALL(device.AllocateDescriptorSet(mDescriptorPool, mRenderLayout, &renderData.descriptorSet));
			}
			{
				vkr::WriteDescriptor write = {};
				write.binding = 1;
				write.arrayIndex = 0;
				write.type = vkr::DESCRIPTOR_TYPE_SAMPLED_IMAGE;
				write.pImageView = mModelTexture->GetSampledImageView();

				CHECKED_CALL(renderData.descriptorSet->UpdateDescriptors(1, &write));
			}

			// Sampler.
			{
				vkr::WriteDescriptor write = {};
				write.binding = 2;
				write.arrayIndex = 0;
				write.type = vkr::DESCRIPTOR_TYPE_SAMPLER;
				write.pSampler = mLinearSampler;

				CHECKED_CALL(renderData.descriptorSet->UpdateDescriptors(1, &write));
			}

			// Uniform buffer (contains transformation matrix).
			{
				vkr::BufferCreateInfo bufferCreateInfo = {};
				bufferCreateInfo.size = vkr::MINIMUM_UNIFORM_BUFFER_SIZE;
				bufferCreateInfo.usageFlags.bits.uniformBuffer = true;
				bufferCreateInfo.memoryUsage = vkr::MemoryUsage::CPUToGPU;

				CHECKED_CALL(device.CreateBuffer(bufferCreateInfo, &renderData.constants));

				vkr::WriteDescriptor write = {};
				write.binding = 0;
				write.arrayIndex = 0;
				write.type = vkr::DESCRIPTOR_TYPE_UNIFORM_BUFFER;
				write.bufferOffset = 0;
				write.bufferRange = WHOLE_SIZE;
				write.pBuffer = renderData.constants;

				CHECKED_CALL(renderData.descriptorSet->UpdateDescriptors(1, &write));
			}

			// Graphics render pass
			{
				vkr::DrawPassCreateInfo dpCreateInfo = {};
				dpCreateInfo.width = GetRender().GetSwapChain().GetWidth();
				dpCreateInfo.height = GetRender().GetSwapChain().GetHeight();
				dpCreateInfo.depthStencilFormat = vkr::FORMAT_D32_FLOAT;
				dpCreateInfo.depthStencilClearValue = { 1.0f, 0 };
				dpCreateInfo.depthStencilInitialState = vkr::ResourceState::DepthStencilWrite;
				dpCreateInfo.renderTargetCount = 1;
				dpCreateInfo.renderTargetFormats[0] = GetRender().GetSwapChain().GetColorFormat();
				dpCreateInfo.renderTargetClearValues[0] = { 100.0f / 255, 149.0f / 255, 237.0f / 255, 1.0f };
				dpCreateInfo.renderTargetInitialStates[0] = vkr::ResourceState::ShaderResource;
				dpCreateInfo.renderTargetUsageFlags[0] = vkr::IMAGE_USAGE_SAMPLED;

				CHECKED_CALL(device.CreateDrawPass(dpCreateInfo, &renderData.drawPass));
			}
		}
	}

	setupCompute();
	setupComposition();
	setupDrawToSwapchain();

	mCamera.LookAt(float3(0, 2, 7), float3(0, 0, 0));

	return true;
}

void Example_017::Shutdown()
{
	mPerFrame.clear();
	// TODO: доделать очистку
}

void Example_017::Update()
{
}

void Example_017::Render()
{
	auto& render = GetRender();
	auto& swapChain = render.GetSwapChain();
	PerFrame& frame = mPerFrame[0];

	uint32_t imageIndex = acquireFrame(frame);

	updateTransforms(frame);

	for (size_t quadIndex = 0; quadIndex < 4; ++quadIndex) {
		drawScene(frame, quadIndex);
		runCompute(frame, quadIndex);
	}

	// We have to record all composition command buffers after we have recorded rendering and compute first.
	// This is because we are using a single logical graphics queue, and due to DX12 requirements on command list execution order, recording composition commands along rendering and compute would preclude async compute from being possible.
	for (size_t quadIndex = 0; quadIndex < 4; ++quadIndex) {
		compose(frame, quadIndex);
	}

	blitAndPresent(frame, imageIndex);
}

void Example_017::MouseMove(int32_t x, int32_t y, int32_t dx, int32_t dy, MouseButton buttons)
{
	if (buttons == MouseButton::Left) {
		mModelTargetRotation += 0.25f * dx;
	}
}

void Example_017::setupComposition()
{
	auto& device = GetRenderDevice();

	// Descriptor set layout
	{
		// Descriptor set layout
		vkr::DescriptorSetLayoutCreateInfo layoutCreateInfo = {};
		layoutCreateInfo.bindings.push_back(vkr::DescriptorBinding(0, vkr::DESCRIPTOR_TYPE_SAMPLED_IMAGE));
		layoutCreateInfo.bindings.push_back(vkr::DescriptorBinding(1, vkr::DESCRIPTOR_TYPE_SAMPLER));

		CHECKED_CALL(device.CreateDescriptorSetLayout(layoutCreateInfo, &mComposeLayout));
	}

	// Pipeline
	{
		vkr::ShaderModulePtr VS;
		CHECKED_CALL(device.CreateShader("basic/shaders", "StaticTexture.vs", &VS));
		vkr::ShaderModulePtr PS;
		CHECKED_CALL(device.CreateShader("basic/shaders", "StaticTexture.ps", &PS));

		vkr::PipelineInterfaceCreateInfo piCreateInfo = {};
		piCreateInfo.setCount = 1;
		piCreateInfo.sets[0].set = 0;
		piCreateInfo.sets[0].pLayout = mComposeLayout;
		CHECKED_CALL(device.CreatePipelineInterface(piCreateInfo, &mComposePipelineInterface));

		mComposeVertexBinding.AppendAttribute({ "POSITION", 0, vkr::FORMAT_R32G32B32A32_FLOAT, 0, APPEND_OFFSET_ALIGNED, vkr::VERTEX_INPUT_RATE_VERTEX });
		mComposeVertexBinding.AppendAttribute({ "TEXCOORD", 1, vkr::FORMAT_R32G32_FLOAT, 0, APPEND_OFFSET_ALIGNED, vkr::VERTEX_INPUT_RATE_VERTEX });

		vkr::GraphicsPipelineCreateInfo2 gpCreateInfo = {};
		gpCreateInfo.VS = { VS.Get(), "vsmain" };
		gpCreateInfo.PS = { PS.Get(), "psmain" };
		gpCreateInfo.vertexInputState.bindingCount = 1;
		gpCreateInfo.vertexInputState.bindings[0] = mComposeVertexBinding;
		gpCreateInfo.topology = vkr::PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		gpCreateInfo.polygonMode = vkr::POLYGON_MODE_FILL;
		gpCreateInfo.cullMode = vkr::CULL_MODE_BACK;
		gpCreateInfo.frontFace = vkr::FRONT_FACE_CCW;
		gpCreateInfo.depthReadEnable = false;
		gpCreateInfo.depthWriteEnable = false;
		gpCreateInfo.blendModes[0] = vkr::BLEND_MODE_NONE;
		gpCreateInfo.outputState.renderTargetCount = 1;
		gpCreateInfo.outputState.renderTargetFormats[0] = GetRender().GetSwapChain().GetColorFormat();
		gpCreateInfo.outputState.depthStencilFormat = vkr::FORMAT_D32_FLOAT;
		gpCreateInfo.pPipelineInterface = mComposePipelineInterface;
		CHECKED_CALL(device.CreateGraphicsPipeline(gpCreateInfo, &mComposePipeline));

		device.DestroyShaderModule(VS);
		device.DestroyShaderModule(PS);
	}

	for (auto& frameData : mPerFrame) {
		// Graphics render pass
		{
			vkr::DrawPassCreateInfo dpCreateInfo = {};
			dpCreateInfo.width = GetRender().GetSwapChain().GetWidth();
			dpCreateInfo.height = GetRender().GetSwapChain().GetHeight();
			dpCreateInfo.depthStencilFormat = vkr::FORMAT_D32_FLOAT;
			dpCreateInfo.renderTargetCount = 1;
			dpCreateInfo.renderTargetFormats[0] = GetRender().GetSwapChain().GetColorFormat();
			dpCreateInfo.renderTargetClearValues[0] = { 0.0f, 0.0f, 0.0f, 0.0f };
			dpCreateInfo.renderTargetInitialStates[0] = vkr::ResourceState::RenderTarget;
			dpCreateInfo.renderTargetUsageFlags[0] = vkr::IMAGE_USAGE_SAMPLED;

			CHECKED_CALL(device.CreateDrawPass(dpCreateInfo, &frameData.composeDrawPass));
		}

		for (size_t i = 0; i < frameData.composeData.size(); ++i) {
			PerFrame::ComposeData& composeData = frameData.composeData[i];
			PerFrame::ComputeData& computeData = frameData.computeData[i];

			// Descriptor set.
			{
				CHECKED_CALL(device.AllocateDescriptorSet(mDescriptorPool, mComposeLayout, &composeData.descriptorSet));
			}

			// Quad vertex buffer.
			{
				// Split the screen into four quads.
				float offsetX = i < 2 ? 0.0f : 1.0f;
				float offsetY = i % 2 ? 0.0f : -1.0f;

				// clang-format off
				std::vector<float> vertexData = {
					// Position.                                    // vkr::Texture coordinates.
					offsetX + 0.0f,  offsetY + 1.0f, 0.0f, 1.0f,   1.0f, 0.0f,
					offsetX + -1.0f,  offsetY + 1.0f, 0.0f, 1.0f,   0.0f, 0.0f,
					offsetX + -1.0f,  offsetY + 0.0f, 0.0f, 1.0f,   0.0f, 1.0f,

					offsetX + -1.0f,  offsetY + 0.0f, 0.0f, 1.0f,   0.0f, 1.0f,
					offsetX + 0.0f,  offsetY + 0.0f, 0.0f, 1.0f,   1.0f, 1.0f,
					offsetX + 0.0f,  offsetY + 1.0f, 0.0f, 1.0f,   1.0f, 0.0f,
				};
				// clang-format on

				uint32_t dataSize = SizeInBytesU32(vertexData);

				vkr::BufferCreateInfo bufferCreateInfo = {};
				bufferCreateInfo.size = dataSize;
				bufferCreateInfo.usageFlags.bits.vertexBuffer = true;
				bufferCreateInfo.memoryUsage = vkr::MemoryUsage::CPUToGPU;

				CHECKED_CALL(device.CreateBuffer(bufferCreateInfo, &composeData.quadVertexBuffer));

				void* pAddr = nullptr;
				CHECKED_CALL(composeData.quadVertexBuffer->MapMemory(0, &pAddr));
				memcpy(pAddr, vertexData.data(), dataSize);
				composeData.quadVertexBuffer->UnmapMemory();
			}

			// Descriptors.
			{
				vkr::WriteDescriptor writes[2] = {};
				writes[0].binding = 0;
				writes[0].arrayIndex = 0;
				writes[0].type = vkr::DESCRIPTOR_TYPE_SAMPLED_IMAGE;

				writes[1].binding = 1;
				writes[1].type = vkr::DESCRIPTOR_TYPE_SAMPLER;
				writes[1].pSampler = mLinearSampler;

				writes[0].pImageView = computeData.outputImageSampledView;
				CHECKED_CALL(composeData.descriptorSet->UpdateDescriptors(2, writes));
			}
		}
	}
}

void Example_017::setupCompute()
{
	auto& device = GetRenderDevice();

	// Descriptor layout for compute pipeline (ImageFilter.hlsl)
	{
		vkr::DescriptorSetLayoutCreateInfo layoutCreateInfo = {};
		layoutCreateInfo.bindings.push_back(vkr::DescriptorBinding(0, vkr::DESCRIPTOR_TYPE_STORAGE_IMAGE));
		layoutCreateInfo.bindings.push_back(vkr::DescriptorBinding(1, vkr::DESCRIPTOR_TYPE_UNIFORM_BUFFER));
		layoutCreateInfo.bindings.push_back(vkr::DescriptorBinding(2, vkr::DESCRIPTOR_TYPE_SAMPLER));
		layoutCreateInfo.bindings.push_back(vkr::DescriptorBinding(3, vkr::DESCRIPTOR_TYPE_SAMPLED_IMAGE));
		CHECKED_CALL(device.CreateDescriptorSetLayout(layoutCreateInfo, &mComputeLayout));
	}

	// Compute pipeline
	{
		vkr::ShaderModulePtr CS;
		CHECKED_CALL(device.CreateShader("basic/shaders", "ImageFilter.cs", &CS));

		vkr::PipelineInterfaceCreateInfo piCreateInfo = {};
		piCreateInfo.setCount = 1;
		piCreateInfo.sets[0].set = 0;
		piCreateInfo.sets[0].pLayout = mComputeLayout;
		CHECKED_CALL(device.CreatePipelineInterface(piCreateInfo, &mComputePipelineInterface));

		vkr::ComputePipelineCreateInfo cpCreateInfo = {};
		cpCreateInfo.CS = { CS.Get(), "csmain" };
		cpCreateInfo.pPipelineInterface = mComputePipelineInterface;

		CHECKED_CALL(device.CreateComputePipeline(cpCreateInfo, &mComputePipeline));

		device.DestroyShaderModule(CS);
	}

	for (auto& frameData : mPerFrame) {
		for (size_t i = 0; i < frameData.computeData.size(); ++i) {
			PerFrame::ComputeData& computeData = frameData.computeData[i];
			vkr::Texture* sourceTexture = frameData.renderData[i].drawPass->GetRenderTargetTexture(0);

			// Descriptor set.
			{
				CHECKED_CALL(device.AllocateDescriptorSet(mDescriptorPool, mComputeLayout, &computeData.descriptorSet));
			}

			// Output image and views.
			{
				vkr::ImageCreateInfo ci = {};
				ci.type = vkr::IMAGE_TYPE_2D;
				ci.width = sourceTexture->GetWidth();
				ci.height = sourceTexture->GetHeight();
				ci.depth = 1;
				ci.format = GetRender().GetSwapChain().GetColorFormat();
				ci.usageFlags.bits.sampled = true;
				ci.usageFlags.bits.storage = true;
				ci.memoryUsage = vkr::MemoryUsage::GPUOnly;
				ci.initialState = vkr::ResourceState::NonPixelShaderResource;

				CHECKED_CALL(device.CreateImage(ci, &computeData.outputImage));

				vkr::SampledImageViewCreateInfo sampledViewCreateInfo = vkr::SampledImageViewCreateInfo::GuessFromImage(computeData.outputImage);
				CHECKED_CALL(device.CreateSampledImageView(sampledViewCreateInfo, &computeData.outputImageSampledView));

				vkr::StorageImageViewCreateInfo storageViewCreateInfo = vkr::StorageImageViewCreateInfo::GuessFromImage(computeData.outputImage);
				CHECKED_CALL(device.CreateStorageImageView(storageViewCreateInfo, &computeData.outputImageStorageView));
			}

			// Uniform buffer (contains filter selection flag).
			{
				vkr::BufferCreateInfo bufferCreateInfo = {};
				bufferCreateInfo.size = vkr::MINIMUM_UNIFORM_BUFFER_SIZE;
				bufferCreateInfo.usageFlags.bits.uniformBuffer = true;
				bufferCreateInfo.memoryUsage = vkr::MemoryUsage::CPUToGPU;

				CHECKED_CALL(device.CreateBuffer(bufferCreateInfo, &computeData.constants));

				struct alignas(16) ParamsData
				{
					float2 texel_size;
					int    filter;
				};
				ParamsData params;

				params.texel_size.x = 1.0f / sourceTexture->GetWidth();
				params.texel_size.y = 1.0f / sourceTexture->GetHeight();
				params.filter = static_cast<int>(i + 1); // Apply a different filter to each quad.

				void* pMappedAddress = nullptr;
				CHECKED_CALL(computeData.constants->MapMemory(0, &pMappedAddress));
				memcpy(pMappedAddress, &params, sizeof(ParamsData));
				computeData.constants->UnmapMemory();
			}

			// Descriptors.
			{
				vkr::WriteDescriptor write = {};
				write.binding = 0;
				write.arrayIndex = 0;
				write.type = vkr::DESCRIPTOR_TYPE_STORAGE_IMAGE;
				write.pImageView = computeData.outputImageStorageView;
				CHECKED_CALL(computeData.descriptorSet->UpdateDescriptors(1, &write));

				write = {};
				write.binding = 1;
				write.arrayIndex = 0;
				write.type = vkr::DESCRIPTOR_TYPE_UNIFORM_BUFFER;
				write.bufferOffset = 0;
				write.bufferRange = WHOLE_SIZE;
				write.pBuffer = computeData.constants;

				CHECKED_CALL(computeData.descriptorSet->UpdateDescriptors(1, &write));

				write = {};
				write.binding = 2;
				write.arrayIndex = 0;
				write.type = vkr::DESCRIPTOR_TYPE_SAMPLER;
				write.pSampler = mNearestSampler;
				CHECKED_CALL(computeData.descriptorSet->UpdateDescriptors(1, &write));

				write = {};
				write.binding = 3;
				write.arrayIndex = 0;
				write.type = vkr::DESCRIPTOR_TYPE_SAMPLED_IMAGE;
				write.pImageView = sourceTexture->GetSampledImageView();
				CHECKED_CALL(computeData.descriptorSet->UpdateDescriptors(1, &write));
			}
		}
	}
}

void Example_017::setupDrawToSwapchain()
{
	auto& device = GetRenderDevice();

	// Descriptor set layout
	{
		// Descriptor set layout
		vkr::DescriptorSetLayoutCreateInfo layoutCreateInfo = {};
		layoutCreateInfo.bindings.push_back(vkr::DescriptorBinding(0, vkr::DESCRIPTOR_TYPE_SAMPLED_IMAGE));
		layoutCreateInfo.bindings.push_back(vkr::DescriptorBinding(1, vkr::DESCRIPTOR_TYPE_SAMPLER));
		CHECKED_CALL(device.CreateDescriptorSetLayout(layoutCreateInfo, &mDrawToSwapchainLayout));
	}

	// Pipeline
	{
		vkr::ShaderModulePtr VS;
		CHECKED_CALL(device.CreateShader("basic/shaders", "FullScreenTriangle.vs", &VS));
		vkr::ShaderModulePtr PS;
		CHECKED_CALL(device.CreateShader("basic/shaders", "FullScreenTriangle.ps", &PS));

		vkr::FullscreenQuadCreateInfo createInfo = {};
		createInfo.VS = VS;
		createInfo.PS = PS;
		createInfo.setCount = 1;
		createInfo.sets[0].set = 0;
		createInfo.sets[0].pLayout = mDrawToSwapchainLayout;
		createInfo.renderTargetCount = 1;
		createInfo.renderTargetFormats[0] = GetRender().GetSwapChain().GetColorFormat();
		createInfo.depthStencilFormat = GetRender().GetSwapChain().GetDepthFormat();

		CHECKED_CALL(device.CreateFullscreenQuad(createInfo, &mDrawToSwapchainPipeline));
	}

	// Allocate descriptor set
	for (auto& frameData : mPerFrame) {
		PerFrame::DrawToSwapchainData& drawData = frameData.drawToSwapchainData;
		CHECKED_CALL(device.AllocateDescriptorSet(mDescriptorPool, mDrawToSwapchainLayout, &drawData.descriptorSet));

		// Write descriptors
		{
			vkr::WriteDescriptor writes[2] = {};
			writes[0].binding = 0;
			writes[0].arrayIndex = 0;
			writes[0].type = vkr::DESCRIPTOR_TYPE_SAMPLED_IMAGE;
			writes[0].pImageView = frameData.composeDrawPass->GetRenderTargetTexture(0)->GetSampledImageView();

			writes[1].binding = 1;
			writes[1].type = vkr::DESCRIPTOR_TYPE_SAMPLER;
			writes[1].pSampler = mLinearSampler;

			CHECKED_CALL(drawData.descriptorSet->UpdateDescriptors(2, writes));
		}
	}
}

void Example_017::updateTransforms(PerFrame& frame)
{
	for (PerFrame::RenderData& renderData : frame.renderData) {
		vkr::BufferPtr buf = renderData.constants;

		mModelRotation += (mModelTargetRotation - mModelRotation) * 0.1f;

		void* pMappedAddress = nullptr;
		CHECKED_CALL(buf->MapMemory(0, &pMappedAddress));

		const float4x4& PV = mCamera.GetViewProjectionMatrix();
		float4x4        M = glm::rotate(glm::radians(mModelRotation + 180.0f), float3(0, 1, 0));
		float4x4        mat = PV * M;
		memcpy(pMappedAddress, &mat, sizeof(mat));

		buf->UnmapMemory();
	}
}

uint32_t Example_017::acquireFrame(PerFrame& frame)
{
	// Wait for and reset render complete fence
	CHECKED_CALL(frame.renderCompleteFence->WaitAndReset());

	uint32_t imageIndex = UINT32_MAX;
	CHECKED_CALL(GetRender().GetSwapChain().AcquireNextImage(UINT64_MAX, frame.imageAcquiredSemaphore, frame.imageAcquiredFence, &imageIndex));

	// Wait for and reset image acquired fence
	CHECKED_CALL(frame.imageAcquiredFence->WaitAndReset());

	return imageIndex;
}

void Example_017::blitAndPresent(PerFrame& frame, uint32_t swapchainImageIndex)
{
	vkr::RenderPassPtr renderPass = GetRender().GetSwapChain().GetRenderPass(swapchainImageIndex);
	ASSERT_MSG(!renderPass.IsNull(), "swapchain render pass object is null");

	vkr::CommandBufferPtr cmd = frame.drawToSwapchainData.cmd;

	CHECKED_CALL(cmd->Begin());
	{
		cmd->SetScissors(renderPass->GetScissor());
		cmd->SetViewports(renderPass->GetViewport());
		cmd->TransitionImageLayout(renderPass->GetRenderTargetImage(0), ALL_SUBRESOURCES, vkr::ResourceState::Present, vkr::ResourceState::RenderTarget);
		cmd->TransitionImageLayout(frame.composeDrawPass->GetRenderTargetTexture(0), ALL_SUBRESOURCES, vkr::ResourceState::RenderTarget, vkr::ResourceState::PixelShaderResource);
		cmd->BeginRenderPass(renderPass);
		{
			// Draw composed image to swapchain.
			cmd->Draw(mDrawToSwapchainPipeline, 1, &frame.drawToSwapchainData.descriptorSet);

			// Draw ImGui.
			GetRender().DrawDebugInfo();
			ImGui::Separator();
			ImGui::SliderInt("Graphics Load", &mGraphicsLoad, 1, 500);
			ImGui::SliderInt("Compute Load", &mComputeLoad, 1, 20);
			GetRender().DrawImGui(cmd);
		}
		cmd->EndRenderPass();
		cmd->TransitionImageLayout(frame.composeDrawPass->GetRenderTargetTexture(0), ALL_SUBRESOURCES, vkr::ResourceState::PixelShaderResource, vkr::ResourceState::RenderTarget);
		cmd->TransitionImageLayout(renderPass->GetRenderTargetImage(0), ALL_SUBRESOURCES, vkr::ResourceState::RenderTarget, vkr::ResourceState::Present);
	}
	CHECKED_CALL(cmd->End());

	const vkr::Semaphore* ppWaitSemaphores[] = {
		frame.composeData[0].completeSemaphore,
		frame.composeData[1].completeSemaphore,
		frame.composeData[2].completeSemaphore,
		frame.composeData[3].completeSemaphore,
		frame.imageAcquiredSemaphore };

	vkr::SubmitInfo submitInfo = {};
	submitInfo.commandBufferCount = 1;
	submitInfo.ppCommandBuffers = &cmd;
	submitInfo.waitSemaphoreCount = sizeof(ppWaitSemaphores) / sizeof(ppWaitSemaphores[0]);
	submitInfo.ppWaitSemaphores = ppWaitSemaphores;
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.ppSignalSemaphores = &frame.renderCompleteSemaphore;
	submitInfo.pFence = frame.renderCompleteFence;

	CHECKED_CALL(GetRenderDevice().GetGraphicsQueue()->Submit(&submitInfo));

	CHECKED_CALL(GetRender().GetSwapChain().Present(swapchainImageIndex, 1, &frame.renderCompleteSemaphore));
}

void Example_017::runCompute(PerFrame& frame, size_t quadIndex)
{
	PerFrame::ComputeData& computeData = frame.computeData[quadIndex];
	PerFrame::RenderData& renderData = frame.renderData[quadIndex];

	CHECKED_CALL(computeData.cmd->Begin());
	{
		// Acquire from graphics queue to compute queue.
		if (mUseQueueFamilyTransfers) {
			computeData.cmd->TransitionImageLayout(renderData.drawPass->GetRenderTargetTexture(0), ALL_SUBRESOURCES, vkr::ResourceState::ShaderResource, vkr::ResourceState::ShaderResource, mGraphicsQueue, mComputeQueue);
		}

		computeData.cmd->TransitionImageLayout(computeData.outputImage, ALL_SUBRESOURCES, vkr::ResourceState::NonPixelShaderResource, vkr::ResourceState::UnorderedAccess);
		{
			vkr::DescriptorSet* sets[1] = { nullptr };
			sets[0] = computeData.descriptorSet;
			computeData.cmd->BindComputeDescriptorSets(mComputePipelineInterface, 1, sets);
			computeData.cmd->BindComputePipeline(mComputePipeline);
			uint32_t dispatchX = static_cast<uint32_t>(std::ceil(computeData.outputImage->GetWidth() / 32.0));
			uint32_t dispatchY = static_cast<uint32_t>(std::ceil(computeData.outputImage->GetHeight() / 32.0));
			for (int i = 0; i < mComputeLoad; ++i)
				computeData.cmd->Dispatch(dispatchX, dispatchY, 1);
		}
		computeData.cmd->TransitionImageLayout(computeData.outputImage, ALL_SUBRESOURCES, vkr::ResourceState::UnorderedAccess, vkr::ResourceState::NonPixelShaderResource);

		// Release from compute queue to graphics queue.
		if (mUseQueueFamilyTransfers) {
			computeData.cmd->TransitionImageLayout(computeData.outputImage, ALL_SUBRESOURCES, vkr::ResourceState::NonPixelShaderResource, vkr::ResourceState::NonPixelShaderResource, mComputeQueue, mGraphicsQueue);
		}
	}
	CHECKED_CALL(computeData.cmd->End());

	vkr::SubmitInfo submitInfo = {};
	submitInfo.commandBufferCount = 1;
	submitInfo.ppCommandBuffers = &computeData.cmd;
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.ppWaitSemaphores = &renderData.completeSemaphore;
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.ppSignalSemaphores = &computeData.completeSemaphore;

	if (mAsyncComputeEnabled) {
		CHECKED_CALL(GetRenderDevice().GetComputeQueue()->Submit(&submitInfo));
	}
	else {
		CHECKED_CALL(GetRenderDevice().GetGraphicsQueue()->Submit(&submitInfo));
	}
}

void Example_017::compose(PerFrame& frame, size_t quadIndex)
{
	PerFrame::ComposeData& composeData = frame.composeData[quadIndex];
	PerFrame::ComputeData& computeData = frame.computeData[quadIndex];

	CHECKED_CALL(composeData.cmd->Begin());
	{
		vkr::DrawPassPtr renderPass = frame.composeDrawPass;

		composeData.cmd->SetScissors(renderPass->GetScissor());
		composeData.cmd->SetViewports(renderPass->GetViewport());

		// Acquire from compute queue to graphics queue.
		if (mUseQueueFamilyTransfers) {
			composeData.cmd->TransitionImageLayout(computeData.outputImage, ALL_SUBRESOURCES, vkr::ResourceState::NonPixelShaderResource, vkr::ResourceState::NonPixelShaderResource, mComputeQueue, mGraphicsQueue);
		}

		composeData.cmd->BeginRenderPass(renderPass, 0 /* do not clear render target */);
		{
			vkr::DescriptorSet* sets[1] = { nullptr };
			sets[0] = composeData.descriptorSet;
			composeData.cmd->BindGraphicsDescriptorSets(mComposePipelineInterface, 1, sets);

			composeData.cmd->BindGraphicsPipeline(mComposePipeline);

			composeData.cmd->BindVertexBuffers(1, &composeData.quadVertexBuffer, &mComposeVertexBinding.GetStride());
			composeData.cmd->Draw(6);
		}
		composeData.cmd->EndRenderPass();
	}
	CHECKED_CALL(composeData.cmd->End());

	vkr::SubmitInfo submitInfo = {};
	submitInfo.commandBufferCount = 1;
	submitInfo.ppCommandBuffers = &composeData.cmd;
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.ppWaitSemaphores = &computeData.completeSemaphore;
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.ppSignalSemaphores = &composeData.completeSemaphore;

	CHECKED_CALL(GetRenderDevice().GetGraphicsQueue()->Submit(&submitInfo));
}

void Example_017::drawScene(PerFrame& frame, size_t quadIndex)
{
	PerFrame::RenderData& renderData = frame.renderData[quadIndex];
	CHECKED_CALL(renderData.cmd->Begin());
	{
		renderData.cmd->SetScissors(renderData.drawPass->GetScissor());
		renderData.cmd->SetViewports(renderData.drawPass->GetViewport());

		// Draw model.
		renderData.cmd->TransitionImageLayout(renderData.drawPass->GetRenderTargetTexture(0), ALL_SUBRESOURCES, vkr::ResourceState::ShaderResource, vkr::ResourceState::RenderTarget);
		renderData.cmd->BeginRenderPass(renderData.drawPass, vkr::DRAW_PASS_CLEAR_FLAG_CLEAR_RENDER_TARGETS | vkr::DRAW_PASS_CLEAR_FLAG_CLEAR_DEPTH);
		{
			vkr::DescriptorSet* sets[1] = { nullptr };
			sets[0] = renderData.descriptorSet;
			renderData.cmd->BindGraphicsDescriptorSets(mRenderPipelineInterface, 1, sets);

			renderData.cmd->BindGraphicsPipeline(mRenderPipeline);

			renderData.cmd->BindIndexBuffer(mModelMesh);
			renderData.cmd->BindVertexBuffers(mModelMesh);
			renderData.cmd->DrawIndexed(mModelMesh->GetIndexCount(), mGraphicsLoad);
		}
		renderData.cmd->EndRenderPass();
		renderData.cmd->TransitionImageLayout(renderData.drawPass->GetRenderTargetTexture(0), ALL_SUBRESOURCES, vkr::ResourceState::RenderTarget, vkr::ResourceState::ShaderResource);

		// Release from graphics queue to compute queue.
		if (mUseQueueFamilyTransfers) {
			renderData.cmd->TransitionImageLayout(renderData.drawPass->GetRenderTargetTexture(0), ALL_SUBRESOURCES, vkr::ResourceState::ShaderResource, vkr::ResourceState::ShaderResource, mGraphicsQueue, mComputeQueue);
		}
	}
	CHECKED_CALL(renderData.cmd->End());

	vkr::SubmitInfo submitInfo = {};
	submitInfo.commandBufferCount = 1;
	submitInfo.ppCommandBuffers = &renderData.cmd;
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.ppSignalSemaphores = &renderData.completeSemaphore;

	CHECKED_CALL(GetRenderDevice().GetGraphicsQueue()->Submit(&submitInfo));
}