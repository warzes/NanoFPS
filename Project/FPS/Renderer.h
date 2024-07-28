#pragma once

#include "VulkanRender.h"
#include "EngineMath.h"

#pragma region DeferredFramebuffer

class DeferredFramebuffer final
{
public:
	DeferredFramebuffer() = default;
	DeferredFramebuffer(
		VulkanRender* device,
		vk::RenderPass                                 renderPass,
		vk::DescriptorSetLayout                        textureSetLayout,
		vk::Sampler                                    sampler,
		const vk::Extent2D& extent,
		const vk::ArrayProxyNoTemporaries<vk::Format>& colorFormats
	);
	DeferredFramebuffer(const DeferredFramebuffer&) = delete;
	DeferredFramebuffer(DeferredFramebuffer&& other) noexcept { Swap(other); }
	~DeferredFramebuffer() { Release(); }

	DeferredFramebuffer& operator=(const DeferredFramebuffer&) = delete;
	DeferredFramebuffer& operator=(DeferredFramebuffer&& other) noexcept
	{
		if (this != &other)
		{
			Release();
			Swap(other);
		}
		return *this;
	}

	void Release();

	void Swap(DeferredFramebuffer& other) noexcept;

	[[nodiscard]] const vk::Framebuffer& GetFramebuffer() const { return m_framebuffer; }
	[[nodiscard]] const vk::DescriptorSet& GetTextureSet() const { return m_textureSet; }
	[[nodiscard]] const vk::ImageView& GetDepthAttachmentView() const { return m_depthAttachmentView; }

private:
	void CreateAttachments(const vk::Extent2D& extent, const vk::ArrayProxyNoTemporaries<vk::Format>& colorFormats);
	void CreateAttachmentViews(const vk::ArrayProxyNoTemporaries<vk::Format>& colorFormats);

	VulkanRender* m_device = nullptr;

	std::vector<VulkanImage>   m_colorAttachments;
	std::vector<vk::ImageView> m_colorAttachmentViews;
	VulkanImage                m_depthAttachment;
	vk::ImageView              m_depthAttachmentView;
	vk::Framebuffer            m_framebuffer;
	vk::DescriptorSet          m_textureSet;
};

#pragma endregion

#pragma region ForwardFramebuffer

class ForwardFramebuffer final
{
public:
	ForwardFramebuffer() = default;
	ForwardFramebuffer(
		VulkanRender* device,
		vk::RenderPass          renderPass,
		vk::DescriptorSetLayout textureSetLayout,
		vk::Sampler             sampler,
		const vk::Extent2D& extent,
		vk::ImageView           depthImageView
	);
	ForwardFramebuffer(const ForwardFramebuffer&) = delete;
	ForwardFramebuffer(ForwardFramebuffer&& other) noexcept { Swap(other); }
	~ForwardFramebuffer() { Release(); }

	ForwardFramebuffer& operator=(const ForwardFramebuffer&) = delete;
	ForwardFramebuffer& operator=(ForwardFramebuffer&& other) noexcept
	{
		if (this != &other)
		{
			Release();
			Swap(other);
		}
		return *this;
	}

	void Release();

	void Swap(ForwardFramebuffer& other) noexcept;

	[[nodiscard]] const vk::Framebuffer& GetFramebuffer() const { return m_framebuffer; }
	[[nodiscard]] const vk::DescriptorSet& GetTextureSet() const { return m_textureSet; }

private:
	void CreateAttachments(const vk::Extent2D& extent);
	void CreateAttachmentViews();

	VulkanRender* m_device = nullptr;

	VulkanImage       m_colorAttachment;
	vk::ImageView     m_colorAttachmentView;
	vk::Framebuffer   m_framebuffer;
	vk::DescriptorSet m_textureSet;
};

#pragma endregion

#pragma region PostProcessingFramebuffer

class PostProcessingFramebuffer final
{
public:
	PostProcessingFramebuffer() = default;
	PostProcessingFramebuffer(
		VulkanRender* device,
		vk::RenderPass          renderPass,
		vk::DescriptorSetLayout textureSetLayout,
		vk::Sampler             sampler,
		const vk::Extent2D& extent,
		vk::Format              format = vk::Format::eB8G8R8A8Unorm
	);
	PostProcessingFramebuffer(const PostProcessingFramebuffer&) = delete;
	PostProcessingFramebuffer(PostProcessingFramebuffer&& other) noexcept { Swap(other); }
	~PostProcessingFramebuffer() { Release(); }

