#pragma once

#pragma region VulkanImage

class VulkanImage final
{
public:
	VulkanImage() = default;
	VulkanImage(VmaAllocator allocator, VkFormat format, const VkExtent2D& extent, VkImageUsageFlags imageUsage, VmaAllocationCreateFlags flags, VmaMemoryUsage memoryUsage, uint32_t layers = 1);
	VulkanImage(const VulkanImage&) = delete;
	VulkanImage(VulkanImage&& other) noexcept;
	~VulkanImage();

	VulkanImage& operator=(const VulkanImage&) = delete;
	VulkanImage& operator=(VulkanImage&& other) noexcept;

	void Release();

	void Swap(VulkanImage& other) noexcept;

	[[nodiscard]] const VkImage& Get() const { return m_image; }

private:
	VmaAllocator m_allocator{ VK_NULL_HANDLE };
	VkImage m_image{ VK_NULL_HANDLE };
	VmaAllocation m_allocation{ VK_NULL_HANDLE };
};

#pragma endregion

#pragma region VulkanBuffer

class VulkanBuffer final
{
public:
	VulkanBuffer() = default;
	VulkanBuffer(VmaAllocator allocator, VkDeviceSize size, VkBufferUsageFlags bufferUsage, VmaAllocationCreateFlags flags, VmaMemoryUsage memoryUsage);
	VulkanBuffer(const VulkanBuffer&) = delete;
	VulkanBuffer(VulkanBuffer&& other) noexcept;
	~VulkanBuffer();

	VulkanBuffer& operator=(const VulkanBuffer&) = delete;
	VulkanBuffer& operator=(VulkanBuffer&& other) noexcept
	{
		if (this != &other)
		{
			Release();
			Swap(other);
		}
		return *this;
	}

	void Release();

	void Swap(VulkanBuffer& other) noexcept;

	void UploadRange(size_t offset, size_t size, const void* data) const;

	void Upload(size_t size, const void* data) const { UploadRange(0, size, data); }

	[[nodiscard]] const VkBuffer& Get() const { return m_buffer; }

private:
	VmaAllocator m_allocator{ VK_NULL_HANDLE };
	VkBuffer m_buffer{ VK_NULL_HANDLE };
	VmaAllocation m_allocation{ VK_NULL_HANDLE };
};

#pragma endregion

#pragma region VulkanTexture

class VulkanTexture final
{
public:
	VulkanTexture() = default;
	VulkanTexture(uint32_t width, uint32_t height, const unsigned char* data, VkFilter filter, VkSamplerAddressMode addressMode);
	VulkanTexture(const VulkanTexture&) = delete;
	VulkanTexture(VulkanTexture&& other) noexcept;
	~VulkanTexture();

	VulkanTexture& operator=(const VulkanTexture&) = delete;
	VulkanTexture& operator=(VulkanTexture&& other) noexcept;

	void Release();

	void Swap(VulkanTexture& other) noexcept;

	void BindToDescriptorSet(VkDescriptorSet descriptorSet, uint32_t binding);

private:
	void createImage(uint32_t width, uint32_t height, VkDeviceSize size, const void* data, VkFormat format);
	void createImageView(VkFormat format);
	void createSampler(VkFilter filter, VkSamplerAddressMode addressMode);

	VulkanImage m_image{};
	VkImageView m_imageView{ VK_NULL_HANDLE };
	VkSampler m_sampler{ VK_NULL_HANDLE };
};

#pragma endregion

#pragma region VulkanUniformBufferSet

struct VulkanUniformBufferInfo final
{
	uint32_t Binding;
	VkShaderStageFlags Stage;
	size_t Size;
};

class VulkanUniformBufferSet final
{
public:
	VulkanUniformBufferSet() = default;
	VulkanUniformBufferSet(const std::initializer_list<VulkanUniformBufferInfo>& uniformBufferInfos);
	VulkanUniformBufferSet(const VulkanUniformBufferSet&) = delete;
	VulkanUniformBufferSet(VulkanUniformBufferSet&& other) noexcept;
	~VulkanUniformBufferSet();

	VulkanUniformBufferSet& operator=(const VulkanUniformBufferSet&) = delete;
	VulkanUniformBufferSet& operator=(VulkanUniformBufferSet&& other) noexcept;

	void Release();

	void Swap(VulkanUniformBufferSet& other) noexcept;

	[[nodiscard]] const VkDescriptorSetLayout& GetDescriptorSetLayout() const { return m_descriptorSetLayout; }

	[[nodiscard]] const VkDescriptorSet& GetDescriptorSet() const { return m_descriptorSet; }

	[[nodiscard]] const std::vector<uint32_t>& GetDynamicOffsets(uint32_t bufferingIndex) const { return m_dynamicOffsets[bufferingIndex]; }

