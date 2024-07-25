#pragma once

#pragma region ShaderCompiler

struct ShaderIncluder;

class ShaderCompiler {
public:
	explicit ShaderCompiler(const std::string& includesDir);

	~ShaderCompiler();

	ShaderCompiler(const ShaderCompiler&) = delete;

	ShaderCompiler& operator=(const ShaderCompiler&) = delete;

	ShaderCompiler(ShaderCompiler&&) = delete;

	ShaderCompiler& operator=(ShaderCompiler&&) = delete;

	std::vector<uint32_t> Compile(vk::ShaderStageFlagBits stage, const std::string& source);

	std::vector<uint32_t> CompileFromFile(vk::ShaderStageFlagBits stage, const std::string& filename);

private:
	std::unique_ptr<ShaderIncluder> m_includer;
};

#pragma endregion

#pragma region VulkanBuffer

class VulkanBuffer {
public:
	VulkanBuffer() = default;

	VulkanBuffer(
		VmaAllocator             allocator,
		vk::DeviceSize           size,
		vk::BufferUsageFlags     bufferUsage,
		VmaAllocationCreateFlags flags,
		VmaMemoryUsage           memoryUsage
	);

	~VulkanBuffer() { Release(); }

	VulkanBuffer(const VulkanBuffer&) = delete;

	VulkanBuffer& operator=(const VulkanBuffer&) = delete;

	VulkanBuffer(VulkanBuffer&& other) noexcept { Swap(other); }

	VulkanBuffer& operator=(VulkanBuffer&& other) noexcept {
		if (this != &other) {
			Release();
			Swap(other);
		}
		return *this;
	}

	void Release();

	void Swap(VulkanBuffer& other) noexcept;

	void UploadRange(size_t offset, size_t size, const void* data) const;

	void Upload(size_t size, const void* data) const { UploadRange(0, size, data); }

	[[nodiscard]] const vk::Buffer& Get() const { return m_buffer; }

private:
	VmaAllocator m_allocator = VK_NULL_HANDLE;

	vk::Buffer    m_buffer;
	VmaAllocation m_allocation = VK_NULL_HANDLE;
};

#pragma endregion

#pragma region VulkanUniformBufferSet

class VulkanBase;

struct VulkanUniformBufferInfo {
	uint32_t             Binding;
	vk::ShaderStageFlags Stage;
	size_t               Size;
};

class VulkanUniformBufferSet {
public:
	VulkanUniformBufferSet() = default;

	VulkanUniformBufferSet(VulkanBase& device, const std::initializer_list<VulkanUniformBufferInfo>& uniformBufferInfos);

	~VulkanUniformBufferSet() { Release(); }

	VulkanUniformBufferSet(const VulkanUniformBufferSet&) = delete;

	VulkanUniformBufferSet& operator=(const VulkanUniformBufferSet&) = delete;

	VulkanUniformBufferSet(VulkanUniformBufferSet&& other) noexcept { Swap(other); }

	VulkanUniformBufferSet& operator=(VulkanUniformBufferSet&& other) noexcept {
		if (this != &other) {
			Release();
			Swap(other);
		}
		return *this;
	}

	void Release();

	void Swap(VulkanUniformBufferSet& other) noexcept;

	[[nodiscard]] const vk::DescriptorSetLayout& GetDescriptorSetLayout() const { return m_descriptorSetLayout; }

	[[nodiscard]] const vk::DescriptorSet& GetDescriptorSet() const { return m_descriptorSet; }

	[[nodiscard]] const std::vector<uint32_t>& GetDynamicOffsets(uint32_t bufferingIndex) const { return m_dynamicOffsets[bufferingIndex]; }

	void UpdateAllBuffers(uint32_t bufferingIndex, const std::initializer_list<const void*>& allBuffersData);

private:
	VulkanBase* m_device = nullptr;

	vk::DescriptorSetLayout            m_descriptorSetLayout;
	vk::DescriptorSet                  m_descriptorSet;
	std::vector<uint32_t>              m_uniformBufferSizes;
	std::vector<VulkanBuffer>          m_uniformBuffers;
	std::vector<std::vector<uint32_t>> m_dynamicOffsets;
};

#pragma endregion

#pragma region VulkanImage

class VulkanImage {
public:
	VulkanImage() = default;

	VulkanImage(
		VmaAllocator             allocator,
		vk::Format               format,
		const vk::Extent2D& extent,
		vk::ImageUsageFlags      imageUsage,
		VmaAllocationCreateFlags flags,
		VmaMemoryUsage           memoryUsage,
		uint32_t                 layers = 1
	);

	~VulkanImage() { Release(); }

