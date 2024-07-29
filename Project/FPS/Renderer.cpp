#include "Base.h"
#include "Core.h"
#include "Renderer.h"

#pragma region DeferredFramebuffer

DeferredFramebuffer::DeferredFramebuffer(
	VulkanRender* device,
	vk::RenderPass                                 renderPass,
	vk::DescriptorSetLayout                        textureSetLayout,
	vk::Sampler                                    sampler,
	const vk::Extent2D& extent,
	const vk::ArrayProxyNoTemporaries<vk::Format>& colorFormats
)
	: m_device(device)
{
	CreateAttachments(extent, colorFormats);
	CreateAttachmentViews(colorFormats);

	std::vector<vk::ImageView> attachmentViews = m_colorAttachmentViews;
	attachmentViews.push_back(m_depthAttachmentView);

	m_framebuffer = m_device->CreateFramebuffer(renderPass, attachmentViews, extent);

	m_textureSet = m_device->AllocateDescriptorSet(textureSetLayout);

	for (int i = 0; i < m_colorAttachmentViews.size(); i++)
	{
		m_device->WriteCombinedImageSamplerToDescriptorSet(
			sampler, //
			m_colorAttachmentViews[i],
			m_textureSet,
			i
		);
	}
}

void DeferredFramebuffer::CreateAttachments(const vk::Extent2D& extent, const vk::ArrayProxyNoTemporaries<vk::Format>& colorFormats)
{
	for (const vk::Format& colorFormat : colorFormats)
	{
		VulkanImage attachment = m_device->CreateImage(
			colorFormat,
			extent,
			vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled,
			0,
			VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE
		);
		m_colorAttachments.push_back(std::move(attachment));
	}
	m_depthAttachment = m_device->CreateImage(
		vk::Format::eD32Sfloat, //
		extent,
		vk::ImageUsageFlagBits::eDepthStencilAttachment,
		0,
		VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE
	);
}

void DeferredFramebuffer::CreateAttachmentViews(const vk::ArrayProxyNoTemporaries<vk::Format>& colorFormats)
{
	uint32_t i = 0;
	for (const vk::Format& colorFormat : colorFormats)
	{
		const vk::ImageView colorAttachmentView = m_device->CreateImageView(
			m_colorAttachments[i++].Get(), //
			colorFormat,
			vk::ImageAspectFlagBits::eColor
		);
		m_colorAttachmentViews.push_back(colorAttachmentView);
	}
	m_depthAttachmentView = m_device->CreateImageView(
		m_depthAttachment.Get(), //
		vk::Format::eD32Sfloat,
		vk::ImageAspectFlagBits::eDepth
	);
}

void DeferredFramebuffer::Release()
{
	if (m_device)
	{
		m_device->FreeDescriptorSet(m_textureSet);
		m_device->DestroyFramebuffer(m_framebuffer);
		m_device->DestroyImageView(m_depthAttachmentView);
		for (const vk::ImageView& imageView : m_colorAttachmentViews)
		{
			m_device->DestroyImageView(imageView);
		}
	}

	m_device = nullptr;
	m_colorAttachments.clear();
	m_depthAttachment = {};
	m_colorAttachmentViews.clear();
	m_depthAttachmentView = VK_NULL_HANDLE;
	m_framebuffer = VK_NULL_HANDLE;
	m_textureSet = VK_NULL_HANDLE;
}

void DeferredFramebuffer::Swap(DeferredFramebuffer& other) noexcept
{
	std::swap(m_device, other.m_device);
	std::swap(m_colorAttachments, other.m_colorAttachments);
	std::swap(m_depthAttachment, other.m_depthAttachment);
	std::swap(m_colorAttachmentViews, other.m_colorAttachmentViews);
	std::swap(m_depthAttachmentView, other.m_depthAttachmentView);
	std::swap(m_framebuffer, other.m_framebuffer);
	std::swap(m_textureSet, other.m_textureSet);
}

#pragma endregion

#pragma region ForwardFramebuffer

ForwardFramebuffer::ForwardFramebuffer(
	VulkanRender* device,
	vk::RenderPass          renderPass,
	vk::DescriptorSetLayout textureSetLayout,
	vk::Sampler             sampler,
	const vk::Extent2D& extent,
	vk::ImageView           depthImageView
)
	: m_device(device)
{
	CreateAttachments(extent);
	CreateAttachmentViews();

	std::vector<vk::ImageView> attachmentViews{ m_colorAttachmentView, depthImageView };

	m_framebuffer = m_device->CreateFramebuffer(renderPass, attachmentViews, extent);

	m_textureSet = m_device->AllocateDescriptorSet(textureSetLayout);

	m_device->WriteCombinedImageSamplerToDescriptorSet(
		sampler, //
		m_colorAttachmentView,
		m_textureSet,
		0
	);
}

void ForwardFramebuffer::CreateAttachments(const vk::Extent2D& extent)
{
	m_colorAttachment = m_device->CreateImage(
		vk::Format::eR32G32B32A32Sfloat,
		extent,
		vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled,
		0,
		VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE
	);
}

void ForwardFramebuffer::CreateAttachmentViews()
{
	m_colorAttachmentView = m_device->CreateImageView(
		m_colorAttachment.Get(), //
		vk::Format::eR32G32B32A32Sfloat,
		vk::ImageAspectFlagBits::eColor
	);
}

void ForwardFramebuffer::Release()
{
	if (m_device)
	{
		m_device->FreeDescriptorSet(m_textureSet);
		m_device->DestroyFramebuffer(m_framebuffer);
		m_device->DestroyImageView(m_colorAttachmentView);
	}

	m_device = nullptr;
	m_colorAttachment = {};
	m_colorAttachmentView = VK_NULL_HANDLE;
	m_framebuffer = VK_NULL_HANDLE;
	m_textureSet = VK_NULL_HANDLE;
}

void ForwardFramebuffer::Swap(ForwardFramebuffer& other) noexcept
{
	std::swap(m_device, other.m_device);
	std::swap(m_colorAttachment, other.m_colorAttachment);
	std::swap(m_colorAttachmentView, other.m_colorAttachmentView);
	std::swap(m_framebuffer, other.m_framebuffer);
	std::swap(m_textureSet, other.m_textureSet);
}

#pragma endregion

#pragma region PostProcessingFramebuffer

PostProcessingFramebuffer::PostProcessingFramebuffer(
	VulkanRender* device,
	vk::RenderPass          renderPass,
	vk::DescriptorSetLayout textureSetLayout,
	vk::Sampler             sampler,
	const vk::Extent2D& extent,
	vk::Format              format
)
	: m_device(device)
	, m_format(format)
{
	CreateAttachments(extent);
	CreateAttachmentViews();

	m_framebuffer = m_device->CreateFramebuffer(renderPass, m_colorAttachmentView, extent);

	m_textureSet = m_device->AllocateDescriptorSet(textureSetLayout);

	m_device->WriteCombinedImageSamplerToDescriptorSet(
		sampler, //
		m_colorAttachmentView,
		m_textureSet,
		0
	);
}

void PostProcessingFramebuffer::CreateAttachments(const vk::Extent2D& extent)
{
	m_colorAttachment = m_device->CreateImage(
		m_format,
		extent,
		vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled,
		0,
		VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE
	);
}

void PostProcessingFramebuffer::CreateAttachmentViews()
{
	m_colorAttachmentView = m_device->CreateImageView(
		m_colorAttachment.Get(), //
		m_format,
		vk::ImageAspectFlagBits::eColor
	);
}

void PostProcessingFramebuffer::Release()
{
	if (m_device)
	{
		m_device->FreeDescriptorSet(m_textureSet);
		m_device->DestroyFramebuffer(m_framebuffer);
		m_device->DestroyImageView(m_colorAttachmentView);
	}

	m_device = nullptr;
	m_colorAttachment = {};
	m_colorAttachmentView = VK_NULL_HANDLE;
	m_framebuffer = VK_NULL_HANDLE;
	m_textureSet = VK_NULL_HANDLE;
}

void PostProcessingFramebuffer::Swap(PostProcessingFramebuffer& other) noexcept
{
	std::swap(m_device, other.m_device);
	std::swap(m_colorAttachment, other.m_colorAttachment);
	std::swap(m_colorAttachmentView, other.m_colorAttachmentView);
	std::swap(m_framebuffer, other.m_framebuffer);
	std::swap(m_textureSet, other.m_textureSet);
}

#pragma endregion

#pragma region DeferredContext

DeferredContext::DeferredContext(VulkanRender& device)
	: m_device(&device)
{
	m_sampler = m_device->CreateSampler(vk::Filter::eNearest, vk::SamplerAddressMode::eClampToEdge);

	CreateRenderPass();
	CreateFramebuffers();
}

void DeferredContext::CreateRenderPass()
{
	m_deferredRenderPass = m_device->CreateRenderPass(
		{ vk::Format::eR32G32B32A32Sfloat, //
		vk::Format::eR32G32B32A32Sfloat,
		vk::Format::eR32G32B32A32Sfloat,
		vk::Format::eR32G32B32A32Sfloat },
		vk::Format::eD32Sfloat
	);

	vk::DescriptorSetLayoutBinding deferredBindings[]
	{
	{0, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment},
	{1, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment},
	{2, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment},
	{3, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment}
	};
	m_deferredTextureSetLayout = m_device->CreateDescriptorSetLayout(deferredBindings);

	VulkanRenderPassOptions forwardRenderPassOptions;
	forwardRenderPassOptions.PreserveDepth = true;

	m_forwardRenderPass = m_device->CreateRenderPass(
		{ vk::Format::eR32G32B32A32Sfloat }, //
		vk::Format::eD32Sfloat,
		forwardRenderPassOptions
	);

	vk::DescriptorSetLayoutBinding forwardBindings[]{
	{0, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment}
	};
	m_forwardTextureSetLayout = m_device->CreateDescriptorSetLayout(forwardBindings);
}

