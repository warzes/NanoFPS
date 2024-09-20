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
		CommandBufferPtr cmd;
		SemaphorePtr     imageAcquiredSemaphore;
		FencePtr         imageAcquiredFence;
		SemaphorePtr     renderCompleteSemaphore;
		FencePtr         renderCompleteFence;
	};

	std::vector<PerFrame> mPerFrame;
	TextureFontPtr        mRoboto;
	TextDrawPtr           mStaticText;
	TextDrawPtr           mDynamicText;
	PerspCamera           mCamera;
};