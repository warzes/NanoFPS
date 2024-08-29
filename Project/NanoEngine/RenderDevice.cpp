#include "Base.h"
#include "Core.h"
#include "RenderCore.h"
#include "RenderResources.h"
#include "RenderDevice.h"


#pragma region Swapchain

namespace grfx {

	Result Swapchain::Create(const grfx::SwapchainCreateInfo* pCreateInfo)
	{
		ASSERT_NULL_ARG(pCreateInfo->pQueue);
		if (IsNull(pCreateInfo->pQueue)) {
			return ERROR_UNEXPECTED_NULL_ARGUMENT;
		}

		Result ppxres = grfx::DeviceObject<grfx::SwapchainCreateInfo>::Create(pCreateInfo);
		if (Failed(ppxres)) {
			return ppxres;
		}

		// Update the stored create info's image count since the actual
		// number of images might be different (hopefully more) than
		// what was originally requested.
		if (!IsHeadless()) {
			mCreateInfo.imageCount = CountU32(mColorImages);
		}
		if (mCreateInfo.imageCount != pCreateInfo->imageCount) {
			LOG_INFO("Swapchain actual image count is different from what was requested\n"
				<< "   actual    : " << mCreateInfo.imageCount << "\n"
				<< "   requested : " << pCreateInfo->imageCount);
		}

		//
		// NOTE: mCreateInfo will be used from this point on.
		//

		// Create color images if needed. This is only needed if we're creating
		// a headless swapchain.
		if (mColorImages.empty()) {
			for (uint32_t i = 0; i < mCreateInfo.imageCount; ++i) {
				grfx::ImageCreateInfo rtCreateInfo = ImageCreateInfo::RenderTarget2D(mCreateInfo.width, mCreateInfo.height, mCreateInfo.colorFormat);
				rtCreateInfo.ownership = grfx::OWNERSHIP_RESTRICTED;
				rtCreateInfo.RTVClearValue = { 0.0f, 0.0f, 0.0f, 0.0f };
				rtCreateInfo.initialState = grfx::RESOURCE_STATE_PRESENT;
				rtCreateInfo.arrayLayerCount = mCreateInfo.arrayLayerCount;
				rtCreateInfo.usageFlags =
					grfx::IMAGE_USAGE_COLOR_ATTACHMENT |
					grfx::IMAGE_USAGE_TRANSFER_SRC |
					grfx::IMAGE_USAGE_TRANSFER_DST |
					grfx::IMAGE_USAGE_SAMPLED;

				grfx::ImagePtr renderTarget;
				ppxres = GetDevice()->CreateImage(&rtCreateInfo, &renderTarget);
				if (Failed(ppxres)) {
					return ppxres;
				}

				mColorImages.push_back(renderTarget);
			}
		}

		// Create depth images if needed. This is usually needed for both normal swapchains
		// and headless swapchains, but not needed for XR swapchains which create their own
		// depth images.
		//
		ppxres = CreateDepthImages();
		if (Failed(ppxres)) {
			return ppxres;
		}

		ppxres = CreateRenderTargets();
		if (Failed(ppxres)) {
			return ppxres;
		}

		ppxres = CreateRenderPasses();
		if (Failed(ppxres)) {
			return ppxres;
		}

		if (IsHeadless()) {
			// Set mCurrentImageIndex to (imageCount - 1) so that the first
			// AcquireNextImage call acquires the first image at index 0.
			mCurrentImageIndex = mCreateInfo.imageCount - 1;

			// Create command buffers to signal and wait semaphores at
			// AcquireNextImage and Present calls.
			for (uint32_t i = 0; i < mCreateInfo.imageCount; ++i) {
				grfx::CommandBufferPtr commandBuffer = nullptr;
				mCreateInfo.pQueue->CreateCommandBuffer(&commandBuffer, 0, 0);
				mHeadlessCommandBuffers.push_back(commandBuffer);
			}
		}

		LOG_INFO("Swapchain created");
		LOG_INFO("   "
			<< "resolution  : " << mCreateInfo.width << "x" << mCreateInfo.height);
		LOG_INFO("   "
			<< "image count : " << mCreateInfo.imageCount);

		return SUCCESS;
	}

	void Swapchain::Destroy()
	{
		DestroyRenderPasses();

		DestroyRenderTargets();

		DestroyDepthImages();

		DestroyColorImages();

#if defined(BUILD_XR)
		if (mXrColorSwapchain != XR_NULL_HANDLE) {
			xrDestroySwapchain(mXrColorSwapchain);
		}
		if (mXrDepthSwapchain != XR_NULL_HANDLE) {
			xrDestroySwapchain(mXrDepthSwapchain);
		}
#endif

		for (auto& elem : mHeadlessCommandBuffers) {
			if (elem) {
				mCreateInfo.pQueue->DestroyCommandBuffer(elem);
			}
		}
		mHeadlessCommandBuffers.clear();

		grfx::DeviceObject<grfx::SwapchainCreateInfo>::Destroy();
	}

	void Swapchain::DestroyColorImages()
	{
		for (auto& elem : mColorImages) {
			if (elem) {
				GetDevice()->DestroyImage(elem);
			}
		}
		mColorImages.clear();
	}

	Result Swapchain::CreateDepthImages()
	{
		if ((mCreateInfo.depthFormat != grfx::FORMAT_UNDEFINED) && mDepthImages.empty()) {
			for (uint32_t i = 0; i < mCreateInfo.imageCount; ++i) {
				grfx::ImageCreateInfo dpCreateInfo = ImageCreateInfo::DepthStencilTarget(mCreateInfo.width, mCreateInfo.height, mCreateInfo.depthFormat);
				dpCreateInfo.ownership = grfx::OWNERSHIP_RESTRICTED;
				dpCreateInfo.arrayLayerCount = mCreateInfo.arrayLayerCount;
				dpCreateInfo.DSVClearValue = { 1.0f, 0xFF };

				grfx::ImagePtr depthStencilTarget;
				auto           ppxres = GetDevice()->CreateImage(&dpCreateInfo, &depthStencilTarget);
				if (Failed(ppxres)) {
					return ppxres;
				}

				mDepthImages.push_back(depthStencilTarget);
			}
		}

		return SUCCESS;
	}

	void Swapchain::DestroyDepthImages()
	{
		for (auto& elem : mDepthImages) {
			if (elem) {
				GetDevice()->DestroyImage(elem);
			}
		}
		mDepthImages.clear();
	}