void DeferredContext::CreateFramebuffers()
{
	m_extent = m_device->GetScaledExtent();

	const size_t numBuffering = m_device->GetNumBuffering();

	const vk::Rect2D renderArea{
	{0, 0},
	m_extent
	};

	{
		static const vk::ClearValue clearValues[]{
		{vk::ClearColorValue{std::array<float, 4>{0.0f, 0.0f, 0.0f, 0.0f}}},
		{vk::ClearColorValue{std::array<float, 4>{0.0f, 0.0f, 0.0f, 0.0f}}},
		{vk::ClearColorValue{std::array<float, 4>{0.0f, 0.0f, 0.0f, 0.0f}}},
		{vk::ClearColorValue{std::array<float, 4>{0.0f, 0.0f, 0.0f, 0.0f}}},
		{vk::ClearDepthStencilValue{1.0f, 0}} };
		static vk::Format colorFormats[] = {
		vk::Format::eR32G32B32A32Sfloat, //
		vk::Format::eR32G32B32A32Sfloat,
		vk::Format::eR32G32B32A32Sfloat,
		vk::Format::eR32G32B32A32Sfloat };
		vk::RenderPassBeginInfo beginInfo(
			m_deferredRenderPass, //
			{},
			renderArea,
			clearValues
		);
		for (int i = 0; i < numBuffering; i++) {
			const DeferredFramebuffer& framebuffer = m_deferredFramebuffers.emplace_back(
				m_device, //
				m_deferredRenderPass,
				m_deferredTextureSetLayout,
				m_sampler,
				m_extent,
				colorFormats
			);
			beginInfo.framebuffer = framebuffer.GetFramebuffer();
			m_deferredRenderPassBeginInfo.push_back(beginInfo);
		}
	}

	{
		static const vk::ClearValue clearValues[]{
		{vk::ClearColorValue{std::array<float, 4>{0.0f, 0.0f, 0.0f, 0.0f}}},
		};
		vk::RenderPassBeginInfo beginInfo(
			m_forwardRenderPass, //
			{},
			renderArea,
			clearValues
		);
		for (int i = 0; i < numBuffering; i++) {
			const ForwardFramebuffer& framebuffer = m_forwardFramebuffers.emplace_back(
				m_device, //
				m_forwardRenderPass,
				m_forwardTextureSetLayout,
				m_sampler,
				m_extent,
				m_deferredFramebuffers[i].GetDepthAttachmentView()
			);
			beginInfo.framebuffer = framebuffer.GetFramebuffer();
			m_forwardRenderPassBeginInfo.push_back(beginInfo);
		}
	}
}

void DeferredContext::CleanupFramebuffers()
{
	m_extent = vk::Extent2D{};
	m_deferredFramebuffers.clear();
	m_deferredRenderPassBeginInfo.clear();
	m_forwardFramebuffers.clear();
	m_forwardRenderPassBeginInfo.clear();
}

void DeferredContext::Release()
{
	if (m_device) {
		m_device->DestroySampler(m_sampler);
		m_device->DestroyDescriptorSetLayout(m_deferredTextureSetLayout);
		m_device->DestroyRenderPass(m_deferredRenderPass);
		m_device->DestroyDescriptorSetLayout(m_forwardTextureSetLayout);
		m_device->DestroyRenderPass(m_forwardRenderPass);
	}

	m_device = nullptr;
	m_deferredRenderPass = VK_NULL_HANDLE;
	m_deferredTextureSetLayout = VK_NULL_HANDLE;
	m_forwardRenderPass = VK_NULL_HANDLE;
	m_forwardTextureSetLayout = VK_NULL_HANDLE;
	m_sampler = VK_NULL_HANDLE;
	CleanupFramebuffers();
}

void DeferredContext::Swap(DeferredContext& other) noexcept
{
	std::swap(m_device, other.m_device);
	std::swap(m_deferredRenderPass, other.m_deferredRenderPass);
	std::swap(m_deferredTextureSetLayout, other.m_deferredTextureSetLayout);
	std::swap(m_forwardRenderPass, other.m_forwardRenderPass);
	std::swap(m_forwardTextureSetLayout, other.m_forwardTextureSetLayout);
	std::swap(m_sampler, other.m_sampler);
	std::swap(m_extent, other.m_extent);
	std::swap(m_deferredFramebuffers, other.m_deferredFramebuffers);
	std::swap(m_deferredRenderPassBeginInfo, other.m_deferredRenderPassBeginInfo);
	std::swap(m_forwardFramebuffers, other.m_forwardFramebuffers);
	std::swap(m_forwardRenderPassBeginInfo, other.m_forwardRenderPassBeginInfo);
}

void DeferredContext::CheckFramebuffersOutOfDate()
{
	if (m_device->GetScaledExtent() == m_extent) {
		return;
	}

	m_device->WaitIdle();
	CleanupFramebuffers();
	CreateFramebuffers();
}

#pragma endregion

#pragma region PostProcessingContext

PostProcessingContext::PostProcessingContext(VulkanRender& device)
	: m_device(&device)
{
	m_sampler = m_device->CreateSampler(vk::Filter::eLinear, vk::SamplerAddressMode::eClampToEdge);

	CreateRenderPass();
	CreateFramebuffers();
}

void PostProcessingContext::CreateRenderPass()
{
	m_renderPass = m_device->CreateRenderPass(
		{ vk::Format::eR8G8B8A8Unorm }, //
		vk::Format::eUndefined
	);

	vk::DescriptorSetLayoutBinding bindings[]{
		{0, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment}
	};
	m_textureSetLayout = m_device->CreateDescriptorSetLayout(bindings);
}

void PostProcessingContext::CreateFramebuffers()
{
	m_extent = m_device->GetScaledExtent();

	const size_t numBuffering = m_device->GetNumBuffering();

	const vk::Rect2D renderArea{
		{0, 0},
		m_extent
	};

	static const vk::ClearValue clearValues[]{
		{vk::ClearColorValue{std::array<float, 4>{0.0f, 0.0f, 0.0f, 0.0f}}},
	};
	vk::RenderPassBeginInfo beginInfo(
		m_renderPass, //
		{},
		renderArea,
		clearValues
	);
	for (int i = 0; i < numBuffering; i++) {
		const PostProcessingFramebuffer& framebuffer = m_framebuffers.emplace_back(
			m_device, //
			m_renderPass,
			m_textureSetLayout,
			m_sampler,
			m_extent,
			vk::Format::eR8G8B8A8Unorm
		);
		beginInfo.framebuffer = framebuffer.GetFramebuffer();
		m_renderPassBeginInfo.push_back(beginInfo);
	}
}

void PostProcessingContext::CleanupFramebuffers()
{
	m_extent = vk::Extent2D{};
	m_framebuffers.clear();
	m_renderPassBeginInfo.clear();
}

void PostProcessingContext::Release()
{
	if (m_device) {
		m_device->DestroySampler(m_sampler);
		m_device->DestroyDescriptorSetLayout(m_textureSetLayout);
		m_device->DestroyRenderPass(m_renderPass);
	}

	m_device = nullptr;
	m_renderPass = VK_NULL_HANDLE;
	m_textureSetLayout = VK_NULL_HANDLE;
	m_sampler = VK_NULL_HANDLE;
	CleanupFramebuffers();
}

void PostProcessingContext::Swap(PostProcessingContext& other) noexcept {
	std::swap(m_device, other.m_device);
	std::swap(m_extent, other.m_extent);
	std::swap(m_sampler, other.m_sampler);
	std::swap(m_renderPass, other.m_renderPass);
	std::swap(m_textureSetLayout, other.m_textureSetLayout);
	std::swap(m_framebuffers, other.m_framebuffers);
	std::swap(m_renderPassBeginInfo, other.m_renderPassBeginInfo);
}

void PostProcessingContext::CheckFramebuffersOutOfDate() {
	if (m_device->GetScaledExtent() == m_extent) {
		return;
	}

	m_device->WaitIdle();
	CleanupFramebuffers();
	CreateFramebuffers();
}

#pragma endregion

#pragma region ShadowMap

ShadowMap::ShadowMap(
	VulkanRender* device,
	vk::RenderPass          renderPass,
	vk::DescriptorSetLayout textureSetLayout,
	vk::Sampler             sampler,
	const vk::Extent2D& extent
)
	: m_device(device) {
	CreateAttachment(extent);
	CreateAttachmentView();

	m_framebuffer = m_device->CreateFramebuffer(
		renderPass, //
		m_depthAttachmentView,
		extent,
		4
	);

	m_textureSet = m_device->AllocateDescriptorSet(textureSetLayout);

	m_device->WriteCombinedImageSamplerToDescriptorSet(
		sampler, //
		m_depthAttachmentView,
		m_textureSet,
		0
	);
}

void ShadowMap::CreateAttachment(const vk::Extent2D& extent) {
	m_depthAttachment = m_device->CreateImage(
		vk::Format::eD32Sfloat,
		extent,
		vk::ImageUsageFlagBits::eDepthStencilAttachment | vk::ImageUsageFlagBits::eSampled,
		0,
		VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE,
		4
	);
}

void ShadowMap::CreateAttachmentView() {
	m_depthAttachmentView = m_device->CreateImageView(
		m_depthAttachment.Get(), //
		vk::Format::eD32Sfloat,
		vk::ImageAspectFlagBits::eDepth,
		4
	);
}

void ShadowMap::Release() {
	if (m_device) {
		m_device->FreeDescriptorSet(m_textureSet);
		m_device->DestroyFramebuffer(m_framebuffer);
		m_device->DestroyImageView(m_depthAttachmentView);
	}

	m_device = nullptr;
	m_depthAttachment = {};
	m_depthAttachmentView = VK_NULL_HANDLE;
	m_framebuffer = VK_NULL_HANDLE;
	m_textureSet = VK_NULL_HANDLE;
}

