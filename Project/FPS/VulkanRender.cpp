#include "Base.h"
#include "Core.h"
#include "VulkanRender.h"
VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE

#pragma region ShaderCompiler

struct ShaderIncluder : glslang::TShader::Includer {
	explicit ShaderIncluder(std::string includesDir)
		: m_includesDir(std::move(includesDir)) {}

	IncludeResult* includeLocal(const char*, const char*, size_t) override {
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
			//DebugInfo("Loading shader header file <{}> into cache.", headerName);
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
	//DebugInfo("glslang version: {}", glslang::GetGlslVersionString());
	if(!glslang::InitializeProcess())
		Fatal("Failed to initialize glslang.");
	m_includer = std::make_unique<ShaderIncluder>(includesDir);
}

ShaderCompiler::~ShaderCompiler() {
	m_includer.reset();
	glslang::FinalizeProcess();
}

static inline EShLanguage GetShaderStageLanguage(vk::ShaderStageFlagBits stage) {
	switch (stage) {
	case vk::ShaderStageFlagBits::eVertex:
		return EShLangVertex;
	case vk::ShaderStageFlagBits::eGeometry:
		return EShLangGeometry;
	case vk::ShaderStageFlagBits::eFragment:
		return EShLangFragment;
	case vk::ShaderStageFlagBits::eCompute:
		return EShLangCompute;
	default:
		return EShLangCount;
	}
}

static inline const char* GetShaderStageName(vk::ShaderStageFlagBits stage) {
	switch (stage) {
	case vk::ShaderStageFlagBits::eVertex:
		return "vertex";
	case vk::ShaderStageFlagBits::eGeometry:
		return "geometry";
	case vk::ShaderStageFlagBits::eFragment:
		return "fragment";
	case vk::ShaderStageFlagBits::eCompute:
		return "compute";
	default:
		return "unsupported stage";
	}
}

std::vector<uint32_t> ShaderCompiler::Compile(vk::ShaderStageFlagBits stage, const std::string& source) {
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
		//DebugError("Failed to parse {} shader: {}", stageName, shader.getInfoLog());
		return {};
	}
	else {
		const char* infoLog = shader.getInfoLog();
		if (std::strlen(infoLog)) {
			//DebugWarning("Shader compilation warning: {}", infoLog);
		}
	}

	glslang::TProgram program;
	program.addShader(&shader);

	if (!program.link(EShMsgDefault)) {
		//DebugError("Failed to link {} shader program: {}", stageName, program.getInfoLog());
		return {};
	}

	std::vector<uint32_t> spirv;
	glslang::GlslangToSpv(*program.getIntermediate(glslStage), spirv);
	return spirv;
}

