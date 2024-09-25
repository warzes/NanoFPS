#include "stdafx.h"
#include "Core.h"
#include "Scene.h"

#pragma region Scene Node

namespace scene
{

	Node::Node(scene::Scene* pScene)
		: mScene(pScene)
	{
	}

	Node::~Node()
	{
	}

	void Node::SetVisible(bool visible, bool recursive)
	{
		mVisible = visible;
		if (recursive) {
			for (auto pChild : mChildren) {
				pChild->SetVisible(visible, recursive);
			}
		}
	}

	const float4x4& Node::GetEvaluatedMatrix() const
	{
		if (mEvaluatedDirty) {
			float4x4 parentEvaluatedMatrix = float4x4(1);
			if (!IsNull(mParent)) {
				parentEvaluatedMatrix = mParent->GetEvaluatedMatrix();
			}
			auto& concatenatedMatrix = GetConcatenatedMatrix();
			mEvaluatedMatrix = parentEvaluatedMatrix * concatenatedMatrix;
			mEvaluatedDirty = false;
		}
		return mEvaluatedMatrix;
	}

	void Node::setParent(scene::Node* pNewParent)
	{
		mParent = pNewParent;
		setEvaluatedDirty();
	}

	void Node::setEvaluatedDirty()
	{
		mEvaluatedDirty = true;
		for (auto& pChild : mChildren) {
			pChild->setEvaluatedDirty();
		}
	}

	void Node::SetTranslation(const float3& translation)
	{
		Transform::SetTranslation(translation);
		setEvaluatedDirty();
	}

	void Node::SetRotation(const float3& rotation)
	{
		Transform::SetRotation(rotation);
		setEvaluatedDirty();
	}

	void Node::SetScale(const float3& scale)
	{
		Transform::SetScale(scale);
		setEvaluatedDirty();
	}

	void Node::SetRotationOrder(Transform::RotationOrder value)
	{
		Transform::SetRotationOrder(value);
		setEvaluatedDirty();
	}

	scene::Node* Node::GetChild(uint32_t index) const
	{
		scene::Node* pChild = nullptr;
		GetElement(index, mChildren, &pChild);
		return pChild;
	}

	bool Node::IsInSubTree(const scene::Node* pNode)
	{
		bool inSubTree = (pNode == this);
		if (!inSubTree) {
			for (auto pChild : mChildren) {
				inSubTree = pChild->IsInSubTree(pNode);
				if (inSubTree) {
					break;
				}
			}
		}
		return inSubTree;
	}

	Result Node::AddChild(scene::Node* pNewChild)
	{
		// Cannot add child if current node is standalone
		if (IsNull(mScene)) {
			return ERROR_SCENE_INVALID_STANDALONE_OPERATION;
		}

		// Cannot add NULL child
		if (IsNull(pNewChild)) {
			return ERROR_UNEXPECTED_NULL_ARGUMENT;
		}

		// Cannot add self as a child
		if (pNewChild == this) {
			return ERROR_SCENE_INVALID_NODE_HIERARCHY;
		}

		// Cannot add child if current node is in child's subtree
		if (pNewChild->IsInSubTree(this)) {
			return ERROR_SCENE_INVALID_NODE_HIERARCHY;
		}

		// Don't add new child if it already exists
		auto it = std::find(mChildren.begin(), mChildren.end(), pNewChild);
		if (it != mChildren.end()) {
			return ERROR_DUPLICATE_ELEMENT;
		}

		// Don't add new child if it currently has a parent
		const auto pCurrentParent = pNewChild->GetParent();
		if (!IsNull(pCurrentParent)) {
			return ERROR_SCENE_NODE_ALREADY_HAS_PARENT;
		}

		pNewChild->setParent(this);

		mChildren.push_back(pNewChild);

		return SUCCESS;
	}

	scene::Node* Node::RemoveChild(const scene::Node* pChild)
	{
		// Return null if pChild is null or if pChild is self
		if (IsNull(pChild) || (pChild == this)) {
			return nullptr;
		}

		// Return NULL if pChild isn't in mChildren
		auto it = std::find(mChildren.begin(), mChildren.end(), pChild);
		if (it == mChildren.end()) {
			return nullptr;
		}

		// Remove pChild from mChildren
		mChildren.erase(
			std::remove(mChildren.begin(), mChildren.end(), pChild),
			mChildren.end());

		scene::Node* pParentlessChild = const_cast<scene::Node*>(pChild);
		pParentlessChild->setParent(nullptr);

		return pParentlessChild;
	}

	MeshNode::MeshNode(const scene::MeshRef& mesh, scene::Scene* pScene)
		: scene::Node(pScene),
		mMesh(mesh)
	{
	}

	MeshNode::~MeshNode()
	{
	}

	void MeshNode::SetMesh(const scene::MeshRef& mesh)
	{
		mMesh = mesh;
	}

	CameraNode::CameraNode(std::unique_ptr<Camera>&& camera, scene::Scene* pScene)
		: scene::Node(pScene),
		mCamera(std::move(camera))
	{
	}

	CameraNode::~CameraNode()
	{
	}

	void CameraNode::UpdateCameraLookAt()
	{
		const float4x4& rotationMatrix = this->GetRotationMatrix();

		// Rotate the view direction
		float3 viewDir = rotationMatrix * float4(0, 0, -1, 0);

		const float3& eyePos = this->GetTranslation();
		float3        target = eyePos + viewDir;

		mCamera->LookAt(eyePos, target, mCamera->GetWorldUp());
	}

	void CameraNode::SetTranslation(const float3& translation)
	{
		scene::Node::SetTranslation(translation);
		UpdateCameraLookAt();
	}

	void CameraNode::SetRotation(const float3& rotation)
	{
		scene::Node::SetRotation(rotation);
		UpdateCameraLookAt();
	}

	LightNode::LightNode(scene::Scene* pScene)
		: scene::Node(pScene)
	{
	}

	LightNode::~LightNode()
	{
	}

	void LightNode::SetRotation(const float3& rotation)
	{
		scene::Node::SetRotation(rotation);
	}

} // namespace scene

#pragma endregion

#pragma region Scene Material

namespace scene
{
	Image::Image(
		vkr::Image* pImage,
		vkr::SampledImageView* pImageView)
		: mImage(pImage),
		mImageView(pImageView)
	{
	}

	Image::~Image()
	{
		if (mImageView) {
			auto pDevice = mImageView->GetDevice();
			pDevice->DestroySampledImageView(mImageView);
		}

		if (mImage) {
			auto pDevice = mImage->GetDevice();
			pDevice->DestroyImage(mImage);
		}
	}

	Sampler::Sampler(
		vkr::Sampler* pSampler)
		: mSampler(pSampler)
	{
	}

	Sampler::~Sampler()
	{
		if (mSampler) {
			auto pDevice = mSampler->GetDevice();
			pDevice->DestroySampler(mSampler);
		}
	}

	Texture::Texture(
		const scene::ImageRef   image,
		const scene::SamplerRef sampler)
		: mImage(image),
		mSampler(sampler)
	{
	}

	TextureView::TextureView(
		const scene::TextureRef& texture,
		float2                   texCoordTranslate,
		float                    texCoordRotate,
		float2                   texCoordScale)
		: mTexture(texture),
		mTexCoordTranslate(texCoordTranslate),
		mTexCoordRotate(texCoordRotate),
		mTexCoordScale(texCoordScale)
	{
		float2x2 T = glm::translate(float3(mTexCoordTranslate, 0));
		float2x2 R = glm::rotate(mTexCoordRotate, float3(0, 0, 1));
		float2x2 S = glm::scale(float3(mTexCoordScale, 0));
		mTexCoordTransform = T * R * S;
	}

	scene::VertexAttributeFlags ErrorMaterial::GetRequiredVertexAttributes() const
	{
		scene::VertexAttributeFlags attrFlags = scene::VertexAttributeFlags::None();
		return attrFlags;
	}

	scene::VertexAttributeFlags DebugMaterial::GetRequiredVertexAttributes() const
	{
		scene::VertexAttributeFlags attrFlags = scene::VertexAttributeFlags::None();
		attrFlags.bits.texCoords = true;
		attrFlags.bits.normals = true;
		attrFlags.bits.tangents = true;
		attrFlags.bits.colors = true;
		return attrFlags;
	}

	scene::VertexAttributeFlags UnlitMaterial::GetRequiredVertexAttributes() const
	{
		scene::VertexAttributeFlags attrFlags = scene::VertexAttributeFlags::None();
		attrFlags.bits.texCoords = true;
		return attrFlags;
	}

	bool UnlitMaterial::HasTextures() const
	{
		bool hasBaseColorTex = HasBaseColorTexture();
		return hasBaseColorTex;
	}

	void UnlitMaterial::SetBaseColorFactor(const float4& value)
	{
		mBaseColorFactor = value;
	}

	scene::VertexAttributeFlags StandardMaterial::GetRequiredVertexAttributes() const
	{
		scene::VertexAttributeFlags attrFlags = scene::VertexAttributeFlags::None();
		attrFlags.bits.texCoords = true;
		attrFlags.bits.normals = true;
		attrFlags.bits.tangents = true;
		attrFlags.bits.colors = true;
		return attrFlags;
	}

	bool StandardMaterial::HasTextures() const
	{
		bool hasBaseColorTex = HasBaseColorTexture();
		bool hasMetallicRoughnessTex = HasMetallicRoughnessTexture();
		bool hasNormalTex = HasNormalTexture();
		bool hasOcclusionTex = HasOcclusionTexture();
		bool hasEmissiveTex = HasEmissiveTexture();
		bool hasTextures = hasBaseColorTex || hasMetallicRoughnessTex || hasNormalTex || hasOcclusionTex || hasNormalTex;
		return hasTextures;
	}

	void StandardMaterial::SetBaseColorFactor(const float4& value)
	{
		mBaseColorFactor = value;
	}

	void StandardMaterial::SetMetallicFactor(float value)
	{
		mMetallicFactor = value;
	}

	void StandardMaterial::SetRoughnessFactor(float value)
	{
		mRoughnessFactor = value;
	}

	void StandardMaterial::SetOcclusionStrength(float value)
	{
		mOcclusionStrength = value;
	}

	void StandardMaterial::SetEmissiveFactor(const float3& value)
	{
		mEmissiveFactor = value;
	}

	void StandardMaterial::SetEmissiveStrength(float value)
	{
		mEmissiveStrength = value;
	}

	scene::VertexAttributeFlags MaterialFactory::GetRequiredVertexAttributes(const std::string& materialIdent) const
	{
		scene::VertexAttributeFlags attrFlags = scene::VertexAttributeFlags::None();

		// Error material only uses position and no other vertex attribues.
		// No need to put in a specific if statement for it.
		if (materialIdent == MATERIAL_IDENT_UNLIT) {
			attrFlags = UnlitMaterial().GetRequiredVertexAttributes();
		}
		else if (materialIdent == MATERIAL_IDENT_STANDARD) {
			attrFlags = StandardMaterial().GetRequiredVertexAttributes();
		}

		return attrFlags;
	}

	scene::Material* MaterialFactory::CreateMaterial(
		const std::string& materialIdent) const
	{
		scene::Material* pMaterial = nullptr;

		if (materialIdent == MATERIAL_IDENT_UNLIT) {
			pMaterial = new scene::UnlitMaterial();
		}
		else if (materialIdent == MATERIAL_IDENT_STANDARD) {
			pMaterial = new scene::StandardMaterial();
		}
		else if (materialIdent == MATERIAL_IDENT_DEBUG) {
			pMaterial = new scene::DebugMaterial();
		}
		else {
			pMaterial = new scene::ErrorMaterial();
		}

		return pMaterial;
	}

} // namespace scene

#pragma endregion

#pragma region Scene Pipeline Args

namespace scene {

	void CopyMaterialTextureParams(
		const std::unordered_map<const scene::Sampler*, uint32_t>& samplersIndexMap,
		const std::unordered_map<const scene::Image*, uint32_t>& imagesIndexMap,
		const scene::TextureView& srcTextureView,
		scene::MaterialTextureParams& dstTextureParams)
	{
		// Set default values
		dstTextureParams.samplerIndex = UINT32_MAX;
		dstTextureParams.textureIndex = UINT32_MAX;
		dstTextureParams.texCoordTransform = srcTextureView.GetTexCoordTransform();

		// Bail if we don't have a texture
		if (IsNull(srcTextureView.GetTexture())) {
			return;
		}

		auto itSampler = samplersIndexMap.find(srcTextureView.GetTexture()->GetSampler());
		auto itImage = imagesIndexMap.find(srcTextureView.GetTexture()->GetImage());
		if ((itSampler != samplersIndexMap.end()) && (itImage != imagesIndexMap.end())) {
			dstTextureParams.samplerIndex = itSampler->second;
			dstTextureParams.textureIndex = itImage->second;
		}
	}

	MaterialPipelineArgs::MaterialPipelineArgs()
	{
	}

	MaterialPipelineArgs::~MaterialPipelineArgs()
	{
		if (mDescriptorSet) {
			auto pDevice = mDescriptorSet->GetDevice();
			pDevice->FreeDescriptorSet(mDescriptorSet.Get());
			mDescriptorSet.Reset();
		}

		if (mDescriptorSetLayout) {
			auto pDevice = mDescriptorSetLayout->GetDevice();
			pDevice->DestroyDescriptorSetLayout(mDescriptorSetLayout.Get());
			mDescriptorSetLayout.Reset();
		}

		if (mDescriptorPool) {
			auto pDevice = mDescriptorPool->GetDevice();
			pDevice->DestroyDescriptorPool(mDescriptorPool.Get());
			mDescriptorPool.Reset();
		}

		if (mCpuConstantParamsBuffer) {
			if (!IsNull(mConstantParamsMappedAddress)) {
				mCpuConstantParamsBuffer->UnmapMemory();
			}

			auto pDevice = mCpuConstantParamsBuffer->GetDevice();
			pDevice->DestroyBuffer(mCpuConstantParamsBuffer.Get());
			mCpuConstantParamsBuffer.Reset();
		}

		if (mGpuConstantParamsBuffer) {
			auto pDevice = mGpuConstantParamsBuffer->GetDevice();
			pDevice->DestroyBuffer(mGpuConstantParamsBuffer.Get());
			mGpuConstantParamsBuffer.Reset();
		}

		if (mCpuInstanceParamsBuffer) {
			if (!IsNull(mInstanceParamsMappedAddress)) {
				mCpuInstanceParamsBuffer->UnmapMemory();
			}

			auto pDevice = mCpuInstanceParamsBuffer->GetDevice();
			pDevice->DestroyBuffer(mCpuInstanceParamsBuffer.Get());
			mCpuInstanceParamsBuffer.Reset();
		}

		if (mGpuInstanceParamsBuffer) {
			auto pDevice = mGpuInstanceParamsBuffer->GetDevice();
			pDevice->DestroyBuffer(mGpuInstanceParamsBuffer.Get());
			mGpuInstanceParamsBuffer.Reset();
		}

		if (mCpuMateriaParamsBuffer) {
			if (!IsNull(mMaterialParamsMappedAddress)) {
				mCpuMateriaParamsBuffer->UnmapMemory();
			}

			auto pDevice = mCpuMateriaParamsBuffer->GetDevice();
			pDevice->DestroyBuffer(mCpuMateriaParamsBuffer.Get());
			mCpuMateriaParamsBuffer.Reset();
		}

		if (mGpuMateriaParamsBuffer) {
			auto pDevice = mGpuMateriaParamsBuffer->GetDevice();
			pDevice->DestroyBuffer(mGpuMateriaParamsBuffer.Get());
			mGpuMateriaParamsBuffer.Reset();
		}
	}

	Result MaterialPipelineArgs::InitializeDefaultObjects(vkr::RenderDevice* pDevice)
	{
		if (IsNull(pDevice)) {
			return ERROR_UNEXPECTED_NULL_ARGUMENT;
		}

		// Default sampler
		{
			vkr::SamplerCreateInfo createInfo = {};

			auto ppxres = pDevice->CreateSampler(createInfo, &mDefaultSampler);
			if (Failed(ppxres)) {
				return ppxres;
			}
		}

		// Default texture
		{
			Bitmap bitmap;
			auto        ppxres = Bitmap::Create(1, 1, Bitmap::FORMAT_RGBA_UINT8, &bitmap);
			if (Failed(ppxres)) {
				return ppxres;
			}

			// Fill purple
			bitmap.Fill<uint8_t>(0xFF, 0, 0xFF, 0xFF);

			ppxres = vkr::vkrUtil::CreateTextureFromBitmap(pDevice->GetGraphicsQueue(), &bitmap, &mDefaultTexture);
			if (Failed(ppxres)) {
				return ppxres;
			}
		}

		// Default BRDF LUT sampler
		{
			vkr::SamplerCreateInfo createInfo = {};
			createInfo.magFilter = vkr::Filter::Linear;
			createInfo.minFilter = vkr::Filter::Linear;
			createInfo.mipmapMode = vkr::SamplerMipmapMode::Linear;
			createInfo.addressModeU = vkr::SamplerAddressMode::ClampToEdge;
			createInfo.addressModeV = vkr::SamplerAddressMode::ClampToEdge;
			createInfo.addressModeW = vkr::SamplerAddressMode::ClampToEdge;

			auto ppxres = pDevice->CreateSampler(createInfo, &mDefaultBRDFLUTSampler);
			if (Failed(ppxres)) {
				return ppxres;
			}
		}

		// Default BRD LUT texture
		{
			// Favor speed, so use the PNG instad of the HDR
			auto ppxres = vkr::vkrUtil::CreateTextureFromFile(
				pDevice->GetGraphicsQueue(),
				"Data/Common/Textures/brdf_lut.png",
				&mDefaultBRDFLUTTexture);
			if (Failed(ppxres)) {
				return ppxres;
			}
		}

		// Default IBL irradiance sampler
		{
			vkr::SamplerCreateInfo createInfo = {};
			createInfo.magFilter = vkr::Filter::Linear;
			createInfo.minFilter = vkr::Filter::Linear;
			createInfo.mipmapMode = vkr::SamplerMipmapMode::Linear;
			createInfo.addressModeU = vkr::SamplerAddressMode::Repeat;
			createInfo.addressModeV = vkr::SamplerAddressMode::ClampToEdge;
			createInfo.addressModeW = vkr::SamplerAddressMode::ClampToEdge;

			auto ppxres = pDevice->CreateSampler(createInfo, &mDefaultIBLIrradianceSampler);
			if (Failed(ppxres)) {
				return ppxres;
			}
		}

		// Default IBL environment sampler
		{
			vkr::SamplerCreateInfo createInfo = {};
			createInfo.magFilter = vkr::Filter::Linear;
			createInfo.minFilter = vkr::Filter::Linear;
			createInfo.mipmapMode = vkr::SamplerMipmapMode::Linear;
			createInfo.addressModeU = vkr::SamplerAddressMode::Repeat;
			createInfo.addressModeV = vkr::SamplerAddressMode::ClampToEdge;
			createInfo.addressModeW = vkr::SamplerAddressMode::ClampToEdge;
			createInfo.mipLodBias = 0.65f;
			createInfo.minLod = 0.0f;
			createInfo.maxLod = 1000.0f;

			auto ppxres = pDevice->CreateSampler(createInfo, &mDefaultIBLEnvironmentSampler);
			if (Failed(ppxres)) {
				return ppxres;
			}
		}

		// Default IBL texture
		{
			Bitmap bitmap;
			auto        ppxres = Bitmap::Create(1, 1, Bitmap::FORMAT_RGBA_UINT8, &bitmap);
			if (Failed(ppxres)) {
				return ppxres;
			}

			// Fill white
			bitmap.Fill<uint8_t>(0xFF, 0xFF, 0xFF, 0xFF);

			ppxres = vkr::vkrUtil::CreateTextureFromBitmap(pDevice->GetGraphicsQueue(), &bitmap, &mDefaultIBLTexture);
			if (Failed(ppxres)) {
				return ppxres;
			}
		}

		return SUCCESS;
	}