	PostProcessingFramebuffer& operator=(const PostProcessingFramebuffer&) = delete;
	PostProcessingFramebuffer& operator=(PostProcessingFramebuffer&& other) noexcept
	{
		if (this != &other)
		{
			Release();
			Swap(other);
		}
		return *this;
	}

	void Release();

	void Swap(PostProcessingFramebuffer& other) noexcept;

	[[nodiscard]] const vk::Framebuffer& GetFramebuffer() const { return m_framebuffer; }
	[[nodiscard]] const vk::DescriptorSet& GetTextureSet() const { return m_textureSet; }

private:
	void CreateAttachments(const vk::Extent2D& extent);
	void CreateAttachmentViews();

	VulkanRender* m_device = nullptr;

	vk::Format        m_format = vk::Format::eUndefined;
	VulkanImage       m_colorAttachment;
	vk::ImageView     m_colorAttachmentView;
	vk::Framebuffer   m_framebuffer;
	vk::DescriptorSet m_textureSet;
};

#pragma endregion

#pragma region DeferredContext

class DeferredContext final
{
public:
	DeferredContext() = default;
	explicit DeferredContext(VulkanRender& device);
	DeferredContext(const DeferredContext&) = delete;
	DeferredContext(DeferredContext&& other) noexcept { Swap(other); }
	~DeferredContext() { Release(); }

	DeferredContext& operator=(const DeferredContext&) = delete;
	DeferredContext& operator=(DeferredContext&& other) noexcept 
	{
		if (this != &other)
		{
			Release();
			Swap(other);
		}
		return *this;
	}

	void Release();

	void Swap(DeferredContext& other) noexcept;

	[[nodiscard]] const vk::Extent2D& GetExtent() const { return m_extent; }
	[[nodiscard]] const vk::RenderPass& GetDeferredRenderPass() const { return m_deferredRenderPass; }
	[[nodiscard]] const vk::DescriptorSetLayout& GetDeferredTextureSetLayout() const { return m_deferredTextureSetLayout; }
	[[nodiscard]] const vk::DescriptorSet& GetDeferredTextureSet(uint32_t bufferingIndex) const
	{
		return m_deferredFramebuffers[bufferingIndex].GetTextureSet();
	}

	[[nodiscard]] const vk::RenderPassBeginInfo* GetDeferredRenderPassBeginInfo(uint32_t bufferingIndex) const
	{
		return &m_deferredRenderPassBeginInfo[bufferingIndex];
	}

	[[nodiscard]] const vk::RenderPass& GetForwardRenderPass() const { return m_forwardRenderPass; }

	[[nodiscard]] const vk::DescriptorSetLayout& GetForwardTextureSetLayout() const { return m_forwardTextureSetLayout; }

	[[nodiscard]] const vk::DescriptorSet& GetForwardTextureSet(uint32_t bufferingIndex) const {
		return m_forwardFramebuffers[bufferingIndex].GetTextureSet();
	}

	[[nodiscard]] const vk::RenderPassBeginInfo* GetForwardRenderPassBeginInfo(uint32_t bufferingIndex) const {
		return &m_forwardRenderPassBeginInfo[bufferingIndex];
	}

	void CheckFramebuffersOutOfDate();

private:
	void CreateRenderPass();
	void CreateFramebuffers();
	void CleanupFramebuffers();

	VulkanRender* m_device = nullptr;

	vk::Extent2D m_extent;
	vk::Sampler  m_sampler;

	vk::RenderPass          m_deferredRenderPass;
	vk::DescriptorSetLayout m_deferredTextureSetLayout;
	vk::RenderPass          m_forwardRenderPass;
	vk::DescriptorSetLayout m_forwardTextureSetLayout;

	std::vector<DeferredFramebuffer>     m_deferredFramebuffers;
	std::vector<vk::RenderPassBeginInfo> m_deferredRenderPassBeginInfo;
	std::vector<ForwardFramebuffer>      m_forwardFramebuffers;
	std::vector<vk::RenderPassBeginInfo> m_forwardRenderPassBeginInfo;
};

