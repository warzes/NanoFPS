#include "stdafx.h"
#include "020_GBuffer.h"

namespace e020
{
	vkr::DescriptorSetLayoutPtr Entity::sModelDataLayout;
	vkr::VertexDescription      Entity::sVertexDescription;
	vkr::PipelineInterfacePtr   Entity::sPipelineInterface;
	vkr::GraphicsPipelinePtr    Entity::sPipeline;

	Result Entity::Create(vkr::RenderDevice& device, vkr::Queue* pQueue, vkr::DescriptorPool* pPool, const EntityCreateInfo* pCreateInfo)
	{
		ASSERT_NULL_ARG(pQueue);
		ASSERT_NULL_ARG(pPool);
		ASSERT_NULL_ARG(pCreateInfo);

		mMesh = pCreateInfo->pMesh;
		mMaterial = pCreateInfo->pMaterial;

		// Model constants
		{
			vkr::BufferCreateInfo bufferCreateInfo = {};
			bufferCreateInfo.size = vkr::MINIMUM_CONSTANT_BUFFER_SIZE;
			bufferCreateInfo.usageFlags.bits.transferSrc = true;
			bufferCreateInfo.memoryUsage = vkr::MemoryUsage::CPUToGPU;
			CHECKED_CALL(device.CreateBuffer(bufferCreateInfo, &mCpuModelConstants));

			bufferCreateInfo.usageFlags.bits.transferDst = true;
			bufferCreateInfo.usageFlags.bits.uniformBuffer = true;
			bufferCreateInfo.memoryUsage = vkr::MemoryUsage::GPUOnly;
			CHECKED_CALL(device.CreateBuffer(bufferCreateInfo, &mGpuModelConstants));
		}

		// Allocate descriptor set
		{
			CHECKED_CALL(device.AllocateDescriptorSet(pPool, sModelDataLayout, &mModelDataSet));
			mModelDataSet->SetName("Model Data");
		}

		// Update descriptors
		{
			vkr::WriteDescriptor write = {};
			write.binding = MODEL_CONSTANTS_REGISTER;
			write.type = vkr::DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			write.bufferOffset = 0;
			write.bufferRange = WHOLE_SIZE;
			write.pBuffer = mGpuModelConstants;
			CHECKED_CALL(mModelDataSet->UpdateDescriptors(1, &write));
		}

		return SUCCESS;
	}

	void Entity::Destroy()
	{
	}

	Result Entity::CreatePipelines(vkr::RenderDevice& device, vkr::DescriptorSetLayout* pSceneDataLayout, vkr::DrawPass* pDrawPass)
	{
		ASSERT_NULL_ARG(pSceneDataLayout);

		// Model data
		{
			vkr::DescriptorSetLayoutCreateInfo createInfo = {};
			createInfo.bindings.push_back({ vkr::DescriptorBinding{MODEL_CONSTANTS_REGISTER, vkr::DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, vkr::SHADER_STAGE_ALL_GRAPHICS} });
			CHECKED_CALL(device.CreateDescriptorSetLayout(createInfo, &sModelDataLayout));
		}

		// Pipeline interface
		{
			vkr::PipelineInterfaceCreateInfo createInfo = {};
			createInfo.setCount = 4;
			createInfo.sets[0].set = 0;
			createInfo.sets[0].pLayout = pSceneDataLayout;
			createInfo.sets[1].set = 1;
			createInfo.sets[1].pLayout = Material::GetMaterialResourcesLayout();
			createInfo.sets[2].set = 2;
			createInfo.sets[2].pLayout = Material::GetMaterialDataLayout();
			createInfo.sets[3].set = 3;
			createInfo.sets[3].pLayout = sModelDataLayout;

			CHECKED_CALL(device.CreatePipelineInterface(createInfo, &sPipelineInterface));
		}

		// Pipeline
		{
			vkr::ShaderModulePtr VS;
			CHECKED_CALL(device.CreateShader("basic/shaders/gbuffer", "VertexShader.vs", &VS));

			vkr::ShaderModulePtr PS;
			CHECKED_CALL(device.CreateShader("basic/shaders/gbuffer", "DeferredRender.ps", &PS));

			const vkr::VertexInputRate inputRate = vkr::VERTEX_INPUT_RATE_VERTEX;
			vkr::VertexDescription     vertexDescription;
			// clang-format off
			vertexDescription.AppendBinding(vkr::VertexAttribute{ vkr::SEMANTIC_NAME_POSITION , 0, vkr::FORMAT_R32G32B32_FLOAT, 0, APPEND_OFFSET_ALIGNED, inputRate });
			vertexDescription.AppendBinding(vkr::VertexAttribute{ vkr::SEMANTIC_NAME_COLOR    , 1, vkr::FORMAT_R32G32B32_FLOAT, 1, APPEND_OFFSET_ALIGNED, inputRate });
			vertexDescription.AppendBinding(vkr::VertexAttribute{ vkr::SEMANTIC_NAME_NORMAL   , 2, vkr::FORMAT_R32G32B32_FLOAT, 2, APPEND_OFFSET_ALIGNED, inputRate });
			vertexDescription.AppendBinding(vkr::VertexAttribute{ vkr::SEMANTIC_NAME_TEXCOORD , 3, vkr::FORMAT_R32G32_FLOAT,    3, APPEND_OFFSET_ALIGNED, inputRate });
			vertexDescription.AppendBinding(vkr::VertexAttribute{ vkr::SEMANTIC_NAME_TANGENT  , 4, vkr::FORMAT_R32G32B32_FLOAT, 4, APPEND_OFFSET_ALIGNED, inputRate });
			vertexDescription.AppendBinding(vkr::VertexAttribute{ vkr::SEMANTIC_NAME_BITANGENT, 5, vkr::FORMAT_R32G32B32_FLOAT, 5, APPEND_OFFSET_ALIGNED, inputRate });
			// clang-format on

			vkr::GraphicsPipelineCreateInfo2 gpCreateInfo = {};
			gpCreateInfo.VS = { VS.Get(), "vsmain" };
			gpCreateInfo.PS = { PS.Get(), "psmain" };
			gpCreateInfo.topology = vkr::PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
			gpCreateInfo.polygonMode = vkr::POLYGON_MODE_FILL;
			gpCreateInfo.cullMode = vkr::CULL_MODE_BACK;
			gpCreateInfo.frontFace = vkr::FRONT_FACE_CCW;
			gpCreateInfo.depthReadEnable = true;
			gpCreateInfo.depthWriteEnable = true;
			gpCreateInfo.pPipelineInterface = sPipelineInterface;
			gpCreateInfo.outputState.depthStencilFormat = pDrawPass->GetDepthStencilTexture()->GetImage()->GetFormat();
			// Render target
			gpCreateInfo.outputState.renderTargetCount = pDrawPass->GetRenderTargetCount();
			for (uint32_t i = 0; i < gpCreateInfo.outputState.renderTargetCount; ++i) {
				gpCreateInfo.blendModes[i] = vkr::BLEND_MODE_NONE;
				gpCreateInfo.outputState.renderTargetFormats[i] = pDrawPass->GetRenderTargetTexture(i)->GetImage()->GetFormat();
			}
			// Vertex description
			gpCreateInfo.vertexInputState.bindingCount = vertexDescription.GetBindingCount();
			for (uint32_t i = 0; i < vertexDescription.GetBindingCount(); ++i) {
				gpCreateInfo.vertexInputState.bindings[i] = *vertexDescription.GetBinding(i);
			}

			CHECKED_CALL(device.CreateGraphicsPipeline(gpCreateInfo, &sPipeline));
			device.DestroyShaderModule(VS);
			device.DestroyShaderModule(PS);
		}

		return SUCCESS;
	}