	Result MaterialPipelineArgs::InitializeDescriptorSet(vkr::RenderDevice* pDevice)
	{
		if (IsNull(pDevice)) {
			return ERROR_UNEXPECTED_NULL_ARGUMENT;
		}

		// Descriptor bindings
		//
		// clang-format off
		std::vector<vkr::DescriptorBinding> bindings = {
		vkr::DescriptorBinding{FRAME_PARAMS_REGISTER,            vkr::DESCRIPTOR_TYPE_UNIFORM_BUFFER,       1, vkr::SHADER_STAGE_ALL},
		vkr::DescriptorBinding{CAMERA_PARAMS_REGISTER,           vkr::DESCRIPTOR_TYPE_UNIFORM_BUFFER,       1, vkr::SHADER_STAGE_ALL},
		vkr::DescriptorBinding{INSTANCE_PARAMS_REGISTER,         vkr::DESCRIPTOR_TYPE_RO_STRUCTURED_BUFFER, 1, vkr::SHADER_STAGE_ALL},
		vkr::DescriptorBinding{MATERIAL_PARAMS_REGISTER,         vkr::DESCRIPTOR_TYPE_RO_STRUCTURED_BUFFER, 1, vkr::SHADER_STAGE_ALL},
		vkr::DescriptorBinding{BRDF_LUT_SAMPLER_REGISTER,        vkr::DESCRIPTOR_TYPE_SAMPLER,              1, vkr::SHADER_STAGE_ALL},
		vkr::DescriptorBinding{BRDF_LUT_TEXTURE_REGISTER,        vkr::DESCRIPTOR_TYPE_SAMPLED_IMAGE,        1, vkr::SHADER_STAGE_ALL},
		vkr::DescriptorBinding{IBL_IRRADIANCE_SAMPLER_REGISTER,  vkr::DESCRIPTOR_TYPE_SAMPLER,              1, vkr::SHADER_STAGE_ALL},
		vkr::DescriptorBinding{IBL_ENVIRONMENT_SAMPLER_REGISTER, vkr::DESCRIPTOR_TYPE_SAMPLER,              1, vkr::SHADER_STAGE_ALL},
		vkr::DescriptorBinding{IBL_IRRADIANCE_MAP_REGISTER,      vkr::DESCRIPTOR_TYPE_SAMPLED_IMAGE,        MAX_IBL_MAPS, vkr::SHADER_STAGE_ALL},
		vkr::DescriptorBinding{IBL_ENVIRONMENT_MAP_REGISTER,     vkr::DESCRIPTOR_TYPE_SAMPLED_IMAGE,        MAX_IBL_MAPS, vkr::SHADER_STAGE_ALL},
		vkr::DescriptorBinding{MATERIAL_SAMPLERS_REGISTER,       vkr::DESCRIPTOR_TYPE_SAMPLER,              MAX_MATERIAL_SAMPLERS, vkr::SHADER_STAGE_ALL},
		vkr::DescriptorBinding{MATERIAL_TEXTURES_REGISTER,       vkr::DESCRIPTOR_TYPE_SAMPLED_IMAGE,        MAX_MATERIAL_TEXTURES, vkr::SHADER_STAGE_ALL},
		};
		// clang-format on

		// Create descriptor pool
		{
			vkr::DescriptorPoolCreateInfo createInfo = {};
			//
			for (const auto& binding : bindings) {
				// clang-format off
				switch (binding.type) {
				case vkr::DESCRIPTOR_TYPE_UNIFORM_BUFFER: createInfo.uniformBuffer += binding.arrayCount; break;
				case vkr::DESCRIPTOR_TYPE_RO_STRUCTURED_BUFFER: createInfo.structuredBuffer += binding.arrayCount; break;
				case vkr::DESCRIPTOR_TYPE_SAMPLER: createInfo.sampler += binding.arrayCount; break;
				case vkr::DESCRIPTOR_TYPE_SAMPLED_IMAGE: createInfo.sampledImage += binding.arrayCount; break;
				default:
					Warning("Found a descriptor binding with unsupported "
						"type " + std::to_string(binding.type) + "; ignoring");
					break;
				}
				// clang-format on
			}

			auto ppxres = pDevice->CreateDescriptorPool(createInfo, &mDescriptorPool);
			if (Failed(ppxres)) {
				return ppxres;
			}
		}

		// Create descriptor set layout
		{
			vkr::DescriptorSetLayoutCreateInfo createInfo = {};
			createInfo.bindings = bindings;

			auto ppxres = pDevice->CreateDescriptorSetLayout(createInfo, &mDescriptorSetLayout);
			if (Failed(ppxres)) {
				return ppxres;
			}
		}

		// Allocate descriptor set
		{
			auto ppxres = pDevice->AllocateDescriptorSet(mDescriptorPool.Get(), mDescriptorSetLayout.Get(), &mDescriptorSet);
			if (Failed(ppxres)) {
				return ppxres;
			}
		}

		return SUCCESS;
	}

	Result MaterialPipelineArgs::InitializeBuffers(vkr::RenderDevice* pDevice)
	{
		if (IsNull(pDevice)) {
			return ERROR_UNEXPECTED_NULL_ARGUMENT;
		}

		static_assert((sizeof(scene::FrameParams) == FRAME_PARAMS_STRUCT_SIZE), "Invalid size for FrameParams");
		static_assert((sizeof(scene::CameraParams) == CAMERA_PARAMS_STRUCT_SIZE), "Invalid size for CameraParams");
		static_assert((sizeof(scene::InstanceParams) == INSTANCE_PARAMS_STRUCT_SIZE), "Invalid size for InstanceParams");
		static_assert((sizeof(scene::MaterialParams) == MATERIAL_PARAMS_STRUCT_SIZE), "Invalid size for MaterialParams");

		// ConstantBuffers
		{
			mFrameParamsPaddedSize = RoundUp<uint32_t>(FRAME_PARAMS_STRUCT_SIZE, 256);
			mCameraParamsPaddedSize = RoundUp<uint32_t>(CAMERA_PARAMS_STRUCT_SIZE, 256);

			mFrameParamsOffset = 0;
			mCameraParamsOffset = mFrameParamsPaddedSize;

			mTotalConstantParamsPaddedSize = mFrameParamsPaddedSize + mCameraParamsPaddedSize;

			vkr::BufferCreateInfo createInfo = {};
			createInfo.size = mTotalConstantParamsPaddedSize;
			createInfo.usageFlags.bits.uniformBuffer = true;

			// CPU buffer
			createInfo.memoryUsage = vkr::MemoryUsage::CPUToGPU;
			createInfo.usageFlags.bits.transferSrc = true;
			createInfo.usageFlags.bits.transferDst = false;
			//
			auto ppxres = pDevice->CreateBuffer(createInfo, &mCpuConstantParamsBuffer);
			if (Failed(ppxres)) {
				return ppxres;
			}

			// GPU buffer
			createInfo.memoryUsage = vkr::MemoryUsage::GPUOnly;
			createInfo.usageFlags.bits.transferSrc = false;
			createInfo.usageFlags.bits.transferDst = true;
			//
			ppxres = pDevice->CreateBuffer(createInfo, &mGpuConstantParamsBuffer);
			if (Failed(ppxres)) {
				return ppxres;
			}
		}

		// Instance params StructuredBuffers
		{
			mTotalInstanceParamsPaddedSize = RoundUp<uint32_t>(MAX_DRAWABLE_INSTANCES * INSTANCE_PARAMS_STRUCT_SIZE, 16);

			vkr::BufferCreateInfo createInfo = {};
			createInfo.structuredElementStride = INSTANCE_PARAMS_STRUCT_SIZE;
			createInfo.size = mTotalInstanceParamsPaddedSize;
			createInfo.usageFlags.bits.roStructuredBuffer = true;

			// CPU buffer
			createInfo.memoryUsage = vkr::MemoryUsage::CPUToGPU;
			createInfo.usageFlags.bits.transferSrc = true;
			createInfo.usageFlags.bits.transferDst = false;
			//
			auto ppxres = pDevice->CreateBuffer(createInfo, &mCpuInstanceParamsBuffer);
			if (Failed(ppxres)) {
				return ppxres;
			}

			// GPU buffer
			createInfo.memoryUsage = vkr::MemoryUsage::GPUOnly;
			createInfo.usageFlags.bits.transferSrc = false;
			createInfo.usageFlags.bits.transferDst = true;
			//
			ppxres = pDevice->CreateBuffer(createInfo, &mGpuInstanceParamsBuffer);
			if (Failed(ppxres)) {
				return ppxres;
			}
		}

		// Material params StructuredBuffers
		{
			mTotalMaterialParamsPaddedSize = RoundUp<uint32_t>(MAX_UNIQUE_MATERIALS * MATERIAL_PARAMS_STRUCT_SIZE, 16);

			vkr::BufferCreateInfo createInfo = {};
			createInfo.structuredElementStride = MATERIAL_PARAMS_STRUCT_SIZE;
			createInfo.size = mTotalMaterialParamsPaddedSize;
			createInfo.usageFlags.bits.roStructuredBuffer = true;

			// CPU buffer
			createInfo.memoryUsage = vkr::MemoryUsage::CPUToGPU;
			createInfo.usageFlags.bits.transferSrc = true;
			createInfo.usageFlags.bits.transferDst = false;
			//
			auto ppxres = pDevice->CreateBuffer(createInfo, &mCpuMateriaParamsBuffer);
			if (Failed(ppxres)) {
				return ppxres;
			}

			// GPU buffer
			createInfo.memoryUsage = vkr::MemoryUsage::GPUOnly;
			createInfo.usageFlags.bits.transferSrc = false;
			createInfo.usageFlags.bits.transferDst = true;
			//
			ppxres = pDevice->CreateBuffer(createInfo, &mGpuMateriaParamsBuffer);
			if (Failed(ppxres)) {
				return ppxres;
			}
		}

		return SUCCESS;
	}

	Result MaterialPipelineArgs::SetDescriptors(vkr::RenderDevice* pDevice)
	{
		if (IsNull(pDevice)) {
			return ERROR_UNEXPECTED_NULL_ARGUMENT;
		}

		//
		// FRAME_PARAMS_REGISTER
		// CAMERA_PARAMS_REGISTER
		// INSTANCE_PARAMS_REGISTER
		// MATERIAL_PARAMS_REGISTER
		//
		{
			std::vector<vkr::WriteDescriptor> writes;

			// Frame
			vkr::WriteDescriptor write = {};
			write.binding = FRAME_PARAMS_REGISTER;
			write.arrayIndex = 0;
			write.type = vkr::DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			write.bufferOffset = mFrameParamsOffset;
			write.bufferRange = mFrameParamsPaddedSize;
			write.pBuffer = mGpuConstantParamsBuffer;
			writes.push_back(write);

			// Camera
			write = {};
			write.binding = CAMERA_PARAMS_REGISTER;
			write.arrayIndex = 0;
			write.type = vkr::DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			write.bufferOffset = mCameraParamsOffset;
			write.bufferRange = mCameraParamsPaddedSize;
			write.pBuffer = mGpuConstantParamsBuffer;
			writes.push_back(write);

			// Instances
			write = {};
			write.binding = INSTANCE_PARAMS_REGISTER;
			write.arrayIndex = 0;
			write.type = vkr::DESCRIPTOR_TYPE_RO_STRUCTURED_BUFFER;
			write.bufferOffset = 0;
			write.bufferRange = mTotalInstanceParamsPaddedSize;
			write.structuredElementCount = MAX_DRAWABLE_INSTANCES;
			write.pBuffer = mGpuInstanceParamsBuffer;
			writes.push_back(write);

			// Materials
			write = {};
			write.binding = MATERIAL_PARAMS_REGISTER;
			write.arrayIndex = 0;
			write.type = vkr::DESCRIPTOR_TYPE_RO_STRUCTURED_BUFFER;
			write.bufferOffset = 0;
			write.bufferRange = mTotalMaterialParamsPaddedSize;
			write.structuredElementCount = MAX_UNIQUE_MATERIALS;
			write.pBuffer = mGpuMateriaParamsBuffer;
			writes.push_back(write);

			auto ppxres = mDescriptorSet->UpdateDescriptors(CountU32(writes), DataPtr(writes));
			if (Failed(ppxres)) {
				return ppxres;
			}
		}

		//
		// BRDF_LUT_SAMPLER_REGISTER
		// BRDF_LUT_TEXTURE_REGISTER
		//
		{
			std::vector<vkr::WriteDescriptor> writes;

			// BRDFLUTSampler
			vkr::WriteDescriptor write = {};
			write.binding = BRDF_LUT_SAMPLER_REGISTER;
			write.arrayIndex = 0;
			write.type = vkr::DESCRIPTOR_TYPE_SAMPLER;
			write.pSampler = mDefaultBRDFLUTSampler;
			writes.push_back(write);

			// BRDFLUTTexture
			write = {};
			write.binding = BRDF_LUT_TEXTURE_REGISTER;
			write.arrayIndex = 0;
			write.type = vkr::DESCRIPTOR_TYPE_SAMPLED_IMAGE;
			write.pImageView = mDefaultBRDFLUTTexture->GetSampledImageView();
			writes.push_back(write);

			auto ppxres = mDescriptorSet->UpdateDescriptors(CountU32(writes), DataPtr(writes));
			if (Failed(ppxres)) {
				return ppxres;
			}
		}

		//
		// IBL_IRRADIANCE_SAMPLER_REGISTER
		// IBL_ENVIRONMENT_SAMPLER_REGISTER
		// IBL_IRRADIANCE_MAP_REGISTER
		// IBL_ENVIRONMENT_MAP_REGISTER
		//
		{
			std::vector<vkr::WriteDescriptor> writes;

			// IBLIrradianceSampler
			vkr::WriteDescriptor write = {};
			write.binding = IBL_IRRADIANCE_SAMPLER_REGISTER;
			write.arrayIndex = 0;
			write.type = vkr::DESCRIPTOR_TYPE_SAMPLER;
			write.pSampler = mDefaultIBLIrradianceSampler;
			writes.push_back(write);

			// IBLEnvironmentSampler
			write = {};
			write.binding = IBL_ENVIRONMENT_SAMPLER_REGISTER;
			write.arrayIndex = 0;
			write.type = vkr::DESCRIPTOR_TYPE_SAMPLER;
			write.pSampler = mDefaultIBLEnvironmentSampler;
			writes.push_back(write);

			for (uint32_t i = 0; i < MAX_IBL_MAPS; ++i) {
				// IBLIrradianceMaps
				write = {};
				write.binding = IBL_IRRADIANCE_MAP_REGISTER;
				write.arrayIndex = i;
				write.type = vkr::DESCRIPTOR_TYPE_SAMPLED_IMAGE;
				write.pImageView = mDefaultIBLTexture->GetSampledImageView();
				writes.push_back(write);

				// IBLEnvironmentMaps
				write = {};
				write.binding = IBL_ENVIRONMENT_MAP_REGISTER;
				write.arrayIndex = i;
				write.type = vkr::DESCRIPTOR_TYPE_SAMPLED_IMAGE;
				write.pImageView = mDefaultIBLTexture->GetSampledImageView();
				writes.push_back(write);
			}

			auto ppxres = mDescriptorSet->UpdateDescriptors(CountU32(writes), DataPtr(writes));
			if (Failed(ppxres)) {
				return ppxres;
			}
		}

		//
		// MATERIAL_SAMPLERS_REGISTER
		// MATERIAL_TEXTURES_REGISTER
		//
		{
			std::vector<vkr::WriteDescriptor> writes;

			// MaterialSamplers
			for (uint32_t i = 0; i < MAX_MATERIAL_SAMPLERS; ++i) {
				vkr::WriteDescriptor write = {};
				write.binding = MATERIAL_SAMPLERS_REGISTER;
				write.arrayIndex = i;
				write.type = vkr::DESCRIPTOR_TYPE_SAMPLER;
				write.pSampler = mDefaultSampler;
				writes.push_back(write);
			}

			// MaterialTextures
			for (uint32_t i = 0; i < MAX_MATERIAL_TEXTURES; ++i) {
				vkr::WriteDescriptor write = {};
				write.binding = MATERIAL_TEXTURES_REGISTER;
				write.arrayIndex = i;
				write.type = vkr::DESCRIPTOR_TYPE_SAMPLED_IMAGE;
				write.pImageView = mDefaultTexture->GetSampledImageView();
				writes.push_back(write);
			}

			auto ppxres = mDescriptorSet->UpdateDescriptors(CountU32(writes), DataPtr(writes));
			if (Failed(ppxres)) {
				return ppxres;
			}
		}

		return SUCCESS;
	}

	Result MaterialPipelineArgs::InitializeResource(vkr::RenderDevice* pDevice)
	{
		if (IsNull(pDevice)) {
			return ERROR_UNEXPECTED_NULL_ARGUMENT;
		}

		auto ppxres = this->InitializeDefaultObjects(pDevice);
		if (Failed(ppxres)) {
			return ppxres;
		}

		ppxres = this->InitializeDescriptorSet(pDevice);
		if (Failed(ppxres)) {
			return ppxres;
		}

		ppxres = this->InitializeBuffers(pDevice);
		if (Failed(ppxres)) {
			return ppxres;
		}

		ppxres = this->SetDescriptors(pDevice);
		if (Failed(ppxres)) {
			return ppxres;
		}

		// Get constant params mapped address
		ppxres = mCpuConstantParamsBuffer->MapMemory(0, reinterpret_cast<void**>(&mConstantParamsMappedAddress));
		if (Failed(ppxres)) {
			return ppxres;
		}

		// Set frame and camera params addresses
		mFrameParamsAddress = reinterpret_cast<scene::FrameParams*>(mConstantParamsMappedAddress + mFrameParamsOffset);
		mCameraParamsAddress = reinterpret_cast<scene::CameraParams*>(mConstantParamsMappedAddress + mCameraParamsOffset);

		// Get instance params mapped address
		ppxres = mCpuInstanceParamsBuffer->MapMemory(0, reinterpret_cast<void**>(&mInstanceParamsMappedAddress));
		if (Failed(ppxres)) {
			return ppxres;
		}

		// Get instance params mapped address
		ppxres = mCpuMateriaParamsBuffer->MapMemory(0, reinterpret_cast<void**>(&mMaterialParamsMappedAddress));
		if (Failed(ppxres)) {
			return ppxres;
		}

		return SUCCESS;
	}

	Result MaterialPipelineArgs::Create(vkr::RenderDevice* pDevice, scene::MaterialPipelineArgs** ppTargetPipelineArgs)
	{
		if (IsNull(pDevice)) {
			return ERROR_UNEXPECTED_NULL_ARGUMENT;
		}

		scene::MaterialPipelineArgs* pPipelineArgs = new scene::MaterialPipelineArgs();
		if (IsNull(pPipelineArgs)) {
			return ERROR_ALLOCATION_FAILED;
		}

		auto ppxres = pPipelineArgs->InitializeResource(pDevice);
		if (Failed(ppxres)) {
			return ppxres;
		}

		*ppTargetPipelineArgs = pPipelineArgs;

		return SUCCESS;
	}

	void MaterialPipelineArgs::SetCameraParams(const Camera* pCamera)
	{
		ASSERT_NULL_ARG(pCamera);

		auto pCameraParams = this->GetCameraParams();
		ASSERT_NULL_ARG(pCameraParams);

		pCameraParams->viewProjectionMatrix = pCamera->GetViewProjectionMatrix();
		pCameraParams->eyePosition = pCamera->GetEyePosition();
		pCameraParams->nearDepth = pCamera->GetNearClip();
		pCameraParams->viewDirection = pCamera->GetViewDirection();
		pCameraParams->farDepth = pCamera->GetFarClip();
	}

	scene::InstanceParams* MaterialPipelineArgs::GetInstanceParams(uint32_t index)
	{
		if (IsNull(mInstanceParamsMappedAddress) || (index >= MAX_DRAWABLE_INSTANCES)) {
			return nullptr;
		}

		uint32_t offset = index * INSTANCE_PARAMS_STRUCT_SIZE;
		char* ptr = mInstanceParamsMappedAddress + offset;
		return reinterpret_cast<scene::InstanceParams*>(ptr);
	}

	scene::MaterialParams* MaterialPipelineArgs::GetMaterialParams(uint32_t index)
	{
		if (IsNull(mMaterialParamsMappedAddress) || (index >= MAX_UNIQUE_MATERIALS)) {
			return nullptr;
		}

		uint32_t offset = index * MATERIAL_PARAMS_STRUCT_SIZE;
		char* ptr = mMaterialParamsMappedAddress + offset;
		return reinterpret_cast<scene::MaterialParams*>(ptr);
	}

	void MaterialPipelineArgs::SetIBLTextures(
		uint32_t                index,
		vkr::SampledImageView* pIrradiance,
		vkr::SampledImageView* pEnvironment)
	{
		ASSERT_NULL_ARG(pIrradiance);
		ASSERT_NULL_ARG(pEnvironment);

		mDescriptorSet->UpdateSampledImage(IBL_IRRADIANCE_MAP_REGISTER, index, pIrradiance);
		mDescriptorSet->UpdateSampledImage(IBL_ENVIRONMENT_MAP_REGISTER, index, pEnvironment);
	}

	void MaterialPipelineArgs::SetMaterialSampler(uint32_t index, const scene::Sampler* pSampler)
	{
		ASSERT_NULL_ARG(pSampler);

		mDescriptorSet->UpdateSampler(MATERIAL_SAMPLERS_REGISTER, index, pSampler->GetSampler());
	}

	void MaterialPipelineArgs::SetMaterialTexture(uint32_t index, const scene::Image* pImage)
	{
		ASSERT_NULL_ARG(pImage);

		mDescriptorSet->UpdateSampledImage(MATERIAL_TEXTURES_REGISTER, index, pImage->GetImageView());
	}

