#pragma once

class Example_019 final : public EngineApplication
{
public:
	EngineApplicationCreateInfo Config() const final;
	bool Setup() final;
	void Shutdown() final;
	void Update() final;
	void Render() final;

	struct PerFrame
	{
		vkr::CommandBufferPtr cmd;
		vkr::SemaphorePtr     imageAcquiredSemaphore;
		vkr::FencePtr         imageAcquiredFence;
		vkr::SemaphorePtr     renderCompleteSemaphore;
		vkr::FencePtr         renderCompleteFence;
	};

	struct Texture
	{
		vkr::ImagePtr            pImage;
		vkr::SampledImageViewPtr pTexture;
		vkr::SamplerPtr          pSampler;
	};

	struct Material
	{
		vkr::PipelineInterfacePtr pInterface;
		vkr::GraphicsPipelinePtr  pPipeline;
		vkr::DescriptorSetPtr     pDescriptorSet;
		std::vector<Texture>       textures;
	};

	struct Primitive
	{
		vkr::Mesh* mesh;
	};

	struct Renderable
	{
		Material* pMaterial;
		Primitive* pPrimitive;
		vkr::DescriptorSet* pDescriptorSet;

		Renderable(Material* m, Primitive* p, vkr::DescriptorSet* set)
			: pMaterial(m), pPrimitive(p), pDescriptorSet(set) {
		}
	};

	struct Object
	{
		float4x4                modelMatrix;
		float4x4                ITModelMatrix;
		vkr::BufferPtr         pUniformBuffer;
		std::vector<Renderable> renderables;
	};

	using RenderList = std::unordered_map<Material*, std::vector<Object*>>;
	using TextureCache = std::unordered_map<std::string, vkr::ImagePtr>;

private:
	std::vector<PerFrame>        mPerFrame;
	vkr::DescriptorPoolPtr      mDescriptorPool;
	vkr::DescriptorSetLayoutPtr mSetLayout;
	vkr::ShaderModulePtr        mVertexShader;
	vkr::ShaderModulePtr        mPbrPixelShader;
	vkr::ShaderModulePtr        mUnlitPixelShader;
	PerspCamera                  mCamera;
	float3                       mLightPosition = float3(10, 100, 10);

	std::vector<Material>  mMaterials;
	std::vector<Primitive> mPrimitives;
	std::vector<Object>    mObjects;
	TextureCache           mTextureCache;

	void loadScene(
		const std::filesystem::path& filename,
		vkr::Queue* pQueue,
		vkr::DescriptorPool* pDescriptorPool,
		TextureCache* pTextureCache,
		std::vector<Object>* pObjects,
		std::vector<Primitive>* pPrimitives,
		std::vector<Material>* pMaterials);

	void loadMaterial(
		const std::filesystem::path& gltfFolder,
		const cgltf_material& material,
		vkr::Queue* pQueue,
		vkr::DescriptorPool* pDescriptorPool,
		TextureCache* pTextureCache,
		Material* pOutput);

	void loadTexture(
		const std::filesystem::path& gltfFolder,
		const cgltf_texture_view& textureView,
		vkr::Queue* pQueue,
		TextureCache* pTextureCache,
		Texture* pOutput);

	void loadTexture(
		const vkr::Bitmap& bitmap,
		vkr::Queue* pQueue,
		Texture* pOutput);

	// Load the given primitive to the GPU.
	// `pStagingBuffer` must already contain all data referenced by `primitive`.
	void loadPrimitive(
		const cgltf_primitive& primitive,
		vkr::BufferPtr        pStagingBuffer,
		vkr::Queue* pQueue,
		Primitive* pOutput);

	void loadNodes(
		const cgltf_data* data,
		vkr::Queue* pQueue,
		vkr::DescriptorPool* pDescriptorPool,
		std::vector<Object>* objects,
		const std::unordered_map<const cgltf_primitive*, size_t>& primitiveToIndex,
		std::vector<Primitive>* pPrimitives,
		std::vector<Material>* pMaterials);
};