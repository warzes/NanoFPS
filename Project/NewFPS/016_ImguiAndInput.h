#pragma once

// TODO: на этом примере починить имгуи

class Example_016 final : public EngineApplication
{
public:
	EngineApplicationCreateInfo Config() const final;
	bool Setup() final;
	void Shutdown() final;
	void Update() final;
	void Render() final;

	void WindowIconify(bool iconified) final;
	void WindowMaximize(bool maximized) final;
	void KeyDown(KeyCode key) final;
	void KeyUp(KeyCode key) final;
	void MouseMove(int32_t x, int32_t y, int32_t dx, int32_t dy, MouseButton buttons) final;
	void MouseDown(int32_t x, int32_t y, MouseButton buttons) final;
	void MouseUp(int32_t x, int32_t y, MouseButton buttons) final;

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
	int32_t               mMouseX = 0;
	int32_t               mMouseY = 0;
	MouseButton           mMouseButtons{ MouseButton::None };
	KeyState              mKeyStates[TOTAL_KEY_COUNT] = { 0 };
};