	void MaterialPipelineArgs::CopyBuffers(vkr::CommandBuffer* pCmd)
	{
		ASSERT_NULL_ARG(pCmd);

		// Constant params buffer
		{
			vkr::BufferToBufferCopyInfo copyInfo = {};
			copyInfo.srcBuffer.offset = 0;
			copyInfo.dstBuffer.offset = 0;
			copyInfo.size = mTotalConstantParamsPaddedSize;

			pCmd->CopyBufferToBuffer(
				&copyInfo,
				mCpuConstantParamsBuffer.Get(),
				mGpuConstantParamsBuffer.Get());
		}

		// Instance params buffer
		{
			vkr::BufferToBufferCopyInfo copyInfo = {};
			copyInfo.srcBuffer.offset = 0;
			copyInfo.dstBuffer.offset = 0;
			copyInfo.size = mTotalInstanceParamsPaddedSize;

			pCmd->CopyBufferToBuffer(
				&copyInfo,
				mCpuInstanceParamsBuffer.Get(),
				mGpuInstanceParamsBuffer.Get());
		}

		// Material params buffer
		{
			vkr::BufferToBufferCopyInfo copyInfo = {};
			copyInfo.srcBuffer.offset = 0;
			copyInfo.dstBuffer.offset = 0;
			copyInfo.size = mTotalMaterialParamsPaddedSize;

			pCmd->CopyBufferToBuffer(
				&copyInfo,
				mCpuMateriaParamsBuffer.Get(),
				mGpuMateriaParamsBuffer.Get());
		}
	}

} // namespace scene

#pragma endregion

#pragma region Scene Resource Manager

namespace scene {

	ResourceManager::ResourceManager()
	{
	}

	ResourceManager::~ResourceManager()
	{
	}

	bool ResourceManager::Find(uint64_t objectId, scene::SamplerRef& outObject) const
	{
		return FindObject<vkr::Sampler>(objectId, mSamplers, outObject);
	}

	bool ResourceManager::Find(uint64_t objectId, scene::ImageRef& outObject) const
	{
		return FindObject<vkr::Image>(objectId, mImages, outObject);
	}

	bool ResourceManager::Find(uint64_t objectId, scene::TextureRef& outObject) const
	{
		return FindObject<scene::Texture>(objectId, mTextures, outObject);
	}

	bool ResourceManager::Find(uint64_t objectId, scene::MaterialRef& outObject) const
	{
		return FindObject<scene::Material>(objectId, mMaterials, outObject);
	}

	bool ResourceManager::Find(uint64_t objectId, scene::MeshDataRef& outObject) const
	{
		return FindObject<scene::MeshData>(objectId, mMeshData, outObject);
	}

	bool ResourceManager::Find(uint64_t objectId, scene::MeshRef& outObject) const
	{
		return FindObject<scene::Mesh>(objectId, mMeshes, outObject);
	}

	Result ResourceManager::Cache(uint64_t objectId, const scene::SamplerRef& object)
	{
		return CacheObject<vkr::Image>(objectId, object, mSamplers);
	}

	Result ResourceManager::Cache(uint64_t objectId, const scene::ImageRef& object)
	{
		return CacheObject<vkr::Image>(objectId, object, mImages);
	}

	Result ResourceManager::Cache(uint64_t objectId, const scene::TextureRef& object)
	{
		return CacheObject<scene::Texture>(objectId, object, mTextures);
	}

	Result ResourceManager::Cache(uint64_t objectId, const scene::MaterialRef& object)
	{
		return CacheObject<scene::Material>(objectId, object, mMaterials);
	}

	Result ResourceManager::Cache(uint64_t objectId, const scene::MeshDataRef& object)
	{
		return CacheObject<scene::MeshData>(objectId, object, mMeshData);
	}

	Result ResourceManager::Cache(uint64_t objectId, const scene::MeshRef& object)
	{
		return CacheObject<scene::Mesh>(objectId, object, mMeshes);
	}

	void ResourceManager::DestroyAll()
	{
		mSamplers.clear();
		mImages.clear();
		mTextures.clear();
		mMaterials.clear();
		mMeshData.clear();
		mMeshes.clear();
	}

	const std::unordered_map<uint64_t, scene::SamplerRef>& ResourceManager::GetSamplers() const
	{
		return mSamplers;
	}

	const std::unordered_map<uint64_t, scene::ImageRef>& ResourceManager::GetImages() const
	{
		return mImages;
	}

	const std::unordered_map<uint64_t, scene::TextureRef>& ResourceManager::GetTextures() const
	{
		return mTextures;
	}

	const std::unordered_map<uint64_t, scene::MaterialRef>& ResourceManager::GetMaterials() const
	{
		return mMaterials;
	}

} // namespace scene

#pragma endregion

#pragma region Scene Mesh

namespace scene {

	MeshData::MeshData(
		const scene::VertexAttributeFlags& availableVertexAttributes,
		vkr::Buffer* pGpuBuffer)
		: mAvailableVertexAttributes(availableVertexAttributes),
		mGpuBuffer(pGpuBuffer)
	{
		// Position
		auto positionBinding = vkr::VertexBinding(0, vkr::VERTEX_INPUT_RATE_VERTEX);
		{
			vkr::VertexAttribute attr = {};
			attr.semanticName = "POSITION";
			attr.location = scene::kVertexPositionLocation;
			attr.format = scene::kVertexPositionFormat;
			attr.binding = scene::kVertexPositionBinding;
			attr.offset = APPEND_OFFSET_ALIGNED;
			attr.inputRate = vkr::VERTEX_INPUT_RATE_VERTEX;
			attr.semantic = vkr::VERTEX_SEMANTIC_POSITION;
			positionBinding.AppendAttribute(attr);
		}
		mVertexBindings.push_back(positionBinding);

		// Done if no vertex attributes
		if (availableVertexAttributes.mask == 0) {
			return;
		}

		auto attributeBinding = vkr::VertexBinding(kVertexAttributeBinding, vkr::VERTEX_INPUT_RATE_VERTEX);
		{
			// TexCoord
			if (availableVertexAttributes.bits.texCoords) {
				vkr::VertexAttribute attr = {};
				attr.semanticName = "TEXCOORD";
				attr.location = scene::kVertexAttributeTexCoordLocation;
				attr.format = scene::kVertexAttributeTexCoordFormat;
				attr.binding = scene::kVertexAttributeBinding;
				attr.offset = APPEND_OFFSET_ALIGNED;
				attr.inputRate = vkr::VERTEX_INPUT_RATE_VERTEX;
				attr.semantic = vkr::VERTEX_SEMANTIC_TEXCOORD;
				attributeBinding.AppendAttribute(attr);
			}

			// Normal
			if (availableVertexAttributes.bits.normals) {
				vkr::VertexAttribute attr = {};
				attr.semanticName = "NORMAL";
				attr.location = scene::kVertexAttributeNormalLocation;
				attr.format = scene::kVertexAttributeNormalFormat;
				attr.binding = scene::kVertexAttributeBinding;
				attr.offset = APPEND_OFFSET_ALIGNED;
				attr.inputRate = vkr::VERTEX_INPUT_RATE_VERTEX;
				attr.semantic = vkr::VERTEX_SEMANTIC_NORMAL;
				attributeBinding.AppendAttribute(attr);
			}

			// Tangent
			if (availableVertexAttributes.bits.tangents) {
				vkr::VertexAttribute attr = {};
				attr.semanticName = "TANGENT";
				attr.location = scene::kVertexAttributeTangentLocation;
				attr.format = scene::kVertexAttributeTagentFormat;
				attr.binding = scene::kVertexAttributeBinding;
				attr.offset = APPEND_OFFSET_ALIGNED;
				attr.inputRate = vkr::VERTEX_INPUT_RATE_VERTEX;
				attr.semantic = vkr::VERTEX_SEMANTIC_TANGENT;
				attributeBinding.AppendAttribute(attr);
			}

			// Color
			if (availableVertexAttributes.bits.colors) {
				vkr::VertexAttribute attr = {};
				attr.semanticName = "COLOR";
				attr.location = scene::kVertexAttributeColorLocation;
				attr.format = scene::kVertexAttributeColorFormat;
				attr.binding = scene::kVertexAttributeBinding;
				attr.offset = APPEND_OFFSET_ALIGNED;
				attr.inputRate = vkr::VERTEX_INPUT_RATE_VERTEX;
				attr.semantic = vkr::VERTEX_SEMANTIC_COLOR;
				attributeBinding.AppendAttribute(attr);
			}
		}
		mVertexBindings.push_back(attributeBinding);
	}

	MeshData::~MeshData()
	{
		if (mGpuBuffer) {
			auto pDevice = mGpuBuffer->GetDevice();
			pDevice->DestroyBuffer(mGpuBuffer);
		}
	}

	PrimitiveBatch::PrimitiveBatch(
		const scene::MaterialRef& material,
		const vkr::IndexBufferView& indexBufferView,
		const vkr::VertexBufferView& positionBufferView,
		const vkr::VertexBufferView& attributeBufferView,
		uint32_t                      indexCount,
		uint32_t                      vertexCount,
		const AABB& boundingBox)
		: mMaterial(material),
		mIndexBufferView(indexBufferView),
		mPositionBufferView(positionBufferView),
		mAttributeBufferView(attributeBufferView),
		mIndexCount(indexCount),
		mVertexCount(vertexCount),
		mBoundingBox(boundingBox)
	{
	}

	Mesh::Mesh(
		const scene::MeshDataRef& meshData,
		std::vector<scene::PrimitiveBatch>&& batches)
		: mMeshData(meshData),
		mBatches(std::move(batches))
	{
		UpdateBoundingBox();
	}

	Mesh::Mesh(
		std::unique_ptr<scene::ResourceManager>&& resourceManager,
		const scene::MeshDataRef& meshData,
		std::vector<scene::PrimitiveBatch>&& batches)
		: mResourceManager(std::move(resourceManager)),
		mMeshData(meshData),
		mBatches(std::move(batches))
	{
		UpdateBoundingBox();
	}

	Mesh::~Mesh()
	{
		if (mResourceManager) {
			mResourceManager->DestroyAll();
			mResourceManager.reset();
		}
	}

	scene::VertexAttributeFlags Mesh::GetAvailableVertexAttributes() const
	{
		return mMeshData ? mMeshData->GetAvailableVertexAttributes() : scene::VertexAttributeFlags::None();
	}

	std::vector<vkr::VertexBinding> Mesh::GetAvailableVertexBindings() const
	{
		std::vector<vkr::VertexBinding> bindings;
		if (mMeshData) {
			bindings = mMeshData->GetAvailableVertexBindings();
		}
		return bindings;
	}

	void Mesh::AddBatch(const scene::PrimitiveBatch& batch)
	{
		mBatches.push_back(batch);
	}

	void Mesh::UpdateBoundingBox()
	{
		if (mBatches.empty()) {
			return;
		}

		mBoundingBox.Set(mBatches[0].GetBoundingBox().min);

		for (const auto& batch : mBatches) {
			mBoundingBox.Expand(batch.GetBoundingBox().min);
			mBoundingBox.Expand(batch.GetBoundingBox().max);
		}
	}

	std::vector<const scene::Material*> Mesh::GetMaterials() const
	{
		std::vector<const scene::Material*> materials;
		for (auto& batch : mBatches) {
			if (IsNull(batch.GetMaterial())) {
				// @TODO: need a better way to handle missing materials
				continue;
			}
			materials.push_back(batch.GetMaterial());
		}
		return materials;
	}

} // namespace scene

#pragma endregion

#pragma region Scene

namespace scene {

	Scene::Scene(std::unique_ptr<scene::ResourceManager>&& resourceManager)
		: mResourceManager(std::move(resourceManager))
	{
	}

	uint32_t Scene::GetSamplerCount() const
	{
		return mResourceManager ? mResourceManager->GetSamplerCount() : 0;
	}

	uint32_t Scene::GetImageCount() const
	{
		return mResourceManager ? mResourceManager->GetImageCount() : 0;
	}

	uint32_t Scene::GetTextureCount() const
	{
		return mResourceManager ? mResourceManager->GetTextureCount() : 0;
	}

	uint32_t Scene::GetMaterialCount() const
	{
		return mResourceManager ? mResourceManager->GetMaterialCount() : 0;
	}

	uint32_t Scene::GetMeshDataCount() const
	{
		return mResourceManager ? mResourceManager->GetMeshDataCount() : 0;
	}

	uint32_t Scene::GetMeshCount() const
	{
		return mResourceManager ? mResourceManager->GetMeshCount() : 0;
	}

	scene::Node* Scene::GetNode(uint32_t index) const
	{
		if (index >= CountU32(mNodes)) {
			return nullptr;
		}

		return mNodes[index].get();
	}

	scene::MeshNode* Scene::GetMeshNode(uint32_t index) const
	{
		scene::MeshNode* pNode = nullptr;
		if (!GetElement(index, mMeshNodes, &pNode)) {
			return nullptr;
		}
		return pNode;
	}

	scene::CameraNode* Scene::GetCameraNode(uint32_t index) const
	{
		scene::CameraNode* pNode = nullptr;
		if (!GetElement(index, mCameraNodes, &pNode)) {
			return nullptr;
		}
		return pNode;
	}

	scene::LightNode* Scene::GetLightNode(uint32_t index) const
	{
		scene::LightNode* pNode = nullptr;
		if (!GetElement(index, mLightNodes, &pNode)) {
			return nullptr;
		}
		return pNode;
	}

	scene::Node* Scene::FindNode(const std::string& name) const
	{
		auto it = FindIf(
			mNodes,
			[name](const scene::NodeRef& elem) {
				bool match = (elem->GetName() == name);
				return match; });
		if (it == mNodes.end()) {
			return nullptr;
		}

		scene::Node* pNode = (*it).get();
		return pNode;
	}

	scene::MeshNode* Scene::FindMeshNode(const std::string& name) const
	{
		return FindNodeByName(name, mMeshNodes);
	}

	scene::CameraNode* Scene::FindCameraNode(const std::string& name) const
	{
		return FindNodeByName(name, mCameraNodes);
	}

	scene::LightNode* Scene::FindLightNode(const std::string& name) const
	{
		return FindNodeByName(name, mLightNodes);
	}

	Result Scene::AddNode(scene::NodeRef&& node)
	{
		if (!node) {
			return ERROR_UNEXPECTED_NULL_ARGUMENT;
		}

		scene::MeshNode* pMeshNode = nullptr;
		scene::CameraNode* pCameraNode = nullptr;
		scene::LightNode* pLightNode = nullptr;
		//
		switch (node->GetNodeType()) {
		default: return ERROR_SCENE_UNSUPPORTED_NODE_TYPE;
		case scene::NODE_TYPE_TRANSFORM: break;
		case scene::NODE_TYPE_MESH: pMeshNode = static_cast<scene::MeshNode*>(node.get()); break;
		case scene::NODE_TYPE_CAMERA: pCameraNode = static_cast<scene::CameraNode*>(node.get()); break;
		case scene::NODE_TYPE_LIGHT: pLightNode = static_cast<scene::LightNode*>(node.get()); break;
		}

		auto it = std::find_if(
			mNodes.begin(),
			mNodes.end(),
			[&node](const scene::NodeRef& elem) -> bool {
				bool match = (elem.get() == node.get());
				return match; });
		if (it != mNodes.end()) {
			return ERROR_DUPLICATE_ELEMENT;
		}

		mNodes.push_back(std::move(node));

		if (!IsNull(pMeshNode)) {
			mMeshNodes.push_back(pMeshNode);
		}
		else if (!IsNull(pCameraNode)) {
			mCameraNodes.push_back(pCameraNode);
		}
		else if (!IsNull(pLightNode)) {
			mLightNodes.push_back(pLightNode);
		}

		return SUCCESS;
	}

	scene::ResourceIndexMap<scene::Sampler> Scene::GetSamplersArrayIndexMap() const
	{
		const auto& objects = mResourceManager->GetSamplers();

		std::unordered_map<const scene::Sampler*, uint32_t> indexMap;
		//
		uint32_t index = 0;
		for (const auto& it : objects) {
			indexMap[it.second.get()] = index;
			++index;
		}

		return indexMap;
	}

	scene::ResourceIndexMap<scene::Image> Scene::GetImagesArrayIndexMap() const
	{
		const auto& objects = mResourceManager->GetImages();

		scene::ResourceIndexMap<scene::Image> indexMap;
		//
		uint32_t index = 0;
		for (const auto& it : objects) {
			indexMap[it.second.get()] = index;
			++index;
		}

		return indexMap;
	}

	scene::ResourceIndexMap<scene::Material> Scene::GetMaterialsArrayIndexMap() const
	{
		const auto& objects = mResourceManager->GetMaterials();

		scene::ResourceIndexMap<scene::Material> indexMap;
		//
		uint32_t index = 0;
		for (const auto& it : objects) {
			indexMap[it.second.get()] = index;
			++index;
		}

		return indexMap;
	}

} // namespace scene

#pragma endregion

#pragma region Scene gltf Loader

#if defined(WIN32) && defined(LoadImage)
#undef LoadImage
#endif

#define KHR_MATERIALS_UNLIT_EXTENSION_NAME "KHR_materials_unlit"

namespace scene {

#define GLTF_LOD_CLAMP_NONE 1000.0f

	enum GltfTextureFilter
	{
		GLTF_TEXTURE_FILTER_NEAREST = 9728,
		GLTF_TEXTURE_FILTER_LINEAR = 9729,
	};

	enum GltfTextureWrap
	{
		GLTF_TEXTURE_WRAP_REPEAT = 10497,
		GLTF_TEXTURE_WRAP_CLAMP_TO_EDGE = 33071,
		GLTF_TEXTURE_WRAP_MIRRORED_REPEAT = 33648,
	};

	struct VertexAccessors
	{
		const cgltf_accessor* pPositions;
		const cgltf_accessor* pNormals;
		const cgltf_accessor* pTangents;
		const cgltf_accessor* pColors;
		const cgltf_accessor* pTexCoords;
	};

	static std::string ToStringSafe(const char* cStr)
	{
		return IsNull(cStr) ? "" : std::string(cStr);
	}

	template <typename GltfObjectT>
	static std::string GetName(const GltfObjectT* pGltfObject)
	{
		ASSERT_MSG(!IsNull(pGltfObject), "Cannot get name of a NULL GLTF object, pGltfObject is NULL");
		return IsNull(pGltfObject->name) ? "" : std::string(pGltfObject->name);
	}

	static vkr::Format GetFormat(const cgltf_accessor* pGltfAccessor)
	{
		if (IsNull(pGltfAccessor)) {
			return vkr::FORMAT_UNDEFINED;
		}

		// clang-format off
		switch (pGltfAccessor->type) {
		default: break;

		case cgltf_type_scalar: {
			switch (pGltfAccessor->component_type) {
			default: return vkr::FORMAT_UNDEFINED;
			case cgltf_component_type_r_8: return vkr::FORMAT_R8_SINT;
			case cgltf_component_type_r_8u: return vkr::FORMAT_R8_UINT;
			case cgltf_component_type_r_16: return vkr::FORMAT_R16_SINT;
			case cgltf_component_type_r_16u: return vkr::FORMAT_R16_UINT;
			case cgltf_component_type_r_32u: return vkr::FORMAT_R32_UINT;
			case cgltf_component_type_r_32f: return vkr::FORMAT_R32_FLOAT;
			}
		} break;

		case cgltf_type_vec2: {
			switch (pGltfAccessor->component_type) {
			default: return vkr::FORMAT_UNDEFINED;
			case cgltf_component_type_r_8: return vkr::FORMAT_R8G8_SINT;
			case cgltf_component_type_r_8u: return vkr::FORMAT_R8G8_UINT;
			case cgltf_component_type_r_16: return vkr::FORMAT_R16G16_SINT;
			case cgltf_component_type_r_16u: return vkr::FORMAT_R16G16_UINT;
			case cgltf_component_type_r_32u: return vkr::FORMAT_R32G32_UINT;
			case cgltf_component_type_r_32f: return vkr::FORMAT_R32G32_FLOAT;
			}
		} break;

		case cgltf_type_vec3: {
			switch (pGltfAccessor->component_type) {
			default: return vkr::FORMAT_UNDEFINED;
			case cgltf_component_type_r_8: return vkr::FORMAT_R8G8B8_SINT;
			case cgltf_component_type_r_8u: return vkr::FORMAT_R8G8B8_UINT;
			case cgltf_component_type_r_16: return vkr::FORMAT_R16G16B16_SINT;
			case cgltf_component_type_r_16u: return vkr::FORMAT_R16G16B16_UINT;
			case cgltf_component_type_r_32u: return vkr::FORMAT_R32G32B32_UINT;
			case cgltf_component_type_r_32f: return vkr::FORMAT_R32G32B32_FLOAT;
			}
		} break;

		case cgltf_type_vec4: {
			switch (pGltfAccessor->component_type) {
			default: return vkr::FORMAT_UNDEFINED;
			case cgltf_component_type_r_8: return vkr::FORMAT_R8G8B8A8_SINT;
			case cgltf_component_type_r_8u: return vkr::FORMAT_R8G8B8A8_UINT;
			case cgltf_component_type_r_16: return vkr::FORMAT_R16G16B16A16_SINT;
			case cgltf_component_type_r_16u: return vkr::FORMAT_R16G16B16A16_UINT;
			case cgltf_component_type_r_32u: return vkr::FORMAT_R32G32B32A32_UINT;
			case cgltf_component_type_r_32f: return vkr::FORMAT_R32G32B32A32_FLOAT;
			}
		} break;
		}
		// clang-format on

		return vkr::FORMAT_UNDEFINED;
	}

