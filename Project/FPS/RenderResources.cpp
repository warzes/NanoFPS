#include "Base.h"
#include "Core.h"
#include "RenderResources.h"
#include "RenderContext.h"
#include "VulkanRender.h"

#pragma region ShaderCompiler

struct ShaderIncluder final : glslang::TShader::Includer
{
	explicit ShaderIncluder(std::string includesDir) : m_includesDir(std::move(includesDir)) {}

	IncludeResult* includeLocal(const char*, const char*, size_t) override
	{
		// relative paths are not supported yet
		return nullptr;
	}

	IncludeResult* includeSystem(const char* headerName, const char*, size_t) override { return include(headerName); }

	void releaseInclude(IncludeResult* result) override { delete result; }

private:
	IncludeResult* include(const std::string& headerName)
	{
		auto pair = m_headerFiles.find(headerName);
		if (pair == m_headerFiles.end())
		{
			Print("Loading shader header file <" + headerName + "> into cache.");
			pair = m_headerFiles.emplace(headerName, FileSystem::Read(m_includesDir + headerName)).first;
		}
		auto& content = pair->second;
		return new IncludeResult(headerName, content.c_str(), content.length(), nullptr);
	}

	std::string m_includesDir;

	std::map<std::string, std::string> m_headerFiles;
};

ShaderCompiler::ShaderCompiler(const std::string& includesDir)
{
	Print("glslang version: " + std::string(glslang::GetGlslVersionString()));
	if (!glslang::InitializeProcess())
		Fatal("Failed to initialize glslang.");
	m_includer = std::make_unique<ShaderIncluder>(includesDir);
}

ShaderCompiler::~ShaderCompiler()
{
	m_includer.reset();
	glslang::FinalizeProcess();
}

inline EShLanguage GetShaderStageLanguage(vk::ShaderStageFlagBits stage)
{
	switch (stage)
	{
	case vk::ShaderStageFlagBits::eVertex:   return EShLangVertex;
	case vk::ShaderStageFlagBits::eGeometry: return EShLangGeometry;
	case vk::ShaderStageFlagBits::eFragment: return EShLangFragment;
	case vk::ShaderStageFlagBits::eCompute:  return EShLangCompute;
	default: return EShLangCount;
	}
}

inline const char* GetShaderStageName(vk::ShaderStageFlagBits stage)
{
	switch (stage)
	{
	case vk::ShaderStageFlagBits::eVertex:   return "vertex";
	case vk::ShaderStageFlagBits::eGeometry: return "geometry";
	case vk::ShaderStageFlagBits::eFragment: return "fragment";
	case vk::ShaderStageFlagBits::eCompute:  return "compute";
	default: return "unsupported stage";
	}
}

std::vector<uint32_t> ShaderCompiler::Compile(vk::ShaderStageFlagBits stage, const std::string& source)
{
	const EShLanguage glslStage = GetShaderStageLanguage(stage);
	const char* stageName = GetShaderStageName(stage);

	glslang::TShader shader(glslStage);
	const char* sourceCStr = source.c_str();
	shader.setStrings(&sourceCStr, 1);
	shader.setPreamble("#extension GL_GOOGLE_include_directive : require\n");
	shader.setEnvInput(glslang::EShSourceGlsl, glslStage, glslang::EShClientVulkan, 100);
	shader.setEnvClient(glslang::EShClientVulkan, glslang::EShTargetVulkan_1_3);
	shader.setEnvTarget(glslang::EshTargetSpv, glslang::EShTargetSpv_1_0);

	if (!shader.parse(GetDefaultResources(), 100, false, EShMsgDefault, *m_includer))
	{
		Error("Failed to parse " + std::string(stageName) + " shader: " + shader.getInfoLog());
		return {};
	}
	else
	{
		const char* infoLog = shader.getInfoLog();
		if (std::strlen(infoLog))
		{
			Warning("Shader compilation warning: " + std::string(infoLog));
		}
	}

	glslang::TProgram program;
	program.addShader(&shader);

	if (!program.link(EShMsgDefault))
	{
		Error("Failed to link " + std::string(stageName) + " shader program: " + program.getInfoLog());
		return {};
	}

	std::vector<uint32_t> spirv;
	glslang::GlslangToSpv(*program.getIntermediate(glslStage), spirv);
	return spirv;
}

