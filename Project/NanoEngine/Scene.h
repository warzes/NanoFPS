#pragma once

#include "RenderSystem.h"
#include "Graphics.h"

//=============================================================================
#pragma region [ Scene Core ]

namespace scene
{
	class Image;
	class Material;
	class MaterialFactory;
	class Mesh;
	class MeshData;
	class Node;
	class ResourceManager;
	class Sampler;
	class Scene;
	class Texture;

	using ImagePtr = std::shared_ptr<scene::Image>;
	using MaterialPtr = std::shared_ptr<scene::Material>;
	using MeshPtr = std::shared_ptr<scene::Mesh>;
	using MeshDataPtr = std::shared_ptr<scene::MeshData>;
	using NodePtr = std::shared_ptr<scene::Node>;
	using SamplerPtr = std::shared_ptr<scene::Sampler>;
	using TexturePtr = std::shared_ptr<scene::Texture>;

	enum LightType
	{
		LIGHT_TYPE_UNDEFINED = 0,
		LIGHT_TYPE_DIRECTIONAL = 1,
		LIGHT_TYPE_POINT = 2,
		LIGHT_TYPE_SPOT = 3,
	};

	const uint32_t kVertexPositionBinding = 0;
	const uint32_t kVertexPositionLocation = 0;
	const uint32_t kVertexAttributeBinding = 1;
	const uint32_t kVertexAttributeTexCoordLocation = 1;
	const uint32_t kVertexAttributeNormalLocation = 2;
	const uint32_t kVertexAttributeTangentLocation = 3;
	const uint32_t kVertexAttributeColorLocation = 4;

	const vkr::Format kVertexPositionFormat = vkr::Format::R32G32B32_FLOAT;
	const vkr::Format kVertexAttributeTexCoordFormat = vkr::Format::R32G32_FLOAT;
	const vkr::Format kVertexAttributeNormalFormat = vkr::Format::R32G32B32_FLOAT;
	const vkr::Format kVertexAttributeTagentFormat = vkr::Format::R32G32B32A32_FLOAT;
	const vkr::Format kVertexAttributeColorFormat = vkr::Format::R32G32B32_FLOAT;

	template <typename ObjectT, typename ObjectRefT = std::shared_ptr<ObjectT>>
	ObjectRefT MakeRef(ObjectT* pObject)
	{
		return ObjectRefT(pObject);
	}

	struct VertexAttributeFlags
	{
		union
		{
			struct
			{
				bool texCoords : 1;
				bool normals : 1;
				bool tangents : 1;
				bool colors : 1;
			} bits;
			uint32_t mask = 0;
		};

		constexpr VertexAttributeFlags(uint32_t initialMask = 0) : mask(initialMask) {}

		static VertexAttributeFlags None()
		{
			VertexAttributeFlags flags = {};
			flags.bits.texCoords = false;
			flags.bits.normals = false;
			flags.bits.tangents = false;
			flags.bits.colors = false;
			return flags;
		}

		static VertexAttributeFlags All()
		{
			VertexAttributeFlags flags = {};
			flags.bits.texCoords = true;
			flags.bits.normals = true;
			flags.bits.tangents = true;
			flags.bits.colors = true;
			return flags;
		}

		VertexAttributeFlags& TexCoords(bool enable = true)
		{
			this->bits.texCoords = enable;
			return *this;
		}

		VertexAttributeFlags& Normals(bool enable = true)
		{
			this->bits.normals = enable;
			return *this;
		}

		VertexAttributeFlags& Tangents(bool enable = true)
		{
			this->bits.tangents = enable;
			return *this;
		}

		VertexAttributeFlags& VertexColors(bool enable = true)
		{
			this->bits.colors = enable;
			return *this;
		}

		VertexAttributeFlags& operator&=(const VertexAttributeFlags& rhs)
		{
			this->mask &= rhs.mask;
			return *this;
		}

		VertexAttributeFlags& operator|=(const VertexAttributeFlags& rhs)
		{
			this->mask |= rhs.mask;
			return *this;
		}

		vkr::VertexBinding GetVertexBinding() const
		{
			vkr::VertexBinding binding = vkr::VertexBinding(1, vkr::VertexInputRate::Vertex);

			uint32_t offset = 0;
			if (this->bits.texCoords)
			{
				binding.AppendAttribute(vkr::VertexAttribute{ "TEXCOORD", kVertexAttributeTexCoordLocation, vkr::Format::R32G32_FLOAT, kVertexAttributeBinding, offset, vkr::VertexInputRate::Vertex });
				offset += 8;
			}
			if (this->bits.normals)
			{
				binding.AppendAttribute(vkr::VertexAttribute{ "NORMAL", kVertexAttributeNormalLocation, vkr::Format::R32G32B32_FLOAT, kVertexAttributeBinding, offset, vkr::VertexInputRate::Vertex });
				offset += 12;
			}
			if (this->bits.tangents)
			{
				binding.AppendAttribute(vkr::VertexAttribute{ "TANGENT", kVertexAttributeTangentLocation, vkr::Format::R32G32B32A32_FLOAT, kVertexAttributeBinding, offset, vkr::VertexInputRate::Vertex });
				offset += 16;
			}
			if (this->bits.colors)
			{
				binding.AppendAttribute(vkr::VertexAttribute{ "COLOR", kVertexAttributeColorLocation, vkr::Format::R32G32B32_FLOAT, kVertexAttributeBinding, offset, vkr::VertexInputRate::Vertex });
			}

			return binding;
		}
	};

	inline scene::VertexAttributeFlags operator&(const scene::VertexAttributeFlags& a, const scene::VertexAttributeFlags& b)
	{
		return scene::VertexAttributeFlags(a.mask & b.mask);
	}

	inline scene::VertexAttributeFlags operator|(const scene::VertexAttributeFlags& a, const scene::VertexAttributeFlags& b)
	{
		return scene::VertexAttributeFlags(a.mask | b.mask);
	}

} // namespace scene

#pragma endregion

//=============================================================================
#pragma region [ Scene Node ]

namespace scene
{
	enum NodeType
	{
		NODE_TYPE_TRANSFORM = 0,
		NODE_TYPE_MESH = 1,
		NODE_TYPE_CAMERA = 2,
		NODE_TYPE_LIGHT = 3,
		NODE_TYPE_UNSUPPORTED = 0x7FFFFFFF
	};

	// Scene Graph Node
	// This is the base class for scene graph nodes. It contains transform, parent, children, and visibility properties. scene::Node is instantiable and can be used as a locator/empty/group node that just contains children nodes.
	// This node objects can also be used as standalone objects outside of a scene. Standalone nodes will have neither a parent or children. Loader implementations must not populate a standalone node's parent or children if loading a standalone node.
	// When used as a standalone node, scene::Node stores only transform information.
	class Node : public Transform, public vkr::NamedObjectTrait
	{
	public:
		Node(scene::Scene* pScene);
		~Node() override;

		virtual scene::NodeType GetNodeType() const { return scene::NODE_TYPE_TRANSFORM; }

		bool IsVisible() const { return mVisible; }
		void SetVisible(bool visible, bool recursive = false);

		void SetTranslation(const float3& translation) override;
		void SetRotation(const float3& rotation) override;
		void SetScale(const float3& scale) override;
		void SetRotationOrder(Transform::RotationOrder rotationOrder) override;

		const float4x4& GetEvaluatedMatrix() const;