	static scene::NodeType GetNodeType(const cgltf_node* pGltfNode)
	{
		if (IsNull(pGltfNode)) {
			return scene::NODE_TYPE_UNSUPPORTED;
		}

		if (!IsNull(pGltfNode->mesh)) {
			return scene::NODE_TYPE_MESH;
		}
		else if (!IsNull(pGltfNode->camera)) {
			return scene::NODE_TYPE_CAMERA;
		}
		else if (!IsNull(pGltfNode->light)) {
			return scene::NODE_TYPE_LIGHT;
		}
		else if (!IsNull(pGltfNode->skin) || !IsNull(pGltfNode->weights)) {
			return scene::NODE_TYPE_UNSUPPORTED;
		}

		return scene::NODE_TYPE_TRANSFORM;
	}

	static vkr::SamplerAddressMode ToSamplerAddressMode(cgltf_int mode)
	{
		switch (mode) {
		default: break;
		case GLTF_TEXTURE_WRAP_CLAMP_TO_EDGE: return vkr::SamplerAddressMode::ClampToEdge;
		case GLTF_TEXTURE_WRAP_MIRRORED_REPEAT: return vkr::SamplerAddressMode::MirrorRepeat;
		}
		return vkr::SamplerAddressMode::Repeat;
	}

	template <typename GltfObjectT>
	static bool HasExtension(
		const std::string& extensionName,
		const GltfObjectT* pGltfObject)
	{
		if (extensionName.empty() || IsNull(pGltfObject) || IsNull(pGltfObject->extensions)) {
			return false;
		}

		for (cgltf_size i = 0; i < pGltfObject->extensions_count; ++i) {
			const std::string name = ToStringSafe(pGltfObject->extensions[i].name);
			if (extensionName == name) {
				return true;
			}
		}

		return false;
	}

	// Returns the widest index type used by a mesh
	vkr::IndexType GetIndexType(
		const cgltf_mesh* pGltfMesh)
	{
		if (IsNull(pGltfMesh)) {
			return vkr::INDEX_TYPE_UNDEFINED;
		}

		uint32_t finalBitCount = 0;
		for (cgltf_size primIdx = 0; primIdx < pGltfMesh->primitives_count; ++primIdx) {
			const cgltf_primitive* pGltfPrimitive = &pGltfMesh->primitives[primIdx];
			// Convert to vkr::Format
			auto format = GetFormat(pGltfPrimitive->indices);

			uint32_t bitCount = 0;
			switch (format) {
				// Bail if we don't recognize the format
			default: return vkr::INDEX_TYPE_UNDEFINED;
			case vkr::FORMAT_R16_UINT: bitCount = 16; break;
			case vkr::FORMAT_R32_UINT: bitCount = 32; break;
			}

			finalBitCount = std::max(bitCount, finalBitCount);
		}

		if (finalBitCount == 32) {
			return vkr::INDEX_TYPE_UINT32;
		}
		else if (finalBitCount == 16) {
			return vkr::INDEX_TYPE_UINT16;
		}

		return vkr::INDEX_TYPE_UNDEFINED;
	}

	// Calcualte a unique hash based a meshes primitive accessors
	static uint64_t GetMeshAccessorsHash(
		const cgltf_data* pGltfData,
		const cgltf_mesh* pGltfMesh)
	{
		const auto kInvalidAccessorIndex = InvalidValue<cgltf_size>();

		std::set<cgltf_size> uniqueAccessorIndices;
		for (cgltf_size primIdx = 0; primIdx < pGltfMesh->primitives_count; ++primIdx) {
			const cgltf_primitive* pGltfPrimitive = &pGltfMesh->primitives[primIdx];

			// Indices
			if (!IsNull(pGltfPrimitive->indices)) {
				cgltf_size accessorIndex = cgltf_accessor_index(pGltfData, pGltfPrimitive->indices);
				uniqueAccessorIndices.insert(accessorIndex);
			}

			for (cgltf_size attrIdx = 0; attrIdx < pGltfPrimitive->attributes_count; ++attrIdx) {
				const cgltf_attribute* pGltfAttr = &pGltfPrimitive->attributes[attrIdx];
				cgltf_size             accessorIndex = cgltf_accessor_index(pGltfData, pGltfAttr->data);

				switch (pGltfAttr->type) {
				default: break;
				case cgltf_attribute_type_position:
				case cgltf_attribute_type_normal:
				case cgltf_attribute_type_tangent:
				case cgltf_attribute_type_color:
				case cgltf_attribute_type_texcoord: {
					uniqueAccessorIndices.insert(accessorIndex);
				} break;
				}
			}
		}

		// Copy to vector
		std::vector<cgltf_size> orderedAccessorIndices(uniqueAccessorIndices.begin(), uniqueAccessorIndices.end());

		// Sort accessor to indices
		std::sort(orderedAccessorIndices.begin(), orderedAccessorIndices.end());

		uint64_t hash = 0;
		if (!orderedAccessorIndices.empty()) {
			const XXH64_hash_t kSeed = 0x5874bc9de50a7627;

			hash = XXH64(
				DataPtr(orderedAccessorIndices),
				static_cast<size_t>(SizeInBytesU32(orderedAccessorIndices)),
				kSeed);
		}

		return hash;
	}

	static VertexAccessors GetVertexAccessors(const cgltf_primitive* pGltfPrimitive)
	{
		VertexAccessors accessors{ nullptr, nullptr, nullptr, nullptr, nullptr };
		if (IsNull(pGltfPrimitive)) {
			return accessors;
		}

		for (cgltf_size i = 0; i < pGltfPrimitive->attributes_count; ++i) {
			const cgltf_attribute* pGltfAttr = &pGltfPrimitive->attributes[i];
			const cgltf_accessor* pGltfAccessor = pGltfAttr->data;

			// clang-format off
			switch (pGltfAttr->type) {
			default: break;
			case cgltf_attribute_type_position: accessors.pPositions = pGltfAccessor; break;
			case cgltf_attribute_type_normal: accessors.pNormals = pGltfAccessor; break;
			case cgltf_attribute_type_tangent: accessors.pTangents = pGltfAccessor; break;
			case cgltf_attribute_type_color: accessors.pColors = pGltfAccessor; break;
			case cgltf_attribute_type_texcoord: accessors.pTexCoords = pGltfAccessor; break;
			};
			// clang-format on
		}
		return accessors;
	}

	// Get a buffer view's start address
	static const void* GetStartAddress(
		const cgltf_buffer_view* pGltfBufferView)
	{
		//
		// NOTE: Don't assert in this function since any of the fields can be NULL for different reasons.
		//

		if (IsNull(pGltfBufferView) || IsNull(pGltfBufferView->buffer) || IsNull(pGltfBufferView->buffer->data)) {
			return nullptr;
		}

		const size_t bufferViewOffset = static_cast<size_t>(pGltfBufferView->offset);
		const char* pBufferAddress = static_cast<const char*>(pGltfBufferView->buffer->data);
		const char* pDataStart = pBufferAddress + bufferViewOffset;

		return static_cast<const void*>(pDataStart);
	}

	// Get an accessor's starting address
	static const void* GetStartAddress(
		const cgltf_data* pGltfData,
		const cgltf_accessor* pGltfAccessor)
	{
		//
		// NOTE: Don't assert in this function since any of the fields can be NULL for different reasons.
		//

		if (IsNull(pGltfData) || IsNull(pGltfAccessor)) {
			return nullptr;
		}

		// Get buffer view's start address
		const char* pBufferViewDataStart = static_cast<const char*>(GetStartAddress(pGltfAccessor->buffer_view));
		if (IsNull(pBufferViewDataStart)) {
			return nullptr;
		}

		// Calculate accesor's start address
		const size_t accessorOffset = static_cast<size_t>(pGltfAccessor->offset);
		const char* pAccessorDataStart = pBufferViewDataStart + accessorOffset;

		return static_cast<const void*>(pAccessorDataStart);
	}

	GltfMaterialSelector::GltfMaterialSelector()
	{
	}

	GltfMaterialSelector::~GltfMaterialSelector()
	{
	}

	std::string GltfMaterialSelector::DetermineMaterial(const cgltf_material* pGltfMaterial) const
	{
		std::string ident = MATERIAL_IDENT_ERROR;

		// Determine material type
		if (pGltfMaterial->unlit) {
			ident = MATERIAL_IDENT_UNLIT;
		}
		else if (pGltfMaterial->has_pbr_metallic_roughness) {
			ident = MATERIAL_IDENT_STANDARD;
		}

		return ident;
	}

	GltfLoader::GltfLoader(
		const std::filesystem::path& filePath,
		const std::filesystem::path& textureDirPath,
		cgltf_data* pGltfData,
		bool                         ownsGtlfData,
		scene::GltfMaterialSelector* pMaterialSelector,
		bool                         ownsMaterialSelector)
		: mGltfFilePath(filePath),
		mGltfTextureDir(textureDirPath),
		mGltfData(pGltfData),
		mOwnsGltfData(ownsGtlfData),
		mMaterialSelector(pMaterialSelector),
		mOwnsMaterialSelector(ownsMaterialSelector)
	{
	}

	GltfLoader::~GltfLoader()
	{
		if (HasGltfData() && mOwnsGltfData) {
			cgltf_free(mGltfData);
			mGltfData = nullptr;

			Print("Closed GLTF file: " + mGltfFilePath.string());
		}

		if (!IsNull(mMaterialSelector) && mOwnsMaterialSelector) {
			delete mMaterialSelector;
			mMaterialSelector = nullptr;
		}
	}

	Result GltfLoader::Create(
		const std::filesystem::path& filePath,
		const std::filesystem::path& textureDirPath,
		scene::GltfMaterialSelector* pMaterialSelector,
		GltfLoader** ppLoader)
	{
		if (IsNull(ppLoader)) {
			return ERROR_UNEXPECTED_NULL_ARGUMENT;
		}

		if (!(std::filesystem::exists(filePath) && std::filesystem::exists(textureDirPath))) {
			return ERROR_PATH_DOES_NOT_EXIST;
		}

		// Parse gltf data
		cgltf_options cgltfOptions = {};
		cgltf_data* pGltfData = nullptr;

		cgltf_result cgres = cgltf_parse_file(
			&cgltfOptions,
			filePath.string().c_str(),
			&pGltfData);
		if (cgres != cgltf_result_success) {
			return ERROR_SCENE_SOURCE_FILE_LOAD_FAILED;
		}

		// Load GLTF buffers
		{
			cgltf_result res = cgltf_load_buffers(
				&cgltfOptions,
				pGltfData,
				filePath.string().c_str());
			if (res != cgltf_result_success) {
				cgltf_free(pGltfData);
				ASSERT_MSG(false, "GLTF: cgltf_load_buffers failed (res=" + std::to_string(res) + ")");
				return ERROR_SCENE_SOURCE_FILE_LOAD_FAILED;
			}
		}

		// Loading from file means we own the GLTF data
		const bool ownsGltfData = true;

		// Create material selector
		const bool ownsMaterialSelector = IsNull(pMaterialSelector) ? true : false;
		if (IsNull(pMaterialSelector)) {
			pMaterialSelector = new GltfMaterialSelector();
			if (IsNull(pMaterialSelector)) {
				cgltf_free(pGltfData);
				return ERROR_ALLOCATION_FAILED;
			}
		}

		// Create loader object
		auto pLoader = new GltfLoader(
			filePath,
			textureDirPath,
			pGltfData,
			ownsGltfData,
			pMaterialSelector,
			ownsMaterialSelector);
		if (IsNull(pLoader)) {
			cgltf_free(pGltfData);
			return ERROR_ALLOCATION_FAILED;
		}

		*ppLoader = pLoader;

		Print("Successfully opened GLTF file: " + filePath.string());

		return SUCCESS;
	}

	Result GltfLoader::Create(
		const std::filesystem::path& path,
		scene::GltfMaterialSelector* pMaterialSelector,
		GltfLoader** ppLoader)
	{
		return Create(
			path,
			path.parent_path(),
			pMaterialSelector,
			ppLoader);
	}

	void GltfLoader::CalculateMeshMaterialVertexAttributeMasks(
		const scene::MaterialFactory* pMaterialFactory,
		scene::GltfLoader::MeshMaterialVertexAttributeMasks* pOutMasks) const
	{
		if (IsNull(mGltfData) || IsNull(pMaterialFactory) || IsNull(pOutMasks)) {
			return;
		}

		auto& outMasks = *pOutMasks;

		for (cgltf_size meshIdx = 0; meshIdx < mGltfData->meshes_count; ++meshIdx) {
			const cgltf_mesh* pGltfMesh = &mGltfData->meshes[meshIdx];

			// Initial value
			outMasks[pGltfMesh] = scene::VertexAttributeFlags::None();

			for (cgltf_size primIdx = 0; primIdx < pGltfMesh->primitives_count; ++primIdx) {
				const cgltf_material* pGltfMaterial = pGltfMesh->primitives[primIdx].material;

				// Skip if no material
				if (IsNull(pGltfMaterial)) {
					continue;
				}

				// Get material ident
				auto materialIdent = mMaterialSelector->DetermineMaterial(pGltfMaterial);

				// Get required vertex attributes
				auto requiredVertexAttributes = pMaterialFactory->GetRequiredVertexAttributes(materialIdent);

				// OR the masks
				outMasks[pGltfMesh] |= requiredVertexAttributes;
			}
		}
	}

	uint64_t GltfLoader::CalculateImageObjectId(const GltfLoader::InternalLoadParams& loadParams, uint32_t objectIndex)
	{
		uint64_t objectId = objectIndex + loadParams.baseObjectIds.image;
		return objectId;
	}

	uint64_t GltfLoader::CalculateSamplerObjectId(const GltfLoader::InternalLoadParams& loadParams, uint32_t objectIndex)
	{
		uint64_t objectId = objectIndex + loadParams.baseObjectIds.sampler;
		return objectId;
	}

	uint64_t GltfLoader::CalculateTextureObjectId(const GltfLoader::InternalLoadParams& loadParams, uint32_t objectIndex)
	{
		uint64_t objectId = objectIndex + loadParams.baseObjectIds.texture;
		return objectId;
	}

	uint64_t GltfLoader::CalculateMaterialObjectId(const GltfLoader::InternalLoadParams& loadParams, uint32_t objectIndex)
	{
		uint64_t objectId = objectIndex + loadParams.baseObjectIds.material;
		return objectId;
	}

	uint64_t GltfLoader::CalculateMeshObjectId(const GltfLoader::InternalLoadParams& loadParams, uint32_t objectIndex)
	{
		uint64_t objectId = objectIndex + loadParams.baseObjectIds.mesh;
		return objectId;
	}

	Result GltfLoader::LoadSamplerInternal(
		const GltfLoader::InternalLoadParams& loadParams,
		const cgltf_sampler* pGltfSampler,
		scene::Sampler** ppTargetSampler)
	{
		if (IsNull(loadParams.pDevice) || IsNull(pGltfSampler) || IsNull(ppTargetSampler)) {
			return ERROR_UNEXPECTED_NULL_ARGUMENT;
		}

		// Get GLTF object name
		const std::string gltfObjectName = GetName(pGltfSampler);

		// Get GLTF object index to use as object id
		const uint32_t gltfObjectIndex = static_cast<uint32_t>(cgltf_sampler_index(mGltfData, pGltfSampler));
		Print("Loading GLTF sampler[" + std::to_string(gltfObjectIndex) + "]: " + gltfObjectName);

		// Load sampler
		vkr::Sampler* pGrfxSampler = nullptr;
		//
		{
			vkr::SamplerCreateInfo createInfo = {};
			createInfo.magFilter = (pGltfSampler->mag_filter == GLTF_TEXTURE_FILTER_LINEAR) ? vkr::Filter::Linear : vkr::Filter::Nearest;
			createInfo.minFilter = (pGltfSampler->mag_filter == GLTF_TEXTURE_FILTER_LINEAR) ? vkr::Filter::Linear : vkr::Filter::Nearest;
			createInfo.mipmapMode = vkr::SamplerMipmapMode::Linear; // @TODO: add option to control this
			createInfo.addressModeU = ToSamplerAddressMode(pGltfSampler->wrap_s);
			createInfo.addressModeV = ToSamplerAddressMode(pGltfSampler->wrap_t);
			createInfo.addressModeW = vkr::SamplerAddressMode::Repeat;
			createInfo.mipLodBias = 0.0f;
			createInfo.anisotropyEnable = false;
			createInfo.maxAnisotropy = 0.0f;
			createInfo.compareEnable = false;
			createInfo.compareOp = vkr::CompareOp::Never;
			createInfo.minLod = 0.0f;
			createInfo.maxLod = GLTF_LOD_CLAMP_NONE;
			createInfo.borderColor = vkr::BorderColor::FloatTransparentBlack;

			auto ppxres = loadParams.pDevice->CreateSampler(createInfo, &pGrfxSampler);
			if (Failed(ppxres)) {
				return ppxres;
			}
		}

		// Create target object
		auto pSampler = new scene::Sampler(pGrfxSampler);
		if (IsNull(pSampler)) {
			loadParams.pDevice->DestroySampler(pGrfxSampler);
			return ERROR_ALLOCATION_FAILED;
		}

		// Set name
		pSampler->SetName(gltfObjectName);

		// Assign output
		*ppTargetSampler = pSampler;

		return SUCCESS;
	}

	Result GltfLoader::FetchSamplerInternal(
		const GltfLoader::InternalLoadParams& loadParams,
		const cgltf_sampler* pGltfSampler,
		scene::SamplerRef& outSampler)
	{
		if (IsNull(loadParams.pDevice) || IsNull(loadParams.pResourceManager) || IsNull(pGltfSampler)) {
			return ERROR_UNEXPECTED_NULL_ARGUMENT;
		}

		// Get GLTF object name
		const std::string gltfObjectName = GetName(pGltfSampler);

		// Get GLTF object index to use as object id
		const uint32_t gltfObjectIndex = static_cast<uint32_t>(cgltf_sampler_index(mGltfData, pGltfSampler));

		// Cached load if object was previously cached
		const uint64_t objectId = CalculateSamplerObjectId(loadParams, gltfObjectIndex);
		if (loadParams.pResourceManager->Find(objectId, outSampler)) {
			Print("Fetched cached sampler[" + std::to_string(gltfObjectIndex) + "]: " + gltfObjectName + " (objectId=" + std::to_string(objectId) + ")");
			return SUCCESS;
		}

		// Cached failed, so load object
		scene::Sampler* pSampler = nullptr;
		//
		auto ppxres = LoadSamplerInternal(loadParams, pGltfSampler, &pSampler);
		if (Failed(ppxres)) {
			return ppxres;
		}
		ASSERT_NULL_ARG(pSampler);

		// Create object ref
		outSampler = scene::MakeRef(pSampler);
		if (!outSampler) {
			delete pSampler;
			return ERROR_ALLOCATION_FAILED;
		}

		// Cache object
		{
			loadParams.pResourceManager->Cache(objectId, outSampler);
			Print("   ...cached sampler[" + std::to_string(gltfObjectIndex) + "]: " + gltfObjectName + " (objectId=" + std::to_string(objectId) + ")");
		}

		return SUCCESS;
	}

