#pragma once

class Example_026 final : public EngineApplication
{
public:
	EngineApplicationCreateInfo Config() const final;
	bool Setup() final;
	void Shutdown() final;
	void Update() final;
	void Render() final;

private:
	struct PerFrame
	{
		vkr::CommandBufferPtr cmd;
		vkr::SemaphorePtr     imageAcquiredSemaphore;
		vkr::FencePtr         imageAcquiredFence;
		vkr::SemaphorePtr     renderCompleteSemaphore;
		vkr::FencePtr         renderCompleteFence;
	};

	std::vector<PerFrame> mPerFrame;
	vkr::TextureFontPtr        mRoboto;
	vkr::TextDrawPtr           mStaticText;
	vkr::TextDrawPtr           mDynamicText;
	PerspCamera           mCamera;
};