	VulkanImage(const VulkanImage&) = delete;

	VulkanImage& operator=(const VulkanImage&) = delete;

	VulkanImage(VulkanImage&& other) noexcept { Swap(other); }

	VulkanImage& operator=(VulkanImage&& other) noexcept {
		if (this != &other) {
			Release();
			Swap(other);
		}
		return *this;
	}

	void Release();

	void Swap(VulkanImage& other) noexcept;

	[[nodiscard]] const vk::Image& Get() const { return m_image; }

private:
	VmaAllocator m_allocator = VK_NULL_HANDLE;

	vk::Image     m_image = VK_NULL_HANDLE;
	VmaAllocation m_allocation = VK_NULL_HANDLE;
};

#pragma endregion

#pragma region VulkanPipeline

struct VulkanPipelineOptions {
	vk::PrimitiveTopology Topology = vk::PrimitiveTopology::eTriangleList;
	vk::PolygonMode       PolygonMode = vk::PolygonMode::eFill;
	vk::CullModeFlags     CullMode = vk::CullModeFlagBits::eBack;
	vk::Bool32            DepthTestEnable = VK_TRUE;
	vk::Bool32            DepthWriteEnable = VK_TRUE;
	vk::CompareOp         DepthCompareOp = vk::CompareOp::eLess;
};

struct VulkanPipelineConfig {
	std::string           VertexShader;
	std::string           GeometryShader;
	std::string           FragmentShader;
	VulkanPipelineOptions Options;

	explicit VulkanPipelineConfig(const std::string& jsonFilename);
};

class VulkanDevice;

class VulkanPipeline
{
public:
	VulkanPipeline() = default;

	VulkanPipeline(
		VulkanDevice& device,
		ShaderCompiler& compiler,
		const std::initializer_list<vk::DescriptorSetLayout>& descriptorSetLayouts,
		const std::initializer_list<vk::PushConstantRange>& pushConstantRanges,
		const vk::PipelineVertexInputStateCreateInfo* vertexInput,
		const std::string& pipelineConfigFile,
		const std::initializer_list<vk::PipelineColorBlendAttachmentState>& attachmentColorBlends,
		vk::RenderPass                                                      renderPass,
		uint32_t                                                            subpass
	);

	~VulkanPipeline() { Release(); }

	VulkanPipeline(const VulkanPipeline&) = delete;

	VulkanPipeline& operator=(const VulkanPipeline&) = delete;

	VulkanPipeline(VulkanPipeline&& other) noexcept { Swap(other); }

	VulkanPipeline& operator=(VulkanPipeline&& other) noexcept {
		if (this != &other) {
			Release();
			Swap(other);
		}
		return *this;
	}

	void Release();

	void Swap(VulkanPipeline& other) noexcept;

	[[nodiscard]] const vk::PipelineLayout& GetLayout() const { return m_pipelineLayout; }

	[[nodiscard]] const vk::Pipeline& Get() const { return m_pipeline; }

private:
	VulkanDevice* m_device = nullptr;

	vk::PipelineLayout m_pipelineLayout;
	vk::ShaderModule   m_vertexShaderModule;
	vk::ShaderModule   m_geometryShaderModule;
	vk::ShaderModule   m_fragmentShaderModule;
	vk::Pipeline       m_pipeline;
};

#pragma endregion

#pragma region VulkanTexture

class VulkanBase;

class VulkanTexture {
public:
	VulkanTexture() = default;

	// R8G8B8A8Unorm
	VulkanTexture(
		VulkanBase& device,
		uint32_t               width,
		uint32_t               height,
		const unsigned char* data,
		vk::Filter             filter,
		vk::SamplerAddressMode addressMode
	);

	~VulkanTexture() { Release(); }

	VulkanTexture(const VulkanTexture&) = delete;

	VulkanTexture& operator=(const VulkanTexture&) = delete;

	VulkanTexture(VulkanTexture&& other) noexcept { Swap(other); }

	VulkanTexture& operator=(VulkanTexture&& other) noexcept {
		if (this != &other) {
			Release();
			Swap(other);
		}
		return *this;
	}

	void Release();

	void Swap(VulkanTexture& other) noexcept;

	void BindToDescriptorSet(vk::DescriptorSet descriptorSet, uint32_t binding);

private:
	void CreateImage(uint32_t width, uint32_t height, vk::DeviceSize size, const void* data, vk::Format format);

	void CreateImageView(vk::Format format);

	void CreateSampler(vk::Filter filter, vk::SamplerAddressMode addressMode);

	VulkanBase* m_device = nullptr;