	Result GltfLoader::LoadImageInternal(
		const GltfLoader::InternalLoadParams& loadParams,
		const cgltf_image* pGltfImage,
		scene::Image** ppTargetImage)
	{
		if (IsNull(loadParams.pDevice) || IsNull(pGltfImage) || IsNull(ppTargetImage)) {
			return ERROR_UNEXPECTED_NULL_ARGUMENT;
		}

		// Get GLTF object name
		const std::string gltfObjectName = GetName(pGltfImage);

		// Get GLTF object index to use as object id
		const uint32_t gltfObjectIndex = static_cast<uint32_t>(cgltf_image_index(mGltfData, pGltfImage));
		Print("Loading GLTF image[" + std::to_string(gltfObjectIndex) + "]: " + gltfObjectName);

		// Load image
		vkr::Image* pGrfxImage = nullptr;
		//
		if (!IsNull(pGltfImage->uri)) {
			std::filesystem::path filePath = mGltfTextureDir / ToStringSafe(pGltfImage->uri);
			if (!std::filesystem::exists(filePath)) {
				Error("GLTF file references an image file that doesn't exist (image=" + ToStringSafe(pGltfImage->name) + ", uri=" + ToStringSafe(pGltfImage->uri) + ", file=" + filePath.string());
				return ERROR_PATH_DOES_NOT_EXIST;
			}

			auto ppxres = vkr::vkrUtil::CreateImageFromFile(
				loadParams.pDevice->GetGraphicsQueue(),
				filePath,
				&pGrfxImage);
			if (Failed(ppxres)) {
				return ppxres;
			}
		}
		else if (!IsNull(pGltfImage->buffer_view)) {
			const size_t dataSize = static_cast<size_t>(pGltfImage->buffer_view->size);
			const void* pData = GetStartAddress(pGltfImage->buffer_view);
			if (IsNull(pData)) {
				return ERROR_BAD_DATA_SOURCE;
			}

			Bitmap bitmap;
			auto        ppxres = Bitmap::LoadFromMemory(dataSize, pData, &bitmap);
			if (Failed(ppxres)) {
				return ppxres;
			}

			ppxres = vkr::vkrUtil::CreateImageFromBitmap(
				loadParams.pDevice->GetGraphicsQueue(),
				&bitmap,
				&pGrfxImage);
			if (Failed(ppxres)) {
				return ppxres;
			}
		}
		else {
			return ERROR_SCENE_INVALID_SOURCE_IMAGE;
		}

		// Create image view
		vkr::SampledImageView* pGrfxImageView = nullptr;
		//
		{
			vkr::SampledImageViewCreateInfo createInfo = {};
			createInfo.pImage = pGrfxImage;
			createInfo.imageViewType = vkr::IMAGE_VIEW_TYPE_2D;
			createInfo.format = pGrfxImage->GetFormat();
			createInfo.sampleCount = vkr::SAMPLE_COUNT_1;
			createInfo.mipLevel = 0;
			createInfo.mipLevelCount = pGrfxImage->GetMipLevelCount();
			createInfo.arrayLayer = 0;
			createInfo.arrayLayerCount = pGrfxImage->GetArrayLayerCount();
			createInfo.components = {};

			auto ppxres = loadParams.pDevice->CreateSampledImageView(
				createInfo,
				&pGrfxImageView);
			if (ppxres) {
				loadParams.pDevice->DestroyImage(pGrfxImage);
				return ppxres;
			}
		}

		// Creat target object
		auto pImage = new scene::Image(pGrfxImage, pGrfxImageView);
		if (IsNull(pImage)) {
			loadParams.pDevice->DestroySampledImageView(pGrfxImageView);
			loadParams.pDevice->DestroyImage(pGrfxImage);
			return ERROR_ALLOCATION_FAILED;
		}

		// Set name
		pImage->SetName(gltfObjectName);

		// Assign output
		*ppTargetImage = pImage;

		return SUCCESS;
	}

	Result GltfLoader::FetchImageInternal(
		const GltfLoader::InternalLoadParams& loadParams,
		const cgltf_image* pGltfImage,
		scene::ImageRef& outImage)
	{
		if (IsNull(loadParams.pDevice) || IsNull(loadParams.pResourceManager) || IsNull(pGltfImage)) {
			return ERROR_UNEXPECTED_NULL_ARGUMENT;
		}

		// Get GLTF object name
		const std::string gltfObjectName = GetName(pGltfImage);

		// Get GLTF object index to use as object id
		const uint32_t gltfObjectIndex = static_cast<uint32_t>(cgltf_image_index(mGltfData, pGltfImage));

		// Cached load if object was previously cached
		const uint64_t objectId = CalculateImageObjectId(loadParams, gltfObjectIndex);
		if (loadParams.pResourceManager->Find(objectId, outImage)) {
			Print("Fetched cached image[" + std::to_string(gltfObjectIndex) + "]: " + gltfObjectName + " (objectId=" + std::to_string(objectId) + ")");
			return SUCCESS;
		}

		// Cached failed, so load object
		scene::Image* pImage = nullptr;
		//
		auto ppxres = LoadImageInternal(loadParams, pGltfImage, &pImage);
		if (Failed(ppxres)) {
			return ppxres;
		}
		ASSERT_NULL_ARG(pImage);

		// Create object ref
		outImage = scene::MakeRef(pImage);
		if (!outImage) {
			delete pImage;
			return ERROR_ALLOCATION_FAILED;
		}

		// Cache object
		{
			loadParams.pResourceManager->Cache(objectId, outImage);
			Print("   ...cached image[" + std::to_string(gltfObjectIndex) + "]: " + gltfObjectName + " (objectId=" + std::to_string(objectId) + ")");
		}

		return SUCCESS;
	}

	Result GltfLoader::LoadTextureInternal(
		const GltfLoader::InternalLoadParams& loadParams,
		const cgltf_texture* pGltfTexture,
		scene::Texture** ppTexture)
	{
		if (IsNull(loadParams.pDevice) || IsNull(pGltfTexture) || IsNull(ppTexture)) {
			return ERROR_UNEXPECTED_NULL_ARGUMENT;
		}

		// Get GLTF object name
		const std::string gltfTextureObjectName = GetName(pGltfTexture);
		const std::string gltfImageObjectName = IsNull(pGltfTexture->image) ? "<NULL>" : GetName(pGltfTexture->image);

		// Get GLTF object index to use as object id
		const uint32_t gltfObjectIndex = static_cast<uint32_t>(cgltf_texture_index(mGltfData, pGltfTexture));
		// Textures are often unnamed, so include image name to make log smg more meaningful.
		Print("Loading GLTF texture[" + std::to_string(gltfObjectIndex) + "]: " + gltfTextureObjectName + " (image=" + gltfImageObjectName + ")");

		// Required objects
		scene::SamplerRef targetSampler = nullptr;
		scene::ImageRef   targetImage = nullptr;

		// Fetch if there's a resource manager...
		if (!IsNull(loadParams.pResourceManager)) {
			auto ppxres = FetchSamplerInternal(loadParams, pGltfTexture->sampler, targetSampler);
			if (Failed(ppxres)) {
				return ppxres;
			}

			ppxres = FetchImageInternal(loadParams, pGltfTexture->image, targetImage);
			if (Failed(ppxres)) {
				return ppxres;
			}
		}
		// ...otherwise load!
		else {
			// Load sampler
			scene::Sampler* pTargetSampler = nullptr;
			auto            ppxres = LoadSamplerInternal(loadParams, pGltfTexture->sampler, &pTargetSampler);
			if (Failed(ppxres)) {
				return ppxres;
			}
			ASSERT_NULL_ARG(pTargetSampler);

			targetSampler = scene::MakeRef(pTargetSampler);
			if (!targetSampler) {
				delete pTargetSampler;
				return ERROR_ALLOCATION_FAILED;
			}

			// Load image
			scene::Image* pTargetImage = nullptr;
			ppxres = LoadImageInternal(loadParams, pGltfTexture->image, &pTargetImage);
			if (Failed(ppxres)) {
				return ppxres;
			}
			ASSERT_NULL_ARG(pTargetImage);

			targetImage = scene::MakeRef(pTargetImage);
			if (!targetImage) {
				delete pTargetImage;
				return ERROR_ALLOCATION_FAILED;
			}
		}

		// Create target object
		auto pTexture = new scene::Texture(targetImage, targetSampler);
		if (IsNull(pTexture)) {
			return ERROR_ALLOCATION_FAILED;
		}

		// Set name
		pTexture->SetName(gltfTextureObjectName);

		// Assign output
		*ppTexture = pTexture;

		return SUCCESS;
	}

	Result GltfLoader::FetchTextureInternal(
		const GltfLoader::InternalLoadParams& loadParams,
		const cgltf_texture* pGltfTexture,
		scene::TextureRef& outTexture)
	{
		if (IsNull(loadParams.pDevice) || IsNull(loadParams.pResourceManager) || IsNull(pGltfTexture)) {
			return ERROR_UNEXPECTED_NULL_ARGUMENT;
		}

		// Get GLTF object name
		const std::string gltfObjectName = GetName(pGltfTexture);

		// Get GLTF object index to use as object id
		const uint32_t gltfObjectIndex = static_cast<uint32_t>(cgltf_texture_index(mGltfData, pGltfTexture));

		// Cached load if object was previously cached
		const uint64_t objectId = CalculateTextureObjectId(loadParams, gltfObjectIndex);
		if (loadParams.pResourceManager->Find(objectId, outTexture)) {
			Print("Fetched cached texture[" + std::to_string(gltfObjectIndex) + "]: " + gltfObjectName + " (objectId=" + std::to_string(objectId) + ")");
			return SUCCESS;
		}

		// Cached failed, so load object
		scene::Texture* pTexture = nullptr;
		//
		auto ppxres = LoadTextureInternal(loadParams, pGltfTexture, &pTexture);
		if (Failed(ppxres)) {
			return ppxres;
		}
		ASSERT_NULL_ARG(pTexture);

		// Create object ref
		outTexture = scene::MakeRef(pTexture);
		if (!outTexture) {
			delete pTexture;
			return ERROR_ALLOCATION_FAILED;
		}

		// Cache object
		{
			loadParams.pResourceManager->Cache(objectId, outTexture);
			Print("   ...cached texture[" + std::to_string(gltfObjectIndex) + "]: " + gltfObjectName + " (objectId=" + std::to_string(objectId) + ")");
		}

		return SUCCESS;
	}

	Result GltfLoader::LoadTextureViewInternal(
		const GltfLoader::InternalLoadParams& loadParams,
		const cgltf_texture_view* pGltfTextureView,
		scene::TextureView* pTargetTextureView)
	{
		ASSERT_NULL_ARG(loadParams.pDevice);
		ASSERT_NULL_ARG(pGltfTextureView);
		ASSERT_NULL_ARG(pTargetTextureView);

		// Required object
		scene::TextureRef targetTexture = nullptr;

		// Fetch if there's a resource manager...
		if (!IsNull(loadParams.pResourceManager)) {
			auto ppxres = FetchTextureInternal(loadParams, pGltfTextureView->texture, targetTexture);
			if (Failed(ppxres)) {
				return ppxres;
			}
		}
		// ...otherwise load!
		else {
			scene::Texture* pTargetTexture = nullptr;
			auto            ppxres = LoadTextureInternal(loadParams, pGltfTextureView->texture, &pTargetTexture);
			if (Failed(ppxres)) {
				return ppxres;
			}

			targetTexture = scene::MakeRef(pTargetTexture);
			if (!targetTexture) {
				delete pTargetTexture;
				return ERROR_ALLOCATION_FAILED;
			}
		}

		// Set texture transform if needed
		float2 texCoordTranslate = float2(0, 0);
		float  texCoordRotate = 0;
		float2 texCoordScale = float2(1, 1);
		if (pGltfTextureView->has_transform) {
			texCoordTranslate = float2(pGltfTextureView->transform.offset[0], pGltfTextureView->transform.offset[1]);
			texCoordRotate = pGltfTextureView->transform.rotation;
			texCoordScale = float2(pGltfTextureView->transform.scale[0], pGltfTextureView->transform.scale[1]);
		}

		*pTargetTextureView = scene::TextureView(
			targetTexture,
			texCoordTranslate,
			texCoordRotate,
			texCoordScale);

		return SUCCESS;
	}

	Result GltfLoader::LoadUnlitMaterialInternal(
		const GltfLoader::InternalLoadParams& loadParams,
		const cgltf_material* pGltfMaterial,
		scene::UnlitMaterial* pTargetMaterial)
	{
		ASSERT_NULL_ARG(loadParams.pDevice);
		ASSERT_NULL_ARG(pGltfMaterial);
		ASSERT_NULL_ARG(pTargetMaterial);

		float4 baseColorFactor = float4(0.5f, 0.5f, 0.5f, 1.0f);

		// KHR_materials_unlit uses attributes from pbrMetallicRoughness
		if (pGltfMaterial->has_pbr_metallic_roughness) {
			if (!IsNull(pGltfMaterial->pbr_metallic_roughness.base_color_texture.texture)) {
				auto ppxres = LoadTextureViewInternal(
					loadParams,
					&pGltfMaterial->pbr_metallic_roughness.base_color_texture,
					pTargetMaterial->GetBaseColorTextureViewPtr());
				if (Failed(ppxres)) {
					return ppxres;
				}
			}

			baseColorFactor = *(reinterpret_cast<const float4*>(pGltfMaterial->pbr_metallic_roughness.base_color_factor));
		}

		// Set base color factor
		pTargetMaterial->SetBaseColorFactor(baseColorFactor);

		return SUCCESS;
	}

	Result GltfLoader::LoadPbrMetallicRoughnessMaterialInternal(
		const GltfLoader::InternalLoadParams& loadParams,
		const cgltf_material* pGltfMaterial,
		scene::StandardMaterial* pTargetMaterial)
	{
		ASSERT_NULL_ARG(loadParams.pDevice);
		ASSERT_NULL_ARG(pGltfMaterial);
		ASSERT_NULL_ARG(pTargetMaterial);

		// pbrMetallicRoughness textures
		if (!IsNull(pGltfMaterial->pbr_metallic_roughness.base_color_texture.texture)) {
			auto ppxres = LoadTextureViewInternal(
				loadParams,
				&pGltfMaterial->pbr_metallic_roughness.base_color_texture,
				pTargetMaterial->GetBaseColorTextureViewPtr());
			if (Failed(ppxres)) {
				return ppxres;
			}
		}
		if (!IsNull(pGltfMaterial->pbr_metallic_roughness.metallic_roughness_texture.texture)) {
			auto ppxres = LoadTextureViewInternal(
				loadParams,
				&pGltfMaterial->pbr_metallic_roughness.metallic_roughness_texture,
				pTargetMaterial->GetMetallicRoughnessTextureViewPtr());
			if (Failed(ppxres)) {
				return ppxres;
			}
		}

		// Normal texture
		if (!IsNull(pGltfMaterial->normal_texture.texture)) {
			auto ppxres = LoadTextureViewInternal(
				loadParams,
				&pGltfMaterial->normal_texture,
				pTargetMaterial->GetNormalTextureViewPtr());
			if (Failed(ppxres)) {
				return ppxres;
			}
		}

		// Occlusion texture
		if (!IsNull(pGltfMaterial->occlusion_texture.texture)) {
			auto ppxres = LoadTextureViewInternal(
				loadParams,
				&pGltfMaterial->occlusion_texture,
				pTargetMaterial->GetOcclusionTextureViewPtr());
			if (Failed(ppxres)) {
				return ppxres;
			}
		}

		// Emissive texture
		if (!IsNull(pGltfMaterial->emissive_texture.texture)) {
			auto ppxres = LoadTextureViewInternal(
				loadParams,
				&pGltfMaterial->emissive_texture,
				pTargetMaterial->GetEmissiveTextureViewPtr());
			if (Failed(ppxres)) {
				return ppxres;
			}
		}

		pTargetMaterial->SetBaseColorFactor(*(reinterpret_cast<const float4*>(pGltfMaterial->pbr_metallic_roughness.base_color_factor)));
		pTargetMaterial->SetMetallicFactor(pGltfMaterial->pbr_metallic_roughness.metallic_factor);
		pTargetMaterial->SetRoughnessFactor(pGltfMaterial->pbr_metallic_roughness.roughness_factor);
		pTargetMaterial->SetEmissiveFactor(*(reinterpret_cast<const float3*>(pGltfMaterial->emissive_factor)));

		if (pGltfMaterial->has_emissive_strength) {
			pTargetMaterial->SetEmissiveStrength(pGltfMaterial->emissive_strength.emissive_strength);
		}

		if (!IsNull(pGltfMaterial->occlusion_texture.texture)) {
			pTargetMaterial->SetOcclusionStrength(pGltfMaterial->occlusion_texture.scale);
		}

		return SUCCESS;
	}

	Result GltfLoader::LoadMaterialInternal(
		const GltfLoader::InternalLoadParams& loadParams,
		const cgltf_material* pGltfMaterial,
		scene::Material** ppTargetMaterial)
	{
		if (IsNull(loadParams.pDevice) || IsNull(pGltfMaterial) || IsNull(ppTargetMaterial)) {
			return ERROR_UNEXPECTED_NULL_ARGUMENT;
		}

		// Get GLTF object name
		const std::string gltfObjectName = GetName(pGltfMaterial);

		// Get GLTF object index to use as object id
		const uint32_t gltfObjectIndex = static_cast<uint32_t>(cgltf_material_index(mGltfData, pGltfMaterial));
		Print("Loading GLTF material[" + std::to_string(gltfObjectIndex) + "]: " + gltfObjectName);

		// Get material ident
		const std::string materialIdent = mMaterialSelector->DetermineMaterial(pGltfMaterial);

		// Create material - this should never return a NULL object
		auto pMaterial = loadParams.pMaterialFactory->CreateMaterial(materialIdent);
		if (IsNull(pMaterial)) {
			ASSERT_MSG(false, "Material factory returned a NULL material");
		}

		// Load Unlit
		if (pMaterial->GetIdentString() == MATERIAL_IDENT_UNLIT) {
			auto ppxres = LoadUnlitMaterialInternal(
				loadParams,
				pGltfMaterial,
				static_cast<scene::UnlitMaterial*>(pMaterial));
			if (Failed(ppxres)) {
				return ppxres;
			}
		}
		// Load MetallicRoughness
		else if (pMaterial->GetIdentString() == MATERIAL_IDENT_STANDARD) {
			auto ppxres = LoadPbrMetallicRoughnessMaterialInternal(
				loadParams,
				pGltfMaterial,
				static_cast<scene::StandardMaterial*>(pMaterial));
			if (Failed(ppxres)) {
				return ppxres;
			}
		}

		// Set name
		pMaterial->SetName(gltfObjectName);

		// Assign output
		*ppTargetMaterial = pMaterial;

		return SUCCESS;
	}

	Result GltfLoader::FetchMaterialInternal(
		const GltfLoader::InternalLoadParams& loadParams,
		const cgltf_material* pGltfMaterial,
		scene::MaterialRef& outMaterial)
	{
		if (IsNull(loadParams.pDevice) || IsNull(loadParams.pResourceManager) || IsNull(pGltfMaterial)) {
			return ERROR_UNEXPECTED_NULL_ARGUMENT;
		}

		// Get GLTF object name
		const std::string gltfObjectName = GetName(pGltfMaterial);

		// Get GLTF object index to use as object id
		const uint32_t gltfObjectIndex = static_cast<uint32_t>(cgltf_material_index(mGltfData, pGltfMaterial));

		// Cached load if object was previously cached
		const uint64_t objectId = CalculateMaterialObjectId(loadParams, gltfObjectIndex);
		if (loadParams.pResourceManager->Find(objectId, outMaterial)) {
			Print("Fetched cached material[" + std::to_string(gltfObjectIndex) + "]: " + gltfObjectName + " (objectId=" + std::to_string(objectId) + ")");
			return SUCCESS;
		}

		// Cached failed, so load object
		scene::Material* pMaterial = nullptr;
		//
		auto ppxres = LoadMaterialInternal(loadParams, pGltfMaterial, &pMaterial);
		if (Failed(ppxres)) {
			return ppxres;
		}
		ASSERT_NULL_ARG(pMaterial);

		// Create object ref
		outMaterial = scene::MakeRef(pMaterial);
		if (!outMaterial) {
			delete pMaterial;
			return ERROR_ALLOCATION_FAILED;
		}

		// Cache object
		{
			loadParams.pResourceManager->Cache(objectId, outMaterial);
			Print("   ...cached material[" + std::to_string(gltfObjectIndex) + "]: " + gltfObjectName + " (objectId=" + std::to_string(objectId) + ")");
		}

		return SUCCESS;
	}