#pragma endregion

#pragma region PostProcessingContext

class PostProcessingContext final
{
public:
	PostProcessingContext() = default;
	explicit PostProcessingContext(VulkanRender& device);
	PostProcessingContext(const PostProcessingContext&) = delete;
	PostProcessingContext(PostProcessingContext&& other) noexcept { Swap(other); }
	~PostProcessingContext() { Release(); }

	PostProcessingContext& operator=(const PostProcessingContext&) = delete;
	PostProcessingContext& operator=(PostProcessingContext&& other) noexcept
	{
		if (this != &other)
		{
			Release();
			Swap(other);
		}
		return *this;
	}

	void Release();

	void Swap(PostProcessingContext& other) noexcept;

	[[nodiscard]] const vk::Extent2D& GetExtent() const { return m_extent; }
	[[nodiscard]] const vk::RenderPass& GetRenderPass() const { return m_renderPass; }
	[[nodiscard]] const vk::DescriptorSetLayout& GetTextureSetLayout() const { return m_textureSetLayout; }
	[[nodiscard]] const vk::DescriptorSet& GetTextureSet(uint32_t bufferingIndex) const { return m_framebuffers[bufferingIndex].GetTextureSet(); }
	[[nodiscard]] const vk::RenderPassBeginInfo* GetRenderPassBeginInfo(uint32_t bufferingIndex) const {
		return &m_renderPassBeginInfo[bufferingIndex];
	}

	void CheckFramebuffersOutOfDate();

private:
	void CreateRenderPass();
	void CreateFramebuffers();
	void CleanupFramebuffers();

	VulkanRender* m_device = nullptr;

	vk::Extent2D m_extent;
	vk::Sampler  m_sampler;

	vk::RenderPass          m_renderPass;
	vk::DescriptorSetLayout m_textureSetLayout;

	std::vector<PostProcessingFramebuffer> m_framebuffers;
	std::vector<vk::RenderPassBeginInfo>   m_renderPassBeginInfo;
};

#pragma endregion

#pragma region ShadowMap

class ShadowMap final
{
public:
	ShadowMap() = default;
	ShadowMap(
		VulkanRender* device,
		vk::RenderPass          renderPass,
		vk::DescriptorSetLayout textureSetLayout,
		vk::Sampler             sampler,
		const vk::Extent2D& extent
	);
	ShadowMap(const ShadowMap&) = delete;
	ShadowMap(ShadowMap&& other) noexcept { Swap(other); }
	~ShadowMap() { Release(); }

	ShadowMap& operator=(const ShadowMap&) = delete;
	ShadowMap& operator=(ShadowMap&& other) noexcept
	{
		if (this != &other)
		{
			Release();
			Swap(other);
		}
		return *this;
	}

	void Release();

	void Swap(ShadowMap& other) noexcept;

	[[nodiscard]] const vk::Framebuffer& GetFramebuffer() const { return m_framebuffer; }
	[[nodiscard]] const vk::DescriptorSet& GetTextureSet() const { return m_textureSet; }

private:
	void CreateAttachment(const vk::Extent2D& extent);
	void CreateAttachmentView();

	VulkanRender* m_device = nullptr;

	VulkanImage       m_depthAttachment;
	vk::ImageView     m_depthAttachmentView;
	vk::Framebuffer   m_framebuffer;
	vk::DescriptorSet m_textureSet;
};

#pragma endregion

#pragma region ShadowContext

class ShadowContext final
{
public:
	ShadowContext() = default;
	explicit ShadowContext(VulkanRender& device);
	ShadowContext(const ShadowContext&) = delete;
	ShadowContext(ShadowContext&& other) noexcept { Swap(other); }
	~ShadowContext() { Release(); }

	ShadowContext& operator=(const ShadowContext&) = delete;
	ShadowContext& operator=(ShadowContext&& other) noexcept
	{
		if (this != &other)
		{
			Release();
			Swap(other);
		}
		return *this;
	}

	void Release();

	void Swap(ShadowContext& other) noexcept;

