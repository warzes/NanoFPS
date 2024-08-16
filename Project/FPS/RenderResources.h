#pragma once

#pragma region ShaderCompiler

struct ShaderIncluder;

class ShaderCompiler final
{
public:
	explicit ShaderCompiler(const std::string& includesDir);
	ShaderCompiler(const ShaderCompiler&) = delete;
	ShaderCompiler(ShaderCompiler&&) = delete;
	~ShaderCompiler();

	ShaderCompiler& operator=(const ShaderCompiler&) = delete;
	ShaderCompiler& operator=(ShaderCompiler&&) = delete;

	std::vector<uint32_t> Compile(vk::ShaderStageFlagBits stage, const std::string& source);
	std::vector<uint32_t> CompileFromFile(vk::ShaderStageFlagBits stage, const std::string& filename);

private:
	std::unique_ptr<ShaderIncluder> m_includer;
};

#pragma endregion

#pragma region VulkanBuffer

class VulkanBuffer final
{
public:
	VulkanBuffer() = default;
	VulkanBuffer(
		VmaAllocator             allocator,
		vk::DeviceSize           size,
		vk::BufferUsageFlags     bufferUsage,
		VmaAllocationCreateFlags flags,
		VmaMemoryUsage           memoryUsage
	);
	VulkanBuffer(const VulkanBuffer&) = delete;
	VulkanBuffer(VulkanBuffer&& other) noexcept { Swap(other); }
	~VulkanBuffer() { Release(); }

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

	[[nodiscard]] const vk::Buffer& Get() const { return m_buffer; }

private:
	VmaAllocator  m_allocator{ VK_NULL_HANDLE };
	vk::Buffer    m_buffer{ VK_NULL_HANDLE };
	VmaAllocation m_allocation{ VK_NULL_HANDLE };
};

#pragma endregion

#pragma region VulkanUniformBufferSet

class VulkanRender;

struct VulkanUniformBufferInfo final
{
	uint32_t             Binding;
	vk::ShaderStageFlags Stage;
	size_t               Size;
};

class VulkanUniformBufferSet final
{
public:
	VulkanUniformBufferSet() = default;
	VulkanUniformBufferSet(VulkanRender& device, const std::initializer_list<VulkanUniformBufferInfo>& uniformBufferInfos);
	VulkanUniformBufferSet(const VulkanUniformBufferSet&) = delete;
	VulkanUniformBufferSet(VulkanUniformBufferSet&& other) noexcept { Swap(other); }
	~VulkanUniformBufferSet() { Release(); }