	Result GltfLoader::LoadMeshData(
		const GltfLoader::InternalLoadParams& loadParams,
		const cgltf_mesh* pGltfMesh,
		scene::MeshDataRef& outMeshData,
		std::vector<scene::PrimitiveBatch>& outBatches)
	{
		if (IsNull(loadParams.pDevice) || IsNull(pGltfMesh)) {
			return ERROR_UNEXPECTED_NULL_ARGUMENT;
		}

		// Get GLTF object name
		const std::string gltfObjectName = GetName(pGltfMesh);

		// Get GLTF mesh index
		const uint64_t gltfMeshIndex = static_cast<uint64_t>(cgltf_mesh_index(mGltfData, pGltfMesh));

		// Calculate id using geometry related accessor hash
		const uint64_t objectId = GetMeshAccessorsHash(mGltfData, pGltfMesh);
		Print("Loading mesh data (id=" + std::to_string(objectId) + ") for GLTF mesh[" + std::to_string(gltfMeshIndex) + "]: " + gltfObjectName);

		// Use cached object if possible
		bool hasCachedGeometry = false;
		if (!IsNull(loadParams.pResourceManager)) {
			if (loadParams.pResourceManager->Find(objectId, outMeshData)) {
				Print("   ...cache load mesh data (objectId=" + std::to_string(objectId) + ") for GLTF mesh[" + std::to_string(gltfMeshIndex) + "]: " + gltfObjectName);

				// We don't return here like the other functions because we still need
				// to process the primitives, instead we just set the flag to prevent
				// geometry creation.
				//
				hasCachedGeometry = true;
			}
		}

		// Target vertex formats
		auto targetPositionFormat = scene::kVertexPositionFormat;
		auto targetTexCoordFormat = loadParams.requiredVertexAttributes.bits.texCoords ? scene::kVertexAttributeTexCoordFormat : vkr::FORMAT_UNDEFINED;
		auto targetNormalFormat = loadParams.requiredVertexAttributes.bits.normals ? scene::kVertexAttributeNormalFormat : vkr::FORMAT_UNDEFINED;
		auto targetTangentFormat = loadParams.requiredVertexAttributes.bits.tangents ? scene::kVertexAttributeTagentFormat : vkr::FORMAT_UNDEFINED;
		auto targetColorFormat = loadParams.requiredVertexAttributes.bits.colors ? scene::kVertexAttributeColorFormat : vkr::FORMAT_UNDEFINED;

		const uint32_t targetTexCoordElementSize = (targetTexCoordFormat != vkr::FORMAT_UNDEFINED) ? vkr::GetFormatDescription(targetTexCoordFormat)->bytesPerTexel : 0;
		const uint32_t targetNormalElementSize = (targetNormalFormat != vkr::FORMAT_UNDEFINED) ? vkr::GetFormatDescription(targetNormalFormat)->bytesPerTexel : 0;
		const uint32_t targetTangentElementSize = (targetTangentFormat != vkr::FORMAT_UNDEFINED) ? vkr::GetFormatDescription(targetTangentFormat)->bytesPerTexel : 0;
		const uint32_t targetColorElementSize = (targetColorFormat != vkr::FORMAT_UNDEFINED) ? vkr::GetFormatDescription(targetColorFormat)->bytesPerTexel : 0;

		const uint32_t targetPositionElementSize = vkr::GetFormatDescription(targetPositionFormat)->bytesPerTexel;
		const uint32_t targetAttributesElementSize = targetTexCoordElementSize + targetNormalElementSize + targetTangentElementSize + targetColorElementSize;

		struct BatchInfo
		{
			scene::MaterialRef material = nullptr;
			uint32_t           indexDataOffset = 0; // Must have 4 byte alignment
			uint32_t           indexDataSize = 0;
			uint32_t           positionDataOffset = 0; // Must have 4 byte alignment
			uint32_t           positionDataSize = 0;
			uint32_t           attributeDataOffset = 0; // Must have 4 byte alignment
			uint32_t           attributeDataSize = 0;
			vkr::Format       indexFormat = vkr::FORMAT_UNDEFINED;
			uint32_t           indexCount = 0;
			uint32_t           vertexCount = 0;
			AABB          boundingBox = {};
		};

		// Build out batch infos
		std::vector<BatchInfo> batchInfos;
		//
		uint32_t totalDataSize = 0;
		//
		for (cgltf_size primIdx = 0; primIdx < pGltfMesh->primitives_count; ++primIdx) {
			const cgltf_primitive* pGltfPrimitive = &pGltfMesh->primitives[primIdx];

			// Only triangle geometry right now
			if (pGltfPrimitive->type != cgltf_primitive_type_triangles) {
				ASSERT_MSG(false, "GLTF: only triangle primitives are supported");
				return ERROR_SCENE_UNSUPPORTED_TOPOLOGY_TYPE;
			}

			// We require index data so bail if there isn't index data.
			if (IsNull(pGltfPrimitive->indices)) {
				ASSERT_MSG(false, "GLTF mesh primitive does not have index data");
				return ERROR_SCENE_INVALID_SOURCE_GEOMETRY_INDEX_DATA;
			}

			// Get index format
			//
			// It's valid for this to be UNDEFINED, means the primitive doesn't have any index data.
			// However, if it's not UNDEFINED, UINT16, or UINT32 then it's a format we can't handle.
			//
			auto indexFormat = GetFormat(pGltfPrimitive->indices);
			if ((indexFormat != vkr::FORMAT_UNDEFINED) && (indexFormat != vkr::FORMAT_R16_UINT) && (indexFormat != vkr::FORMAT_R32_UINT)) {
				ASSERT_MSG(false, "GLTF mesh primitive has unrecognized index format");
				return ERROR_SCENE_INVALID_SOURCE_GEOMETRY_INDEX_TYPE;
			}

			// Index data size
			const uint32_t indexCount = !IsNull(pGltfPrimitive->indices) ? static_cast<uint32_t>(pGltfPrimitive->indices->count) : 0;
			const uint32_t indexElementSize = vkr::GetFormatDescription(indexFormat)->bytesPerTexel;
			const uint32_t indexDataSize = indexCount * indexElementSize;

			// Get position accessor
			const VertexAccessors gltflAccessors = GetVertexAccessors(pGltfPrimitive);
			if (IsNull(gltflAccessors.pPositions)) {
				ASSERT_MSG(false, "GLTF mesh primitive position accessor is NULL");
				return ERROR_SCENE_INVALID_SOURCE_GEOMETRY_VERTEX_DATA;
			}

			// Vertex data sizes
			const uint32_t vertexCount = static_cast<uint32_t>(gltflAccessors.pPositions->count);
			const uint32_t positionDataSize = vertexCount * targetPositionElementSize;
			const uint32_t attributeDataSize = vertexCount * targetAttributesElementSize;

			// Index data offset
			const uint32_t indexDataOffset = totalDataSize;
			totalDataSize += RoundUp<uint32_t>(indexDataSize, 4);
			// Position data offset
			const uint32_t positionDataOffset = totalDataSize;
			totalDataSize += RoundUp<uint32_t>(positionDataSize, 4);
			// Attribute data offset;
			const uint32_t attributeDataOffset = totalDataSize;
			totalDataSize += RoundUp<uint32_t>(attributeDataSize, 4);

			// Build out batch info with data we'll need later
			BatchInfo batchInfo = {};
			batchInfo.indexDataOffset = indexDataOffset;
			batchInfo.indexDataSize = indexDataSize;
			batchInfo.positionDataOffset = positionDataOffset;
			batchInfo.positionDataSize = positionDataSize;
			batchInfo.attributeDataOffset = attributeDataOffset;
			batchInfo.attributeDataSize = attributeDataSize;
			batchInfo.indexFormat = indexFormat;
			batchInfo.indexCount = indexCount;

			// Material
			{
				// Yes, it's completely possible for GLTF primitives to have no material.
				// For example, if you create a cube in Blender and export it without
				// assigning a material to it. Obviously, this results in material being
				// NULL. Use error material if GLTF material is NULL.
				//
				if (!IsNull(pGltfPrimitive->material)) {
					const uint64_t materialId = cgltf_material_index(mGltfData, pGltfPrimitive->material);
					loadParams.pResourceManager->Find(materialId, batchInfo.material);
				}
				else {
					auto pMaterial = loadParams.pMaterialFactory->CreateMaterial(MATERIAL_IDENT_ERROR);
					if (IsNull(pMaterial)) {
						ASSERT_MSG(false, "could not create ErrorMaterial for GLTF mesh primitive");
						return ERROR_SCENE_INVALID_SOURCE_MATERIAL;
					}

					batchInfo.material = scene::MakeRef(pMaterial);
					if (!batchInfo.material) {
						delete pMaterial;
						return ERROR_ALLOCATION_FAILED;
					}
				}
				ASSERT_MSG(batchInfo.material != nullptr, "GLTF mesh primitive material is NULL");
			}

			// Add
			batchInfos.push_back(batchInfo);
		}

		// Create GPU buffer and copy geometry data to it
		vkr::BufferPtr targetGpuBuffer = outMeshData ? outMeshData->GetGpuBuffer() : nullptr;
		//
		if (!targetGpuBuffer) {
			vkr::BufferCreateInfo bufferCreateInfo = {};
			bufferCreateInfo.size = totalDataSize;
			bufferCreateInfo.usageFlags.bits.transferSrc = true;
			bufferCreateInfo.memoryUsage = vkr::MemoryUsage::CPUToGPU;
			bufferCreateInfo.initialState = vkr::ResourceState::CopySrc;

			// Create staging buffer
			//
			vkr::BufferPtr stagingBuffer;
			auto            ppxres = loadParams.pDevice->CreateBuffer(bufferCreateInfo, &stagingBuffer);
			if (Failed(ppxres)) {
				ASSERT_MSG(false, "staging buffer creation failed");
				return ppxres;
			}

			// Scoped destory buffers if there's an early exit
			vkr::ScopeDestroyer SCOPED_DESTROYER = vkr::ScopeDestroyer(loadParams.pDevice);
			SCOPED_DESTROYER.AddObject(stagingBuffer);

			// Create GPU buffer
			bufferCreateInfo.usageFlags.bits.indexBuffer = true;
			bufferCreateInfo.usageFlags.bits.vertexBuffer = true;
			bufferCreateInfo.usageFlags.bits.transferDst = true;
			bufferCreateInfo.memoryUsage = vkr::MemoryUsage::GPUOnly;
			bufferCreateInfo.initialState = vkr::ResourceState::General;
			//
			ppxres = loadParams.pDevice->CreateBuffer(bufferCreateInfo, &targetGpuBuffer);
			if (Failed(ppxres)) {
				ASSERT_MSG(false, "GPU buffer creation failed");
				return ppxres;
			}
			SCOPED_DESTROYER.AddObject(targetGpuBuffer);

			// Map staging buffer
			char* pStagingBaseAddr = nullptr;
			ppxres = stagingBuffer->MapMemory(0, reinterpret_cast<void**>(&pStagingBaseAddr));
			if (Failed(ppxres)) {
				ASSERT_MSG(false, "staging buffer mapping failed");
				return ppxres;
			}

			// Stage data for copy
			for (cgltf_size primIdx = 0; primIdx < pGltfMesh->primitives_count; ++primIdx) {
				const cgltf_primitive* pGltfPrimitive = &pGltfMesh->primitives[primIdx];
				BatchInfo& batch = batchInfos[primIdx];

				// Our resulting geometry must have index data for draw efficiency.
				// This means that if the index format is undefined we need to generate
				// topology indices for it.
				//
				//
				bool genTopologyIndices = false;
				if (batch.indexFormat == vkr::FORMAT_UNDEFINED) {
					genTopologyIndices = true;
					batch.indexFormat = (batch.vertexCount < 65536) ? vkr::FORMAT_R16_UINT : vkr::FORMAT_R32_UINT;
				}

				// Create genTopologyIndices so we can repack gemetry data into position planar + packed vertex attributes.
				vkr::Geometry   targetGeometry = {};
				const bool hasAttributes = (loadParams.requiredVertexAttributes.mask != 0);
				//
				{
					auto createInfo = hasAttributes ? vkr::GeometryCreateInfo::PositionPlanarU16() : vkr::GeometryCreateInfo::PlanarU16();
					if (batch.indexFormat == vkr::FORMAT_R32_UINT) {
						createInfo = hasAttributes ? vkr::GeometryCreateInfo::PositionPlanarU32() : vkr::GeometryCreateInfo::PlanarU32();
					}
					if (loadParams.requiredVertexAttributes.bits.texCoords) createInfo.AddTexCoord(targetTexCoordFormat);
					if (loadParams.requiredVertexAttributes.bits.normals) createInfo.AddNormal(targetNormalFormat);
					if (loadParams.requiredVertexAttributes.bits.tangents) createInfo.AddTangent(targetTangentFormat);
					if (loadParams.requiredVertexAttributes.bits.colors) createInfo.AddColor(targetColorFormat);

					auto ppxres = vkr::Geometry::Create(createInfo, &targetGeometry);
					if (Failed(ppxres)) {
						return ppxres;
					}
				}

				// Repack geometry data for batch
				{
					// Process indices
					//
					// REMINDER: It's possible for a primitive to not have index data
					if (!IsNull(pGltfPrimitive->indices)) {
						// Get start of index data
						auto pGltfAccessor = pGltfPrimitive->indices;
						auto pGltfIndices = GetStartAddress(mGltfData, pGltfAccessor);
						ASSERT_MSG(!IsNull(pGltfIndices), "GLTF: indices data start is NULL");

						// UINT32
						if (batch.indexFormat == vkr::FORMAT_R32_UINT) {
							const uint32_t* pGltfIndex = static_cast<const uint32_t*>(pGltfIndices);
							for (cgltf_size i = 0; i < pGltfAccessor->count; ++i, ++pGltfIndex) {
								targetGeometry.AppendIndex(*pGltfIndex);
							}
						}
						// UINT16
						else if (batch.indexFormat == vkr::FORMAT_R16_UINT) {
							const uint16_t* pGltfIndex = static_cast<const uint16_t*>(pGltfIndices);
							for (cgltf_size i = 0; i < pGltfAccessor->count; ++i, ++pGltfIndex) {
								targetGeometry.AppendIndex(*pGltfIndex);
							}
						}
					}
				}

				// Vertices
				{
					VertexAccessors gltflAccessors = GetVertexAccessors(pGltfPrimitive);
					//
					// Bail if position accessor is NULL: no vertex positions, no geometry data
					//
					if (IsNull(gltflAccessors.pPositions)) {
						ASSERT_MSG(false, "GLTF mesh primitive is missing position data");
						return ERROR_SCENE_INVALID_SOURCE_GEOMETRY_VERTEX_DATA;
					}

					// Bounding box
					bool hasBoundingBox = (gltflAccessors.pPositions->has_min && gltflAccessors.pPositions->has_max);
					if (hasBoundingBox) {
						batch.boundingBox = AABB(
							*reinterpret_cast<const float3*>(gltflAccessors.pPositions->min),
							*reinterpret_cast<const float3*>(gltflAccessors.pPositions->max));
					}

					// Determine if we need to process vertices
					//
					// Assume we have to procss the vertices
					bool processVertices = true;
					if (hasCachedGeometry) {
						// If we have cached geometry, we only process the vertices if
						// we need the bounding box.
						//
						processVertices = !hasBoundingBox;
					}

					// Check vertex data formats
					auto positionFormat = GetFormat(gltflAccessors.pPositions);
					auto texCoordFormat = GetFormat(gltflAccessors.pTexCoords);
					auto normalFormat = GetFormat(gltflAccessors.pNormals);
					auto tangentFormat = GetFormat(gltflAccessors.pTangents);
					auto colorFormat = GetFormat(gltflAccessors.pColors);
					//
					ASSERT_MSG((positionFormat == targetPositionFormat), "GLTF: vertex positions format is not supported");
					//
					if (loadParams.requiredVertexAttributes.bits.texCoords && !IsNull(gltflAccessors.pTexCoords)) {
						ASSERT_MSG((texCoordFormat == targetTexCoordFormat), "GLTF: vertex tex coords sourceIndexTypeFormat is not supported");
					}
					if (loadParams.requiredVertexAttributes.bits.normals && !IsNull(gltflAccessors.pNormals)) {
						ASSERT_MSG((normalFormat == targetNormalFormat), "GLTF: vertex normals format is not supported");
					}
					if (loadParams.requiredVertexAttributes.bits.tangents && !IsNull(gltflAccessors.pTangents)) {
						ASSERT_MSG((tangentFormat == targetTangentFormat), "GLTF: vertex tangents format is not supported");
					}
					if (loadParams.requiredVertexAttributes.bits.colors && !IsNull(gltflAccessors.pColors)) {
						ASSERT_MSG((colorFormat == targetColorFormat), "GLTF: vertex colors format is not supported");
					}

					// Data starts
					const float3* pGltflPositions = static_cast<const float3*>(GetStartAddress(mGltfData, gltflAccessors.pPositions));
					const float3* pGltflNormals = static_cast<const float3*>(GetStartAddress(mGltfData, gltflAccessors.pNormals));
					const float4* pGltflTangents = static_cast<const float4*>(GetStartAddress(mGltfData, gltflAccessors.pTangents));
					const float3* pGltflColors = static_cast<const float3*>(GetStartAddress(mGltfData, gltflAccessors.pColors));
					const float2* pGltflTexCoords = static_cast<const float2*>(GetStartAddress(mGltfData, gltflAccessors.pTexCoords));

					// Process vertex data
					for (cgltf_size i = 0; i < gltflAccessors.pPositions->count; ++i) {
						vkr::TriMeshVertexData vertexData = {};

						// Position
						vertexData.position = *pGltflPositions;
						++pGltflPositions;
						// Normals
						if (loadParams.requiredVertexAttributes.bits.normals && !IsNull(pGltflNormals)) {
							vertexData.normal = *pGltflNormals;
							++pGltflNormals;
						}
						// Tangents
						if (loadParams.requiredVertexAttributes.bits.tangents && !IsNull(pGltflTangents)) {
							vertexData.tangent = *pGltflTangents;
							++pGltflTangents;
						}
						// Colors
						if (loadParams.requiredVertexAttributes.bits.colors && !IsNull(pGltflColors)) {
							vertexData.color = *pGltflColors;
							++pGltflColors;
						}
						// Tex cooord
						if (loadParams.requiredVertexAttributes.bits.texCoords && !IsNull(pGltflTexCoords)) {
							vertexData.texCoord = *pGltflTexCoords;
							++pGltflTexCoords;
						}

						// Append vertex data
						targetGeometry.AppendVertexData(vertexData);

						// Generate topolgoy indices if necessary
						if (genTopologyIndices) {
							uint32_t index = (targetGeometry.GetVertexCount() - 1);
							targetGeometry.AppendIndex(index);
						}

						if (!hasBoundingBox) {
							if (i > 0) {
								batch.boundingBox.Expand(vertexData.position);
							}
							else {
								batch.boundingBox = AABB(vertexData.position, vertexData.position);
							}
						}
					}
				}

				// Geometry data must match what's in the batch
				const uint32_t repackedIndexBufferSize = targetGeometry.GetIndexBuffer()->GetSize();
				const uint32_t repackedPositionBufferSize = targetGeometry.GetVertexBuffer(0)->GetSize();
				const uint32_t repackedAttributeBufferSize = hasAttributes ? targetGeometry.GetVertexBuffer(1)->GetSize() : 0;
				if (repackedIndexBufferSize != batch.indexDataSize) {
					ASSERT_MSG(false, "repacked index buffer size does not match batch's index data size");
					return ERROR_SCENE_INVALID_SOURCE_GEOMETRY_INDEX_DATA;
				}
				if (repackedPositionBufferSize != batch.positionDataSize) {
					ASSERT_MSG(false, "repacked position buffer size does not match batch's position data size");
					return ERROR_SCENE_INVALID_SOURCE_GEOMETRY_INDEX_DATA;
				}
				if (repackedAttributeBufferSize != batch.attributeDataSize) {
					ASSERT_MSG(false, "repacked attribute buffer size does not match batch's attribute data size");
					return ERROR_SCENE_INVALID_SOURCE_GEOMETRY_INDEX_DATA;
				}

				// We're good - copy data to the staging buffer
				{
					// Indices
					const void* pSrcData = targetGeometry.GetIndexBuffer()->GetData();
					char* pDstData = pStagingBaseAddr + batch.indexDataOffset;
					ASSERT_MSG((static_cast<uint32_t>((pDstData + repackedIndexBufferSize) - pStagingBaseAddr) <= stagingBuffer->GetSize()), "index data exceeds buffer range");
					memcpy(pDstData, pSrcData, repackedIndexBufferSize);

					// Positions
					pSrcData = targetGeometry.GetVertexBuffer(0)->GetData();
					pDstData = pStagingBaseAddr + batch.positionDataOffset;
					ASSERT_MSG((static_cast<uint32_t>((pDstData + repackedPositionBufferSize) - pStagingBaseAddr) <= stagingBuffer->GetSize()), "position data exceeds buffer range");
					memcpy(pDstData, pSrcData, repackedPositionBufferSize);

					// Attributes
					if (hasAttributes) {
						pSrcData = targetGeometry.GetVertexBuffer(1)->GetData();
						pDstData = pStagingBaseAddr + batch.attributeDataOffset;
						ASSERT_MSG((static_cast<uint32_t>((pDstData + repackedAttributeBufferSize) - pStagingBaseAddr) <= stagingBuffer->GetSize()), "attribute data exceeds buffer range");
						memcpy(pDstData, pSrcData, repackedAttributeBufferSize);
					}
				}
			}

			// Copy staging buffer to GPU buffer
			vkr::BufferToBufferCopyInfo copyInfo = {};
			copyInfo.srcBuffer.offset = 0;
			copyInfo.dstBuffer.offset = 0;
			copyInfo.size = stagingBuffer->GetSize();
			//
			ppxres = loadParams.pDevice->GetGraphicsQueue()->CopyBufferToBuffer(
				&copyInfo,
				stagingBuffer,
				targetGpuBuffer,
				vkr::ResourceState::General,
				vkr::ResourceState::General);
			if (Failed(ppxres)) {
				ASSERT_MSG(false, "staging buffer to GPU buffer copy failed");
				return ppxres;
			}

			// We're good if we got here, release objects from scoped destroy
			SCOPED_DESTROYER.ReleaseAll();
			// Destroy staging buffer since we're done with it
			stagingBuffer->UnmapMemory();
			loadParams.pDevice->DestroyBuffer(stagingBuffer);
		}

		// Build batches
		for (uint32_t batchIdx = 0; batchIdx < CountU32(batchInfos); ++batchIdx) {
			const auto& batch = batchInfos[batchIdx];

			const vkr::IndexType indexType = (batch.indexFormat == vkr::FORMAT_R32_UINT) ? vkr::INDEX_TYPE_UINT32 : vkr::INDEX_TYPE_UINT16;
			vkr::IndexBufferView indexBufferView = vkr::IndexBufferView(targetGpuBuffer, indexType, batch.indexDataOffset, batch.indexDataSize);

			vkr::VertexBufferView positionBufferView = vkr::VertexBufferView(targetGpuBuffer, targetPositionElementSize, batch.positionDataOffset, batch.positionDataSize);
			vkr::VertexBufferView attributeBufferView = vkr::VertexBufferView((batch.attributeDataSize != 0) ? targetGpuBuffer : nullptr, targetAttributesElementSize, batch.attributeDataOffset, batch.attributeDataSize);

			scene::PrimitiveBatch targetBatch = scene::PrimitiveBatch(
				batch.material,
				indexBufferView,
				positionBufferView,
				attributeBufferView,
				batch.indexCount,
				batch.vertexCount,
				batch.boundingBox);

			outBatches.push_back(targetBatch);
		}

		// Create GPU mesh from geometry if we don't have cached geometry
		if (!hasCachedGeometry) {
			// Allocate mesh data
			auto pTargetMeshData = new scene::MeshData(loadParams.requiredVertexAttributes, targetGpuBuffer);
			if (IsNull(pTargetMeshData)) {
				loadParams.pDevice->DestroyBuffer(targetGpuBuffer);
				return ERROR_ALLOCATION_FAILED;
			}

			// Create ref
			outMeshData = scene::MakeRef(pTargetMeshData);

			// Set name
			outMeshData->SetName(gltfObjectName);

			// Cache object if caching
			if (!IsNull(loadParams.pResourceManager)) {
				Print("   ...caching mesh data (objectId=" + std::to_string(objectId) + ") for GLTF mesh[" + std::to_string(gltfMeshIndex) + "]: " + gltfObjectName);
				loadParams.pResourceManager->Cache(objectId, outMeshData);
			}
		}

		return SUCCESS;
	}