	void Entity::DestroyPipelines()
	{
	}

	void Entity::UpdateConstants(vkr::Queue* pQueue)
	{
		const float4x4& M = mTransform.GetConcatenatedMatrix();

		HLSL_PACK_BEGIN();
		struct HlslModelData
		{
			hlsl_float4x4<64> modelMatrix;
			hlsl_float4x4<64> normalMatrix;
			hlsl_float3<12>   debugColor;
		};
		HLSL_PACK_END();

		void* pMappedAddress = nullptr;
		CHECKED_CALL(mCpuModelConstants->MapMemory(0, &pMappedAddress));

		HlslModelData* pModelData = static_cast<HlslModelData*>(pMappedAddress);
		pModelData->modelMatrix = M;
		pModelData->normalMatrix = glm::inverseTranspose(M);

		mCpuModelConstants->UnmapMemory();

		vkr::BufferToBufferCopyInfo copyInfo = { mCpuModelConstants->GetSize() };
		CHECKED_CALL(pQueue->CopyBufferToBuffer(&copyInfo, mCpuModelConstants, mGpuModelConstants, vkr::ResourceState::ConstantBuffer, vkr::ResourceState::ConstantBuffer));
	}

	void Entity::Draw(vkr::DescriptorSet* pSceneDataSet, vkr::CommandBuffer* pCmd)
	{
		vkr::DescriptorSet* sets[4] = { nullptr };
		sets[0] = pSceneDataSet;
		sets[1] = mMaterial->GetMaterialResourceSet();
		sets[2] = mMaterial->GetMaterialDataSet();
		sets[3] = mModelDataSet;
		pCmd->BindGraphicsDescriptorSets(sPipelineInterface, 4, sets);

		pCmd->BindGraphicsPipeline(sPipeline);

		pCmd->BindIndexBuffer(mMesh);
		pCmd->BindVertexBuffers(mMesh);
		pCmd->DrawIndexed(mMesh->GetIndexCount());
	}

	const float  F0_Generic = 0.04f;
	const float3 F0_MetalTitanium = float3(0.542f, 0.497f, 0.449f);
	const float3 F0_MetalChromium = float3(0.549f, 0.556f, 0.554f);
	const float3 F0_MetalIron = float3(0.562f, 0.565f, 0.578f);
	const float3 F0_MetalNickel = float3(0.660f, 0.609f, 0.526f);
	const float3 F0_MetalPlatinum = float3(0.673f, 0.637f, 0.585f);
	const float3 F0_MetalCopper = float3(0.955f, 0.638f, 0.538f);
	const float3 F0_MetalPalladium = float3(0.733f, 0.697f, 0.652f);
	const float3 F0_MetalZinc = float3(0.664f, 0.824f, 0.850f);
	const float3 F0_MetalGold = float3(1.022f, 0.782f, 0.344f);
	const float3 F0_MetalAluminum = float3(0.913f, 0.922f, 0.924f);
	const float3 F0_MetalSilver = float3(0.972f, 0.960f, 0.915f);
	const float3 F0_DiletricWater = float3(0.020f);
	const float3 F0_DiletricPlastic = float3(0.040f);
	const float3 F0_DiletricGlass = float3(0.045f);
	const float3 F0_DiletricCrystal = float3(0.050f);
	const float3 F0_DiletricGem = float3(0.080f);
	const float3 F0_DiletricDiamond = float3(0.150f);

	vkr::TexturePtr             Material::s1x1BlackTexture;
	vkr::TexturePtr             Material::s1x1WhiteTexture;
	vkr::SamplerPtr             Material::sClampedSampler;
	vkr::DescriptorSetLayoutPtr Material::sMaterialResourcesLayout;
	vkr::DescriptorSetLayoutPtr Material::sMaterialDataLayout;

	Material Material::sWood;
	Material Material::sTiles;

	static std::map<std::string, vkr::TexturePtr> mTextureCache;

	static Result LoadTexture(vkr::Queue* pQueue, const std::filesystem::path& path, vkr::Texture** ppTexture)
	{
		if (!std::filesystem::exists(path)) {
			return ERROR_PATH_DOES_NOT_EXIST;
		}

		// Try to load from cache
		auto it = mTextureCache.find(path.string());
		if (it != std::end(mTextureCache)) {
			*ppTexture = it->second;
		}
		else {
			vkr::vkrUtil::TextureOptions textureOptions = vkr::vkrUtil::TextureOptions().MipLevelCount(RemainingMipLevels);
			CHECKED_CALL(vkr::vkrUtil::CreateTextureFromFile(pQueue, path, ppTexture, textureOptions));
		}

		return SUCCESS;
	}