void ShadowMap::Swap(ShadowMap& other) noexcept {
	std::swap(m_device, other.m_device);
	std::swap(m_depthAttachment, other.m_depthAttachment);
	std::swap(m_depthAttachmentView, other.m_depthAttachmentView);
	std::swap(m_framebuffer, other.m_framebuffer);
	std::swap(m_textureSet, other.m_textureSet);
}

#pragma endregion

#pragma region ShadowContext

ShadowContext::ShadowContext(VulkanRender& device)
	: m_device(&device) {
	CreateRenderPass();
	CreateFramebuffers();
}

void ShadowContext::CreateRenderPass() {
	VulkanRenderPassOptions options;
	options.ShaderReadsDepth = true;

	m_renderPass = m_device->CreateRenderPass(
		{}, //
		vk::Format::eD32Sfloat,
		options
	);

	vk::DescriptorSetLayoutBinding bindings[]{
		{0, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment}
	};
	m_textureSetLayout = m_device->CreateDescriptorSetLayout(bindings);

	m_sampler = m_device->CreateSampler(
		vk::Filter::eLinear, //
		vk::SamplerAddressMode::eClampToEdge,
		VK_TRUE,
		vk::CompareOp::eGreaterOrEqual
	);
}

void ShadowContext::CreateFramebuffers() {
	m_extent = vk::Extent2D{ 4096, 4096 };
	const size_t numBuffering = m_device->GetNumBuffering();

	const vk::Rect2D renderArea{
		{0, 0},
		m_extent
	};
	static vk::ClearValue const clearValues[]{ {vk::ClearDepthStencilValue{1.0f, 0}} };

	vk::RenderPassBeginInfo beginInfo(
		m_renderPass, //
		{},
		renderArea,
		clearValues
	);


	for (int i = 0; i < numBuffering; i++)
	{
		const ShadowMap& framebuffer = m_framebuffers.emplace_back(
			m_device, //
			m_renderPass,
			m_textureSetLayout,
			m_sampler,
			m_extent
		);
		beginInfo.framebuffer = framebuffer.GetFramebuffer();
		m_renderPassBeginInfos.push_back(beginInfo);
	}
}

void ShadowContext::CleanupFramebuffers() {
	m_extent = vk::Extent2D{};
	m_framebuffers.clear();
	m_renderPassBeginInfos.clear();
}

void ShadowContext::Release() {
	if (m_device) {
		m_device->DestroySampler(m_sampler);
		m_device->DestroyDescriptorSetLayout(m_textureSetLayout);
		m_device->DestroyRenderPass(m_renderPass);
	}

	m_device = nullptr;
	m_renderPass = VK_NULL_HANDLE;
	m_textureSetLayout = VK_NULL_HANDLE;
	m_sampler = VK_NULL_HANDLE;
	CleanupFramebuffers();
}

void ShadowContext::Swap(ShadowContext& other) noexcept {
	std::swap(m_device, other.m_device);
	std::swap(m_renderPass, other.m_renderPass);
	std::swap(m_textureSetLayout, other.m_textureSetLayout);
	std::swap(m_sampler, other.m_sampler);
	std::swap(m_extent, other.m_extent);
	std::swap(m_framebuffers, other.m_framebuffers);
	std::swap(m_renderPassBeginInfos, other.m_renderPassBeginInfos);
}

#pragma endregion

#pragma region PbrMaterialCache

PbrMaterialConfig::PbrMaterialConfig(const std::string& jsonFilename) {
	JsonFile json(jsonFilename);
	Albedo = json.GetString("albedo");
	Normal = json.GetString("normal");
	MRA = json.GetString("mra");
	Emissive = json.GetString("emissive");
	Transparent = json.GetBoolean("transparent", false);
	Shadow = json.GetBoolean("shadow", !Transparent);
}


PbrMaterialCache::PbrMaterialCache(VulkanRender& device, TextureCache& textureCache)
	: m_device(device)
	, m_textureCache(textureCache) {
	vk::DescriptorSetLayoutBinding bindings[]{
		{0, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment},
		{1, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment},
		{2, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment},
		{3, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment}
	};
	m_textureSetLayout = m_device.CreateDescriptorSetLayout(bindings);
}

PbrMaterialCache::~PbrMaterialCache() {
	for (const auto& [name, material] : m_materials) {
		m_device.FreeDescriptorSet(material.DescriptorSet);
	}
	m_device.DestroyDescriptorSetLayout(m_textureSetLayout);
}

PbrMaterial* PbrMaterialCache::LoadMaterial(const std::string& filename) {
	auto pair = m_materials.find(filename);
	if (pair == m_materials.end())
	{
		//DebugInfo("Caching material {}.", filename);

		const vk::DescriptorSet textureSet = m_device.AllocateDescriptorSet(m_textureSetLayout);

		const PbrMaterialConfig config(filename);
		m_textureCache.LoadTexture(config.Albedo)->BindToDescriptorSet(textureSet, 0);
		m_textureCache.LoadTexture(config.Normal)->BindToDescriptorSet(textureSet, 1);
		m_textureCache.LoadTexture(config.MRA)->BindToDescriptorSet(textureSet, 2);
		m_textureCache.LoadTexture(config.Emissive)->BindToDescriptorSet(textureSet, 3);

		const PbrMaterial material{
			textureSet, //
			config.Transparent,
			config.Shadow };

		pair = m_materials.emplace(filename, material).first;
	}
	return &pair->second;
}

#pragma endregion

#pragma region SingleTextureMaterialCache

SingleTextureMaterialCache::SingleTextureMaterialCache(VulkanRender& device, TextureCache& textureCache)
	: m_device(device)
	, m_textureCache(textureCache) {
	vk::DescriptorSetLayoutBinding bindings[]{
		{0, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment}
	};
	m_textureSetLayout = m_device.CreateDescriptorSetLayout(bindings);
}

SingleTextureMaterialCache::~SingleTextureMaterialCache() {
	for (const auto& [name, material] : m_materials) {
		m_device.FreeDescriptorSet(material.DescriptorSet);
	}
	m_device.DestroyDescriptorSetLayout(m_textureSetLayout);
}

SingleTextureMaterial* SingleTextureMaterialCache::LoadMaterial(const std::string& filename) {
	auto pair = m_materials.find(filename);
	if (pair == m_materials.end()) {
		//DebugInfo("Caching texture material {}.", filename);

		const vk::DescriptorSet textureSet = m_device.AllocateDescriptorSet(m_textureSetLayout);

		m_textureCache.LoadTexture(filename)->BindToDescriptorSet(textureSet, 0);

		const SingleTextureMaterial material{ textureSet };

		pair = m_materials.emplace(filename, material).first;
	}
	return &pair->second;
}

#pragma endregion

#pragma region MeshUtilities

std::vector<VertexPositionOnly> CreateSkyboxVertices() {
	return {
		VertexPositionOnly{{-1.0f, 1.0f, 1.0f}},
		VertexPositionOnly{{1.0f, 1.0f, 1.0f}},
		VertexPositionOnly{{-1.0f, -1.0f, 1.0f}},
		VertexPositionOnly{{1.0f, -1.0f, 1.0f}},
		VertexPositionOnly{{1.0f, -1.0f, -1.0f}},
		VertexPositionOnly{{1.0f, 1.0f, 1.0f}},
		VertexPositionOnly{{1.0f, 1.0f, -1.0f}},
		VertexPositionOnly{{-1.0f, 1.0f, 1.0f}},
		VertexPositionOnly{{-1.0f, 1.0f, -1.0f}},
		VertexPositionOnly{{-1.0f, -1.0f, 1.0f}},
		VertexPositionOnly{{-1.0f, -1.0f, -1.0f}},
		VertexPositionOnly{{1.0f, -1.0f, -1.0f}},
		VertexPositionOnly{{-1.0f, 1.0f, -1.0f}},
		VertexPositionOnly{{1.0f, 1.0f, -1.0}} };
}

void AppendRectVertices(
	std::vector<VertexCanvas>& vertices,
	const glm::vec2& minPosition,
	const glm::vec2& maxPosition,
	const glm::vec2& minTexCoord,
	const glm::vec2& maxTexCoord
) {
	const glm::vec2 P00{ minPosition.x, minPosition.y };
	const glm::vec2 P01{ minPosition.x, maxPosition.y };
	const glm::vec2 P10{ maxPosition.x, minPosition.y };
	const glm::vec2 P11{ maxPosition.x, maxPosition.y };

	const glm::vec2 UV00{ minTexCoord.x, minTexCoord.y };
	const glm::vec2 UV01{ minTexCoord.x, maxTexCoord.y };
	const glm::vec2 UV10{ maxTexCoord.x, minTexCoord.y };
	const glm::vec2 UV11{ maxTexCoord.x, maxTexCoord.y };

	vertices.reserve(vertices.size() + 4);
	vertices.emplace_back(P00, UV00);
	vertices.emplace_back(P10, UV10);
	vertices.emplace_back(P01, UV01);
	vertices.emplace_back(P11, UV11);
}