	VulkanUniformBufferSet& operator=(const VulkanUniformBufferSet&) = delete;
	VulkanUniformBufferSet& operator=(VulkanUniformBufferSet&& other) noexcept
	{
		if (this != &other)
		{
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
	VulkanRender* m_device = nullptr;
	vk::DescriptorSetLayout            m_descriptorSetLayout;
	vk::DescriptorSet                  m_descriptorSet;
	std::vector<uint32_t>              m_uniformBufferSizes;
	std::vector<VulkanBuffer>          m_uniformBuffers;
	std::vector<std::vector<uint32_t>> m_dynamicOffsets;
};

#pragma endregion

#pragma region VulkanImage

class VulkanImage final
{
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
	VulkanImage(const VulkanImage&) = delete;
	VulkanImage(VulkanImage&& other) noexcept { Swap(other); }
	~VulkanImage() { Release(); }

	VulkanImage& operator=(const VulkanImage&) = delete;
	VulkanImage& operator=(VulkanImage&& other) noexcept
	{
		if (this != &other)
		{
			Release();
			Swap(other);
		}
		return *this;
	}

	void Release();

	void Swap(VulkanImage& other) noexcept;

	[[nodiscard]] const vk::Image& Get() const { return m_image; }

private:
	VmaAllocator  m_allocator{ VK_NULL_HANDLE };
	vk::Image     m_image{ VK_NULL_HANDLE };
	VmaAllocation m_allocation{ VK_NULL_HANDLE };
};

#pragma endregion

#pragma region VulkanPipeline

struct VulkanPipelineOptions final
{
	vk::PrimitiveTopology Topology = vk::PrimitiveTopology::eTriangleList;
	vk::PolygonMode       PolygonMode = vk::PolygonMode::eFill;
	vk::CullModeFlags     CullMode = vk::CullModeFlagBits::eBack;
	vk::Bool32            DepthTestEnable = VK_TRUE;
	vk::Bool32            DepthWriteEnable = VK_TRUE;
	vk::CompareOp         DepthCompareOp = vk::CompareOp::eLess;
};

struct VulkanPipelineConfig final
{
	std::string           VertexShader;
	std::string           GeometryShader;
	std::string           FragmentShader;
	VulkanPipelineOptions Options;

	explicit VulkanPipelineConfig(const std::string& jsonFilename);
};

class VulkanRender;

class VulkanPipeline final
{
public:
	VulkanPipeline() = default;
	VulkanPipeline(
		VulkanRender& device,
		ShaderCompiler& compiler,
		const std::initializer_list<vk::DescriptorSetLayout>& descriptorSetLayouts,
		const std::initializer_list<vk::PushConstantRange>& pushConstantRanges,
		const vk::PipelineVertexInputStateCreateInfo* vertexInput,
		const std::string& pipelineConfigFile,
		const std::initializer_list<vk::PipelineColorBlendAttachmentState>& attachmentColorBlends,
		vk::RenderPass                                                      renderPass,
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

	[[nodiscard]] const vk::PipelineLayout& GetLayout() const { return m_pipelineLayout; }
	[[nodiscard]] const vk::Pipeline& Get() const { return m_pipeline; }

private:
	VulkanRender* m_device = nullptr;
	vk::PipelineLayout m_pipelineLayout;
	vk::ShaderModule   m_vertexShaderModule;
	vk::ShaderModule   m_geometryShaderModule;
	vk::ShaderModule   m_fragmentShaderModule;
	vk::Pipeline       m_pipeline;
};

#pragma endregion

#pragma region VulkanTexture

class VulkanTexture final
{
public:
	VulkanTexture() = default;
	// R8G8B8A8Unorm
	VulkanTexture(
		VulkanRender& device,
		uint32_t               width,
		uint32_t               height,
		const unsigned char* data,
		vk::Filter             filter,
		vk::SamplerAddressMode addressMode
	);
	VulkanTexture(const VulkanTexture&) = delete;
	VulkanTexture(VulkanTexture&& other) noexcept { Swap(other); }
	~VulkanTexture() { Release(); }

	VulkanTexture& operator=(const VulkanTexture&) = delete;
	VulkanTexture& operator=(VulkanTexture&& other) noexcept
	{
		if (this != &other)
		{
			Release();
			Swap(other);
		}
		return *this;
	}

	void Release();

	void Swap(VulkanTexture& other) noexcept;

	void BindToDescriptorSet(vk::DescriptorSet descriptorSet, uint32_t binding);

private:
	void createImage(uint32_t width, uint32_t height, vk::DeviceSize size, const void* data, vk::Format format);
	void createImageView(vk::Format format);
	void createSampler(vk::Filter filter, vk::SamplerAddressMode addressMode);

	VulkanRender* m_device = nullptr;
	VulkanImage   m_image;
	vk::ImageView m_imageView;
	vk::Sampler   m_sampler;
};

#pragma endregion

#pragma region VulkanMesh

class VulkanMesh final
{
public:
	VulkanMesh() = default;
	VulkanMesh(VulkanRender& device, size_t vertexCount, size_t vertexSize, const void* data);
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

	void BindAndDraw(vk::CommandBuffer commandBuffer) const;

private:
	VulkanBuffer m_vertexBuffer;
	uint32_t     m_vertexCount = 0;
};

#pragma endregion

#pragma region VertexFormats

struct VertexPositionOnly final
{
	glm::vec3 Position;

	VertexPositionOnly() = default;
	explicit VertexPositionOnly(const glm::vec3& position) : Position(position) {}

	static const vk::PipelineVertexInputStateCreateInfo* GetPipelineVertexInputStateCreateInfo();
};

struct VertexCanvas final
{
	glm::vec2 Position;
	glm::vec2 TexCoord;

	VertexCanvas() = default;
	VertexCanvas(const glm::vec2& position, const glm::vec2& texCoord)
		: Position(position)
		, TexCoord(texCoord) {}

	static const vk::PipelineVertexInputStateCreateInfo* GetPipelineVertexInputStateCreateInfo();
};

struct VertexBase final
{
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

class TextureCache final
{
public:
	explicit TextureCache(VulkanRender& device) : m_device(device) {}
	TextureCache(const TextureCache&) = delete;
	TextureCache(TextureCache&&) = delete;
	~TextureCache() = default;

	TextureCache& operator=(const TextureCache&) = delete;
	TextureCache& operator=(TextureCache&&) = delete;

	VulkanTexture* LoadTexture(
		const std::string& filename,
		vk::Filter             filter = vk::Filter::eNearest,
		vk::SamplerAddressMode addressMode = vk::SamplerAddressMode::eRepeat
	);

private:
	VulkanRender& m_device;
	std::map<std::string, VulkanTexture> m_textures;
};

#pragma endregion

#pragma region MeshCache

class MeshCache final
{
public:
	explicit MeshCache(VulkanRender& device) : m_device(device) {}
	MeshCache(const MeshCache&) = delete;
	MeshCache(MeshCache&&) = delete;
	~MeshCache() = default;

	MeshCache& operator=(const MeshCache&) = delete;
	MeshCache& operator=(MeshCache&&) = delete;

	VulkanMesh* LoadObjMesh(const std::string& filename);

private:
	VulkanRender& m_device;
	std::map<std::string, VulkanMesh> m_meshes;
};

#pragma endregion