	Result Material::Create(vkr::RenderDevice& device, vkr::Queue* pQueue, vkr::DescriptorPool* pPool, const MaterialCreateInfo* pCreateInfo)
	{
		Result ppxres = ERROR_FAILED;

		// Material constants temp buffer
		vkr::BufferPtr    tmpCpuMaterialConstants;
		MaterialConstants* pMaterialConstants = nullptr;
		{
			vkr::BufferCreateInfo bufferCreateInfo = {};
			bufferCreateInfo.size = vkr::MINIMUM_CONSTANT_BUFFER_SIZE;
			bufferCreateInfo.usageFlags.bits.transferSrc = true;
			bufferCreateInfo.memoryUsage = vkr::MemoryUsage::CPUToGPU;
			CHECKED_CALL(device.CreateBuffer(bufferCreateInfo, &tmpCpuMaterialConstants));

			CHECKED_CALL(tmpCpuMaterialConstants->MapMemory(0, reinterpret_cast<void**>(&pMaterialConstants)));
		}

		// Values
		pMaterialConstants->F0 = pCreateInfo->F0;
		pMaterialConstants->albedo = pCreateInfo->albedo;
		pMaterialConstants->roughness = pCreateInfo->roughness;
		pMaterialConstants->metalness = pCreateInfo->metalness;
		pMaterialConstants->iblStrength = pCreateInfo->iblStrength;
		pMaterialConstants->envStrength = pCreateInfo->envStrength;

		// Albedo texture
		mAlbedoTexture = s1x1WhiteTexture;
		if (!pCreateInfo->albedoTexturePath.empty()) {
			CHECKED_CALL(LoadTexture(pQueue, pCreateInfo->albedoTexturePath, &mAlbedoTexture));
			pMaterialConstants->albedoSelect = 1;
		}

		// Roughness texture
		mRoughnessTexture = s1x1BlackTexture;
		if (!pCreateInfo->roughnessTexturePath.empty()) {
			CHECKED_CALL(LoadTexture(pQueue, pCreateInfo->roughnessTexturePath, &mRoughnessTexture));
			pMaterialConstants->roughnessSelect = 1;
		}

		// Metalness texture
		mMetalnessTexture = s1x1BlackTexture;
		if (!pCreateInfo->metalnessTexturePath.empty()) {
			CHECKED_CALL(LoadTexture(pQueue, pCreateInfo->metalnessTexturePath, &mMetalnessTexture));
			pMaterialConstants->metalnessSelect = 1;
		}

		// Normal map texture
		mNormalMapTexture = s1x1BlackTexture;
		if (!pCreateInfo->normalTexturePath.empty()) {
			CHECKED_CALL(LoadTexture(pQueue, pCreateInfo->normalTexturePath, &mNormalMapTexture));
			pMaterialConstants->normalSelect = 1;
		}

		// Unmap since we're done
		tmpCpuMaterialConstants->UnmapMemory();

		// Copy material constants to GPU buffer
		{
			vkr::BufferCreateInfo bufferCreateInfo = {};
			bufferCreateInfo.size = tmpCpuMaterialConstants->GetSize();
			bufferCreateInfo.usageFlags.bits.transferDst = true;
			bufferCreateInfo.usageFlags.bits.uniformBuffer = true;
			bufferCreateInfo.memoryUsage = vkr::MemoryUsage::GPUOnly;
			CHECKED_CALL(device.CreateBuffer(bufferCreateInfo, &mMaterialConstants));

			vkr::BufferToBufferCopyInfo copyInfo = { tmpCpuMaterialConstants->GetSize() };

			ppxres = pQueue->CopyBufferToBuffer(&copyInfo, tmpCpuMaterialConstants, mMaterialConstants, vkr::ResourceState::ConstantBuffer, vkr::ResourceState::ConstantBuffer);
			if (Failed(ppxres)) {
				return ppxres;
			}
		}

		// Allocate descriptor sets
		CHECKED_CALL(device.AllocateDescriptorSet(pPool, sMaterialResourcesLayout, &mMaterialResourcesSet));
		CHECKED_CALL(device.AllocateDescriptorSet(pPool, sMaterialDataLayout, &mMaterialDataSet));
		mMaterialResourcesSet->SetName("Material Resource");
		mMaterialDataSet->SetName("Material Data");

		// Update material resource descriptors
		{
			vkr::WriteDescriptor writes[5] = {};
			writes[0].binding = MATERIAL_ALBEDO_TEXTURE_REGISTER;
			writes[0].arrayIndex = 0;
			writes[0].type = vkr::DESCRIPTOR_TYPE_SAMPLED_IMAGE;
			writes[0].pImageView = mAlbedoTexture->GetSampledImageView();
			writes[1].binding = MATERIAL_ROUGHNESS_TEXTURE_REGISTER;
			writes[1].arrayIndex = 0;
			writes[1].type = vkr::DESCRIPTOR_TYPE_SAMPLED_IMAGE;
			writes[1].pImageView = mRoughnessTexture->GetSampledImageView();
			writes[2].binding = MATERIAL_METALNESS_TEXTURE_REGISTER;
			writes[2].arrayIndex = 0;
			writes[2].type = vkr::DESCRIPTOR_TYPE_SAMPLED_IMAGE;
			writes[2].pImageView = mMetalnessTexture->GetSampledImageView();
			writes[3].binding = MATERIAL_NORMAL_MAP_TEXTURE_REGISTER;
			writes[3].arrayIndex = 0;
			writes[3].type = vkr::DESCRIPTOR_TYPE_SAMPLED_IMAGE;
			writes[3].pImageView = mNormalMapTexture->GetSampledImageView();
			writes[4].binding = CLAMPED_SAMPLER_REGISTER;
			writes[4].type = vkr::DESCRIPTOR_TYPE_SAMPLER;
			writes[4].pSampler = sClampedSampler;

			CHECKED_CALL(mMaterialResourcesSet->UpdateDescriptors(5, writes));
		}

		// Update material data descriptors
		{
			vkr::WriteDescriptor write = {};
			write.binding = MATERIAL_CONSTANTS_REGISTER;
			write.type = vkr::DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			write.bufferOffset = 0;
			write.bufferRange = WHOLE_SIZE;
			write.pBuffer = mMaterialConstants;
			CHECKED_CALL(mMaterialDataSet->UpdateDescriptors(1, &write));
		}

		return SUCCESS;
	}

	void Material::Destroy()
	{
	}

	Result Material::CreateMaterials(vkr::RenderDevice& device, vkr::Queue* pQueue, vkr::DescriptorPool* pPool)
	{
		// Create 1x1 black and white textures
		{
			CHECKED_CALL(vkr::vkrUtil::CreateTexture1x1<uint8_t>(pQueue, { 0, 0, 0, 0 }, &s1x1BlackTexture));
			CHECKED_CALL(vkr::vkrUtil::CreateTexture1x1<uint8_t>(pQueue, { 255, 255, 255, 255 }, &s1x1WhiteTexture));
		}

		// Create sampler
		{
			vkr::SamplerCreateInfo createInfo = {};
			createInfo.magFilter = vkr::Filter::Linear;
			createInfo.minFilter = vkr::Filter::Linear;
			createInfo.mipmapMode = vkr::SamplerMipmapMode::Linear;
			CHECKED_CALL(device.CreateSampler(createInfo, &sClampedSampler));
		}

		// Material resources layout
		{
			vkr::DescriptorSetLayoutCreateInfo createInfo = {};
			createInfo.bindings.push_back({ vkr::DescriptorBinding{MATERIAL_ALBEDO_TEXTURE_REGISTER,     vkr::DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1, vkr::SHADER_STAGE_ALL_GRAPHICS} });
			createInfo.bindings.push_back({ vkr::DescriptorBinding{MATERIAL_ROUGHNESS_TEXTURE_REGISTER,  vkr::DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1, vkr::SHADER_STAGE_ALL_GRAPHICS} });
			createInfo.bindings.push_back({ vkr::DescriptorBinding{MATERIAL_METALNESS_TEXTURE_REGISTER,  vkr::DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1, vkr::SHADER_STAGE_ALL_GRAPHICS} });
			createInfo.bindings.push_back({ vkr::DescriptorBinding{MATERIAL_NORMAL_MAP_TEXTURE_REGISTER, vkr::DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1, vkr::SHADER_STAGE_ALL_GRAPHICS} });
			createInfo.bindings.push_back({ vkr::DescriptorBinding{CLAMPED_SAMPLER_REGISTER,             vkr::DESCRIPTOR_TYPE_SAMPLER,       1, vkr::SHADER_STAGE_ALL_GRAPHICS} });
			CHECKED_CALL(device.CreateDescriptorSetLayout(createInfo, &sMaterialResourcesLayout));
		}

		// Material data layout
		{
			vkr::DescriptorSetLayoutCreateInfo createInfo = {};
			createInfo.bindings.push_back({ vkr::DescriptorBinding{MATERIAL_CONSTANTS_REGISTER, vkr::DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, vkr::SHADER_STAGE_ALL_GRAPHICS} });
			CHECKED_CALL(device.CreateDescriptorSetLayout(createInfo, &sMaterialDataLayout));
		}

		// Wood
		{
			MaterialCreateInfo createInfo = {};
			createInfo.F0 = F0_Generic;
			createInfo.albedo = F0_DiletricPlastic;
			createInfo.roughness = 1;
			createInfo.metalness = 0;
			createInfo.iblStrength = 0;
			createInfo.envStrength = 0.0f;
			createInfo.albedoTexturePath = "materials/textures/wood/albedo.png";
			createInfo.roughnessTexturePath = "materials/textures/wood/roughness.png";
			createInfo.metalnessTexturePath = "materials/textures/wood/metalness.png";
			createInfo.normalTexturePath = "materials/textures/wood/normal.png";

			CHECKED_CALL(sWood.Create(device, pQueue, pPool, &createInfo));
		}

		// Tiles
		{
			MaterialCreateInfo createInfo = {};
			createInfo.F0 = F0_Generic;
			createInfo.albedo = F0_DiletricCrystal;
			createInfo.roughness = 0.4f;
			createInfo.metalness = 0;
			createInfo.iblStrength = 0;
			createInfo.envStrength = 0.0f;
			createInfo.albedoTexturePath = "materials/textures/tiles/albedo.png";
			createInfo.roughnessTexturePath = "materials/textures/tiles/roughness.png";
			createInfo.metalnessTexturePath = "materials/textures/tiles/metalness.png";
			createInfo.normalTexturePath = "materials/textures/tiles/normal.png";

			CHECKED_CALL(sTiles.Create(device, pQueue, pPool, &createInfo));
		}

		return SUCCESS;
	}

