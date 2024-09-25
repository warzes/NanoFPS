#include "stdafx.h"
#include "024_OIT.h"

static constexpr float MESH_SCALE_DEFAULT = 2.0f;
static constexpr float MESH_SCALE_MIN = 1.0f;
static constexpr float MESH_SCALE_MAX = 5.0f;

// константы в Common.hlsli

#define BUFFER_BUCKETS_SIZE_PER_PIXEL           8
#define BUFFER_LISTS_FRAGMENT_BUFFER_MAX_SCALE  8
#define BUFFER_LISTS_SORTED_FRAGMENT_MAX_COUNT  64
#define BUFFER_LISTS_INVALID_INDEX              0xFFFFFFFFU

#if defined(IS_SHADER)
#define SHADER_REGISTER(type, num) type##num
#else
#define SHADER_REGISTER(type, num) num
#endif

#define SHADER_GLOBALS_REGISTER       SHADER_REGISTER(b, 0)
#define CUSTOM_SAMPLER_0_REGISTER     SHADER_REGISTER(s, 1)
#define CUSTOM_SAMPLER_1_REGISTER     SHADER_REGISTER(s, 2)
#define CUSTOM_TEXTURE_0_REGISTER     SHADER_REGISTER(t, 3)
#define CUSTOM_TEXTURE_1_REGISTER     SHADER_REGISTER(t, 4)
#define CUSTOM_UAV_0_REGISTER         SHADER_REGISTER(u, 11)
#define CUSTOM_UAV_1_REGISTER         SHADER_REGISTER(u, 12)
#define CUSTOM_UAV_2_REGISTER         SHADER_REGISTER(u, 13)

struct ShaderGlobals
{
	float4x4 backgroundMVP;
	float4   backgroundColor;
	float4x4 meshMVP;

	float    meshOpacity;
	float    _floatUnused0;
	float    _floatUnused1;
	float    _floatUnused2;

	int      depthPeelingFrontLayerIndex;
	int      depthPeelingBackLayerIndex;
	int      bufferBucketsFragmentsMaxCount;
	int      bufferListsFragmentBufferScale;
	int      bufferListsSortedFragmentMaxCount;
	int      _intUnused0;
	int      _intUnused1;
	int      _intUnused2;
};

EngineApplicationCreateInfo Example_024::Config() const
{
	EngineApplicationCreateInfo createInfo{};
	createInfo.render.swapChain.depthFormat = vkr::FORMAT_D32_FLOAT;
	createInfo.render.showImgui = true;
	return createInfo;
}

bool Example_024::Setup()
{
	auto& device = GetRenderDevice();

	SetupCommon();
	FillSupportedAlgorithmData();
	ParseCommandLineOptions();

	void (Example_024:: * setupFuncs[])() =
	{
		&Example_024::SetupUnsortedOver,
		&Example_024::SetupWeightedSum,
		&Example_024::SetupWeightedAverage,
		&Example_024::SetupDepthPeeling,
		&Example_024::SetupBuffer,
	};
	static_assert(sizeof(setupFuncs) / sizeof(setupFuncs[0]) == ALGORITHMS_COUNT, "Algorithm setup func count mismatch");

	for (const Algorithm algorithm : mSupportedAlgorithmIds) {
		ASSERT_MSG(algorithm >= 0 && algorithm < ALGORITHMS_COUNT, "unknown algorithm");
		(this->*setupFuncs[algorithm])();
	}

	return true;
}

void Example_024::Shutdown()
{
	// TODO: доделать очистку
}

void Example_024::Update()
{
	const float elapsedSeconds = 0.01f;// GetElapsedSeconds();
	const float deltaSeconds = elapsedSeconds - mPreviousElapsedSeconds;
	mPreviousElapsedSeconds = elapsedSeconds;

	// Shader globals
	{
		const float4x4 VP =
			glm::perspective(glm::radians(60.0f), GetWindowAspect(), 0.001f, 10000.0f) *
			glm::lookAt(float3(0, 0, 8), float3(0, 0, 0), float3(0, 1, 0));

		ShaderGlobals shaderGlobals = {};
		{
			const float4x4 M = glm::scale(float3(20.0));
			shaderGlobals.backgroundMVP = VP * M;

			shaderGlobals.backgroundColor.r = mGuiParameters.background.color[0];
			shaderGlobals.backgroundColor.g = mGuiParameters.background.color[1];
			shaderGlobals.backgroundColor.b = mGuiParameters.background.color[2];
			shaderGlobals.backgroundColor.a = 1.0f;
		}
		{
			if (mGuiParameters.mesh.auto_rotate) {
				mMeshAnimationSeconds += deltaSeconds;
			}
			const float4x4 M =
				glm::rotate(mMeshAnimationSeconds, float3(0.0f, 0.0f, 1.0f)) *
				glm::rotate(2.0f * mMeshAnimationSeconds, float3(0.0f, 1.0f, 0.0f)) *
				glm::rotate(mMeshAnimationSeconds, float3(1.0f, 0.0f, 0.0f)) *
				glm::scale(float3(mGuiParameters.mesh.scale));
			shaderGlobals.meshMVP = VP * M;
		}
		shaderGlobals.meshOpacity = mGuiParameters.mesh.opacity;

		shaderGlobals.depthPeelingFrontLayerIndex = std::max(0, mGuiParameters.depthPeeling.startLayer);
		shaderGlobals.depthPeelingBackLayerIndex = std::min(DEPTH_PEELING_LAYERS_COUNT - 1, mGuiParameters.depthPeeling.startLayer + mGuiParameters.depthPeeling.layersCount - 1);

		shaderGlobals.bufferBucketsFragmentsMaxCount = std::min(BUFFER_BUCKETS_SIZE_PER_PIXEL, mGuiParameters.buffer.bucketsFragmentsMaxCount);
		shaderGlobals.bufferListsFragmentBufferScale = std::min(BUFFER_LISTS_FRAGMENT_BUFFER_MAX_SCALE, mGuiParameters.buffer.listsFragmentBufferScale);
		shaderGlobals.bufferListsSortedFragmentMaxCount = std::min(BUFFER_LISTS_SORTED_FRAGMENT_MAX_COUNT, mGuiParameters.buffer.listsSortedFragmentMaxCount);

		mShaderGlobalsBuffer->CopyFromSource(sizeof(shaderGlobals), &shaderGlobals);
	}

	//UpdateGUI();
}

void Example_024::Render()
{
	auto& render = GetRender();
	auto& device = GetRenderDevice();
	auto& swapChain = render.GetSwapChain();

	CHECKED_CALL(mRenderCompleteFence->WaitAndReset());
	uint32_t imageIndex = UINT32_MAX;
	CHECKED_CALL(swapChain.AcquireNextImage(UINT64_MAX, mImageAcquiredSemaphore, mImageAcquiredFence, &imageIndex));
	CHECKED_CALL(mImageAcquiredFence->WaitAndReset());

	// Update state
	Update();

	// Record command buffer
	CHECKED_CALL(mCommandBuffer->Begin());
	RecordOpaque();
	RecordTransparency();
	RecordComposite(swapChain.GetRenderPass(imageIndex));
	CHECKED_CALL(mCommandBuffer->End());

	// Submit and present
	vkr::SubmitInfo submitInfo = {};
	submitInfo.commandBufferCount = 1;
	submitInfo.ppCommandBuffers = &mCommandBuffer;
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.ppWaitSemaphores = &mImageAcquiredSemaphore;
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.ppSignalSemaphores = &mRenderCompleteSemaphore;
	submitInfo.pFence = mRenderCompleteFence;
	CHECKED_CALL(device.GetGraphicsQueue()->Submit(&submitInfo));
	CHECKED_CALL(swapChain.Present(imageIndex, 1, &mRenderCompleteSemaphore));
}

void Example_024::SetupCommon()
{
	auto& render = GetRender();
	auto& device = GetRenderDevice();
	auto& swapChain = render.GetSwapChain();

	mPreviousElapsedSeconds = 0.01f;//GetElapsedSeconds();
	mMeshAnimationSeconds = mPreviousElapsedSeconds;

	////////////////////////////////////////
	// Shared
	////////////////////////////////////////

	// Synchronization objects
	{
		vkr::SemaphoreCreateInfo semaCreateInfo = {};
		CHECKED_CALL(GetRenderDevice().CreateSemaphore(semaCreateInfo, &mImageAcquiredSemaphore));

		vkr::FenceCreateInfo fenceCreateInfo = {};
		CHECKED_CALL(GetRenderDevice().CreateFence(fenceCreateInfo, &mImageAcquiredFence));

		CHECKED_CALL(GetRenderDevice().CreateSemaphore(semaCreateInfo, &mRenderCompleteSemaphore));

		fenceCreateInfo.signaled = true;
		CHECKED_CALL(GetRenderDevice().CreateFence(fenceCreateInfo, &mRenderCompleteFence));
	}

	// Command buffer
	{
		CHECKED_CALL(GetRenderDevice().GetGraphicsQueue()->CreateCommandBuffer(&mCommandBuffer));
	}

	// Descriptor pool
	{
		vkr::DescriptorPoolCreateInfo createInfo = {};
		createInfo.sampler = 16;
		createInfo.sampledImage = 16;
		createInfo.uniformBuffer = 16;
		createInfo.structuredBuffer = 16;
		createInfo.storageTexelBuffer = 16;
		CHECKED_CALL(GetRenderDevice().CreateDescriptorPool(createInfo, &mDescriptorPool));
	}

	// Sampler
	{
		vkr::SamplerCreateInfo createInfo = {};
		createInfo.magFilter = vkr::Filter::Nearest;
		createInfo.minFilter = vkr::Filter::Nearest;
		createInfo.mipmapMode = vkr::SamplerMipmapMode::Nearest;
		CHECKED_CALL(GetRenderDevice().CreateSampler(createInfo, &mNearestSampler));
	}

	// Meshes
	{
		vkr::QueuePtr queue = GetRenderDevice().GetGraphicsQueue();
		vkr::TriMeshOptions options = vkr::TriMeshOptions().Indices();
		CHECKED_CALL(vkr::vkrUtil::CreateMeshFromFile(queue, "basic/models/cube.obj", &mBackgroundMesh, options));
		CHECKED_CALL(vkr::vkrUtil::CreateMeshFromFile(queue, "basic/models/monkey.obj", &mTransparentMeshes[MESH_TYPE_MONKEY], options));
		CHECKED_CALL(vkr::vkrUtil::CreateMeshFromFile(queue, "basic/models/horse.obj", &mTransparentMeshes[MESH_TYPE_HORSE], options));
		CHECKED_CALL(vkr::vkrUtil::CreateMeshFromFile(queue, "basic/models/megaphone.obj", &mTransparentMeshes[MESH_TYPE_MEGAPHONE], options));
		CHECKED_CALL(vkr::vkrUtil::CreateMeshFromFile(queue, "basic/models/cannon.obj", &mTransparentMeshes[MESH_TYPE_CANNON], options));
	}

	// Shader globals
	{
		vkr::BufferCreateInfo bufferCreateInfo = {};
		bufferCreateInfo.size = std::max(sizeof(ShaderGlobals), static_cast<size_t>(vkr::MINIMUM_UNIFORM_BUFFER_SIZE));
		bufferCreateInfo.usageFlags.bits.uniformBuffer = true;
		bufferCreateInfo.memoryUsage = vkr::MemoryUsage::CPUToGPU;
		CHECKED_CALL(GetRenderDevice().CreateBuffer(bufferCreateInfo, &mShaderGlobalsBuffer));
	}

	////////////////////////////////////////
	// Opaque
	////////////////////////////////////////

	// Pass
	{
		vkr::DrawPassCreateInfo createInfo = {};
		createInfo.width = swapChain.GetWidth();
		createInfo.height = swapChain.GetHeight();
		createInfo.renderTargetCount = 1;
		createInfo.renderTargetFormats[0] = swapChain.GetColorFormat();
		createInfo.depthStencilFormat = vkr::FORMAT_D32_FLOAT;
		createInfo.renderTargetUsageFlags[0] = vkr::IMAGE_USAGE_SAMPLED;
		createInfo.depthStencilUsageFlags = vkr::IMAGE_USAGE_TRANSFER_SRC | vkr::IMAGE_USAGE_SAMPLED;
		createInfo.renderTargetInitialStates[0] = vkr::ResourceState::ShaderResource;
		createInfo.depthStencilInitialState = vkr::ResourceState::ShaderResource;
		createInfo.renderTargetClearValues[0] = { 0, 0, 0, 0 };
		createInfo.depthStencilClearValue = { 1.0f, 0xFF };
		CHECKED_CALL(GetRenderDevice().CreateDrawPass(createInfo, &mOpaquePass));
	}

	// Descriptor
	{
		vkr::DescriptorSetLayoutCreateInfo layoutCreateInfo = {};
		layoutCreateInfo.bindings.push_back(vkr::DescriptorBinding{ SHADER_GLOBALS_REGISTER, vkr::DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, vkr::SHADER_STAGE_ALL_GRAPHICS });
		CHECKED_CALL(GetRenderDevice().CreateDescriptorSetLayout(layoutCreateInfo, &mOpaqueDescriptorSetLayout));

		CHECKED_CALL(GetRenderDevice().AllocateDescriptorSet(mDescriptorPool, mOpaqueDescriptorSetLayout, &mOpaqueDescriptorSet));

		vkr::WriteDescriptor write = {};
		write.binding = SHADER_GLOBALS_REGISTER;
		write.type = vkr::DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		write.bufferOffset = 0;
		write.bufferRange = WHOLE_SIZE;
		write.pBuffer = mShaderGlobalsBuffer;
		CHECKED_CALL(mOpaqueDescriptorSet->UpdateDescriptors(1, &write));
	}

	// Pipeline
	{
		vkr::PipelineInterfaceCreateInfo piCreateInfo = {};
		piCreateInfo.setCount = 1;
		piCreateInfo.sets[0].set = 0;
		piCreateInfo.sets[0].pLayout = mOpaqueDescriptorSetLayout;
		CHECKED_CALL(GetRenderDevice().CreatePipelineInterface(piCreateInfo, &mOpaquePipelineInterface));

		vkr::ShaderModulePtr VS, PS;
		CHECKED_CALL(device.CreateShader("basic/shaders/oit_demo", "Opaque.vs", &VS));
		CHECKED_CALL(device.CreateShader("basic/shaders/oit_demo", "Opaque.ps", &PS));

		vkr::GraphicsPipelineCreateInfo2 gpCreateInfo = {};
		gpCreateInfo.VS = { VS, "vsmain" };
		gpCreateInfo.PS = { PS, "psmain" };
		gpCreateInfo.vertexInputState.bindingCount = 1;
		gpCreateInfo.vertexInputState.bindings[0] = mBackgroundMesh->GetDerivedVertexBindings()[0];
		gpCreateInfo.topology = vkr::PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		gpCreateInfo.polygonMode = vkr::POLYGON_MODE_FILL;
		gpCreateInfo.cullMode = vkr::CULL_MODE_FRONT;
		gpCreateInfo.frontFace = vkr::FRONT_FACE_CCW;
		gpCreateInfo.depthReadEnable = true;
		gpCreateInfo.depthWriteEnable = true;
		gpCreateInfo.blendModes[0] = vkr::BLEND_MODE_NONE;
		gpCreateInfo.outputState.renderTargetCount = 1;
		gpCreateInfo.outputState.renderTargetFormats[0] = mOpaquePass->GetRenderTargetTexture(0)->GetImageFormat();
		gpCreateInfo.outputState.depthStencilFormat = mOpaquePass->GetDepthStencilTexture()->GetImageFormat();
		gpCreateInfo.pPipelineInterface = mOpaquePipelineInterface;
		CHECKED_CALL(GetRenderDevice().CreateGraphicsPipeline(gpCreateInfo, &mOpaquePipeline));

		GetRenderDevice().DestroyShaderModule(VS);
		GetRenderDevice().DestroyShaderModule(PS);
	}

	////////////////////////////////////////
	// Transparency
	////////////////////////////////////////

	// Texture
	{
		vkr::TextureCreateInfo createInfo = {};
		createInfo.imageType = vkr::IMAGE_TYPE_2D;
		createInfo.width = swapChain.GetWidth();
		createInfo.height = swapChain.GetHeight();
		createInfo.depth = 1;
		createInfo.imageFormat = vkr::FORMAT_R16G16B16A16_FLOAT;
		createInfo.sampleCount = vkr::SAMPLE_COUNT_1;
		createInfo.mipLevelCount = 1;
		createInfo.arrayLayerCount = 1;
		createInfo.usageFlags.bits.colorAttachment = true;
		createInfo.usageFlags.bits.sampled = true;
		createInfo.memoryUsage = vkr::MemoryUsage::GPUOnly;
		createInfo.initialState = vkr::ResourceState::ShaderResource;
		createInfo.RTVClearValue = { 0, 0, 0, 0 };
		createInfo.DSVClearValue = { 1.0f, 0xFF };
		CHECKED_CALL(GetRenderDevice().CreateTexture(createInfo, &mTransparencyTexture));
	}

	// Pass
	{
		vkr::DrawPassCreateInfo2 createInfo = {};
		createInfo.width = swapChain.GetWidth();
		createInfo.height = swapChain.GetHeight();
		createInfo.renderTargetCount = 1;
		createInfo.pRenderTargetImages[0] = mTransparencyTexture->GetImage();
		createInfo.pDepthStencilImage = mOpaquePass->GetDepthStencilTexture()->GetImage();
		createInfo.depthStencilState = vkr::ResourceState::DepthStencilWrite;
		createInfo.renderTargetClearValues[0] = { 0, 0, 0, 0 };
		createInfo.depthStencilClearValue = { 1.0f, 0xFF };
		CHECKED_CALL(GetRenderDevice().CreateDrawPass(createInfo, &mTransparencyPass));
	}

	////////////////////////////////////////
	// Composite
	////////////////////////////////////////

	// Descriptor
	{
		vkr::DescriptorSetLayoutCreateInfo layoutCreateInfo = {};
		layoutCreateInfo.bindings.push_back(vkr::DescriptorBinding{ CUSTOM_SAMPLER_0_REGISTER, vkr::DESCRIPTOR_TYPE_SAMPLER, 1, vkr::SHADER_STAGE_ALL_GRAPHICS });
		layoutCreateInfo.bindings.push_back(vkr::DescriptorBinding{ CUSTOM_TEXTURE_0_REGISTER, vkr::DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1, vkr::SHADER_STAGE_ALL_GRAPHICS });
		layoutCreateInfo.bindings.push_back(vkr::DescriptorBinding{ CUSTOM_TEXTURE_1_REGISTER, vkr::DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1, vkr::SHADER_STAGE_ALL_GRAPHICS });
		CHECKED_CALL(GetRenderDevice().CreateDescriptorSetLayout(layoutCreateInfo, &mCompositeDescriptorSetLayout));
		CHECKED_CALL(GetRenderDevice().AllocateDescriptorSet(mDescriptorPool, mCompositeDescriptorSetLayout, &mCompositeDescriptorSet));

		std::array<vkr::WriteDescriptor, 3> writes = {};

		writes[0].binding = CUSTOM_SAMPLER_0_REGISTER;
		writes[0].type = vkr::DESCRIPTOR_TYPE_SAMPLER;
		writes[0].pSampler = mNearestSampler;

		writes[1].binding = CUSTOM_TEXTURE_0_REGISTER;
		writes[1].arrayIndex = 0;
		writes[1].type = vkr::DESCRIPTOR_TYPE_SAMPLED_IMAGE;
		writes[1].pImageView = mOpaquePass->GetRenderTargetTexture(0)->GetSampledImageView();

		writes[2].binding = CUSTOM_TEXTURE_1_REGISTER;
		writes[2].arrayIndex = 0;
		writes[2].type = vkr::DESCRIPTOR_TYPE_SAMPLED_IMAGE;
		writes[2].pImageView = mTransparencyTexture->GetSampledImageView();

		CHECKED_CALL(mCompositeDescriptorSet->UpdateDescriptors(static_cast<uint32_t>(writes.size()), writes.data()));
	}

	// Pipeline
	{
		vkr::PipelineInterfaceCreateInfo piCreateInfo = {};
		piCreateInfo.setCount = 1;
		piCreateInfo.sets[0].set = 0;
		piCreateInfo.sets[0].pLayout = mCompositeDescriptorSetLayout;
		CHECKED_CALL(GetRenderDevice().CreatePipelineInterface(piCreateInfo, &mCompositePipelineInterface));

		vkr::ShaderModulePtr VS, PS;
		CHECKED_CALL(device.CreateShader("basic/shaders/oit_demo", "Composite.vs", &VS));
		CHECKED_CALL(device.CreateShader("basic/shaders/oit_demo", "Composite.ps", &PS));

		vkr::GraphicsPipelineCreateInfo2 gpCreateInfo = {};
		gpCreateInfo.VS = { VS, "vsmain" };
		gpCreateInfo.PS = { PS, "psmain" };
		gpCreateInfo.vertexInputState.bindingCount = 0;
		gpCreateInfo.topology = vkr::PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		gpCreateInfo.polygonMode = vkr::POLYGON_MODE_FILL;
		gpCreateInfo.cullMode = vkr::CULL_MODE_BACK;
		gpCreateInfo.frontFace = vkr::FRONT_FACE_CCW;
		gpCreateInfo.depthReadEnable = false;
		gpCreateInfo.depthWriteEnable = false;
		gpCreateInfo.blendModes[0] = vkr::BLEND_MODE_NONE;
		gpCreateInfo.outputState.renderTargetCount = 1;
		gpCreateInfo.outputState.renderTargetFormats[0] = swapChain.GetColorFormat();
		gpCreateInfo.outputState.depthStencilFormat = swapChain.GetDepthFormat();
		gpCreateInfo.pPipelineInterface = mCompositePipelineInterface;
		CHECKED_CALL(GetRenderDevice().CreateGraphicsPipeline(gpCreateInfo, &mCompositePipeline));

		GetRenderDevice().DestroyShaderModule(VS);
		GetRenderDevice().DestroyShaderModule(PS);
	}
}