std::vector<uint32_t> ShaderCompiler::CompileFromFile(vk::ShaderStageFlagBits stage, const std::string& filename)
{
	const char* stageName = GetShaderStageName(stage);
	Print("Compiling " + std::string(stageName) + " shader: " + filename);
	return Compile(stage, FileSystem::Read(filename));
}

#pragma endregion

#pragma region VulkanBuffer

VulkanBuffer::VulkanBuffer(
	VmaAllocator             allocator,
	vk::DeviceSize           size,
	vk::BufferUsageFlags     bufferUsage,
	VmaAllocationCreateFlags flags,
	VmaMemoryUsage           memoryUsage
)
	: m_allocator(allocator)
{
	vk::BufferCreateInfo bufferCreateInfo({}, size, bufferUsage);

	VmaAllocationCreateInfo allocationCreateInfo{};
	allocationCreateInfo.flags = flags;
	allocationCreateInfo.usage = memoryUsage;

	if (vmaCreateBuffer(
		m_allocator,
		reinterpret_cast<const VkBufferCreateInfo*>(&bufferCreateInfo),
		&allocationCreateInfo,
		reinterpret_cast<VkBuffer*>(&m_buffer),
		&m_allocation,
		nullptr
	) != VK_SUCCESS)
	{
		Fatal("Failed to create Vulkan buffer.");
	}
}

void VulkanBuffer::Release()
{
	if (m_allocator)
	{
		vmaDestroyBuffer(m_allocator, m_buffer, m_allocation);
	}

	m_allocator = VK_NULL_HANDLE;
	m_buffer = VK_NULL_HANDLE;
	m_allocation = VK_NULL_HANDLE;
}

void VulkanBuffer::Swap(VulkanBuffer& other) noexcept
{
	std::swap(m_allocator, other.m_allocator);
	std::swap(m_buffer, other.m_buffer);
	std::swap(m_allocation, other.m_allocation);
}

void VulkanBuffer::UploadRange(size_t offset, size_t size, const void* data) const
{
	void* mappedMemory = nullptr;
	if (vmaMapMemory(m_allocator, m_allocation, &mappedMemory) != VK_SUCCESS)
	{
		Fatal("Failed to map Vulkan memory.");
		return;
	}

	memcpy(static_cast<unsigned char*>(mappedMemory) + offset, data, size);
	vmaUnmapMemory(m_allocator, m_allocation);
}

#pragma endregion

#pragma region VulkanUniformBufferSet