	void UpdateAllBuffers(uint32_t bufferingIndex, const std::initializer_list<const void*>& allBuffersData);

private:
	VkDescriptorSetLayout m_descriptorSetLayout{ nullptr };
	VkDescriptorSet m_descriptorSet{ nullptr };
	std::vector<uint32_t> m_uniformBufferSizes;
	std::vector<VulkanBuffer> m_uniformBuffers;
	std::vector<std::vector<uint32_t>> m_dynamicOffsets;
};

#pragma endregion

#pragma region ShaderCompiler

struct ShaderIncluder;

class ShaderCompiler
{
public:
	explicit ShaderCompiler(const std::string& includesDir);

	~ShaderCompiler();

	ShaderCompiler(const ShaderCompiler&) = delete;

	ShaderCompiler& operator=(const ShaderCompiler&) = delete;

	ShaderCompiler(ShaderCompiler&&) = delete;

	ShaderCompiler& operator=(ShaderCompiler&&) = delete;

	std::vector<uint32_t> Compile(VkShaderStageFlagBits stage, const std::string& source);

	std::vector<uint32_t> CompileFromFile(VkShaderStageFlagBits stage, const std::string& filename);

private:
	std::unique_ptr<ShaderIncluder> m_includer;
};

#pragma endregion

#pragma region VulkanPipeline

struct VulkanPipelineOptions
{
	VkPrimitiveTopology Topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	VkPolygonMode PolygonMode = VK_POLYGON_MODE_FILL;
	VkCullModeFlags CullMode = VK_CULL_MODE_BACK_BIT;
	VkBool32 DepthTestEnable = VK_TRUE;
	VkBool32 DepthWriteEnable = VK_TRUE;
	VkCompareOp DepthCompareOp = VK_COMPARE_OP_LESS;
};

struct VulkanPipelineConfig 
{
	explicit VulkanPipelineConfig(const std::string& jsonFilename);

	std::string VertexShader;
	std::string GeometryShader;
	std::string FragmentShader;
	VulkanPipelineOptions Options;
};

class VulkanPipeline final
{
public:
	VulkanPipeline() = default;
	VulkanPipeline(
		ShaderCompiler& compiler,
		const std::initializer_list<VkDescriptorSetLayout>& descriptorSetLayouts,
		const std::initializer_list<VkPushConstantRange>& pushConstantRanges,
		const VkPipelineVertexInputStateCreateInfo* vertexInput,
		const std::string& pipelineConfigFile,
		const std::initializer_list<VkPipelineColorBlendAttachmentState>& attachmentColorBlends,
		VkRenderPass                                                        renderPass,
		uint32_t                                                            subpass
	);
	VulkanPipeline(const VulkanPipeline&) = delete;
	VulkanPipeline(VulkanPipeline&& other) noexcept { Swap(other); }
	~VulkanPipeline() { Release(); }

	VulkanPipeline& operator=(const VulkanPipeline&) = delete;
	VulkanPipeline& operator=(VulkanPipeline&& other) noexcept
	{
		if (this != &other)
		{
			Release();
			Swap(other);
		}
		return *this;
	}

	void Release();

	void Swap(VulkanPipeline& other) noexcept;

	[[nodiscard]] const VkPipelineLayout& GetLayout() const { return m_pipelineLayout; }

	[[nodiscard]] const VkPipeline& Get() const { return m_pipeline; }

private:
	VkPipelineLayout m_pipelineLayout{ nullptr };
	VkShaderModule m_vertexShaderModule{ nullptr };
	VkShaderModule m_geometryShaderModule{ nullptr };
	VkShaderModule m_fragmentShaderModule{ nullptr };
	VkPipeline m_pipeline{ nullptr };
};

#pragma endregion

#pragma region VulkanMesh

class VulkanMesh final
{
public:
	VulkanMesh() = default;
	VulkanMesh(size_t vertexCount, size_t vertexSize, const void* data);
	VulkanMesh(const VulkanMesh&) = delete;
	VulkanMesh(VulkanMesh&& other) noexcept { Swap(other); }
	~VulkanMesh() { Release(); }

	VulkanMesh& operator=(const VulkanMesh&) = delete;
	VulkanMesh& operator=(VulkanMesh&& other) noexcept
	{
		if (this != &other)
		{
			Release();
			Swap(other);
		}
		return *this;
	}

	void Release();

	void Swap(VulkanMesh& other) noexcept;

	void BindAndDraw(VkCommandBuffer commandBuffer) const;

private:
	VulkanBuffer m_vertexBuffer;
	uint32_t m_vertexCount = 0;
};

#pragma endregion

#pragma region VertexFormats

struct VertexPositionOnly
{
	glm::vec3 Position;