Example_024::Algorithm Example_024::GetSelectedAlgorithm() const
{
	return mSupportedAlgorithmIds[mGuiParameters.algorithmDataIndex];
}

vkr::MeshPtr Example_024::GetTransparentMesh() const
{
	return mTransparentMeshes[mGuiParameters.mesh.type];
}

void Example_024::FillSupportedAlgorithmData()
{
	const auto addSupportedAlgorithm = [this](const char* name, Algorithm algorithm) {
		mSupportedAlgorithmNames.push_back(name);
		mSupportedAlgorithmIds.push_back(algorithm);
		ASSERT_MSG(mSupportedAlgorithmNames.size() == mSupportedAlgorithmIds.size(), "supported algorithm data is out-of-sync");
		};

	addSupportedAlgorithm("Unsorted over", ALGORITHM_UNSORTED_OVER);
	addSupportedAlgorithm("Weighted sum", ALGORITHM_WEIGHTED_SUM);
	if (GetRenderDevice().IndependentBlendingSupported()) {
		addSupportedAlgorithm("Weighted average", ALGORITHM_WEIGHTED_AVERAGE);
	}
	addSupportedAlgorithm("Depth peeling", ALGORITHM_DEPTH_PEELING);
	if (GetRenderDevice().FragmentStoresAndAtomicsSupported()) {
		addSupportedAlgorithm("Buffer", ALGORITHM_BUFFER);
	}
}

void Example_024::ParseCommandLineOptions()
{
	const Algorithm defaultAlgorithm = ALGORITHM_UNSORTED_OVER;
	for (size_t i = 0; i < mSupportedAlgorithmIds.size(); ++i)
	{
		if (mSupportedAlgorithmIds[i] == defaultAlgorithm) {
			mGuiParameters.algorithmDataIndex = static_cast<int32_t>(i);
			break;
		}
	}

	mGuiParameters.background.display = true;
	mGuiParameters.background.color[0] = std::clamp(0.51f, 0.0f, 1.0f);
	mGuiParameters.background.color[1] = std::clamp(0.71f, 0.0f, 1.0f);
	mGuiParameters.background.color[2] = std::clamp(0.85f, 0.0f, 1.0f);

	mGuiParameters.mesh.type = static_cast<MeshType>(std::clamp(0, 0, MESH_TYPES_COUNT - 1));
	mGuiParameters.mesh.opacity = std::clamp(1.0f, 0.0f, 1.0f);
	mGuiParameters.mesh.scale = std::clamp(MESH_SCALE_DEFAULT, MESH_SCALE_MIN, MESH_SCALE_MAX);
	mGuiParameters.mesh.auto_rotate = true;

	mGuiParameters.unsortedOver.faceMode = static_cast<FaceMode>(std::clamp(0, 0, FACE_MODES_COUNT - 1));

	mGuiParameters.weightedAverage.type = static_cast<WeightAverageType>(std::clamp(0, 0, WEIGHTED_AVERAGE_TYPES_COUNT - 1));

	mGuiParameters.depthPeeling.startLayer = std::clamp(0, 0, DEPTH_PEELING_LAYERS_COUNT - 1);
	mGuiParameters.depthPeeling.layersCount = std::clamp(DEPTH_PEELING_LAYERS_COUNT, 1, DEPTH_PEELING_LAYERS_COUNT);

	mGuiParameters.buffer.type = static_cast<BufferAlgorithmType>(std::clamp(0, 0, BUFFER_ALGORITHMS_COUNT - 1));
	mGuiParameters.buffer.bucketsFragmentsMaxCount = std::clamp(BUFFER_BUCKETS_SIZE_PER_PIXEL, 1, BUFFER_BUCKETS_SIZE_PER_PIXEL);
	mGuiParameters.buffer.listsFragmentBufferScale = std::clamp(BUFFER_LISTS_FRAGMENT_BUFFER_MAX_SCALE, 1, BUFFER_LISTS_FRAGMENT_BUFFER_MAX_SCALE);
	mGuiParameters.buffer.listsSortedFragmentMaxCount = std::clamp(BUFFER_LISTS_SORTED_FRAGMENT_MAX_COUNT, 1, BUFFER_LISTS_SORTED_FRAGMENT_MAX_COUNT);
}

void Example_024::UpdateGUI()
{
	// GUI
	if (ImGui::Begin("Parameters"))
	{
		ImGui::Combo("Algorithm", &mGuiParameters.algorithmDataIndex, mSupportedAlgorithmNames.data(), static_cast<int>(mSupportedAlgorithmNames.size()));

		ImGui::Separator();
		ImGui::Text("Model");
		const char* const meshesChoices[] =
		{
		"Monkey",
		"Horse",
		"Megaphone",
		"Cannon",
		};
		static_assert(IM_ARRAYSIZE(meshesChoices) == MESH_TYPES_COUNT, "Mesh types count mismatch");
		ImGui::Combo("Mesh", reinterpret_cast<int32_t*>(&mGuiParameters.mesh.type), meshesChoices, IM_ARRAYSIZE(meshesChoices));
		ImGui::SliderFloat("Opacity", &mGuiParameters.mesh.opacity, 0.0f, 1.0f, "%.2f");
		ImGui::SliderFloat("Scale", &mGuiParameters.mesh.scale, MESH_SCALE_MIN, MESH_SCALE_MAX, "%.2f");
		ImGui::Checkbox("Auto rotate", &mGuiParameters.mesh.auto_rotate);

		ImGui::Separator();
		ImGui::Text("Background");
		ImGui::Checkbox("BG display", &mGuiParameters.background.display);
		if (mGuiParameters.background.display) {
			ImGui::ColorPicker3(
				"BG color", mGuiParameters.background.color, ImGuiColorEditFlags_NoAlpha | ImGuiColorEditFlags_NoTooltip | ImGuiColorEditFlags_NoSidePreview | ImGuiColorEditFlags_NoDragDrop | ImGuiColorEditFlags_NoOptions | ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoSmallPreview | ImGuiColorEditFlags_DisplayRGB | ImGuiColorEditFlags_InputRGB);
		}

		ImGui::Separator();

		switch (GetSelectedAlgorithm()) {
		case ALGORITHM_UNSORTED_OVER: {
			ImGui::Text("%s", mSupportedAlgorithmNames[mGuiParameters.algorithmDataIndex]);
			const char* faceModeChoices[] =
			{
			"All",
			"Back first, then front",
			"Back only",
			"Front only",
			};
			static_assert(IM_ARRAYSIZE(faceModeChoices) == FACE_MODES_COUNT, "Face modes count mismatch");
			ImGui::Combo("UO face mode", reinterpret_cast<int32_t*>(&mGuiParameters.unsortedOver.faceMode), faceModeChoices, IM_ARRAYSIZE(faceModeChoices));
			break;
		}
		case ALGORITHM_WEIGHTED_AVERAGE: {
			ImGui::Text("%s", mSupportedAlgorithmNames[mGuiParameters.algorithmDataIndex]);
			const char* typeChoices[] =
			{
			"Fragment count",
			"Exact coverage",
			};
			static_assert(IM_ARRAYSIZE(typeChoices) == WEIGHTED_AVERAGE_TYPES_COUNT, "Weighted average types count mismatch");
			ImGui::Combo("WA Type", reinterpret_cast<int32_t*>(&mGuiParameters.weightedAverage.type), typeChoices, IM_ARRAYSIZE(typeChoices));
			break;
		}
		case ALGORITHM_DEPTH_PEELING: {
			ImGui::Text("%s", mSupportedAlgorithmNames[mGuiParameters.algorithmDataIndex]);
			ImGui::SliderInt("DP first layer", &mGuiParameters.depthPeeling.startLayer, 0, DEPTH_PEELING_LAYERS_COUNT - 1);
			ImGui::SliderInt("DP layers count", &mGuiParameters.depthPeeling.layersCount, 1, DEPTH_PEELING_LAYERS_COUNT);
			break;
		}
		case ALGORITHM_BUFFER: {
			ImGui::Text("%s", mSupportedAlgorithmNames[mGuiParameters.algorithmDataIndex]);
			const char* typeChoices[] =
			{
			"Buckets",
			"Linked list",
			};
			static_assert(IM_ARRAYSIZE(typeChoices) == BUFFER_ALGORITHMS_COUNT, "Buffer algorithm types count mismatch");
			ImGui::Combo("BU type", reinterpret_cast<int32_t*>(&mGuiParameters.buffer.type), typeChoices, IM_ARRAYSIZE(typeChoices));
			switch (mGuiParameters.buffer.type) {
			case BUFFER_ALGORITHM_BUCKETS: {
				ImGui::SliderInt("BU bucket fragments max count", &mGuiParameters.buffer.bucketsFragmentsMaxCount, 1, BUFFER_BUCKETS_SIZE_PER_PIXEL);
				break;
			}
			case BUFFER_ALGORITHM_LINKED_LISTS: {
				ImGui::SliderInt("BU fragment buffer scale", &mGuiParameters.buffer.listsFragmentBufferScale, 1, BUFFER_LISTS_FRAGMENT_BUFFER_MAX_SCALE);
				ImGui::SliderInt("BU linked list max size", &mGuiParameters.buffer.listsSortedFragmentMaxCount, 1, BUFFER_LISTS_SORTED_FRAGMENT_MAX_COUNT);
				break;
			}
			default: {
				break;
			}
			}
			break;
		}
		default: {
			break;
		}
		}
	}
	ImGui::End();
}

