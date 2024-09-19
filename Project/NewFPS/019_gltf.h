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
		CommandBufferPtr cmd;
		SemaphorePtr     imageAcquiredSemaphore;
		FencePtr         imageAcquiredFence;
		SemaphorePtr     renderCompleteSemaphore;
		FencePtr         renderCompleteFence;
	};

	struct Texture
	{
		ImagePtr            pImage;
		SampledImageViewPtr pTexture;
		SamplerPtr          pSampler;
	};

	struct Material
	{
		PipelineInterfacePtr pInterface;
		GraphicsPipelinePtr  pPipeline;
		DescriptorSetPtr     pDescriptorSet;
		std::vector<Texture>       textures;
	};

	struct Primitive
	{
		Mesh* mesh;
	};

	struct Renderable
	{
		Material* pMaterial;
		Primitive* pPrimitive;
		DescriptorSet* pDescriptorSet;

		Renderable(Material* m, Primitive* p, DescriptorSet* set)
			: pMaterial(m), pPrimitive(p), pDescriptorSet(set) {
		}
	};

	struct Object
	{
		float4x4                modelMatrix;
		float4x4                ITModelMatrix;
		BufferPtr         pUniformBuffer;
		std::vector<Renderable> renderables;
	};

	using RenderList = std::unordered_map<Material*, std::vector<Object*>>;
	using TextureCache = std::unordered_map<std::string, ImagePtr>;

private:
	std::vector<PerFrame>        mPerFrame;
	DescriptorPoolPtr      mDescriptorPool;
	DescriptorSetLayoutPtr mSetLayout;
	ShaderModulePtr        mVertexShader;
	ShaderModulePtr        mPbrPixelShader;
	ShaderModulePtr        mUnlitPixelShader;
	PerspCamera                  mCamera;
	float3                       mLightPosition = float3(10, 100, 10);

	std::vector<Material>  mMaterials;
	std::vector<Primitive> mPrimitives;
	std::vector<Object>    mObjects;
	TextureCache           mTextureCache;

	void loadScene(
		const std::filesystem::path& filename,
		Queue* pQueue,
		DescriptorPool* pDescriptorPool,
		TextureCache* pTextureCache,
		std::vector<Object>* pObjects,
		std::vector<Primitive>* pPrimitives,
		std::vector<Material>* pMaterials);

	void loadMaterial(
		const std::filesystem::path& gltfFolder,
		const cgltf_material& material,
		Queue* pQueue,
		DescriptorPool* pDescriptorPool,
		TextureCache* pTextureCache,
		Material* pOutput);

	void loadTexture(
		const std::filesystem::path& gltfFolder,
		const cgltf_texture_view& textureView,
		Queue* pQueue,
		TextureCache* pTextureCache,
		Texture* pOutput);

	void loadTexture(
		const Bitmap& bitmap,
		Queue* pQueue,
		Texture* pOutput);

	// Load the given primitive to the GPU.
	// `pStagingBuffer` must already contain all data referenced by `primitive`.
	void loadPrimitive(
		const cgltf_primitive& primitive,
		BufferPtr        pStagingBuffer,
		Queue* pQueue,
		Primitive* pOutput);

	void loadNodes(
		const cgltf_data* data,
		Queue* pQueue,
		DescriptorPool* pDescriptorPool,
		std::vector<Object>* objects,
		const std::unordered_map<const cgltf_primitive*, size_t>& primitiveToIndex,
		std::vector<Primitive>* pPrimitives,
		std::vector<Material>* pMaterials);
};