	[[nodiscard]] const vk::RenderPass& GetRenderPass() const { return m_renderPass; }
	[[nodiscard]] const vk::DescriptorSetLayout& GetTextureSetLayout() const { return m_textureSetLayout; }
	[[nodiscard]] const vk::Extent2D& GetExtent() const { return m_extent; }
	[[nodiscard]] const vk::DescriptorSet& GetTextureSet(uint32_t bufferingIndex) const { return m_framebuffers[bufferingIndex].GetTextureSet(); }
	[[nodiscard]] const vk::RenderPassBeginInfo* GetRenderPassBeginInfo(uint32_t bufferingIndex) const {
		return &m_renderPassBeginInfos[bufferingIndex];
	}

private:
	void CreateRenderPass();
	void CreateFramebuffers();
	void CleanupFramebuffers();

	VulkanRender* m_device = nullptr;

	vk::RenderPass          m_renderPass;
	vk::DescriptorSetLayout m_textureSetLayout;
	vk::Sampler             m_sampler;

	vk::Extent2D                         m_extent;
	std::vector<ShadowMap>               m_framebuffers;
	std::vector<vk::RenderPassBeginInfo> m_renderPassBeginInfos;
};

#pragma endregion

#pragma region PbrMaterialCache

struct PbrMaterialConfig final
{
	std::string Albedo;
	std::string Normal;
	std::string MRA;
	std::string Emissive;
	bool        Transparent;
	bool        Shadow;

	explicit PbrMaterialConfig(const std::string& jsonFilename);
};


struct PbrMaterial final
{
	vk::DescriptorSet DescriptorSet;
	bool              Transparent = false;
	bool              Shadow = true;
};

class PbrMaterialCache final
{
public:
	PbrMaterialCache(VulkanRender& device, TextureCache& textureCache);
	PbrMaterialCache(const PbrMaterialCache&) = delete;
	PbrMaterialCache(PbrMaterialCache&&) = delete;
	~PbrMaterialCache();

	PbrMaterialCache& operator=(const PbrMaterialCache&) = delete;
	PbrMaterialCache& operator=(PbrMaterialCache&&) = delete;

	[[nodiscard]] const vk::DescriptorSetLayout& GetDescriptorSetLayout() const { return m_textureSetLayout; }

	PbrMaterial* LoadMaterial(const std::string& filename);

private:
	VulkanRender& m_device;
	TextureCache& m_textureCache;

	vk::DescriptorSetLayout            m_textureSetLayout;
	std::map<std::string, PbrMaterial> m_materials;
};

#pragma endregion

#pragma region SingleTextureMaterialCache

struct SingleTextureMaterial final
{
	vk::DescriptorSet DescriptorSet;
};

class SingleTextureMaterialCache final
{
public:
	SingleTextureMaterialCache(VulkanRender& device, TextureCache& textureCache);
	SingleTextureMaterialCache(const SingleTextureMaterialCache&) = delete;
	SingleTextureMaterialCache(SingleTextureMaterialCache&&) = delete;
	~SingleTextureMaterialCache();

	SingleTextureMaterialCache& operator=(const SingleTextureMaterialCache&) = delete;
	SingleTextureMaterialCache& operator=(SingleTextureMaterialCache&&) = delete;

	[[nodiscard]] const vk::DescriptorSetLayout& GetDescriptorSetLayout() const { return m_textureSetLayout; }

	SingleTextureMaterial* LoadMaterial(const std::string& filename);

private:
	VulkanRender& m_device;
	TextureCache& m_textureCache;
	vk::DescriptorSetLayout                      m_textureSetLayout;
	std::map<std::string, SingleTextureMaterial> m_materials;
};

#pragma endregion

#pragma region MeshUtilities

std::vector<VertexPositionOnly> CreateSkyboxVertices();

void AppendRectVertices(
	std::vector<VertexCanvas>& vertices,
	const glm::vec2& minPosition,
	const glm::vec2& maxPosition,
	const glm::vec2& minTexCoord,
	const glm::vec2& maxTexCoord
);

void AppendBoxVertices(std::vector<VertexBase>& vertices, const glm::vec3& min, const glm::vec3& max);

#pragma endregion

#pragma region PbrRenderer

struct alignas(256) RendererUniformData final
{
	glm::mat4              Projection;
	glm::mat4              View;
	glm::vec3              CameraPosition;
	[[maybe_unused]] float Padding;
	glm::vec4              ScreenInfo;
	glm::vec4              ScaledScreenInfo;
};