	Result Swapchain::CreateRenderTargets()
	{
		uint32_t imageCount = CountU32(mColorImages);
		ASSERT_MSG((imageCount > 0), "No color images found for swapchain renderpasses");
		for (size_t i = 0; i < imageCount; ++i) {
			auto                             imagePtr = mColorImages[i];
			grfx::RenderTargetViewCreateInfo rtvCreateInfo = grfx::RenderTargetViewCreateInfo::GuessFromImage(imagePtr);
			rtvCreateInfo.loadOp = grfx::ATTACHMENT_LOAD_OP_CLEAR;
			rtvCreateInfo.ownership = grfx::OWNERSHIP_RESTRICTED;
			rtvCreateInfo.arrayLayerCount = mCreateInfo.arrayLayerCount;

			grfx::RenderTargetViewPtr rtv;
			Result                    ppxres = GetDevice()->CreateRenderTargetView(&rtvCreateInfo, &rtv);
			if (Failed(ppxres)) {
				ASSERT_MSG(false, "grfx::Swapchain::CreateRenderTargets() for LOAD_OP_CLEAR failed");
				return ppxres;
			}
			mClearRenderTargets.push_back(rtv);

			rtvCreateInfo.loadOp = grfx::ATTACHMENT_LOAD_OP_LOAD;
			ppxres = GetDevice()->CreateRenderTargetView(&rtvCreateInfo, &rtv);
			if (Failed(ppxres)) {
				ASSERT_MSG(false, "grfx::Swapchain::CreateRenderTargets() for LOAD_OP_LOAD failed");
				return ppxres;
			}
			mLoadRenderTargets.push_back(rtv);

			if (!mDepthImages.empty()) {
				auto                             depthImage = mDepthImages[i];
				grfx::DepthStencilViewCreateInfo dsvCreateInfo = grfx::DepthStencilViewCreateInfo::GuessFromImage(depthImage);
				dsvCreateInfo.depthLoadOp = grfx::ATTACHMENT_LOAD_OP_CLEAR;
				dsvCreateInfo.stencilLoadOp = grfx::ATTACHMENT_LOAD_OP_CLEAR;
				dsvCreateInfo.ownership = grfx::OWNERSHIP_RESTRICTED;
				dsvCreateInfo.arrayLayerCount = mCreateInfo.arrayLayerCount;

				grfx::DepthStencilViewPtr clearDsv;
				ppxres = GetDevice()->CreateDepthStencilView(&dsvCreateInfo, &clearDsv);
				if (Failed(ppxres)) {
					ASSERT_MSG(false, "grfx::Swapchain::CreateRenderTargets() for depth stencil view failed");
					return ppxres;
				}

				mClearDepthStencilViews.push_back(clearDsv);

				dsvCreateInfo.depthLoadOp = grfx::ATTACHMENT_LOAD_OP_LOAD;
				dsvCreateInfo.stencilLoadOp = grfx::ATTACHMENT_LOAD_OP_LOAD;
				grfx::DepthStencilViewPtr loadDsv;
				ppxres = GetDevice()->CreateDepthStencilView(&dsvCreateInfo, &loadDsv);
				if (Failed(ppxres)) {
					ASSERT_MSG(false, "grfx::Swapchain::CreateRenderTargets() for depth stencil view failed");
					return ppxres;
				}

				mLoadDepthStencilViews.push_back(loadDsv);
			}
		}

		return SUCCESS;
	}

	Result Swapchain::CreateRenderPasses()
	{
		uint32_t imageCount = CountU32(mColorImages);
		ASSERT_MSG((imageCount > 0), "No color images found for swapchain renderpasses");

		// Create render passes with grfx::ATTACHMENT_LOAD_OP_CLEAR for render target.
		for (size_t i = 0; i < imageCount; ++i) {
			grfx::RenderPassCreateInfo rpCreateInfo = {};
			rpCreateInfo.width = mCreateInfo.width;
			rpCreateInfo.height = mCreateInfo.height;
			rpCreateInfo.renderTargetCount = 1;
			rpCreateInfo.pRenderTargetViews[0] = mClearRenderTargets[i];
			rpCreateInfo.pDepthStencilView = mDepthImages.empty() ? nullptr : mClearDepthStencilViews[i];
			rpCreateInfo.renderTargetClearValues[0] = { {0.0f, 0.0f, 0.0f, 0.0f} };
			rpCreateInfo.depthStencilClearValue = { 1.0f, 0xFF };
			rpCreateInfo.ownership = grfx::OWNERSHIP_RESTRICTED;
			rpCreateInfo.pShadingRatePattern = mCreateInfo.pShadingRatePattern;
			rpCreateInfo.arrayLayerCount = mCreateInfo.arrayLayerCount;
#if defined(BUILD_XR)
			if (mCreateInfo.pXrComponent && mCreateInfo.arrayLayerCount > 1) {
				rpCreateInfo.multiViewState.viewMask = mCreateInfo.pXrComponent->GetDefaultViewMask();
			}
			rpCreateInfo.multiViewState.correlationMask = rpCreateInfo.multiViewState.viewMask;
#endif

			grfx::RenderPassPtr renderPass;
			auto                ppxres = GetDevice()->CreateRenderPass(&rpCreateInfo, &renderPass);
			if (Failed(ppxres)) {
				ASSERT_MSG(false, "grfx::Swapchain::CreateRenderPass(CLEAR) failed");
				return ppxres;
			}

			mClearRenderPasses.push_back(renderPass);
		}

		// Create render passes with grfx::ATTACHMENT_LOAD_OP_LOAD for render target.
		for (size_t i = 0; i < imageCount; ++i) {
			grfx::RenderPassCreateInfo rpCreateInfo = {};
			rpCreateInfo.width = mCreateInfo.width;
			rpCreateInfo.height = mCreateInfo.height;
			rpCreateInfo.renderTargetCount = 1;
			rpCreateInfo.pRenderTargetViews[0] = mLoadRenderTargets[i];
			rpCreateInfo.pDepthStencilView = mDepthImages.empty() ? nullptr : mLoadDepthStencilViews[i];
			rpCreateInfo.renderTargetClearValues[0] = { {0.0f, 0.0f, 0.0f, 0.0f} };
			rpCreateInfo.depthStencilClearValue = { 1.0f, 0xFF };
			rpCreateInfo.ownership = grfx::OWNERSHIP_RESTRICTED;
			rpCreateInfo.pShadingRatePattern = mCreateInfo.pShadingRatePattern;
#if defined(BUILD_XR)
			if (mCreateInfo.pXrComponent && mCreateInfo.arrayLayerCount > 1) {
				rpCreateInfo.multiViewState.viewMask = mCreateInfo.pXrComponent->GetDefaultViewMask();
			}
			rpCreateInfo.multiViewState.correlationMask = rpCreateInfo.multiViewState.viewMask;
#endif

			grfx::RenderPassPtr renderPass;
			auto                ppxres = GetDevice()->CreateRenderPass(&rpCreateInfo, &renderPass);
			if (Failed(ppxres)) {
				ASSERT_MSG(false, "grfx::Swapchain::CreateRenderPass(LOAD) failed");
				return ppxres;
			}

			mLoadRenderPasses.push_back(renderPass);
		}

		return SUCCESS;
	}