void AppendBoxVertices(std::vector<VertexBase>& vertices, const glm::vec3& min, const glm::vec3& max) {
	const glm::vec3 P000{ min.x, min.y, min.z };
	const glm::vec3 P001{ min.x, min.y, max.z };
	const glm::vec3 P010{ min.x, max.y, min.z };
	const glm::vec3 P011{ min.x, max.y, max.z };
	const glm::vec3 P100{ max.x, min.y, min.z };
	const glm::vec3 P101{ max.x, min.y, max.z };
	const glm::vec3 P110{ max.x, max.y, min.z };
	const glm::vec3 P111{ max.x, max.y, max.z };

	constexpr glm::vec3 NPX{ 1, 0, 0 };
	constexpr glm::vec3 NNX{ -1, 0, 0 };
	constexpr glm::vec3 NPY{ 0, 1, 0 };
	constexpr glm::vec3 NNY{ 0, -1, 0 };
	constexpr glm::vec3 NPZ{ 0, 0, 1 };
	constexpr glm::vec3 NNZ{ 0, 0, -1 };

	const float WIDTH = max.x - min.x;
	const float HEIGHT = max.y - min.y;
	const float DEPTH = max.z - min.z;

	constexpr glm::vec2 UVX00{ 0.0f, 0.0f };
	const glm::vec2     UVX01{ 0.0f, HEIGHT };
	const glm::vec2     UVX10{ DEPTH, 0.0f };
	const glm::vec2     UVX11{ DEPTH, HEIGHT };

	constexpr glm::vec2 UVY00{ 0.0f, 0.0f };
	const glm::vec2     UVY01{ 0.0f, DEPTH };
	const glm::vec2     UVY10{ WIDTH, 0.0f };
	const glm::vec2     UVY11{ WIDTH, DEPTH };

	constexpr glm::vec2 UVZ00{ 0.0f, 0.0f };
	const glm::vec2     UVZ01{ 0.0f, HEIGHT };
	const glm::vec2     UVZ10{ WIDTH, 0.0f };
	const glm::vec2     UVZ11{ WIDTH, HEIGHT };

	vertices.reserve(vertices.size() + 36);
	// +x
	vertices.emplace_back(P100, NPX, UVX00);
	vertices.emplace_back(P101, NPX, UVX10);
	vertices.emplace_back(P110, NPX, UVX01);
	vertices.emplace_back(P110, NPX, UVX01);
	vertices.emplace_back(P101, NPX, UVX10);
	vertices.emplace_back(P111, NPX, UVX11);
	// -x
	vertices.emplace_back(P001, NNX, UVX00);
	vertices.emplace_back(P000, NNX, UVX10);
	vertices.emplace_back(P011, NNX, UVX01);
	vertices.emplace_back(P011, NNX, UVX01);
	vertices.emplace_back(P000, NNX, UVX10);
	vertices.emplace_back(P010, NNX, UVX11);
	// +y
	vertices.emplace_back(P010, NPY, UVY00);
	vertices.emplace_back(P110, NPY, UVY10);
	vertices.emplace_back(P011, NPY, UVY01);
	vertices.emplace_back(P011, NPY, UVY01);
	vertices.emplace_back(P110, NPY, UVY10);
	vertices.emplace_back(P111, NPY, UVY11);
	// -y
	vertices.emplace_back(P001, NNY, UVY00);
	vertices.emplace_back(P101, NNY, UVY10);
	vertices.emplace_back(P000, NNY, UVY01);
	vertices.emplace_back(P000, NNY, UVY01);
	vertices.emplace_back(P101, NNY, UVY10);
	vertices.emplace_back(P100, NNY, UVY11);
	// +z
	vertices.emplace_back(P101, NPZ, UVZ00);
	vertices.emplace_back(P001, NPZ, UVZ10);
	vertices.emplace_back(P111, NPZ, UVZ01);
	vertices.emplace_back(P111, NPZ, UVZ01);
	vertices.emplace_back(P001, NPZ, UVZ10);
	vertices.emplace_back(P011, NPZ, UVZ11);
	// -z
	vertices.emplace_back(P000, NNZ, UVZ00);
	vertices.emplace_back(P100, NNZ, UVZ10);
	vertices.emplace_back(P010, NNZ, UVZ01);
	vertices.emplace_back(P010, NNZ, UVZ01);
	vertices.emplace_back(P100, NNZ, UVZ10);
	vertices.emplace_back(P110, NNZ, UVZ11);
}

#pragma endregion

#pragma region PbrRenderer

PbrRenderer::PbrRenderer(GLFWwindow* window)
	: m_device(window)
	, m_textureCache(m_device)
	, m_pbrMaterialCache(m_device, m_textureCache)
	, m_singleTextureMaterialCache(m_device, m_textureCache)
	, m_meshCache(m_device) {
	m_shadowContext = ShadowContext(m_device);
	m_deferredContext = DeferredContext(m_device);
	m_toneMappingContext = PostProcessingContext(m_device);
	CreateUniformBuffers();
	CreateIblTextureSet();
	CreatePipelines();
	CreateSkyboxCube();
	CreateFullScreenQuad();
	CreateScreenPrimitiveMeshes();
}

void PbrRenderer::CreateUniformBuffers() {
	m_uniformBufferSet = VulkanUniformBufferSet(
		m_device,
		{
			{0, vk::ShaderStageFlagBits::eAllGraphics,                                   sizeof(RendererUniformData)},
			{1, vk::ShaderStageFlagBits::eGeometry | vk::ShaderStageFlagBits::eFragment, sizeof(LightingUniformData)}
		}
	);
}

void PbrRenderer::CreateIblTextureSet() {
	vk::DescriptorSetLayoutBinding iblBindings[]{
		{0, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment},
		{1, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment},
		{2, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment},
		{3, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment}
	};
	m_iblTextureSetLayout = m_device.CreateDescriptorSetLayout(iblBindings);

	m_iblTextureSet = m_device.AllocateDescriptorSet(m_iblTextureSetLayout);
	m_textureCache
		.LoadTexture(
			"Textures/brdf_lut.png", //
			vk::Filter::eLinear,
			vk::SamplerAddressMode::eClampToEdge
		)
		->BindToDescriptorSet(m_iblTextureSet, 0);
	m_textureCache
		.LoadTexture(
			"Textures/ibl/sunset.png", //
			vk::Filter::eLinear,
			vk::SamplerAddressMode::eClampToEdge
		)
		->BindToDescriptorSet(m_iblTextureSet, 1);
	m_textureCache
		.LoadTexture(
			"Textures/ibl/sunset_specular.png", //
			vk::Filter::eLinear,
			vk::SamplerAddressMode::eClampToEdge
		)
		->BindToDescriptorSet(m_iblTextureSet, 2);
	m_textureCache
		.LoadTexture(
			"Textures/ibl/sunset_irradiance.png", //
			vk::Filter::eLinear,
			vk::SamplerAddressMode::eClampToEdge
		)
		->BindToDescriptorSet(m_iblTextureSet, 3);
}

void PbrRenderer::CreatePipelines() {
	const vk::PipelineColorBlendAttachmentState NO_BLEND(
		VK_FALSE,
		vk::BlendFactor::eZero,
		vk::BlendFactor::eZero,
		vk::BlendOp::eAdd,
		vk::BlendFactor::eZero,
		vk::BlendFactor::eZero,
		vk::BlendOp::eAdd,
		vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA
	);

	const vk::PipelineColorBlendAttachmentState ALPHA_BLEND(
		VK_TRUE,
		vk::BlendFactor::eSrcAlpha,
		vk::BlendFactor::eOneMinusSrcAlpha,
		vk::BlendOp::eAdd,
		vk::BlendFactor::eOne,
		vk::BlendFactor::eZero,
		vk::BlendOp::eAdd,
		vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA
	);

	ShaderCompiler compiler("shader_includes/");

	m_shadowPipeline = VulkanPipeline(
		m_device,
		compiler,
		{
			m_uniformBufferSet.GetDescriptorSetLayout()
		},
		{
			{vk::ShaderStageFlagBits::eVertex, 0, sizeof(glm::mat4)} //
		},
		VertexBase::GetPipelineVertexInputStateCreateInfo(),
		"pipelines/shadow.json",
		{},
		m_shadowContext.GetRenderPass(),
		0
	);

	m_basePipeline = VulkanPipeline(
		m_device,
		compiler,
		{
			m_uniformBufferSet.GetDescriptorSetLayout(),
			m_pbrMaterialCache.GetDescriptorSetLayout()
		},
		{
			{vk::ShaderStageFlagBits::eVertex, 0, sizeof(glm::mat4)} //
		},
		VertexBase::GetPipelineVertexInputStateCreateInfo(),
		"pipelines/base.json",
		{ NO_BLEND, NO_BLEND, NO_BLEND, NO_BLEND },
		m_deferredContext.GetDeferredRenderPass(),
		0
	);

	m_skyboxPipeline = VulkanPipeline(
		m_device,
		compiler,
		{ m_uniformBufferSet.GetDescriptorSetLayout(), m_iblTextureSetLayout },
		{},
		VertexPositionOnly::GetPipelineVertexInputStateCreateInfo(),
		"pipelines/skybox.json",
		{ NO_BLEND, NO_BLEND, NO_BLEND, NO_BLEND },
		m_deferredContext.GetDeferredRenderPass(),
		0
	);

	m_combinePipeline = VulkanPipeline(
		m_device,
		compiler,
		{ m_uniformBufferSet.GetDescriptorSetLayout(),
		 m_deferredContext.GetDeferredTextureSetLayout(),
		 m_iblTextureSetLayout,
		 m_shadowContext.GetTextureSetLayout() },
		{},
		VertexCanvas::GetPipelineVertexInputStateCreateInfo(),
		"pipelines/combine.json",
		{ NO_BLEND },
		m_deferredContext.GetForwardRenderPass(),
		0
	);

	m_baseForwardPipeline = VulkanPipeline(
		m_device,
		compiler,
		{
			m_uniformBufferSet.GetDescriptorSetLayout(),
			m_pbrMaterialCache.GetDescriptorSetLayout(),
			m_iblTextureSetLayout,
			m_shadowContext.GetTextureSetLayout()
		},
		{
			{vk::ShaderStageFlagBits::eVertex, 0, sizeof(glm::mat4)} //
		},
		VertexBase::GetPipelineVertexInputStateCreateInfo(),
		"pipelines/base_forward.json",
		{ ALPHA_BLEND },
		m_deferredContext.GetForwardRenderPass(),
		0
	);

	m_postProcessingPipeline = VulkanPipeline(
		m_device,
		compiler,
		{ m_deferredContext.GetForwardTextureSetLayout() },
		{},
		VertexCanvas::GetPipelineVertexInputStateCreateInfo(),
		"pipelines/post_processing.json",
		{ NO_BLEND },
		m_toneMappingContext.GetRenderPass(),
		0
	);

	m_presentPipeline = VulkanPipeline(
		m_device,
		compiler,
		{ m_uniformBufferSet.GetDescriptorSetLayout(), m_toneMappingContext.GetTextureSetLayout() },
		{},
		VertexCanvas::GetPipelineVertexInputStateCreateInfo(),
		"pipelines/present.json",
		{ NO_BLEND },
		m_device.GetPrimaryRenderPass(),
		0
	);

	m_screenRectPipeline = VulkanPipeline(
		m_device,
		compiler,
		{
			m_uniformBufferSet.GetDescriptorSetLayout(),
			m_singleTextureMaterialCache.GetDescriptorSetLayout()
		},
		{ {vk::ShaderStageFlagBits::eVertex, 0, SCREEN_RECT_DRAW_CALL_DATA_SIZE} },
		VertexCanvas::GetPipelineVertexInputStateCreateInfo(),
		"pipelines/screen_rect.json",
		{ ALPHA_BLEND },
		m_device.GetPrimaryRenderPass(),
		0
	);

	m_screenLinePipeline = VulkanPipeline(
		m_device,
		compiler,
		{
			m_uniformBufferSet.GetDescriptorSetLayout()
		},
		{
			{vk::ShaderStageFlagBits::eVertex, 0, sizeof(ScreenLineDrawCall)} //
		},
		VertexCanvas::GetPipelineVertexInputStateCreateInfo(),
		"pipelines/screen_line.json",
		{ ALPHA_BLEND },
		m_device.GetPrimaryRenderPass(),
		0
	);
}