void Example_024::RecordOpaque()
{
	mCommandBuffer->TransitionImageLayout(
		mOpaquePass,
		vkr::ResourceState::ShaderResource,
		vkr::ResourceState::RenderTarget,
		vkr::ResourceState::ShaderResource,
		vkr::ResourceState::DepthStencilWrite);
	mCommandBuffer->BeginRenderPass(mOpaquePass, vkr::DRAW_PASS_CLEAR_FLAG_CLEAR_ALL);

	mCommandBuffer->SetScissors(mOpaquePass->GetScissor());
	mCommandBuffer->SetViewports(mOpaquePass->GetViewport());

	if (mGuiParameters.background.display) {
		mCommandBuffer->BindGraphicsDescriptorSets(mOpaquePipelineInterface, 1, &mOpaqueDescriptorSet);
		mCommandBuffer->BindGraphicsPipeline(mOpaquePipeline);
		mCommandBuffer->BindIndexBuffer(mBackgroundMesh);
		mCommandBuffer->BindVertexBuffers(mBackgroundMesh);
		mCommandBuffer->DrawIndexed(mBackgroundMesh->GetIndexCount());
	}

	mCommandBuffer->EndRenderPass();
	mCommandBuffer->TransitionImageLayout(
		mOpaquePass,
		vkr::ResourceState::RenderTarget,
		vkr::ResourceState::ShaderResource,
		vkr::ResourceState::DepthStencilWrite,
		vkr::ResourceState::ShaderResource);
}

void Example_024::RecordTransparency()
{
	void (Example_024:: * recordFuncs[])() =
	{
	&Example_024::RecordUnsortedOver,
	&Example_024::RecordWeightedSum,
	&Example_024::RecordWeightedAverage,
	&Example_024::RecordDepthPeeling,
	&Example_024::RecordBuffer,
	};
	static_assert(sizeof(recordFuncs) / sizeof(recordFuncs[0]) == ALGORITHMS_COUNT, "Algorithm record func count mismatch");

	const Algorithm algorithm = GetSelectedAlgorithm();
	ASSERT_MSG(algorithm >= 0 && algorithm < ALGORITHMS_COUNT, "unknown algorithm");
	(this->*recordFuncs[algorithm])();
}

void Example_024::RecordComposite(vkr::RenderPassPtr renderPass)
{
	ASSERT_MSG(!renderPass.IsNull(), "render pass object is null");

	mCommandBuffer->TransitionImageLayout(renderPass->GetRenderTargetImage(0), ALL_SUBRESOURCES, vkr::ResourceState::Present, vkr::ResourceState::RenderTarget);

	vkr::RenderPassBeginInfo beginInfo = {};
	beginInfo.pRenderPass = renderPass;
	beginInfo.renderArea = renderPass->GetRenderArea();
	beginInfo.RTVClearCount = 1;
	beginInfo.RTVClearValues[0] = { {0, 0, 0, 0} };
	beginInfo.DSVClearValue = { 1.0f, 0xFF };
	mCommandBuffer->BeginRenderPass(&beginInfo);

	mCommandBuffer->SetScissors(renderPass->GetScissor());
	mCommandBuffer->SetViewports(renderPass->GetViewport());

	mCommandBuffer->BindGraphicsDescriptorSets(mCompositePipelineInterface, 1, &mCompositeDescriptorSet);
	mCommandBuffer->BindGraphicsPipeline(mCompositePipeline);
	mCommandBuffer->Draw(3);

	GetRender().DrawImGui(mCommandBuffer);

	mCommandBuffer->EndRenderPass();
	mCommandBuffer->TransitionImageLayout(renderPass->GetRenderTargetImage(0), ALL_SUBRESOURCES, vkr::ResourceState::RenderTarget, vkr::ResourceState::Present);
}

//=============================================================================
// Buffer
//=============================================================================

void Example_024::SetupBufferBuckets()
{
	mBuffer.buckets.countTextureNeedClear = true;

	// Count texture
	{
		vkr::TextureCreateInfo createInfo = {};
		createInfo.imageType = vkr::IMAGE_TYPE_2D;
		createInfo.width = mTransparencyTexture->GetWidth();
		createInfo.height = mTransparencyTexture->GetHeight();
		createInfo.depth = 1;
		createInfo.imageFormat = vkr::FORMAT_R32_UINT;
		createInfo.sampleCount = vkr::SAMPLE_COUNT_1;
		createInfo.mipLevelCount = 1;
		createInfo.arrayLayerCount = 1;
		createInfo.usageFlags.bits.colorAttachment = true;
		createInfo.usageFlags.bits.storage = true;
		createInfo.memoryUsage = vkr::MemoryUsage::GPUOnly;
		createInfo.initialState = vkr::ResourceState::ShaderResource;

		CHECKED_CALL(GetRenderDevice().CreateTexture(createInfo, &mBuffer.buckets.countTexture));
	}

	// Fragment texture
	{
		vkr::TextureCreateInfo createInfo = {};
		createInfo.imageType = vkr::IMAGE_TYPE_2D;
		createInfo.width = mBuffer.buckets.countTexture->GetWidth();
		createInfo.height = mBuffer.buckets.countTexture->GetHeight() * BUFFER_BUCKETS_SIZE_PER_PIXEL;
		createInfo.depth = 1;
		createInfo.imageFormat = vkr::FORMAT_R32G32_UINT;
		createInfo.sampleCount = vkr::SAMPLE_COUNT_1;
		createInfo.mipLevelCount = 1;
		createInfo.arrayLayerCount = 1;
		createInfo.usageFlags.bits.storage = true;
		createInfo.memoryUsage = vkr::MemoryUsage::GPUOnly;
		createInfo.initialState = vkr::ResourceState::ShaderResource;

		CHECKED_CALL(GetRenderDevice().CreateTexture(createInfo, &mBuffer.buckets.fragmentTexture));
	}

	// Clear pass
	{
		vkr::DrawPassCreateInfo2 createInfo = {};
		createInfo.width = mBuffer.buckets.countTexture->GetWidth();
		createInfo.height = mBuffer.buckets.countTexture->GetHeight();
		createInfo.renderTargetCount = 1;
		createInfo.pRenderTargetImages[0] = mBuffer.buckets.countTexture->GetImage();
		createInfo.pDepthStencilImage = nullptr;
		createInfo.renderTargetClearValues[0] = { 0, 0, 0, 0 };
		CHECKED_CALL(GetRenderDevice().CreateDrawPass(createInfo, &mBuffer.buckets.clearPass));
	}

	// Gather pass
	{
		vkr::DrawPassCreateInfo2 createInfo = {};
		createInfo.width = mBuffer.buckets.countTexture->GetWidth();
		createInfo.height = mBuffer.buckets.countTexture->GetHeight();
		createInfo.renderTargetCount = 0;
		createInfo.pDepthStencilImage = nullptr;
		CHECKED_CALL(GetRenderDevice().CreateDrawPass(createInfo, &mBuffer.buckets.gatherPass));
	}

	////////////////////////////////////////
	// Gather
	////////////////////////////////////////

	// Descriptor
	{
		vkr::DescriptorSetLayoutCreateInfo layoutCreateInfo = {};
		layoutCreateInfo.bindings.push_back(vkr::DescriptorBinding{ SHADER_GLOBALS_REGISTER, vkr::DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, vkr::SHADER_STAGE_ALL_GRAPHICS });
		layoutCreateInfo.bindings.push_back(vkr::DescriptorBinding{ CUSTOM_TEXTURE_0_REGISTER, vkr::DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1, vkr::SHADER_STAGE_ALL_GRAPHICS });
		layoutCreateInfo.bindings.push_back(vkr::DescriptorBinding{ CUSTOM_UAV_0_REGISTER, vkr::DESCRIPTOR_TYPE_STORAGE_IMAGE, 1, vkr::SHADER_STAGE_ALL_GRAPHICS });
		layoutCreateInfo.bindings.push_back(vkr::DescriptorBinding{ CUSTOM_UAV_1_REGISTER, vkr::DESCRIPTOR_TYPE_STORAGE_IMAGE, 1, vkr::SHADER_STAGE_ALL_GRAPHICS });
		CHECKED_CALL(GetRenderDevice().CreateDescriptorSetLayout(layoutCreateInfo, &mBuffer.buckets.gatherDescriptorSetLayout));

		CHECKED_CALL(GetRenderDevice().AllocateDescriptorSet(mDescriptorPool, mBuffer.buckets.gatherDescriptorSetLayout, &mBuffer.buckets.gatherDescriptorSet));

		std::array<vkr::WriteDescriptor, 4> writes = {};

		writes[0].binding = SHADER_GLOBALS_REGISTER;
		writes[0].type = vkr::DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		writes[0].bufferOffset = 0;
		writes[0].bufferRange = WHOLE_SIZE;
		writes[0].pBuffer = mShaderGlobalsBuffer;

		writes[1].binding = CUSTOM_TEXTURE_0_REGISTER;
		writes[1].arrayIndex = 0;
		writes[1].type = vkr::DESCRIPTOR_TYPE_SAMPLED_IMAGE;
		writes[1].pImageView = mOpaquePass->GetDepthStencilTexture()->GetSampledImageView();

		writes[2].binding = CUSTOM_UAV_0_REGISTER;
		writes[2].arrayIndex = 0;
		writes[2].type = vkr::DESCRIPTOR_TYPE_STORAGE_IMAGE;
		writes[2].pImageView = mBuffer.buckets.countTexture->GetStorageImageView();

		writes[3].binding = CUSTOM_UAV_1_REGISTER;
		writes[3].arrayIndex = 0;
		writes[3].type = vkr::DESCRIPTOR_TYPE_STORAGE_IMAGE;
		writes[3].pImageView = mBuffer.buckets.fragmentTexture->GetStorageImageView();

		CHECKED_CALL(mBuffer.buckets.gatherDescriptorSet->UpdateDescriptors(static_cast<uint32_t>(writes.size()), writes.data()));
	}

	// Pipeline
	{
		vkr::PipelineInterfaceCreateInfo piCreateInfo = {};
		piCreateInfo.setCount = 1;
		piCreateInfo.sets[0].set = 0;
		piCreateInfo.sets[0].pLayout = mBuffer.buckets.gatherDescriptorSetLayout;
		CHECKED_CALL(GetRenderDevice().CreatePipelineInterface(piCreateInfo, &mBuffer.buckets.gatherPipelineInterface));

		vkr::ShaderModulePtr VS, PS;
		CHECKED_CALL(GetRenderDevice().CreateShader("basic/shaders/oit_demo", "BufferBucketsGather.vs", &VS));
		CHECKED_CALL(GetRenderDevice().CreateShader("basic/shaders/oit_demo", "BufferBucketsGather.ps", &PS));

		vkr::GraphicsPipelineCreateInfo2 gpCreateInfo = {};
		gpCreateInfo.VS = { VS, "vsmain" };
		gpCreateInfo.PS = { PS, "psmain" };
		gpCreateInfo.vertexInputState.bindingCount = 1;
		gpCreateInfo.vertexInputState.bindings[0] = GetTransparentMesh()->GetDerivedVertexBindings()[0];
		gpCreateInfo.topology = vkr::PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		gpCreateInfo.polygonMode = vkr::POLYGON_MODE_FILL;
		gpCreateInfo.cullMode = vkr::CULL_MODE_NONE;
		gpCreateInfo.frontFace = vkr::FRONT_FACE_CCW;
		gpCreateInfo.depthReadEnable = false;
		gpCreateInfo.depthWriteEnable = false;
		gpCreateInfo.blendModes[0] = vkr::BLEND_MODE_NONE;
		gpCreateInfo.outputState.renderTargetCount = 0;
		gpCreateInfo.pPipelineInterface = mBuffer.buckets.gatherPipelineInterface;
		CHECKED_CALL(GetRenderDevice().CreateGraphicsPipeline(gpCreateInfo, &mBuffer.buckets.gatherPipeline));

		GetRenderDevice().DestroyShaderModule(VS);
		GetRenderDevice().DestroyShaderModule(PS);
	}

	////////////////////////////////////////
	// Combine
	////////////////////////////////////////

	// Descriptor
	{
		vkr::DescriptorSetLayoutCreateInfo layoutCreateInfo = {};
		layoutCreateInfo.bindings.push_back(vkr::DescriptorBinding{ SHADER_GLOBALS_REGISTER, vkr::DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, vkr::SHADER_STAGE_ALL_GRAPHICS });
		layoutCreateInfo.bindings.push_back(vkr::DescriptorBinding{ CUSTOM_UAV_0_REGISTER, vkr::DESCRIPTOR_TYPE_STORAGE_IMAGE, 1, vkr::SHADER_STAGE_ALL_GRAPHICS });
		layoutCreateInfo.bindings.push_back(vkr::DescriptorBinding{ CUSTOM_UAV_1_REGISTER, vkr::DESCRIPTOR_TYPE_STORAGE_IMAGE, 1, vkr::SHADER_STAGE_ALL_GRAPHICS });
		CHECKED_CALL(GetRenderDevice().CreateDescriptorSetLayout(layoutCreateInfo, &mBuffer.buckets.combineDescriptorSetLayout));

		CHECKED_CALL(GetRenderDevice().AllocateDescriptorSet(mDescriptorPool, mBuffer.buckets.combineDescriptorSetLayout, &mBuffer.buckets.combineDescriptorSet));

		std::array<vkr::WriteDescriptor, 3> writes = {};

		writes[0].binding = SHADER_GLOBALS_REGISTER;
		writes[0].type = vkr::DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		writes[0].bufferOffset = 0;
		writes[0].bufferRange = WHOLE_SIZE;
		writes[0].pBuffer = mShaderGlobalsBuffer;

		writes[1].binding = CUSTOM_UAV_0_REGISTER;
		writes[1].arrayIndex = 0;
		writes[1].type = vkr::DESCRIPTOR_TYPE_STORAGE_IMAGE;
		writes[1].pImageView = mBuffer.buckets.countTexture->GetStorageImageView();

		writes[2].binding = CUSTOM_UAV_1_REGISTER;
		writes[2].arrayIndex = 0;
		writes[2].type = vkr::DESCRIPTOR_TYPE_STORAGE_IMAGE;
		writes[2].pImageView = mBuffer.buckets.fragmentTexture->GetStorageImageView();

		CHECKED_CALL(mBuffer.buckets.combineDescriptorSet->UpdateDescriptors(static_cast<uint32_t>(writes.size()), writes.data()));
	}

	// Pipeline
	{
		vkr::PipelineInterfaceCreateInfo piCreateInfo = {};
		piCreateInfo.setCount = 1;
		piCreateInfo.sets[0].set = 0;
		piCreateInfo.sets[0].pLayout = mBuffer.buckets.combineDescriptorSetLayout;
		CHECKED_CALL(GetRenderDevice().CreatePipelineInterface(piCreateInfo, &mBuffer.buckets.combinePipelineInterface));

		vkr::ShaderModulePtr VS, PS;
		CHECKED_CALL(GetRenderDevice().CreateShader("basic/shaders/oit_demo", "BufferBucketsCombine.vs", &VS));
		CHECKED_CALL(GetRenderDevice().CreateShader("basic/shaders/oit_demo", "BufferBucketsCombine.ps", &PS));

		vkr::GraphicsPipelineCreateInfo2 gpCreateInfo = {};
		gpCreateInfo.VS = { VS, "vsmain" };
		gpCreateInfo.PS = { PS, "psmain" };
		gpCreateInfo.vertexInputState.bindingCount = 0;
		gpCreateInfo.topology = vkr::PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		gpCreateInfo.polygonMode = vkr::POLYGON_MODE_FILL;
		gpCreateInfo.cullMode = vkr::CULL_MODE_BACK;
		gpCreateInfo.frontFace = vkr::FRONT_FACE_CCW;
		gpCreateInfo.depthReadEnable = false;
		gpCreateInfo.depthWriteEnable = false;
		gpCreateInfo.blendModes[0] = vkr::BLEND_MODE_NONE;
		gpCreateInfo.outputState.renderTargetCount = 1;
		gpCreateInfo.outputState.renderTargetFormats[0] = mTransparencyPass->GetRenderTargetTexture(0)->GetImageFormat();
		gpCreateInfo.outputState.depthStencilFormat = mTransparencyPass->GetDepthStencilTexture()->GetImageFormat();
		gpCreateInfo.pPipelineInterface = mBuffer.buckets.combinePipelineInterface;
		CHECKED_CALL(GetRenderDevice().CreateGraphicsPipeline(gpCreateInfo, &mBuffer.buckets.combinePipeline));

		GetRenderDevice().DestroyShaderModule(VS);
		GetRenderDevice().DestroyShaderModule(PS);
	}
}