std::vector<uint32_t> ShaderCompiler::CompileFromFile(vk::ShaderStageFlagBits stage, const std::string& filename) {
	const char* stageName = GetShaderStageName(stage);
	//DebugInfo("Compiling {} shader: {}", stageName, filename);
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

VulkanUniformBufferSet::VulkanUniformBufferSet(VulkanBase& device, const std::initializer_list<VulkanUniformBufferInfo>& uniformBufferInfos)
	: m_device(&device) {
	const size_t numBuffering = m_device->GetNumBuffering();
	const size_t numUniformBuffers = uniformBufferInfos.size();

	std::vector<vk::DescriptorSetLayoutBinding> bindings;
	bindings.reserve(numUniformBuffers);
	for (const auto& uniformBufferInfo : uniformBufferInfos) {
		bindings.emplace_back(uniformBufferInfo.Binding, vk::DescriptorType::eUniformBufferDynamic, 1, uniformBufferInfo.Stage);
	}
	m_descriptorSetLayout = m_device->CreateDescriptorSetLayout(bindings);
	m_descriptorSet = m_device->AllocateDescriptorSet(m_descriptorSetLayout);

	m_uniformBufferSizes.reserve(numUniformBuffers);
	m_uniformBuffers.reserve(numUniformBuffers);
	for (const auto& uniformBufferInfo : uniformBufferInfos) {
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
	for (int i = 0; i < numBuffering; i++) {
		std::vector<uint32_t> dynamicOffsets;
		dynamicOffsets.reserve(numUniformBuffers);
		for (int j = 0; j < numUniformBuffers; j++) {
			dynamicOffsets.push_back(i * m_uniformBufferSizes[j]);
		}
		m_dynamicOffsets.push_back(dynamicOffsets);
	}
}

void VulkanUniformBufferSet::Release() {
	if (m_device) {
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

void VulkanUniformBufferSet::Swap(VulkanUniformBufferSet& other) noexcept {
	std::swap(m_device, other.m_device);
	std::swap(m_descriptorSetLayout, other.m_descriptorSetLayout);
	std::swap(m_descriptorSet, other.m_descriptorSet);
	std::swap(m_uniformBufferSizes, other.m_uniformBufferSizes);
	std::swap(m_uniformBuffers, other.m_uniformBuffers);
	std::swap(m_dynamicOffsets, other.m_dynamicOffsets);
}

void VulkanUniformBufferSet::UpdateAllBuffers(uint32_t bufferingIndex, const std::initializer_list<const void*>& allBuffersData) {
	int i = 0;
	for (const void* bufferData : allBuffersData) {
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
	: m_allocator(allocator) {
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

void VulkanImage::Release() {
	if (m_allocator) {
		vmaDestroyImage(m_allocator, m_image, m_allocation);
	}

	m_allocator = VK_NULL_HANDLE;
	m_image = VK_NULL_HANDLE;
	m_allocation = VK_NULL_HANDLE;
}

void VulkanImage::Swap(VulkanImage& other) noexcept {
	std::swap(m_allocator, other.m_allocator);
	std::swap(m_image, other.m_image);
	std::swap(m_allocation, other.m_allocation);
}

#pragma endregion

#pragma region VulkanPipeline

static vk::PrimitiveTopology TopologyFromString(const std::string& topology) {
	if (topology == "point_list") {
		return vk::PrimitiveTopology::ePointList;
	}
	else if (topology == "line_list") {
		return vk::PrimitiveTopology::eLineList;
	}
	else if (topology == "line_strip") {
		return vk::PrimitiveTopology::eLineStrip;
	}
	else if (topology == "triangle_list") {
		return vk::PrimitiveTopology::eTriangleList;
	}
	else if (topology == "triangle_strip") {
		return vk::PrimitiveTopology::eTriangleStrip;
	}
	else if (topology == "triangle_fan") {
		return vk::PrimitiveTopology::eTriangleFan;
	}
	//DebugError("Invalid topology: {}", topology);
	exit(EXIT_FAILURE);
}

static vk::PolygonMode PolygonModeFromString(const std::string& polygonMode) {
	if (polygonMode == "fill") {
		return vk::PolygonMode::eFill;
	}
	else if (polygonMode == "line") {
		return vk::PolygonMode::eLine;
	}
	else if (polygonMode == "point") {
		return vk::PolygonMode::ePoint;
	}
	//DebugError("Invalid polygon mode: {}", polygonMode);
	exit(EXIT_FAILURE);
}

static vk::CullModeFlags CullModeFromString(const std::string& cullMode) {
	if (cullMode == "none") {
		return vk::CullModeFlagBits::eNone;
	}
	else if (cullMode == "back") {
		return vk::CullModeFlagBits::eBack;
	}
	else if (cullMode == "front") {
		return vk::CullModeFlagBits::eFront;
	}
	else if (cullMode == "front_and_back") {
		return vk::CullModeFlagBits::eFrontAndBack;
	}
	//DebugError("Invalid cull mode: {}", cullMode);
	exit(EXIT_FAILURE);
}

static vk::CompareOp CompareOpFromString(const std::string& compareOp) {
	if (compareOp == "never") {
		return vk::CompareOp::eNever;
	}
	else if (compareOp == "less") {
		return vk::CompareOp::eLess;
	}
	else if (compareOp == "equal") {
		return vk::CompareOp::eEqual;
	}
	else if (compareOp == "less_or_equal") {
		return vk::CompareOp::eLessOrEqual;
	}
	else if (compareOp == "greater") {
		return vk::CompareOp::eGreater;
	}
	else if (compareOp == "not_equal") {
		return vk::CompareOp::eNotEqual;
	}
	else if (compareOp == "greater_or_equal") {
		return vk::CompareOp::eGreaterOrEqual;
	}
	else if (compareOp == "always") {
		return vk::CompareOp::eAlways;
	}
	//DebugError("Invalid compare op: {}", compareOp);
	exit(EXIT_FAILURE);
}

VulkanPipelineConfig::VulkanPipelineConfig(const std::string& jsonFilename) {
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
	VulkanDevice& device,
	ShaderCompiler& compiler,
	const std::initializer_list<vk::DescriptorSetLayout>& descriptorSetLayouts,
	const std::initializer_list<vk::PushConstantRange>& pushConstantRanges,
	const vk::PipelineVertexInputStateCreateInfo* vertexInput,
	const std::string& pipelineConfigFile,
	const std::initializer_list<vk::PipelineColorBlendAttachmentState>& attachmentColorBlends,
	vk::RenderPass                                                      renderPass,
	uint32_t                                                            subpass
)
	: m_device(&device) {
	m_pipelineLayout = m_device->CreatePipelineLayout(descriptorSetLayouts, pushConstantRanges);

	const VulkanPipelineConfig pipelineConfig(pipelineConfigFile);

	const std::vector<uint32_t> vertexSpirv = compiler.CompileFromFile(vk::ShaderStageFlagBits::eVertex, pipelineConfig.VertexShader);
	m_vertexShaderModule = m_device->CreateShaderModule(vertexSpirv);
	if (!pipelineConfig.GeometryShader.empty()) {
		const std::vector<uint32_t> geometrySpirv = compiler.CompileFromFile(vk::ShaderStageFlagBits::eGeometry, pipelineConfig.GeometryShader);
		m_geometryShaderModule = m_device->CreateShaderModule(geometrySpirv);
	}
	const std::vector<uint32_t> fragmentSpirv = compiler.CompileFromFile(vk::ShaderStageFlagBits::eFragment, pipelineConfig.FragmentShader);
	m_fragmentShaderModule = m_device->CreateShaderModule(fragmentSpirv);

	std::vector<vk::PipelineShaderStageCreateInfo> shaderStages;

	shaderStages.emplace_back(vk::PipelineShaderStageCreateFlags{}, vk::ShaderStageFlagBits::eVertex, m_vertexShaderModule, "main");
	if (!pipelineConfig.GeometryShader.empty()) {
		shaderStages.emplace_back(vk::PipelineShaderStageCreateFlags{}, vk::ShaderStageFlagBits::eGeometry, m_geometryShaderModule, "main");
	}
	shaderStages.emplace_back(vk::PipelineShaderStageCreateFlags{}, vk::ShaderStageFlagBits::eFragment, m_fragmentShaderModule, "main");

	m_pipeline =
		m_device->CreatePipeline(m_pipelineLayout, vertexInput, shaderStages, pipelineConfig.Options, attachmentColorBlends, renderPass, subpass);
}

void VulkanPipeline::Release() {
	if (m_device) {
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

void VulkanPipeline::Swap(VulkanPipeline& other) noexcept {
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
	VulkanBase& device,
	uint32_t               width,
	uint32_t               height,
	const unsigned char* data,
	vk::Filter             filter,
	vk::SamplerAddressMode addressMode
)
	: m_device(&device) {
	constexpr size_t PIXEL_SIZE = sizeof(unsigned char) * 4;
	CreateImage(width, height, width * height * PIXEL_SIZE, data, vk::Format::eR8G8B8A8Unorm);
	CreateImageView(vk::Format::eR8G8B8A8Unorm);
	CreateSampler(filter, addressMode);
}

void VulkanTexture::CreateImage(uint32_t width, uint32_t height, vk::DeviceSize size, const void* data, vk::Format format) {
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

void VulkanTexture::CreateImageView(vk::Format format) {
	m_imageView = m_device->CreateImageView(m_image.Get(), format, vk::ImageAspectFlagBits::eColor);
}

void VulkanTexture::CreateSampler(vk::Filter filter, vk::SamplerAddressMode addressMode) {
	m_sampler = m_device->CreateSampler(filter, addressMode);
}

void VulkanTexture::Release() {
	if (m_device) {
		m_device->DestroySampler(m_sampler);
		m_device->DestroyImageView(m_imageView);
	}

	m_device = nullptr;
	m_image = {};
	m_imageView = VK_NULL_HANDLE;
	m_sampler = VK_NULL_HANDLE;
}

void VulkanTexture::Swap(VulkanTexture& other) noexcept {
	std::swap(m_device, other.m_device);
	std::swap(m_image, other.m_image);
	std::swap(m_imageView, other.m_imageView);
	std::swap(m_sampler, other.m_sampler);
}

void VulkanTexture::BindToDescriptorSet(vk::DescriptorSet descriptorSet, uint32_t binding) {
	m_device->WriteCombinedImageSamplerToDescriptorSet(m_sampler, m_imageView, descriptorSet, binding);
}

#pragma endregion

#pragma region VulkanMesh

VulkanMesh::VulkanMesh(VulkanBase& device, size_t vertexCount, size_t vertexSize, const void* data) {
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

void VulkanMesh::Release() {
	m_vertexBuffer = {};
	m_vertexCount = 0;
}

void VulkanMesh::Swap(VulkanMesh& other) noexcept {
	std::swap(m_vertexBuffer, other.m_vertexBuffer);
	std::swap(m_vertexCount, other.m_vertexCount);
}

void VulkanMesh::BindAndDraw(vk::CommandBuffer commandBuffer) const {
	const vk::DeviceSize offset = 0;
	commandBuffer.bindVertexBuffers(0, 1, &m_vertexBuffer.Get(), &offset);
	commandBuffer.draw(m_vertexCount, 1, 0, 0);
}

#pragma endregion

#pragma region VertexFormats

const vk::PipelineVertexInputStateCreateInfo* VertexPositionOnly::GetPipelineVertexInputStateCreateInfo() {
	static const std::vector<vk::VertexInputBindingDescription> bindings{
		{0, sizeof(VertexPositionOnly), vk::VertexInputRate::eVertex}
	};

	static const std::vector<vk::VertexInputAttributeDescription> attributes{
		{0, 0, vk::Format::eR32G32B32Sfloat, static_cast<uint32_t>(offsetof(VertexPositionOnly, Position))},
	};

	static const vk::PipelineVertexInputStateCreateInfo vertexInput{ {}, bindings, attributes };

	return &vertexInput;
}

const vk::PipelineVertexInputStateCreateInfo* VertexCanvas::GetPipelineVertexInputStateCreateInfo() {
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

const vk::PipelineVertexInputStateCreateInfo* VertexBase::GetPipelineVertexInputStateCreateInfo() {
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

VulkanTexture* TextureCache::LoadTexture(const std::string& filename, vk::Filter filter, vk::SamplerAddressMode addressMode) {
	auto pair = m_textures.find(filename);
	if (pair == m_textures.end()) {
		//DebugInfo("Caching texture {}.", filename);
		const ImageFile image(filename);
		pair = m_textures.emplace(filename, VulkanTexture(m_device, image.GetWidth(), image.GetHeight(), image.GetData(), filter, addressMode)).first;
	}
	return &pair->second;
}

#pragma endregion

#pragma region MeshCache

static std::vector<VertexBase> LoadObj(const std::string& filename) {
	tinyobj::ObjReader reader;

	if(!reader.ParseFromString(FileSystem::Read(filename), "")) Fatal("Failed to load OBJ file " + filename + ": " + reader.Error());

	//DebugCheck(reader.Warning().empty(), "Warning when loading OBJ file {}: {}", filename, reader.Warning());

	const tinyobj::attrib_t& attrib = reader.GetAttrib();
	const std::vector<tinyobj::real_t>& positions = attrib.vertices;
	const std::vector<tinyobj::real_t>& normals = attrib.normals;
	const std::vector<tinyobj::real_t>& texCoords = attrib.texcoords;

	std::vector<VertexBase> vertices;
	for (const tinyobj::shape_t& shape : reader.GetShapes()) {
		const tinyobj::mesh_t& mesh = shape.mesh;
		//DebugInfo("Loading OBJ shape {}.", shape.name);

		size_t i = 0;
		for (const size_t f : mesh.num_face_vertices) {
			//DebugCheckCritical(f == 3, "All polygons must be triangle.");

			for (size_t v = 0; v < f; v++) {
				const tinyobj::index_t& index = mesh.indices[i + v];
				//DebugCheckCritical(index.normal_index >= 0, "Missing normal data on vertex {}/{}.", f, v);
				//DebugCheckCritical(index.texcoord_index >= 0, "Missing texture coordinates data on vertex {}/{}.", f, v);

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

VulkanMesh* MeshCache::LoadObjMesh(const std::string& filename) {
	auto pair = m_meshes.find(filename);
	if (pair == m_meshes.end()) {
		//DebugInfo("Caching OBJ mesh {}.", filename);
		const std::vector<VertexBase> vertices = LoadObj(filename);
		pair = m_meshes.emplace(filename, VulkanMesh(m_device, vertices.size(), sizeof(VertexBase), vertices.data())).first;
	}
	return &pair->second;
}

#pragma endregion

#pragma region VulkanDevice

VulkanDevice::VulkanDevice() {
	CreateInstance();
	CreateDebugMessenger();
	PickPhysicalDevice();
	CreateDevice();
	CreateCommandPool();
	CreateDescriptorPool();
	CreateAllocator();
}

static std::vector<const char*> GetEnabledInstanceLayers() {
	return { "VK_LAYER_KHRONOS_validation" };
}

static std::vector<const char*> GetEnabledInstanceExtensions() {
	uint32_t                  numGlfwExtensions = 0;
	const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&numGlfwExtensions);
	std::vector<const char*> extensions(glfwExtensions, glfwExtensions + numGlfwExtensions);
	extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
	return extensions;
}

void VulkanDevice::CreateInstance() {
	// See https://github.com/KhronosGroup/Vulkan-Hpp/pull/1755
   // Currently Vulkan-Hpp doesn't check for libvulkan.1.dylib
   // Which affects vkcube installation on Apple platforms.
	VkResult err = volkInitialize();
	if (err != VK_SUCCESS) {
		/*ERR_EXIT(
			"Unable to find the Vulkan runtime on the system.\n\n"
			"This likely indicates that no Vulkan capable drivers are installed.",
			"Installation Failure");*/
	}

	VULKAN_HPP_DEFAULT_DISPATCHER.init(vkGetInstanceProcAddr);



	const vk::ApplicationInfo applicationInfo("Game", VK_MAKE_VERSION(1, 0, 0), "Game", VK_MAKE_VERSION(1, 0, 0), VK_API_VERSION_1_3);

	const std::vector<const char*> enabledLayers = GetEnabledInstanceLayers();
	const std::vector<const char*> enabledExtensions = GetEnabledInstanceExtensions();

	const vk::InstanceCreateInfo
		instanceCreateInfo({}, &applicationInfo, enabledLayers.size(), enabledLayers.data(), enabledExtensions.size(), enabledExtensions.data());

	const auto [result, instance] = vk::createInstance(instanceCreateInfo);
	//DebugCheckCriticalVk(result, "Failed to create Vulkan instance.");
	m_instance = instance;

	VULKAN_HPP_DEFAULT_DISPATCHER.init(m_instance);
	volkLoadInstanceOnly(m_instance);

}

static VKAPI_ATTR VkBool32 VKAPI_CALL VulkanDebugCallback(
	[[maybe_unused]] VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
	[[maybe_unused]] VkDebugUtilsMessageTypeFlagsEXT        messageTypes,
	const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
	[[maybe_unused]] void* pUserData
) {
	if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) {
		//DebugError("Vulkan: {}", pCallbackData->pMessage);
	}
	else if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
		//DebugWarning("Vulkan: {}", pCallbackData->pMessage);
	}
	else if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT) {
		//DebugInfo("Vulkan: {}", pCallbackData->pMessage);
	}
	else if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT) {
		//DebugVerbose("Vulkan: {}", pCallbackData->pMessage);
	}
	else {
		/*DebugError("Vulkan debug message with unknown severity level 0x{:X}: {}", messageSeverity, pCallbackData->pMessage);*/
	}
	return VK_FALSE;
}

void VulkanDevice::CreateDebugMessenger() {
	const vk::DebugUtilsMessengerCreateInfoEXT createInfo(
		{},
		vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo | vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning |
		vk::DebugUtilsMessageSeverityFlagBitsEXT::eError,
		vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral | vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation |
		vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance,
		VulkanDebugCallback
	);
	const auto [result, debugMessenger] = m_instance.createDebugUtilsMessengerEXT(createInfo);
	//DebugCheckCriticalVk(result, "Failed to create Vulkan debug messenger.");
	m_debugMessenger = debugMessenger;
}

void VulkanDevice::PickPhysicalDevice() {
	const auto [result, physicalDevices] = m_instance.enumeratePhysicalDevices();
	//DebugCheckCriticalVk(result, "Failed to enumerate Vulkan physical devices.");
	//DebugCheck(!physicalDevices.empty(), "Can't find any Vulkan physical device.");

	// select the first discrete gpu or the first physical device
	vk::PhysicalDevice selected = physicalDevices.front();
	for (const vk::PhysicalDevice& physicalDevice : physicalDevices) {
		if (physicalDevice.getProperties().deviceType == vk::PhysicalDeviceType::eDiscreteGpu) {
			selected = physicalDevice;
			break;
		}
	}
	m_physicalDevice = selected;

	const std::vector<vk::QueueFamilyProperties> queueFamilies = m_physicalDevice.getQueueFamilyProperties();
	m_graphicsQueueFamily = 0;
	for (const vk::QueueFamilyProperties& queueFamily : queueFamilies) {
		if (queueFamily.queueFlags & vk::QueueFlagBits::eGraphics) {
			break;
		}
		m_graphicsQueueFamily++;
	}
	//DebugCheckCritical(m_graphicsQueueFamily != queueFamilies.size(), "Failed to find a Vulkan graphics queue family.");
}

static std::vector<const char*> GetEnabledDeviceExtensions() {
	return { VK_KHR_SWAPCHAIN_EXTENSION_NAME, VK_KHR_GET_MEMORY_REQUIREMENTS_2_EXTENSION_NAME };
}

void VulkanDevice::CreateDevice() {
	const float                     queuePriority = 1.0f;
	const vk::DeviceQueueCreateInfo queueCreateInfo({}, m_graphicsQueueFamily, 1, &queuePriority);

	const std::vector<const char*> enabledExtensions = GetEnabledDeviceExtensions();

	vk::PhysicalDeviceFeatures features;
	features.geometryShader = VK_TRUE;
	features.fillModeNonSolid = VK_TRUE;

	const vk::DeviceCreateInfo createInfo({}, 1, &queueCreateInfo, 0, nullptr, enabledExtensions.size(), enabledExtensions.data(), &features);
	const auto [result, device] = m_physicalDevice.createDevice(createInfo);
	//DebugCheckCriticalVk(result, "Failed to create Vulkan device.");
	m_device = device;

	m_graphicsQueue = m_device.getQueue(m_graphicsQueueFamily, 0);
	//DebugCheckCritical(m_device, "Failed to get Vulkan graphics queue.");

	VULKAN_HPP_DEFAULT_DISPATCHER.init(m_device);
	volkLoadDevice(m_device);
}

void VulkanDevice::SubmitToGraphicsQueue(const vk::SubmitInfo& submitInfo, vk::Fence fence)
{
	if(m_graphicsQueue.submit(submitInfo, fence) != vk::Result::eSuccess)
		Fatal("Failed to submit Vulkan command buffer.");
}

void VulkanDevice::CreateCommandPool() {
	const vk::CommandPoolCreateInfo createInfo(vk::CommandPoolCreateFlagBits::eResetCommandBuffer, m_graphicsQueueFamily);
	const auto [result, commandPool] = m_device.createCommandPool(createInfo);
	//DebugCheckCriticalVk(result, "Failed to create Vulkan command pool.");
	m_commandPool = commandPool;
}

void VulkanDevice::CreateDescriptorPool() {
	const std::vector<vk::DescriptorPoolSize> poolSizes{
	{vk::DescriptorType::eSampler,              1024},
	{vk::DescriptorType::eCombinedImageSampler, 1024},
	{vk::DescriptorType::eSampledImage,         1024},
	{vk::DescriptorType::eStorageImage,         1024},
	{vk::DescriptorType::eUniformTexelBuffer,   1024},
	{vk::DescriptorType::eStorageTexelBuffer,   1024},
	{vk::DescriptorType::eUniformBuffer,        1024},
	{vk::DescriptorType::eStorageBuffer,        1024},
	{vk::DescriptorType::eUniformBufferDynamic, 1024},
	{vk::DescriptorType::eStorageBufferDynamic, 1024},
	{vk::DescriptorType::eInputAttachment,      1024}
	};
	const vk::DescriptorPoolCreateInfo createInfo(vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet, 1024, poolSizes);
	const auto [result, descriptorPool] = m_device.createDescriptorPool(createInfo);
	//DebugCheckCriticalVk(result, "Failed to create Vulkan descriptor pool.");
	m_descriptorPool = descriptorPool;
}

void VulkanDevice::WriteDescriptorSet(const vk::WriteDescriptorSet& writeDescriptorSet) {
	m_device.updateDescriptorSets(writeDescriptorSet, {});
}

void VulkanDevice::CreateAllocator() {

	VmaVulkanFunctions vmaVulkanFunc{};
	vmaVulkanFunc.vkGetInstanceProcAddr = vkGetInstanceProcAddr;
	vmaVulkanFunc.vkGetDeviceProcAddr = vkGetDeviceProcAddr;
	vmaVulkanFunc.vkAllocateMemory = vkAllocateMemory;
	vmaVulkanFunc.vkBindBufferMemory = vkBindBufferMemory;
	vmaVulkanFunc.vkBindImageMemory = vkBindImageMemory;
	vmaVulkanFunc.vkCreateBuffer = vkCreateBuffer;
	vmaVulkanFunc.vkCreateImage = vkCreateImage;
	vmaVulkanFunc.vkDestroyBuffer = vkDestroyBuffer;
	vmaVulkanFunc.vkDestroyImage = vkDestroyImage;
	vmaVulkanFunc.vkFlushMappedMemoryRanges = vkFlushMappedMemoryRanges;
	vmaVulkanFunc.vkFreeMemory = vkFreeMemory;
	vmaVulkanFunc.vkGetBufferMemoryRequirements = vkGetBufferMemoryRequirements;
	vmaVulkanFunc.vkGetImageMemoryRequirements = vkGetImageMemoryRequirements;
	vmaVulkanFunc.vkGetPhysicalDeviceMemoryProperties = vkGetPhysicalDeviceMemoryProperties;
	vmaVulkanFunc.vkGetPhysicalDeviceProperties = vkGetPhysicalDeviceProperties;
	vmaVulkanFunc.vkInvalidateMappedMemoryRanges = vkInvalidateMappedMemoryRanges;
	vmaVulkanFunc.vkMapMemory = vkMapMemory;
	vmaVulkanFunc.vkUnmapMemory = vkUnmapMemory;
	vmaVulkanFunc.vkCmdCopyBuffer = vkCmdCopyBuffer;
	vmaVulkanFunc.vkGetImageMemoryRequirements2KHR = vkGetImageMemoryRequirements2KHR;

	VmaAllocatorCreateInfo createInfo{};
	createInfo.physicalDevice = m_physicalDevice;
	createInfo.device = m_device;
	createInfo.instance = m_instance;
	createInfo.vulkanApiVersion = VK_API_VERSION_1_0;
	createInfo.pVulkanFunctions = &vmaVulkanFunc;
	if(vmaCreateAllocator(&createInfo, &m_allocator) != VK_SUCCESS)
		Fatal("Failed to create Vulkan Memory Allocator.");
}

VulkanDevice::~VulkanDevice() {
	WaitIdle();

	vmaDestroyAllocator(m_allocator);
	m_device.destroy(m_descriptorPool);
	m_device.destroy(m_commandPool);
	m_device.destroy();
	m_instance.destroy(m_debugMessenger);
	m_instance.destroy();
}

void VulkanDevice::WaitIdle() {
	if(m_device.waitIdle() != vk::Result::eSuccess)
		Fatal("Failed when waiting for Vulkan device to be idle.");
}

vk::Fence VulkanDevice::CreateFence(vk::FenceCreateFlags flags) {
	const vk::FenceCreateInfo createInfo(flags);
	const auto [result, fence] = m_device.createFence(createInfo);
	//DebugCheckCriticalVk(result, "Failed to create Vulkan fence.");
	return fence;
}

void VulkanDevice::WaitAndResetFence(vk::Fence fence, uint64_t timeout) {
	if(m_device.waitForFences(fence, true, timeout) != vk::Result::eSuccess)
		Fatal("Failed to wait for Vulkan fence.");
	m_device.resetFences(fence);
}

vk::Semaphore VulkanDevice::CreateSemaphore() {
	const vk::SemaphoreCreateInfo createInfo;
	const auto [result, semaphore] = m_device.createSemaphore(createInfo);
	//DebugCheckCriticalVk(result, "Failed to create Vulkan semaphore.");
	return semaphore;
}

vk::CommandBuffer VulkanDevice::AllocateCommandBuffer() {
	const vk::CommandBufferAllocateInfo allocateInfo(m_commandPool, vk::CommandBufferLevel::ePrimary, 1);
	vk::CommandBuffer                   commandBuffer;
	if(m_device.allocateCommandBuffers(&allocateInfo, &commandBuffer) != vk::Result::eSuccess)
		Fatal("Failed to allocate Vulkan command buffer.");
	return commandBuffer;
}

vk::SurfaceKHR VulkanDevice::CreateSurface(GLFWwindow* window) {
	VkSurfaceKHR surface;
	if(glfwCreateWindowSurface(m_instance, window, nullptr, &surface) != VK_SUCCESS)
		Fatal("Failed to create Vulkan surface.");
	return surface;
}

vk::SwapchainKHR VulkanDevice::CreateSwapchain(
	vk::SurfaceKHR                  surface,
	uint32_t                        imageCount,
	vk::Format                      imageFormat,
	vk::ColorSpaceKHR               imageColorSpace,
	const vk::Extent2D& imageExtent,
	vk::SurfaceTransformFlagBitsKHR preTransform,
	vk::PresentModeKHR              presentMode,
	vk::SwapchainKHR                oldSwapchain
) {
	const vk::SwapchainCreateInfoKHR createInfo(
		{},
		surface,
		imageCount,
		imageFormat,
		imageColorSpace,
		imageExtent,
		1,
		vk::ImageUsageFlagBits::eColorAttachment,
		vk::SharingMode::eExclusive,
		{},
		preTransform,
		vk::CompositeAlphaFlagBitsKHR::eOpaque,
		presentMode,
		VK_TRUE,
		oldSwapchain,
		nullptr
	);
	const auto [result, swapchain] = m_device.createSwapchainKHR(createInfo);
	//DebugCheckCriticalVk(result, "Failed to create Vulkan swapchain.");
	return swapchain;
}

vk::ImageView VulkanDevice::CreateImageView(vk::Image image, vk::Format format, vk::ImageAspectFlags aspectMask, uint32_t layers) {
	const vk::ImageViewCreateInfo
		createInfo({}, image, layers == 1 ? vk::ImageViewType::e2D : vk::ImageViewType::e2DArray, format, {}, { aspectMask, 0, 1, 0, layers });
	const auto [result, imageView] = m_device.createImageView(createInfo);
	//DebugCheckCriticalVk(result, "Failed to create Vulkan image view.");
	return imageView;
}

void VulkanDevice::DestroyImageView(vk::ImageView imageView) {
	m_device.destroyImageView(imageView);
}

vk::Sampler VulkanDevice::CreateSampler(vk::Filter filter, vk::SamplerAddressMode addressMode, vk::Bool32 compareEnable, vk::CompareOp compareOp) {
	const vk::SamplerCreateInfo createInfo(
		{},
		filter,
		filter,
		{},
		addressMode,
		addressMode,
		addressMode,
		0.0f,
		VK_FALSE,
		0.0f,
		compareEnable,
		compareOp,
		0.0f,
		0.0f,
		vk::BorderColor::eFloatTransparentBlack,
		{}
	);
	const auto [result, sampler] = m_device.createSampler(createInfo);
	//DebugCheckCriticalVk(result, "Failed to create Vulkan sampler");
	return sampler;
}

void VulkanDevice::DestroySampler(vk::Sampler sampler) {
	m_device.destroySampler(sampler);
}

vk::RenderPass VulkanDevice::CreateRenderPass(
	const std::initializer_list<vk::Format>& colorAttachmentFormats,
	vk::Format                               depthStencilAttachmentFormat,
	const VulkanRenderPassOptions& options
) {
	std::vector<vk::AttachmentDescription> attachments;
	std::vector<vk::AttachmentReference>   colorAttachmentRefs;
	vk::AttachmentReference                depthStencilAttachmentRef;

	uint32_t attachmentIndex = 0;
	for (const vk::Format& colorAttachmentFormat : colorAttachmentFormats) {
		attachments.emplace_back(
			vk::AttachmentDescriptionFlags{},
			colorAttachmentFormat,
			vk::SampleCountFlagBits::e1,
			vk::AttachmentLoadOp::eClear,
			vk::AttachmentStoreOp::eStore,
			vk::AttachmentLoadOp::eDontCare,
			vk::AttachmentStoreOp::eDontCare,
			vk::ImageLayout::eUndefined,
			options.ForPresentation ? vk::ImageLayout::ePresentSrcKHR : vk::ImageLayout::eShaderReadOnlyOptimal
		);

		colorAttachmentRefs.emplace_back(attachmentIndex++, vk::ImageLayout::eColorAttachmentOptimal);
	}

	if (depthStencilAttachmentFormat != vk::Format::eUndefined) {
		attachments.emplace_back(
			vk::AttachmentDescriptionFlags{},
			depthStencilAttachmentFormat,
			vk::SampleCountFlagBits::e1,
			options.PreserveDepth ? vk::AttachmentLoadOp::eLoad : vk::AttachmentLoadOp::eClear,
			vk::AttachmentStoreOp::eStore,
			vk::AttachmentLoadOp::eDontCare,
			vk::AttachmentStoreOp::eDontCare,
			options.PreserveDepth ? vk::ImageLayout::eDepthStencilAttachmentOptimal : vk::ImageLayout::eUndefined,
			options.ShaderReadsDepth ? vk::ImageLayout::eShaderReadOnlyOptimal : vk::ImageLayout::eDepthStencilAttachmentOptimal
		);

		depthStencilAttachmentRef = { attachmentIndex, vk::ImageLayout::eDepthStencilAttachmentOptimal };
	}

	vk::SubpassDescription subpass({}, vk::PipelineBindPoint::eGraphics, {}, colorAttachmentRefs);

	if (depthStencilAttachmentFormat != vk::Format::eUndefined) {
		subpass.setPDepthStencilAttachment(&depthStencilAttachmentRef);
	}

	const vk::RenderPassCreateInfo createInfo({}, attachments, subpass);
	const auto [result, renderPass] = m_device.createRenderPass(createInfo);
	//DebugCheckCriticalVk(result, "Failed to create Vulkan render pass.");
	return renderPass;
}

void VulkanDevice::DestroyRenderPass(vk::RenderPass renderPass) {
	m_device.destroyRenderPass(renderPass);
}

vk::Framebuffer VulkanDevice::CreateFramebuffer(
	vk::RenderPass                                    renderPass,
	const vk::ArrayProxyNoTemporaries<vk::ImageView>& attachments,
	const vk::Extent2D& extent,
	uint32_t                                          layers
) {
	const vk::FramebufferCreateInfo createInfo({}, renderPass, attachments, extent.width, extent.height, layers);
	const auto [result, framebuffer] = m_device.createFramebuffer(createInfo);
	//DebugCheckCriticalVk(result, "Failed to create Vulkan framebuffer.");
	return framebuffer;
}

void VulkanDevice::DestroyFramebuffer(vk::Framebuffer framebuffer) {
	m_device.destroyFramebuffer(framebuffer);
}

vk::DescriptorSetLayout VulkanDevice::CreateDescriptorSetLayout(const vk::ArrayProxyNoTemporaries<vk::DescriptorSetLayoutBinding>& bindings) {
	const vk::DescriptorSetLayoutCreateInfo createInfo({}, bindings);
	const auto [result, descriptorSetLayout] = m_device.createDescriptorSetLayout(createInfo);
	//DebugCheckCriticalVk(result, "Failed to create Vulkan descriptor set layout.");
	return descriptorSetLayout;
}

void VulkanDevice::DestroyDescriptorSetLayout(vk::DescriptorSetLayout descriptorSetLayout) {
	m_device.destroyDescriptorSetLayout(descriptorSetLayout);
}

vk::DescriptorSet VulkanDevice::AllocateDescriptorSet(vk::DescriptorSetLayout descriptorSetLayout) {
	const vk::DescriptorSetAllocateInfo allocateInfo(m_descriptorPool, descriptorSetLayout);
	vk::DescriptorSet                   descriptorSet;
	if(m_device.allocateDescriptorSets(&allocateInfo, &descriptorSet) != vk::Result::eSuccess)
		Fatal("Failed to allocate Vulkan descriptor set.");
	return descriptorSet;
}

void VulkanDevice::FreeDescriptorSet(vk::DescriptorSet descriptorSet) {
	m_device.freeDescriptorSets(m_descriptorPool, descriptorSet);
}

void VulkanDevice::WriteCombinedImageSamplerToDescriptorSet(
	vk::Sampler       sampler,
	vk::ImageView     imageView,
	vk::DescriptorSet descriptorSet,
	uint32_t          binding
) {
	const vk::DescriptorImageInfo imageInfo(sampler, imageView, vk::ImageLayout::eShaderReadOnlyOptimal);
	const vk::WriteDescriptorSet  writeDescriptorSet(descriptorSet, binding, 0, vk::DescriptorType::eCombinedImageSampler, imageInfo);
	WriteDescriptorSet(writeDescriptorSet);
}

void VulkanDevice::WriteDynamicUniformBufferToDescriptorSet(
	vk::Buffer        buffer,
	vk::DeviceSize    size,
	vk::DescriptorSet descriptorSet,
	uint32_t          binding
) {
	const vk::DescriptorBufferInfo bufferInfo(buffer, 0, size);
	const vk::WriteDescriptorSet   writeDescriptorSet(descriptorSet, binding, 0, vk::DescriptorType::eUniformBufferDynamic, {}, bufferInfo);
	WriteDescriptorSet(writeDescriptorSet);
}

vk::PipelineLayout VulkanDevice::CreatePipelineLayout(
	const std::initializer_list<vk::DescriptorSetLayout>& descriptorSetLayouts,
	const std::initializer_list<vk::PushConstantRange>& pushConstantRanges
) {
	const vk::PipelineLayoutCreateInfo createInfo({}, descriptorSetLayouts, pushConstantRanges);
	const auto [result, pipelineLayout] = m_device.createPipelineLayout(createInfo);
	//DebugCheckCriticalVk(result, "Failed to create Vulkan pipeline layout.");
	return pipelineLayout;
}

void VulkanDevice::DestroyPipelineLayout(vk::PipelineLayout pipelineLayout) {
	m_device.destroyPipelineLayout(pipelineLayout);
}

vk::ShaderModule VulkanDevice::CreateShaderModule(const std::vector<uint32_t>& spirv) {
	const vk::ShaderModuleCreateInfo createInfo({}, spirv.size() * sizeof(uint32_t), spirv.data());
	const auto [result, shaderModule] = m_device.createShaderModule(createInfo);
	//DebugCheckCriticalVk(result, "Failed to create Vulkan shader module.");
	return shaderModule;
}

void VulkanDevice::DestroyShaderModule(vk::ShaderModule shaderModule) {
	m_device.destroyShaderModule(shaderModule);
}

vk::Pipeline VulkanDevice::CreatePipeline(
	vk::PipelineLayout                                                    pipelineLayout,
	const vk::PipelineVertexInputStateCreateInfo* vertexInput,
	const vk::ArrayProxyNoTemporaries<vk::PipelineShaderStageCreateInfo>& shaderStages,
	const VulkanPipelineOptions& options,
	const std::initializer_list<vk::PipelineColorBlendAttachmentState>& attachmentColorBlends,
	vk::RenderPass                                                        renderPass,
	uint32_t                                                              subpass
) {
	const vk::PipelineInputAssemblyStateCreateInfo inputAssemblyState({}, options.Topology);

	const vk::PipelineViewportStateCreateInfo viewportState({}, 1, nullptr, 1, nullptr);

	const vk::PipelineRasterizationStateCreateInfo rasterizationState(
		{},
		VK_FALSE,
		VK_FALSE,
		options.PolygonMode,
		options.CullMode,
		vk::FrontFace::eCounterClockwise,
		VK_FALSE,
		0.0f,
		0.0f,
		0.0f,
		1.0f
	);

	const vk::PipelineMultisampleStateCreateInfo multisampleState({}, vk::SampleCountFlagBits::e1);

	const vk::PipelineDepthStencilStateCreateInfo depthStencilState({}, options.DepthTestEnable, options.DepthWriteEnable, options.DepthCompareOp);

	const vk::PipelineColorBlendStateCreateInfo colorBlendState({}, VK_FALSE, vk::LogicOp::eCopy, attachmentColorBlends);

	const vk::DynamicState                   dynamicStates[] = { vk::DynamicState::eViewport, vk::DynamicState::eScissor };
	const vk::PipelineDynamicStateCreateInfo dynamicState({}, dynamicStates);

	const vk::GraphicsPipelineCreateInfo createInfo(
		{},
		shaderStages,
		vertexInput,
		&inputAssemblyState,
		nullptr,
		&viewportState,
		&rasterizationState,
		&multisampleState,
		&depthStencilState,
		&colorBlendState,
		&dynamicState,
		pipelineLayout,
		renderPass,
		subpass
	);
	const auto [result, pipeline] = m_device.createGraphicsPipeline({}, createInfo);
	//DebugCheckCriticalVk(result, "Failed to create Vulkan graphics pipeline.");
	return pipeline;
}

void VulkanDevice::DestroyPipeline(vk::Pipeline pipeline)
{
	m_device.destroyPipeline(pipeline);
}

void BeginCommandBuffer(vk::CommandBuffer commandBuffer, vk::CommandBufferUsageFlags flags)
{
	if(commandBuffer.begin({ flags })!= vk::Result::eSuccess)
		Fatal("Failed to begin Vulkan command buffer.");
}

void EndCommandBuffer(vk::CommandBuffer commandBuffer)
{
	if(commandBuffer.end() != vk::Result::eSuccess)
		Fatal("Failed to end Vulkan command buffer.");
}


#pragma endregion

#pragma region VulkanBase

static vk::Extent2D CalcSwapchainExtent(const vk::SurfaceCapabilitiesKHR& capabilities, GLFWwindow* window) {
	if (capabilities.currentExtent.width == std::numeric_limits<uint32_t>::max() &&
		capabilities.currentExtent.height == std::numeric_limits<uint32_t>::max()) {
		int width, height;
		glfwGetFramebufferSize(window, &width, &height);
		const vk::Extent2D extent(
			std::clamp(static_cast<uint32_t>(width), capabilities.minImageExtent.width, capabilities.maxImageExtent.width),
			std::clamp(static_cast<uint32_t>(height), capabilities.minImageExtent.height, capabilities.maxImageExtent.height)
		);
		return extent;
	}
	return capabilities.currentExtent;
}

VulkanBase::VulkanBase(GLFWwindow* window)
	: m_window(window) {
	CreateImmediateContext();
	CreateBufferingObjects();
	CreateSurfaceSwapchainAndImageViews();
	CreatePrimaryRenderPass();
	CreatePrimaryFramebuffers();
}

void VulkanBase::CreateImmediateContext() {
	m_immediateFence = CreateFence();
	m_immediateCommandBuffer = AllocateCommandBuffer();
}

void VulkanBase::CreateBufferingObjects() {
	for (BufferingObjects& bufferingObject : m_bufferingObjects) {
		bufferingObject.RenderFence = CreateFence(vk::FenceCreateFlagBits::eSignaled);
		bufferingObject.PresentSemaphore = CreateSemaphore();
		bufferingObject.RenderSemaphore = CreateSemaphore();
		bufferingObject.CommandBuffer = AllocateCommandBuffer();
	}
}

void VulkanBase::CreateSurfaceSwapchainAndImageViews() {
	m_surface = CreateSurface(m_window);

	const auto [capabilitiesResult, capabilities] = m_physicalDevice.getSurfaceCapabilitiesKHR(m_surface);
	//DebugCheckCriticalVk(capabilitiesResult, "Failed to get Vulkan surface capabilities.");
	uint32_t imageCount = capabilities.minImageCount + 1;
	if (capabilities.maxImageCount > 0 && imageCount > capabilities.maxImageCount) {
		imageCount = capabilities.maxImageCount;
	}
	m_swapchainExtent = CalcSwapchainExtent(capabilities, m_window);

	m_swapchain = CreateSwapchain(
		m_surface,
		imageCount,
		SURFACE_FORMAT,
		SURFACE_COLOR_SPACE,
		m_swapchainExtent,
		capabilities.currentTransform,
		PRESENT_MODE,
		m_swapchain
	);

	const auto [swapchainImagesResult, swapchainImages] = m_device.getSwapchainImagesKHR(m_swapchain);
	//DebugCheckCriticalVk(swapchainImagesResult, "Failed to get Vulkan swapchain images.");
	for (const vk::Image& image : swapchainImages) {
		m_swapchainImageViews.push_back(CreateImageView(image, SURFACE_FORMAT, vk::ImageAspectFlagBits::eColor));
	}
}

void VulkanBase::CreatePrimaryRenderPass() {
	VulkanRenderPassOptions options;
	options.ForPresentation = true;

	m_primaryRenderPass = CreateRenderPass(
		{ SURFACE_FORMAT }, //
		vk::Format::eUndefined,
		options
	);
}

void VulkanBase::CreatePrimaryFramebuffers() {
	const size_t numSwapchainImages = m_swapchainImageViews.size();
	for (int i = 0; i < numSwapchainImages; i++) {
		m_primaryFramebuffers.push_back(CreateFramebuffer(m_primaryRenderPass, m_swapchainImageViews[i], m_swapchainExtent));
	}

	const vk::Rect2D renderArea{
		{0, 0},
		m_swapchainExtent
	};
	static const vk::ClearValue clearValues{ vk::ClearColorValue{std::array<float, 4>{0.0f, 0.0f, 0.0f, 1.0f}} };

	vk::RenderPassBeginInfo beginInfo(m_primaryRenderPass, {}, renderArea, clearValues);
	for (const vk::Framebuffer& framebuffer : m_primaryFramebuffers) {
		beginInfo.framebuffer = framebuffer;
		m_primaryRenderPassBeginInfos.push_back(beginInfo);
	}
}

void VulkanBase::CleanupSurfaceSwapchainAndImageViews() {
	for (const vk::ImageView& imageView : m_swapchainImageViews) {
		m_device.destroy(imageView);
	}
	m_swapchainImageViews.clear();
	m_device.destroy(m_swapchain);
	m_swapchain = VK_NULL_HANDLE;
	m_instance.destroy(m_surface);
	m_surface = VK_NULL_HANDLE;
}

void VulkanBase::CleanupPrimaryFramebuffers() {
	m_primaryRenderPassBeginInfos.clear();
	for (const vk::Framebuffer& framebuffer : m_primaryFramebuffers) {
		DestroyFramebuffer(framebuffer);
	}
	m_primaryFramebuffers.clear();
}

void VulkanBase::RecreateSwapchain() {
	// block when minimized
	while (true) {
		int width, height;
		glfwGetFramebufferSize(m_window, &width, &height);
		if (width != 0 && height != 0) {
			break;
		}
		glfwWaitEvents();
	}

	WaitIdle();

	CleanupPrimaryFramebuffers();
	CleanupSurfaceSwapchainAndImageViews();
	CreateSurfaceSwapchainAndImageViews();
	CreatePrimaryFramebuffers();
}

VulkanBase::~VulkanBase() {
	WaitIdle();

	for (const BufferingObjects& bufferingObject : m_bufferingObjects) {
		m_device.destroy(bufferingObject.RenderFence);
		m_device.destroy(bufferingObject.PresentSemaphore);
		m_device.destroy(bufferingObject.RenderSemaphore);
		m_device.free(m_commandPool, bufferingObject.CommandBuffer);
	}

	CleanupPrimaryFramebuffers();
	DestroyRenderPass(m_primaryRenderPass);
	CleanupSurfaceSwapchainAndImageViews();

	m_device.free(m_commandPool, m_immediateCommandBuffer);
	m_device.destroy(m_immediateFence);
}

vk::Result VulkanBase::TryAcquiringNextSwapchainImage() {
	const BufferingObjects& bufferingObjects = m_bufferingObjects[m_currentBufferingIndex];

	return m_device.acquireNextImageKHR(m_swapchain, 100'000'000, bufferingObjects.PresentSemaphore, nullptr, &m_currentSwapchainImageIndex);
}

void VulkanBase::AcquireNextSwapchainImage() {
	vk::Result result = TryAcquiringNextSwapchainImage();
	if (result != vk::Result::eSuccess && result != vk::Result::eSuboptimalKHR) {
		if (result == vk::Result::eErrorOutOfDateKHR) {
			// recreate swapchain and try acquiring again
			RecreateSwapchain();
			result = TryAcquiringNextSwapchainImage();
		}

		//DebugCheckCriticalVk(result, "Failed to acquire next Vulkan swapchain image.");
	}
}

VulkanFrameInfo VulkanBase::BeginFrame() {
	const BufferingObjects& bufferingObjects = m_bufferingObjects[m_currentBufferingIndex];

	WaitAndResetFence(bufferingObjects.RenderFence);

	AcquireNextSwapchainImage();

	bufferingObjects.CommandBuffer.reset();
	BeginCommandBuffer(bufferingObjects.CommandBuffer, vk::CommandBufferUsageFlagBits::eOneTimeSubmit);

	return { &m_primaryRenderPassBeginInfos[m_currentSwapchainImageIndex], m_currentBufferingIndex, bufferingObjects.CommandBuffer };
}

void VulkanBase::EndFrame() {
	BufferingObjects& bufferingObjects = m_bufferingObjects[m_currentBufferingIndex];

	EndCommandBuffer(bufferingObjects.CommandBuffer);

	vk::PipelineStageFlags waitStage = vk::PipelineStageFlagBits::eColorAttachmentOutput;
	const vk::SubmitInfo   submitInfo(bufferingObjects.PresentSemaphore, waitStage, bufferingObjects.CommandBuffer, bufferingObjects.RenderSemaphore);
	SubmitToGraphicsQueue(submitInfo, bufferingObjects.RenderFence);

	const vk::PresentInfoKHR presentInfo(bufferingObjects.RenderSemaphore, m_swapchain, m_currentSwapchainImageIndex);
	const vk::Result         result = m_graphicsQueue.presentKHR(presentInfo);
	if (result == vk::Result::eErrorOutOfDateKHR || result == vk::Result::eSuboptimalKHR) {
		RecreateSwapchain();
	}
	else {
		//DebugCheckCriticalVk(result, "Failed to present Vulkan swapchain image.");
	}

	m_currentBufferingIndex = (m_currentBufferingIndex + 1) % m_bufferingObjects.size();
}

#pragma endregion