VulkanUniformBufferSet::VulkanUniformBufferSet(VulkanRender& device, const std::initializer_list<VulkanUniformBufferInfo>& uniformBufferInfos)
	: m_device(&device)
{
	const size_t numBuffering = m_device->GetNumBuffering();
	const size_t numUniformBuffers = uniformBufferInfos.size();

	std::vector<vk::DescriptorSetLayoutBinding> bindings;
	bindings.reserve(numUniformBuffers);
	for (const auto& uniformBufferInfo : uniformBufferInfos)
	{
		bindings.emplace_back(uniformBufferInfo.Binding, vk::DescriptorType::eUniformBufferDynamic, 1, uniformBufferInfo.Stage);
	}
	m_descriptorSetLayout = m_device->CreateDescriptorSetLayout(bindings);
	m_descriptorSet = m_device->AllocateDescriptorSet(m_descriptorSetLayout);

	m_uniformBufferSizes.reserve(numUniformBuffers);
	m_uniformBuffers.reserve(numUniformBuffers);
	for (const auto& uniformBufferInfo : uniformBufferInfos)
	{
		m_uniformBufferSizes.push_back(uniformBufferInfo.Size);

		VulkanBuffer uniformBuffer = m_device->CreateBuffer(
			numBuffering * uniformBufferInfo.Size,
			vk::BufferUsageFlagBits::eUniformBuffer,
			VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
			VMA_MEMORY_USAGE_AUTO_PREFER_HOST
		);

		m_device->WriteDynamicUniformBufferToDescriptorSet(uniformBuffer.Get(), uniformBufferInfo.Size, m_descriptorSet, uniformBufferInfo.Binding);

		m_uniformBuffers.push_back(std::move(uniformBuffer));
	}

	m_dynamicOffsets.reserve(numBuffering);
	for (int i = 0; i < numBuffering; i++)
	{
		std::vector<uint32_t> dynamicOffsets;
		dynamicOffsets.reserve(numUniformBuffers);
		for (int j = 0; j < numUniformBuffers; j++)
		{
			dynamicOffsets.push_back(i * m_uniformBufferSizes[j]);
		}
		m_dynamicOffsets.push_back(dynamicOffsets);
	}
}

void VulkanUniformBufferSet::Release()
{
	if (m_device)
	{
		m_device->FreeDescriptorSet(m_descriptorSet);
		m_device->DestroyDescriptorSetLayout(m_descriptorSetLayout);
	}

	m_device = nullptr;
	m_descriptorSetLayout = VK_NULL_HANDLE;
	m_uniformBufferSizes.clear();
	m_uniformBuffers.clear();
	m_descriptorSet = VK_NULL_HANDLE;
	m_dynamicOffsets.clear();
}

void VulkanUniformBufferSet::Swap(VulkanUniformBufferSet& other) noexcept
{
	std::swap(m_device, other.m_device);
	std::swap(m_descriptorSetLayout, other.m_descriptorSetLayout);
	std::swap(m_descriptorSet, other.m_descriptorSet);
	std::swap(m_uniformBufferSizes, other.m_uniformBufferSizes);
	std::swap(m_uniformBuffers, other.m_uniformBuffers);
	std::swap(m_dynamicOffsets, other.m_dynamicOffsets);
}

void VulkanUniformBufferSet::UpdateAllBuffers(uint32_t bufferingIndex, const std::initializer_list<const void*>& allBuffersData)
{
	int i = 0;
	for (const void* bufferData : allBuffersData)
	{
		const uint32_t size = m_uniformBufferSizes[i];
		const uint32_t offset = bufferingIndex * size;
		m_uniformBuffers[i].UploadRange(offset, size, bufferData);
		i++;
	}
}

#pragma endregion

#pragma region VulkanImage

VulkanImage::VulkanImage(
	VmaAllocator             allocator,
	vk::Format               format,
	const vk::Extent2D& extent,
	vk::ImageUsageFlags      imageUsage,
	VmaAllocationCreateFlags flags,
	VmaMemoryUsage           memoryUsage,
	uint32_t                 layers
)
	: m_allocator(allocator)
{
	vk::ImageCreateInfo imageCreateInfo(
		{},
		vk::ImageType::e2D,
		format,
		{ extent.width, extent.height, 1 },
		1,
		layers,
		vk::SampleCountFlagBits::e1,
		vk::ImageTiling::eOptimal,
		imageUsage
	);

	VmaAllocationCreateInfo allocationCreateInfo{};
	allocationCreateInfo.flags = flags;
	allocationCreateInfo.usage = memoryUsage;

	if (vmaCreateImage(
		m_allocator,
		reinterpret_cast<const VkImageCreateInfo*>(&imageCreateInfo),
		&allocationCreateInfo,
		reinterpret_cast<VkImage*>(&m_image),
		&m_allocation,
		nullptr
	) != VK_SUCCESS)
	{
		Fatal("Failed to create Vulkan 2d image.");
	}
}