	Result GltfLoader::LoadMeshInternal(
		const GltfLoader::InternalLoadParams& externalLoadParams,
		const cgltf_mesh* pGltfMesh,
		scene::Mesh** ppTargetMesh)
	{
		if (IsNull(externalLoadParams.pDevice) || IsNull(pGltfMesh) || IsNull(ppTargetMesh)) {
			return ERROR_UNEXPECTED_NULL_ARGUMENT;
		}

		// Get GLTF object name
		const std::string gltfObjectName = GetName(pGltfMesh);

		// Get GLTF object index to use as object id
		const uint32_t gltfObjectIndex = static_cast<uint32_t>(cgltf_mesh_index(mGltfData, pGltfMesh));
		Print("Loading GLTF mesh[" + std::to_string(gltfObjectIndex) + "]: " + gltfObjectName);

		// Create target mesh - scoped to prevent localLoadParams from leaking
		scene::Mesh* pTargetMesh = nullptr;
		//
		{
			// Copy the external load params so we can control the resource manager and vertex attributes.
			//
			auto localLoadParams = externalLoadParams;
			localLoadParams.requiredVertexAttributes = scene::VertexAttributeFlags::None();

			// If a resource manager wasn't passed in, this means we're dealing with
			// a standalone mesh which needs a local resource manager. So, we'll
			// create one if that's the case.
			//
			std::unique_ptr<scene::ResourceManager> localResourceManager;
			if (IsNull(localLoadParams.pResourceManager)) {
				localResourceManager = std::make_unique<scene::ResourceManager>();
				if (!localResourceManager) {
					return ERROR_ALLOCATION_FAILED;
				}

				// Override resource manager
				localLoadParams.pResourceManager = localResourceManager.get();
			}

			// Load materials for primitives and get required vertex attributes
			for (cgltf_size primIdx = 0; primIdx < pGltfMesh->primitives_count; ++primIdx) {
				const cgltf_primitive* pGltfPrimitive = &pGltfMesh->primitives[primIdx];
				const cgltf_material* pGltfMaterial = pGltfPrimitive->material;

				// Yes, it's completely possible for GLTF primitives to have no material.
				// For example, if you create a cube in Blender and export it without
				// assigning a material to it. Obviously, this results in material being
				// NULL. No need to load anything if it's NULL.
				//
				if (IsNull(pGltfMaterial)) {
					continue;
				}

				// Fetch material since we'll always have a resource manager
				scene::MaterialRef loadedMaterial;
				//
				auto ppxres = FetchMaterialInternal(
					localLoadParams,
					pGltfMaterial,
					loadedMaterial);
				if (ppxres) {
					return ppxres;
				}

				// Get material ident
				const std::string materialIdent = loadedMaterial->GetIdentString();

				// Get material's required vertex attributes
				auto materialRequiredVertexAttributes = localLoadParams.pMaterialFactory->GetRequiredVertexAttributes(materialIdent);
				localLoadParams.requiredVertexAttributes |= materialRequiredVertexAttributes;
			}

			// If we don't have a local resource manager, then it means we're loading in through a scene.
			// If we're loading in through a scene, then we need to user the mesh data vertex attributes
			// supplied to this function...if they were supplied.
			//
			if (!localResourceManager && !IsNull(localLoadParams.pMeshMaterialVertexAttributeMasks)) {
				auto it = localLoadParams.pMeshMaterialVertexAttributeMasks->find(pGltfMesh);

				// Keeps the local mesh's vertex attribute if search failed.
				//
				if (it != localLoadParams.pMeshMaterialVertexAttributeMasks->end()) {
					localLoadParams.requiredVertexAttributes = (*it).second;
				}
			}

			// Override the local vertex attributes with loadParams has vertex attributes
			if (externalLoadParams.requiredVertexAttributes.mask != 0) {
				localLoadParams.requiredVertexAttributes = externalLoadParams.requiredVertexAttributes;
			}

			//
			// Disable vertex colors for now: some work is needed to handle format conversion.
			//
			localLoadParams.requiredVertexAttributes.bits.colors = false;

			// Load mesh data and batches
			scene::MeshDataRef                 meshData = nullptr;
			std::vector<scene::PrimitiveBatch> batches = {};
			//
			{
				auto ppxres = LoadMeshData(
					localLoadParams,
					pGltfMesh,
					meshData,
					batches);
				if (Failed(ppxres)) {
					return ppxres;
				}
			}

			// Create target mesh
			if (localResourceManager) {
				// Allocate mesh with local resource manager
				pTargetMesh = new scene::Mesh(
					std::move(localResourceManager),
					meshData,
					std::move(batches));
				if (IsNull(pTargetMesh)) {
					return ERROR_ALLOCATION_FAILED;
				}
			}
			else {
				// Allocate mesh
				pTargetMesh = new scene::Mesh(meshData, std::move(batches));
				if (IsNull(pTargetMesh)) {
					return ERROR_ALLOCATION_FAILED;
				}
			}
		}

		// Set name
		pTargetMesh->SetName(gltfObjectName);

		// Assign output
		*ppTargetMesh = pTargetMesh;

		return SUCCESS;
	}

	Result GltfLoader::FetchMeshInternal(
		const GltfLoader::InternalLoadParams& loadParams,
		const cgltf_mesh* pGltfMesh,
		scene::MeshRef& outMesh)
	{
		if (IsNull(loadParams.pDevice) || IsNull(loadParams.pResourceManager) || IsNull(pGltfMesh)) {
			return ERROR_UNEXPECTED_NULL_ARGUMENT;
		}

		// Get GLTF object name
		const std::string gltfObjectName = GetName(pGltfMesh);

		// Get GLTF object index to use as object id
		const uint32_t gltfObjectIndex = static_cast<uint32_t>(cgltf_mesh_index(mGltfData, pGltfMesh));

		// Cached load if object was previously cached
		const uint64_t objectId = CalculateMeshObjectId(loadParams, gltfObjectIndex);
		if (loadParams.pResourceManager->Find(objectId, outMesh)) {
			Print("Fetched cached mesh[" + std::to_string(gltfObjectIndex) + "]: " + gltfObjectName + " (objectId=" + std::to_string(objectId) + ")");
			return SUCCESS;
		}

		// Cached failed, so load object
		scene::Mesh* pMesh = nullptr;
		//
		auto ppxres = LoadMeshInternal(loadParams, pGltfMesh, &pMesh);
		if (Failed(ppxres)) {
			return ppxres;
		}
		ASSERT_NULL_ARG(pMesh);

		// Create object ref
		outMesh = scene::MakeRef(pMesh);
		if (!outMesh) {
			delete pMesh;
			return ERROR_ALLOCATION_FAILED;
		}

		// Cache object
		{
			loadParams.pResourceManager->Cache(objectId, outMesh);
			Print("   ...cached mesh[" + std::to_string(gltfObjectIndex) + "]: " + gltfObjectName + " (objectId=" + std::to_string(objectId) + ")");
		}

		return SUCCESS;
	}

	Result GltfLoader::LoadNodeInternal(
		const GltfLoader::InternalLoadParams& loadParams,
		const cgltf_node* pGltfNode,
		scene::Node** ppTargetNode)
	{
		if ((!loadParams.transformOnly && IsNull(loadParams.pDevice)) || IsNull(pGltfNode) || IsNull(ppTargetNode)) {
			return ERROR_UNEXPECTED_NULL_ARGUMENT;
		}

		// Get GLTF object name
		const std::string gltfObjectName = GetName(pGltfNode);

		// Get GLTF object index to use as object id
		const uint32_t index = static_cast<uint32_t>(cgltf_node_index(mGltfData, pGltfNode));
		Print("Loading GLTF node[" + std::to_string(index) + "]: " + gltfObjectName);

		// Get node type
		const scene::NodeType nodeType = loadParams.transformOnly ? scene::NODE_TYPE_TRANSFORM : GetNodeType(pGltfNode);

		// Load based on node type
		scene::Node* pTargetNode = nullptr;
		//
		switch (nodeType) {
		default: {
			return ERROR_SCENE_UNSUPPORTED_NODE_TYPE;
		} break;

			   // Transform node
		case scene::NODE_TYPE_TRANSFORM: {
			pTargetNode = new scene::Node(loadParams.pTargetScene);
		} break;

									   // Mesh node
		case scene::NODE_TYPE_MESH: {
			const cgltf_mesh* pGltfMesh = pGltfNode->mesh;
			if (IsNull(pGltfMesh)) {
				return ERROR_SCENE_INVALID_SOURCE_MESH;
			}

			// Required object
			scene::MeshRef targetMesh = nullptr;

			// Fetch if there's a resource manager...
			if (!IsNull(loadParams.pResourceManager)) {
				auto ppxres = FetchMeshInternal(loadParams, pGltfMesh, targetMesh);
				if (Failed(ppxres)) {
					return ppxres;
				}
			}
			// ...otherwise load!
			else {
				scene::Mesh* pTargetMesh = nullptr;
				auto         ppxres = LoadMeshInternal(loadParams, pGltfMesh, &pTargetMesh);
				if (Failed(ppxres)) {
					return ppxres;
				}

				targetMesh = scene::MakeRef(pTargetMesh);
				if (!targetMesh) {
					delete pTargetMesh;
					return ERROR_ALLOCATION_FAILED;
				}
			}

			// Allocate node
			pTargetNode = new scene::MeshNode(targetMesh, loadParams.pTargetScene);
		} break;

								  // Camera node
		case scene::NODE_TYPE_CAMERA: {
			const cgltf_camera* pGltfCamera = pGltfNode->camera;
			if (IsNull(pGltfCamera)) {
				return ERROR_SCENE_INVALID_SOURCE_CAMERA;
			}

			// Create camera
			std::unique_ptr<Camera> camera;
			if (pGltfCamera->type == cgltf_camera_type_perspective) {
				float fov = pGltfCamera->data.perspective.yfov;

				float aspect = 1.0f;
				if (pGltfCamera->data.perspective.has_aspect_ratio) {
					aspect = pGltfCamera->data.perspective.aspect_ratio;
					// BigWheels using horizontal FoV
					fov = aspect * pGltfCamera->data.perspective.yfov;
				}

				float nearClip = pGltfCamera->data.perspective.znear;
				float farClip = pGltfCamera->data.perspective.has_zfar ? pGltfCamera->data.perspective.zfar : (nearClip + 1000.0f);

				camera = std::unique_ptr<Camera>(new PerspCamera(glm::degrees(fov), aspect, nearClip, farClip));
			}
			else if (pGltfCamera->type == cgltf_camera_type_orthographic) {
				float left = -pGltfCamera->data.orthographic.xmag;
				float right = pGltfCamera->data.orthographic.xmag;
				float top = -pGltfCamera->data.orthographic.ymag;
				float bottom = pGltfCamera->data.orthographic.ymag;
				float nearClip = pGltfCamera->data.orthographic.znear;
				float farClip = pGltfCamera->data.orthographic.zfar;

				camera = std::unique_ptr<Camera>(new OrthoCamera(left, right, bottom, top, nearClip, farClip));
			}
			//
			if (!camera) {
				return ERROR_SCENE_INVALID_SOURCE_CAMERA;
			}

			// Allocate node
			pTargetNode = new scene::CameraNode(std::move(camera), loadParams.pTargetScene);
		} break;

									// Light node
		case scene::NODE_TYPE_LIGHT: {
			const cgltf_light* pGltfLight = pGltfNode->light;
			if (IsNull(pGltfLight)) {
				return ERROR_SCENE_INVALID_SOURCE_LIGHT;
			}

			scene::LightType lightType = scene::LIGHT_TYPE_UNDEFINED;
			//
			switch (pGltfLight->type) {
			default: break;
			case cgltf_light_type_directional: lightType = scene::LIGHT_TYPE_DIRECTIONAL; break;
			case cgltf_light_type_point: lightType = scene::LIGHT_TYPE_POINT; break;
			case cgltf_light_type_spot: lightType = scene::LIGHT_TYPE_SPOT; break;
			}
			//
			if (lightType == scene::LIGHT_TYPE_UNDEFINED) {
				return ERROR_SCENE_INVALID_SOURCE_LIGHT;
			}

			// Allocate node
			auto pLightNode = new scene::LightNode(loadParams.pTargetScene);
			if (IsNull(pLightNode)) {
				return ERROR_ALLOCATION_FAILED;
			}

			pLightNode->SetType(lightType);
			pLightNode->SetColor(float3(pGltfLight->color[0], pGltfLight->color[1], pGltfLight->color[2]));
			pLightNode->SetIntensity(pGltfLight->intensity);
			pLightNode->SetDistance(pGltfLight->range);
			pLightNode->SetSpotInnerConeAngle(pGltfLight->spot_inner_cone_angle);
			pLightNode->SetSpotOuterConeAngle(pGltfLight->spot_outer_cone_angle);

			// Assign node
			pTargetNode = pLightNode;
		} break;
		}
		//
		if (IsNull(pTargetNode)) {
			return ERROR_ALLOCATION_FAILED;
		}

		// Set transform
		{
			if (pGltfNode->has_translation) {
				pTargetNode->SetTranslation(float3(
					pGltfNode->translation[0],
					pGltfNode->translation[1],
					pGltfNode->translation[2]));
			}

			if (pGltfNode->has_rotation) {
				float x = pGltfNode->rotation[0];
				float y = pGltfNode->rotation[1];
				float z = pGltfNode->rotation[2];
				float w = pGltfNode->rotation[3];

				// Extract euler angles using a matrix
				//
				// The values returned by glm::eulerAngles(quat) expects a
				// certain rotation order. It wasn't clear at the time of
				// this writing what that should be exactly. So, for the time
				// being, we'll use the matrix route and stick with XYZ.
				//
				auto q = glm::quat(w, x, y, z);
				auto R = glm::toMat4(q);

				float3 euler = float3(0);
				glm::extractEulerAngleXYZ(R, euler.x, euler.y, euler.z);

				pTargetNode->SetRotation(euler);
				pTargetNode->SetRotationOrder(Transform::RotationOrder::XYZ);
			}

			if (pGltfNode->has_scale) {
				pTargetNode->SetScale(float3(
					pGltfNode->scale[0],
					pGltfNode->scale[1],
					pGltfNode->scale[2]));
			}
		}

		// Set name
		pTargetNode->SetName(gltfObjectName);

		// Assign output
		*ppTargetNode = pTargetNode;

		return SUCCESS;
	}

	Result GltfLoader::FetchNodeInternal(
		const GltfLoader::InternalLoadParams& loadParams,
		const cgltf_node* pGltfNode,
		scene::NodeRef& outNode)
	{
		if (IsNull(loadParams.pDevice) || IsNull(loadParams.pResourceManager) || IsNull(pGltfNode)) {
			return ERROR_UNEXPECTED_NULL_ARGUMENT;
		}

		// Load object
		scene::Node* pNode = nullptr;
		//
		auto ppxres = LoadNodeInternal(loadParams, pGltfNode, &pNode);
		if (Failed(ppxres)) {
			return ppxres;
		}
		ASSERT_NULL_ARG(pNode);

		// Create object ref
		outNode = scene::MakeRef(pNode);
		if (!outNode) {
			delete pNode;
			return ERROR_ALLOCATION_FAILED;
		}

		return SUCCESS;
	}

	void GltfLoader::GetUniqueGltfNodeIndices(
		const cgltf_node* pGltfNode,
		std::set<cgltf_size>& uniqueGltfNodeIndices) const
	{
		if (IsNull(pGltfNode)) {
			return;
		}

		cgltf_size nodeIndex = cgltf_node_index(mGltfData, pGltfNode);
		uniqueGltfNodeIndices.insert(nodeIndex);

		// Process children
		for (cgltf_size i = 0; i < pGltfNode->children_count; ++i) {
			const cgltf_node* pGltfChildNode = pGltfNode->children[i];

			// Recurse!
			GetUniqueGltfNodeIndices(pGltfChildNode, uniqueGltfNodeIndices);
		}
	}

	Result GltfLoader::LoadSceneInternal(
		const GltfLoader::InternalLoadParams& loadParams,
		const cgltf_scene* pGltfScene,
		scene::Scene* pTargetScene)
	{
		if (IsNull(loadParams.pDevice) || IsNull(pGltfScene)) {
			return ERROR_UNEXPECTED_NULL_ARGUMENT;
		}

		// Get GLTF object name
		const std::string gltfObjectName = GetName(pGltfScene);

		// Get GLTF object index to use as object id
		const uint32_t index = static_cast<uint32_t>(cgltf_scene_index(mGltfData, pGltfScene));
		Print("Loading GLTF scene[" + std::to_string(index) + "]: " + gltfObjectName);

		// GLTF scenes contain only the root nodes. We need to walk scene's
		// root nodes and collect the file level indices for all the nodes.
		//
		std::set<cgltf_size> uniqueGltfNodeIndices;
		{
			for (cgltf_size i = 0; i < pGltfScene->nodes_count; ++i) {
				const cgltf_node* pGltfNode = pGltfScene->nodes[i];
				GetUniqueGltfNodeIndices(pGltfNode, uniqueGltfNodeIndices);
			}
		}

		// Load scene
		//
		// Keeps some maps so we can process the children
		std::unordered_map<cgltf_size, scene::Node*> indexToNodeMap;
		//
		{
			// Load nodes
			for (cgltf_size gltfNodeIndex : uniqueGltfNodeIndices) {
				const cgltf_node* pGltfNode = &mGltfData->nodes[gltfNodeIndex];

				scene::NodeRef node;
				//
				auto ppxres = FetchNodeInternal(
					loadParams,
					pGltfNode,
					node);
				if (Failed(ppxres)) {
					return ppxres;
				}

				// Save pointer to update map
				auto pNode = node.get();

				// Add node to scene
				ppxres = pTargetScene->AddNode(std::move(node));
				if (Failed(ppxres)) {
					return ppxres;
				}

				// Update map
				indexToNodeMap[gltfNodeIndex] = pNode;
			}
		}

		// Build children nodes
		{
			// Since all the nodes were flattened out, we don't need to recurse.
			for (cgltf_size gltfNodeIndex : uniqueGltfNodeIndices) {
				// Get target node
				auto it = indexToNodeMap.find(gltfNodeIndex);
				ASSERT_MSG((it != indexToNodeMap.end()), "GLTF node gltfObjectIndex has no mappping to a target node");
				scene::Node* pTargetNode = (*it).second;

				// GLTF node
				const cgltf_node* pGltfNode = &mGltfData->nodes[gltfNodeIndex];

				// Iterate node's children
				for (cgltf_size i = 0; i < pGltfNode->children_count; ++i) {
					// Get GLTF child node
					const cgltf_node* pGltfChildNode = pGltfNode->children[i];

					// Get GLTF child node index
					cgltf_size gltfChildNodeIndex = cgltf_node_index(mGltfData, pGltfChildNode);

					// Get target child node
					auto itChild = indexToNodeMap.find(gltfChildNodeIndex);
					ASSERT_MSG((it != indexToNodeMap.end()), "GLTF child node gltfObjectIndex has no mappping to a target child node");
					scene::Node* pTargetChildNode = (*itChild).second;

					// Add target child node to target node
					pTargetNode->AddChild(pTargetChildNode);
				}
			}
		}

		// Set name
		pTargetScene->SetName(gltfObjectName);

		return SUCCESS;
	}