	void Material::DestroyMaterials()
	{
	}
}

bool gUpdateOnce = false;

EngineApplicationCreateInfo Example_020::Config() const
{
	EngineApplicationCreateInfo createInfo{};
	createInfo.render.showImgui = true;
	return createInfo;
}

bool Example_020::Setup()
{
	auto& device = GetRenderDevice();

	// Cameras
	{
		mCamera = PerspCamera(60.0f, GetWindowAspect());
	}

	// Create descriptor pool
	{
		vkr::DescriptorPoolCreateInfo createInfo = {};
		createInfo.sampler = 1000;
		createInfo.sampledImage = 1000;
		createInfo.uniformBuffer = 1000;
		createInfo.structuredBuffer = 1000;

		CHECKED_CALL(device.CreateDescriptorPool(createInfo, &mDescriptorPool));
	}

	// Sampler
	{
		vkr::SamplerCreateInfo createInfo = {};
		createInfo.magFilter = vkr::Filter::Linear;
		createInfo.minFilter = vkr::Filter::Linear;
		createInfo.mipmapMode = vkr::SamplerMipmapMode::Linear;
		createInfo.minLod = 0.0f;
		createInfo.maxLod = FLT_MAX;
		CHECKED_CALL(device.CreateSampler(createInfo, &mSampler));
	}

	// GBuffer passes
	setupGBufferPasses();

	// GBuffer attribute selection buffer
	{
		vkr::BufferCreateInfo bufferCreateInfo = {};
		bufferCreateInfo.size = vkr::MINIMUM_UNIFORM_BUFFER_SIZE;
		bufferCreateInfo.usageFlags.bits.uniformBuffer = true;
		bufferCreateInfo.memoryUsage = vkr::MemoryUsage::CPUToGPU;

		CHECKED_CALL(device.CreateBuffer(bufferCreateInfo, &mGBufferDrawAttrConstants));
	}

	// GBuffer read
	{
		// clang-format off
		vkr::DescriptorSetLayoutCreateInfo createInfo = {};
		createInfo.bindings.push_back({ vkr::DescriptorBinding{GBUFFER_RT0_REGISTER,       vkr::DESCRIPTOR_TYPE_SAMPLED_IMAGE,  1, vkr::SHADER_STAGE_ALL_GRAPHICS} });
		createInfo.bindings.push_back({ vkr::DescriptorBinding{GBUFFER_RT1_REGISTER,       vkr::DESCRIPTOR_TYPE_SAMPLED_IMAGE,  1, vkr::SHADER_STAGE_ALL_GRAPHICS} });
		createInfo.bindings.push_back({ vkr::DescriptorBinding{GBUFFER_RT2_REGISTER,       vkr::DESCRIPTOR_TYPE_SAMPLED_IMAGE,  1, vkr::SHADER_STAGE_ALL_GRAPHICS} });
		createInfo.bindings.push_back({ vkr::DescriptorBinding{GBUFFER_RT3_REGISTER,       vkr::DESCRIPTOR_TYPE_SAMPLED_IMAGE,  1, vkr::SHADER_STAGE_ALL_GRAPHICS} });
		createInfo.bindings.push_back({ vkr::DescriptorBinding{GBUFFER_ENV_REGISTER,       vkr::DESCRIPTOR_TYPE_SAMPLED_IMAGE,  1, vkr::SHADER_STAGE_ALL_GRAPHICS} });
		createInfo.bindings.push_back({ vkr::DescriptorBinding{GBUFFER_IBL_REGISTER,       vkr::DESCRIPTOR_TYPE_SAMPLED_IMAGE,  1, vkr::SHADER_STAGE_ALL_GRAPHICS} });
		createInfo.bindings.push_back({ vkr::DescriptorBinding{GBUFFER_SAMPLER_REGISTER,   vkr::DESCRIPTOR_TYPE_SAMPLER,        1, vkr::SHADER_STAGE_ALL_GRAPHICS} });
		createInfo.bindings.push_back({ vkr::DescriptorBinding{GBUFFER_CONSTANTS_REGISTER, vkr::DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, vkr::SHADER_STAGE_ALL_GRAPHICS} });
		CHECKED_CALL(device.CreateDescriptorSetLayout(createInfo, &mGBufferReadLayout));
		// clang-format on

		// Allocate descriptor set
		CHECKED_CALL(device.AllocateDescriptorSet(mDescriptorPool, mGBufferReadLayout, &mGBufferReadSet));
		mGBufferReadSet->SetName("GBuffer Read");

		// Write descriptors
		vkr::WriteDescriptor writes[8] = {};
		writes[0].binding = GBUFFER_RT0_REGISTER;
		writes[0].arrayIndex = 0;
		writes[0].type = vkr::DESCRIPTOR_TYPE_SAMPLED_IMAGE;
		writes[0].pImageView = mGBufferRenderPass->GetRenderTargetTexture(0)->GetSampledImageView();
		writes[1].binding = GBUFFER_RT1_REGISTER;
		writes[1].arrayIndex = 0;
		writes[1].type = vkr::DESCRIPTOR_TYPE_SAMPLED_IMAGE;
		writes[1].pImageView = mGBufferRenderPass->GetRenderTargetTexture(1)->GetSampledImageView();
		writes[2].binding = GBUFFER_RT2_REGISTER;
		writes[2].arrayIndex = 0;
		writes[2].type = vkr::DESCRIPTOR_TYPE_SAMPLED_IMAGE;
		writes[2].pImageView = mGBufferRenderPass->GetRenderTargetTexture(2)->GetSampledImageView();
		writes[3].binding = GBUFFER_RT3_REGISTER;
		writes[3].arrayIndex = 0;
		writes[3].type = vkr::DESCRIPTOR_TYPE_SAMPLED_IMAGE;
		writes[3].pImageView = mGBufferRenderPass->GetRenderTargetTexture(3)->GetSampledImageView();
		// Environment map and IBL are not currently used.
		// Create a 1x1 image for unused textures.
		CHECKED_CALL(vkr::vkrUtil::CreateTexture1x1<uint8_t>(device.GetGraphicsQueue(), { 255, 255, 255, 255 }, &m1x1WhiteTexture));
		writes[4].binding = GBUFFER_ENV_REGISTER;
		writes[4].arrayIndex = 0;
		writes[4].type = vkr::DESCRIPTOR_TYPE_SAMPLED_IMAGE;
		writes[4].pImageView = m1x1WhiteTexture->GetSampledImageView();
		writes[5].binding = GBUFFER_IBL_REGISTER;
		writes[5].arrayIndex = 0;
		writes[5].type = vkr::DESCRIPTOR_TYPE_SAMPLED_IMAGE;
		writes[5].pImageView = m1x1WhiteTexture->GetSampledImageView();
		writes[6].binding = GBUFFER_SAMPLER_REGISTER;
		writes[6].type = vkr::DESCRIPTOR_TYPE_SAMPLER;
		writes[6].pSampler = mSampler;
		writes[7].binding = GBUFFER_CONSTANTS_REGISTER;
		writes[7].type = vkr::DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		writes[7].bufferOffset = 0;
		writes[7].bufferRange = WHOLE_SIZE;
		writes[7].pBuffer = mGBufferDrawAttrConstants;

		CHECKED_CALL(mGBufferReadSet->UpdateDescriptors(8, writes));
	}

	// Create per frame objects
	setupPerFrame();

	// Scene data
	{
		// Scene constants
		vkr::BufferCreateInfo bufferCreateInfo = {};
		bufferCreateInfo.size = vkr::MINIMUM_CONSTANT_BUFFER_SIZE;
		bufferCreateInfo.usageFlags.bits.transferSrc = true;
		bufferCreateInfo.memoryUsage = vkr::MemoryUsage::CPUToGPU;
		CHECKED_CALL(device.CreateBuffer(bufferCreateInfo, &mCpuSceneConstants));

		bufferCreateInfo.usageFlags.bits.transferDst = true;
		bufferCreateInfo.usageFlags.bits.uniformBuffer = true;
		bufferCreateInfo.memoryUsage = vkr::MemoryUsage::GPUOnly;
		CHECKED_CALL(device.CreateBuffer(bufferCreateInfo, &mGpuSceneConstants));

		// Light constants
		bufferCreateInfo = {};
		bufferCreateInfo.size = vkr::MINIMUM_STRUCTURED_BUFFER_SIZE;
		bufferCreateInfo.usageFlags.bits.transferSrc = true;
		bufferCreateInfo.memoryUsage = vkr::MemoryUsage::CPUToGPU;
		bufferCreateInfo.structuredElementStride = 32;
		CHECKED_CALL(device.CreateBuffer(bufferCreateInfo, &mCpuLightConstants));

		bufferCreateInfo.structuredElementStride = 32;
		bufferCreateInfo.usageFlags.bits.transferDst = true;
		bufferCreateInfo.usageFlags.bits.roStructuredBuffer = true;
		bufferCreateInfo.memoryUsage = vkr::MemoryUsage::GPUOnly;
		CHECKED_CALL(device.CreateBuffer(bufferCreateInfo, &mGpuLightConstants));

		// Descriptor set layout
		vkr::DescriptorSetLayoutCreateInfo createInfo = {};
		createInfo.bindings.push_back({ vkr::DescriptorBinding{SCENE_CONSTANTS_REGISTER, vkr::DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, vkr::SHADER_STAGE_ALL_GRAPHICS} });
		createInfo.bindings.push_back({ vkr::DescriptorBinding{LIGHT_DATA_REGISTER, vkr::DESCRIPTOR_TYPE_RO_STRUCTURED_BUFFER, 1, vkr::SHADER_STAGE_ALL_GRAPHICS} });
		CHECKED_CALL(device.CreateDescriptorSetLayout(createInfo, &mSceneDataLayout));

		// Allocate descriptor set
		CHECKED_CALL(device.AllocateDescriptorSet(mDescriptorPool, mSceneDataLayout, &mSceneDataSet));
		mSceneDataSet->SetName("Scene Data");

		// Update descriptor
		vkr::WriteDescriptor writes[2] = {};
		writes[0].binding = SCENE_CONSTANTS_REGISTER;
		writes[0].type = vkr::DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		writes[0].bufferOffset = 0;
		writes[0].bufferRange = WHOLE_SIZE;
		writes[0].pBuffer = mGpuSceneConstants;

		writes[1].binding = LIGHT_DATA_REGISTER;
		writes[1].arrayIndex = 0;
		writes[1].type = vkr::DESCRIPTOR_TYPE_RO_STRUCTURED_BUFFER;
		writes[1].bufferOffset = 0;
		writes[1].bufferRange = WHOLE_SIZE;
		writes[1].structuredElementCount = 1;
		writes[1].pBuffer = mGpuLightConstants;
		CHECKED_CALL(mSceneDataSet->UpdateDescriptors(2, writes));
	}

	// Create materials
	CHECKED_CALL(e020::Material::CreateMaterials(device, device.GetGraphicsQueue(), mDescriptorPool));

	// Create pipelines
	CHECKED_CALL(e020::Entity::CreatePipelines(device, mSceneDataLayout, mGBufferRenderPass));

	// Entities
	setupEntities();

	// Setup GBuffer lighting
	setupGBufferLightQuad();

	// Setup fullscreen quad for debug
	setupDebugDraw();

	// Setup fullscreen quad to draw to swapchain
	setupDrawToSwapchain();

	return true;
}