void VulkanImage::Release()
{
	if (m_allocator)
	{
		vmaDestroyImage(m_allocator, m_image, m_allocation);
	}

	m_allocator = VK_NULL_HANDLE;
	m_image = VK_NULL_HANDLE;
	m_allocation = VK_NULL_HANDLE;
}

void VulkanImage::Swap(VulkanImage& other) noexcept
{
	std::swap(m_allocator, other.m_allocator);
	std::swap(m_image, other.m_image);
	std::swap(m_allocation, other.m_allocation);
}

#pragma endregion

#pragma region VulkanPipeline

inline vk::PrimitiveTopology TopologyFromString(const std::string& topology)
{
	if (topology == "point_list")          return vk::PrimitiveTopology::ePointList;
	else if (topology == "line_list")      return vk::PrimitiveTopology::eLineList;
	else if (topology == "line_strip")     return vk::PrimitiveTopology::eLineStrip;
	else if (topology == "triangle_list")  return vk::PrimitiveTopology::eTriangleList;
	else if (topology == "triangle_strip") return vk::PrimitiveTopology::eTriangleStrip;
	else if (topology == "triangle_fan")   return vk::PrimitiveTopology::eTriangleFan;
	Fatal("Invalid topology.");
	return {};
}

inline vk::PolygonMode PolygonModeFromString(const std::string& polygonMode)
{
	if (polygonMode == "fill")       return vk::PolygonMode::eFill;
	else if (polygonMode == "line")  return vk::PolygonMode::eLine;
	else if (polygonMode == "point") return vk::PolygonMode::ePoint;
	Fatal("Invalid polygon mode.");
	return {};
}

inline vk::CullModeFlags CullModeFromString(const std::string& cullMode)
{
	if (cullMode == "none")                return vk::CullModeFlagBits::eNone;
	else if (cullMode == "back")           return vk::CullModeFlagBits::eBack;
	else if (cullMode == "front")          return vk::CullModeFlagBits::eFront;
	else if (cullMode == "front_and_back") return vk::CullModeFlagBits::eFrontAndBack;
	Fatal("Invalid cull mode.");
	return {};
}

inline vk::CompareOp CompareOpFromString(const std::string& compareOp)
{
	if (compareOp == "never")                 return vk::CompareOp::eNever;
	else if (compareOp == "less")             return vk::CompareOp::eLess;
	else if (compareOp == "equal")            return vk::CompareOp::eEqual;
	else if (compareOp == "less_or_equal")    return vk::CompareOp::eLessOrEqual;
	else if (compareOp == "greater")          return vk::CompareOp::eGreater;
	else if (compareOp == "not_equal")        return vk::CompareOp::eNotEqual;
	else if (compareOp == "greater_or_equal") return vk::CompareOp::eGreaterOrEqual;
	else if (compareOp == "always")           return vk::CompareOp::eAlways;
	Fatal("Invalid compare op.");
	return {};
}

VulkanPipelineConfig::VulkanPipelineConfig(const std::string& jsonFilename)
{
	JsonFile pipelineJson(jsonFilename);
	VertexShader = pipelineJson.GetString("vertex");
	GeometryShader = pipelineJson.GetString("geometry", {});
	FragmentShader = pipelineJson.GetString("fragment");
	const std::string topology = pipelineJson.GetString("topology", {});
	Options.Topology = topology.empty() ? vk::PrimitiveTopology::eTriangleList : TopologyFromString(topology);
	const std::string polygonMode = pipelineJson.GetString("polygon_mode", {});
	Options.PolygonMode = polygonMode.empty() ? vk::PolygonMode::eFill : PolygonModeFromString(polygonMode);
	const std::string cullMode = pipelineJson.GetString("cull_mode", {});
	Options.CullMode = cullMode.empty() ? vk::CullModeFlagBits::eBack : CullModeFromString(cullMode);
	Options.DepthTestEnable = pipelineJson.GetBoolean("depth_test", true) ? VK_TRUE : VK_FALSE;
	Options.DepthWriteEnable = pipelineJson.GetBoolean("depth_write", true) ? VK_TRUE : VK_FALSE;
	const std::string compareOp = pipelineJson.GetString("compare_op", {});
	Options.DepthCompareOp = compareOp.empty() ? vk::CompareOp::eLess : CompareOpFromString(compareOp);
}