		scene::Node* GetParent() const { return mParent; }

		uint32_t     GetChildCount() const { return CountU32(mChildren); }
		scene::Node* GetChild(uint32_t index) const;

		// Returns true if pNode is in current node's subtree, this includes the current node itself.
		bool IsInSubTree(const scene::Node* pNode);

		// Adds pNewChild if it doesn't have a parent and if it isn't already a child.
		Result AddChild(scene::Node* pNewChild);

		// Removes pChild if it exists and returns a non-const parentless child, otherwise returns NULL.
		scene::Node* RemoveChild(const scene::Node* pChild);

	private:
		void setParent(scene::Node* pNewParent);
		void setEvaluatedDirty();

		scene::Scene*             mScene = nullptr;
		bool                      mVisible = true;
		mutable Transform         mTransform = {};
		mutable float4x4          mEvaluatedMatrix = float4x4(1);
		mutable bool              mEvaluatedDirty = false;
		scene::Node*              mParent = nullptr;
		std::vector<scene::Node*> mChildren = {};
	};

	// Scene Graph Mesh Node
	// Scene graph nodes that contains reference to a scene::Mesh object.
	// When used as a standalone node, scene::Mesh stores a mesh and its required objects as populated by a loader.
	class MeshNode final : public Node
	{
	public:
		MeshNode(const scene::MeshPtr& mesh, scene::Scene* pScene = nullptr);
		~MeshNode() final;

		scene::NodeType GetNodeType() const final { return scene::NODE_TYPE_MESH; }
		const scene::Mesh* GetMesh() const { return mMesh.get(); }
		void SetMesh(const scene::MeshPtr& mesh);

	private:
		scene::MeshPtr mMesh = nullptr;
	};

	// Scene Graph Camera Node
	// Scene graph node that contains an instance of a Camera object.
	// Can be used for both perspective and orthographic cameras.
	// When used as a standalone node, scene::Camera stores a camera object as populated by a loader.
	class CameraNode final : public Node
	{
	public:
		CameraNode(std::unique_ptr<Camera>&& camera, scene::Scene* pScene = nullptr);
		~CameraNode() final;

		scene::NodeType GetNodeType() const final { return scene::NODE_TYPE_CAMERA; }
		const Camera* GetCamera() const { return mCamera.get(); }
		void SetTranslation(const float3& translation) final;
		void SetRotation(const float3& rotation) final;

	private:
		void UpdateCameraLookAt();

		std::unique_ptr<Camera> mCamera;
	};

	// Scene Graph Light Node
	// Scene graph node that contains all properties to represent different light types.
	// When used as a standalone node, scene::Light stores light information as populated by a loader.
	class LightNode final : public Node
	{
	public:
		LightNode(scene::Scene* pScene = nullptr);
		~LightNode() final;

		scene::NodeType GetNodeType() const final { return scene::NODE_TYPE_LIGHT; }

		const float3& GetColor() const { return mColor; }
		float         GetIntensity() const { return mIntensity; }
		float         GetDistance() const { return mDistance; }
		const float3& GetDirection() const { return mDirection; }
		float         GetSpotInnerConeAngle() const { return mSpotInnerConeAngle; }
		float         GetSpotOuterConeAngle() const { return mSpotOuterConeAngle; }

		void SetType(const scene::LightType type) { mLightType = type; }
		void SetColor(const float3& color) { mColor = color; }
		void SetIntensity(float intensity) { mIntensity = intensity; }
		void SetDistance(float distance) { mDistance = distance; }
		void SetSpotInnerConeAngle(float angle) { mSpotInnerConeAngle = angle; }
		void SetSpotOuterConeAngle(float angle) { mSpotOuterConeAngle = angle; }

		void SetRotation(const float3& rotation) final;

	private:
		scene::LightType mLightType = scene::LIGHT_TYPE_UNDEFINED;
		float3           mColor = float3(1, 1, 1);
		float            mIntensity = 1.0f;
		float            mDistance = 100.0f;
		float3           mDirection = float3(0, -1, 0);
		float            mSpotInnerConeAngle = glm::radians(45.0f);
		float            mSpotOuterConeAngle = glm::radians(50.0f);
	};

} // namespace scene

#pragma endregion

//=============================================================================
#pragma region [ Scene Material ]

// TODO: возможно это надо в графику

#define MATERIAL_IDENT_ERROR    "material_ident:error"
#define MATERIAL_IDENT_DEBUG    "material_ident:debug"
#define MATERIAL_IDENT_UNLIT    "material_ident:unlit"
#define MATERIAL_IDENT_STANDARD "material_ident:standard"

namespace scene
{
	// Image
	// This class wraps a vkr::Image and a vkr::SampledImage objects to make pathing access to the GPU pipelines easier.
	// This class owns mImage and mImageView and destroys them in the destructor.
	// scene::Image objects can be shared between different scene::Texture objects.
	// Corresponds to GLTF's image object.
	class Image final : public vkr::NamedObjectTrait
	{
	public:
		Image(vkr::Image* pImage, vkr::SampledImageView* pImageView);
		~Image();

		vkr::Image* GetImage() const { return mImage.Get(); }
		vkr::SampledImageView* GetImageView() const { return mImageView.Get(); }

	private:
		vkr::ImagePtr            mImage = nullptr;
		vkr::SampledImageViewPtr mImageView = nullptr;
	};

	// Sampler
	// This class wraps a vkr::Sampler object to make sharability on the scene level easier to understand.
	// This class owns mSampler and destroys it in the destructor.
	// scene::Sampler objects can be shared between different scene::Texture objects.
	// Corresponds to GLTF's sampler object.
	class Sampler final : public vkr::NamedObjectTrait
	{
	public:
		Sampler(vkr::Sampler* pSampler);
		~Sampler();

		vkr::Sampler* GetSampler() const { return mSampler.Get(); }

	private:
		vkr::SamplerPtr mSampler;
	};

	// Texture
	// This class is container for references to a scene::Image and a scene::Sampler objects.
	// scene::Texture objects can be shared between different scene::Material objects via the scene::TextureView struct.
	// Corresponds to GLTF's texture objects
	class Texture final : public vkr::NamedObjectTrait
	{
	public:
		Texture() = default;
		Texture( const scene::ImagePtr image, const scene::SamplerPtr sampler);

		const scene::Image* GetImage() const { return mImage.get(); }
		const scene::Sampler* GetSampler() const { return mSampler.get(); }

	private:
		scene::ImagePtr   mImage = nullptr;
		scene::SamplerPtr mSampler = nullptr;
	};

	// Texture View
	// This class contains a reference to a texture object and the transform data that must be applied by the shader before sampling a pixel.
	// scene::Texture view objects are used directly by scene::Material objects.
	// Corresponds to cgltf's texture view object.
	class TextureView final
	{
	public:
		TextureView() = default;
		TextureView(
			const scene::TexturePtr& texture,
			float2                   texCoordTranslate,
			float                    texCoordRotate,
			float2                   texCoordScale);

		const scene::Texture* GetTexture() const { return mTexture.get(); }
		const float2& GetTexCoordTranslate() const { return mTexCoordTranslate; }
		float                 GetTexCoordRotate() const { return mTexCoordRotate; }
		const float2& GetTexCoordScale() const { return mTexCoordScale; }
		const float2x2& GetTexCoordTransform() const { return mTexCoordTransform; }

		bool HasTexture() const { return mTexture ? true : false; }