	void Swapchain::DestroyRenderTargets()
	{
		for (auto& rtv : mClearRenderTargets) {
			GetDevice()->DestroyRenderTargetView(rtv);
		}
		mClearRenderTargets.clear();
		for (auto& rtv : mLoadRenderTargets) {
			GetDevice()->DestroyRenderTargetView(rtv);
		}
		mLoadRenderTargets.clear();
		for (auto& rtv : mClearDepthStencilViews) {
			GetDevice()->DestroyDepthStencilView(rtv);
		}
		mClearDepthStencilViews.clear();
		for (auto& rtv : mLoadDepthStencilViews) {
			GetDevice()->DestroyDepthStencilView(rtv);
		}
		mLoadDepthStencilViews.clear();
	}

	void Swapchain::DestroyRenderPasses()
	{
		for (auto& elem : mClearRenderPasses) {
			if (elem) {
				GetDevice()->DestroyRenderPass(elem);
			}
		}
		mClearRenderPasses.clear();

		for (auto& elem : mLoadRenderPasses) {
			if (elem) {
				GetDevice()->DestroyRenderPass(elem);
			}
		}
		mLoadRenderPasses.clear();
	}

	bool Swapchain::IsHeadless() const
	{
#if defined(BUILD_XR)
		return mCreateInfo.pXrComponent == nullptr && mCreateInfo.pSurface == nullptr;
#else
		return mCreateInfo.pSurface == nullptr;
#endif
	}

	Result Swapchain::GetColorImage(uint32_t imageIndex, grfx::Image** ppImage) const
	{
		if (!IsIndexInRange(imageIndex, mColorImages)) {
			return ERROR_OUT_OF_RANGE;
		}
		*ppImage = mColorImages[imageIndex];
		return SUCCESS;
	}

	Result Swapchain::GetDepthImage(uint32_t imageIndex, grfx::Image** ppImage) const
	{
		if (!IsIndexInRange(imageIndex, mDepthImages)) {
			return ERROR_OUT_OF_RANGE;
		}
		*ppImage = mDepthImages[imageIndex];
		return SUCCESS;
	}

	Result Swapchain::GetRenderPass(uint32_t imageIndex, grfx::AttachmentLoadOp loadOp, grfx::RenderPass** ppRenderPass) const
	{
		if (!IsIndexInRange(imageIndex, mClearRenderPasses)) {
			return ERROR_OUT_OF_RANGE;
		}
		if (loadOp == grfx::ATTACHMENT_LOAD_OP_CLEAR) {
			*ppRenderPass = mClearRenderPasses[imageIndex];
		}
		else {
			*ppRenderPass = mLoadRenderPasses[imageIndex];
		}
		return SUCCESS;
	}

	Result Swapchain::GetRenderTargetView(uint32_t imageIndex, grfx::AttachmentLoadOp loadOp, grfx::RenderTargetView** ppView) const
	{
		if (!IsIndexInRange(imageIndex, mClearRenderTargets)) {
			return ERROR_OUT_OF_RANGE;
		}
		if (loadOp == grfx::ATTACHMENT_LOAD_OP_CLEAR) {
			*ppView = mClearRenderTargets[imageIndex];
		}
		else {
			*ppView = mLoadRenderTargets[imageIndex];
		}
		return SUCCESS;
	}

	Result Swapchain::GetDepthStencilView(uint32_t imageIndex, grfx::AttachmentLoadOp loadOp, grfx::DepthStencilView** ppView) const
	{
		if (!IsIndexInRange(imageIndex, mClearDepthStencilViews)) {
			return ERROR_OUT_OF_RANGE;
		}
		if (loadOp == grfx::ATTACHMENT_LOAD_OP_CLEAR) {
			*ppView = mClearDepthStencilViews[imageIndex];
		}
		else {
			*ppView = mLoadDepthStencilViews[imageIndex];
		}
		return SUCCESS;
	}

	grfx::ImagePtr Swapchain::GetColorImage(uint32_t imageIndex) const
	{
		grfx::ImagePtr object;
		GetColorImage(imageIndex, &object);
		return object;
	}

	grfx::ImagePtr Swapchain::GetDepthImage(uint32_t imageIndex) const
	{
		grfx::ImagePtr object;
		GetDepthImage(imageIndex, &object);
		return object;
	}

	grfx::RenderPassPtr Swapchain::GetRenderPass(uint32_t imageIndex, grfx::AttachmentLoadOp loadOp) const
	{
		grfx::RenderPassPtr object;
		GetRenderPass(imageIndex, loadOp, &object);
		return object;
	}

	grfx::RenderTargetViewPtr Swapchain::GetRenderTargetView(uint32_t imageIndex, grfx::AttachmentLoadOp loadOp) const
	{
		grfx::RenderTargetViewPtr object;
		GetRenderTargetView(imageIndex, loadOp, &object);
		return object;
	}

	grfx::DepthStencilViewPtr Swapchain::GetDepthStencilView(uint32_t imageIndex, grfx::AttachmentLoadOp loadOp) const
	{
		grfx::DepthStencilViewPtr object;
		GetDepthStencilView(imageIndex, loadOp, &object);
		return object;
	}

	Result Swapchain::AcquireNextImage(
		uint64_t         timeout,    // Nanoseconds
		grfx::Semaphore* pSemaphore, // Wait sempahore
		grfx::Fence* pFence,     // Wait fence
		uint32_t* pImageIndex)
	{
#if defined(BUILD_XR)
		if (mCreateInfo.pXrComponent != nullptr) {
			ASSERT_MSG(mXrColorSwapchain != XR_NULL_HANDLE, "Invalid color xrSwapchain handle!");
			ASSERT_MSG(pSemaphore == nullptr, "Should not use semaphore when XR is enabled!");
			ASSERT_MSG(pFence == nullptr, "Should not use fence when XR is enabled!");
			XrSwapchainImageAcquireInfo acquire_info = { XR_TYPE_SWAPCHAIN_IMAGE_ACQUIRE_INFO };
			CHECK_XR_CALL(xrAcquireSwapchainImage(mXrColorSwapchain, &acquire_info, pImageIndex));

			XrSwapchainImageWaitInfo wait_info = { XR_TYPE_SWAPCHAIN_IMAGE_WAIT_INFO };
			wait_info.timeout = XR_INFINITE_DURATION;
			CHECK_XR_CALL(xrWaitSwapchainImage(mXrColorSwapchain, &wait_info));

			if (mXrDepthSwapchain != XR_NULL_HANDLE) {
				uint32_t                    colorImageIndex = *pImageIndex;
				XrSwapchainImageAcquireInfo acquire_info = { XR_TYPE_SWAPCHAIN_IMAGE_ACQUIRE_INFO };
				CHECK_XR_CALL(xrAcquireSwapchainImage(mXrDepthSwapchain, &acquire_info, pImageIndex));

				XrSwapchainImageWaitInfo wait_info = { XR_TYPE_SWAPCHAIN_IMAGE_WAIT_INFO };
				wait_info.timeout = XR_INFINITE_DURATION;
				CHECK_XR_CALL(xrWaitSwapchainImage(mXrDepthSwapchain, &wait_info));

				ASSERT_MSG(colorImageIndex == *pImageIndex, "Color and depth swapchain image indices are different");
			}
			return SUCCESS;
		}
#endif

		if (IsHeadless()) {
			return AcquireNextImageHeadless(timeout, pSemaphore, pFence, pImageIndex);
		}

		return AcquireNextImageInternal(timeout, pSemaphore, pFence, pImageIndex);
	}