VulkanPipeline::VulkanPipeline(
	VulkanRender& device,
	ShaderCompiler& compiler,
	const std::initializer_list<vk::DescriptorSetLayout>& descriptorSetLayouts,
	const std::initializer_list<vk::PushConstantRange>& pushConstantRanges,
	const vk::PipelineVertexInputStateCreateInfo* vertexInput,
	const std::string& pipelineConfigFile,
	const std::initializer_list<vk::PipelineColorBlendAttachmentState>& attachmentColorBlends,
	vk::RenderPass                                                      renderPass,
	uint32_t                                                            subpass
)
	: m_device(&device)
{
	m_pipelineLayout = m_device->CreatePipelineLayout(descriptorSetLayouts, pushConstantRanges);

	const VulkanPipelineConfig pipelineConfig(pipelineConfigFile);

	const std::vector<uint32_t> vertexSpirv = compiler.CompileFromFile(vk::ShaderStageFlagBits::eVertex, pipelineConfig.VertexShader);
	m_vertexShaderModule = m_device->CreateShaderModule(vertexSpirv);
	if (!pipelineConfig.GeometryShader.empty())
	{
		const std::vector<uint32_t> geometrySpirv = compiler.CompileFromFile(vk::ShaderStageFlagBits::eGeometry, pipelineConfig.GeometryShader);
		m_geometryShaderModule = m_device->CreateShaderModule(geometrySpirv);
	}
	const std::vector<uint32_t> fragmentSpirv = compiler.CompileFromFile(vk::ShaderStageFlagBits::eFragment, pipelineConfig.FragmentShader);
	m_fragmentShaderModule = m_device->CreateShaderModule(fragmentSpirv);

	std::vector<vk::PipelineShaderStageCreateInfo> shaderStages;

	shaderStages.emplace_back(vk::PipelineShaderStageCreateFlags{}, vk::ShaderStageFlagBits::eVertex, m_vertexShaderModule, "main");
	if (!pipelineConfig.GeometryShader.empty())
	{
		shaderStages.emplace_back(vk::PipelineShaderStageCreateFlags{}, vk::ShaderStageFlagBits::eGeometry, m_geometryShaderModule, "main");
	}
	shaderStages.emplace_back(vk::PipelineShaderStageCreateFlags{}, vk::ShaderStageFlagBits::eFragment, m_fragmentShaderModule, "main");

	m_pipeline = m_device->CreatePipeline(m_pipelineLayout, vertexInput, shaderStages, pipelineConfig.Options, attachmentColorBlends, renderPass, subpass);
}

void VulkanPipeline::Release()
{
	if (m_device)
	{
		m_device->DestroyPipeline(m_pipeline);
		m_device->DestroyShaderModule(m_fragmentShaderModule);
		m_device->DestroyShaderModule(m_geometryShaderModule);
		m_device->DestroyShaderModule(m_vertexShaderModule);
		m_device->DestroyPipelineLayout(m_pipelineLayout);
	}

	m_device = nullptr;
	m_pipeline = VK_NULL_HANDLE;
	m_vertexShaderModule = VK_NULL_HANDLE;
	m_geometryShaderModule = VK_NULL_HANDLE;
	m_fragmentShaderModule = VK_NULL_HANDLE;
	m_pipelineLayout = VK_NULL_HANDLE;
}