	private:
		scene::TexturePtr mTexture = nullptr;
		float2            mTexCoordTranslate = float2(0, 0);
		float             mTexCoordRotate = 0;
		float2            mTexCoordScale = float2(1, 1);
		float2x2          mTexCoordTransform = float2x2(1);
	};

	// Base Material Class
	// All materials deriving from this class must have a uniquely identifiable Material Ident String that's returned by GetIdentString().
	// Materials must also provide a mask of all the vertex attributes it requires for rendering.
	// scene::Material derivative objects can be shared beween different scene::Mesh objects via scene::PrimitiveBatch.
	class Material : public vkr::NamedObjectTrait
	{
	public:
		Material() = default;
		virtual ~Material() = default;

		virtual std::string GetIdentString() const = 0;

		virtual scene::VertexAttributeFlags GetRequiredVertexAttributes() const = 0;

		// Returns true if material has parameters for shader
		virtual bool HasParams() const = 0;
		// Returns true if material has at least one texture, otherwise false
		virtual bool HasTextures() const = 0;
	};

	// Error Material for when a primitive batch is missing a material.
	// Implementations should render something recognizable. Default version renders solid purple: float3(1, 0, 1).
	class ErrorMaterial final : public scene::Material
	{
	public:
		std::string GetIdentString() const final { return MATERIAL_IDENT_ERROR; }

		scene::VertexAttributeFlags GetRequiredVertexAttributes() const final;

		bool HasParams() const final { return false; }
		bool HasTextures() const final { return false; }
	};

	// Debug Material
	// Implementations should render position, tex coord, normal, and tangent as color output.
	class DebugMaterial final : public scene::Material
	{
	public:
		std::string GetIdentString() const final { return MATERIAL_IDENT_DEBUG; }

		scene::VertexAttributeFlags GetRequiredVertexAttributes() const final;

		bool HasParams() const final { return false; }
		bool HasTextures() const final { return false; }
	};

	// Unlit Material
	// Implementations should render a texture without any lighting. Base color factor can act as a multiplier for the values from base color texture.
	// Corresponds to GLTF's KHR_materials_unlit:
	//   https://github.com/KhronosGroup/glTF/blob/main/extensions/2.0/Khronos/KHR_materials_unlit/README.md
	class UnlitMaterial final : public scene::Material
	{
	public:
		std::string GetIdentString() const final { return MATERIAL_IDENT_UNLIT; }

		scene::VertexAttributeFlags GetRequiredVertexAttributes() const final;

		bool HasParams() const final { return true; }
		bool HasTextures() const final;

		const float4& GetBaseColorFactor() const { return mBaseColorFactor; }
		const scene::TextureView& GetBaseColorTextureView() const { return mBaseColorTextureView; }
		scene::TextureView* GetBaseColorTextureViewPtr() { return &mBaseColorTextureView; }

		bool HasBaseColorTexture() const { return mBaseColorTextureView.HasTexture(); }

		void SetBaseColorFactor(const float4& value);

	private:
		float4             mBaseColorFactor = float4(1, 1, 1, 1);
		scene::TextureView mBaseColorTextureView = {};
	};

	// Standard (PBR) Material
	// Implementations should render a lit pixel using a PBR method that makes use of the provided fields and textures of this class.
	// Corresponds to GLTF's metallic roughness material:
	//   https://registry.khronos.org/glTF/specs/2.0/glTF-2.0.html#reference-material-pbrmetallicroughness
	class StandardMaterial final : public scene::Material
	{
	public:
		std::string GetIdentString() const final { return MATERIAL_IDENT_STANDARD; }

		scene::VertexAttributeFlags GetRequiredVertexAttributes() const final;

		bool HasParams() const final { return true; }
		bool HasTextures() const final;

		const float4& GetBaseColorFactor() const { return mBaseColorFactor; }
		float         GetMetallicFactor() const { return mMetallicFactor; }
		float         GetRoughnessFactor() const { return mRoughnessFactor; }
		float         GetOcclusionStrength() const { return mOcclusionStrength; }
		const float3& GetEmissiveFactor() const { return mEmissiveFactor; }
		float         GetEmissiveStrength() const { return mEmissiveStrength; }

		const scene::TextureView& GetBaseColorTextureView() const { return mBaseColorTextureView; }
		const scene::TextureView& GetMetallicRoughnessTextureView() const { return mMetallicRoughnessTextureView; }
		const scene::TextureView& GetNormalTextureView() const { return mNormalTextureView; }
		const scene::TextureView& GetOcclusionTextureView() const { return mOcclusionTextureView; }
		const scene::TextureView& GetEmissiveTextureView() const { return mEmissiveTextureView; }

		scene::TextureView* GetBaseColorTextureViewPtr() { return &mBaseColorTextureView; }
		scene::TextureView* GetMetallicRoughnessTextureViewPtr() { return &mMetallicRoughnessTextureView; }
		scene::TextureView* GetNormalTextureViewPtr() { return &mNormalTextureView; }
		scene::TextureView* GetOcclusionTextureViewPtr() { return &mOcclusionTextureView; }
		scene::TextureView* GetEmissiveTextureViewPtr() { return &mEmissiveTextureView; }

		bool HasBaseColorTexture() const { return mBaseColorTextureView.HasTexture(); }
		bool HasMetallicRoughnessTexture() const { return mMetallicRoughnessTextureView.HasTexture(); }
		bool HasNormalTexture() const { return mNormalTextureView.HasTexture(); }
		bool HasOcclusionTexture() const { return mOcclusionTextureView.HasTexture(); }
		bool HasEmissiveTexture() const { return mEmissiveTextureView.HasTexture(); }

		void SetBaseColorFactor(const float4& value);
		void SetMetallicFactor(float value);
		void SetRoughnessFactor(float value);
		void SetOcclusionStrength(float value);
		void SetEmissiveFactor(const float3& value);
		void SetEmissiveStrength(float value);

	private:
		float4             mBaseColorFactor = float4(1, 1, 1, 1);
		float              mMetallicFactor = 1;
		float              mRoughnessFactor = 1;
		float              mOcclusionStrength = 1;
		float3             mEmissiveFactor = float3(0, 0, 0);
		float              mEmissiveStrength = 0;
		scene::TextureView mBaseColorTextureView = {};
		scene::TextureView mMetallicRoughnessTextureView = {};
		scene::TextureView mNormalTextureView = {};
		scene::TextureView mOcclusionTextureView = {};
		scene::TextureView mEmissiveTextureView = {};
	};

	// Material Factory
	// Customizable factory that provides implementations of materials. An application can inherit this class to provide implementations of materials as it sees fit.
	// Materials must be uniquely identifiable by their Material Ident string.
	class MaterialFactory final
	{
	public:
		scene::VertexAttributeFlags GetRequiredVertexAttributes(const std::string& materialIdent) const;
		scene::Material* CreateMaterial(const std::string& materialIdent) const;
	};

} // namespace scene

#pragma endregion

//=============================================================================
#pragma region [ Scene Pipeline Args ]

namespace scene
{
	// size = 8
	struct FrameParams final
	{
		uint32_t frameIndex; // offset = 0
		float    time;       // offset = 4
	};

	// size = 96
	struct CameraParams final
	{
		float4x4 viewProjectionMatrix; // offset = 0
		float3   eyePosition;          // offset = 64
		float    nearDepth;            // offset = 76
		float3   viewDirection;        // offset = 80
		float    farDepth;             // offset = 92
	};