struct alignas(16) PointLightData final
{
	glm::vec3              Position;
	[[maybe_unused]] float Radius;
	glm::vec3              Color;
	[[maybe_unused]] float Padding;

	PointLightData() = default;
	PointLightData(const glm::vec3& position, float radius, const glm::vec3& color)
		: Position(position)
		, Radius(radius)
		, Color(color)
		, Padding(0.0f) {}
};

struct alignas(256) LightingUniformData final
{
	glm::vec3              LightDirection;
	int32_t                NumPointLights;
	glm::vec3              LightColor;
	[[maybe_unused]] float Padding0;
	glm::vec3              CascadeShadowMapSplits;
	[[maybe_unused]] float Padding1;
	glm::mat4              ShadowMatrices[4];
	PointLightData         PointLights[128];
};

class PbrRenderer final
{
public:
	explicit PbrRenderer(GLFWwindow* window);
	PbrRenderer(const PbrRenderer&) = delete;
	PbrRenderer(PbrRenderer&&) = delete;
	~PbrRenderer();

	PbrRenderer& operator=(const PbrRenderer&) = delete;
	PbrRenderer& operator=(PbrRenderer&&) = delete;

	[[nodiscard]] glm::vec2 GetScreenExtent() const 
	{
		const vk::Extent2D& swapchainExtent = m_device.GetSwapchainExtent();
		return { swapchainExtent.width, swapchainExtent.height };
	}

	void WaitDeviceIdle() { m_device.WaitIdle(); }

	VulkanMesh CreateMesh(const std::vector<VertexBase>& vertices) { return { m_device, vertices.size(), sizeof(VertexBase), vertices.data() }; }

	VulkanMesh* LoadObjMesh(const std::string& objFilename) { return m_meshCache.LoadObjMesh(objFilename); }

	PbrMaterial* LoadPbrMaterial(const std::string& materialFilename) { return m_pbrMaterialCache.LoadMaterial(materialFilename); }

	SingleTextureMaterial* LoadSingleTextureMaterial(const std::string& materialFilename)
	{
		return m_singleTextureMaterialCache.LoadMaterial(materialFilename);
	}

	void SetCameraData(const glm::vec3& cameraPosition, const glm::mat4& view, float fov, float near, float far);

	void SetLightingData(const glm::vec3& lightDirection, const glm::vec3& lightColor);

	void SetWorldBounds(const glm::vec3& min, const glm::vec3& max);

	void DrawPointLight(const glm::vec3& position, const glm::vec3& color, float radius) { m_pointLights.emplace_back(position, radius, color); }

	void Draw(const VulkanMesh* mesh, const glm::mat4& modelMatrix, const PbrMaterial* material)
	{
		if (material->Transparent)
		{
			m_forwardDrawCalls.emplace_back(mesh, modelMatrix, material);
		}
		else
		{
			m_deferredDrawCalls.emplace_back(mesh, modelMatrix, material);
		}
	}

	void DrawScreenRect(
		const glm::vec2& pMin,
		const glm::vec2& pMax,
		const glm::vec2& uvMin,
		const glm::vec2& uvMax,
		const glm::vec4& color,
		SingleTextureMaterial* texture
	)
	{
		m_screenRectDrawCalls.emplace_back(pMin, pMax, uvMin, uvMax, color, texture);
	}

	void DrawScreenLine(const glm::vec2& p0, const glm::vec2& p1, const glm::vec4& color) { m_screenLineDrawCalls.emplace_back(p0, p1, color); }

	void FinishDrawing();

private:
	void CreateUniformBuffers();
	void CreateIblTextureSet();
	void CreatePipelines();
	void CreateSkyboxCube();
	void CreateFullScreenQuad();
	void CreateScreenPrimitiveMeshes();
	void DrawToShadowMaps(vk::CommandBuffer cmd, uint32_t bufferingIndex);
	void DrawDeferred(vk::CommandBuffer cmd, uint32_t bufferingIndex);
	void DrawForward(vk::CommandBuffer cmd, uint32_t bufferingIndex);
	void PostProcess(vk::CommandBuffer cmd, uint32_t bufferingIndex);
	void DrawToScreen(const vk::RenderPassBeginInfo* primaryRenderPassBeginInfo, vk::CommandBuffer cmd, uint32_t bufferingIndex);