	uint32_t GltfLoader::GetSamplerCount() const
	{
		return IsNull(mGltfData) ? 0 : static_cast<uint32_t>(mGltfData->samplers_count);
	}

	uint32_t GltfLoader::GetImageCount() const
	{
		return IsNull(mGltfData) ? 0 : static_cast<uint32_t>(mGltfData->images_count);
	}

	uint32_t GltfLoader::GetTextureCount() const
	{
		return IsNull(mGltfData) ? 0 : static_cast<uint32_t>(mGltfData->textures_count);
	}

	uint32_t GltfLoader::GetMaterialCount() const
	{
		return IsNull(mGltfData) ? 0 : static_cast<uint32_t>(mGltfData->materials_count);
	}

	uint32_t GltfLoader::GetMeshCount() const
	{
		return IsNull(mGltfData) ? 0 : static_cast<uint32_t>(mGltfData->meshes_count);
	}

	uint32_t GltfLoader::GetNodeCount() const
	{
		return IsNull(mGltfData) ? 0 : static_cast<uint32_t>(mGltfData->nodes_count);
	}

	uint32_t GltfLoader::GetSceneCount() const
	{
		return IsNull(mGltfData) ? 0 : static_cast<uint32_t>(mGltfData->scenes_count);
	}

	int32_t GltfLoader::GetSamplerIndex(const std::string& name) const
	{
		auto arrayBegin = mGltfData->samplers;
		auto arrayEnd = mGltfData->samplers + mGltfData->samplers_count;

		auto pGltfObject = std::find_if(
			arrayBegin,
			arrayEnd,
			[&name](const cgltf_sampler& elem) -> bool {
				bool match = !IsNull(elem.name) && (name == elem.name);
				return match;
			});

		int32_t index = (pGltfObject >= arrayEnd) ? -1 : static_cast<int32_t>(pGltfObject - arrayBegin);
		return index;
	}

	int32_t GltfLoader::GetImageIndex(const std::string& name) const
	{
		auto arrayBegin = mGltfData->images;
		auto arrayEnd = mGltfData->images + mGltfData->images_count;

		auto pGltfObject = std::find_if(
			arrayBegin,
			arrayEnd,
			[&name](const cgltf_image& elem) -> bool {
				bool match = !IsNull(elem.name) && (name == elem.name);
				return match;
			});

		int32_t index = (pGltfObject >= arrayEnd) ? -1 : static_cast<int32_t>(pGltfObject - arrayBegin);
		return index;
	}

	int32_t GltfLoader::GetTextureIndex(const std::string& name) const
	{
		auto arrayBegin = mGltfData->textures;
		auto arrayEnd = mGltfData->textures + mGltfData->textures_count;

		auto pGltfObject = std::find_if(
			arrayBegin,
			arrayEnd,
			[&name](const cgltf_texture& elem) -> bool {
				bool match = !IsNull(elem.name) && (name == elem.name);
				return match;
			});

		int32_t index = (pGltfObject >= arrayEnd) ? -1 : static_cast<int32_t>(pGltfObject - arrayBegin);
		return index;
	}

	int32_t GltfLoader::GetMaterialIndex(const std::string& name) const
	{
		auto arrayBegin = mGltfData->materials;
		auto arrayEnd = mGltfData->materials + mGltfData->materials_count;

		auto pGltfObject = std::find_if(
			arrayBegin,
			arrayEnd,
			[&name](const cgltf_material& elem) -> bool {
				bool match = !IsNull(elem.name) && (name == elem.name);
				return match;
			});

		int32_t index = (pGltfObject >= arrayEnd) ? -1 : static_cast<int32_t>(pGltfObject - arrayBegin);
		return index;
	}

	int32_t GltfLoader::GetMeshIndex(const std::string& name) const
	{
		auto arrayBegin = mGltfData->meshes;
		auto arrayEnd = mGltfData->meshes + mGltfData->meshes_count;

		auto pGltfObject = std::find_if(
			arrayBegin,
			arrayEnd,
			[&name](const cgltf_mesh& elem) -> bool {
				bool match = !IsNull(elem.name) && (name == elem.name);
				return match;
			});

		int32_t index = (pGltfObject >= arrayEnd) ? -1 : static_cast<int32_t>(pGltfObject - arrayBegin);
		return index;
	}

	int32_t GltfLoader::GetNodeIndex(const std::string& name) const
	{
		auto arrayBegin = mGltfData->nodes;
		auto arrayEnd = mGltfData->nodes + mGltfData->nodes_count;

		auto pGltfObject = std::find_if(
			arrayBegin,
			arrayEnd,
			[&name](const cgltf_node& elem) -> bool {
				bool match = !IsNull(elem.name) && (name == elem.name);
				return match;
			});

		int32_t index = (pGltfObject >= arrayEnd) ? -1 : static_cast<int32_t>(pGltfObject - arrayBegin);
		return index;
	}

	int32_t GltfLoader::GetSceneIndex(const std::string& name) const
	{
		auto arrayBegin = mGltfData->scenes;
		auto arrayEnd = mGltfData->scenes + mGltfData->scenes_count;

		auto pGltfObject = std::find_if(
			arrayBegin,
			arrayEnd,
			[&name](const cgltf_scene& elem) -> bool {
				bool match = !IsNull(elem.name) && (name == elem.name);
				return match;
			});

		int32_t index = (pGltfObject >= arrayEnd) ? -1 : static_cast<int32_t>(pGltfObject - arrayBegin);
		return index;
	}

	Result GltfLoader::LoadSampler(
		vkr::RenderDevice* pDevice,
		uint32_t         samplerIndex,
		scene::Sampler** ppTargetSampler)
	{
		if (IsNull(pDevice)) {
			return ERROR_UNEXPECTED_NULL_ARGUMENT;
		}

		if (!HasGltfData()) {
			return ERROR_SCENE_NO_SOURCE_DATA;
		}

		if (samplerIndex >= static_cast<uint32_t>(mGltfData->samplers_count)) {
			return ERROR_OUT_OF_RANGE;
		}

		auto pGltfSampler = &mGltfData->samplers[samplerIndex];
		if (IsNull(pGltfSampler)) {
			return ERROR_ELEMENT_NOT_FOUND;
		}

		GltfLoader::InternalLoadParams loadParams = {};
		loadParams.pDevice = pDevice;

		auto ppxres = LoadSamplerInternal(
			loadParams,
			pGltfSampler,
			ppTargetSampler);
		if (Failed(ppxres)) {
			return ppxres;
		}

		return SUCCESS;
	}

	Result GltfLoader::LoadSampler(
		vkr::RenderDevice* pDevice,
		const std::string& samplerName,
		scene::Sampler** ppTargetSampler)
	{
		if (!HasGltfData()) {
			return ERROR_SCENE_NO_SOURCE_DATA;
		}

		int32_t meshIndex = this->GetSamplerIndex(samplerName);
		if (meshIndex < 0) {
			return ERROR_ELEMENT_NOT_FOUND;
		}

		return LoadSampler(
			pDevice,
			static_cast<uint32_t>(meshIndex),
			ppTargetSampler);
	}

	Result GltfLoader::LoadImage(
		vkr::RenderDevice* pDevice,
		uint32_t       imageIndex,
		scene::Image** ppTargetImage)
	{
		if (IsNull(pDevice)) {
			return ERROR_UNEXPECTED_NULL_ARGUMENT;
		}

		if (!HasGltfData()) {
			return ERROR_SCENE_NO_SOURCE_DATA;
		}

		if (imageIndex >= static_cast<uint32_t>(mGltfData->samplers_count)) {
			return ERROR_OUT_OF_RANGE;
		}

		auto pGltfImage = &mGltfData->images[imageIndex];
		if (IsNull(pGltfImage)) {
			return ERROR_ELEMENT_NOT_FOUND;
		}

		GltfLoader::InternalLoadParams loadParams = {};
		loadParams.pDevice = pDevice;

		auto ppxres = LoadImageInternal(
			loadParams,
			pGltfImage,
			ppTargetImage);
		if (Failed(ppxres)) {
			return ppxres;
		}

		return SUCCESS;
	}

	Result GltfLoader::LoadImage(
		vkr::RenderDevice* pDevice,
		const std::string& imageName,
		scene::Image** ppTargetImage)
	{
		if (!HasGltfData()) {
			return ERROR_SCENE_NO_SOURCE_DATA;
		}

		int32_t imageIndex = this->GetImageIndex(imageName);
		if (imageIndex < 0) {
			return ERROR_ELEMENT_NOT_FOUND;
		}

		return LoadImage(
			pDevice,
			static_cast<uint32_t>(imageIndex),
			ppTargetImage);
	}

	Result GltfLoader::LoadTexture(
		vkr::RenderDevice* pDevice,
		uint32_t         textureIndex,
		scene::Texture** ppTargetTexture)
	{
		if (IsNull(pDevice)) {
			return ERROR_UNEXPECTED_NULL_ARGUMENT;
		}

		if (!HasGltfData()) {
			return ERROR_SCENE_NO_SOURCE_DATA;
		}

		if (textureIndex >= static_cast<uint32_t>(mGltfData->samplers_count)) {
			return ERROR_OUT_OF_RANGE;
		}

		auto pGltfTexture = &mGltfData->textures[textureIndex];
		if (IsNull(pGltfTexture)) {
			return ERROR_ELEMENT_NOT_FOUND;
		}

		GltfLoader::InternalLoadParams loadParams = {};
		loadParams.pDevice = pDevice;

		auto ppxres = LoadTextureInternal(
			loadParams,
			pGltfTexture,
			ppTargetTexture);
		if (Failed(ppxres)) {
			return ppxres;
		}

		return SUCCESS;
	}

	Result GltfLoader::LoadTexture(
		vkr::RenderDevice* pDevice,
		const std::string& textureName,
		scene::Texture** ppTargetTexture)
	{
		if (!HasGltfData()) {
			return ERROR_SCENE_NO_SOURCE_DATA;
		}

		int32_t textureIndex = this->GetTextureIndex(textureName);
		if (textureIndex < 0) {
			return ERROR_ELEMENT_NOT_FOUND;
		}

		return LoadTexture(
			pDevice,
			static_cast<uint32_t>(textureIndex),
			ppTargetTexture);
	}

	Result GltfLoader::LoadMaterial(
		vkr::RenderDevice* pDevice,
		uint32_t                  materialIndex,
		scene::Material** ppTargetMaterial,
		const scene::LoadOptions& loadOptions)
	{
		if (IsNull(pDevice)) {
			return ERROR_UNEXPECTED_NULL_ARGUMENT;
		}

		if (!HasGltfData()) {
			return ERROR_SCENE_NO_SOURCE_DATA;
		}

		if (materialIndex >= static_cast<uint32_t>(mGltfData->samplers_count)) {
			return ERROR_OUT_OF_RANGE;
		}

		auto pGltfMaterial = &mGltfData->materials[materialIndex];
		if (IsNull(pGltfMaterial)) {
			return ERROR_ELEMENT_NOT_FOUND;
		}

		GltfLoader::InternalLoadParams loadParams = {};
		loadParams.pDevice = pDevice;

		auto ppxres = LoadMaterialInternal(
			loadParams,
			pGltfMaterial,
			ppTargetMaterial);
		if (Failed(ppxres)) {
			return ppxres;
		}

		return SUCCESS;
	}

	Result GltfLoader::LoadMaterial(
		vkr::RenderDevice* pDevice,
		const std::string& materialName,
		scene::Material** ppTargetMaterial,
		const scene::LoadOptions& loadOptions)
	{
		if (!HasGltfData()) {
			return ERROR_SCENE_NO_SOURCE_DATA;
		}

		int32_t materialIndex = this->GetMaterialIndex(materialName);
		if (materialIndex < 0) {
			return ERROR_ELEMENT_NOT_FOUND;
		}

		return LoadMaterial(
			pDevice,
			static_cast<uint32_t>(materialIndex),
			ppTargetMaterial,
			loadOptions);
	}

	Result GltfLoader::LoadMesh(
		vkr::RenderDevice* pDevice,
		uint32_t                  meshIndex,
		scene::Mesh** ppTargetMesh,
		const scene::LoadOptions& loadOptions)
	{
		if (IsNull(pDevice)) {
			return ERROR_UNEXPECTED_NULL_ARGUMENT;
		}

		if (!HasGltfData()) {
			return ERROR_SCENE_NO_SOURCE_DATA;
		}

		if (meshIndex >= static_cast<uint32_t>(mGltfData->meshes_count)) {
			return ERROR_OUT_OF_RANGE;
		}

		auto pGltfMesh = &mGltfData->meshes[meshIndex];
		if (IsNull(pGltfMesh)) {
			return ERROR_ELEMENT_NOT_FOUND;
		}

		GltfLoader::InternalLoadParams loadParams = {};
		loadParams.pDevice = pDevice;
		loadParams.pMaterialFactory = loadOptions.GetMaterialFactory();
		loadParams.requiredVertexAttributes = loadOptions.GetRequiredAttributes();

		// Use default material factory if one wasn't supplied
		if (IsNull(loadParams.pMaterialFactory)) {
			loadParams.pMaterialFactory = &mDefaultMaterialFactory;
		}

		auto ppxres = LoadMeshInternal(
			loadParams,
			pGltfMesh,
			ppTargetMesh);
		if (Failed(ppxres)) {
			return ppxres;
		}

		return SUCCESS;
	}

	Result GltfLoader::LoadMesh(
		vkr::RenderDevice* pDevice,
		const std::string& meshName,
		scene::Mesh** ppTargetMesh,
		const scene::LoadOptions& loadOptions)
	{
		if (!HasGltfData()) {
			return ERROR_SCENE_NO_SOURCE_DATA;
		}

		int32_t meshIndex = this->GetMeshIndex(meshName);
		if (meshIndex < 0) {
			return ERROR_ELEMENT_NOT_FOUND;
		}

		return LoadMesh(
			pDevice,
			static_cast<uint32_t>(meshIndex),
			ppTargetMesh,
			loadOptions);
	}

	Result GltfLoader::LoadNode(
		vkr::RenderDevice* pDevice,
		uint32_t                  nodeIndex,
		scene::Node** ppTargetNode,
		const scene::LoadOptions& loadOptions)
	{
		if (IsNull(pDevice)) {
			return ERROR_UNEXPECTED_NULL_ARGUMENT;
		}

		if (!HasGltfData()) {
			return ERROR_SCENE_NO_SOURCE_DATA;
		}

		if (nodeIndex >= static_cast<uint32_t>(mGltfData->nodes_count)) {
			return ERROR_OUT_OF_RANGE;
		}

		auto pGltNode = &mGltfData->nodes[nodeIndex];
		if (IsNull(pGltNode)) {
			return ERROR_ELEMENT_NOT_FOUND;
		}

		GltfLoader::InternalLoadParams loadParams = {};
		loadParams.pDevice = pDevice;
		loadParams.pMaterialFactory = loadOptions.GetMaterialFactory();
		loadParams.requiredVertexAttributes = loadOptions.GetRequiredAttributes();

		// Use default material factory if one wasn't supplied
		if (IsNull(loadParams.pMaterialFactory)) {
			loadParams.pMaterialFactory = &mDefaultMaterialFactory;
		}

		auto ppxres = LoadNodeInternal(
			loadParams,
			pGltNode,
			ppTargetNode);
		if (Failed(ppxres)) {
			return ppxres;
		}

		return SUCCESS;
	}

	Result GltfLoader::LoadNode(
		vkr::RenderDevice* pDevice,
		const std::string& nodeName,
		scene::Node** ppTargetNode,
		const scene::LoadOptions& loadOptions)
	{
		if (IsNull(pDevice)) {
			return ERROR_UNEXPECTED_NULL_ARGUMENT;
		}

		int32_t nodeIndex = this->GetNodeIndex(nodeName);
		if (nodeIndex < 0) {
			return ERROR_ELEMENT_NOT_FOUND;
		}

		return LoadNode(
			pDevice,
			static_cast<uint32_t>(nodeIndex),
			ppTargetNode,
			loadOptions);
	}

	Result GltfLoader::LoadNodeTransformOnly(
		uint32_t      nodeIndex,
		scene::Node** ppTargetNode)
	{
		if (!HasGltfData()) {
			return ERROR_SCENE_NO_SOURCE_DATA;
		}

		if (nodeIndex >= static_cast<uint32_t>(mGltfData->nodes_count)) {
			return ERROR_OUT_OF_RANGE;
		}

		auto pGltNode = &mGltfData->nodes[nodeIndex];
		if (IsNull(pGltNode)) {
			return ERROR_ELEMENT_NOT_FOUND;
		}

		GltfLoader::InternalLoadParams loadParams = {};
		loadParams.transformOnly = true;

		auto ppxres = LoadNodeInternal(
			loadParams,
			pGltNode,
			ppTargetNode);
		if (Failed(ppxres)) {
			return ppxres;
		}

		return SUCCESS;
	}

	Result GltfLoader::LoadNodeTransformOnly(
		const std::string& nodeName,
		scene::Node** ppTargetNode)
	{
		int32_t nodeIndex = this->GetNodeIndex(nodeName);
		if (nodeIndex < 0) {
			return ERROR_ELEMENT_NOT_FOUND;
		}

		return LoadNodeTransformOnly(
			static_cast<uint32_t>(nodeIndex),
			ppTargetNode);
	}

	Result GltfLoader::LoadScene(
		vkr::RenderDevice* pDevice,
		uint32_t                  sceneIndex,
		scene::Scene** ppTargetScene,
		const scene::LoadOptions& loadOptions)
	{
		if (IsNull(pDevice) || IsNull(ppTargetScene)) {
			return ERROR_UNEXPECTED_NULL_ARGUMENT;
		}

		if (!HasGltfData()) {
			return ERROR_SCENE_NO_SOURCE_DATA;
		}

		if (sceneIndex >= static_cast<uint32_t>(mGltfData->scenes_count)) {
			return ERROR_OUT_OF_RANGE;
		}

		auto pGltfScene = &mGltfData->scenes[sceneIndex];
		if (IsNull(pGltfScene)) {
			return ERROR_ELEMENT_NOT_FOUND;
		}

		GltfLoader::InternalLoadParams loadParams = {};
		loadParams.pDevice = pDevice;
		loadParams.pMaterialFactory = loadOptions.GetMaterialFactory();
		loadParams.requiredVertexAttributes = loadOptions.GetRequiredAttributes();

		// Use default material factory if one wasn't supplied
		if (IsNull(loadParams.pMaterialFactory)) {
			loadParams.pMaterialFactory = &mDefaultMaterialFactory;
		}

		// Build mesh material -> vertex attribute masks mappings
		GltfLoader::MeshMaterialVertexAttributeMasks meshDataVertexAttributes;
		CalculateMeshMaterialVertexAttributeMasks(
			loadParams.pMaterialFactory,
			&meshDataVertexAttributes);

		loadParams.pMeshMaterialVertexAttributeMasks = &meshDataVertexAttributes;

		// Allocate resource manager
		auto resourceManager = std::make_unique<scene::ResourceManager>();

		// Set laod params resource manager
		loadParams.pResourceManager = resourceManager.get();

		// Allocate the node so we can set the resource manager and target scene
		scene::Scene* pTargetScene = new scene::Scene(std::move(resourceManager));
		if (IsNull(pTargetScene)) {
			return ERROR_ALLOCATION_FAILED;
		}

		// Set load params target scene
		loadParams.pTargetScene = pTargetScene;

		// Load scene
		auto ppxres = LoadSceneInternal(
			loadParams,
			pGltfScene,
			pTargetScene);
		if (Failed(ppxres)) {
			return ppxres;
		}

		Print("Scene load complete: " + GetName(pGltfScene));
		Print("   Num samplers : " + std::to_string(pTargetScene->GetSamplerCount()));
		Print("   Num images   : " + std::to_string(pTargetScene->GetImageCount()));
		Print("   Num textures : " + std::to_string(pTargetScene->GetTextureCount()));
		Print("   Num materials: " + std::to_string(pTargetScene->GetMaterialCount()));
		Print("   Num mesh data: " + std::to_string(pTargetScene->GetMeshDataCount()));
		Print("   Num meshes   : " + std::to_string(pTargetScene->GetMeshCount()));

		*ppTargetScene = pTargetScene;

		return SUCCESS;
	}

	Result GltfLoader::LoadScene(
		vkr::RenderDevice* pDevice,
		const std::string& sceneName,
		scene::Scene** ppTargetScene,
		const scene::LoadOptions& loadOptions)
	{
		if (IsNull(pDevice)) {
			return ERROR_UNEXPECTED_NULL_ARGUMENT;
		}

		int32_t sceneIndex = this->GetSceneIndex(sceneName);
		if (sceneIndex < 0) {
			return ERROR_ELEMENT_NOT_FOUND;
		}

		return LoadScene(
			pDevice,
			static_cast<uint32_t>(sceneIndex),
			ppTargetScene,
			loadOptions);
	}

} // namespace scene

#pragma endregion