	// size = 128
	struct InstanceParams final
	{
		float4x4 modelMatrix;        // offset = 0
		float4x4 inverseModelMatrix; // offset = 64
	};

	// size = 24
	struct MaterialTextureParams final
	{
		uint     textureIndex;      // offset = 0
		uint     samplerIndex;      // offset = 4
		float2x2 texCoordTransform; // offset = 8
	};

	// size = 164
	struct MaterialParams final
	{
		float4                       baseColorFactor;      // offset = 0
		float                        metallicFactor;       // offset = 16
		float                        roughnessFactor;      // offset = 20
		float                        occlusionStrength;    // offset = 24
		float3                       emissiveFactor;       // offset = 28
		float                        emissiveStrength;     // offset = 40
		scene::MaterialTextureParams baseColorTex;         // offset = 44
		scene::MaterialTextureParams metallicRoughnessTex; // offset = 68
		scene::MaterialTextureParams normalTex;            // offset = 92
		scene::MaterialTextureParams occlusionTex;         // offset = 116
		scene::MaterialTextureParams emssiveTex;           // offset = 140
	};

	class MaterialPipelineArgs final
	{
	public:
		// These constants correspond to values in:
		//   - scene_renderer/shaders/Config.hlsli
		//   - scene_renderer/shaders/MaterialInterface.hlsli
		//
		// @TODO: Find a more appropriate location for these
		static const uint32_t MAX_UNIQUE_MATERIALS = 8192;
		static const uint32_t MAX_TEXTURES_PER_MATERIAL = 6;
		static const uint32_t MAX_MATERIAL_TEXTURES = MAX_UNIQUE_MATERIALS * MAX_TEXTURES_PER_MATERIAL;

		static const uint32_t MAX_MATERIAL_SAMPLERS = 64;

		static const uint32_t MAX_IBL_MAPS = 16;

		static const uint32_t FRAME_PARAMS_REGISTER = 1;
		static const uint32_t CAMERA_PARAMS_REGISTER = 2;
		static const uint32_t INSTANCE_PARAMS_REGISTER = 3;
		static const uint32_t MATERIAL_PARAMS_REGISTER = 4;
		static const uint32_t BRDF_LUT_SAMPLER_REGISTER = 124;
		static const uint32_t BRDF_LUT_TEXTURE_REGISTER = 125;
		static const uint32_t IBL_IRRADIANCE_SAMPLER_REGISTER = 126;
		static const uint32_t IBL_ENVIRONMENT_SAMPLER_REGISTER = 127;
		static const uint32_t IBL_IRRADIANCE_MAP_REGISTER = 128;
		static const uint32_t IBL_ENVIRONMENT_MAP_REGISTER = 144;
		static const uint32_t MATERIAL_SAMPLERS_REGISTER = 512;
		static const uint32_t MATERIAL_TEXTURES_REGISTER = 1024;

		// ---------------------------------------------------------------------------------------------
		static const uint32_t INSTANCE_INDEX_CONSTANT_OFFSET = 0;
		static const uint32_t MATERIAL_INDEX_CONSTANT_OFFSET = 1;
		static const uint32_t IBL_INDEX_CONSTANT_OFFSET = 2;
		static const uint32_t IBL_LEVEL_COUNT_CONSTANT_OFFSET = 3;
		static const uint32_t DBG_VTX_ATTR_INDEX_CONSTANT_OFFSET = 4;

		// ---------------------------------------------------------------------------------------------

		// Maximum number of drawable instances
		static const uint32_t MAX_DRAWABLE_INSTANCES = 65536;

		// Required size of structs that map to HLSL
		static const uint32_t FRAME_PARAMS_STRUCT_SIZE = 8;
		static const uint32_t CAMERA_PARAMS_STRUCT_SIZE = 96;
		static const uint32_t INSTANCE_PARAMS_STRUCT_SIZE = 128;
		static const uint32_t MATERIAL_TEXTURE_PARAMS_STRUCT_SIZE = 28;
		static const uint32_t MATERIAL_PARAMS_STRUCT_SIZE = 164;

		// ---------------------------------------------------------------------------------------------

		MaterialPipelineArgs() = default;
		~MaterialPipelineArgs();

		static Result Create(vkr::RenderDevice* pDevice, scene::MaterialPipelineArgs** ppTargetPipelineArgs);

		vkr::DescriptorPool*      GetDescriptorPool() const { return mDescriptorPool.Get(); }
		vkr::DescriptorSetLayout* GetDescriptorSetLayout() const { return mDescriptorSetLayout.Get(); }
		vkr::DescriptorSet*       GetDescriptorSet() const { return mDescriptorSet.Get(); }

		scene::FrameParams*  GetFrameParams() { return mFrameParamsAddress; }
		scene::CameraParams* GetCameraParams() { return mCameraParamsAddress; }
		void                 SetCameraParams(const Camera* pCamera);

		scene::InstanceParams* GetInstanceParams(uint32_t index);
		scene::MaterialParams* GetMaterialParams(uint32_t index);

		void SetIBLTextures(
			uint32_t                index,
			vkr::SampledImageView* pIrradiance,
			vkr::SampledImageView* pEnvironment);

		void SetMaterialSampler(uint32_t index, const scene::Sampler* pSampler);
		void SetMaterialTexture(uint32_t index, const scene::Image* pImage);

		void CopyBuffers(vkr::CommandBuffer* pCmd);

	private:
		Result InitializeDefaultObjects(vkr::RenderDevice* pDevice);
		Result InitializeDescriptorSet(vkr::RenderDevice* pDevice);
		Result InitializeBuffers(vkr::RenderDevice* pDevice);
		Result SetDescriptors(vkr::RenderDevice* pDevice);
		Result InitializeResource(vkr::RenderDevice* pDevice);

	protected:
		vkr::DescriptorPoolPtr      mDescriptorPool;
		vkr::DescriptorSetLayoutPtr mDescriptorSetLayout;
		vkr::DescriptorSetPtr       mDescriptorSet;

		vkr::BufferPtr mCpuConstantParamsBuffer;
		vkr::BufferPtr mGpuConstantParamsBuffer;
		vkr::BufferPtr mCpuInstanceParamsBuffer;
		vkr::BufferPtr mGpuInstanceParamsBuffer;
		vkr::BufferPtr mCpuMateriaParamsBuffer;
		vkr::BufferPtr mGpuMateriaParamsBuffer;

		uint32_t mFrameParamsPaddedSize = 0;
		uint32_t mCameraParamsPaddedSize = 0;
		uint32_t mFrameParamsOffset = UINT32_MAX;
		uint32_t mCameraParamsOffset = UINT32_MAX;

		uint32_t mTotalConstantParamsPaddedSize = 0;
		uint32_t mTotalInstanceParamsPaddedSize = 0;
		uint32_t mTotalMaterialParamsPaddedSize = 0;

		char* mConstantParamsMappedAddress = nullptr;
		scene::FrameParams* mFrameParamsAddress = nullptr;
		scene::CameraParams* mCameraParamsAddress = nullptr;

		char* mInstanceParamsMappedAddress = nullptr;
		char* mMaterialParamsMappedAddress = nullptr;

		vkr::SamplerPtr mDefaultSampler; // Nearest, repeats
		vkr::TexturePtr mDefaultTexture; // Purple texture