	Result Swapchain::Present(
		uint32_t                      imageIndex,
		uint32_t                      waitSemaphoreCount,
		const grfx::Semaphore* const* ppWaitSemaphores)
	{
		if (IsHeadless()) {
			return PresentHeadless(imageIndex, waitSemaphoreCount, ppWaitSemaphores);
		}

		return PresentInternal(imageIndex, waitSemaphoreCount, ppWaitSemaphores);
	}

	Result Swapchain::AcquireNextImageHeadless(uint64_t timeout, grfx::Semaphore* pSemaphore, grfx::Fence* pFence, uint32_t* pImageIndex)
	{
		*pImageIndex = (mCurrentImageIndex + 1u) % CountU32(mColorImages);
		mCurrentImageIndex = *pImageIndex;

		grfx::CommandBufferPtr commandBuffer = mHeadlessCommandBuffers[mCurrentImageIndex];

		commandBuffer->Begin();
		commandBuffer->End();

		grfx::SubmitInfo sInfo = {};
		sInfo.ppCommandBuffers = &commandBuffer;
		sInfo.commandBufferCount = 1;
		sInfo.pFence = pFence;
		sInfo.ppSignalSemaphores = &pSemaphore;
		sInfo.signalSemaphoreCount = 1;
		mCreateInfo.pQueue->Submit(&sInfo);

		return SUCCESS;
	}

	Result Swapchain::PresentHeadless(uint32_t imageIndex, uint32_t waitSemaphoreCount, const grfx::Semaphore* const* ppWaitSemaphores)
	{
		grfx::CommandBufferPtr commandBuffer = mHeadlessCommandBuffers[mCurrentImageIndex];

		commandBuffer->Begin();
		commandBuffer->End();

		grfx::SubmitInfo sInfo = {};
		sInfo.ppCommandBuffers = &commandBuffer;
		sInfo.commandBufferCount = 1;
		sInfo.ppWaitSemaphores = ppWaitSemaphores;
		sInfo.waitSemaphoreCount = waitSemaphoreCount;
		mCreateInfo.pQueue->Submit(&sInfo);

		return SUCCESS;
	}

} // namespace grfx

#pragma endregion


#pragma region Device

namespace grfx {

	Result Device::Create(const grfx::DeviceCreateInfo* pCreateInfo)
	{
		ASSERT_NULL_ARG(pCreateInfo->pGpu);
		Result ppxres = grfx::InstanceObject<grfx::DeviceCreateInfo>::Create(pCreateInfo);
		if (Failed(ppxres)) {
			return ppxres;
		}
		LOG_INFO("Created device: " << pCreateInfo->pGpu->GetDeviceName());
		return SUCCESS;
	}

	void Device::Destroy()
	{
		// Destroy queues first to clear any pending work
		DestroyAllObjects(mGraphicsQueues);
		DestroyAllObjects(mComputeQueues);
		DestroyAllObjects(mTransferQueues);

		// Destroy helper objects first
		DestroyAllObjects(mDrawPasses);
		DestroyAllObjects(mFullscreenQuads);
		DestroyAllObjects(mTextDraws);
		DestroyAllObjects(mTextures);
		DestroyAllObjects(mTextureFonts);

		// Destroy render passes before images and views
		DestroyAllObjects(mRenderPasses);

		DestroyAllObjects(mBuffers);
		DestroyAllObjects(mCommandBuffers);
		DestroyAllObjects(mCommandPools);
		DestroyAllObjects(mComputePipelines);
		DestroyAllObjects(mDepthStencilViews);
		DestroyAllObjects(mDescriptorSets); // Descriptor sets need to be destroyed before pools
		DestroyAllObjects(mDescriptorPools);
		DestroyAllObjects(mDescriptorSetLayouts);
		DestroyAllObjects(mFences);
		DestroyAllObjects(mImages);
		DestroyAllObjects(mGraphicsPipelines);
		DestroyAllObjects(mPipelineInterfaces);
		DestroyAllObjects(mQuerys);
		DestroyAllObjects(mRenderTargetViews);
		DestroyAllObjects(mSampledImageViews);
		DestroyAllObjects(mSamplers);
		DestroyAllObjects(mSemaphores);
		DestroyAllObjects(mStorageImageViews);
		DestroyAllObjects(mShaderModules);
		DestroyAllObjects(mSwapchains);

		// Destroy Ycbcr Conversions after images and views
		DestroyAllObjects(mSamplerYcbcrConversions);

		grfx::InstanceObject<grfx::DeviceCreateInfo>::Destroy();
		LOG_INFO("Destroyed device: " << mCreateInfo.pGpu->GetDeviceName());
	}

	bool Device::isDebugEnabled() const
	{
		return GetInstance()->IsDebugEnabled();
	}

	grfx::Api Device::GetApi() const
	{
		return GetInstance()->GetApi();
	}

	const char* Device::GetDeviceName() const
	{
		return mCreateInfo.pGpu->GetDeviceName();
	}

	grfx::VendorId Device::GetDeviceVendorId() const
	{
		return mCreateInfo.pGpu->GetDeviceVendorId();
	}

	template <
		typename ObjectT,
		typename CreateInfoT,
		typename ContainerT>
	Result Device::CreateObject(const CreateInfoT* pCreateInfo, ContainerT& container, ObjectT** ppObject)
	{
		// Allocate object
		ObjectT* pObject = nullptr;
		Result   ppxres = AllocateObject(&pObject);
		if (Failed(ppxres)) {
			return ppxres;
		}
		// Set parent
		pObject->SetParent(this);
		// Create internal objects
		ppxres = pObject->Create(pCreateInfo);
		if (Failed(ppxres)) {
			pObject->Destroy();
			delete pObject;
			pObject = nullptr;
			return ppxres;
		}
		// Store
		container.push_back(ObjPtr<ObjectT>(pObject));
		// Assign
		*ppObject = pObject;
		// Success
		return SUCCESS;
	}

	template <
		typename ObjectT,
		typename ContainerT>
	void Device::DestroyObject(ContainerT& container, const ObjectT* pObject)
	{
		// Make sure object is in container
		auto it = std::find_if(
			std::begin(container),
			std::end(container),
			[pObject](const ObjPtr<ObjectT>& elem) -> bool {
				bool res = (elem == pObject);
				return res; });
		if (it == std::end(container)) {
			return;
		}
		// Copy pointer
		ObjPtr<ObjectT> object = *it;
		// Remove object pointer from container
		RemoveElement(object, container);
		// Destroy internal objects
		object->Destroy();
		// Delete allocation
		ObjectT* ptr = object.Get();
		delete ptr;
	}