void Example_020::Shutdown()
{
	mPerFrame.clear();
	// TODO: доделать очистку
}

void Example_020::Update()
{
}

void Example_020::Render()
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

	// Update constants
	updateConstants();

#ifdef ENABLE_GPU_QUERIES
	// Read query results
	//if (GetFrameCount() > 0)
	{
		uint64_t data[2] = { 0 };
		CHECKED_CALL(frame.timestampQuery->GetData(data, 2 * sizeof(uint64_t)));
		mTotalGpuFrameTime = data[1] - data[0];
		if (device.PipelineStatsAvailable())
		{
			CHECKED_CALL(frame.pipelineStatsQuery->GetData(&mPipelineStatistics, sizeof(vkr::PipelineStatistics)));
		}
	}

	// Reset query
	frame.timestampQuery->Reset(0, 2);
	if (device.PipelineStatsAvailable()) 
	{
		frame.pipelineStatsQuery->Reset(0, 1);
	}
#endif

	// Build command buffer
	CHECKED_CALL(frame.cmd->Begin());
	{
		frame.cmd->SetScissors(mGBufferRenderPass->GetScissor());
		frame.cmd->SetViewports(mGBufferRenderPass->GetViewport());

		// =====================================================================
		//  GBuffer render
		// =====================================================================
		frame.cmd->TransitionImageLayout(
			mGBufferRenderPass,
			vkr::ResourceState::ShaderResource,
			vkr::ResourceState::RenderTarget,
			vkr::ResourceState::ShaderResource,
			vkr::ResourceState::DepthStencilWrite);
		frame.cmd->BeginRenderPass(mGBufferRenderPass, vkr::DRAW_PASS_CLEAR_FLAG_CLEAR_RENDER_TARGETS | vkr::DRAW_PASS_CLEAR_FLAG_CLEAR_DEPTH);
		{
#ifdef ENABLE_GPU_QUERIES
			frame.cmd->WriteTimestamp(frame.timestampQuery, vkr::PIPELINE_STAGE_TOP_OF_PIPE_BIT, 0);
#endif

#ifdef ENABLE_GPU_QUERIES
			if (device.PipelineStatsAvailable())
			{
				frame.cmd->BeginQuery(frame.pipelineStatsQuery, 0);
			}
#endif
			for (size_t i = 0; i < mEntities.size(); ++i) {
				mEntities[i].Draw(mSceneDataSet, frame.cmd);
			}
#ifdef ENABLE_GPU_QUERIES
			if (device.PipelineStatsAvailable())
			{
				frame.cmd->EndQuery(frame.pipelineStatsQuery, 0);
			}
#endif
		}
		frame.cmd->EndRenderPass();
		frame.cmd->TransitionImageLayout(
			mGBufferRenderPass,
			vkr::ResourceState::RenderTarget,
			vkr::ResourceState::ShaderResource,
			vkr::ResourceState::DepthStencilWrite,
			vkr::ResourceState::ShaderResource);

		// =====================================================================
		//  GBuffer light
		// =====================================================================
		frame.cmd->TransitionImageLayout(
			mGBufferLightPass,
			vkr::ResourceState::ShaderResource,
			vkr::ResourceState::RenderTarget,
			vkr::ResourceState::ShaderResource,
			vkr::ResourceState::DepthStencilRead);
		frame.cmd->BeginRenderPass(mGBufferLightPass, vkr::DRAW_PASS_CLEAR_FLAG_CLEAR_RENDER_TARGETS);
		{
			// Light scene using gbuffer data
			//
			vkr::DescriptorSet* sets[2] = { nullptr };
			sets[0] = mSceneDataSet;
			sets[1] = mGBufferReadSet;

			vkr::FullscreenQuad* pDrawQuad = mGBufferLightQuad;
			if (mDrawGBufferAttr) {
				pDrawQuad = mDebugDrawQuad;
			}
			frame.cmd->Draw(pDrawQuad, 2, sets);
		}
		frame.cmd->EndRenderPass();
#ifdef ENABLE_GPU_QUERIES
		frame.cmd->WriteTimestamp(frame.timestampQuery, vkr::PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 1);
#endif

		frame.cmd->TransitionImageLayout(
			mGBufferLightPass,
			vkr::ResourceState::RenderTarget,
			vkr::ResourceState::ShaderResource,
			vkr::ResourceState::DepthStencilRead,
			vkr::ResourceState::ShaderResource);

		// =====================================================================
		//  Blit to swapchain
		// =====================================================================
		vkr::RenderPassPtr renderPass = swapChain.GetRenderPass(imageIndex);
		ASSERT_MSG(!renderPass.IsNull(), "swapchain render pass object is null");

		frame.cmd->SetScissors(renderPass->GetScissor());
		frame.cmd->SetViewports(renderPass->GetViewport());

		frame.cmd->TransitionImageLayout(renderPass->GetRenderTargetImage(0), ALL_SUBRESOURCES, vkr::ResourceState::Present, vkr::ResourceState::RenderTarget);
		frame.cmd->BeginRenderPass(renderPass);
		{
			// Draw gbuffer light output to swapchain
			frame.cmd->Draw(mDrawToSwapchain, 1, &mDrawToSwapchainSet);

			// Draw ImGui
			render.DrawDebugInfo();
			{
				ImGui::Separator();

				ImGui::Checkbox("Draw GBuffer Attribute", &mDrawGBufferAttr);

				static const char* currentGBufferAttrName = mGBufferAttrNames[mGBufferAttrIndex];
				if (ImGui::BeginCombo("GBuffer Attribute", currentGBufferAttrName)) {
					for (size_t i = 0; i < mGBufferAttrNames.size(); ++i) {
						bool isSelected = (currentGBufferAttrName == mGBufferAttrNames[i]);
						if (ImGui::Selectable(mGBufferAttrNames[i], isSelected)) {
							currentGBufferAttrName = mGBufferAttrNames[i];
							mGBufferAttrIndex = static_cast<uint32_t>(i);
						}
						if (isSelected) {
							ImGui::SetItemDefaultFocus();
						}
					}
					ImGui::EndCombo();
				}

				ImGui::Separator();

				ImGui::Columns(2);

				uint64_t frequency = 0;
				device.GetGraphicsQueue()->GetTimestampFrequency(&frequency);
				ImGui::Text("Previous GPU Frame Time");
				ImGui::NextColumn();
				ImGui::Text("%f ms ", static_cast<float>(mTotalGpuFrameTime / static_cast<double>(frequency)) * 1000.0f);
				ImGui::NextColumn();

				ImGui::Separator();

				ImGui::Text("IAVertices");
				ImGui::NextColumn();
				ImGui::Text("%" PRIu64, mPipelineStatistics.IAVertices);
				ImGui::NextColumn();

				ImGui::Text("IAPrimitives");
				ImGui::NextColumn();
				ImGui::Text("%" PRIu64, mPipelineStatistics.IAPrimitives);
				ImGui::NextColumn();

				ImGui::Text("VSInvocations");
				ImGui::NextColumn();
				ImGui::Text("%" PRIu64, mPipelineStatistics.VSInvocations);
				ImGui::NextColumn();

				ImGui::Text("CInvocations");
				ImGui::NextColumn();
				ImGui::Text("%" PRIu64, mPipelineStatistics.CInvocations);
				ImGui::NextColumn();

				ImGui::Text("CPrimitives");
				ImGui::NextColumn();
				ImGui::Text("%" PRIu64, mPipelineStatistics.CPrimitives);
				ImGui::NextColumn();

				ImGui::Text("PSInvocations");
				ImGui::NextColumn();
				ImGui::Text("%" PRIu64, mPipelineStatistics.PSInvocations);
				ImGui::NextColumn();

				ImGui::Columns(1);
			}
			render.DrawImGui(frame.cmd);
		}
		frame.cmd->EndRenderPass();
		frame.cmd->TransitionImageLayout(renderPass->GetRenderTargetImage(0), ALL_SUBRESOURCES, vkr::ResourceState::RenderTarget, vkr::ResourceState::Present);
	}