		vkr::SamplerPtr mDefaultBRDFLUTSampler; // Linear, clamps to edge
		vkr::TexturePtr mDefaultBRDFLUTTexture;

		// Make samplers useable for most cases
		vkr::SamplerPtr mDefaultIBLIrradianceSampler;  // Linear, U repeats, V clamps to edge, mip 0 only
		vkr::SamplerPtr mDefaultIBLEnvironmentSampler; // Linear, U repeats, V clamps to edge
		vkr::TexturePtr mDefaultIBLTexture;            // White texture
	};

	void CopyMaterialTextureParams(
		const std::unordered_map<const scene::Sampler*, uint32_t>& samplersIndexMap,
		const std::unordered_map<const scene::Image*, uint32_t>& imagesIndexMap,
		const scene::TextureView& srcTextureView,
		scene::MaterialTextureParams& dstTextureParams);

} // namespace scene

#pragma endregion

//=============================================================================
#pragma region [ Scene Resource Manager ]

namespace scene
{
	// This class stores required objects for scenes and standalone meshes.
	// It also acts as a cache during scene loading to prevent loading of duplicate objects.
	//
	// The resource manager acts as the external owner of all shared resources for scenes and meshes. Required objects can be shared in a variety of different cases:
	//   - images and image views can be shared among textures
	//   - textures can be shared among materials by way of texture views
	//   - materials can be shared among primitive batches
	//   - mesh data can be shared among meshes
	//   - meshes can be shared among nodes
	//
	// Both scene::Scene and scene::Mesh will call ResourceManager::DestroyAll() in their destructors to destroy all its references to shared objects.
	// If afterwards a shared object has an external reference, the code holding the reference is responsible for the shared objct.
	class ResourceManager final
	{
	public:
		bool Find(uint64_t objectId, scene::SamplerPtr& outObject) const;
		bool Find(uint64_t objectId, scene::ImagePtr& outObject) const;
		bool Find(uint64_t objectId, scene::TexturePtr& outObject) const;
		bool Find(uint64_t objectId, scene::MaterialPtr& outObject) const;
		bool Find(uint64_t objectId, scene::MeshDataPtr& outObject) const;
		bool Find(uint64_t objectId, scene::MeshPtr& outObject) const;

		// Cache functions assumes ownership of pObject
		Result Cache(uint64_t objectId, const scene::SamplerPtr& object);
		Result Cache(uint64_t objectId, const scene::ImagePtr& object);
		Result Cache(uint64_t objectId, const scene::TexturePtr& object);
		Result Cache(uint64_t objectId, const scene::MaterialPtr& object);
		Result Cache(uint64_t objectId, const scene::MeshDataPtr& object);
		Result Cache(uint64_t objectId, const scene::MeshPtr& object);

		uint32_t GetSamplerCount() const { return static_cast<uint32_t>(mSamplers.size()); }
		uint32_t GetImageCount() const { return static_cast<uint32_t>(mImages.size()); }
		uint32_t GetTextureCount() const { return static_cast<uint32_t>(mTextures.size()); }
		uint32_t GetMaterialCount() const { return static_cast<uint32_t>(mMaterials.size()); }
		uint32_t GetMeshDataCount() const { return static_cast<uint32_t>(mMeshData.size()); }
		uint32_t GetMeshCount() const { return static_cast<uint32_t>(mMeshes.size()); }

		void DestroyAll();

		const std::unordered_map<uint64_t, scene::SamplerPtr>& GetSamplers() const;
		const std::unordered_map<uint64_t, scene::ImagePtr>& GetImages() const;
		const std::unordered_map<uint64_t, scene::TexturePtr>& GetTextures() const;
		const std::unordered_map<uint64_t, scene::MaterialPtr>& GetMaterials() const;

	private:
		template <typename ObjectT, typename ObjectRefT = std::shared_ptr<ObjectT>>
		bool FindObject(uint64_t objectId, 	const std::unordered_map<uint64_t, ObjectRefT>& container, ObjectRefT& outObject) const
		{
			auto it = container.find(objectId);
			if (it == container.end()) { return false; }
			outObject = (*it).second;
			return true;
		}

		template <typename ObjectT, typename ObjectRefT = std::shared_ptr<ObjectT>>
		Result CacheObject(uint64_t objectId, const ObjectRefT& object, std::unordered_map<uint64_t, ObjectRefT>& container)
		{
			// Don't cache NULL objects
			if (!object) return ERROR_UNEXPECTED_NULL_ARGUMENT;

			auto it = container.find(objectId);
			if (it != container.end()) return ERROR_DUPLICATE_ELEMENT;

			container[objectId] = object;

			return SUCCESS;
		}

	private:
		std::unordered_map<uint64_t, scene::SamplerPtr>  mSamplers;
		std::unordered_map<uint64_t, scene::ImagePtr>    mImages;
		std::unordered_map<uint64_t, scene::TexturePtr>  mTextures;
		std::unordered_map<uint64_t, scene::MaterialPtr> mMaterials;
		std::unordered_map<uint64_t, scene::MeshDataPtr> mMeshData;
		std::unordered_map<uint64_t, scene::MeshPtr>     mMeshes;
	};

} // namespace scene

#pragma endregion

//=============================================================================
#pragma region [ Scene Mesh ]

namespace scene
{

	// Container for geometry data and the buffer views required by a renderer. scene::MeshData objects can be shared among different scene::Mesh instances.
	// It's necessary to separate out the mesh data from the mesh since it's possible for a series of meshes to use the same geometry data but a different set of scene::PrimitiveBatch descriptions.
	class MeshData final : public vkr::NamedObjectTrait
	{
	public:
		MeshData(const scene::VertexAttributeFlags& availableVertexAttributes, vkr::Buffer* pGpuBuffer);
		~MeshData();

		const scene::VertexAttributeFlags& GetAvailableVertexAttributes() const { return mAvailableVertexAttributes; }
		const std::vector<vkr::VertexBinding>& GetAvailableVertexBindings() const { return mVertexBindings; }
		vkr::Buffer* GetGpuBuffer() const { return mGpuBuffer.Get(); }

	private:
		scene::VertexAttributeFlags      mAvailableVertexAttributes = {};
		std::vector<vkr::VertexBinding> mVertexBindings;
		vkr::BufferPtr                  mGpuBuffer;
	};

	// Contains all information necessary for a draw call. The material reference will determine which pipeline gets used. The offsets and counts correspond to the graphics API's draw call. Bounding box can be used by renderer.
	class PrimitiveBatch final
	{
	public:
		PrimitiveBatch() = default;
		PrimitiveBatch(
			const scene::MaterialPtr& material,
			const vkr::IndexBufferView& indexBufferView,
			const vkr::VertexBufferView& positionBufferView,
			const vkr::VertexBufferView& attributeBufferView,
			uint32_t                      indexCount,
			uint32_t                      vertexCount,
			const AABB& boundingBox);

		const scene::Material* GetMaterial() const { return mMaterial.get(); }
		const vkr::IndexBufferView& GetIndexBufferView() const { return mIndexBufferView; }
		const vkr::VertexBufferView& GetPositionBufferView() const { return mPositionBufferView; }
		const vkr::VertexBufferView& GetAttributeBufferView() const { return mAttributeBufferView; }
		AABB                     GetBoundingBox() const { return mBoundingBox; }
		uint32_t                      GetIndexCount() const { return mIndexCount; }
		uint32_t                      GetVertexCount() const { return mVertexCount; }