void VulkanPipeline::Swap(VulkanPipeline& other) noexcept
{
	std::swap(m_device, other.m_device);
	std::swap(m_pipeline, other.m_pipeline);
	std::swap(m_vertexShaderModule, other.m_vertexShaderModule);
	std::swap(m_geometryShaderModule, other.m_geometryShaderModule);
	std::swap(m_fragmentShaderModule, other.m_fragmentShaderModule);
	std::swap(m_pipelineLayout, other.m_pipelineLayout);
}

#pragma endregion

#pragma region VulkanTexture

VulkanTexture::VulkanTexture(
	VulkanRender& device,
	uint32_t               width,
	uint32_t               height,
	const unsigned char* data,
	vk::Filter             filter,
	vk::SamplerAddressMode addressMode
)
	: m_device(&device)
{
	constexpr size_t PIXEL_SIZE = sizeof(unsigned char) * 4;
	createImage(width, height, width * height * PIXEL_SIZE, data, vk::Format::eR8G8B8A8Unorm);
	createImageView(vk::Format::eR8G8B8A8Unorm);
	createSampler(filter, addressMode);
}

void VulkanTexture::createImage(uint32_t width, uint32_t height, vk::DeviceSize size, const void* data, vk::Format format)
{
	VulkanBuffer uploadBuffer = m_device->CreateBuffer(
		size,
		vk::BufferUsageFlagBits::eTransferSrc,
		VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
		VMA_MEMORY_USAGE_AUTO_PREFER_HOST
	);
	uploadBuffer.Upload(size, data);

	VulkanImage image = m_device->CreateImage(
		format,
		VkExtent2D{ width, height },
		vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferDst,
		0,
		VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE
	);

	const vk::Extent3D extent{ width, height, 1 };

	m_device->ImmediateSubmit([extent, &uploadBuffer, &image](vk::CommandBuffer cmd) {
		vk::ImageMemoryBarrier imageMemoryBarrier(
			{},
			vk::AccessFlagBits::eTransferWrite,
			vk::ImageLayout::eUndefined,
			vk::ImageLayout::eTransferDstOptimal,
			{},
			{},
			image.Get(),
			{ vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1 }
		);
		cmd.pipelineBarrier(vk::PipelineStageFlagBits::eTopOfPipe, vk::PipelineStageFlagBits::eTransfer, {}, {}, {}, imageMemoryBarrier);

		const vk::BufferImageCopy imageCopy(0, 0, 0, { vk::ImageAspectFlagBits::eColor, 0, 0, 1 }, { 0, 0, 0 }, extent);
		cmd.copyBufferToImage(uploadBuffer.Get(), image.Get(), vk::ImageLayout::eTransferDstOptimal, imageCopy);

		imageMemoryBarrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
		imageMemoryBarrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;
		imageMemoryBarrier.oldLayout = vk::ImageLayout::eTransferDstOptimal;
		imageMemoryBarrier.newLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
		cmd.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eFragmentShader, {}, {}, {}, imageMemoryBarrier);
		});

	m_image = std::move(image);
}

void VulkanTexture::createImageView(vk::Format format)
{
	m_imageView = m_device->CreateImageView(m_image.Get(), format, vk::ImageAspectFlagBits::eColor);
}

void VulkanTexture::createSampler(vk::Filter filter, vk::SamplerAddressMode addressMode)
{
	m_sampler = m_device->CreateSampler(filter, addressMode);
}

void VulkanTexture::Release()
{
	if (m_device)
	{
		m_device->DestroySampler(m_sampler);
		m_device->DestroyImageView(m_imageView);
	}

	m_device = nullptr;
	m_image = {};
	m_imageView = VK_NULL_HANDLE;
	m_sampler = VK_NULL_HANDLE;
}