#ifdef ENABLE_GPU_QUERIES
	// Resolve queries
	frame.cmd->ResolveQueryData(frame.timestampQuery, 0, 2);
	if (device.PipelineStatsAvailable())
	{
		frame.cmd->ResolveQueryData(frame.pipelineStatsQuery, 0, 1);
	}
#endif
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

void Example_020::MouseMove(int32_t x, int32_t y, int32_t dx, int32_t dy, MouseButton buttons)
{
	if (buttons == MouseButton::Left) {
		mTargetCamSwing += 0.25f * dx;
	}
}

void Example_020::setupPerFrame()
{
	auto& device = GetRenderDevice();
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

#ifdef ENABLE_GPU_QUERIES
		vkr::QueryCreateInfo queryCreateInfo = {};
		queryCreateInfo.type = vkr::QueryType::Timestamp;
		queryCreateInfo.count = 2;
		CHECKED_CALL(device.CreateQuery(queryCreateInfo, &frame.timestampQuery));

		// Pipeline statistics query pool
		if (device.PipelineStatsAvailable())
		{
			queryCreateInfo = {};
			queryCreateInfo.type = vkr::QueryType::PipelineStatistics;
			queryCreateInfo.count = 1;
			CHECKED_CALL(device.CreateQuery(queryCreateInfo, &frame.pipelineStatsQuery));
		}
#endif

		mPerFrame.push_back(frame);
	}
}