	template <typename ObjectT>
	void Device::DestroyAllObjects(std::vector<ObjPtr<ObjectT>>& container)
	{
		size_t n = container.size();
		for (size_t i = 0; i < n; ++i) {
			// Get object pointer
			ObjPtr<ObjectT> object = container[i];
			// Destroy internal objects
			object->Destroy();
			// Delete allocation
			ObjectT* ptr = object.Get();
			delete ptr;
		}
		// Clear container
		container.clear();
	}

	Result Device::AllocateObject(grfx::DrawPass** ppObject)
	{
		grfx::DrawPass* pObject = new grfx::DrawPass();
		if (IsNull(pObject)) {
			return ERROR_ALLOCATION_FAILED;
		}
		*ppObject = pObject;
		return SUCCESS;
	}

	Result Device::AllocateObject(grfx::FullscreenQuad** ppObject)
	{
		grfx::FullscreenQuad* pObject = new grfx::FullscreenQuad();
		if (IsNull(pObject)) {
			return ERROR_ALLOCATION_FAILED;
		}
		*ppObject = pObject;
		return SUCCESS;
	}

	Result Device::AllocateObject(grfx::Mesh** ppObject)
	{
		grfx::Mesh* pObject = new grfx::Mesh();
		if (IsNull(pObject)) {
			return ERROR_ALLOCATION_FAILED;
		}
		*ppObject = pObject;
		return SUCCESS;
	}

	Result Device::AllocateObject(grfx::TextDraw** ppObject)
	{
		grfx::TextDraw* pObject = new grfx::TextDraw();
		if (IsNull(pObject)) {
			return ERROR_ALLOCATION_FAILED;
		}
		*ppObject = pObject;
		return SUCCESS;
	}

	Result Device::AllocateObject(grfx::Texture** ppObject)
	{
		grfx::Texture* pObject = new grfx::Texture();
		if (IsNull(pObject)) {
			return ERROR_ALLOCATION_FAILED;
		}
		*ppObject = pObject;
		return SUCCESS;
	}

	Result Device::AllocateObject(grfx::TextureFont** ppObject)
	{
		grfx::TextureFont* pObject = new grfx::TextureFont();
		if (IsNull(pObject)) {
			return ERROR_ALLOCATION_FAILED;
		}
		*ppObject = pObject;
		return SUCCESS;
	}

	Result Device::CreateBuffer(const grfx::BufferCreateInfo* pCreateInfo, grfx::Buffer** ppBuffer)
	{
		ASSERT_NULL_ARG(pCreateInfo);
		ASSERT_NULL_ARG(ppBuffer);
		return CreateObject(pCreateInfo, mBuffers, ppBuffer);
	}

	void Device::DestroyBuffer(const grfx::Buffer* pBuffer)
	{
		ASSERT_NULL_ARG(pBuffer);
		DestroyObject(mBuffers, pBuffer);
	}

	Result Device::CreateCommandPool(const grfx::CommandPoolCreateInfo* pCreateInfo, grfx::CommandPool** ppCommandPool)
	{
		ASSERT_NULL_ARG(pCreateInfo);
		ASSERT_NULL_ARG(ppCommandPool);
		return CreateObject(pCreateInfo, mCommandPools, ppCommandPool);
	}

	void Device::DestroyCommandPool(const grfx::CommandPool* pCommandPool)
	{
		ASSERT_NULL_ARG(pCommandPool);
		DestroyObject(mCommandPools, pCommandPool);
	}

	Result Device::CreateComputePipeline(const grfx::ComputePipelineCreateInfo* pCreateInfo, grfx::ComputePipeline** ppComputePipeline)
	{
		ASSERT_NULL_ARG(pCreateInfo);
		ASSERT_NULL_ARG(ppComputePipeline);
		return CreateObject(pCreateInfo, mComputePipelines, ppComputePipeline);
	}

	void Device::DestroyComputePipeline(const grfx::ComputePipeline* pComputePipeline)
	{
		ASSERT_NULL_ARG(pComputePipeline);
		DestroyObject(mComputePipelines, pComputePipeline);
	}

	Result Device::CreateDepthStencilView(const grfx::DepthStencilViewCreateInfo* pCreateInfo, grfx::DepthStencilView** ppDepthStencilView)
	{
		ASSERT_NULL_ARG(pCreateInfo);
		ASSERT_NULL_ARG(ppDepthStencilView);
		return CreateObject(pCreateInfo, mDepthStencilViews, ppDepthStencilView);
	}

	void Device::DestroyDepthStencilView(const grfx::DepthStencilView* pDepthStencilView)
	{
		ASSERT_NULL_ARG(pDepthStencilView);
		DestroyObject(mDepthStencilViews, pDepthStencilView);
	}

	Result Device::CreateDescriptorPool(const grfx::DescriptorPoolCreateInfo* pCreateInfo, grfx::DescriptorPool** ppDescriptorPool)
	{
		ASSERT_NULL_ARG(pCreateInfo);
		ASSERT_NULL_ARG(ppDescriptorPool);
		return CreateObject(pCreateInfo, mDescriptorPools, ppDescriptorPool);
	}

	void Device::DestroyDescriptorPool(const grfx::DescriptorPool* pDescriptorPool)
	{
		ASSERT_NULL_ARG(pDescriptorPool);
		DestroyObject(mDescriptorPools, pDescriptorPool);
	}

	Result Device::CreateDescriptorSetLayout(const grfx::DescriptorSetLayoutCreateInfo* pCreateInfo, grfx::DescriptorSetLayout** ppDescriptorSetLayout)
	{
		ASSERT_NULL_ARG(pCreateInfo);
		ASSERT_NULL_ARG(ppDescriptorSetLayout);
		return CreateObject(pCreateInfo, mDescriptorSetLayouts, ppDescriptorSetLayout);
	}

	void Device::DestroyDescriptorSetLayout(const grfx::DescriptorSetLayout* pDescriptorSetLayout)
	{
		ASSERT_NULL_ARG(pDescriptorSetLayout);
		DestroyObject(mDescriptorSetLayouts, pDescriptorSetLayout);
	}

	Result Device::CreateDrawPass(const grfx::DrawPassCreateInfo* pCreateInfo, grfx::DrawPass** ppDrawPass)
	{
		ASSERT_NULL_ARG(pCreateInfo);
		ASSERT_NULL_ARG(ppDrawPass);

		grfx::internal::DrawPassCreateInfo createInfo = grfx::internal::DrawPassCreateInfo(*pCreateInfo);

		return CreateObject(&createInfo, mDrawPasses, ppDrawPass);
	}