	VulkanImage   m_image;
	vk::ImageView m_imageView;
	vk::Sampler   m_sampler;
};

#pragma endregion

#pragma region VulkanMesh

class VulkanBase;

class VulkanMesh {
public:
	VulkanMesh() = default;

	VulkanMesh(VulkanBase& device, size_t vertexCount, size_t vertexSize, const void* data);

	~VulkanMesh() { Release(); }

	VulkanMesh(const VulkanMesh&) = delete;

	VulkanMesh& operator=(const VulkanMesh&) = delete;

	VulkanMesh(VulkanMesh&& other) noexcept { Swap(other); }

	VulkanMesh& operator=(VulkanMesh&& other) noexcept {
		if (this != &other) {
			Release();
			Swap(other);
		}
		return *this;
	}

	void Release();

	void Swap(VulkanMesh& other) noexcept;

	void BindAndDraw(vk::CommandBuffer commandBuffer) const;

private:
	VulkanBuffer m_vertexBuffer;
	uint32_t     m_vertexCount = 0;
};

#pragma endregion

#pragma region VertexFormats

struct VertexPositionOnly {
	glm::vec3 Position;

	VertexPositionOnly() = default;

	explicit VertexPositionOnly(const glm::vec3& position)
		: Position(position) {}

	static const vk::PipelineVertexInputStateCreateInfo* GetPipelineVertexInputStateCreateInfo();
};

struct VertexCanvas {
	glm::vec2 Position;
	glm::vec2 TexCoord;

	VertexCanvas() = default;

	VertexCanvas(const glm::vec2& position, const glm::vec2& texCoord)
		: Position(position)
		, TexCoord(texCoord) {}

	static const vk::PipelineVertexInputStateCreateInfo* GetPipelineVertexInputStateCreateInfo();
};

struct VertexBase {
	glm::vec3 Position;
	glm::vec3 Normal;
	glm::vec2 TexCoord;

	VertexBase() = default;

	VertexBase(const glm::vec3& position, const glm::vec3& normal, const glm::vec2& texCoord)
		: Position(position)
		, Normal(normal)
		, TexCoord(texCoord) {}

	static const vk::PipelineVertexInputStateCreateInfo* GetPipelineVertexInputStateCreateInfo();
};

#pragma endregion

#pragma region TextureCache

class TextureCache {
public:
	explicit TextureCache(VulkanBase& device)
		: m_device(device) {}

	~TextureCache() = default;

	TextureCache(const TextureCache&) = delete;

	TextureCache& operator=(const TextureCache&) = delete;

	TextureCache(TextureCache&&) = delete;

	TextureCache& operator=(TextureCache&&) = delete;

	VulkanTexture* LoadTexture(
		const std::string& filename,
		vk::Filter             filter = vk::Filter::eNearest,
		vk::SamplerAddressMode addressMode = vk::SamplerAddressMode::eRepeat
	);

private:
	VulkanBase& m_device;

	std::map<std::string, VulkanTexture> m_textures;
};

#pragma endregion

#pragma region MeshCache

class MeshCache {
public:
	explicit MeshCache(VulkanBase& device)
		: m_device(device) {}

	~MeshCache() = default;

	MeshCache(const MeshCache&) = delete;

	MeshCache& operator=(const MeshCache&) = delete;

	MeshCache(MeshCache&&) = delete;

	MeshCache& operator=(MeshCache&&) = delete;

	VulkanMesh* LoadObjMesh(const std::string& filename);

private:
	VulkanBase& m_device;

	std::map<std::string, VulkanMesh> m_meshes;
};

#pragma endregion

#pragma region VulkanDevice

struct VulkanRenderPassOptions {
	bool ForPresentation = false;
	bool PreserveDepth = false;
	bool ShaderReadsDepth = false;
};

class VulkanDevice {
public:
	VulkanDevice();

	~VulkanDevice();

	VulkanDevice(const VulkanDevice&) = delete;

	VulkanDevice& operator=(const VulkanDevice&) = delete;

	VulkanDevice(VulkanDevice&&) = delete;

	VulkanDevice& operator=(VulkanDevice&&) = delete;

	void WaitIdle();

	VulkanBuffer CreateBuffer(vk::DeviceSize size, vk::BufferUsageFlags bufferUsage, VmaAllocationCreateFlags flags, VmaMemoryUsage memoryUsage) {
		return { m_allocator, size, bufferUsage, flags, memoryUsage };
	}

	VulkanImage CreateImage(
		vk::Format               format,
		const vk::Extent2D& extent,
		vk::ImageUsageFlags      imageUsage,
		VmaAllocationCreateFlags flags,
		VmaMemoryUsage           memoryUsage,
		uint32_t                 layers = 1
	) {
		return { m_allocator, format, extent, imageUsage, flags, memoryUsage, layers };
	}