void Example_020::setupEntities()
{
	auto& device = GetRenderDevice();

	vkr::TriMeshOptions options = vkr::TriMeshOptions().Indices().Normals().VertexColors().TexCoords().Tangents();
	vkr::TriMesh        mesh = vkr::TriMesh::CreateSphere(1.0f, 128, 64, options);
	vkr::Geometry       geo;
	CHECKED_CALL(vkr::Geometry::Create(mesh, &geo));
	CHECKED_CALL(vkr::vkrUtil::CreateMeshFromGeometry(device.GetGraphicsQueue(), &geo, &mSphere));

	mEntities.resize(7);

	std::vector<e020::Material*> materials;
	materials.push_back(e020::Material::GetMaterialWood());
	materials.push_back(e020::Material::GetMaterialTiles());

	size_t n = 6;
	for (size_t i = 0; i < n; ++i)
	{
		size_t materialIndex = i % materials.size();

		e020::EntityCreateInfo createInfo = {};
		createInfo.pMesh = mSphere;
		createInfo.pMaterial = materials[materialIndex];
		CHECKED_CALL(mEntities[i].Create(device, device.GetGraphicsQueue(), mDescriptorPool, &createInfo));

		float t = i / static_cast<float>(n) * 2.0f * 3.141592f;
		float r = 3.0f;
		float x = r * cos(t);
		float y = 1.0f;
		float z = r * sin(t);
		mEntities[i].GetTransform().SetTranslation(float3(x, y, z));
	}

	// Box
	{
		e020::Entity* pEntity = &mEntities[n];

		mesh = vkr::TriMesh::CreateCube(float3(10, 1, 10), vkr::TriMeshOptions(options).TexCoordScale(float2(5)));
		CHECKED_CALL(vkr::Geometry::Create(mesh, &geo));
		CHECKED_CALL(vkr::vkrUtil::CreateMeshFromGeometry(device.GetGraphicsQueue(), &geo, &mBox));

		e020::EntityCreateInfo createInfo = {};
		createInfo.pMesh = mBox;
		createInfo.pMaterial = e020::Material::GetMaterialTiles();
		CHECKED_CALL(pEntity->Create(device, device.GetGraphicsQueue(), mDescriptorPool, &createInfo));
		pEntity->GetTransform().SetTranslation(float3(0, -0.5f, 0));
	}
}

void Example_020::setupGBufferPasses()
{
	auto& device = GetRenderDevice();

	// GBuffer render draw pass
	{
		// Usage flags for render target and depth stencil will automatically be added during create. So we only need to specify the additional usage flags here.
		vkr::ImageUsageFlags        additionalUsageFlags = vkr::IMAGE_USAGE_SAMPLED;
		vkr::RenderTargetClearValue rtvClearValue = { 0, 0, 0, 0 };
		vkr::DepthStencilClearValue dsvClearValue = { 1.0f, 0xFF };

		vkr::DrawPassCreateInfo createInfo = {};
		createInfo.width = GetWindowWidth();
		createInfo.height = GetWindowHeight();
		createInfo.renderTargetCount = 4;
		createInfo.renderTargetFormats[0] = vkr::FORMAT_R16G16B16A16_FLOAT;
		createInfo.renderTargetFormats[1] = vkr::FORMAT_R16G16B16A16_FLOAT;
		createInfo.renderTargetFormats[2] = vkr::FORMAT_R16G16B16A16_FLOAT;
		createInfo.renderTargetFormats[3] = vkr::FORMAT_R16G16B16A16_FLOAT;
		createInfo.depthStencilFormat = vkr::FORMAT_D32_FLOAT;
		createInfo.renderTargetUsageFlags[0] = additionalUsageFlags;
		createInfo.renderTargetUsageFlags[1] = additionalUsageFlags;
		createInfo.renderTargetUsageFlags[2] = additionalUsageFlags;
		createInfo.renderTargetUsageFlags[3] = additionalUsageFlags;
		createInfo.depthStencilUsageFlags = additionalUsageFlags;
		createInfo.renderTargetInitialStates[0] = vkr::ResourceState::ShaderResource;
		createInfo.renderTargetInitialStates[1] = vkr::ResourceState::ShaderResource;
		createInfo.renderTargetInitialStates[2] = vkr::ResourceState::ShaderResource;
		createInfo.renderTargetInitialStates[3] = vkr::ResourceState::ShaderResource;
		createInfo.depthStencilInitialState = vkr::ResourceState::ShaderResource;
		createInfo.renderTargetClearValues[0] = rtvClearValue;
		createInfo.renderTargetClearValues[1] = rtvClearValue;
		createInfo.renderTargetClearValues[2] = rtvClearValue;
		createInfo.renderTargetClearValues[3] = rtvClearValue;
		createInfo.depthStencilClearValue = dsvClearValue;

		CHECKED_CALL(device.CreateDrawPass(createInfo, &mGBufferRenderPass));
	}

	// GBuffer light render target
	{
		vkr::TextureCreateInfo createInfo = {};
		createInfo.imageType = vkr::IMAGE_TYPE_2D;
		createInfo.width = mGBufferRenderPass->GetWidth();
		createInfo.height = mGBufferRenderPass->GetHeight();
		createInfo.depth = 1;
		createInfo.imageFormat = vkr::FORMAT_R8G8B8A8_UNORM;
		createInfo.sampleCount = vkr::SAMPLE_COUNT_1;
		createInfo.mipLevelCount = 1;
		createInfo.arrayLayerCount = 1;
		createInfo.usageFlags.bits.colorAttachment = true;
		createInfo.usageFlags.bits.sampled = true;
		createInfo.memoryUsage = vkr::MemoryUsage::GPUOnly;
		createInfo.initialState = vkr::ResourceState::ShaderResource;
		createInfo.RTVClearValue = { 0, 0, 0, 0 };
		createInfo.DSVClearValue = { 1.0f, 0xFF }; // Not used - we won't write to depth

		CHECKED_CALL(device.CreateTexture(createInfo, &mGBufferLightRenderTarget));
	}

	// GBuffer light draw pass
	{
		vkr::DrawPassCreateInfo3 createInfo = {};
		createInfo.width = mGBufferRenderPass->GetWidth();
		createInfo.height = mGBufferRenderPass->GetHeight();
		createInfo.renderTargetCount = 1;
		createInfo.pRenderTargetTextures[0] = mGBufferLightRenderTarget;
		createInfo.pDepthStencilTexture = mGBufferRenderPass->GetDepthStencilTexture();
		createInfo.depthStencilState = vkr::ResourceState::DepthStencilRead;

		CHECKED_CALL(device.CreateDrawPass(createInfo, &mGBufferLightPass));
	}
}

void Example_020::setupGBufferLightQuad()
{
	auto& device = GetRenderDevice();

	vkr::ShaderModulePtr VS;
	CHECKED_CALL(device.CreateShader("basic/shaders/gbuffer", "DeferredLight.vs", &VS));

	vkr::ShaderModulePtr PS;
	CHECKED_CALL(device.CreateShader("basic/shaders/gbuffer", "DeferredLight.ps", &PS));

	vkr::FullscreenQuadCreateInfo createInfo = {};
	createInfo.VS = VS;
	createInfo.PS = PS;
	createInfo.setCount = 2;
	createInfo.sets[0].set = 0;
	createInfo.sets[0].pLayout = mSceneDataLayout;
	createInfo.sets[1].set = 1;
	createInfo.sets[1].pLayout = mGBufferReadLayout;
	createInfo.renderTargetCount = 1;
	createInfo.renderTargetFormats[0] = mGBufferLightPass->GetRenderTargetTexture(0)->GetImageFormat();
	createInfo.depthStencilFormat = mGBufferLightPass->GetDepthStencilTexture()->GetImageFormat();

	CHECKED_CALL(device.CreateFullscreenQuad(createInfo, &mGBufferLightQuad));
}