	private:
		scene::MaterialPtr     mMaterial = nullptr;
		vkr::IndexBufferView  mIndexBufferView = {};
		vkr::VertexBufferView mPositionBufferView = {};
		vkr::VertexBufferView mAttributeBufferView = {};
		uint32_t               mIndexCount = 0;
		uint32_t               mVertexCount = 0;
		AABB              mBoundingBox = {};
	};

	// Contains all the information necessary to render a model:
	//   - geometry data reference
	//   - primitive batches
	//   - material references
	// If a mesh is loaded standalone, it will use its own resource manager for required materials, textures, images, and samplers.
	// If a mesh is loaded as part of a scene, the scene's resource manager will be used instead.
	class Mesh final : public vkr::NamedObjectTrait
	{
	public:
		Mesh(
			const scene::MeshDataPtr& meshData,
			std::vector<scene::PrimitiveBatch>&& batches);
		Mesh(
			std::unique_ptr<scene::ResourceManager>&& resourceManager,
			const scene::MeshDataPtr& meshData,
			std::vector<scene::PrimitiveBatch>&& batches);
		~Mesh();

		bool HasResourceManager() const { return mResourceManager ? true : false; }

		scene::VertexAttributeFlags      GetAvailableVertexAttributes() const;
		std::vector<vkr::VertexBinding> GetAvailableVertexBindings() const;

		scene::MeshData* GetMeshData() const { return mMeshData.get(); }
		const std::vector<scene::PrimitiveBatch>& GetBatches() const { return mBatches; }

		void AddBatch(const scene::PrimitiveBatch& batch);

		const AABB& GetBoundingBox() const { return mBoundingBox; }
		void             UpdateBoundingBox();

		std::vector<const scene::Material*> GetMaterials() const;

	private:
		std::unique_ptr<scene::ResourceManager> mResourceManager = nullptr;
		scene::MeshDataPtr                      mMeshData = nullptr;
		std::vector<scene::PrimitiveBatch>      mBatches = {};
		AABB                               mBoundingBox = {};
	};

} // namespace scene

#pragma endregion

//=============================================================================
#pragma region [ Scene ]

namespace scene
{

	// Map a resource pointer to an index, used for indexing resource on GPU.
	template <typename ResObjT>
	using ResourceIndexMap = std::unordered_map<const ResObjT*, uint32_t>;

	// Basic scene graph designed to feed into a renderer. Manages nodes and all their required objects: meshes, mesh geometry data, materials, textures, images, and samplers.
	// See scene::ResourceManager for details on object sharing among the various elements of the scene.
	class Scene final : public vkr::NamedObjectTrait
	{
	public:
		Scene(std::unique_ptr<scene::ResourceManager>&& resourceManager);

		// Returns the number of all samplers in the scene
		uint32_t GetSamplerCount() const;
		// Returns the number of all images in the scene
		uint32_t GetImageCount() const;
		// Returns the number of all textures in the scene
		uint32_t GetTextureCount() const;
		// Returns the number of all materials in the scene
		uint32_t GetMaterialCount() const;
		// Returns the number of all mesh data objects in the scene
		uint32_t GetMeshDataCount() const;
		// Returns the number of all meshes in the scene
		uint32_t GetMeshCount() const;

		// Returns the number of all the nodes in the scene
		uint32_t GetNodeCount() const { return CountU32(mNodes); }
		// Returns the number of mesh nodes in the scene
		uint32_t GetMeshNodeCount() const { return CountU32(mMeshNodes); }
		// Returns the number of camera nodes in the scene
		uint32_t GetCameraNodeCount() const { return CountU32(mCameraNodes); }
		// Returns the number of light nodes in the scene
		uint32_t GetLightNodeCount() const { return CountU32(mLightNodes); }

		// Returns the node at index or NULL if idx is out of range
		scene::Node* GetNode(uint32_t index) const;
		// Returns the mesh node at index or NULL if idx is out of range
		scene::MeshNode* GetMeshNode(uint32_t index) const;
		// Returns the camera node at index or NULL if idx is out of range
		scene::CameraNode* GetCameraNode(uint32_t index) const;
		// Returns the light node at index or NULL if idx is out of range
		scene::LightNode* GetLightNode(uint32_t index) const;

		// ---------------------------------------------------------------------------------------------
		// Find*Node functions return the first node that matches the name argument.
		// Since it's possible for source data to have multiple nodes of the same name, there can't be any type of ordering or search consistency guarantees.
		// Best to avoid using the same name for multiple nodes in source data.
		// ---------------------------------------------------------------------------------------------
		// Returns a node that matches name
		scene::Node* FindNode(const std::string& name) const;
		// Returns a mesh node that matches name
		scene::MeshNode* FindMeshNode(const std::string& name) const;
		// Returns a camera node that matches name
		scene::CameraNode* FindCameraNode(const std::string& name) const;
		// Returns a light node that matches name
		scene::LightNode* FindLightNode(const std::string& name) const;

		Result AddNode(scene::NodePtr&& node);

		// ---------------------------------------------------------------------------------------------
		// Get*ArrayIndexMap functions are used when populating resource and parameter arguments for the shader. The return value of these functions are two parts:
		//   - the first is an array of resources from the resource manager
		//   - the second is a map of the resource to its array index
		// These two parts are used to build textures, sampler, and material resources arrays for the shader.
		// ---------------------------------------------------------------------------------------------
		// Returns an array of samplers and their index mappings
		scene::ResourceIndexMap<scene::Sampler> GetSamplersArrayIndexMap() const;
		// Returns an array of images and their index mappings
		scene::ResourceIndexMap<scene::Image> GetImagesArrayIndexMap() const;
		// Returns an array of materials and their index mappings
		scene::ResourceIndexMap<scene::Material> GetMaterialsArrayIndexMap() const;

	private:
		template <typename NodeT>
		NodeT* FindNodeByName(const std::string& name, const std::vector<NodeT*>& container) const
		{
			auto it = FindIf(
				container,
				[name](const NodeT* pElem) {
					bool match = (pElem->GetName() == name);
					return match; });
			if (it == container.end()) {
				return nullptr;
			}

			NodeT* pNode = (*it);
			return pNode;
		}

	private:
		std::unique_ptr<scene::ResourceManager> mResourceManager = nullptr;
		std::vector<scene::NodePtr>             mNodes = {};
		std::vector<scene::MeshNode*>           mMeshNodes = {};
		std::vector<scene::CameraNode*>         mCameraNodes = {};
		std::vector<scene::LightNode*>          mLightNodes = {};
	};

} // namespace scene
#pragma endregion

//=============================================================================
#pragma region [ Scene Loader ]

namespace scene
{
	// Stores optional paramters that are passed to scene loader implementations.
	class LoadOptions final
	{
	public:
		// Returns current material factory or NULL if one has not been set.
		scene::MaterialFactory* GetMaterialFactory() const { return mMaterialFactory; }

		// Sets material factory used to create materials during loading.
		LoadOptions& SetMaterialFactory(scene::MaterialFactory* pMaterialFactory)
		{
			mMaterialFactory = pMaterialFactory;
			return *this;
		}

		// Returns true if the calling application requires a specific set of attrbutes
		bool HasRequiredVertexAttributes() const { return (mRequiredVertexAttributes.mask != 0); }

		// Returns the attributes that the calling application rquires or none if attributes has not been set.
		const scene::VertexAttributeFlags& GetRequiredAttributes() const { return mRequiredVertexAttributes; }