	vk::ImageView CreateImageView(vk::Image image, vk::Format format, vk::ImageAspectFlags aspectMask, uint32_t layers = 1);

	void DestroyImageView(vk::ImageView imageView);

	vk::Sampler CreateSampler(
		vk::Filter             filter,
		vk::SamplerAddressMode addressMode,
		vk::Bool32             compareEnable = VK_FALSE,
		vk::CompareOp          compareOp = vk::CompareOp::eNever
	);

	void DestroySampler(vk::Sampler sampler);

	vk::RenderPass CreateRenderPass(
		const std::initializer_list<vk::Format>& colorAttachmentFormats,
		vk::Format                               depthStencilAttachmentFormat = vk::Format::eUndefined,
		const VulkanRenderPassOptions& options = {}
	);

	void DestroyRenderPass(vk::RenderPass renderPass);

	vk::Framebuffer CreateFramebuffer(
		vk::RenderPass                                    renderPass,
		const vk::ArrayProxyNoTemporaries<vk::ImageView>& attachments,
		const vk::Extent2D& extent,
		uint32_t                                          layers = 1
	);

	void DestroyFramebuffer(vk::Framebuffer framebuffer);

	vk::DescriptorSetLayout CreateDescriptorSetLayout(const vk::ArrayProxyNoTemporaries<vk::DescriptorSetLayoutBinding>& bindings);

	void DestroyDescriptorSetLayout(vk::DescriptorSetLayout descriptorSetLayout);

	vk::DescriptorSet AllocateDescriptorSet(vk::DescriptorSetLayout descriptorSetLayout);

	void FreeDescriptorSet(vk::DescriptorSet descriptorSet);

	void WriteCombinedImageSamplerToDescriptorSet(vk::Sampler sampler, vk::ImageView imageView, vk::DescriptorSet descriptorSet, uint32_t binding);

	void WriteDynamicUniformBufferToDescriptorSet(vk::Buffer buffer, vk::DeviceSize size, vk::DescriptorSet descriptorSet, uint32_t binding);

	vk::PipelineLayout CreatePipelineLayout(
		const std::initializer_list<vk::DescriptorSetLayout>& descriptorSetLayouts,
		const std::initializer_list<vk::PushConstantRange>& pushConstantRanges
	);

	void DestroyPipelineLayout(vk::PipelineLayout pipelineLayout);

	vk::ShaderModule CreateShaderModule(const std::vector<uint32_t>& spirv);

	void DestroyShaderModule(vk::ShaderModule shaderModule);

	vk::Pipeline CreatePipeline(
		vk::PipelineLayout                                                    pipelineLayout,
		const vk::PipelineVertexInputStateCreateInfo* vertexInput,
		const vk::ArrayProxyNoTemporaries<vk::PipelineShaderStageCreateInfo>& shaderStages,
		const VulkanPipelineOptions& options,
		const std::initializer_list<vk::PipelineColorBlendAttachmentState>& attachmentColorBlends,
		vk::RenderPass                                                        renderPass,
		uint32_t                                                              subpass
	);

	void DestroyPipeline(vk::Pipeline pipeline);

protected:
	void CreateInstance();

	void CreateDebugMessenger();

	void PickPhysicalDevice();

	void CreateDevice();

	void SubmitToGraphicsQueue(const vk::SubmitInfo& submitInfo, vk::Fence fence);

	void CreateCommandPool();

	void CreateDescriptorPool();

	void WriteDescriptorSet(const vk::WriteDescriptorSet& writeDescriptorSet);

	void CreateAllocator();

	vk::Fence CreateFence(vk::FenceCreateFlags flags = {});

	void WaitAndResetFence(vk::Fence fence, uint64_t timeout = 100'000'000);

	vk::Semaphore CreateSemaphore();

	vk::CommandBuffer AllocateCommandBuffer();

	vk::SurfaceKHR CreateSurface(GLFWwindow* window);

	vk::SwapchainKHR CreateSwapchain(
		vk::SurfaceKHR                  surface,
		uint32_t                        imageCount,
		vk::Format                      imageFormat,
		vk::ColorSpaceKHR               imageColorSpace,
		const vk::Extent2D& imageExtent,
		vk::SurfaceTransformFlagBitsKHR preTransform,
		vk::PresentModeKHR              presentMode,
		vk::SwapchainKHR                oldSwapchain
	);

	vk::Instance               m_instance;
	vk::DebugUtilsMessengerEXT m_debugMessenger;