	Result Device::CreateDrawPass(const grfx::DrawPassCreateInfo2* pCreateInfo, grfx::DrawPass** ppDrawPass)
	{
		ASSERT_NULL_ARG(pCreateInfo);
		ASSERT_NULL_ARG(ppDrawPass);

		grfx::internal::DrawPassCreateInfo createInfo = grfx::internal::DrawPassCreateInfo(*pCreateInfo);

		return CreateObject(&createInfo, mDrawPasses, ppDrawPass);
	}

	Result Device::CreateDrawPass(const grfx::DrawPassCreateInfo3* pCreateInfo, grfx::DrawPass** ppDrawPass)
	{
		ASSERT_NULL_ARG(pCreateInfo);
		ASSERT_NULL_ARG(ppDrawPass);

		grfx::internal::DrawPassCreateInfo createInfo = grfx::internal::DrawPassCreateInfo(*pCreateInfo);

		return CreateObject(&createInfo, mDrawPasses, ppDrawPass);
	}

	void Device::DestroyDrawPass(const grfx::DrawPass* pDrawPass)
	{
		ASSERT_NULL_ARG(pDrawPass);
		DestroyObject(mDrawPasses, pDrawPass);
	}

	Result Device::CreateFence(const grfx::FenceCreateInfo* pCreateInfo, grfx::Fence** ppFence)
	{
		ASSERT_NULL_ARG(pCreateInfo);
		ASSERT_NULL_ARG(ppFence);
		return CreateObject(pCreateInfo, mFences, ppFence);
	}

	void Device::DestroyFence(const grfx::Fence* pFence)
	{
		ASSERT_NULL_ARG(pFence);
		DestroyObject(mFences, pFence);
	}

	Result Device::CreateShadingRatePattern(const grfx::ShadingRatePatternCreateInfo* pCreateInfo, grfx::ShadingRatePattern** ppShadingRatePattern)
	{
		ASSERT_NULL_ARG(pCreateInfo);
		ASSERT_NULL_ARG(ppShadingRatePattern);
		return CreateObject(pCreateInfo, mShadingRatePatterns, ppShadingRatePattern);
	}

	void Device::DestroyShadingRatePattern(const grfx::ShadingRatePattern* pShadingRatePattern)
	{
		ASSERT_NULL_ARG(pShadingRatePattern);
		DestroyObject(mShadingRatePatterns, pShadingRatePattern);
	}

	Result Device::CreateFullscreenQuad(const grfx::FullscreenQuadCreateInfo* pCreateInfo, grfx::FullscreenQuad** ppFullscreenQuad)
	{
		ASSERT_NULL_ARG(pCreateInfo);
		ASSERT_NULL_ARG(ppFullscreenQuad);
		return CreateObject(pCreateInfo, mFullscreenQuads, ppFullscreenQuad);
	}

	void Device::DestroyFullscreenQuad(const grfx::FullscreenQuad* pFullscreenQuad)
	{
		ASSERT_NULL_ARG(pFullscreenQuad);
		DestroyObject(mFullscreenQuads, pFullscreenQuad);
	}

	Result Device::CreateGraphicsPipeline(const grfx::GraphicsPipelineCreateInfo* pCreateInfo, grfx::GraphicsPipeline** ppGraphicsPipeline)
	{
		ASSERT_NULL_ARG(pCreateInfo);
		ASSERT_NULL_ARG(ppGraphicsPipeline);
		return CreateObject(pCreateInfo, mGraphicsPipelines, ppGraphicsPipeline);
	}

	Result Device::CreateGraphicsPipeline(const grfx::GraphicsPipelineCreateInfo2* pCreateInfo, grfx::GraphicsPipeline** ppGraphicsPipeline)
	{
		ASSERT_NULL_ARG(pCreateInfo);
		ASSERT_NULL_ARG(ppGraphicsPipeline);

		grfx::GraphicsPipelineCreateInfo createInfo = {};
		grfx::internal::FillOutGraphicsPipelineCreateInfo(pCreateInfo, &createInfo);

		return CreateObject(&createInfo, mGraphicsPipelines, ppGraphicsPipeline);
	}

	void Device::DestroyGraphicsPipeline(const grfx::GraphicsPipeline* pGraphicsPipeline)
	{
		ASSERT_NULL_ARG(pGraphicsPipeline);
		DestroyObject(mGraphicsPipelines, pGraphicsPipeline);
	}

	Result Device::CreateImage(const grfx::ImageCreateInfo* pCreateInfo, grfx::Image** ppImage)
	{
		ASSERT_NULL_ARG(pCreateInfo);
		ASSERT_NULL_ARG(ppImage);
		return CreateObject(pCreateInfo, mImages, ppImage);
	}

	void Device::DestroyImage(const grfx::Image* pImage)
	{
		ASSERT_NULL_ARG(pImage);
		DestroyObject(mImages, pImage);
	}

	Result Device::CreateMesh(const grfx::MeshCreateInfo* pCreateInfo, grfx::Mesh** ppMesh)
	{
		ASSERT_NULL_ARG(pCreateInfo);
		ASSERT_NULL_ARG(ppMesh);
		return CreateObject(pCreateInfo, mMeshes, ppMesh);
	}

	void Device::DestroyMesh(const grfx::Mesh* pMesh)
	{
		ASSERT_NULL_ARG(pMesh);
		DestroyObject(mMeshes, pMesh);
	}

	Result Device::CreatePipelineInterface(const grfx::PipelineInterfaceCreateInfo* pCreateInfo, grfx::PipelineInterface** ppPipelineInterface)
	{
		ASSERT_NULL_ARG(pCreateInfo);
		ASSERT_NULL_ARG(ppPipelineInterface);
		return CreateObject(pCreateInfo, mPipelineInterfaces, ppPipelineInterface);
	}

	void Device::DestroyPipelineInterface(const grfx::PipelineInterface* pPipelineInterface)
	{
		ASSERT_NULL_ARG(pPipelineInterface);
		DestroyObject(mPipelineInterfaces, pPipelineInterface);
	}

	Result Device::CreateQuery(const grfx::QueryCreateInfo* pCreateInfo, grfx::Query** ppQuery)
	{
		ASSERT_NULL_ARG(pCreateInfo);
		ASSERT_NULL_ARG(ppQuery);
		return CreateObject(pCreateInfo, mQuerys, ppQuery);
	}

	void Device::DestroyQuery(const grfx::Query* pQuery)
	{
		ASSERT_NULL_ARG(pQuery);
		DestroyObject(mQuerys, pQuery);
	}

	Result Device::CreateRenderPass(const grfx::RenderPassCreateInfo* pCreateInfo, grfx::RenderPass** ppRenderPass)
	{
		ASSERT_NULL_ARG(pCreateInfo);
		ASSERT_NULL_ARG(ppRenderPass);

		grfx::internal::RenderPassCreateInfo createInfo = grfx::internal::RenderPassCreateInfo(*pCreateInfo);

		return CreateObject(&createInfo, mRenderPasses, ppRenderPass);
	}