	VulkanRender                 m_device;
	TextureCache               m_textureCache;
	PbrMaterialCache           m_pbrMaterialCache;
	SingleTextureMaterialCache m_singleTextureMaterialCache;
	MeshCache                  m_meshCache;

	ShadowContext         m_shadowContext;
	DeferredContext       m_deferredContext;
	PostProcessingContext m_toneMappingContext;

	ShadowMatrixCalculator m_shadowMatrixCalculator;

	RendererUniformData    m_rendererUniformData{};
	LightingUniformData    m_lightingUniformData{};
	VulkanUniformBufferSet m_uniformBufferSet;

	vk::DescriptorSetLayout m_iblTextureSetLayout;
	vk::DescriptorSet       m_iblTextureSet;

	// shadow pass
	VulkanPipeline m_shadowPipeline;

	// deferred pass
	VulkanPipeline m_basePipeline;
	VulkanPipeline m_skyboxPipeline;

	// forward pass
	VulkanPipeline m_combinePipeline;
	VulkanPipeline m_baseForwardPipeline;

	// post-processing
	VulkanPipeline m_postProcessingPipeline;

	// presentation
	VulkanPipeline m_presentPipeline;
	VulkanPipeline m_screenRectPipeline;
	VulkanPipeline m_screenLinePipeline;

	VulkanMesh m_skyboxCube;
	VulkanMesh m_fullScreenQuad;
	VulkanMesh m_screenRectMesh;
	VulkanMesh m_screenLineMesh;

	std::vector<PointLightData> m_pointLights;

	struct DrawCall {
		const VulkanMesh* Mesh;
		glm::mat4          ModelMatrix;
		const PbrMaterial* Material;

		DrawCall(const VulkanMesh* mesh, const glm::mat4& modelMatrix, const PbrMaterial* material)
			: Mesh(mesh)
			, ModelMatrix(modelMatrix)
			, Material(material) {}
	};

	std::vector<DrawCall> m_deferredDrawCalls;
	std::vector<DrawCall> m_forwardDrawCalls;

	struct ScreenRectDrawCall {
		glm::vec2              PMin;
		glm::vec2              PMax;
		glm::vec2              UvMin;
		glm::vec2              UvMax;
		glm::vec4              Color;
		SingleTextureMaterial* Texture;

		ScreenRectDrawCall(
			const glm::vec2& pMin,
			const glm::vec2& pMax,
			const glm::vec2& uvMin,
			const glm::vec2& uvMax,
			const glm::vec4& color,
			SingleTextureMaterial* texture
		)
			: PMin(pMin)
			, PMax(pMax)
			, UvMin(uvMin)
			, UvMax(uvMax)
			, Color(color)
			, Texture(texture) {}
	};

	static constexpr size_t SCREEN_RECT_DRAW_CALL_DATA_SIZE = sizeof(ScreenRectDrawCall) - sizeof(SingleTextureMaterial*);

	std::vector<ScreenRectDrawCall> m_screenRectDrawCalls;

	struct ScreenLineDrawCall {
		glm::vec2 P0;
		glm::vec2 P1;
		glm::vec4 Color;

		ScreenLineDrawCall(const glm::vec2& p0, const glm::vec2& p1, const glm::vec4& color)
			: P0(p0)
			, P1(p1)
			, Color(color) {}
	};

	std::vector<ScreenLineDrawCall> m_screenLineDrawCalls;
};

#pragma endregion

#pragma region BitmapTextRenderer

class SingleTextureMaterial;

class BitmapTextRenderer final 
{
public:
	BitmapTextRenderer(PbrRenderer* renderer, const std::string& fontTexture, const glm::vec2& charSize);

	[[nodiscard]] const glm::vec2& GetCharSize() const { return m_charSize; }

	void DrawText(const std::string& text, const glm::vec2& position, const glm::vec4& color = { 1.0f, 1.0f, 1.0f, 1.0f });

private:
	SingleTextureMaterial* m_fontTexture = nullptr;
	PbrRenderer* m_renderer = nullptr;
	glm::vec2 m_charSize;
};

#pragma endregion