void VulkanTexture::Swap(VulkanTexture& other) noexcept
{
	std::swap(m_device, other.m_device);
	std::swap(m_image, other.m_image);
	std::swap(m_imageView, other.m_imageView);
	std::swap(m_sampler, other.m_sampler);
}

void VulkanTexture::BindToDescriptorSet(vk::DescriptorSet descriptorSet, uint32_t binding)
{
	m_device->WriteCombinedImageSamplerToDescriptorSet(m_sampler, m_imageView, descriptorSet, binding);
}

#pragma endregion

#pragma region VulkanMesh

VulkanMesh::VulkanMesh(VulkanRender& device, size_t vertexCount, size_t vertexSize, const void* data)
{
	const vk::DeviceSize size = vertexCount * vertexSize;

	VulkanBuffer uploadBuffer = device.CreateBuffer(
		size,
		vk::BufferUsageFlagBits::eTransferSrc,
		VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
		VMA_MEMORY_USAGE_AUTO_PREFER_HOST
	);
	uploadBuffer.Upload(size, data);

	VulkanBuffer vertexBuffer = device.CreateBuffer(
		size,
		vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eTransferDst,
		0,
		VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE
	);

	device.ImmediateSubmit([size, &uploadBuffer, &vertexBuffer](vk::CommandBuffer cmd) {
		const vk::BufferCopy copy(0, 0, size);
		cmd.copyBuffer(uploadBuffer.Get(), vertexBuffer.Get(), 1, &copy);
		});

	m_vertexBuffer = std::move(vertexBuffer);
	m_vertexCount = vertexCount;
}

void VulkanMesh::Release()
{
	m_vertexBuffer = {};
	m_vertexCount = 0;
}

void VulkanMesh::Swap(VulkanMesh& other) noexcept
{
	std::swap(m_vertexBuffer, other.m_vertexBuffer);
	std::swap(m_vertexCount, other.m_vertexCount);
}

void VulkanMesh::BindAndDraw(vk::CommandBuffer commandBuffer) const
{
	const vk::DeviceSize offset = 0;
	commandBuffer.bindVertexBuffers(0, 1, &m_vertexBuffer.Get(), &offset);
	commandBuffer.draw(m_vertexCount, 1, 0, 0);
}

#pragma endregion

#pragma region VertexFormats

const vk::PipelineVertexInputStateCreateInfo* VertexPositionOnly::GetPipelineVertexInputStateCreateInfo()
{
	static const std::vector<vk::VertexInputBindingDescription> bindings{
		{0, sizeof(VertexPositionOnly), vk::VertexInputRate::eVertex}
	};

	static const std::vector<vk::VertexInputAttributeDescription> attributes{
		{0, 0, vk::Format::eR32G32B32Sfloat, static_cast<uint32_t>(offsetof(VertexPositionOnly, Position))},
	};

	static const vk::PipelineVertexInputStateCreateInfo vertexInput{ {}, bindings, attributes };

	return &vertexInput;
}

const vk::PipelineVertexInputStateCreateInfo* VertexCanvas::GetPipelineVertexInputStateCreateInfo()
{
	static const std::vector<vk::VertexInputBindingDescription> bindings{
		{0, sizeof(VertexCanvas), vk::VertexInputRate::eVertex}
	};

	static const std::vector<vk::VertexInputAttributeDescription> attributes{
		{0, 0, vk::Format::eR32G32Sfloat, static_cast<uint32_t>(offsetof(VertexCanvas, Position))},
		{1, 0, vk::Format::eR32G32Sfloat, static_cast<uint32_t>(offsetof(VertexCanvas, TexCoord))}
	};

	static const vk::PipelineVertexInputStateCreateInfo vertexInput{ {}, bindings, attributes };

	return &vertexInput;
}