void PbrRenderer::CreateSkyboxCube() {
	const std::vector<VertexPositionOnly> vertices = CreateSkyboxVertices();

	m_skyboxCube = VulkanMesh(m_device, vertices.size(), sizeof(VertexPositionOnly), vertices.data());
}

void PbrRenderer::CreateFullScreenQuad() {
	std::vector<VertexCanvas> vertices;
	AppendRectVertices(vertices, { -1.0f, -1.0f }, { 1.0f, 1.0f }, { 0.0f, 0.0f }, { 1.0f, 1.0f });
	m_fullScreenQuad = VulkanMesh(m_device, vertices.size(), sizeof(VertexCanvas), vertices.data());
}

void PbrRenderer::CreateScreenPrimitiveMeshes() {
	static const VertexCanvas rectVertices[4]{
		{{0.0f, 0.0f}, {0.0f, 0.0f}},
		{{1.0f, 0.0f}, {1.0f, 0.0f}},
		{{0.0f, 1.0f}, {0.0f, 1.0f}},
		{{1.0f, 1.0f}, {1.0f, 1.0f}}
	};
	m_screenRectMesh = VulkanMesh(m_device, 4, sizeof(VertexCanvas), rectVertices);

	static const VertexCanvas lineVertices[2]{
		{{0.0f, 0.0f}, {0.0f, 0.0f}},
		{{1.0f, 1.0f}, {1.0f, 1.0f}}
	};
	m_screenLineMesh = VulkanMesh(m_device, 2, sizeof(VertexCanvas), lineVertices);
}

PbrRenderer::~PbrRenderer() {
	m_device.WaitIdle();

	m_screenRectMesh = {};
	m_screenLineMesh = {};
	m_fullScreenQuad = {};
	m_skyboxCube = {};
	m_shadowPipeline = {};
	m_basePipeline = {};
	m_skyboxPipeline = {};
	m_combinePipeline = {};
	m_baseForwardPipeline = {};
	m_postProcessingPipeline = {};
	m_presentPipeline = {};
	m_screenRectPipeline = {};
	m_screenLinePipeline = {};
	m_device.FreeDescriptorSet(m_iblTextureSet);
	m_device.DestroyDescriptorSetLayout(m_iblTextureSetLayout);
	m_uniformBufferSet = {};
	m_deferredContext = {};
	m_shadowContext = {};
}

void PbrRenderer::SetCameraData(const glm::vec3& cameraPosition, const glm::mat4& view, float fov, float near, float far) {
	const vk::Extent2D& extent = m_device.GetSwapchainExtent();
	const float         aspectRatio = static_cast<float>(extent.width) / static_cast<float>(extent.height);

	m_rendererUniformData.Projection = glm::perspective(fov, aspectRatio, near, far);
	m_rendererUniformData.View = view;
	m_rendererUniformData.CameraPosition = cameraPosition;

	m_shadowMatrixCalculator.SetCameraInfo(view, fov, aspectRatio);
}

void PbrRenderer::SetLightingData(const glm::vec3& lightDirection, const glm::vec3& lightColor) {
	m_lightingUniformData.LightDirection = lightDirection;
	m_lightingUniformData.LightColor = lightColor;

	m_shadowMatrixCalculator.SetLightDirection(lightDirection);
}

void PbrRenderer::SetWorldBounds(const glm::vec3& min, const glm::vec3& max) {
	static constexpr float SHADOW_SAFE_DISTANCE = 4.0f;
	m_shadowMatrixCalculator.SetWorldBounds(min - SHADOW_SAFE_DISTANCE, max + SHADOW_SAFE_DISTANCE);
}

void PbrRenderer::FinishDrawing() {
	const VulkanFrameInfo frameInfo = m_device.BeginFrame();

	m_deferredContext.CheckFramebuffersOutOfDate();
	m_toneMappingContext.CheckFramebuffersOutOfDate();

	// update raw screen size
	{
		const vk::Extent2D& size = m_device.GetSwapchainExtent();
		m_rendererUniformData.ScreenInfo.x = static_cast<float>(size.width);
		m_rendererUniformData.ScreenInfo.y = static_cast<float>(size.height);
		m_rendererUniformData.ScreenInfo.z = 1.0f / m_rendererUniformData.ScreenInfo.x;
		m_rendererUniformData.ScreenInfo.w = 1.0f / m_rendererUniformData.ScreenInfo.y;
	}

	// update scaled screen size (for world rendering)
	{
		const vk::Extent2D scaledSize = m_device.GetScaledExtent();
		m_rendererUniformData.ScaledScreenInfo.x = static_cast<float>(scaledSize.width);
		m_rendererUniformData.ScaledScreenInfo.y = static_cast<float>(scaledSize.height);
		m_rendererUniformData.ScaledScreenInfo.z = 1.0f / m_rendererUniformData.ScaledScreenInfo.x;
		m_rendererUniformData.ScaledScreenInfo.w = 1.0f / m_rendererUniformData.ScaledScreenInfo.y;
	}

	// update shadow data
	{
		constexpr float     shadowNear = 0.01f;
		constexpr float     shadowFar = 64.0f;
		constexpr glm::vec3 csmSplits{ 8.0f, 16.0f, 32.0f };
		m_lightingUniformData.CascadeShadowMapSplits = csmSplits;
		m_lightingUniformData.ShadowMatrices[0] = m_shadowMatrixCalculator.CalcShadowMatrix(shadowNear, csmSplits[0]);
		m_lightingUniformData.ShadowMatrices[1] = m_shadowMatrixCalculator.CalcShadowMatrix(csmSplits[0], csmSplits[1]);
		m_lightingUniformData.ShadowMatrices[2] = m_shadowMatrixCalculator.CalcShadowMatrix(csmSplits[1], csmSplits[2]);
		m_lightingUniformData.ShadowMatrices[3] = m_shadowMatrixCalculator.CalcShadowMatrix(csmSplits[2], shadowFar);
	}

	// update point lights
	{
		const int numPointLights = static_cast<int>(m_pointLights.size());
		m_lightingUniformData.NumPointLights = numPointLights;
		memcpy(m_lightingUniformData.PointLights, m_pointLights.data(), numPointLights * sizeof(PointLightData));
		m_pointLights.clear();
	}

	m_uniformBufferSet.UpdateAllBuffers(frameInfo.BufferingIndex, { &m_rendererUniformData, &m_lightingUniformData });

	DrawToShadowMaps(frameInfo.CommandBuffer, frameInfo.BufferingIndex);
	DrawDeferred(frameInfo.CommandBuffer, frameInfo.BufferingIndex);
	DrawForward(frameInfo.CommandBuffer, frameInfo.BufferingIndex);
	PostProcess(frameInfo.CommandBuffer, frameInfo.BufferingIndex);
	DrawToScreen(frameInfo.PrimaryRenderPassBeginInfo, frameInfo.CommandBuffer, frameInfo.BufferingIndex);

	m_device.EndFrame();
}

void PbrRenderer::DrawToShadowMaps(vk::CommandBuffer cmd, uint32_t bufferingIndex) {
	const auto [viewport, scissor] = CalcViewportAndScissorFromExtent(m_shadowContext.GetExtent(), false);

	cmd.beginRenderPass(m_shadowContext.GetRenderPassBeginInfo(bufferingIndex), vk::SubpassContents::eInline);

	cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, m_shadowPipeline.Get());

	cmd.setViewport(0, viewport);
	cmd.setScissor(0, scissor);

	cmd.bindDescriptorSets(
		vk::PipelineBindPoint::eGraphics,
		m_shadowPipeline.GetLayout(),
		0,
		m_uniformBufferSet.GetDescriptorSet(),
		m_uniformBufferSet.GetDynamicOffsets(bufferingIndex)
	);

	for (const DrawCall& drawCall : m_deferredDrawCalls) {
		if (!drawCall.Material->Shadow) {
			continue;
		}

		cmd.pushConstants(
			m_shadowPipeline.GetLayout(), //
			vk::ShaderStageFlagBits::eVertex,
			0,
			sizeof(glm::mat4),
			glm::value_ptr(drawCall.ModelMatrix)
		);
		drawCall.Mesh->BindAndDraw(cmd);
	}

	for (const DrawCall& drawCall : m_forwardDrawCalls) {
		if (!drawCall.Material->Shadow) {
			continue;
		}

		cmd.pushConstants(
			m_shadowPipeline.GetLayout(), //
			vk::ShaderStageFlagBits::eVertex,
			0,
			sizeof(glm::mat4),
			glm::value_ptr(drawCall.ModelMatrix)
		);
		drawCall.Mesh->BindAndDraw(cmd);
	}

	cmd.endRenderPass();
}