void Example_024::SetupBufferLinkedLists()
{
	mBuffer.lists.linkedListHeadTextureNeedClear = true;

	// Linked list head texture
	{
		vkr::TextureCreateInfo createInfo = {};
		createInfo.imageType = vkr::IMAGE_TYPE_2D;
		createInfo.width = mTransparencyTexture->GetWidth();
		createInfo.height = mTransparencyTexture->GetHeight();
		createInfo.depth = 1;
		createInfo.imageFormat = vkr::FORMAT_R32_UINT;
		createInfo.sampleCount = vkr::SAMPLE_COUNT_1;
		createInfo.mipLevelCount = 1;
		createInfo.arrayLayerCount = 1;
		createInfo.usageFlags.bits.colorAttachment = true;
		createInfo.usageFlags.bits.storage = true;
		createInfo.memoryUsage = vkr::MemoryUsage::GPUOnly;
		createInfo.initialState = vkr::ResourceState::ShaderResource;

		CHECKED_CALL(GetRenderDevice().CreateTexture(createInfo, &mBuffer.lists.linkedListHeadTexture));
	}

	const uint32_t fragmentBufferElementCount = mTransparencyTexture->GetWidth() * mTransparencyTexture->GetHeight() * BUFFER_LISTS_FRAGMENT_BUFFER_MAX_SCALE;
	const uint32_t fragmentBufferElementSize = static_cast<uint32_t>(sizeof(uint4));
	const uint32_t fragmentBufferSize = fragmentBufferElementCount * fragmentBufferElementSize;

	// Fragment buffer
	{
		vkr::BufferCreateInfo bufferCreateInfo = {};
		bufferCreateInfo.size = fragmentBufferSize;
		bufferCreateInfo.structuredElementStride = fragmentBufferElementSize;
		bufferCreateInfo.usageFlags.bits.rwStructuredBuffer = true;
		bufferCreateInfo.memoryUsage = vkr::MemoryUsage::GPUOnly;
		bufferCreateInfo.initialState = vkr::ResourceState::General;
		CHECKED_CALL(GetRenderDevice().CreateBuffer(bufferCreateInfo, &mBuffer.lists.fragmentBuffer));
	}

	// Atomic counter
	{
		vkr::BufferCreateInfo bufferCreateInfo = {};
		bufferCreateInfo.size = std::max(sizeof(uint), static_cast<size_t>(vkr::MINIMUM_UNIFORM_BUFFER_SIZE));
		bufferCreateInfo.structuredElementStride = sizeof(uint);
		bufferCreateInfo.usageFlags.bits.rwStructuredBuffer = true;
		bufferCreateInfo.memoryUsage = vkr::MemoryUsage::GPUOnly;
		bufferCreateInfo.initialState = vkr::ResourceState::General;
		CHECKED_CALL(GetRenderDevice().CreateBuffer(bufferCreateInfo, &mBuffer.lists.atomicCounter));
	}

	// Clear pass
	{
		constexpr uint            clearValueUint = BUFFER_LISTS_INVALID_INDEX;
		const float               clearValueFloat = *reinterpret_cast<const float*>(&clearValueUint);
		vkr::DrawPassCreateInfo2 createInfo = {};
		createInfo.width = mBuffer.lists.linkedListHeadTexture->GetWidth();
		createInfo.height = mBuffer.lists.linkedListHeadTexture->GetHeight();
		createInfo.renderTargetCount = 1;
		createInfo.pRenderTargetImages[0] = mBuffer.lists.linkedListHeadTexture->GetImage();
		createInfo.pDepthStencilImage = nullptr;
		createInfo.renderTargetClearValues[0] = { clearValueFloat, 0, 0, 0 };
		CHECKED_CALL(GetRenderDevice().CreateDrawPass(createInfo, &mBuffer.lists.clearPass));
	}

	// Gather pass
	{
		vkr::DrawPassCreateInfo2 createInfo = {};
		createInfo.width = mBuffer.lists.linkedListHeadTexture->GetWidth();
		createInfo.height = mBuffer.lists.linkedListHeadTexture->GetHeight();
		createInfo.renderTargetCount = 0;
		createInfo.pDepthStencilImage = nullptr;
		createInfo.renderTargetClearValues[0] = { 0, 0, 0, 0 };
		CHECKED_CALL(GetRenderDevice().CreateDrawPass(createInfo, &mBuffer.lists.gatherPass));
	}

	////////////////////////////////////////
	// Gather
	////////////////////////////////////////

	// Descriptor
	{
		vkr::DescriptorSetLayoutCreateInfo layoutCreateInfo = {};
		layoutCreateInfo.bindings.push_back(vkr::DescriptorBinding{ SHADER_GLOBALS_REGISTER, vkr::DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, vkr::SHADER_STAGE_ALL_GRAPHICS });
		layoutCreateInfo.bindings.push_back(vkr::DescriptorBinding{ CUSTOM_TEXTURE_0_REGISTER, vkr::DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1, vkr::SHADER_STAGE_ALL_GRAPHICS });
		layoutCreateInfo.bindings.push_back(vkr::DescriptorBinding{ CUSTOM_UAV_0_REGISTER, vkr::DESCRIPTOR_TYPE_STORAGE_IMAGE, 1, vkr::SHADER_STAGE_ALL_GRAPHICS });
		layoutCreateInfo.bindings.push_back(vkr::DescriptorBinding{ CUSTOM_UAV_1_REGISTER, vkr::DESCRIPTOR_TYPE_RW_STRUCTURED_BUFFER, 1, vkr::SHADER_STAGE_ALL_GRAPHICS });
		layoutCreateInfo.bindings.push_back(vkr::DescriptorBinding{ CUSTOM_UAV_2_REGISTER, vkr::DESCRIPTOR_TYPE_RW_STRUCTURED_BUFFER, 1, vkr::SHADER_STAGE_ALL_GRAPHICS });
		CHECKED_CALL(GetRenderDevice().CreateDescriptorSetLayout(layoutCreateInfo, &mBuffer.lists.gatherDescriptorSetLayout));

		CHECKED_CALL(GetRenderDevice().AllocateDescriptorSet(mDescriptorPool, mBuffer.lists.gatherDescriptorSetLayout, &mBuffer.lists.gatherDescriptorSet));

		std::array<vkr::WriteDescriptor, 5> writes = {};

		writes[0].binding = SHADER_GLOBALS_REGISTER;
		writes[0].type = vkr::DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		writes[0].bufferOffset = 0;
		writes[0].bufferRange = WHOLE_SIZE;
		writes[0].pBuffer = mShaderGlobalsBuffer;

		writes[1].binding = CUSTOM_TEXTURE_0_REGISTER;
		writes[1].arrayIndex = 0;
		writes[1].type = vkr::DESCRIPTOR_TYPE_SAMPLED_IMAGE;
		writes[1].pImageView = mOpaquePass->GetDepthStencilTexture()->GetSampledImageView();

		writes[2].binding = CUSTOM_UAV_0_REGISTER;
		writes[2].arrayIndex = 0;
		writes[2].type = vkr::DESCRIPTOR_TYPE_STORAGE_IMAGE;
		writes[2].pImageView = mBuffer.lists.linkedListHeadTexture->GetStorageImageView();

		writes[3].binding = CUSTOM_UAV_1_REGISTER;
		writes[3].type = vkr::DESCRIPTOR_TYPE_RW_STRUCTURED_BUFFER;
		writes[3].bufferOffset = 0;
		writes[3].bufferRange = WHOLE_SIZE;
		writes[3].structuredElementCount = fragmentBufferElementCount;
		writes[3].pBuffer = mBuffer.lists.fragmentBuffer;

		writes[4].binding = CUSTOM_UAV_2_REGISTER;
		writes[4].type = vkr::DESCRIPTOR_TYPE_RW_STRUCTURED_BUFFER;
		writes[4].bufferOffset = 0;
		writes[4].bufferRange = WHOLE_SIZE;
		writes[4].structuredElementCount = 1;
		writes[4].pBuffer = mBuffer.lists.atomicCounter;

		CHECKED_CALL(mBuffer.lists.gatherDescriptorSet->UpdateDescriptors(static_cast<uint32_t>(writes.size()), writes.data()));
	}

	// Pipeline
	{
		vkr::PipelineInterfaceCreateInfo piCreateInfo = {};
		piCreateInfo.setCount = 1;
		piCreateInfo.sets[0].set = 0;
		piCreateInfo.sets[0].pLayout = mBuffer.lists.gatherDescriptorSetLayout;
		CHECKED_CALL(GetRenderDevice().CreatePipelineInterface(piCreateInfo, &mBuffer.lists.gatherPipelineInterface));

		vkr::ShaderModulePtr VS, PS;
		CHECKED_CALL(GetRenderDevice().CreateShader("basic/shaders/oit_demo", "BufferLinkedListsGather.vs", &VS));
		CHECKED_CALL(GetRenderDevice().CreateShader("basic/shaders/oit_demo", "BufferLinkedListsGather.ps", &PS));

		vkr::GraphicsPipelineCreateInfo2 gpCreateInfo = {};
		gpCreateInfo.VS = { VS, "vsmain" };
		gpCreateInfo.PS = { PS, "psmain" };
		gpCreateInfo.vertexInputState.bindingCount = 1;
		gpCreateInfo.vertexInputState.bindings[0] = GetTransparentMesh()->GetDerivedVertexBindings()[0];
		gpCreateInfo.topology = vkr::PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		gpCreateInfo.polygonMode = vkr::POLYGON_MODE_FILL;
		gpCreateInfo.cullMode = vkr::CULL_MODE_NONE;
		gpCreateInfo.frontFace = vkr::FRONT_FACE_CCW;
		gpCreateInfo.depthReadEnable = false;
		gpCreateInfo.depthWriteEnable = false;
		gpCreateInfo.blendModes[0] = vkr::BLEND_MODE_NONE;
		gpCreateInfo.outputState.renderTargetCount = 0;
		gpCreateInfo.pPipelineInterface = mBuffer.lists.gatherPipelineInterface;
		CHECKED_CALL(GetRenderDevice().CreateGraphicsPipeline(gpCreateInfo, &mBuffer.lists.gatherPipeline));

		GetRenderDevice().DestroyShaderModule(VS);
		GetRenderDevice().DestroyShaderModule(PS);
	}

	////////////////////////////////////////
	// Combine
	////////////////////////////////////////

	// Descriptor
	{
		vkr::DescriptorSetLayoutCreateInfo layoutCreateInfo = {};
		layoutCreateInfo.bindings.push_back(vkr::DescriptorBinding{ SHADER_GLOBALS_REGISTER, vkr::DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, vkr::SHADER_STAGE_ALL_GRAPHICS });
		layoutCreateInfo.bindings.push_back(vkr::DescriptorBinding{ CUSTOM_UAV_0_REGISTER, vkr::DESCRIPTOR_TYPE_STORAGE_IMAGE, 1, vkr::SHADER_STAGE_ALL_GRAPHICS });
		layoutCreateInfo.bindings.push_back(vkr::DescriptorBinding{ CUSTOM_UAV_1_REGISTER, vkr::DESCRIPTOR_TYPE_RW_STRUCTURED_BUFFER, 1, vkr::SHADER_STAGE_ALL_GRAPHICS });
		layoutCreateInfo.bindings.push_back(vkr::DescriptorBinding{ CUSTOM_UAV_2_REGISTER, vkr::DESCRIPTOR_TYPE_RW_STRUCTURED_BUFFER, 1, vkr::SHADER_STAGE_ALL_GRAPHICS });
		CHECKED_CALL(GetRenderDevice().CreateDescriptorSetLayout(layoutCreateInfo, &mBuffer.lists.combineDescriptorSetLayout));

		CHECKED_CALL(GetRenderDevice().AllocateDescriptorSet(mDescriptorPool, mBuffer.lists.combineDescriptorSetLayout, &mBuffer.lists.combineDescriptorSet));

		std::array<vkr::WriteDescriptor, 4> writes = {};

		writes[0].binding = SHADER_GLOBALS_REGISTER;
		writes[0].type = vkr::DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		writes[0].bufferOffset = 0;
		writes[0].bufferRange = WHOLE_SIZE;
		writes[0].pBuffer = mShaderGlobalsBuffer;

		writes[1].binding = CUSTOM_UAV_0_REGISTER;
		writes[1].arrayIndex = 0;
		writes[1].type = vkr::DESCRIPTOR_TYPE_STORAGE_IMAGE;
		writes[1].pImageView = mBuffer.lists.linkedListHeadTexture->GetStorageImageView();

		writes[2].binding = CUSTOM_UAV_1_REGISTER;
		writes[2].type = vkr::DESCRIPTOR_TYPE_RW_STRUCTURED_BUFFER;
		writes[2].bufferOffset = 0;
		writes[2].bufferRange = WHOLE_SIZE;
		writes[2].structuredElementCount = fragmentBufferElementCount;
		writes[2].pBuffer = mBuffer.lists.fragmentBuffer;

		writes[3].binding = CUSTOM_UAV_2_REGISTER;
		writes[3].type = vkr::DESCRIPTOR_TYPE_RW_STRUCTURED_BUFFER;
		writes[3].bufferOffset = 0;
		writes[3].bufferRange = WHOLE_SIZE;
		writes[3].structuredElementCount = 1;
		writes[3].pBuffer = mBuffer.lists.atomicCounter;

		CHECKED_CALL(mBuffer.lists.combineDescriptorSet->UpdateDescriptors(static_cast<uint32_t>(writes.size()), writes.data()));
	}

	// Pipeline
	{
		vkr::PipelineInterfaceCreateInfo piCreateInfo = {};
		piCreateInfo.setCount = 1;
		piCreateInfo.sets[0].set = 0;
		piCreateInfo.sets[0].pLayout = mBuffer.lists.combineDescriptorSetLayout;
		CHECKED_CALL(GetRenderDevice().CreatePipelineInterface(piCreateInfo, &mBuffer.lists.combinePipelineInterface));

		vkr::ShaderModulePtr VS, PS;
		CHECKED_CALL(GetRenderDevice().CreateShader("basic/shaders/oit_demo", "BufferLinkedListsCombine.vs", &VS));
		CHECKED_CALL(GetRenderDevice().CreateShader("basic/shaders/oit_demo", "BufferLinkedListsCombine.ps", &PS));

		vkr::GraphicsPipelineCreateInfo2 gpCreateInfo = {};
		gpCreateInfo.VS = { VS, "vsmain" };
		gpCreateInfo.PS = { PS, "psmain" };
		gpCreateInfo.vertexInputState.bindingCount = 0;
		gpCreateInfo.topology = vkr::PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		gpCreateInfo.polygonMode = vkr::POLYGON_MODE_FILL;
		gpCreateInfo.cullMode = vkr::CULL_MODE_BACK;
		gpCreateInfo.frontFace = vkr::FRONT_FACE_CCW;
		gpCreateInfo.depthReadEnable = false;
		gpCreateInfo.depthWriteEnable = false;
		gpCreateInfo.blendModes[0] = vkr::BLEND_MODE_NONE;
		gpCreateInfo.outputState.renderTargetCount = 1;
		gpCreateInfo.outputState.renderTargetFormats[0] = mTransparencyPass->GetRenderTargetTexture(0)->GetImageFormat();
		gpCreateInfo.outputState.depthStencilFormat = mTransparencyPass->GetDepthStencilTexture()->GetImageFormat();
		gpCreateInfo.pPipelineInterface = mBuffer.lists.combinePipelineInterface;
		CHECKED_CALL(GetRenderDevice().CreateGraphicsPipeline(gpCreateInfo, &mBuffer.lists.combinePipeline));

		GetRenderDevice().DestroyShaderModule(VS);
		GetRenderDevice().DestroyShaderModule(PS);
	}
}