		// Set attributes required by calling allication.
		LoadOptions& SetRequiredAttributes(const scene::VertexAttributeFlags& attributes)
		{
			mRequiredVertexAttributes = attributes;
			return *this;
		}

		// Clears required attributes (sets required attributs to none)
		void ClearRequiredAttributes() { SetRequiredAttributes(scene::VertexAttributeFlags::None()); }

	private:
		// Pointer to custom material factory for loader to use.
		scene::MaterialFactory* mMaterialFactory = nullptr;

		// Required attributes for meshes nodes, meshes - this should override
		// whatever a loader is using to determine which vertex attributes to laod.
		// If the source data doesn't provide data for an attribute, an sensible
		// default value is used - usually zeroes.
		//
		scene::VertexAttributeFlags mRequiredVertexAttributes = scene::VertexAttributeFlags::None();
	};

} // namespace scene

#pragma endregion

//=============================================================================
#pragma region [ Scene gltf Loader ]

#if defined(WIN32) && defined(LoadImage)
#define _SAVE_MACRO_LoadImage LoadImage
#undef LoadImage
#endif

namespace scene {

	class GltfMaterialSelector
	{
	public:
		GltfMaterialSelector();
		virtual ~GltfMaterialSelector();

		virtual std::string DetermineMaterial(const cgltf_material* pGltfMaterial) const;
	};

	class GltfLoader
	{
	private:
		// Make non-copyable
		GltfLoader(const GltfLoader&) = delete;
		GltfLoader& operator=(const GltfLoader&) = delete;

	protected:
		GltfLoader(
			const std::filesystem::path& filePath,
			const std::filesystem::path& textureDirPath,
			cgltf_data* pGltfData,
			bool                         ownsGtlfData,
			scene::GltfMaterialSelector* pMaterialSelector,
			bool                         ownsMaterialSelector);

	public:
		~GltfLoader();

		// Calling code is responsbile for lifetime of pMaterialSeelctor
		static Result Create(
			const std::filesystem::path& filePath,
			const std::filesystem::path& textureDirPath,
			scene::GltfMaterialSelector* pMaterialSelector, // Use null for default selector
			GltfLoader** ppLoader);

		// Calling code is responsbile for lifetime of pMaterialSeelctor
		static Result Create(
			const std::filesystem::path& path,
			scene::GltfMaterialSelector* pMaterialSelector, // Use null for default selector
			GltfLoader** ppLoader);

		cgltf_data* GetGltfData() const { return mGltfData; }
		bool        HasGltfData() const { return (mGltfData != nullptr); }

		uint32_t GetSamplerCount() const;
		uint32_t GetImageCount() const;
		uint32_t GetTextureCount() const;
		uint32_t GetMaterialCount() const;
		uint32_t GetMeshCount() const;
		uint32_t GetNodeCount() const;
		uint32_t GetSceneCount() const;

		// These functions return a -1 if the object cannot be located by name.
		int32_t GetSamplerIndex(const std::string& name) const;
		int32_t GetImageIndex(const std::string& name) const;
		int32_t GetTextureIndex(const std::string& name) const;
		int32_t GetMaterialIndex(const std::string& name) const;
		int32_t GetMeshIndex(const std::string& name) const;
		int32_t GetNodeIndex(const std::string& name) const;
		int32_t GetSceneIndex(const std::string& name) const;

		// ---------------------------------------------------------------------------------------------
		// Loads a GLTF sampler, image, texture, or material
		//
		// These load functions create standalone objects that can be used
		// outside of a scene. Caching is not used.
		//
		// ---------------------------------------------------------------------------------------------
		Result LoadSampler(
			vkr::RenderDevice* pDevice,
			uint32_t         samplerIndex,
			scene::Sampler** ppTargetSampler);

		Result LoadSampler(
			vkr::RenderDevice* pDevice,
			const std::string& samplerName,
			scene::Sampler** ppTargetSampler);

		Result LoadImage(
			vkr::RenderDevice* pDevice,
			uint32_t       imageIndex,
			scene::Image** ppTargetImage);

		Result LoadImage(
			vkr::RenderDevice* pDevice,
			const std::string& imageName,
			scene::Image** ppTargetImage);

		Result LoadTexture(
			vkr::RenderDevice* pDevice,
			uint32_t         textureIndex,
			scene::Texture** ppTargetTexture);

		Result LoadTexture(
			vkr::RenderDevice* pDevice,
			const std::string& textureName,
			scene::Texture** ppTargetTexture);

		Result LoadMaterial(
			vkr::RenderDevice* pDevice,
			uint32_t                  materialIndex,
			scene::Material** ppTargetMaterial,
			const scene::LoadOptions& loadOptions = scene::LoadOptions());

		Result LoadMaterial(
			vkr::RenderDevice* pDevice,
			const std::string& materialName,
			scene::Material** ppTargetMaterial,
			const scene::LoadOptions& loadOptions = scene::LoadOptions());

		// ---------------------------------------------------------------------------------------------
		// Loads a GLTF mesh
		//
		// This function creates a standalone mesh that usable outside of a scene.
		//
		// Standalone meshes uses an internal scene::ResourceManager to
		// manage its required objects. All required objects created as apart
		// of the mesh loading will be managed by the mesh. The reasoning
		// for this is because images, textures, and materials can be shared
		// between primitive batches.
		//
		// Object sharing requires that the lifetime of an object to be managed
		// by an external mechanism, scene::ResourceManager. When a mesh is
		// destroyed, its destructor calls scene::ResourceManager::DestroyAll
		// to destroy its reference to the shared objects. If afterwards a shared
		// object has an external reference, the code holding the reference
		// is responsible for the shared objct.
		//
		// activeVertexAttributes - specifies the attributes that the a mesh
		// should be created with. If a GLTF file doesn't provide data for
		// an attribute, an sensible default value is used - usually zeroes.
		//
		// ---------------------------------------------------------------------------------------------
		Result LoadMesh(
			vkr::RenderDevice* pDevice,
			uint32_t                  meshIndex,
			scene::Mesh** ppTargetMesh,
			const scene::LoadOptions& loadOptions = scene::LoadOptions());

		Result LoadMesh(
			vkr::RenderDevice* pDevice,
			const std::string& meshName,
			scene::Mesh** ppTargetMesh,
			const scene::LoadOptions& loadOptions = scene::LoadOptions());

		// ---------------------------------------------------------------------------------------------
		// Loads a GLTF node
		// ---------------------------------------------------------------------------------------------
		Result LoadNode(
			vkr::RenderDevice* pDevice,
			uint32_t                  nodeIndex,
			scene::Node** ppTargetNode,
			const scene::LoadOptions& loadOptions = scene::LoadOptions());

		Result LoadNode(
			vkr::RenderDevice* pDevice,
			const std::string& nodeName,
			scene::Node** ppTargetNode,
			const scene::LoadOptions& loadOptions = scene::LoadOptions());

		Result LoadNodeTransformOnly(
			uint32_t      nodeIndex,
			scene::Node** ppTargetNode);

		Result LoadNodeTransformOnly(
			const std::string& nodeName,
			scene::Node** ppTargetNode);