void Example_020::setupDebugDraw()
{
	auto& device = GetRenderDevice();

	vkr::ShaderModulePtr VS;
	CHECKED_CALL(device.CreateShader("basic/shaders/gbuffer", "DrawGBufferAttribute.vs", &VS));

	vkr::ShaderModulePtr PS;
	CHECKED_CALL(device.CreateShader("basic/shaders/gbuffer", "DrawGBufferAttribute.ps", &PS));

	vkr::FullscreenQuadCreateInfo createInfo = {};
	createInfo.VS = VS;
	createInfo.PS = PS;
	createInfo.setCount = 2;
	createInfo.sets[0].set = 0;
	createInfo.sets[0].pLayout = mSceneDataLayout;
	createInfo.sets[1].set = 1;
	createInfo.sets[1].pLayout = mGBufferReadLayout;
	createInfo.renderTargetCount = 1;
	createInfo.renderTargetFormats[0] = mGBufferLightPass->GetRenderTargetTexture(0)->GetImageFormat();
	createInfo.depthStencilFormat = mGBufferLightPass->GetDepthStencilTexture()->GetImageFormat();

	CHECKED_CALL(device.CreateFullscreenQuad(createInfo, &mDebugDrawQuad));
}

void Example_020::setupDrawToSwapchain()
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

		CHECKED_CALL(device.CreateFullscreenQuad(createInfo, &mDrawToSwapchain));
	}

	// Allocate descriptor set
	CHECKED_CALL(device.AllocateDescriptorSet(mDescriptorPool, mDrawToSwapchainLayout, &mDrawToSwapchainSet));

	// Write descriptors
	{
		vkr::WriteDescriptor writes[2] = {};
		writes[0].binding = 0;
		writes[0].arrayIndex = 0;
		writes[0].type = vkr::DESCRIPTOR_TYPE_SAMPLED_IMAGE;
		writes[0].pImageView = mGBufferLightPass->GetRenderTargetTexture(0)->GetSampledImageView();

		writes[1].binding = 1;
		writes[1].type = vkr::DESCRIPTOR_TYPE_SAMPLER;
		writes[1].pSampler = mSampler;

		CHECKED_CALL(mDrawToSwapchainSet->UpdateDescriptors(2, writes));
	}
}

void Example_020::updateConstants()
{
	auto& device = GetRenderDevice();

	const float kPi = 3.1451592f;

	// Scene constants
	{
		mCamSwing += (mTargetCamSwing - mCamSwing) * 0.1f;

		float t = glm::radians(mCamSwing - kPi / 2.0f);
		float x = 8.0f * cos(t);
		float z = 8.0f * sin(t);
		mCamera.LookAt(float3(x, 3, z), float3(0, 0.5f, 0));

		HLSL_PACK_BEGIN();
		struct HlslSceneData
		{
			hlsl_uint<4>      frameNumber;
			hlsl_float<12>    time;
			hlsl_float4x4<64> viewProjectionMatrix;
			hlsl_float3<12>   eyePosition;
			hlsl_uint<4>      lightCount;
			hlsl_float<4>     ambient;
			hlsl_float<4>     iblLevelCount;
			hlsl_float<4>     envLevelCount;
		};
		HLSL_PACK_END();

		void* pMappedAddress = nullptr;
		CHECKED_CALL(mCpuSceneConstants->MapMemory(0, &pMappedAddress));

		HlslSceneData* pSceneData = static_cast<HlslSceneData*>(pMappedAddress);
		pSceneData->viewProjectionMatrix = mCamera.GetViewProjectionMatrix();
		pSceneData->eyePosition = mCamera.GetEyePosition();
		pSceneData->lightCount = 5;
		pSceneData->ambient = 0.0f;
		pSceneData->iblLevelCount = 0;
		pSceneData->envLevelCount = 0;

		mCpuSceneConstants->UnmapMemory();

		vkr::BufferToBufferCopyInfo copyInfo = { mCpuSceneConstants->GetSize() };
		CHECKED_CALL(device.GetGraphicsQueue()->CopyBufferToBuffer(&copyInfo, mCpuSceneConstants, mGpuSceneConstants, vkr::ResourceState::ConstantBuffer, vkr::ResourceState::ConstantBuffer));
	}

	// Light constants
	{
		HLSL_PACK_BEGIN();
		struct HlslLight
		{
			hlsl_uint<4>    type;
			hlsl_float3<12> position;
			hlsl_float3<12> color;
			hlsl_float<4>   intensity;
		};
		HLSL_PACK_END();

		void* pMappedAddress = nullptr;
		CHECKED_CALL(mCpuLightConstants->MapMemory(0, &pMappedAddress));

		//float t = GetElapsedSeconds();
		static float t = 0.0;
		t += 0.001f;

		HlslLight* pLight = static_cast<HlslLight*>(pMappedAddress);
		pLight[0].position = float3(10, 5, 10) * float3(sin(t), 1, cos(t));
		pLight[1].position = float3(-10, 2, 5) * float3(cos(t), 1, sin(t / 4 + kPi / 2));
		pLight[2].position = float3(1, 10, 3) * float3(sin(t / 2), 1, cos(t / 2));
		pLight[3].position = float3(-1, 0, 15) * float3(sin(t / 3), 1, cos(t / 3));
		pLight[4].position = float3(-1, 2, -5) * float3(sin(t / 4), 1, cos(t / 4));
		pLight[5].position = float3(-6, 3, -4) * float3(sin(t / 5), 1, cos(t / 5));

		pLight[0].intensity = 0.5f;
		pLight[1].intensity = 0.25f;
		pLight[2].intensity = 0.5f;
		pLight[3].intensity = 0.25f;
		pLight[4].intensity = 0.5f;
		pLight[5].intensity = 0.25f;

		mCpuLightConstants->UnmapMemory();

		vkr::BufferToBufferCopyInfo copyInfo = { mCpuLightConstants->GetSize() };
		device.GetGraphicsQueue()->CopyBufferToBuffer(&copyInfo, mCpuLightConstants, mGpuLightConstants, vkr::ResourceState::ConstantBuffer, vkr::ResourceState::ConstantBuffer);
	}

	// Model constants
	for (size_t i = 0; i < mEntities.size(); ++i) {
		mEntities[i].UpdateConstants(device.GetGraphicsQueue());
	}

	// GBuffer constants
	{
		HLSL_PACK_BEGIN();
		struct HlslGBufferData
		{
			hlsl_uint<4> enableIBL;
			hlsl_uint<4> enableEnv;
			hlsl_uint<4> debugAttrIndex;
		};
		HLSL_PACK_END();

		void* pMappedAddress = nullptr;
		CHECKED_CALL(mGBufferDrawAttrConstants->MapMemory(0, &pMappedAddress));

		HlslGBufferData* pGBufferData = static_cast<HlslGBufferData*>(pMappedAddress);

		pGBufferData->enableIBL = mEnableIBL;
		pGBufferData->enableEnv = mEnableEnv;
		pGBufferData->debugAttrIndex = mGBufferAttrIndex;

		mGBufferDrawAttrConstants->UnmapMemory();
	}
}