void Example_024::SetupBuffer()
{
	SetupBufferBuckets();
	SetupBufferLinkedLists();
}

void Example_024::RecordBufferBuckets()
{
	if (mBuffer.buckets.countTextureNeedClear) {
		mCommandBuffer->TransitionImageLayout(
			mBuffer.buckets.clearPass,
			vkr::ResourceState::ShaderResource,
			vkr::ResourceState::RenderTarget,
			vkr::ResourceState::ShaderResource,
			vkr::ResourceState::ShaderResource);
		mCommandBuffer->BeginRenderPass(mBuffer.buckets.clearPass, vkr::DRAW_PASS_CLEAR_FLAG_CLEAR_ALL);

		mCommandBuffer->SetScissors(mBuffer.buckets.clearPass->GetScissor());
		mCommandBuffer->SetViewports(mBuffer.buckets.clearPass->GetViewport());

		mCommandBuffer->EndRenderPass();
		mCommandBuffer->TransitionImageLayout(
			mBuffer.buckets.clearPass,
			vkr::ResourceState::RenderTarget,
			vkr::ResourceState::ShaderResource,
			vkr::ResourceState::ShaderResource,
			vkr::ResourceState::ShaderResource);

		mBuffer.buckets.countTextureNeedClear = false;
	}

	{
		mCommandBuffer->TransitionImageLayout(mBuffer.buckets.countTexture, 0, 1, 0, 1, vkr::ResourceState::ShaderResource, vkr::ResourceState::General);
		mCommandBuffer->TransitionImageLayout(mBuffer.buckets.fragmentTexture, 0, 1, 0, 1, vkr::ResourceState::ShaderResource, vkr::ResourceState::General);
		mCommandBuffer->BeginRenderPass(mBuffer.buckets.gatherPass, 0);

		mCommandBuffer->SetScissors(mBuffer.buckets.gatherPass->GetScissor());
		mCommandBuffer->SetViewports(mBuffer.buckets.gatherPass->GetViewport());

		mCommandBuffer->BindGraphicsDescriptorSets(mBuffer.buckets.gatherPipelineInterface, 1, &mBuffer.buckets.gatherDescriptorSet);
		mCommandBuffer->BindGraphicsPipeline(mBuffer.buckets.gatherPipeline);
		mCommandBuffer->BindIndexBuffer(GetTransparentMesh());
		mCommandBuffer->BindVertexBuffers(GetTransparentMesh());
		mCommandBuffer->DrawIndexed(GetTransparentMesh()->GetIndexCount());

		mCommandBuffer->EndRenderPass();
		mCommandBuffer->TransitionImageLayout(mBuffer.buckets.countTexture, 0, 1, 0, 1, vkr::ResourceState::General, vkr::ResourceState::ShaderResource);
		mCommandBuffer->TransitionImageLayout(mBuffer.buckets.fragmentTexture, 0, 1, 0, 1, vkr::ResourceState::General, vkr::ResourceState::ShaderResource);
	}

	{
		mCommandBuffer->TransitionImageLayout(mBuffer.buckets.countTexture, 0, 1, 0, 1, vkr::ResourceState::ShaderResource, vkr::ResourceState::General);
		mCommandBuffer->TransitionImageLayout(mBuffer.buckets.fragmentTexture, 0, 1, 0, 1, vkr::ResourceState::ShaderResource, vkr::ResourceState::General);
		mCommandBuffer->TransitionImageLayout(
			mTransparencyPass,
			vkr::ResourceState::ShaderResource,
			vkr::ResourceState::RenderTarget,
			vkr::ResourceState::ShaderResource,
			vkr::ResourceState::DepthStencilWrite);
		mCommandBuffer->BeginRenderPass(mTransparencyPass, vkr::DRAW_PASS_CLEAR_FLAG_CLEAR_RENDER_TARGETS);

		mCommandBuffer->SetScissors(mTransparencyPass->GetScissor());
		mCommandBuffer->SetViewports(mTransparencyPass->GetViewport());

		mCommandBuffer->BindGraphicsDescriptorSets(mBuffer.buckets.combinePipelineInterface, 1, &mBuffer.buckets.combineDescriptorSet);
		mCommandBuffer->BindGraphicsPipeline(mBuffer.buckets.combinePipeline);
		mCommandBuffer->Draw(3);

		mCommandBuffer->EndRenderPass();
		mCommandBuffer->TransitionImageLayout(
			mTransparencyPass,
			vkr::ResourceState::RenderTarget,
			vkr::ResourceState::ShaderResource,
			vkr::ResourceState::DepthStencilWrite,
			vkr::ResourceState::ShaderResource);
		mCommandBuffer->TransitionImageLayout(mBuffer.buckets.countTexture, 0, 1, 0, 1, vkr::ResourceState::General, vkr::ResourceState::ShaderResource);
		mCommandBuffer->TransitionImageLayout(mBuffer.buckets.fragmentTexture, 0, 1, 0, 1, vkr::ResourceState::General, vkr::ResourceState::ShaderResource);
	}
}

void Example_024::RecordBufferLinkedLists()
{
	if (mBuffer.lists.linkedListHeadTextureNeedClear) {
		mCommandBuffer->TransitionImageLayout(
			mBuffer.lists.clearPass,
			vkr::ResourceState::ShaderResource,
			vkr::ResourceState::RenderTarget,
			vkr::ResourceState::ShaderResource,
			vkr::ResourceState::ShaderResource);
		mCommandBuffer->BeginRenderPass(mBuffer.lists.clearPass, vkr::DRAW_PASS_CLEAR_FLAG_CLEAR_ALL);

		mCommandBuffer->SetScissors(mBuffer.lists.clearPass->GetScissor());
		mCommandBuffer->SetViewports(mBuffer.lists.clearPass->GetViewport());

		mCommandBuffer->EndRenderPass();
		mCommandBuffer->TransitionImageLayout(
			mBuffer.lists.clearPass,
			vkr::ResourceState::RenderTarget,
			vkr::ResourceState::ShaderResource,
			vkr::ResourceState::ShaderResource,
			vkr::ResourceState::ShaderResource);

		mBuffer.lists.linkedListHeadTextureNeedClear = false;
	}

	{
		mCommandBuffer->TransitionImageLayout(mBuffer.lists.linkedListHeadTexture, 0, 1, 0, 1, vkr::ResourceState::ShaderResource, vkr::ResourceState::General);
		mCommandBuffer->BeginRenderPass(mBuffer.lists.gatherPass, 0);

		mCommandBuffer->SetScissors(mBuffer.lists.gatherPass->GetScissor());
		mCommandBuffer->SetViewports(mBuffer.lists.gatherPass->GetViewport());

		mCommandBuffer->BindGraphicsDescriptorSets(mBuffer.lists.gatherPipelineInterface, 1, &mBuffer.lists.gatherDescriptorSet);
		mCommandBuffer->BindGraphicsPipeline(mBuffer.lists.gatherPipeline);
		mCommandBuffer->BindIndexBuffer(GetTransparentMesh());
		mCommandBuffer->BindVertexBuffers(GetTransparentMesh());
		mCommandBuffer->DrawIndexed(GetTransparentMesh()->GetIndexCount());

		mCommandBuffer->EndRenderPass();
		mCommandBuffer->TransitionImageLayout(mBuffer.lists.linkedListHeadTexture, 0, 1, 0, 1, vkr::ResourceState::General, vkr::ResourceState::ShaderResource);
	}

	{
		mCommandBuffer->TransitionImageLayout(mBuffer.lists.linkedListHeadTexture, 0, 1, 0, 1, vkr::ResourceState::ShaderResource, vkr::ResourceState::General);
		mCommandBuffer->TransitionImageLayout(
			mTransparencyPass,
			vkr::ResourceState::ShaderResource,
			vkr::ResourceState::RenderTarget,
			vkr::ResourceState::ShaderResource,
			vkr::ResourceState::DepthStencilWrite);
		mCommandBuffer->BeginRenderPass(mTransparencyPass, vkr::DRAW_PASS_CLEAR_FLAG_CLEAR_RENDER_TARGETS);

		mCommandBuffer->SetScissors(mTransparencyPass->GetScissor());
		mCommandBuffer->SetViewports(mTransparencyPass->GetViewport());

		mCommandBuffer->BindGraphicsDescriptorSets(mBuffer.lists.combinePipelineInterface, 1, &mBuffer.lists.combineDescriptorSet);
		mCommandBuffer->BindGraphicsPipeline(mBuffer.lists.combinePipeline);
		mCommandBuffer->Draw(3);

		mCommandBuffer->EndRenderPass();
		mCommandBuffer->TransitionImageLayout(
			mTransparencyPass,
			vkr::ResourceState::RenderTarget,
			vkr::ResourceState::ShaderResource,
			vkr::ResourceState::DepthStencilWrite,
			vkr::ResourceState::ShaderResource);
		mCommandBuffer->TransitionImageLayout(mBuffer.lists.linkedListHeadTexture, 0, 1, 0, 1, vkr::ResourceState::General, vkr::ResourceState::ShaderResource);
	}
}

void Example_024::RecordBuffer()
{
	void (Example_024:: * recordFuncs[])() =
	{
	&Example_024::RecordBufferBuckets,
	&Example_024::RecordBufferLinkedLists,
	};
	static_assert(sizeof(recordFuncs) / sizeof(recordFuncs[0]) == BUFFER_ALGORITHMS_COUNT, "Algorithm record func count mismatch");

	ASSERT_MSG(mGuiParameters.buffer.type >= 0 && mGuiParameters.buffer.type < BUFFER_ALGORITHMS_COUNT, "unknown buffer algorithm type");
	(this->*recordFuncs[mGuiParameters.buffer.type])();
}

//=============================================================================
// DepthPeeling
//=============================================================================

