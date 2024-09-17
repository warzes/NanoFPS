#include "015_ImguiAndInput.h"

EngineApplicationCreateInfo Example_015::Config() const
{
	EngineApplicationCreateInfo createInfo{};
	createInfo.render.swapChain.depthFormat = FORMAT_D32_FLOAT;
	createInfo.render.showImgui = true;
	return createInfo;
}

bool Example_015::Setup()
{
	auto& device = GetRenderDevice();

	// Per frame data
	{
		PerFrame frame = {};

		CHECKED_CALL(device.GetGraphicsQueue()->CreateCommandBuffer(&frame.cmd));

		SemaphoreCreateInfo semaCreateInfo = {};
		CHECKED_CALL(device.CreateSemaphore(semaCreateInfo, &frame.imageAcquiredSemaphore));

		FenceCreateInfo fenceCreateInfo = {};
		CHECKED_CALL(device.CreateFence(fenceCreateInfo, &frame.imageAcquiredFence));

		CHECKED_CALL(device.CreateSemaphore(semaCreateInfo, &frame.renderCompleteSemaphore));

		fenceCreateInfo = { true }; // Create signaled
		CHECKED_CALL(device.CreateFence(fenceCreateInfo, &frame.renderCompleteFence));

		mPerFrame.push_back(frame);
	}

	return true;
}

void Example_015::Shutdown()
{
	mPerFrame.clear();
	// TODO: доделать очистку
}

void Example_015::Update()
{
}

void Example_015::Render()
{
	auto& render = GetRender();
	auto& swapChain = render.GetSwapChain();
	PerFrame& frame = mPerFrame[0];

	// Wait for and reset render complete fence
	CHECKED_CALL(frame.renderCompleteFence->WaitAndReset());

	uint32_t imageIndex = UINT32_MAX;
	CHECKED_CALL(swapChain.AcquireNextImage(UINT64_MAX, frame.imageAcquiredSemaphore, frame.imageAcquiredFence, &imageIndex));

	// Wait for and reset image acquired fence
	CHECKED_CALL(frame.imageAcquiredFence->WaitAndReset());
		
	// Build command buffer
	CHECKED_CALL(frame.cmd->Begin());
	{
		RenderPassPtr renderPass = swapChain.GetRenderPass(imageIndex);
		ASSERT_MSG(!renderPass.IsNull(), "render pass object is null");

		RenderPassBeginInfo beginInfo = {};
		beginInfo.pRenderPass = renderPass;
		beginInfo.renderArea = renderPass->GetRenderArea();
		beginInfo.RTVClearCount = 1;
		beginInfo.RTVClearValues[0] = { {0, 0, 0, 0} };

		frame.cmd->TransitionImageLayout(renderPass->GetRenderTargetImage(0), ALL_SUBRESOURCES, RESOURCE_STATE_PRESENT, RESOURCE_STATE_RENDER_TARGET);
		frame.cmd->BeginRenderPass(&beginInfo);
		{
			// Draw ImGui
			render.DrawDebugInfo();
			{
				if (ImGui::Begin("Mouse Info")) {
					ImGui::Columns(2);

					ImGui::Text("Position");
					ImGui::NextColumn();
					ImGui::Text("%d, %d", mMouseX, mMouseY);
					ImGui::NextColumn();

					ImGui::Text("Left Button");
					ImGui::NextColumn();
					ImGui::Text((mMouseButtons == MouseButton::Left) ? "DOWN" : "UP");
					ImGui::NextColumn();

					ImGui::Text("Middle Button");
					ImGui::NextColumn();
					ImGui::Text((mMouseButtons == MouseButton::Middle) ? "DOWN" : "UP");
					ImGui::NextColumn();

					ImGui::Text("Right Button");
					ImGui::NextColumn();
					ImGui::Text((mMouseButtons == MouseButton::Right) ? "DOWN" : "UP");
					ImGui::NextColumn();
				}
				ImGui::End();

				if (ImGui::Begin("Key State")) {
					ImGui::Columns(3);

					ImGui::Text("KEY CODE");
					ImGui::NextColumn();
					ImGui::Text("STATE");
					ImGui::NextColumn();
					ImGui::Text("TIME DOWN");
					ImGui::NextColumn();

					for (uint32_t i = KEY_RANGE_FIRST; i <= KEY_RANGE_LAST; ++i)
					{
						const KeyState& state = mKeyStates[i];

						ImGui::Text("%s", GetKeyCodeString(static_cast<KeyCode>(i)));
						ImGui::NextColumn();
						ImGui::Text(state.down ? "DOWN" : "UP");
						ImGui::NextColumn();
						ImGui::NextColumn();
					}
				}
				ImGui::End();
			}
			render.DrawImGui(frame.cmd);
		}
		frame.cmd->EndRenderPass();
		frame.cmd->TransitionImageLayout(renderPass->GetRenderTargetImage(0), ALL_SUBRESOURCES, RESOURCE_STATE_RENDER_TARGET, RESOURCE_STATE_PRESENT);
	}
	CHECKED_CALL(frame.cmd->End());

	SubmitInfo submitInfo = {};
	submitInfo.commandBufferCount = 1;
	submitInfo.ppCommandBuffers = &frame.cmd;
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.ppWaitSemaphores = &frame.imageAcquiredSemaphore;
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.ppSignalSemaphores = &frame.renderCompleteSemaphore;
	submitInfo.pFence = frame.renderCompleteFence;

	CHECKED_CALL(render.GetGraphicsQueue()->Submit(&submitInfo));

	CHECKED_CALL(swapChain.Present(imageIndex, 1, &frame.renderCompleteSemaphore));
}

void Example_015::WindowIconify(bool iconified)
{
	Print("Window " + std::string(iconified ? "iconified" : "restored"));
}

void Example_015::WindowMaximize(bool maximized)
{
	Print("Window " + std::string(maximized ? "maximized" : "restored"));
}

void Example_015::KeyDown(KeyCode key)
{
	mKeyStates[key].down = true;
	//mKeyStates[key].timeDown = GetElapsedSeconds();
}

void Example_015::KeyUp(KeyCode key)
{
	mKeyStates[key].down = false;
	mKeyStates[key].timeDown = FLT_MAX;
}

void Example_015::MouseMove(int32_t x, int32_t y, int32_t dx, int32_t dy, MouseButton buttons)
{
	mMouseX = x;
	mMouseY = y;
}

void Example_015::MouseDown(int32_t x, int32_t y, MouseButton buttons)
{
	mMouseButtons = buttons;
}

void Example_015::MouseUp(int32_t x, int32_t y, MouseButton buttons)
{
	mMouseButtons = MouseButton::None;
}