		// ---------------------------------------------------------------------------------------------
		// Loads a GLTF scene
		//
		// @TODO: Figure out a way to load more than one GLTF scene into a
		//        a target scene object without cache stomping.
		//
		// ---------------------------------------------------------------------------------------------
		Result LoadScene(
			vkr::RenderDevice* pDevice,
			uint32_t                  sceneIndex, // Use 0 if unsure
			scene::Scene** ppTargetScene,
			const scene::LoadOptions& loadOptions = scene::LoadOptions());

		Result LoadScene(
			vkr::RenderDevice* pDevice,
			const std::string& sceneName,
			scene::Scene** ppTargetScene,
			const scene::LoadOptions& loadOptions = scene::LoadOptions());

	private:
		// Stores a look up of a vertex attribute mask that is comprised of all
		// the required vertex attributes in a meshes materials in a given
		// material factory.
		//
		// For example:
		//   If a mesh has 2 primitives that uses 2 different materials, A and B:
		//     - material A requires only tex-coords
		//     - material B requires normals and tex-coords
		//   Then the mask for the mesh would be an OR of material A's material flags
		//   and material B's required flags, resulting in:
		//     mask.texCoords = true;
		//     mask.normals   = true;
		//     mask.tangetns  = false;
		//     mask.colors    = false;
		//
		// This masks enables the loader to select the different vertex attributes
		// required by a meshes mesh data so that it doesn't have to generically
		// load all attributes if the mesh data is shared between multiple meshes.
		//
		using MeshMaterialVertexAttributeMasks = std::unordered_map<const cgltf_mesh*, scene::VertexAttributeFlags>;

		void CalculateMeshMaterialVertexAttributeMasks(
			const scene::MaterialFactory* pMaterialFactory,
			GltfLoader::MeshMaterialVertexAttributeMasks* pOutMasks) const;

		struct InternalLoadParams
		{
			vkr::RenderDevice* pDevice = nullptr;
			scene::MaterialFactory* pMaterialFactory = nullptr;
			scene::VertexAttributeFlags       requiredVertexAttributes = scene::VertexAttributeFlags::None();
			scene::ResourceManager* pResourceManager = nullptr;
			MeshMaterialVertexAttributeMasks* pMeshMaterialVertexAttributeMasks = nullptr;
			bool                              transformOnly = false;
			scene::Scene* pTargetScene = nullptr;

			struct
			{
				uint64_t image = 0;
				uint64_t sampler = 0;
				uint64_t texture = 0;
				uint64_t material = 0;
				uint64_t mesh = 0;
			} baseObjectIds;
		};

		// To avoid potential cache collision when loading multiple GLTF files into the
		// same scene we need to apply an offset (base object id) to the object index
		// so that the final object id is unique.
		//
		// These functions calculate the object id used when caching the resource.
		//
		uint64_t CalculateImageObjectId(const GltfLoader::InternalLoadParams& loadParams, uint32_t objectIndex);
		uint64_t CalculateSamplerObjectId(const GltfLoader::InternalLoadParams& loadParams, uint32_t objectIndex);
		uint64_t CalculateTextureObjectId(const GltfLoader::InternalLoadParams& loadParams, uint32_t objectIndex);
		uint64_t CalculateMaterialObjectId(const GltfLoader::InternalLoadParams& loadParams, uint32_t objectIndex);
		uint64_t CalculateMeshObjectId(const GltfLoader::InternalLoadParams& loadParams, uint32_t objectIndexk);

		Result LoadSamplerInternal(
			const GltfLoader::InternalLoadParams& loadParams,
			const cgltf_sampler* pGltfSampler,
			scene::Sampler** ppTargetSampler);

		Result FetchSamplerInternal(
			const GltfLoader::InternalLoadParams& loadParams,
			const cgltf_sampler* pGltfSampler,
			scene::SamplerPtr& outSampler);

		Result LoadImageInternal(
			const GltfLoader::InternalLoadParams& loadParams,
			const cgltf_image* pGltfImage,
			scene::Image** ppTargetImage);

		Result FetchImageInternal(
			const GltfLoader::InternalLoadParams& loadParams,
			const cgltf_image* pGltfImage,
			scene::ImagePtr& outImage);

		Result LoadTextureInternal(
			const GltfLoader::InternalLoadParams& loadParams,
			const cgltf_texture* pGltfTexture,
			scene::Texture** pptTexture);

		Result FetchTextureInternal(
			const GltfLoader::InternalLoadParams& loadParams,
			const cgltf_texture* pGltfTexture,
			scene::TexturePtr& outTexture);

		Result LoadTextureViewInternal(
			const GltfLoader::InternalLoadParams& loadParams,
			const cgltf_texture_view* pGltfTextureView,
			scene::TextureView* pTargetTextureView);

		Result LoadUnlitMaterialInternal(
			const GltfLoader::InternalLoadParams& loadParams,
			const cgltf_material* pGltfMaterial,
			scene::UnlitMaterial* pTargetMaterial);

		Result LoadPbrMetallicRoughnessMaterialInternal(
			const GltfLoader::InternalLoadParams& loadParams,
			const cgltf_material* pGltfMaterial,
			scene::StandardMaterial* pTargetMaterial);

		Result LoadMaterialInternal(
			const GltfLoader::InternalLoadParams& loadParams,
			const cgltf_material* pGltfMaterial,
			scene::Material** ppTargetMaterial);

		Result FetchMaterialInternal(
			const GltfLoader::InternalLoadParams& loadParams,
			const cgltf_material* pGltfMaterial,
			scene::MaterialPtr& outMaterial);

		Result LoadMeshData(
			const GltfLoader::InternalLoadParams& extgernalLoadParams,
			const cgltf_mesh* pGltfMesh,
			scene::MeshDataPtr& outMeshData,
			std::vector<scene::PrimitiveBatch>& outBatches);

		Result LoadMeshInternal(
			const GltfLoader::InternalLoadParams& loadParams,
			const cgltf_mesh* pGltfMesh,
			scene::Mesh** ppTargetMesh);

		Result FetchMeshInternal(
			const GltfLoader::InternalLoadParams& loadParams,
			const cgltf_mesh* pGltfMesh,
			scene::MeshPtr& outMesh);

		Result LoadNodeInternal(
			const GltfLoader::InternalLoadParams& loadParams,
			const cgltf_node* pGltfNode,
			scene::Node** ppTargetNode);

		Result FetchNodeInternal(
			const GltfLoader::InternalLoadParams& loadParams,
			const cgltf_node* pGltfNode,
			scene::NodePtr& outNode);

		Result LoadSceneInternal(
			const GltfLoader::InternalLoadParams& externalLoadParams,
			const cgltf_scene* pGltfScene,
			scene::Scene* pTargetScene);

	private:
		// Builds a set of node indices that include pGltfNode and all its children.
		void GetUniqueGltfNodeIndices(
			const cgltf_node* pGltfNode,
			std::set<cgltf_size>& uniqueGltfNodeIndices) const;

	private:
		std::filesystem::path        mGltfFilePath = "";
		std::filesystem::path        mGltfTextureDir = ""; // This might be different than the parent dir of mGltfFilePath
		cgltf_data* mGltfData = nullptr;
		bool                         mOwnsGltfData = false;
		scene::GltfMaterialSelector* mMaterialSelector = nullptr;
		bool                         mOwnsMaterialSelector = false;
		scene::MaterialFactory       mDefaultMaterialFactory = {};
	};

} // namespace scene

#if defined(WIN32) && defined(LoadImage)
#define LoadImage _SAVE_MACRO_LoadImage
#endif

#pragma endregion