void Example_024::SetupDepthPeeling()
{
	// Layer texture
	{
		vkr::TextureCreateInfo createInfo = {};
		createInfo.imageType = vkr::IMAGE_TYPE_2D;
		createInfo.width = mTransparencyTexture->GetWidth();
		createInfo.height = mTransparencyTexture->GetHeight();
		createInfo.depth = 1;
		createInfo.imageFormat = vkr::FORMAT_B8G8R8A8_UNORM;
		createInfo.sampleCount = vkr::SAMPLE_COUNT_1;
		createInfo.mipLevelCount = 1;
		createInfo.arrayLayerCount = 1;
		createInfo.usageFlags.bits.colorAttachment = true;
		createInfo.usageFlags.bits.sampled = true;
		createInfo.memoryUsage = vkr::MemoryUsage::GPUOnly;
		createInfo.initialState = vkr::ResourceState::ShaderResource;

		for (uint32_t i = 0; i < DEPTH_PEELING_LAYERS_COUNT; ++i) {
			CHECKED_CALL(GetRenderDevice().CreateTexture(createInfo, &mDepthPeeling.layerTextures[i]));
		}
	}

	// Depth texture
	{
		vkr::TextureCreateInfo createInfo = {};
		createInfo.imageType = vkr::IMAGE_TYPE_2D;
		createInfo.width = mDepthPeeling.layerTextures[0]->GetWidth();
		createInfo.height = mDepthPeeling.layerTextures[0]->GetHeight();
		createInfo.depth = 1;
		createInfo.imageFormat = mOpaquePass->GetDepthStencilTexture()->GetDepthStencilViewFormat();
		createInfo.sampleCount = vkr::SAMPLE_COUNT_1;
		createInfo.mipLevelCount = 1;
		createInfo.arrayLayerCount = 1;
		createInfo.usageFlags.bits.transferDst = true;
		createInfo.usageFlags.bits.depthStencilAttachment = true;
		createInfo.usageFlags.bits.sampled = true;
		createInfo.memoryUsage = vkr::MemoryUsage::GPUOnly;
		createInfo.initialState = vkr::ResourceState::ShaderResource;

		for (uint32_t i = 0; i < DEPTH_PEELING_DEPTH_TEXTURES_COUNT; ++i) {
			CHECKED_CALL(GetRenderDevice().CreateTexture(createInfo, &mDepthPeeling.depthTextures[i]));
		}
	}

	// Pass
	{
		vkr::DrawPassCreateInfo2 createInfo = {};
		createInfo.width = mDepthPeeling.layerTextures[0]->GetWidth();
		createInfo.height = mDepthPeeling.layerTextures[0]->GetHeight();
		createInfo.renderTargetCount = 1;
		createInfo.depthStencilState = vkr::ResourceState::DepthStencilWrite;
		createInfo.renderTargetClearValues[0] = { 0, 0, 0, 0 };
		createInfo.depthStencilClearValue = { 1.0f, 0xFF };

		for (uint32_t i = 0; i < DEPTH_PEELING_LAYERS_COUNT; ++i) {
			createInfo.pRenderTargetImages[0] = mDepthPeeling.layerTextures[i]->GetImage();
			createInfo.pDepthStencilImage = mDepthPeeling.depthTextures[i % DEPTH_PEELING_DEPTH_TEXTURES_COUNT]->GetImage();
			CHECKED_CALL(GetRenderDevice().CreateDrawPass(createInfo, &mDepthPeeling.layerPasses[i]));
		}
	}

	////////////////////////////////////////
	// Layer
	////////////////////////////////////////

	// Descriptor
	{
		vkr::DescriptorSetLayoutCreateInfo layoutCreateInfo = {};
		layoutCreateInfo.bindings.push_back(vkr::DescriptorBinding{ SHADER_GLOBALS_REGISTER, vkr::DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, vkr::SHADER_STAGE_ALL_GRAPHICS });
		layoutCreateInfo.bindings.push_back(vkr::DescriptorBinding{ CUSTOM_SAMPLER_0_REGISTER, vkr::DESCRIPTOR_TYPE_SAMPLER, 1, vkr::SHADER_STAGE_ALL_GRAPHICS });
		layoutCreateInfo.bindings.push_back(vkr::DescriptorBinding{ CUSTOM_TEXTURE_0_REGISTER, vkr::DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1, vkr::SHADER_STAGE_ALL_GRAPHICS });
		layoutCreateInfo.bindings.push_back(vkr::DescriptorBinding{ CUSTOM_TEXTURE_1_REGISTER, vkr::DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1, vkr::SHADER_STAGE_ALL_GRAPHICS });
		CHECKED_CALL(GetRenderDevice().CreateDescriptorSetLayout(layoutCreateInfo, &mDepthPeeling.layerDescriptorSetLayout));

		for (uint32_t i = 0; i < DEPTH_PEELING_DEPTH_TEXTURES_COUNT; ++i) {
			CHECKED_CALL(GetRenderDevice().AllocateDescriptorSet(mDescriptorPool, mDepthPeeling.layerDescriptorSetLayout, &mDepthPeeling.layerDescriptorSets[i]));

			std::array<vkr::WriteDescriptor, 4> writes = {};

			writes[0].binding = SHADER_GLOBALS_REGISTER;
			writes[0].type = vkr::DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			writes[0].bufferOffset = 0;
			writes[0].bufferRange = WHOLE_SIZE;
			writes[0].pBuffer = mShaderGlobalsBuffer;

			writes[1].binding = CUSTOM_SAMPLER_0_REGISTER;
			writes[1].type = vkr::DESCRIPTOR_TYPE_SAMPLER;
			writes[1].pSampler = mNearestSampler;

			writes[2].binding = CUSTOM_TEXTURE_0_REGISTER;
			writes[2].arrayIndex = 0;
			writes[2].type = vkr::DESCRIPTOR_TYPE_SAMPLED_IMAGE;
			writes[2].pImageView = mOpaquePass->GetDepthStencilTexture()->GetSampledImageView();

			writes[3].binding = CUSTOM_TEXTURE_1_REGISTER;
			writes[3].arrayIndex = 0;
			writes[3].type = vkr::DESCRIPTOR_TYPE_SAMPLED_IMAGE;
			writes[3].pImageView = mDepthPeeling.depthTextures[(i + 1) % DEPTH_PEELING_DEPTH_TEXTURES_COUNT]->GetSampledImageView();

			CHECKED_CALL(mDepthPeeling.layerDescriptorSets[i]->UpdateDescriptors(static_cast<uint32_t>(writes.size()), writes.data()));
		}
	}

	// Pipeline
	{
		vkr::PipelineInterfaceCreateInfo piCreateInfo = {};
		piCreateInfo.setCount = 1;
		piCreateInfo.sets[0].set = 0;
		piCreateInfo.sets[0].pLayout = mDepthPeeling.layerDescriptorSetLayout;
		CHECKED_CALL(GetRenderDevice().CreatePipelineInterface(piCreateInfo, &mDepthPeeling.layerPipelineInterface));

		vkr::GraphicsPipelineCreateInfo2 gpCreateInfo = {};
		gpCreateInfo.vertexInputState.bindingCount = 1;
		gpCreateInfo.vertexInputState.bindings[0] = GetTransparentMesh()->GetDerivedVertexBindings()[0];
		gpCreateInfo.topology = vkr::PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		gpCreateInfo.polygonMode = vkr::POLYGON_MODE_FILL;
		gpCreateInfo.cullMode = vkr::CULL_MODE_NONE;
		gpCreateInfo.frontFace = vkr::FRONT_FACE_CCW;
		gpCreateInfo.depthReadEnable = true;
		gpCreateInfo.depthWriteEnable = true;
		gpCreateInfo.blendModes[0] = vkr::BLEND_MODE_NONE;
		gpCreateInfo.outputState.renderTargetCount = 1;
		gpCreateInfo.outputState.renderTargetFormats[0] = mDepthPeeling.layerTextures[0]->GetImageFormat();
		gpCreateInfo.outputState.depthStencilFormat = mDepthPeeling.depthTextures[0]->GetImageFormat();
		gpCreateInfo.pPipelineInterface = mDepthPeeling.layerPipelineInterface;

		vkr::ShaderModulePtr VS, PS;

		CHECKED_CALL(GetRenderDevice().CreateShader("basic/shaders/oit_demo", "DepthPeelingLayer_First.vs", &VS));
		CHECKED_CALL(GetRenderDevice().CreateShader("basic/shaders/oit_demo", "DepthPeelingLayer_First.ps", &PS));
		gpCreateInfo.VS = { VS, "vsmain" };
		gpCreateInfo.PS = { PS, "psmain" };
		CHECKED_CALL(GetRenderDevice().CreateGraphicsPipeline(gpCreateInfo, &mDepthPeeling.layerPipeline_FirstLayer));
		GetRenderDevice().DestroyShaderModule(VS);
		GetRenderDevice().DestroyShaderModule(PS);

		CHECKED_CALL(GetRenderDevice().CreateShader("basic/shaders/oit_demo", "DepthPeelingLayer_Others.vs", &VS));
		CHECKED_CALL(GetRenderDevice().CreateShader("basic/shaders/oit_demo", "DepthPeelingLayer_Others.ps", &PS));
		gpCreateInfo.VS = { VS, "vsmain" };
		gpCreateInfo.PS = { PS, "psmain" };
		CHECKED_CALL(GetRenderDevice().CreateGraphicsPipeline(gpCreateInfo, &mDepthPeeling.layerPipeline_OtherLayers));
		GetRenderDevice().DestroyShaderModule(VS);
		GetRenderDevice().DestroyShaderModule(PS);
	}

	////////////////////////////////////////
	// Combine
	////////////////////////////////////////

	// Descriptor
	{
		vkr::DescriptorSetLayoutCreateInfo layoutCreateInfo = {};
		layoutCreateInfo.bindings.push_back(vkr::DescriptorBinding{ SHADER_GLOBALS_REGISTER, vkr::DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, vkr::SHADER_STAGE_ALL_GRAPHICS });
		layoutCreateInfo.bindings.push_back(vkr::DescriptorBinding{ CUSTOM_SAMPLER_0_REGISTER, vkr::DESCRIPTOR_TYPE_SAMPLER, 1, vkr::SHADER_STAGE_ALL_GRAPHICS });
		layoutCreateInfo.bindings.push_back(vkr::DescriptorBinding{ CUSTOM_TEXTURE_0_REGISTER, vkr::DESCRIPTOR_TYPE_SAMPLED_IMAGE, DEPTH_PEELING_LAYERS_COUNT, vkr::SHADER_STAGE_ALL_GRAPHICS });
		CHECKED_CALL(GetRenderDevice().CreateDescriptorSetLayout(layoutCreateInfo, &mDepthPeeling.combineDescriptorSetLayout));

		CHECKED_CALL(GetRenderDevice().AllocateDescriptorSet(mDescriptorPool, mDepthPeeling.combineDescriptorSetLayout, &mDepthPeeling.combineDescriptorSet));

		std::array<vkr::WriteDescriptor, 2 + DEPTH_PEELING_LAYERS_COUNT> writes = {};

		writes[0].binding = SHADER_GLOBALS_REGISTER;
		writes[0].type = vkr::DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		writes[0].bufferOffset = 0;
		writes[0].bufferRange = WHOLE_SIZE;
		writes[0].pBuffer = mShaderGlobalsBuffer;

		writes[1].binding = CUSTOM_SAMPLER_0_REGISTER;
		writes[1].type = vkr::DESCRIPTOR_TYPE_SAMPLER;
		writes[1].pSampler = mNearestSampler;

		for (uint32_t i = 0; i < DEPTH_PEELING_LAYERS_COUNT; ++i) {
			writes[2 + i].binding = CUSTOM_TEXTURE_0_REGISTER;
			writes[2 + i].arrayIndex = i;
			writes[2 + i].type = vkr::DESCRIPTOR_TYPE_SAMPLED_IMAGE;
			writes[2 + i].pImageView = mDepthPeeling.layerTextures[i]->GetSampledImageView();
		}

		CHECKED_CALL(mDepthPeeling.combineDescriptorSet->UpdateDescriptors(static_cast<uint32_t>(writes.size()), writes.data()));
	}

	// Pipeline
	{
		vkr::PipelineInterfaceCreateInfo piCreateInfo = {};
		piCreateInfo.setCount = 1;
		piCreateInfo.sets[0].set = 0;
		piCreateInfo.sets[0].pLayout = mDepthPeeling.combineDescriptorSetLayout;
		CHECKED_CALL(GetRenderDevice().CreatePipelineInterface(piCreateInfo, &mDepthPeeling.combinePipelineInterface));

		vkr::ShaderModulePtr VS, PS;
		CHECKED_CALL(GetRenderDevice().CreateShader("basic/shaders/oit_demo", "DepthPeelingCombine.vs", &VS));
		CHECKED_CALL(GetRenderDevice().CreateShader("basic/shaders/oit_demo", "DepthPeelingCombine.ps", &PS));

		vkr::GraphicsPipelineCreateInfo2 gpCreateInfo = {};
		gpCreateInfo.VS = { VS, "vsmain" };
		gpCreateInfo.PS = { PS, "psmain" };
		gpCreateInfo.vertexInputState.bindingCount = 0;
		gpCreateInfo.topology = vkr::PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		gpCreateInfo.polygonMode = vkr::POLYGON_MODE_FILL;
		gpCreateInfo.cullMode = vkr::CULL_MODE_BACK;
		gpCreateInfo.frontFace = vkr::FRONT_FACE_CCW;
		gpCreateInfo.depthReadEnable = false;
		gpCreateInfo.depthWriteEnable = false;
		gpCreateInfo.blendModes[0] = vkr::BLEND_MODE_NONE;
		gpCreateInfo.outputState.renderTargetCount = 1;
		gpCreateInfo.outputState.renderTargetFormats[0] = mTransparencyTexture->GetImageFormat();
		gpCreateInfo.outputState.depthStencilFormat = mOpaquePass->GetDepthStencilTexture()->GetImageFormat();
		gpCreateInfo.pPipelineInterface = mDepthPeeling.combinePipelineInterface;
		CHECKED_CALL(GetRenderDevice().CreateGraphicsPipeline(gpCreateInfo, &mDepthPeeling.combinePipeline));

		GetRenderDevice().DestroyShaderModule(VS);
		GetRenderDevice().DestroyShaderModule(PS);
	}
}

void Example_024::RecordDepthPeeling()
{
	// Layer passes: extract all layers
	for (uint32_t i = 0; i < DEPTH_PEELING_LAYERS_COUNT; ++i) {
		vkr::DrawPassPtr layerPass = mDepthPeeling.layerPasses[i];
		mCommandBuffer->TransitionImageLayout(
			layerPass,
			vkr::ResourceState::ShaderResource,
			vkr::ResourceState::RenderTarget,
			vkr::ResourceState::ShaderResource,
			vkr::ResourceState::DepthStencilWrite);
		mCommandBuffer->BeginRenderPass(layerPass, vkr::DRAW_PASS_CLEAR_FLAG_CLEAR_ALL);

		mCommandBuffer->SetScissors(layerPass->GetScissor());
		mCommandBuffer->SetViewports(layerPass->GetViewport());

		mCommandBuffer->BindGraphicsDescriptorSets(mDepthPeeling.layerPipelineInterface, 1, &mDepthPeeling.layerDescriptorSets[i % DEPTH_PEELING_DEPTH_TEXTURES_COUNT]);
		mCommandBuffer->BindGraphicsPipeline(i == 0 ? mDepthPeeling.layerPipeline_FirstLayer : mDepthPeeling.layerPipeline_OtherLayers);
		mCommandBuffer->BindIndexBuffer(GetTransparentMesh());
		mCommandBuffer->BindVertexBuffers(GetTransparentMesh());
		mCommandBuffer->DrawIndexed(GetTransparentMesh()->GetIndexCount());

		mCommandBuffer->EndRenderPass();
		mCommandBuffer->TransitionImageLayout(
			layerPass,
			vkr::ResourceState::RenderTarget,
			vkr::ResourceState::ShaderResource,
			vkr::ResourceState::DepthStencilWrite,
			vkr::ResourceState::ShaderResource);
	}

	// Transparency pass: combine the results for each pixels
	{
		mCommandBuffer->TransitionImageLayout(
			mTransparencyPass,
			vkr::ResourceState::ShaderResource,
			vkr::ResourceState::RenderTarget,
			vkr::ResourceState::ShaderResource,
			vkr::ResourceState::DepthStencilWrite);
		mCommandBuffer->BeginRenderPass(mTransparencyPass, vkr::DRAW_PASS_CLEAR_FLAG_CLEAR_RENDER_TARGETS);

		mCommandBuffer->SetScissors(mTransparencyPass->GetScissor());
		mCommandBuffer->SetViewports(mTransparencyPass->GetViewport());

		mCommandBuffer->BindGraphicsDescriptorSets(mDepthPeeling.combinePipelineInterface, 1, &mDepthPeeling.combineDescriptorSet);
		mCommandBuffer->BindGraphicsPipeline(mDepthPeeling.combinePipeline);
		mCommandBuffer->Draw(3);

		mCommandBuffer->EndRenderPass();
		mCommandBuffer->TransitionImageLayout(
			mTransparencyPass,
			vkr::ResourceState::RenderTarget,
			vkr::ResourceState::ShaderResource,
			vkr::ResourceState::DepthStencilWrite,
			vkr::ResourceState::ShaderResource);
	}
}

//=============================================================================
// UnsortedOver
//=============================================================================

