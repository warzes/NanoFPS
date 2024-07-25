#include "Base.h"
#include "Core.h"
#include "RenderResources.h"
#include "RenderContext.h"

struct sBufferingObjects
{
	VkFence RenderFence;
	VkSemaphore PresentSemaphore;
	VkSemaphore RenderSemaphore;
	VkCommandBuffer CommandBuffer;
};

namespace
{
	std::array<sBufferingObjects, 3> BufferingObjects;

	std::map<std::string, VulkanTexture> TexturesCache;

	std::map<std::string, VulkanMesh> MeshesCache;
}

#pragma region VulkanImage

VulkanImage::VulkanImage(VmaAllocator allocator, VkFormat format, const VkExtent2D& extent, VkImageUsageFlags imageUsage, VmaAllocationCreateFlags flags, VmaMemoryUsage memoryUsage, uint32_t layers)
	: m_allocator(allocator)
{
	VkImageCreateInfo imageInfo{ .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };
	imageInfo.imageType = VK_IMAGE_TYPE_2D;
	imageInfo.format = format;
	imageInfo.extent.width = extent.width;
	imageInfo.extent.height = extent.height;
	imageInfo.extent.depth = 1;
	imageInfo.mipLevels = 1;
	imageInfo.arrayLayers = layers;
	imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
	imageInfo.usage = imageUsage;

	VmaAllocationCreateInfo allocationCreateInfo{};
	allocationCreateInfo.flags = flags;
	allocationCreateInfo.usage = memoryUsage;

	if (vmaCreateImage(m_allocator, &imageInfo, &allocationCreateInfo, &m_image, &m_allocation, nullptr) != VK_SUCCESS)
	{
		Fatal("Failed to create Vulkan 2d image.");
		m_image = nullptr;
		return;
	}
}

VulkanImage::VulkanImage(VulkanImage&& other) noexcept
{
	Swap(other);
}

VulkanImage::~VulkanImage()
{
	Release();
}

VulkanImage& VulkanImage::operator=(VulkanImage&& other) noexcept
{
	if (this != &other)
	{
		Release();
		Swap(other);
	}
	return *this;
}

