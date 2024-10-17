#include "stdafx.h"
#include "GameApp.h"

// TODO:
//https://www.youtube.com/watch?v=kh1zqOVvBVo
// Pangeon

EngineApplicationCreateInfo GameApplication::Config() const
{
	EngineApplicationCreateInfo createInfo{};
	createInfo.render.swapChain.depthFormat = vkr::Format::D32_FLOAT;
	createInfo.render.showImgui = true;

	createInfo.physics.enable = true;
	return createInfo;
}

bool GameApplication::Setup()
{
	auto& device = GetRenderDevice();

	GameGraphicsCreateInfo ggci = {};
	ggci.descriptorPool.maxUniformBuffer = game::NumMaxEntities;
	ggci.descriptorPool.maxSampledImage = game::NumMaxEntities;
	ggci.descriptorPool.maxSampler = game::NumMaxEntities;
	if (!m_gameGraphics.Setup(device, ggci)) return false;

	WorldCreateInfo worldCI = {};
	if (!m_world.Setup(this, worldCI)) return false;

	if (m_cursorVisible) GetInput().SetCursorMode(CursorMode::Disabled);

	return true;
}

void GameApplication::Shutdown()
{
	auto& device = GetRenderDevice();

	m_gameGraphics.Shutdown(device);

	m_world.Shutdown();
	// TODO: очистка
}

void GameApplication::Update()
{
	processInput();
	m_world.Update(GetDeltaTime());
}

void GameApplication::FixedUpdate(float fixedDeltaTime)
{
	m_world.FixedUpdate(fixedDeltaTime);
}