void Example_024::SetupUnsortedOver()
{
	// Descriptor
	{
		vkr::DescriptorSetLayoutCreateInfo layoutCreateInfo = {};
		layoutCreateInfo.bindings.push_back(vkr::DescriptorBinding{ SHADER_GLOBALS_REGISTER, vkr::DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, vkr::SHADER_STAGE_ALL_GRAPHICS });
		CHECKED_CALL(GetRenderDevice().CreateDescriptorSetLayout(layoutCreateInfo, &mUnsortedOver.descriptorSetLayout));

		CHECKED_CALL(GetRenderDevice().AllocateDescriptorSet(mDescriptorPool, mUnsortedOver.descriptorSetLayout, &mUnsortedOver.descriptorSet));

		vkr::WriteDescriptor write = {};
		write.binding = SHADER_GLOBALS_REGISTER;
		write.type = vkr::DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		write.bufferOffset = 0;
		write.bufferRange = WHOLE_SIZE;
		write.pBuffer = mShaderGlobalsBuffer;
		CHECKED_CALL(mUnsortedOver.descriptorSet->UpdateDescriptors(1, &write));
	}

	// Pipeline
	{
		vkr::PipelineInterfaceCreateInfo piCreateInfo = {};
		piCreateInfo.setCount = 1;
		piCreateInfo.sets[0].set = 0;
		piCreateInfo.sets[0].pLayout = mUnsortedOver.descriptorSetLayout;
		CHECKED_CALL(GetRenderDevice().CreatePipelineInterface(piCreateInfo, &mUnsortedOver.pipelineInterface));

		vkr::ShaderModulePtr VS, PS;
		CHECKED_CALL(GetRenderDevice().CreateShader("basic/shaders/oit_demo", "UnsortedOver.vs", &VS));
		CHECKED_CALL(GetRenderDevice().CreateShader("basic/shaders/oit_demo", "UnsortedOver.ps", &PS));

		vkr::GraphicsPipelineCreateInfo gpCreateInfo = {};
		gpCreateInfo.VS = { VS, "vsmain" };
		gpCreateInfo.PS = { PS, "psmain" };
		gpCreateInfo.vertexInputState.bindingCount = 1;
		gpCreateInfo.vertexInputState.bindings[0] = GetTransparentMesh()->GetDerivedVertexBindings()[0];
		gpCreateInfo.inputAssemblyState.topology = vkr::PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		gpCreateInfo.rasterState.polygonMode = vkr::POLYGON_MODE_FILL;
		gpCreateInfo.rasterState.frontFace = vkr::FRONT_FACE_CCW;
		gpCreateInfo.rasterState.rasterizationSamples = vkr::SAMPLE_COUNT_1;
		gpCreateInfo.depthStencilState.depthTestEnable = true;
		gpCreateInfo.depthStencilState.depthWriteEnable = false;
		gpCreateInfo.colorBlendState.blendAttachmentCount = 1;
		gpCreateInfo.colorBlendState.blendAttachments[0].blendEnable = true;
		gpCreateInfo.colorBlendState.blendAttachments[0].srcColorBlendFactor = vkr::BLEND_FACTOR_ONE;
		gpCreateInfo.colorBlendState.blendAttachments[0].dstColorBlendFactor = vkr::BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
		gpCreateInfo.colorBlendState.blendAttachments[0].colorBlendOp = vkr::BLEND_OP_ADD;
		gpCreateInfo.colorBlendState.blendAttachments[0].srcAlphaBlendFactor = vkr::BLEND_FACTOR_ONE;
		gpCreateInfo.colorBlendState.blendAttachments[0].dstAlphaBlendFactor = vkr::BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
		gpCreateInfo.colorBlendState.blendAttachments[0].alphaBlendOp = vkr::BLEND_OP_ADD;
		gpCreateInfo.colorBlendState.blendAttachments[0].colorWriteMask = vkr::ColorComponentFlags::RGBA();
		gpCreateInfo.outputState.renderTargetCount = 1;
		gpCreateInfo.outputState.renderTargetFormats[0] = mTransparencyPass->GetRenderTargetTexture(0)->GetImageFormat();
		gpCreateInfo.outputState.depthStencilFormat = mTransparencyPass->GetDepthStencilTexture()->GetImageFormat();
		gpCreateInfo.pPipelineInterface = mUnsortedOver.pipelineInterface;

		gpCreateInfo.rasterState.cullMode = vkr::CULL_MODE_NONE;
		CHECKED_CALL(GetRenderDevice().CreateGraphicsPipeline(gpCreateInfo, &mUnsortedOver.meshAllFacesPipeline));

		gpCreateInfo.rasterState.cullMode = vkr::CULL_MODE_FRONT;
		CHECKED_CALL(GetRenderDevice().CreateGraphicsPipeline(gpCreateInfo, &mUnsortedOver.meshBackFacesPipeline));

		gpCreateInfo.rasterState.cullMode = vkr::CULL_MODE_BACK;
		CHECKED_CALL(GetRenderDevice().CreateGraphicsPipeline(gpCreateInfo, &mUnsortedOver.meshFrontFacesPipeline));

		GetRenderDevice().DestroyShaderModule(VS);
		GetRenderDevice().DestroyShaderModule(PS);
	}
}

void Example_024::RecordUnsortedOver()
{
	mCommandBuffer->TransitionImageLayout(
		mTransparencyPass,
		vkr::ResourceState::ShaderResource,
		vkr::ResourceState::RenderTarget,
		vkr::ResourceState::ShaderResource,
		vkr::ResourceState::DepthStencilWrite);
	mCommandBuffer->BeginRenderPass(mTransparencyPass, vkr::DRAW_PASS_CLEAR_FLAG_CLEAR_RENDER_TARGETS);

	mCommandBuffer->SetScissors(mTransparencyPass->GetScissor());
	mCommandBuffer->SetViewports(mTransparencyPass->GetViewport());

	mCommandBuffer->BindGraphicsDescriptorSets(mUnsortedOver.pipelineInterface, 1, &mUnsortedOver.descriptorSet);
	mCommandBuffer->BindIndexBuffer(GetTransparentMesh());
	mCommandBuffer->BindVertexBuffers(GetTransparentMesh());
	switch (mGuiParameters.unsortedOver.faceMode) {
	case FACE_MODE_ALL: {
		mCommandBuffer->BindGraphicsPipeline(mUnsortedOver.meshAllFacesPipeline);
		mCommandBuffer->DrawIndexed(GetTransparentMesh()->GetIndexCount());
		break;
	}
	case FACE_MODE_ALL_BACK_THEN_FRONT: {
		mCommandBuffer->BindGraphicsPipeline(mUnsortedOver.meshBackFacesPipeline);
		mCommandBuffer->DrawIndexed(GetTransparentMesh()->GetIndexCount());
		mCommandBuffer->BindGraphicsPipeline(mUnsortedOver.meshFrontFacesPipeline);
		mCommandBuffer->DrawIndexed(GetTransparentMesh()->GetIndexCount());
		break;
	}
	case FACE_MODE_BACK_ONLY: {
		mCommandBuffer->BindGraphicsPipeline(mUnsortedOver.meshBackFacesPipeline);
		mCommandBuffer->DrawIndexed(GetTransparentMesh()->GetIndexCount());
		break;
	}
	case FACE_MODE_FRONT_ONLY: {
		mCommandBuffer->BindGraphicsPipeline(mUnsortedOver.meshFrontFacesPipeline);
		mCommandBuffer->DrawIndexed(GetTransparentMesh()->GetIndexCount());
		break;
	}
	default: {
		ASSERT_MSG(false, "unknown face mode");
		break;
	}
	}

	mCommandBuffer->EndRenderPass();
	mCommandBuffer->TransitionImageLayout(
		mTransparencyPass,
		vkr::ResourceState::RenderTarget,
		vkr::ResourceState::ShaderResource,
		vkr::ResourceState::DepthStencilWrite,
		vkr::ResourceState::ShaderResource);
}

//=============================================================================
// WeightedAverage
//=============================================================================

void Example_024::SetupWeightedAverage()
{
	////////////////////////////////////////
	// Common
	////////////////////////////////////////

	// Texture
	{
		vkr::TextureCreateInfo createInfo = {};
		createInfo.imageType = vkr::IMAGE_TYPE_2D;
		createInfo.width = mTransparencyTexture->GetWidth();
		createInfo.height = mTransparencyTexture->GetHeight();
		createInfo.depth = 1;
		createInfo.sampleCount = vkr::SAMPLE_COUNT_1;
		createInfo.mipLevelCount = 1;
		createInfo.arrayLayerCount = 1;
		createInfo.usageFlags.bits.colorAttachment = true;
		createInfo.usageFlags.bits.sampled = true;
		createInfo.memoryUsage = vkr::MemoryUsage::GPUOnly;
		createInfo.initialState = vkr::ResourceState::ShaderResource;

		createInfo.imageFormat = vkr::FORMAT_R16G16B16A16_FLOAT;
		CHECKED_CALL(GetRenderDevice().CreateTexture(createInfo, &mWeightedAverage.colorTexture));

		createInfo.imageFormat = vkr::FORMAT_R16_FLOAT;
		CHECKED_CALL(GetRenderDevice().CreateTexture(createInfo, &mWeightedAverage.extraTexture));
	}

	////////////////////////////////////////
	// Gather
	////////////////////////////////////////

	// Pass
	{
		vkr::DrawPassCreateInfo2 createInfo = {};
		createInfo.width = mWeightedAverage.colorTexture->GetWidth();
		createInfo.height = mWeightedAverage.colorTexture->GetHeight();
		createInfo.renderTargetCount = 2;
		createInfo.pRenderTargetImages[0] = mWeightedAverage.colorTexture->GetImage();
		createInfo.pRenderTargetImages[1] = mWeightedAverage.extraTexture->GetImage();
		createInfo.pDepthStencilImage = mOpaquePass->GetDepthStencilTexture()->GetImage();
		createInfo.depthStencilState = vkr::ResourceState::DepthStencilWrite;
		createInfo.renderTargetClearValues[0] = { 0, 0, 0, 0 };
		createInfo.depthStencilClearValue = { 1.0f, 0xFF };

		// Count type
		createInfo.renderTargetClearValues[1] = { 0, 0, 0, 0 };
		CHECKED_CALL(GetRenderDevice().CreateDrawPass(createInfo, &mWeightedAverage.count.gatherPass));

		// Coverage type
		createInfo.renderTargetClearValues[1] = { 1, 1, 1, 1 };
		CHECKED_CALL(GetRenderDevice().CreateDrawPass(createInfo, &mWeightedAverage.coverage.gatherPass));
	}

	// Descriptor
	{
		vkr::DescriptorSetLayoutCreateInfo layoutCreateInfo = {};
		layoutCreateInfo.bindings.push_back(vkr::DescriptorBinding{ SHADER_GLOBALS_REGISTER, vkr::DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, vkr::SHADER_STAGE_ALL_GRAPHICS });
		CHECKED_CALL(GetRenderDevice().CreateDescriptorSetLayout(layoutCreateInfo, &mWeightedAverage.gatherDescriptorSetLayout));

		CHECKED_CALL(GetRenderDevice().AllocateDescriptorSet(mDescriptorPool, mWeightedAverage.gatherDescriptorSetLayout, &mWeightedAverage.gatherDescriptorSet));

		vkr::WriteDescriptor write = {};
		write.binding = SHADER_GLOBALS_REGISTER;
		write.type = vkr::DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		write.bufferOffset = 0;
		write.bufferRange = WHOLE_SIZE;
		write.pBuffer = mShaderGlobalsBuffer;
		CHECKED_CALL(mWeightedAverage.gatherDescriptorSet->UpdateDescriptors(1, &write));
	}

	// Pipeline
	{
		vkr::PipelineInterfaceCreateInfo piCreateInfo = {};
		piCreateInfo.setCount = 1;
		piCreateInfo.sets[0].set = 0;
		piCreateInfo.sets[0].pLayout = mWeightedAverage.gatherDescriptorSetLayout;
		CHECKED_CALL(GetRenderDevice().CreatePipelineInterface(piCreateInfo, &mWeightedAverage.gatherPipelineInterface));

		vkr::ShaderModulePtr            VS, PS;
		vkr::GraphicsPipelineCreateInfo gpCreateInfo = {};
		gpCreateInfo.vertexInputState.bindingCount = 1;
		gpCreateInfo.vertexInputState.bindings[0] = GetTransparentMesh()->GetDerivedVertexBindings()[0];
		gpCreateInfo.inputAssemblyState.topology = vkr::PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		gpCreateInfo.rasterState.polygonMode = vkr::POLYGON_MODE_FILL;
		gpCreateInfo.rasterState.cullMode = vkr::CULL_MODE_NONE;
		gpCreateInfo.rasterState.frontFace = vkr::FRONT_FACE_CCW;
		gpCreateInfo.rasterState.rasterizationSamples = vkr::SAMPLE_COUNT_1;
		gpCreateInfo.depthStencilState.depthTestEnable = true;
		gpCreateInfo.depthStencilState.depthWriteEnable = false;

		gpCreateInfo.colorBlendState.blendAttachmentCount = 2;

		gpCreateInfo.colorBlendState.blendAttachments[0].blendEnable = true;
		gpCreateInfo.colorBlendState.blendAttachments[0].srcColorBlendFactor = vkr::BLEND_FACTOR_ONE;
		gpCreateInfo.colorBlendState.blendAttachments[0].dstColorBlendFactor = vkr::BLEND_FACTOR_ONE;
		gpCreateInfo.colorBlendState.blendAttachments[0].colorBlendOp = vkr::BLEND_OP_ADD;
		gpCreateInfo.colorBlendState.blendAttachments[0].srcAlphaBlendFactor = vkr::BLEND_FACTOR_ONE;
		gpCreateInfo.colorBlendState.blendAttachments[0].dstAlphaBlendFactor = vkr::BLEND_FACTOR_ONE;
		gpCreateInfo.colorBlendState.blendAttachments[0].alphaBlendOp = vkr::BLEND_OP_ADD;
		gpCreateInfo.colorBlendState.blendAttachments[0].colorWriteMask = vkr::ColorComponentFlags::RGBA();

		gpCreateInfo.colorBlendState.blendAttachments[1].blendEnable = true;
		gpCreateInfo.colorBlendState.blendAttachments[1].colorBlendOp = vkr::BLEND_OP_ADD;
		gpCreateInfo.colorBlendState.blendAttachments[1].srcAlphaBlendFactor = vkr::BLEND_FACTOR_ZERO;
		gpCreateInfo.colorBlendState.blendAttachments[1].dstAlphaBlendFactor = vkr::BLEND_FACTOR_ZERO;
		gpCreateInfo.colorBlendState.blendAttachments[1].alphaBlendOp = vkr::BLEND_OP_ADD;
		gpCreateInfo.colorBlendState.blendAttachments[1].colorWriteMask = vkr::ColorComponentFlags::RGBA();

		gpCreateInfo.outputState.renderTargetCount = 2;
		gpCreateInfo.outputState.renderTargetFormats[0] = mWeightedAverage.colorTexture->GetImageFormat();
		gpCreateInfo.outputState.renderTargetFormats[1] = mWeightedAverage.extraTexture->GetImageFormat();
		gpCreateInfo.outputState.depthStencilFormat = mOpaquePass->GetDepthStencilTexture()->GetImageFormat();
		gpCreateInfo.pPipelineInterface = mWeightedAverage.gatherPipelineInterface;

		// Count type
		CHECKED_CALL(GetRenderDevice().CreateShader("basic/shaders/oit_demo", "WeightedAverageFragmentCountGather.vs", &VS));
		CHECKED_CALL(GetRenderDevice().CreateShader("basic/shaders/oit_demo", "WeightedAverageFragmentCountGather.ps", &PS));
		gpCreateInfo.VS = { VS, "vsmain" };
		gpCreateInfo.PS = { PS, "psmain" };
		gpCreateInfo.colorBlendState.blendAttachments[1].srcColorBlendFactor = vkr::BLEND_FACTOR_ONE;
		gpCreateInfo.colorBlendState.blendAttachments[1].dstColorBlendFactor = vkr::BLEND_FACTOR_ONE;
		CHECKED_CALL(GetRenderDevice().CreateGraphicsPipeline(gpCreateInfo, &mWeightedAverage.count.gatherPipeline));
		GetRenderDevice().DestroyShaderModule(VS);
		GetRenderDevice().DestroyShaderModule(PS);

		// Coverage type
		CHECKED_CALL(GetRenderDevice().CreateShader("basic/shaders/oit_demo", "WeightedAverageExactCoverageGather.vs", &VS));
		CHECKED_CALL(GetRenderDevice().CreateShader("basic/shaders/oit_demo", "WeightedAverageExactCoverageGather.ps", &PS));
		gpCreateInfo.VS = { VS, "vsmain" };
		gpCreateInfo.PS = { PS, "psmain" };
		gpCreateInfo.colorBlendState.blendAttachments[1].srcColorBlendFactor = vkr::BLEND_FACTOR_ZERO;
		gpCreateInfo.colorBlendState.blendAttachments[1].dstColorBlendFactor = vkr::BLEND_FACTOR_SRC_COLOR;
		CHECKED_CALL(GetRenderDevice().CreateGraphicsPipeline(gpCreateInfo, &mWeightedAverage.coverage.gatherPipeline));
		GetRenderDevice().DestroyShaderModule(VS);
		GetRenderDevice().DestroyShaderModule(PS);
	}

	////////////////////////////////////////
	// Combine
	////////////////////////////////////////

	// Descriptor
	{
		vkr::DescriptorSetLayoutCreateInfo layoutCreateInfo = {};
		layoutCreateInfo.bindings.push_back(vkr::DescriptorBinding{ CUSTOM_SAMPLER_0_REGISTER, vkr::DESCRIPTOR_TYPE_SAMPLER, 1, vkr::SHADER_STAGE_ALL_GRAPHICS });
		layoutCreateInfo.bindings.push_back(vkr::DescriptorBinding{ CUSTOM_TEXTURE_0_REGISTER, vkr::DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1, vkr::SHADER_STAGE_ALL_GRAPHICS });
		layoutCreateInfo.bindings.push_back(vkr::DescriptorBinding{ CUSTOM_TEXTURE_1_REGISTER, vkr::DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1, vkr::SHADER_STAGE_ALL_GRAPHICS });
		CHECKED_CALL(GetRenderDevice().CreateDescriptorSetLayout(layoutCreateInfo, &mWeightedAverage.combineDescriptorSetLayout));

		CHECKED_CALL(GetRenderDevice().AllocateDescriptorSet(mDescriptorPool, mWeightedAverage.combineDescriptorSetLayout, &mWeightedAverage.combineDescriptorSet));

		std::array<vkr::WriteDescriptor, 3> writes = {};

		writes[0].binding = CUSTOM_SAMPLER_0_REGISTER;
		writes[0].type = vkr::DESCRIPTOR_TYPE_SAMPLER;
		writes[0].pSampler = mNearestSampler;

		writes[1].binding = CUSTOM_TEXTURE_0_REGISTER;
		writes[1].arrayIndex = 0;
		writes[1].type = vkr::DESCRIPTOR_TYPE_SAMPLED_IMAGE;
		writes[1].pImageView = mWeightedAverage.colorTexture->GetSampledImageView();

		writes[2].binding = CUSTOM_TEXTURE_1_REGISTER;
		writes[2].arrayIndex = 0;
		writes[2].type = vkr::DESCRIPTOR_TYPE_SAMPLED_IMAGE;
		writes[2].pImageView = mWeightedAverage.extraTexture->GetSampledImageView();

		CHECKED_CALL(mWeightedAverage.combineDescriptorSet->UpdateDescriptors(static_cast<uint32_t>(writes.size()), writes.data()));
	}

	// Pipeline
	{
		vkr::PipelineInterfaceCreateInfo piCreateInfo = {};
		piCreateInfo.setCount = 1;
		piCreateInfo.sets[0].set = 0;
		piCreateInfo.sets[0].pLayout = mWeightedAverage.combineDescriptorSetLayout;
		CHECKED_CALL(GetRenderDevice().CreatePipelineInterface(piCreateInfo, &mWeightedAverage.combinePipelineInterface));

		vkr::ShaderModulePtr             VS, PS;
		vkr::GraphicsPipelineCreateInfo2 gpCreateInfo = {};
		gpCreateInfo.vertexInputState.bindingCount = 0;
		gpCreateInfo.topology = vkr::PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		gpCreateInfo.polygonMode = vkr::POLYGON_MODE_FILL;
		gpCreateInfo.cullMode = vkr::CULL_MODE_BACK;
		gpCreateInfo.frontFace = vkr::FRONT_FACE_CCW;
		gpCreateInfo.depthReadEnable = false;
		gpCreateInfo.depthWriteEnable = false;
		gpCreateInfo.blendModes[0] = vkr::BLEND_MODE_NONE;
		gpCreateInfo.outputState.renderTargetCount = 1;
		gpCreateInfo.outputState.renderTargetFormats[0] = mTransparencyTexture->GetImageFormat();
		gpCreateInfo.outputState.depthStencilFormat = mOpaquePass->GetDepthStencilTexture()->GetImageFormat();
		gpCreateInfo.pPipelineInterface = mWeightedAverage.combinePipelineInterface;

		// Count type
		CHECKED_CALL(GetRenderDevice().CreateShader("basic/shaders/oit_demo", "WeightedAverageFragmentCountCombine.vs", &VS));
		CHECKED_CALL(GetRenderDevice().CreateShader("basic/shaders/oit_demo", "WeightedAverageFragmentCountCombine.ps", &PS));
		gpCreateInfo.VS = { VS, "vsmain" };
		gpCreateInfo.PS = { PS, "psmain" };
		CHECKED_CALL(GetRenderDevice().CreateGraphicsPipeline(gpCreateInfo, &mWeightedAverage.count.combinePipeline));
		GetRenderDevice().DestroyShaderModule(VS);
		GetRenderDevice().DestroyShaderModule(PS);

		// Coverage type
		CHECKED_CALL(GetRenderDevice().CreateShader("basic/shaders/oit_demo", "WeightedAverageExactCoverageCombine.vs", &VS));
		CHECKED_CALL(GetRenderDevice().CreateShader("basic/shaders/oit_demo", "WeightedAverageExactCoverageCombine.ps", &PS));
		gpCreateInfo.VS = { VS, "vsmain" };
		gpCreateInfo.PS = { PS, "psmain" };
		CHECKED_CALL(GetRenderDevice().CreateGraphicsPipeline(gpCreateInfo, &mWeightedAverage.coverage.combinePipeline));
		GetRenderDevice().DestroyShaderModule(VS);
		GetRenderDevice().DestroyShaderModule(PS);
	}
}