void VulkanImage::Release()
{
	if (m_allocator)
		vmaDestroyImage(m_allocator, m_image, m_allocation);

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

#pragma region VulkanBuffer

VulkanBuffer::VulkanBuffer(VmaAllocator allocator, VkDeviceSize size, VkBufferUsageFlags bufferUsage, VmaAllocationCreateFlags flags, VmaMemoryUsage memoryUsage)
	: m_allocator(allocator)
{
	VkBufferCreateInfo bufferCreateInfo{ .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
	bufferCreateInfo.size = size;
	bufferCreateInfo.usage = bufferUsage;
	
	VmaAllocationCreateInfo allocationCreateInfo{};
	allocationCreateInfo.flags = flags;
	allocationCreateInfo.usage = memoryUsage;

	if (vmaCreateBuffer(m_allocator, &bufferCreateInfo, &allocationCreateInfo, &m_buffer, &m_allocation, nullptr) != VK_SUCCESS)
	{
		Fatal("Failed to create Vulkan buffer.");
		m_buffer = nullptr;
		return;
	}
}

VulkanBuffer::VulkanBuffer(VulkanBuffer&& other) noexcept
{
	Swap(other);
}

VulkanBuffer::~VulkanBuffer()
{
	Release();
}

void VulkanBuffer::Release()
{
	if (m_allocator)
		vmaDestroyBuffer(m_allocator, m_buffer, m_allocation);

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

#pragma region VulkanTexture

VulkanTexture::VulkanTexture(uint32_t width, uint32_t height, const unsigned char* data, VkFilter filter, VkSamplerAddressMode addressMode)
{
	constexpr size_t PIXEL_SIZE = sizeof(unsigned char) * 4;
	createImage(width, height, width * height * PIXEL_SIZE, data, VK_FORMAT_R8G8B8A8_UNORM);
	createImageView(VK_FORMAT_R8G8B8A8_UNORM);
	createSampler(filter, addressMode);
}

VulkanTexture::VulkanTexture(VulkanTexture&& other) noexcept
{
	Swap(other);
}

VulkanTexture::~VulkanTexture()
{
	Release();
}

VulkanTexture& VulkanTexture::operator=(VulkanTexture&& other) noexcept
{
	if (this != &other)
	{
		Release();
		Swap(other);
	}
	return *this;
}

void VulkanTexture::Release()
{
	Render::DestroySampler(m_sampler);
	Render::DestroyImageView(m_imageView);

	m_image = {};
	m_imageView = VK_NULL_HANDLE;
	m_sampler = VK_NULL_HANDLE;
}

void VulkanTexture::Swap(VulkanTexture& other) noexcept
{
	std::swap(m_image, other.m_image);
	std::swap(m_imageView, other.m_imageView);
	std::swap(m_sampler, other.m_sampler);
}

void VulkanTexture::BindToDescriptorSet(VkDescriptorSet descriptorSet, uint32_t binding)
{
	Render::WriteCombinedImageSamplerToDescriptorSet(m_sampler, m_imageView, descriptorSet, binding);
}

void VulkanTexture::createImage(uint32_t width, uint32_t height, VkDeviceSize size, const void* data, VkFormat format)
{
	VulkanBuffer uploadBuffer = Render::CreateBuffer(
		size,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
		VMA_MEMORY_USAGE_AUTO_PREFER_HOST
	);
	uploadBuffer.Upload(size, data);

	VulkanImage image = Render::CreateImage(
		format,
		VkExtent2D{ width, height },
		VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
		0,
		VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE
	);

	const VkExtent3D extent{ width, height, 1 };

	Render::BeginImmediateSubmit();
	{
		VkImageMemoryBarrier imageMemoryBarrier{ .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
		imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		imageMemoryBarrier.image = image.Get();
		imageMemoryBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		imageMemoryBarrier.subresourceRange.baseMipLevel = 0;
		imageMemoryBarrier.subresourceRange.levelCount = 1;
		imageMemoryBarrier.subresourceRange.baseArrayLayer = 0;
		imageMemoryBarrier.subresourceRange.layerCount = 1;

		vkCmdPipelineBarrier(
			RenderContext::GetInstance().ImmediateCommandBuffer,
			VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
			0,
			0, nullptr,
			0, nullptr,
			1, &imageMemoryBarrier);

		VkBufferImageCopy imageCopy{};
		imageCopy.bufferOffset = 0;
		imageCopy.bufferRowLength = 0;
		imageCopy.bufferImageHeight = 0;

		imageCopy.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		imageCopy.imageSubresource.mipLevel = 0;
		imageCopy.imageSubresource.baseArrayLayer = 0;
		imageCopy.imageSubresource.layerCount = 1;

		imageCopy.imageOffset = { 0, 0, 0 };
		imageCopy.imageExtent = extent;

		vkCmdCopyBufferToImage(
			RenderContext::GetInstance().ImmediateCommandBuffer,
			uploadBuffer.Get(),
			image.Get(),
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			1,
			&imageCopy
		);


		imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		imageMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
		imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

		vkCmdPipelineBarrier(RenderContext::GetInstance().ImmediateCommandBuffer,
			VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
			0, 0, 0, 0, 0,
			1, &imageMemoryBarrier);
	}
	Render::EndImmediateSubmit();

	m_image = std::move(image);
}

void VulkanTexture::createImageView(VkFormat format)
{
	m_imageView = Render::CreateImageView(m_image.Get(), format, VK_IMAGE_ASPECT_COLOR_BIT);
}

void VulkanTexture::createSampler(VkFilter filter, VkSamplerAddressMode addressMode)
{
	m_sampler = Render::CreateSampler(filter, addressMode);
}

#pragma endregion

#pragma region VulkanUniformBufferSet

VulkanUniformBufferSet::VulkanUniformBufferSet(const std::initializer_list<VulkanUniformBufferInfo>& uniformBufferInfos)
{
	const size_t numBuffering = Render::GetNumBuffering();
	const size_t numUniformBuffers = uniformBufferInfos.size();

	std::vector<VkDescriptorSetLayoutBinding> bindings;
	bindings.reserve(numUniformBuffers);
	for (const auto& uniformBufferInfo : uniformBufferInfos)
	{
		bindings.emplace_back(uniformBufferInfo.Binding, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1, uniformBufferInfo.Stage);
	}
	m_descriptorSetLayout = Render::CreateDescriptorSetLayout(bindings);
	m_descriptorSet = Render::AllocateDescriptorSet(m_descriptorSetLayout);

	m_uniformBufferSizes.reserve(numUniformBuffers);
	m_uniformBuffers.reserve(numUniformBuffers);
	for (const auto& uniformBufferInfo : uniformBufferInfos) {
		m_uniformBufferSizes.push_back(uniformBufferInfo.Size);

		VulkanBuffer uniformBuffer = Render::CreateBuffer(
			numBuffering * uniformBufferInfo.Size,
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
			VMA_MEMORY_USAGE_AUTO_PREFER_HOST
		);

		Render::WriteDynamicUniformBufferToDescriptorSet(uniformBuffer.Get(), uniformBufferInfo.Size, m_descriptorSet, uniformBufferInfo.Binding);

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

VulkanUniformBufferSet::VulkanUniformBufferSet(VulkanUniformBufferSet&& other) noexcept
{
	Swap(other);
}

VulkanUniformBufferSet::~VulkanUniformBufferSet()
{
	Release();
}

VulkanUniformBufferSet& VulkanUniformBufferSet::operator=(VulkanUniformBufferSet&& other) noexcept
{
	if (this != &other)
	{
		Release();
		Swap(other);
	}
	return *this;
}

void VulkanUniformBufferSet::Release()
{
	Render::FreeDescriptorSet(m_descriptorSet);
	Render::DestroyDescriptorSetLayout(m_descriptorSetLayout);

	m_descriptorSetLayout = VK_NULL_HANDLE;
	m_uniformBufferSizes.clear();
	m_uniformBuffers.clear();
	m_descriptorSet = VK_NULL_HANDLE;
	m_dynamicOffsets.clear();
}

void VulkanUniformBufferSet::Swap(VulkanUniformBufferSet& other) noexcept
{
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

#pragma region ShaderCompiler

struct ShaderIncluder : glslang::TShader::Includer
{
	explicit ShaderIncluder(std::string includesDir) : m_includesDir(std::move(includesDir)) {}

	IncludeResult* includeLocal(const char*, const char*, size_t) override
	{
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
	{
		Fatal("Failed to initialize glslang.");
		return;
	}
	m_includer = std::make_unique<ShaderIncluder>(includesDir);
}

ShaderCompiler::~ShaderCompiler()
{
	m_includer.reset();
	glslang::FinalizeProcess();
}

static inline EShLanguage GetShaderStageLanguage(VkShaderStageFlagBits stage)
{
	switch (stage) {
	case VK_SHADER_STAGE_VERTEX_BIT:
		return EShLangVertex;
	case VK_SHADER_STAGE_GEOMETRY_BIT:
		return EShLangGeometry;
	case VK_SHADER_STAGE_FRAGMENT_BIT:
		return EShLangFragment;
	case VK_SHADER_STAGE_COMPUTE_BIT:
		return EShLangCompute;
	default:
		return EShLangCount;
	}
}

static inline const char* GetShaderStageName(VkShaderStageFlagBits stage) {
	switch (stage) {
	case VK_SHADER_STAGE_VERTEX_BIT:
		return "vertex";
	case VK_SHADER_STAGE_GEOMETRY_BIT:
		return "geometry";
	case VK_SHADER_STAGE_FRAGMENT_BIT:
		return "fragment";
	case VK_SHADER_STAGE_COMPUTE_BIT:
		return "compute";
	default:
		return "unsupported stage";
	}
}

std::vector<uint32_t> ShaderCompiler::Compile(VkShaderStageFlagBits stage, const std::string& source)
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

	if (!shader.parse(GetDefaultResources(), 100, false, EShMsgDefault, *m_includer)) {
		Fatal("Failed to parse " + std::string(stageName) + " shader: " + std::string(shader.getInfoLog()));
		return {};
	}
	else {
		const char* infoLog = shader.getInfoLog();
		if (std::strlen(infoLog)) {
			Warning("Shader compilation warning: " +  std::string(infoLog));
		}
	}

	glslang::TProgram program;
	program.addShader(&shader);

	if (!program.link(EShMsgDefault))
	{
		Fatal("Failed to link " + std::string(stageName) + " shader program: " + std::string(program.getInfoLog()));
		return {};
	}

	std::vector<uint32_t> spirv;
	glslang::GlslangToSpv(*program.getIntermediate(glslStage), spirv);
	return spirv;
}

std::vector<uint32_t> ShaderCompiler::CompileFromFile(VkShaderStageFlagBits stage, const std::string& filename)
{
	const char* stageName = GetShaderStageName(stage);
	Print("Compiling " + std::string(stageName) + " shader: " + filename);
	return Compile(stage, FileSystem::Read(filename));
}

#pragma endregion

#pragma region VulkanPipeline

VkPrimitiveTopology TopologyFromString(const std::string& topology) {
	if (topology == "point_list") {
		return VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
	}
	else if (topology == "line_list") {
		return VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
	}
	else if (topology == "line_strip") {
		return VK_PRIMITIVE_TOPOLOGY_LINE_STRIP;
	}
	else if (topology == "triangle_list") {
		return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	}
	else if (topology == "triangle_strip") {
		return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
	}
	else if (topology == "triangle_fan") {
		return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN;
	}
	Fatal("Invalid topology: " + topology);
	return {}; // TODO: optional
}

VkPolygonMode PolygonModeFromString(const std::string& polygonMode)
{
	if (polygonMode == "fill") {
		return VK_POLYGON_MODE_FILL;
	}
	else if (polygonMode == "line") {
		return VK_POLYGON_MODE_LINE;
	}
	else if (polygonMode == "point") {
		return VK_POLYGON_MODE_POINT;
	}
	Fatal("Invalid polygon mode: " + polygonMode);
	return {}; // TODO: optional
}

VkCullModeFlags CullModeFromString(const std::string& cullMode)
{
	if (cullMode == "none") {
		return VK_CULL_MODE_NONE;
	}
	else if (cullMode == "back") {
		return VK_CULL_MODE_BACK_BIT;
	}
	else if (cullMode == "front") {
		return VK_CULL_MODE_FRONT_BIT;
	}
	else if (cullMode == "front_and_back") {
		return VK_CULL_MODE_FRONT_AND_BACK;
	}
	Fatal("Invalid cull mode: " + cullMode);
	return {};
}

VkCompareOp CompareOpFromString(const std::string& compareOp) {
	if (compareOp == "never") {
		return VK_COMPARE_OP_NEVER;
	}
	else if (compareOp == "less") {
		return VK_COMPARE_OP_LESS;
	}
	else if (compareOp == "equal") {
		return VK_COMPARE_OP_EQUAL;
	}
	else if (compareOp == "less_or_equal") {
		return VK_COMPARE_OP_LESS_OR_EQUAL;
	}
	else if (compareOp == "greater") {
		return VK_COMPARE_OP_GREATER;
	}
	else if (compareOp == "not_equal") {
		return VK_COMPARE_OP_NOT_EQUAL;
	}
	else if (compareOp == "greater_or_equal") {
		return VK_COMPARE_OP_GREATER_OR_EQUAL;
	}
	else if (compareOp == "always") {
		return VK_COMPARE_OP_ALWAYS;
	}
	Fatal("Invalid compare op: " + compareOp);
	return {};
}

VulkanPipelineConfig::VulkanPipelineConfig(const std::string& jsonFilename)
{
	JsonFile pipelineJson(jsonFilename);
	VertexShader = pipelineJson.GetString("vertex");
	GeometryShader = pipelineJson.GetString("geometry", {});
	FragmentShader = pipelineJson.GetString("fragment");
	const std::string topology = pipelineJson.GetString("topology", {});
	Options.Topology = topology.empty() ? VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST : TopologyFromString(topology);
	const std::string polygonMode = pipelineJson.GetString("polygon_mode", {});
	Options.PolygonMode = polygonMode.empty() ? VK_POLYGON_MODE_FILL : PolygonModeFromString(polygonMode);
	const std::string cullMode = pipelineJson.GetString("cull_mode", {});
	Options.CullMode = cullMode.empty() ? VK_CULL_MODE_BACK_BIT : CullModeFromString(cullMode);
	Options.DepthTestEnable = pipelineJson.GetBoolean("depth_test", true) ? VK_TRUE : VK_FALSE;
	Options.DepthWriteEnable = pipelineJson.GetBoolean("depth_write", true) ? VK_TRUE : VK_FALSE;
	const std::string compareOp = pipelineJson.GetString("compare_op", {});
	Options.DepthCompareOp = compareOp.empty() ? VK_COMPARE_OP_LESS : CompareOpFromString(compareOp);
}

VulkanPipeline::VulkanPipeline(
	ShaderCompiler& compiler,
	const std::initializer_list<VkDescriptorSetLayout>& descriptorSetLayouts,
	const std::initializer_list<VkPushConstantRange>& pushConstantRanges,
	const VkPipelineVertexInputStateCreateInfo* vertexInput,
	const std::string& pipelineConfigFile,
	const std::initializer_list<VkPipelineColorBlendAttachmentState>& attachmentColorBlends,
	VkRenderPass renderPass,
	uint32_t subpass
)
{
	m_pipelineLayout = Render::CreatePipelineLayout(descriptorSetLayouts, pushConstantRanges);

	const VulkanPipelineConfig pipelineConfig(pipelineConfigFile);

	const std::vector<uint32_t> vertexSpirv = compiler.CompileFromFile(VK_SHADER_STAGE_VERTEX_BIT, pipelineConfig.VertexShader);
	m_vertexShaderModule = Render::CreateShaderModule(vertexSpirv);
	if (!pipelineConfig.GeometryShader.empty())
	{
		const std::vector<uint32_t> geometrySpirv = compiler.CompileFromFile(VK_SHADER_STAGE_GEOMETRY_BIT, pipelineConfig.GeometryShader);
		m_geometryShaderModule = Render::CreateShaderModule(geometrySpirv);
	}
	const std::vector<uint32_t> fragmentSpirv = compiler.CompileFromFile(VK_SHADER_STAGE_FRAGMENT_BIT, pipelineConfig.FragmentShader);
	m_fragmentShaderModule = Render::CreateShaderModule(fragmentSpirv);


	std::vector<VkPipelineShaderStageCreateInfo> shaderStages;

	VkPipelineShaderStageCreateInfo vertShaderStageInfo = { .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO };
	vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
	vertShaderStageInfo.module = m_vertexShaderModule;
	vertShaderStageInfo.pName = "main";
	shaderStages.push_back(vertShaderStageInfo);

	if (!pipelineConfig.GeometryShader.empty())
	{
		VkPipelineShaderStageCreateInfo geomShaderStageInfo = { .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO };
		geomShaderStageInfo.stage = VK_SHADER_STAGE_GEOMETRY_BIT;
		geomShaderStageInfo.module = m_geometryShaderModule;
		geomShaderStageInfo.pName = "main";
		shaderStages.push_back(geomShaderStageInfo);
	}

	VkPipelineShaderStageCreateInfo fragShaderStageInfo = { .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO };
	fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	fragShaderStageInfo.module = m_fragmentShaderModule;
	fragShaderStageInfo.pName = "main";
	shaderStages.push_back(fragShaderStageInfo);

	m_pipeline = Render::CreatePipeline(m_pipelineLayout, vertexInput, shaderStages, pipelineConfig.Options, attachmentColorBlends, renderPass, subpass);
}

void VulkanPipeline::Release()
{
	{
		vkDestroyShaderModule(RenderContext::GetInstance().Device, m_fragmentShaderModule, nullptr);
		vkDestroyShaderModule(RenderContext::GetInstance().Device, m_geometryShaderModule, nullptr);
		vkDestroyShaderModule(RenderContext::GetInstance().Device, m_vertexShaderModule, nullptr);
		vkDestroyPipelineLayout(RenderContext::GetInstance().Device, m_pipelineLayout, nullptr);
	}

	m_pipeline = VK_NULL_HANDLE;
	m_vertexShaderModule = VK_NULL_HANDLE;
	m_geometryShaderModule = VK_NULL_HANDLE;
	m_fragmentShaderModule = VK_NULL_HANDLE;
	m_pipelineLayout = VK_NULL_HANDLE;
}

void VulkanPipeline::Swap(VulkanPipeline& other) noexcept
{
	std::swap(m_pipeline, other.m_pipeline);
	std::swap(m_vertexShaderModule, other.m_vertexShaderModule);
	std::swap(m_geometryShaderModule, other.m_geometryShaderModule);
	std::swap(m_fragmentShaderModule, other.m_fragmentShaderModule);
	std::swap(m_pipelineLayout, other.m_pipelineLayout);
}

#pragma endregion

#pragma region VulkanMesh

VulkanMesh::VulkanMesh(size_t vertexCount, size_t vertexSize, const void* data)
{
	const VkDeviceSize size = vertexCount * vertexSize;

	VulkanBuffer uploadBuffer = Render::CreateBuffer(
		size,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
		VMA_MEMORY_USAGE_AUTO_PREFER_HOST
	);
	uploadBuffer.Upload(size, data);

	VulkanBuffer vertexBuffer = Render::CreateBuffer(
		size,
		VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		0,
		VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE
	);

	Render::BeginImmediateSubmit();
	{
		VkBufferCopy copy{};
		copy.size = size;
		vkCmdCopyBuffer(RenderContext::GetInstance().ImmediateCommandBuffer, uploadBuffer.Get(), vertexBuffer.Get(), 1, &copy);
	}
	Render::EndImmediateSubmit();

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

void VulkanMesh::BindAndDraw(VkCommandBuffer commandBuffer) const
{
	const VkDeviceSize offset = 0;

	vkCmdBindVertexBuffers(commandBuffer, 0, 1, &m_vertexBuffer.Get(), &offset);
	vkCmdDraw(commandBuffer, m_vertexCount, 1, 0, 0);
}

#pragma endregion

#pragma region VertexFormats

const VkPipelineVertexInputStateCreateInfo* VertexPositionOnly::GetPipelineVertexInputStateCreateInfo()
{
	VkVertexInputBindingDescription bindings{}; // TODO: static
	bindings.binding = 0;
	bindings.stride = sizeof(VertexPositionOnly);
	bindings.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

	VkVertexInputAttributeDescription attrib;// TODO: static
	attrib.binding = 0;
	attrib.location = 0;
	attrib.format = VK_FORMAT_R32G32B32_SFLOAT;
	attrib.offset = offsetof(VertexPositionOnly, Position);

	std::vector<VkVertexInputAttributeDescription> attributes{ attrib };// TODO: static

	VkPipelineVertexInputStateCreateInfo vertexInput = { .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO };// TODO: static
	vertexInput.vertexBindingDescriptionCount = 1;
	vertexInput.pVertexBindingDescriptions = &bindings;
	vertexInput.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributes.size());
	vertexInput.pVertexAttributeDescriptions = attributes.data();

	return &vertexInput;
}

const VkPipelineVertexInputStateCreateInfo* VertexCanvas::GetPipelineVertexInputStateCreateInfo()
{
	VkVertexInputBindingDescription bindings{}; // TODO: static
	bindings.binding = 0;
	bindings.stride = sizeof(VertexCanvas);
	bindings.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

	VkVertexInputAttributeDescription attrib1;// TODO: static
	attrib1.binding = 0;
	attrib1.location = 0;
	attrib1.format = VK_FORMAT_R32G32_SFLOAT;
	attrib1.offset = offsetof(VertexCanvas, Position);
	VkVertexInputAttributeDescription attrib2;// TODO: static
	attrib2.binding = 0;
	attrib2.location = 1;
	attrib2.format = VK_FORMAT_R32G32_SFLOAT;
	attrib2.offset = offsetof(VertexCanvas, TexCoord);

	std::vector<VkVertexInputAttributeDescription> attributes{ attrib1, attrib2 };// TODO: static

	VkPipelineVertexInputStateCreateInfo vertexInput = { .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO };// TODO: static
	vertexInput.vertexBindingDescriptionCount = 1;
	vertexInput.pVertexBindingDescriptions = &bindings;
	vertexInput.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributes.size());
	vertexInput.pVertexAttributeDescriptions = attributes.data();

	return &vertexInput;
}

const VkPipelineVertexInputStateCreateInfo* VertexBase::GetPipelineVertexInputStateCreateInfo()
{
	VkVertexInputBindingDescription bindings{}; // TODO: static
	bindings.binding = 0;
	bindings.stride = sizeof(VertexBase);
	bindings.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

	VkVertexInputAttributeDescription attrib1;// TODO: static
	attrib1.binding = 0;
	attrib1.location = 0;
	attrib1.format = VK_FORMAT_R32G32B32_SFLOAT;
	attrib1.offset = offsetof(VertexBase, Position);
	VkVertexInputAttributeDescription attrib2;// TODO: static
	attrib2.binding = 0;
	attrib2.location = 1;
	attrib2.format = VK_FORMAT_R32G32B32_SFLOAT;
	attrib2.offset = offsetof(VertexBase, Normal);
	VkVertexInputAttributeDescription attrib3;// TODO: static
	attrib3.binding = 0;
	attrib3.location = 2;
	attrib3.format = VK_FORMAT_R32G32_SFLOAT;
	attrib3.offset = offsetof(VertexBase, TexCoord);


	std::vector<VkVertexInputAttributeDescription> attributes{ attrib1, attrib2, attrib3 };// TODO: static

	VkPipelineVertexInputStateCreateInfo vertexInput = { .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO };// TODO: static
	vertexInput.vertexBindingDescriptionCount = 1;
	vertexInput.pVertexBindingDescriptions = &bindings;
	vertexInput.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributes.size());
	vertexInput.pVertexAttributeDescriptions = attributes.data();

	return &vertexInput;
}

#pragma endregion

#pragma region TextureCache


#pragma endregion

#pragma region Renderer

size_t Render::GetNumBuffering()
{
	return BufferingObjects.size();
}

VulkanImage Render::CreateImage(VkFormat format, const VkExtent2D& extent, VkImageUsageFlags imageUsage, VmaAllocationCreateFlags flags, VmaMemoryUsage memoryUsage, uint32_t layers)
{
	return { RenderContext::GetInstance().Allocator, format, extent, imageUsage, flags, memoryUsage, layers };
}

VkImageView Render::CreateImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectMask, uint32_t layers)
{
	VkImageViewCreateInfo viewInfo{ .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
	viewInfo.image = image;
	viewInfo.viewType = layers == 1 ? VK_IMAGE_VIEW_TYPE_2D : VK_IMAGE_VIEW_TYPE_2D_ARRAY;
	viewInfo.format = format;
	viewInfo.subresourceRange.aspectMask = aspectMask;
	viewInfo.subresourceRange.baseMipLevel = 0;
	viewInfo.subresourceRange.levelCount = 1;
	viewInfo.subresourceRange.baseArrayLayer = 0;
	viewInfo.subresourceRange.layerCount = layers;

	VkImageView imageView;
	if (vkCreateImageView(RenderContext::GetInstance().Device, &viewInfo, nullptr, &imageView) != VK_SUCCESS)
	{
		Fatal("Failed to create Vulkan image view.");
		return {}; // TODO: return optional<VkImageView>
	}
	return imageView;
}

VkSampler Render::CreateSampler(VkFilter filter, VkSamplerAddressMode addressMode, VkBool32 compareEnable, VkCompareOp compareOp)
{
	VkSamplerCreateInfo samplerInfo{ .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO };
	samplerInfo.magFilter = filter;
	samplerInfo.minFilter = filter;
	samplerInfo.addressModeU = addressMode;
	samplerInfo.addressModeV = addressMode;
	samplerInfo.addressModeW = addressMode;
	samplerInfo.compareEnable = compareEnable;
	samplerInfo.compareOp = compareOp;
	samplerInfo.borderColor = VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK;

	VkSampler sampler;
	if (vkCreateSampler(RenderContext::GetInstance().Device, &samplerInfo, nullptr, &sampler) != VK_SUCCESS)
	{
		Fatal("Failed to create Vulkan sampler");
		return {}; // TODO: return optional<VkImageView>
	}
	return sampler;
}

VulkanBuffer Render::CreateBuffer(VkDeviceSize size, VkBufferUsageFlags bufferUsage, VmaAllocationCreateFlags flags, VmaMemoryUsage memoryUsage)
{
	return { RenderContext::GetInstance().Allocator, size, bufferUsage, flags, memoryUsage};
}

VkDescriptorSetLayout Render::CreateDescriptorSetLayout(const std::vector<VkDescriptorSetLayoutBinding>& bindings)
{
	VkDescriptorSetLayoutCreateInfo layoutInfo{ .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
	layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
	layoutInfo.pBindings = bindings.data();

	VkDescriptorSetLayout descriptorSetLayout;
	if (vkCreateDescriptorSetLayout(RenderContext::GetInstance().Device, &layoutInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS)
	{
		Fatal("Failed to create Vulkan descriptor set layout.");
		return {}; // TODO: return optional<VkDescriptorSetLayout>
	}
	return descriptorSetLayout;
}

VkDescriptorSet Render::AllocateDescriptorSet(VkDescriptorSetLayout descriptorSetLayout)
{
	VkDescriptorSetAllocateInfo allocateInfo{ .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
	allocateInfo.descriptorPool = RenderContext::GetInstance().DescriptorPool;
	allocateInfo.descriptorSetCount = 1;
	allocateInfo.pSetLayouts = &descriptorSetLayout;

	VkDescriptorSet descriptorSet;
	if (vkAllocateDescriptorSets(RenderContext::GetInstance().Device, &allocateInfo, &descriptorSet) != VK_SUCCESS)
	{
		Fatal("Failed to allocate Vulkan descriptor set.");
		return {}; // TODO: return optional<VkDescriptorSet>
	}
	return descriptorSet;
}

VkPipelineLayout Render::CreatePipelineLayout(const std::vector<VkDescriptorSetLayout>& descriptorSetLayouts, const std::vector<VkPushConstantRange>& pushConstantRanges)
{
	VkPipelineLayoutCreateInfo createInfo = { .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };
	createInfo.setLayoutCount = descriptorSetLayouts.size();
	createInfo.pSetLayouts = descriptorSetLayouts.data();
	createInfo.pushConstantRangeCount = pushConstantRanges.size();
	createInfo.pPushConstantRanges = pushConstantRanges.data();

	VkPipelineLayout pipelineLayout{ nullptr };
	if (vkCreatePipelineLayout(RenderContext::GetInstance().Device, &createInfo, nullptr, &pipelineLayout) != VK_SUCCESS)
	{
		Fatal("Failed to create Vulkan pipeline layout.");
		return {}; // TODO: return optional
	}
	return pipelineLayout;
}

VkShaderModule Render::CreateShaderModule(const std::vector<uint32_t>& spirv)
{
	VkShaderModuleCreateInfo createInfo = { .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO };
	createInfo.codeSize = spirv.size() * sizeof(uint32_t);
	createInfo.pCode = spirv.data();

	VkShaderModule shaderModule;
	if (vkCreateShaderModule(RenderContext::GetInstance().Device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS)
	{
		Fatal("Failed to create Vulkan shader module.");
		return {}; // TODO: optional
	}
}

VkPipeline Render::CreatePipeline(VkPipelineLayout pipelineLayout, const VkPipelineVertexInputStateCreateInfo* vertexInput, const std::vector<VkPipelineShaderStageCreateInfo>& shaderStages, const VulkanPipelineOptions& options, const std::vector<VkPipelineColorBlendAttachmentState>& attachmentColorBlends, VkRenderPass renderPass, uint32_t subpass)
{
	VkPipelineInputAssemblyStateCreateInfo inputAssemblyState = { .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO };
	inputAssemblyState.topology = options.Topology;

	VkPipelineViewportStateCreateInfo viewportState = { .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO };
	viewportState.viewportCount = 1;
	viewportState.scissorCount = 1;

	VkPipelineRasterizationStateCreateInfo rasterizationState = { .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO };
	rasterizationState.depthClampEnable = VK_FALSE;
	rasterizationState.rasterizerDiscardEnable = VK_FALSE;
	rasterizationState.polygonMode = options.PolygonMode;
	rasterizationState.cullMode = options.CullMode;
	rasterizationState.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	rasterizationState.depthBiasEnable = VK_FALSE;
	rasterizationState.lineWidth = 1.0f;

	VkPipelineMultisampleStateCreateInfo multisampleState = { .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO };
	multisampleState.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

	VkPipelineDepthStencilStateCreateInfo depthStencilState{ .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO };
	depthStencilState.depthTestEnable = options.DepthTestEnable;
	depthStencilState.depthWriteEnable = options.DepthWriteEnable;
	depthStencilState.depthCompareOp = options.DepthCompareOp;

	VkPipelineColorBlendStateCreateInfo colorBlendState = { .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO };
	colorBlendState.logicOpEnable = VK_FALSE;
	colorBlendState.logicOp = VK_LOGIC_OP_COPY;
	colorBlendState.attachmentCount = attachmentColorBlends.size();
	colorBlendState.pAttachments = attachmentColorBlends.data();

	std::vector<VkDynamicState> dynamicStates = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };

	VkPipelineDynamicStateCreateInfo dynamicState = { .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO };
	dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
	dynamicState.pDynamicStates = dynamicStates.data();

	VkGraphicsPipelineCreateInfo createInfo = { .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO };
	createInfo.stageCount = shaderStages.size();
	createInfo.pStages = shaderStages.data();
	createInfo.pVertexInputState = vertexInput;
	createInfo.pInputAssemblyState = &inputAssemblyState;
	createInfo.pViewportState = &viewportState;
	createInfo.pRasterizationState = &rasterizationState;
	createInfo.pMultisampleState = &multisampleState;
	createInfo.pColorBlendState = &colorBlendState;
	createInfo.pDepthStencilState = &depthStencilState;
	createInfo.pDynamicState = &dynamicState;
	createInfo.layout = pipelineLayout;
	createInfo.renderPass = renderPass;
	createInfo.subpass = subpass;

	VkPipeline pipeline;
	if (vkCreateGraphicsPipelines(RenderContext::GetInstance().Device, VK_NULL_HANDLE, 1, &createInfo, nullptr, &pipeline) != VK_SUCCESS)
	{
		Fatal("Failed to create Vulkan graphics pipeline.");
		return {}; // TODO: optional
	}
	return pipeline;
}

VulkanTexture* Render::LoadTexture(const std::string& filename, VkFilter filter, VkSamplerAddressMode addressMode)
{
	auto pair = TexturesCache.find(filename);
	if (pair == TexturesCache.end())
	{
		Print("Caching texture " + filename);
		const ImageFile image(filename);
		pair = TexturesCache.emplace(filename, VulkanTexture(image.GetWidth(), image.GetHeight(), image.GetData(), filter, addressMode)).first;
	}
	return &pair->second;
}

std::vector<VertexBase> loadObj(const std::string& filename)
{
	tinyobj::ObjReader reader;

	if (!reader.ParseFromString(FileSystem::Read(filename), ""))
	{
		Fatal("Failed to load OBJ file " + filename + ": " + reader.Error());
		return {}; // TODO: return optional
	}

	if (reader.Warning().empty())
	{
		Warning("Warning when loading OBJ file " + filename + ": " + reader.Warning());
	}

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
			if (f != 3)
			{
				Fatal("All polygons must be triangle.");
				return {}; // TODO: return optional
			}

			for (size_t v = 0; v < f; v++)
			{
				const tinyobj::index_t& index = mesh.indices[i + v];

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

VulkanMesh* Render::LoadObjMesh(const std::string& filename)
{
	auto pair = MeshesCache.find(filename);
	if (pair == MeshesCache.end())
	{
		Print("Caching OBJ mesh " + filename);
		const std::vector<VertexBase> vertices = loadObj(filename);
		pair = MeshesCache.emplace(filename, VulkanMesh(vertices.size(), sizeof(VertexBase), vertices.data())).first;
	}
	return &pair->second;
}

void Render::BeginImmediateSubmit()
{
	vkResetCommandBuffer(RenderContext::GetInstance().ImmediateCommandBuffer, 0);
	BeginCommandBuffer(RenderContext::GetInstance().ImmediateCommandBuffer, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
}

void Render::EndImmediateSubmit()
{
	EndCommandBuffer(RenderContext::GetInstance().ImmediateCommandBuffer);
	
	VkSubmitInfo submitInfo = { .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO };
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &RenderContext::GetInstance().ImmediateCommandBuffer;
	if (vkQueueSubmit(RenderContext::GetInstance().GraphicsQueue, 1, &submitInfo, RenderContext::GetInstance().ImmediateFence) != VK_SUCCESS)
	{
		Fatal("failed to submit draw command buffer");
		return;
	}
	WaitAndResetFence(RenderContext::GetInstance().ImmediateFence);
}

bool Render::BeginCommandBuffer(VkCommandBuffer commandBuffer, VkCommandBufferUsageFlags flags)
{
	VkCommandBufferBeginInfo beginInfo = { .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
	beginInfo.flags = flags;
	if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS)
	{
		Fatal("failed to begin recording command buffer!");
		return false;
	}

	return true;
}

bool Render::EndCommandBuffer(VkCommandBuffer commandBuffer)
{
	if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS)
	{
		Fatal("failed to record command buffer");
		return false;
	}

	return true;
}

void Render::WaitAndResetFence(VkFence fence, uint64_t timeout)
{
	vkWaitForFences(RenderContext::GetInstance().Device, 1, &fence, VK_TRUE, timeout);
	vkResetFences(RenderContext::GetInstance().Device, 1, &fence);
}

void writeDescriptorSet(const VkWriteDescriptorSet& writeDescriptorSet)
{
	vkUpdateDescriptorSets(RenderContext::GetInstance().Device, 1, &writeDescriptorSet, 0, 0);
}

void Render::WriteCombinedImageSamplerToDescriptorSet(VkSampler sampler, VkImageView imageView, VkDescriptorSet descriptorSet, uint32_t binding)
{
	VkDescriptorImageInfo imageInfo{};
	imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	imageInfo.imageView = imageView;
	imageInfo.sampler = sampler;

	VkWriteDescriptorSet writeDescriptorSet{ .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
	writeDescriptorSet.dstSet = descriptorSet;
	writeDescriptorSet.dstBinding = binding;
	writeDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	writeDescriptorSet.descriptorCount = 1;
	writeDescriptorSet.pImageInfo = &imageInfo;
	
	vkUpdateDescriptorSets(RenderContext::GetInstance().Device, 1, &writeDescriptorSet, 0, nullptr);
}

void Render::WriteDynamicUniformBufferToDescriptorSet(VkBuffer buffer, VkDeviceSize size, VkDescriptorSet descriptorSet, uint32_t binding)
{
	VkDescriptorBufferInfo bufferInfo{};
	bufferInfo.buffer = buffer;
	bufferInfo.offset = 0;
	bufferInfo.range = size;

	VkWriteDescriptorSet writeDescSet{ .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
	writeDescSet.dstSet = descriptorSet;
	writeDescSet.dstBinding = binding;
	writeDescSet.dstArrayElement = 0;
	writeDescSet.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
	writeDescSet.descriptorCount = 1;
	writeDescSet.pBufferInfo = &bufferInfo;
	writeDescriptorSet(writeDescSet);
}

void Render::DestroySampler(VkSampler sampler)
{
	vkDestroySampler(RenderContext::GetInstance().Device, sampler, nullptr);
}

void Render::DestroyImageView(VkImageView imageView)
{
	vkDestroyImageView(RenderContext::GetInstance().Device, imageView, nullptr);
}

void Render::FreeDescriptorSet(VkDescriptorSet descriptorSet)
{
	vkFreeDescriptorSets(RenderContext::GetInstance().Device, RenderContext::GetInstance().DescriptorPool, 1, &descriptorSet);
}

void Render::DestroyDescriptorSetLayout(VkDescriptorSetLayout descriptorSetLayout)
{
	vkDestroyDescriptorSetLayout(RenderContext::GetInstance().Device, descriptorSetLayout, nullptr);
}

#pragma endregion