	Result Device::CreateRenderPass(const grfx::RenderPassCreateInfo2* pCreateInfo, grfx::RenderPass** ppRenderPass)
	{
		ASSERT_NULL_ARG(pCreateInfo);
		ASSERT_NULL_ARG(ppRenderPass);

		grfx::internal::RenderPassCreateInfo createInfo = grfx::internal::RenderPassCreateInfo(*pCreateInfo);

		return CreateObject(&createInfo, mRenderPasses, ppRenderPass);
	}

	Result Device::CreateRenderPass(const grfx::RenderPassCreateInfo3* pCreateInfo, grfx::RenderPass** ppRenderPass)
	{
		ASSERT_NULL_ARG(pCreateInfo);
		ASSERT_NULL_ARG(ppRenderPass);

		grfx::internal::RenderPassCreateInfo createInfo = grfx::internal::RenderPassCreateInfo(*pCreateInfo);

		return CreateObject(&createInfo, mRenderPasses, ppRenderPass);
	}

	void Device::DestroyRenderPass(const grfx::RenderPass* pRenderPass)
	{
		ASSERT_NULL_ARG(pRenderPass);
		DestroyObject(mRenderPasses, pRenderPass);
	}

	Result Device::CreateRenderTargetView(const grfx::RenderTargetViewCreateInfo* pCreateInfo, grfx::RenderTargetView** ppRenderTargetView)
	{
		ASSERT_NULL_ARG(pCreateInfo);
		ASSERT_NULL_ARG(ppRenderTargetView);
		return CreateObject(pCreateInfo, mRenderTargetViews, ppRenderTargetView);
	}

	void Device::DestroyRenderTargetView(const grfx::RenderTargetView* pRenderTargetView)
	{
		ASSERT_NULL_ARG(pRenderTargetView);
		DestroyObject(mRenderTargetViews, pRenderTargetView);
	}

	Result Device::CreateSampledImageView(const grfx::SampledImageViewCreateInfo* pCreateInfo, grfx::SampledImageView** ppSampledImageView)
	{
		ASSERT_NULL_ARG(pCreateInfo);
		ASSERT_NULL_ARG(ppSampledImageView);
		return CreateObject(pCreateInfo, mSampledImageViews, ppSampledImageView);
	}

	void Device::DestroySampledImageView(const grfx::SampledImageView* pSampledImageView)
	{
		ASSERT_NULL_ARG(pSampledImageView);
		DestroyObject(mSampledImageViews, pSampledImageView);
	}

	Result Device::CreateSampler(const grfx::SamplerCreateInfo* pCreateInfo, grfx::Sampler** ppSampler)
	{
		ASSERT_NULL_ARG(pCreateInfo);
		ASSERT_NULL_ARG(ppSampler);
		return CreateObject(pCreateInfo, mSamplers, ppSampler);
	}

	void Device::DestroySampler(const grfx::Sampler* pSampler)
	{
		ASSERT_NULL_ARG(pSampler);
		DestroyObject(mSamplers, pSampler);
	}

	Result Device::CreateSamplerYcbcrConversion(const grfx::SamplerYcbcrConversionCreateInfo* pCreateInfo, grfx::SamplerYcbcrConversion** ppConversion)
	{
		ASSERT_NULL_ARG(pCreateInfo);
		ASSERT_NULL_ARG(ppConversion);
		return CreateObject(pCreateInfo, mSamplerYcbcrConversions, ppConversion);
	}

	void Device::DestroySamplerYcbcrConversion(const grfx::SamplerYcbcrConversion* pConversion)
	{
		ASSERT_NULL_ARG(pConversion);
		DestroyObject(mSamplerYcbcrConversions, pConversion);
	}

	Result Device::CreateSemaphore(const grfx::SemaphoreCreateInfo* pCreateInfo, grfx::Semaphore** ppSemaphore)
	{
		ASSERT_NULL_ARG(pCreateInfo);
		ASSERT_NULL_ARG(ppSemaphore);
		return CreateObject(pCreateInfo, mSemaphores, ppSemaphore);
	}

	void Device::DestroySemaphore(const grfx::Semaphore* pSemaphore)
	{
		ASSERT_NULL_ARG(pSemaphore);
		DestroyObject(mSemaphores, pSemaphore);
	}

	Result Device::CreateShaderModule(const grfx::ShaderModuleCreateInfo* pCreateInfo, grfx::ShaderModule** ppShaderModule)
	{
		ASSERT_NULL_ARG(pCreateInfo);
		ASSERT_NULL_ARG(ppShaderModule);
		return CreateObject(pCreateInfo, mShaderModules, ppShaderModule);
	}

	void Device::DestroyShaderModule(const grfx::ShaderModule* pShaderModule)
	{
		ASSERT_NULL_ARG(pShaderModule);
		DestroyObject(mShaderModules, pShaderModule);
	}

	Result Device::CreateStorageImageView(const grfx::StorageImageViewCreateInfo* pCreateInfo, grfx::StorageImageView** ppStorageImageView)
	{
		ASSERT_NULL_ARG(pCreateInfo);
		ASSERT_NULL_ARG(ppStorageImageView);
		return CreateObject(pCreateInfo, mStorageImageViews, ppStorageImageView);
	}

	void Device::DestroyStorageImageView(const grfx::StorageImageView* pStorageImageView)
	{
		ASSERT_NULL_ARG(pStorageImageView);
		DestroyObject(mStorageImageViews, pStorageImageView);
	}

	Result Device::CreateSwapchain(const grfx::SwapchainCreateInfo* pCreateInfo, grfx::Swapchain** ppSwapchain)
	{
		ASSERT_NULL_ARG(pCreateInfo);
		ASSERT_NULL_ARG(ppSwapchain);
		return CreateObject(pCreateInfo, mSwapchains, ppSwapchain);
	}

	void Device::DestroySwapchain(const grfx::Swapchain* pSwapchain)
	{
		ASSERT_NULL_ARG(pSwapchain);
		DestroyObject(mSwapchains, pSwapchain);
	}

	Result Device::CreateTextDraw(const grfx::TextDrawCreateInfo* pCreateInfo, grfx::TextDraw** ppTextDraw)
	{
		ASSERT_NULL_ARG(pCreateInfo);
		ASSERT_NULL_ARG(ppTextDraw);
		return CreateObject(pCreateInfo, mTextDraws, ppTextDraw);
	}

	void Device::DestroyTextDraw(const grfx::TextDraw* pTextDraw)
	{
		ASSERT_NULL_ARG(pTextDraw);
		DestroyObject(mTextDraws, pTextDraw);
	}

	Result Device::CreateTexture(const grfx::TextureCreateInfo* pCreateInfo, grfx::Texture** ppTexture)
	{
		ASSERT_NULL_ARG(pCreateInfo);
		ASSERT_NULL_ARG(ppTexture);
		return CreateObject(pCreateInfo, mTextures, ppTexture);
	}