	vk::PhysicalDevice m_physicalDevice;
	uint32_t           m_graphicsQueueFamily = 0;
	vk::Device         m_device;
	vk::Queue          m_graphicsQueue;

	vk::CommandPool m_commandPool;

	vk::DescriptorPool m_descriptorPool;

	VmaAllocator m_allocator = VK_NULL_HANDLE;
};

void BeginCommandBuffer(vk::CommandBuffer commandBuffer, vk::CommandBufferUsageFlags flags = {});

void EndCommandBuffer(vk::CommandBuffer commandBuffer);

#pragma endregion

#pragma region VulkanBase

static inline std::pair<vk::Viewport, vk::Rect2D> CalcViewportAndScissorFromExtent(const vk::Extent2D& extent, bool flipped = true) {
	// flipped upside down so that it's consistent with OpenGL
	const auto width = static_cast<float>(extent.width);
	const auto height = static_cast<float>(extent.height);
	return {
		{0.0f, flipped ? height : 0, width, flipped ? -height : height, 0.0f, 1.0f},
		{{0, 0}, extent}
	};
}

struct VulkanFrameInfo {
	const vk::RenderPassBeginInfo* PrimaryRenderPassBeginInfo = nullptr;
	uint32_t                       BufferingIndex = 0;
	vk::CommandBuffer              CommandBuffer;
};

class VulkanBase : public VulkanDevice {
public:
	static constexpr vk::Format         SURFACE_FORMAT = vk::Format::eB8G8R8A8Unorm;
	static constexpr vk::ColorSpaceKHR  SURFACE_COLOR_SPACE = vk::ColorSpaceKHR::eSrgbNonlinear;
	static constexpr vk::PresentModeKHR PRESENT_MODE = vk::PresentModeKHR::eFifo;

	explicit VulkanBase(GLFWwindow* window);

	~VulkanBase();

	VulkanBase(const VulkanBase&) = delete;

	VulkanBase& operator=(const VulkanBase&) = delete;

	VulkanBase(VulkanBase&&) = delete;

	VulkanBase& operator=(VulkanBase&&) = delete;

	[[nodiscard]] const vk::RenderPass& GetPrimaryRenderPass() const { return m_primaryRenderPass; }

	[[nodiscard]] const vk::Extent2D& GetSwapchainExtent() const { return m_swapchainExtent; }

	[[nodiscard]] vk::Extent2D GetScaledExtent() const { return m_swapchainExtent; }

	[[nodiscard]] size_t GetNumBuffering() const { return m_bufferingObjects.size(); }

	VulkanFrameInfo BeginFrame();

	void EndFrame();

	template<class Func>
	void ImmediateSubmit(Func&& func) {
		m_immediateCommandBuffer.reset();
		BeginCommandBuffer(m_immediateCommandBuffer, vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
		func(m_immediateCommandBuffer);
		EndCommandBuffer(m_immediateCommandBuffer);

		const vk::SubmitInfo submitInfo({}, {}, m_immediateCommandBuffer, {});
		SubmitToGraphicsQueue(submitInfo, m_immediateFence);
		WaitAndResetFence(m_immediateFence);
	}

private:
	void CreateImmediateContext();

	void CreateBufferingObjects();

	void CreateSurfaceSwapchainAndImageViews();

	void CreatePrimaryRenderPass();

	void CreatePrimaryFramebuffers();

	void CleanupSurfaceSwapchainAndImageViews();

	void CleanupPrimaryFramebuffers();

	void RecreateSwapchain();

	vk::Result TryAcquiringNextSwapchainImage();

	void AcquireNextSwapchainImage();

	GLFWwindow* m_window = nullptr;

	vk::Fence         m_immediateFence;
	vk::CommandBuffer m_immediateCommandBuffer;

	struct BufferingObjects {
		vk::Fence         RenderFence;
		vk::Semaphore     PresentSemaphore;
		vk::Semaphore     RenderSemaphore;
		vk::CommandBuffer CommandBuffer;
	};

	std::array<BufferingObjects, 3> m_bufferingObjects;

	vk::SurfaceKHR             m_surface;
	vk::Extent2D               m_swapchainExtent;
	vk::SwapchainKHR           m_swapchain;
	std::vector<vk::ImageView> m_swapchainImageViews;

	vk::RenderPass m_primaryRenderPass;

	std::vector<vk::Framebuffer>         m_primaryFramebuffers;
	std::vector<vk::RenderPassBeginInfo> m_primaryRenderPassBeginInfos;

	uint32_t m_currentSwapchainImageIndex = 0;
	uint32_t m_currentBufferingIndex = 0;
};

#pragma endregion