void GameApplication::Render()
{
	m_world.UpdateUniformBuffer();

	auto& render = GetRender();
	auto& swapChain = render.GetSwapChain();
	auto& frame = m_gameGraphics.FrameData(0);

	uint32_t imageIndex = UINT32_MAX;

	// Wait for and reset render complete fence
	CHECKED_CALL(frame.renderCompleteFence->WaitAndReset());
	CHECKED_CALL(swapChain.AcquireNextImage(UINT64_MAX, frame.imageAcquiredSemaphore, frame.imageAcquiredFence, &imageIndex));
	// Wait for and reset image acquired fence
	CHECKED_CALL(frame.imageAcquiredFence->WaitAndReset());

	vkr::RenderPassPtr mainRenderPass = swapChain.GetRenderPass(imageIndex);
	ASSERT_MSG(!mainRenderPass.IsNull(), "render pass object is null");

	// Build command buffer
	CHECKED_CALL(frame.cmd->Begin());
	{
		// render pass
		m_gameGraphics.GetShadowPass().Draw(frame.cmd, m_world.GetEntities());

		// Render main frame
		{
			vkr::RenderPassBeginInfo renderPassBeginInfo = {};
			renderPassBeginInfo.pRenderPass = mainRenderPass;
			renderPassBeginInfo.renderArea = mainRenderPass->GetRenderArea();
			renderPassBeginInfo.RTVClearCount = 1;
			renderPassBeginInfo.RTVClearValues[0] = {0.2f, 0.4f, 1.0f, 0.0f};
			renderPassBeginInfo.DSVClearValue = { 1.0f, 0xFF };

			frame.cmd->TransitionImageLayout(mainRenderPass->GetRenderTargetImage(0), ALL_SUBRESOURCES, vkr::ResourceState::Present, vkr::ResourceState::RenderTarget);
			frame.cmd->BeginRenderPass(&renderPassBeginInfo);
			{
				frame.cmd->SetScissors(render.GetScissor());
				frame.cmd->SetViewports(render.GetViewport());

				// render scene
				m_world.Draw(frame.cmd);

				// Draw ImGui
				//render.DrawDebugInfo();
				{
					// drawCameraInfo
					{
						ImGui::Columns(2);
						ImGui::Text("Camera position");
						ImGui::NextColumn();
						ImGui::Text("(%.4f, %.4f, %.4f)", 
							m_world.GetPlayer().GetTransform().GetTranslation()[0], 
							m_world.GetPlayer().GetTransform().GetTranslation()[1], 
							m_world.GetPlayer().GetTransform().GetTranslation()[2]);
						ImGui::NextColumn();

						ImGui::Columns(2);
						ImGui::Text("Camera looking at");
						ImGui::NextColumn();
						ImGui::Text("(%.4f, %.4f, %.4f)", m_world.GetPlayer().GetTransform().GetForwardVector()[0], m_world.GetPlayer().GetTransform().GetForwardVector()[1], m_world.GetPlayer().GetTransform().GetForwardVector()[2]);
						ImGui::NextColumn();

						ImGui::Separator();

						ImGui::Columns(2);
						ImGui::Text("Person location");
						ImGui::NextColumn();
						ImGui::Text("(%.4f, %.4f, %.4f)", m_world.GetPlayer().GetPosition()[0], m_world.GetPlayer().GetPosition()[1], m_world.GetPlayer().GetPosition()[2]);
						ImGui::NextColumn();

						/*ImGui::Columns(2);
						ImGui::Text("Person looking at");
						ImGui::NextColumn();
						ImGui::Text("(%.4f, %.4f, %.4f)", m_world.GetPlayer().GetLookAt()[0], m_world.GetPlayer().GetLookAt()[1], m_world.GetPlayer().GetLookAt()[2]);
						ImGui::NextColumn();

						ImGui::Columns(2);
						ImGui::Text("Azimuth");
						ImGui::NextColumn();
						ImGui::Text("%.4f", m_world.GetPlayer().GetAzimuth());
						ImGui::NextColumn();

						ImGui::Columns(2);
						ImGui::Text("Altitude");
						ImGui::NextColumn();
						ImGui::Text("%.4f", m_world.GetPlayer().GetAltitude());
						ImGui::NextColumn();*/
					}

					ImGui::Separator();
					ImGui::Checkbox("Use PCF Shadows", &m_gameGraphics.GetShadowPass().UsePCF());
				}
				render.DrawImGui(frame.cmd);
			}
			frame.cmd->EndRenderPass();
			frame.cmd->TransitionImageLayout(mainRenderPass->GetRenderTargetImage(0), ALL_SUBRESOURCES, vkr::ResourceState::RenderTarget, vkr::ResourceState::Present);
		}
	}
	CHECKED_CALL(frame.cmd->End());

	vkr::SubmitInfo submitInfo = frame.SetupSubmitInfo();
	CHECKED_CALL(render.GetGraphicsQueue()->Submit(&submitInfo));
	CHECKED_CALL(swapChain.Present(imageIndex, 1, &frame.renderCompleteSemaphore));
}

void GameApplication::MouseMove(int32_t x, int32_t y, int32_t dx, int32_t dy, MouseButton buttons)
{
	//float2 prevPos = GetRender().GetNormalizedDeviceCoordinates(x - dx, y - dy);
	//float2 curPos = GetRender().GetNormalizedDeviceCoordinates(x, y);
	//float2 deltaPos = prevPos - curPos;
	//float  deltaAzimuth = deltaPos[0] * pi<float>() / 4.0f;
	//float  deltaAltitude = deltaPos[1] * pi<float>() / 2.0f;
	//m_world.GetPlayer().Turn(-deltaAzimuth, deltaAltitude);
}

void GameApplication::KeyDown(KeyCode key)
{
	m_pressedKeys.insert(key);
}

void GameApplication::KeyUp(KeyCode key)
{
	m_pressedKeys.erase(key);
}

void GameApplication::processInput()
{
	if (m_pressedKeys.empty()) return;

	if (m_pressedKeys.count(KEY_ESCAPE))
	{
		m_cursorVisible = !m_cursorVisible;
		if (m_cursorVisible) GetInput().SetCursorMode(CursorMode::Disabled);
		else GetInput().SetCursorMode(CursorMode::Normal);
	}
	//m_world.GetPlayer().ProcessInput(m_pressedKeys);
}