	void Device::DestroyTexture(const grfx::Texture* pTexture)
	{
		ASSERT_NULL_ARG(pTexture);
		DestroyObject(mTextures, pTexture);
	}

	Result Device::CreateTextureFont(const grfx::TextureFontCreateInfo* pCreateInfo, grfx::TextureFont** ppTextureFont)
	{
		ASSERT_NULL_ARG(pCreateInfo);
		ASSERT_NULL_ARG(ppTextureFont);
		return CreateObject(pCreateInfo, mTextureFonts, ppTextureFont);
	}

	void Device::DestroyTextureFont(const grfx::TextureFont* pTextureFont)
	{
		ASSERT_NULL_ARG(pTextureFont);
		DestroyObject(mTextureFonts, pTextureFont);
	}

	Result Device::AllocateCommandBuffer(
		const grfx::CommandPool* pPool,
		grfx::CommandBuffer** ppCommandBuffer,
		uint32_t                 resourceDescriptorCount,
		uint32_t                 samplerDescriptorCount)
	{
		ASSERT_NULL_ARG(ppCommandBuffer);

		grfx::internal::CommandBufferCreateInfo createInfo = {};
		createInfo.pPool = pPool;
		createInfo.resourceDescriptorCount = resourceDescriptorCount;
		createInfo.samplerDescriptorCount = samplerDescriptorCount;

		return CreateObject(&createInfo, mCommandBuffers, ppCommandBuffer);
	}

	void Device::FreeCommandBuffer(const grfx::CommandBuffer* pCommandBuffer)
	{
		ASSERT_NULL_ARG(pCommandBuffer);
		DestroyObject(mCommandBuffers, pCommandBuffer);
	}

	Result Device::AllocateDescriptorSet(grfx::DescriptorPool* pPool, const grfx::DescriptorSetLayout* pLayout, grfx::DescriptorSet** ppSet)
	{
		ASSERT_NULL_ARG(pPool);
		ASSERT_NULL_ARG(pLayout);
		ASSERT_NULL_ARG(ppSet);

		// Prevent allocation using layouts that are pushable
		if (pLayout->IsPushable()) {
			return ERROR_GRFX_OPERATION_NOT_PERMITTED;
		}

		grfx::internal::DescriptorSetCreateInfo createInfo = {};
		createInfo.pPool = pPool;
		createInfo.pLayout = pLayout;

		return CreateObject(&createInfo, mDescriptorSets, ppSet);
	}

	void Device::FreeDescriptorSet(const grfx::DescriptorSet* pSet)
	{
		ASSERT_NULL_ARG(pSet);
		DestroyObject(mDescriptorSets, pSet);
	}

	Result Device::CreateGraphicsQueue(const grfx::internal::QueueCreateInfo* pCreateInfo, grfx::Queue** ppQueue)
	{
		ASSERT_NULL_ARG(pCreateInfo);
		ASSERT_NULL_ARG(ppQueue);
		return CreateObject(pCreateInfo, mGraphicsQueues, ppQueue);
	}

	Result Device::CreateComputeQueue(const grfx::internal::QueueCreateInfo* pCreateInfo, grfx::Queue** ppQueue)
	{
		ASSERT_NULL_ARG(pCreateInfo);
		ASSERT_NULL_ARG(ppQueue);
		return CreateObject(pCreateInfo, mComputeQueues, ppQueue);
	}

	Result Device::CreateTransferQueue(const grfx::internal::QueueCreateInfo* pCreateInfo, grfx::Queue** ppQueue)
	{
		ASSERT_NULL_ARG(pCreateInfo);
		ASSERT_NULL_ARG(ppQueue);
		return CreateObject(pCreateInfo, mTransferQueues, ppQueue);
	}

	uint32_t Device::GetGraphicsQueueCount() const
	{
		uint32_t count = CountU32(mGraphicsQueues);
		return count;
	}

	Result Device::GetGraphicsQueue(uint32_t index, grfx::Queue** ppQueue) const
	{
		if (!IsIndexInRange(index, mGraphicsQueues)) {
			return ERROR_OUT_OF_RANGE;
		}
		*ppQueue = mGraphicsQueues[index].Get();
		return SUCCESS;
	}

	grfx::QueuePtr Device::GetGraphicsQueue(uint32_t index) const
	{
		grfx::QueuePtr queue;
		Result         ppxres = GetGraphicsQueue(index, &queue);
		if (Failed(ppxres)) {
			// @TODO: something?
		}
		return queue;
	}

	uint32_t Device::GetComputeQueueCount() const
	{
		uint32_t count = CountU32(mComputeQueues);
		return count;
	}

	Result Device::GetComputeQueue(uint32_t index, grfx::Queue** ppQueue) const
	{
		if (!IsIndexInRange(index, mComputeQueues)) {
			return ERROR_OUT_OF_RANGE;
		}
		*ppQueue = mComputeQueues[index].Get();
		return SUCCESS;
	}

	grfx::QueuePtr Device::GetComputeQueue(uint32_t index) const
	{
		grfx::QueuePtr queue;
		Result         ppxres = GetComputeQueue(index, &queue);
		if (Failed(ppxres)) {
			// @TODO: something?
		}
		return queue;
	}

	uint32_t Device::GetTransferQueueCount() const
	{
		uint32_t count = CountU32(mTransferQueues);
		return count;
	}

	Result Device::GetTransferQueue(uint32_t index, grfx::Queue** ppQueue) const
	{
		if (!IsIndexInRange(index, mTransferQueues)) {
			return ERROR_OUT_OF_RANGE;
		}
		*ppQueue = mTransferQueues[index].Get();
		return SUCCESS;
	}

	grfx::QueuePtr Device::GetTransferQueue(uint32_t index) const
	{
		grfx::QueuePtr queue;
		Result         ppxres = GetTransferQueue(index, &queue);
		if (Failed(ppxres)) {
			// @TODO: something?
		}
		return queue;
	}

	grfx::QueuePtr Device::GetAnyAvailableQueue() const
	{
		grfx::QueuePtr queue;

		uint32_t count = GetGraphicsQueueCount();
		if (count > 0) {
			for (uint32_t i = 0; i < count; ++i) {
				queue = GetGraphicsQueue(i);
				if (queue) {
					break;
				}
			}
		}

		if (!queue) {
			count = GetComputeQueueCount();
			if (count > 0) {
				for (uint32_t i = 0; i < count; ++i) {
					queue = GetComputeQueue(i);
					if (queue) {
						break;
					}
				}
			}
		}

		if (!queue) {
			count = GetTransferQueueCount();
			if (count > 0) {
				for (uint32_t i = 0; i < count; ++i) {
					queue = GetTransferQueue(i);
					if (queue) {
						break;
					}
				}
			}
		}

		return queue;
	}

} // namespace grfx

#pragma endregion


#pragma region Instance

#pragma endregion