	VertexPositionOnly() = default;

	explicit VertexPositionOnly(const glm::vec3& position) : Position(position) {}

	static const VkPipelineVertexInputStateCreateInfo* GetPipelineVertexInputStateCreateInfo();
};

struct VertexCanvas
{
	VertexCanvas() = default;
	VertexCanvas(const glm::vec2& position, const glm::vec2& texCoord)
		: Position(position)
		, TexCoord(texCoord) {}

	static const VkPipelineVertexInputStateCreateInfo* GetPipelineVertexInputStateCreateInfo();

	glm::vec2 Position;
	glm::vec2 TexCoord;
};

struct VertexBase
{
	VertexBase() = default;
	VertexBase(const glm::vec3& position, const glm::vec3& normal, const glm::vec2& texCoord)
		: Position(position)
		, Normal(normal)
		, TexCoord(texCoord) {}

	static const VkPipelineVertexInputStateCreateInfo* GetPipelineVertexInputStateCreateInfo();

	glm::vec3 Position;
	glm::vec3 Normal;
	glm::vec2 TexCoord;
};

#pragma endregion

#pragma region Renderer

namespace Render
{
	std::optional<VkRenderPass> CreateRenderPass(const std::vector<VkFormat>& colorAttachmentFormats, VkFormat depthStencilAttachmentFormat = VK_FORMAT_UNDEFINED, const VulkanRenderPassOptions& options = {});

	std::optional<VkFramebuffer> CreateFramebuffer(VkRenderPass renderPass, const std::vector<VkImageView>& attachments, const VkExtent2D& extent, uint32_t layers = 1);



	[[nodiscard]] size_t GetNumBuffering();

	VulkanImage CreateImage(VkFormat format, const VkExtent2D& extent, VkImageUsageFlags imageUsage, VmaAllocationCreateFlags flags, VmaMemoryUsage memoryUsage, uint32_t layers = 1);
	VkImageView CreateImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectMask, uint32_t layers = 1);
	VkSampler CreateSampler(VkFilter filter, VkSamplerAddressMode addressMode, VkBool32 compareEnable = VK_FALSE, VkCompareOp compareOp = VK_COMPARE_OP_NEVER);

	VulkanBuffer CreateBuffer(VkDeviceSize size, VkBufferUsageFlags bufferUsage, VmaAllocationCreateFlags flags, VmaMemoryUsage memoryUsage);

	VkDescriptorSetLayout CreateDescriptorSetLayout(const std::vector<VkDescriptorSetLayoutBinding>& bindings);

	VkDescriptorSet AllocateDescriptorSet(VkDescriptorSetLayout descriptorSetLayout);

	VkPipelineLayout CreatePipelineLayout(
		const std::vector<VkDescriptorSetLayout>& descriptorSetLayouts,
		const std::vector<VkPushConstantRange>& pushConstantRanges
	);

	VkShaderModule CreateShaderModule(const std::vector<uint32_t>& spirv);

	VkPipeline CreatePipeline(
		VkPipelineLayout pipelineLayout,
		const VkPipelineVertexInputStateCreateInfo* vertexInput,
		const std::vector<VkPipelineShaderStageCreateInfo>& shaderStages,
		const VulkanPipelineOptions& options,
		const std::vector<VkPipelineColorBlendAttachmentState>& attachmentColorBlends,
		VkRenderPass renderPass,
		uint32_t subpass
	);

	VulkanTexture* LoadTexture(
		const std::string& filename,
		VkFilter filter = VK_FILTER_NEAREST,
		VkSamplerAddressMode addressMode = VK_SAMPLER_ADDRESS_MODE_REPEAT
	);

	VulkanMesh* LoadObjMesh(const std::string& filename);

	void BeginImmediateSubmit();
	void EndImmediateSubmit();

	bool BeginCommandBuffer(VkCommandBuffer commandBuffer, VkCommandBufferUsageFlags flags = {});
	bool EndCommandBuffer(VkCommandBuffer commandBuffer);

	void WaitAndResetFence(VkFence fence, uint64_t timeout = 100'000'000);

	void WriteCombinedImageSamplerToDescriptorSet(VkSampler sampler, VkImageView imageView, VkDescriptorSet descriptorSet, uint32_t binding);
	void WriteDynamicUniformBufferToDescriptorSet(VkBuffer buffer, VkDeviceSize size, VkDescriptorSet descriptorSet, uint32_t binding);

	void DestroySampler(VkSampler sampler);
	void DestroyImageView(VkImageView imageView);
	void FreeDescriptorSet(VkDescriptorSet descriptorSet);
	void DestroyDescriptorSetLayout(VkDescriptorSetLayout descriptorSetLayout);
}

#pragma endregion