const vk::PipelineVertexInputStateCreateInfo* VertexBase::GetPipelineVertexInputStateCreateInfo()
{
	static const std::vector<vk::VertexInputBindingDescription> bindings{
		{0, sizeof(VertexBase), vk::VertexInputRate::eVertex}
	};

	static const std::vector<vk::VertexInputAttributeDescription> attributes{
		{0, 0, vk::Format::eR32G32B32Sfloat, static_cast<uint32_t>(offsetof(VertexBase, Position))},
		{1, 0, vk::Format::eR32G32B32Sfloat, static_cast<uint32_t>(offsetof(VertexBase, Normal))  },
		{2, 0, vk::Format::eR32G32Sfloat,    static_cast<uint32_t>(offsetof(VertexBase, TexCoord))}
	};

	static const vk::PipelineVertexInputStateCreateInfo vertexInput{ {}, bindings, attributes };

	return &vertexInput;
}

#pragma endregion

#pragma region TextureCache

VulkanTexture* TextureCache::LoadTexture(const std::string& filename, vk::Filter filter, vk::SamplerAddressMode addressMode)
{
	auto pair = m_textures.find(filename);
	if (pair == m_textures.end())
	{
		Print("Caching texture " + filename);
		const ImageFile image(filename);
		pair = m_textures.emplace(filename, VulkanTexture(m_device, image.GetWidth(), image.GetHeight(), image.GetData(), filter, addressMode)).first;
	}
	return &pair->second;
}

#pragma endregion

#pragma region MeshCache

std::vector<VertexBase> LoadObj(const std::string& filename)
{
	tinyobj::ObjReader reader;

	if (!reader.ParseFromString(FileSystem::Read(filename), ""))
		Fatal("Failed to load OBJ file " + filename + ": " + reader.Error());
	if (!reader.Warning().empty())
		Warning("Warning when loading OBJ file " + filename + ": " + reader.Warning());

	const tinyobj::attrib_t& attrib = reader.GetAttrib();
	const std::vector<tinyobj::real_t>& positions = attrib.vertices;
	const std::vector<tinyobj::real_t>& normals = attrib.normals;
	const std::vector<tinyobj::real_t>& texCoords = attrib.texcoords;

	std::vector<VertexBase> vertices;
	for (const tinyobj::shape_t& shape : reader.GetShapes())
	{
		const tinyobj::mesh_t& mesh = shape.mesh;
		Print("Loading OBJ shape " + shape.name);

		size_t i = 0;
		for (const size_t f : mesh.num_face_vertices)
		{
			if (f != 3) Fatal("All polygons must be triangle.");

			for (size_t v = 0; v < f; v++)
			{
				const tinyobj::index_t& index = mesh.indices[i + v];
				if (!(index.normal_index >= 0))
					Fatal("Missing normal data on vertex " + std::to_string(f) + "/" + std::to_string(v));
				if (!(index.texcoord_index >= 0))
					Fatal("Missing texture coordinates data on vertex " + std::to_string(f) + "/" + std::to_string(v));

				// X axis is flipped because Blender uses right-handed coordinates
				vertices.emplace_back(
					glm::vec3{ -positions[3 * index.vertex_index + 0], positions[3 * index.vertex_index + 1], positions[3 * index.vertex_index + 2] },
					glm::vec3{ -normals[3 * index.normal_index + 0], normals[3 * index.normal_index + 1], normals[3 * index.normal_index + 2] },
					glm::vec2{ texCoords[2 * index.texcoord_index + 0], texCoords[2 * index.texcoord_index + 1] }
				);
			}
			i += f;
		}
	}
	return vertices;
}

VulkanMesh* MeshCache::LoadObjMesh(const std::string& filename)
{
	auto pair = m_meshes.find(filename);
	if (pair == m_meshes.end())
	{
		Print("Caching OBJ mesh " + filename);
		const std::vector<VertexBase> vertices = LoadObj(filename);
		pair = m_meshes.emplace(filename, VulkanMesh(m_device, vertices.size(), sizeof(VertexBase), vertices.data())).first;
	}
	return &pair->second;
}

#pragma endregion