void Example_024::RecordWeightedAverage()
{
	vkr::DrawPassPtr         gatherPass;
	vkr::GraphicsPipelinePtr gatherPipeline;
	vkr::GraphicsPipelinePtr combinePipeline;
	switch (mGuiParameters.weightedAverage.type) {
	case WEIGHTED_AVERAGE_TYPE_FRAGMENT_COUNT: {
		gatherPass = mWeightedAverage.count.gatherPass;
		gatherPipeline = mWeightedAverage.count.gatherPipeline;
		combinePipeline = mWeightedAverage.count.combinePipeline;
		break;
	}
	case WEIGHTED_AVERAGE_TYPE_EXACT_COVERAGE: {
		gatherPass = mWeightedAverage.coverage.gatherPass;
		gatherPipeline = mWeightedAverage.coverage.gatherPipeline;
		combinePipeline = mWeightedAverage.coverage.combinePipeline;
		break;
	}
	default: {
		ASSERT_MSG(false, "unknown weighted average type");
		break;
	}
	}

	// Gather pass: compute the formula factors for each pixels
	{
		mCommandBuffer->TransitionImageLayout(
			gatherPass,
			vkr::ResourceState::ShaderResource,
			vkr::ResourceState::RenderTarget,
			vkr::ResourceState::ShaderResource,
			vkr::ResourceState::DepthStencilWrite);
		mCommandBuffer->BeginRenderPass(gatherPass, vkr::DRAW_PASS_CLEAR_FLAG_CLEAR_RENDER_TARGETS);

		mCommandBuffer->SetScissors(gatherPass->GetScissor());
		mCommandBuffer->SetViewports(gatherPass->GetViewport());

		mCommandBuffer->BindGraphicsDescriptorSets(mWeightedAverage.gatherPipelineInterface, 1, &mWeightedAverage.gatherDescriptorSet);
		mCommandBuffer->BindGraphicsPipeline(gatherPipeline);
		mCommandBuffer->BindIndexBuffer(GetTransparentMesh());
		mCommandBuffer->BindVertexBuffers(GetTransparentMesh());
		mCommandBuffer->DrawIndexed(GetTransparentMesh()->GetIndexCount());

		mCommandBuffer->EndRenderPass();
		mCommandBuffer->TransitionImageLayout(
			gatherPass,
			vkr::ResourceState::RenderTarget,
			vkr::ResourceState::ShaderResource,
			vkr::ResourceState::DepthStencilWrite,
			vkr::ResourceState::ShaderResource);
	}

	// Transparency pass: combine the results for each pixels
	{
		mCommandBuffer->TransitionImageLayout(
			mTransparencyPass,
			vkr::ResourceState::ShaderResource,
			vkr::ResourceState::RenderTarget,
			vkr::ResourceState::ShaderResource,
			vkr::ResourceState::DepthStencilWrite);
		mCommandBuffer->BeginRenderPass(mTransparencyPass, vkr::DRAW_PASS_CLEAR_FLAG_CLEAR_RENDER_TARGETS);

		mCommandBuffer->SetScissors(mTransparencyPass->GetScissor());
		mCommandBuffer->SetViewports(mTransparencyPass->GetViewport());

		mCommandBuffer->BindGraphicsDescriptorSets(mWeightedAverage.combinePipelineInterface, 1, &mWeightedAverage.combineDescriptorSet);
		mCommandBuffer->BindGraphicsPipeline(combinePipeline);
		mCommandBuffer->Draw(3);

		mCommandBuffer->EndRenderPass();
		mCommandBuffer->TransitionImageLayout(
			mTransparencyPass,
			vkr::ResourceState::RenderTarget,
			vkr::ResourceState::ShaderResource,
			vkr::ResourceState::DepthStencilWrite,
			vkr::ResourceState::ShaderResource);
	}
}

//=============================================================================
// WeightedSum
//=============================================================================

void Example_024::SetupWeightedSum()
{
	// Descriptor
	{
		vkr::DescriptorSetLayoutCreateInfo layoutCreateInfo = {};
		layoutCreateInfo.bindings.push_back(vkr::DescriptorBinding{ SHADER_GLOBALS_REGISTER, vkr::DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, vkr::SHADER_STAGE_ALL_GRAPHICS });
		CHECKED_CALL(GetRenderDevice().CreateDescriptorSetLayout(layoutCreateInfo, &mWeightedSum.descriptorSetLayout));

		CHECKED_CALL(GetRenderDevice().AllocateDescriptorSet(mDescriptorPool, mWeightedSum.descriptorSetLayout, &mWeightedSum.descriptorSet));

		vkr::WriteDescriptor write = {};
		write.binding = SHADER_GLOBALS_REGISTER;
		write.type = vkr::DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		write.bufferOffset = 0;
		write.bufferRange = WHOLE_SIZE;
		write.pBuffer = mShaderGlobalsBuffer;
		CHECKED_CALL(mWeightedSum.descriptorSet->UpdateDescriptors(1, &write));
	}

	// Pipeline
	{
		vkr::PipelineInterfaceCreateInfo piCreateInfo = {};
		piCreateInfo.setCount = 1;
		piCreateInfo.sets[0].set = 0;
		piCreateInfo.sets[0].pLayout = mWeightedSum.descriptorSetLayout;
		CHECKED_CALL(GetRenderDevice().CreatePipelineInterface(piCreateInfo, &mWeightedSum.pipelineInterface));

		vkr::ShaderModulePtr VS, PS;
		CHECKED_CALL(GetRenderDevice().CreateShader("basic/shaders/oit_demo", "WeightedSum.vs", &VS));
		CHECKED_CALL(GetRenderDevice().CreateShader("basic/shaders/oit_demo", "WeightedSum.ps", &PS));

		vkr::GraphicsPipelineCreateInfo gpCreateInfo = {};
		gpCreateInfo.VS = { VS, "vsmain" };
		gpCreateInfo.PS = { PS, "psmain" };
		gpCreateInfo.vertexInputState.bindingCount = 1;
		gpCreateInfo.vertexInputState.bindings[0] = GetTransparentMesh()->GetDerivedVertexBindings()[0];
		gpCreateInfo.inputAssemblyState.topology = vkr::PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		gpCreateInfo.rasterState.polygonMode = vkr::POLYGON_MODE_FILL;
		gpCreateInfo.rasterState.cullMode = vkr::CULL_MODE_NONE;
		gpCreateInfo.rasterState.frontFace = vkr::FRONT_FACE_CCW;
		gpCreateInfo.rasterState.rasterizationSamples = vkr::SAMPLE_COUNT_1;
		gpCreateInfo.depthStencilState.depthTestEnable = true;
		gpCreateInfo.depthStencilState.depthWriteEnable = false;
		gpCreateInfo.colorBlendState.blendAttachmentCount = 1;
		gpCreateInfo.colorBlendState.blendAttachments[0].blendEnable = true;
		gpCreateInfo.colorBlendState.blendAttachments[0].srcColorBlendFactor = vkr::BLEND_FACTOR_ONE;
		gpCreateInfo.colorBlendState.blendAttachments[0].dstColorBlendFactor = vkr::BLEND_FACTOR_ONE;
		gpCreateInfo.colorBlendState.blendAttachments[0].colorBlendOp = vkr::BLEND_OP_ADD;
		gpCreateInfo.colorBlendState.blendAttachments[0].srcAlphaBlendFactor = vkr::BLEND_FACTOR_ONE;
		gpCreateInfo.colorBlendState.blendAttachments[0].dstAlphaBlendFactor = vkr::BLEND_FACTOR_ONE;
		gpCreateInfo.colorBlendState.blendAttachments[0].alphaBlendOp = vkr::BLEND_OP_ADD;
		gpCreateInfo.colorBlendState.blendAttachments[0].colorWriteMask = vkr::ColorComponentFlags::RGBA();
		gpCreateInfo.outputState.renderTargetCount = 1;
		gpCreateInfo.outputState.renderTargetFormats[0] = mTransparencyPass->GetRenderTargetTexture(0)->GetImageFormat();
		gpCreateInfo.outputState.depthStencilFormat = mTransparencyPass->GetDepthStencilTexture()->GetImageFormat();
		gpCreateInfo.pPipelineInterface = mWeightedSum.pipelineInterface;
		CHECKED_CALL(GetRenderDevice().CreateGraphicsPipeline(gpCreateInfo, &mWeightedSum.pipeline));

		GetRenderDevice().DestroyShaderModule(VS);
		GetRenderDevice().DestroyShaderModule(PS);
	}
}

void Example_024::RecordWeightedSum()
{
	mCommandBuffer->TransitionImageLayout(
		mTransparencyPass,
		vkr::ResourceState::ShaderResource,
		vkr::ResourceState::RenderTarget,
		vkr::ResourceState::ShaderResource,
		vkr::ResourceState::DepthStencilWrite);
	mCommandBuffer->BeginRenderPass(mTransparencyPass, vkr::DRAW_PASS_CLEAR_FLAG_CLEAR_RENDER_TARGETS);

	mCommandBuffer->SetScissors(mTransparencyPass->GetScissor());
	mCommandBuffer->SetViewports(mTransparencyPass->GetViewport());

	mCommandBuffer->BindGraphicsDescriptorSets(mWeightedSum.pipelineInterface, 1, &mWeightedSum.descriptorSet);
	mCommandBuffer->BindGraphicsPipeline(mWeightedSum.pipeline);
	mCommandBuffer->BindIndexBuffer(GetTransparentMesh());
	mCommandBuffer->BindVertexBuffers(GetTransparentMesh());
	mCommandBuffer->DrawIndexed(GetTransparentMesh()->GetIndexCount());

	mCommandBuffer->EndRenderPass();
	mCommandBuffer->TransitionImageLayout(
		mTransparencyPass,
		vkr::ResourceState::RenderTarget,
		vkr::ResourceState::ShaderResource,
		vkr::ResourceState::DepthStencilWrite,
		vkr::ResourceState::ShaderResource);
}