void PbrRenderer::DrawDeferred(vk::CommandBuffer cmd, uint32_t bufferingIndex) {
	const auto [viewport, scissor] = CalcViewportAndScissorFromExtent(m_deferredContext.GetExtent());

	cmd.beginRenderPass(m_deferredContext.GetDeferredRenderPassBeginInfo(bufferingIndex), vk::SubpassContents::eInline);

	cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, m_basePipeline.Get());

	cmd.setViewport(0, viewport);
	cmd.setScissor(0, scissor);

	cmd.bindDescriptorSets(
		vk::PipelineBindPoint::eGraphics,
		m_basePipeline.GetLayout(),
		0,
		m_uniformBufferSet.GetDescriptorSet(),
		m_uniformBufferSet.GetDynamicOffsets(bufferingIndex)
	);

	for (const DrawCall& drawCall : m_deferredDrawCalls) {
		cmd.bindDescriptorSets(
			vk::PipelineBindPoint::eGraphics, //
			m_basePipeline.GetLayout(),
			1,
			drawCall.Material->DescriptorSet,
			{}
		);
		cmd.pushConstants(
			m_basePipeline.GetLayout(), //
			vk::ShaderStageFlagBits::eVertex,
			0,
			sizeof(glm::mat4),
			glm::value_ptr(drawCall.ModelMatrix)
		);
		drawCall.Mesh->BindAndDraw(cmd);
	}
	m_deferredDrawCalls.clear();

	cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, m_skyboxPipeline.Get());

	cmd.setViewport(0, viewport);
	cmd.setScissor(0, scissor);

	cmd.bindDescriptorSets(
		vk::PipelineBindPoint::eGraphics,
		m_skyboxPipeline.GetLayout(),
		0,
		{ m_uniformBufferSet.GetDescriptorSet(), m_iblTextureSet },
		m_uniformBufferSet.GetDynamicOffsets(bufferingIndex)
	);
	m_skyboxCube.BindAndDraw(cmd);

	cmd.endRenderPass();
}

void PbrRenderer::DrawForward(vk::CommandBuffer cmd, uint32_t bufferingIndex) {
	const auto [viewport, scissor] = CalcViewportAndScissorFromExtent(m_deferredContext.GetExtent());

	cmd.beginRenderPass(m_deferredContext.GetForwardRenderPassBeginInfo(bufferingIndex), vk::SubpassContents::eInline);

	cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, m_combinePipeline.Get());

	cmd.setViewport(0, viewport);
	cmd.setScissor(0, scissor);

	cmd.bindDescriptorSets(
		vk::PipelineBindPoint::eGraphics,
		m_combinePipeline.GetLayout(),
		0,
		{ m_uniformBufferSet.GetDescriptorSet(),
		 m_deferredContext.GetDeferredTextureSet(bufferingIndex),
		 m_iblTextureSet,
		 m_shadowContext.GetTextureSet(bufferingIndex) },
		m_uniformBufferSet.GetDynamicOffsets(bufferingIndex)
	);

	m_fullScreenQuad.BindAndDraw(cmd);

	cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, m_baseForwardPipeline.Get());

	cmd.setViewport(0, viewport);
	cmd.setScissor(0, scissor);

	cmd.bindDescriptorSets(
		vk::PipelineBindPoint::eGraphics,
		m_baseForwardPipeline.GetLayout(),
		0,
		m_uniformBufferSet.GetDescriptorSet(),
		m_uniformBufferSet.GetDynamicOffsets(bufferingIndex)
	);

	cmd.bindDescriptorSets(
		vk::PipelineBindPoint::eGraphics,
		m_baseForwardPipeline.GetLayout(),
		2,
		{ m_iblTextureSet, m_shadowContext.GetTextureSet(bufferingIndex) },
		{}
	);

	for (const DrawCall& drawCall : m_forwardDrawCalls) {
		cmd.bindDescriptorSets(
			vk::PipelineBindPoint::eGraphics, //
			m_baseForwardPipeline.GetLayout(),
			1,
			drawCall.Material->DescriptorSet,
			{}
		);
		cmd.pushConstants(
			m_baseForwardPipeline.GetLayout(), //
			vk::ShaderStageFlagBits::eVertex,
			0,
			sizeof(glm::mat4),
			glm::value_ptr(drawCall.ModelMatrix)
		);
		drawCall.Mesh->BindAndDraw(cmd);
	}
	m_forwardDrawCalls.clear();

	cmd.endRenderPass();
}

void PbrRenderer::PostProcess(vk::CommandBuffer cmd, uint32_t bufferingIndex) {
	const auto [viewport, scissor] = CalcViewportAndScissorFromExtent(m_toneMappingContext.GetExtent());

	cmd.beginRenderPass(m_toneMappingContext.GetRenderPassBeginInfo(bufferingIndex), vk::SubpassContents::eInline);

	cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, m_postProcessingPipeline.Get());

	cmd.setViewport(0, viewport);
	cmd.setScissor(0, scissor);

	cmd.bindDescriptorSets(
		vk::PipelineBindPoint::eGraphics,
		m_postProcessingPipeline.GetLayout(),
		0,
		{ m_deferredContext.GetForwardTextureSet(bufferingIndex) },
		{}
	);
	m_fullScreenQuad.BindAndDraw(cmd);

	cmd.endRenderPass();
}

void PbrRenderer::DrawToScreen(const vk::RenderPassBeginInfo* primaryRenderPassBeginInfo, vk::CommandBuffer cmd, uint32_t bufferingIndex) {
	const auto [viewport, scissor] = CalcViewportAndScissorFromExtent(m_device.GetSwapchainExtent());

	cmd.beginRenderPass(primaryRenderPassBeginInfo, vk::SubpassContents::eInline);

	{
		cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, m_presentPipeline.Get());

		cmd.setViewport(0, viewport);
		cmd.setScissor(0, scissor);

		cmd.bindDescriptorSets(
			vk::PipelineBindPoint::eGraphics,
			m_presentPipeline.GetLayout(),
			0,
			{ m_uniformBufferSet.GetDescriptorSet(), m_toneMappingContext.GetTextureSet(bufferingIndex) },
			m_uniformBufferSet.GetDynamicOffsets(bufferingIndex)
		);

		m_fullScreenQuad.BindAndDraw(cmd);
	}

	{
		cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, m_screenRectPipeline.Get());

		cmd.setViewport(0, viewport);
		cmd.setScissor(0, scissor);

		cmd.bindDescriptorSets(
			vk::PipelineBindPoint::eGraphics,
			m_screenRectPipeline.GetLayout(),
			0,
			{ m_uniformBufferSet.GetDescriptorSet() },
			m_uniformBufferSet.GetDynamicOffsets(bufferingIndex)
		);

		for (const ScreenRectDrawCall& screenRectDrawCall : m_screenRectDrawCalls) {
			cmd.bindDescriptorSets(
				vk::PipelineBindPoint::eGraphics, //
				m_screenRectPipeline.GetLayout(),
				1,
				screenRectDrawCall.Texture->DescriptorSet,
				{}
			);
			cmd.pushConstants(
				m_screenRectPipeline.GetLayout(), //
				vk::ShaderStageFlagBits::eVertex,
				0,
				SCREEN_RECT_DRAW_CALL_DATA_SIZE,
				&screenRectDrawCall
			);
			m_screenRectMesh.BindAndDraw(cmd);
		}
		m_screenRectDrawCalls.clear();
	}

	{
		cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, m_screenLinePipeline.Get());

		cmd.setViewport(0, viewport);
		cmd.setScissor(0, scissor);

		cmd.bindDescriptorSets(
			vk::PipelineBindPoint::eGraphics,
			m_screenLinePipeline.GetLayout(),
			0,
			{ m_uniformBufferSet.GetDescriptorSet() },
			m_uniformBufferSet.GetDynamicOffsets(bufferingIndex)
		);

		for (const ScreenLineDrawCall& screenLineDrawCall : m_screenLineDrawCalls) {
			cmd.pushConstants(
				m_screenLinePipeline.GetLayout(), //
				vk::ShaderStageFlagBits::eVertex,
				0,
				sizeof(ScreenLineDrawCall),
				&screenLineDrawCall
			);
			m_screenLineMesh.BindAndDraw(cmd);
		}
		m_screenLineDrawCalls.clear();
	}

	cmd.endRenderPass();
}

#pragma endregion

#pragma region BitmapTextRenderer

BitmapTextRenderer::BitmapTextRenderer(PbrRenderer* renderer, const std::string& fontTexture, const glm::vec2& charSize)
	: m_charSize(charSize)
	, m_renderer(renderer)
{
	m_fontTexture = m_renderer->LoadSingleTextureMaterial(fontTexture);
}

static const std::tuple<const glm::vec2, const glm::vec2> CHAR_UVS[256] = {
	{{0.0f, 0.9375f},    {0.0625f, 1.0f}   },
	{{0.0625f, 0.9375f}, {0.125f, 1.0f}    },
	{{0.125f, 0.9375f},  {0.1875f, 1.0f}   },
	{{0.1875f, 0.9375f}, {0.25f, 1.0f}     },
	{{0.25f, 0.9375f},   {0.3125f, 1.0f}   },
	{{0.3125f, 0.9375f}, {0.375f, 1.0f}    },
	{{0.375f, 0.9375f},  {0.4375f, 1.0f}   },
	{{0.4375f, 0.9375f}, {0.5f, 1.0f}      },
	{{0.5f, 0.9375f},    {0.5625f, 1.0f}   },
	{{0.5625f, 0.9375f}, {0.625f, 1.0f}    },
	{{0.625f, 0.9375f},  {0.6875f, 1.0f}   },
	{{0.6875f, 0.9375f}, {0.75f, 1.0f}     },
	{{0.75f, 0.9375f},   {0.8125f, 1.0f}   },
	{{0.8125f, 0.9375f}, {0.875f, 1.0f}    },
	{{0.875f, 0.9375f},  {0.9375f, 1.0f}   },
	{{0.9375f, 0.9375f}, {1.0f, 1.0f}      },
	{{0.0f, 0.875f},     {0.0625f, 0.9375f}},
	{{0.0625f, 0.875f},  {0.125f, 0.9375f} },
	{{0.125f, 0.875f},   {0.1875f, 0.9375f}},
	{{0.1875f, 0.875f},  {0.25f, 0.9375f}  },
	{{0.25f, 0.875f},    {0.3125f, 0.9375f}},
	{{0.3125f, 0.875f},  {0.375f, 0.9375f} },
	{{0.375f, 0.875f},   {0.4375f, 0.9375f}},
	{{0.4375f, 0.875f},  {0.5f, 0.9375f}   },
	{{0.5f, 0.875f},     {0.5625f, 0.9375f}},
	{{0.5625f, 0.875f},  {0.625f, 0.9375f} },
	{{0.625f, 0.875f},   {0.6875f, 0.9375f}},
	{{0.6875f, 0.875f},  {0.75f, 0.9375f}  },
	{{0.75f, 0.875f},    {0.8125f, 0.9375f}},
	{{0.8125f, 0.875f},  {0.875f, 0.9375f} },
	{{0.875f, 0.875f},   {0.9375f, 0.9375f}},
	{{0.9375f, 0.875f},  {1.0f, 0.9375f}   },
	{{0.0f, 0.8125f},    {0.0625f, 0.875f} },
	{{0.0625f, 0.8125f}, {0.125f, 0.875f}  },
	{{0.125f, 0.8125f},  {0.1875f, 0.875f} },
	{{0.1875f, 0.8125f}, {0.25f, 0.875f}   },
	{{0.25f, 0.8125f},   {0.3125f, 0.875f} },
	{{0.3125f, 0.8125f}, {0.375f, 0.875f}  },
	{{0.375f, 0.8125f},  {0.4375f, 0.875f} },
	{{0.4375f, 0.8125f}, {0.5f, 0.875f}    },
	{{0.5f, 0.8125f},    {0.5625f, 0.875f} },
	{{0.5625f, 0.8125f}, {0.625f, 0.875f}  },
	{{0.625f, 0.8125f},  {0.6875f, 0.875f} },
	{{0.6875f, 0.8125f}, {0.75f, 0.875f}   },
	{{0.75f, 0.8125f},   {0.8125f, 0.875f} },
	{{0.8125f, 0.8125f}, {0.875f, 0.875f}  },
	{{0.875f, 0.8125f},  {0.9375f, 0.875f} },
	{{0.9375f, 0.8125f}, {1.0f, 0.875f}    },
	{{0.0f, 0.75f},      {0.0625f, 0.8125f}},
	{{0.0625f, 0.75f},   {0.125f, 0.8125f} },
	{{0.125f, 0.75f},    {0.1875f, 0.8125f}},
	{{0.1875f, 0.75f},   {0.25f, 0.8125f}  },
	{{0.25f, 0.75f},     {0.3125f, 0.8125f}},
	{{0.3125f, 0.75f},   {0.375f, 0.8125f} },
	{{0.375f, 0.75f},    {0.4375f, 0.8125f}},
	{{0.4375f, 0.75f},   {0.5f, 0.8125f}   },
	{{0.5f, 0.75f},      {0.5625f, 0.8125f}},
	{{0.5625f, 0.75f},   {0.625f, 0.8125f} },
	{{0.625f, 0.75f},    {0.6875f, 0.8125f}},
	{{0.6875f, 0.75f},   {0.75f, 0.8125f}  },
	{{0.75f, 0.75f},     {0.8125f, 0.8125f}},
	{{0.8125f, 0.75f},   {0.875f, 0.8125f} },
	{{0.875f, 0.75f},    {0.9375f, 0.8125f}},
	{{0.9375f, 0.75f},   {1.0f, 0.8125f}   },
	{{0.0f, 0.6875f},    {0.0625f, 0.75f}  },
	{{0.0625f, 0.6875f}, {0.125f, 0.75f}   },
	{{0.125f, 0.6875f},  {0.1875f, 0.75f}  },
	{{0.1875f, 0.6875f}, {0.25f, 0.75f}    },
	{{0.25f, 0.6875f},   {0.3125f, 0.75f}  },
	{{0.3125f, 0.6875f}, {0.375f, 0.75f}   },
	{{0.375f, 0.6875f},  {0.4375f, 0.75f}  },
	{{0.4375f, 0.6875f}, {0.5f, 0.75f}     },
	{{0.5f, 0.6875f},    {0.5625f, 0.75f}  },
	{{0.5625f, 0.6875f}, {0.625f, 0.75f}   },
	{{0.625f, 0.6875f},  {0.6875f, 0.75f}  },
	{{0.6875f, 0.6875f}, {0.75f, 0.75f}    },
	{{0.75f, 0.6875f},   {0.8125f, 0.75f}  },
	{{0.8125f, 0.6875f}, {0.875f, 0.75f}   },
	{{0.875f, 0.6875f},  {0.9375f, 0.75f}  },
	{{0.9375f, 0.6875f}, {1.0f, 0.75f}     },
	{{0.0f, 0.625f},     {0.0625f, 0.6875f}},
	{{0.0625f, 0.625f},  {0.125f, 0.6875f} },
	{{0.125f, 0.625f},   {0.1875f, 0.6875f}},
	{{0.1875f, 0.625f},  {0.25f, 0.6875f}  },
	{{0.25f, 0.625f},    {0.3125f, 0.6875f}},
	{{0.3125f, 0.625f},  {0.375f, 0.6875f} },
	{{0.375f, 0.625f},   {0.4375f, 0.6875f}},
	{{0.4375f, 0.625f},  {0.5f, 0.6875f}   },
	{{0.5f, 0.625f},     {0.5625f, 0.6875f}},
	{{0.5625f, 0.625f},  {0.625f, 0.6875f} },
	{{0.625f, 0.625f},   {0.6875f, 0.6875f}},
	{{0.6875f, 0.625f},  {0.75f, 0.6875f}  },
	{{0.75f, 0.625f},    {0.8125f, 0.6875f}},
	{{0.8125f, 0.625f},  {0.875f, 0.6875f} },
	{{0.875f, 0.625f},   {0.9375f, 0.6875f}},
	{{0.9375f, 0.625f},  {1.0f, 0.6875f}   },
	{{0.0f, 0.5625f},    {0.0625f, 0.625f} },
	{{0.0625f, 0.5625f}, {0.125f, 0.625f}  },
	{{0.125f, 0.5625f},  {0.1875f, 0.625f} },
	{{0.1875f, 0.5625f}, {0.25f, 0.625f}   },
	{{0.25f, 0.5625f},   {0.3125f, 0.625f} },
	{{0.3125f, 0.5625f}, {0.375f, 0.625f}  },
	{{0.375f, 0.5625f},  {0.4375f, 0.625f} },
	{{0.4375f, 0.5625f}, {0.5f, 0.625f}    },
	{{0.5f, 0.5625f},    {0.5625f, 0.625f} },
	{{0.5625f, 0.5625f}, {0.625f, 0.625f}  },
	{{0.625f, 0.5625f},  {0.6875f, 0.625f} },
	{{0.6875f, 0.5625f}, {0.75f, 0.625f}   },
	{{0.75f, 0.5625f},   {0.8125f, 0.625f} },
	{{0.8125f, 0.5625f}, {0.875f, 0.625f}  },
	{{0.875f, 0.5625f},  {0.9375f, 0.625f} },
	{{0.9375f, 0.5625f}, {1.0f, 0.625f}    },
	{{0.0f, 0.5f},       {0.0625f, 0.5625f}},
	{{0.0625f, 0.5f},    {0.125f, 0.5625f} },
	{{0.125f, 0.5f},     {0.1875f, 0.5625f}},
	{{0.1875f, 0.5f},    {0.25f, 0.5625f}  },
	{{0.25f, 0.5f},      {0.3125f, 0.5625f}},
	{{0.3125f, 0.5f},    {0.375f, 0.5625f} },
	{{0.375f, 0.5f},     {0.4375f, 0.5625f}},
	{{0.4375f, 0.5f},    {0.5f, 0.5625f}   },
	{{0.5f, 0.5f},       {0.5625f, 0.5625f}},
	{{0.5625f, 0.5f},    {0.625f, 0.5625f} },
	{{0.625f, 0.5f},     {0.6875f, 0.5625f}},
	{{0.6875f, 0.5f},    {0.75f, 0.5625f}  },
	{{0.75f, 0.5f},      {0.8125f, 0.5625f}},
	{{0.8125f, 0.5f},    {0.875f, 0.5625f} },
	{{0.875f, 0.5f},     {0.9375f, 0.5625f}},
	{{0.9375f, 0.5f},    {1.0f, 0.5625f}   },
	{{0.0f, 0.4375f},    {0.0625f, 0.5f}   },
	{{0.0625f, 0.4375f}, {0.125f, 0.5f}    },
	{{0.125f, 0.4375f},  {0.1875f, 0.5f}   },
	{{0.1875f, 0.4375f}, {0.25f, 0.5f}     },
	{{0.25f, 0.4375f},   {0.3125f, 0.5f}   },
	{{0.3125f, 0.4375f}, {0.375f, 0.5f}    },
	{{0.375f, 0.4375f},  {0.4375f, 0.5f}   },
	{{0.4375f, 0.4375f}, {0.5f, 0.5f}      },
	{{0.5f, 0.4375f},    {0.5625f, 0.5f}   },
	{{0.5625f, 0.4375f}, {0.625f, 0.5f}    },
	{{0.625f, 0.4375f},  {0.6875f, 0.5f}   },
	{{0.6875f, 0.4375f}, {0.75f, 0.5f}     },
	{{0.75f, 0.4375f},   {0.8125f, 0.5f}   },
	{{0.8125f, 0.4375f}, {0.875f, 0.5f}    },
	{{0.875f, 0.4375f},  {0.9375f, 0.5f}   },
	{{0.9375f, 0.4375f}, {1.0f, 0.5f}      },
	{{0.0f, 0.375f},     {0.0625f, 0.4375f}},
	{{0.0625f, 0.375f},  {0.125f, 0.4375f} },
	{{0.125f, 0.375f},   {0.1875f, 0.4375f}},
	{{0.1875f, 0.375f},  {0.25f, 0.4375f}  },
	{{0.25f, 0.375f},    {0.3125f, 0.4375f}},
	{{0.3125f, 0.375f},  {0.375f, 0.4375f} },
	{{0.375f, 0.375f},   {0.4375f, 0.4375f}},
	{{0.4375f, 0.375f},  {0.5f, 0.4375f}   },
	{{0.5f, 0.375f},     {0.5625f, 0.4375f}},
	{{0.5625f, 0.375f},  {0.625f, 0.4375f} },
	{{0.625f, 0.375f},   {0.6875f, 0.4375f}},
	{{0.6875f, 0.375f},  {0.75f, 0.4375f}  },
	{{0.75f, 0.375f},    {0.8125f, 0.4375f}},
	{{0.8125f, 0.375f},  {0.875f, 0.4375f} },
	{{0.875f, 0.375f},   {0.9375f, 0.4375f}},
	{{0.9375f, 0.375f},  {1.0f, 0.4375f}   },
	{{0.0f, 0.3125f},    {0.0625f, 0.375f} },
	{{0.0625f, 0.3125f}, {0.125f, 0.375f}  },
	{{0.125f, 0.3125f},  {0.1875f, 0.375f} },
	{{0.1875f, 0.3125f}, {0.25f, 0.375f}   },
	{{0.25f, 0.3125f},   {0.3125f, 0.375f} },
	{{0.3125f, 0.3125f}, {0.375f, 0.375f}  },
	{{0.375f, 0.3125f},  {0.4375f, 0.375f} },
	{{0.4375f, 0.3125f}, {0.5f, 0.375f}    },
	{{0.5f, 0.3125f},    {0.5625f, 0.375f} },
	{{0.5625f, 0.3125f}, {0.625f, 0.375f}  },
	{{0.625f, 0.3125f},  {0.6875f, 0.375f} },
	{{0.6875f, 0.3125f}, {0.75f, 0.375f}   },
	{{0.75f, 0.3125f},   {0.8125f, 0.375f} },
	{{0.8125f, 0.3125f}, {0.875f, 0.375f}  },
	{{0.875f, 0.3125f},  {0.9375f, 0.375f} },
	{{0.9375f, 0.3125f}, {1.0f, 0.375f}    },
	{{0.0f, 0.25f},      {0.0625f, 0.3125f}},
	{{0.0625f, 0.25f},   {0.125f, 0.3125f} },
	{{0.125f, 0.25f},    {0.1875f, 0.3125f}},
	{{0.1875f, 0.25f},   {0.25f, 0.3125f}  },
	{{0.25f, 0.25f},     {0.3125f, 0.3125f}},
	{{0.3125f, 0.25f},   {0.375f, 0.3125f} },
	{{0.375f, 0.25f},    {0.4375f, 0.3125f}},
	{{0.4375f, 0.25f},   {0.5f, 0.3125f}   },
	{{0.5f, 0.25f},      {0.5625f, 0.3125f}},
	{{0.5625f, 0.25f},   {0.625f, 0.3125f} },
	{{0.625f, 0.25f},    {0.6875f, 0.3125f}},
	{{0.6875f, 0.25f},   {0.75f, 0.3125f}  },
	{{0.75f, 0.25f},     {0.8125f, 0.3125f}},
	{{0.8125f, 0.25f},   {0.875f, 0.3125f} },
	{{0.875f, 0.25f},    {0.9375f, 0.3125f}},
	{{0.9375f, 0.25f},   {1.0f, 0.3125f}   },
	{{0.0f, 0.1875f},    {0.0625f, 0.25f}  },
	{{0.0625f, 0.1875f}, {0.125f, 0.25f}   },
	{{0.125f, 0.1875f},  {0.1875f, 0.25f}  },
	{{0.1875f, 0.1875f}, {0.25f, 0.25f}    },
	{{0.25f, 0.1875f},   {0.3125f, 0.25f}  },
	{{0.3125f, 0.1875f}, {0.375f, 0.25f}   },
	{{0.375f, 0.1875f},  {0.4375f, 0.25f}  },
	{{0.4375f, 0.1875f}, {0.5f, 0.25f}     },
	{{0.5f, 0.1875f},    {0.5625f, 0.25f}  },
	{{0.5625f, 0.1875f}, {0.625f, 0.25f}   },
	{{0.625f, 0.1875f},  {0.6875f, 0.25f}  },
	{{0.6875f, 0.1875f}, {0.75f, 0.25f}    },
	{{0.75f, 0.1875f},   {0.8125f, 0.25f}  },
	{{0.8125f, 0.1875f}, {0.875f, 0.25f}   },
	{{0.875f, 0.1875f},  {0.9375f, 0.25f}  },
	{{0.9375f, 0.1875f}, {1.0f, 0.25f}     },
	{{0.0f, 0.125f},     {0.0625f, 0.1875f}},
	{{0.0625f, 0.125f},  {0.125f, 0.1875f} },
	{{0.125f, 0.125f},   {0.1875f, 0.1875f}},
	{{0.1875f, 0.125f},  {0.25f, 0.1875f}  },
	{{0.25f, 0.125f},    {0.3125f, 0.1875f}},
	{{0.3125f, 0.125f},  {0.375f, 0.1875f} },
	{{0.375f, 0.125f},   {0.4375f, 0.1875f}},
	{{0.4375f, 0.125f},  {0.5f, 0.1875f}   },
	{{0.5f, 0.125f},     {0.5625f, 0.1875f}},
	{{0.5625f, 0.125f},  {0.625f, 0.1875f} },
	{{0.625f, 0.125f},   {0.6875f, 0.1875f}},
	{{0.6875f, 0.125f},  {0.75f, 0.1875f}  },
	{{0.75f, 0.125f},    {0.8125f, 0.1875f}},
	{{0.8125f, 0.125f},  {0.875f, 0.1875f} },
	{{0.875f, 0.125f},   {0.9375f, 0.1875f}},
	{{0.9375f, 0.125f},  {1.0f, 0.1875f}   },
	{{0.0f, 0.0625f},    {0.0625f, 0.125f} },
	{{0.0625f, 0.0625f}, {0.125f, 0.125f}  },
	{{0.125f, 0.0625f},  {0.1875f, 0.125f} },
	{{0.1875f, 0.0625f}, {0.25f, 0.125f}   },
	{{0.25f, 0.0625f},   {0.3125f, 0.125f} },
	{{0.3125f, 0.0625f}, {0.375f, 0.125f}  },
	{{0.375f, 0.0625f},  {0.4375f, 0.125f} },
	{{0.4375f, 0.0625f}, {0.5f, 0.125f}    },
	{{0.5f, 0.0625f},    {0.5625f, 0.125f} },
	{{0.5625f, 0.0625f}, {0.625f, 0.125f}  },
	{{0.625f, 0.0625f},  {0.6875f, 0.125f} },
	{{0.6875f, 0.0625f}, {0.75f, 0.125f}   },
	{{0.75f, 0.0625f},   {0.8125f, 0.125f} },
	{{0.8125f, 0.0625f}, {0.875f, 0.125f}  },
	{{0.875f, 0.0625f},  {0.9375f, 0.125f} },
	{{0.9375f, 0.0625f}, {1.0f, 0.125f}    },
	{{0.0f, 0.0f},       {0.0625f, 0.0625f}},
	{{0.0625f, 0.0f},    {0.125f, 0.0625f} },
	{{0.125f, 0.0f},     {0.1875f, 0.0625f}},
	{{0.1875f, 0.0f},    {0.25f, 0.0625f}  },
	{{0.25f, 0.0f},      {0.3125f, 0.0625f}},
	{{0.3125f, 0.0f},    {0.375f, 0.0625f} },
	{{0.375f, 0.0f},     {0.4375f, 0.0625f}},
	{{0.4375f, 0.0f},    {0.5f, 0.0625f}   },
	{{0.5f, 0.0f},       {0.5625f, 0.0625f}},
	{{0.5625f, 0.0f},    {0.625f, 0.0625f} },
	{{0.625f, 0.0f},     {0.6875f, 0.0625f}},
	{{0.6875f, 0.0f},    {0.75f, 0.0625f}  },
	{{0.75f, 0.0f},      {0.8125f, 0.0625f}},
	{{0.8125f, 0.0f},    {0.875f, 0.0625f} },
	{{0.875f, 0.0f},     {0.9375f, 0.0625f}},
	{{0.9375f, 0.0f},    {1.0f, 0.0625f}   },
};

void BitmapTextRenderer::DrawText(const std::string& text, const glm::vec2& position, const glm::vec4& color) {
	glm::vec2 p = position;
	for (char c : text) {
		const auto& [uvMin, uvMax] = CHAR_UVS[c];
		m_renderer->DrawScreenRect(p, p + m_charSize, uvMin, uvMax, color, m_fontTexture);
		p.x += m_charSize.x;
	}
}

#pragma endregion
