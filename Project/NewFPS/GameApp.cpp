#include "stdafx.h"
#include "GameApp.h"

// TODO:
// сделать таймер ScopedTimer
//https://www.youtube.com/watch?v=kh1zqOVvBVo
// Pangeon

EngineApplicationCreateInfo GameApplication::Config() const
{
	EngineApplicationCreateInfo createInfo{};
	createInfo.render.swapChain.depthFormat = vkr::Format::D32_FLOAT;
	createInfo.render.showImgui = true;
	return createInfo;
}

bool GameApplication::Setup()
{
	auto& device = GetRenderDevice();

	GameGraphicsCreateInfo ggci = {};
	ggci.descriptorPool.maxUniformBuffer = game::NumMaxEntities;
	ggci.descriptorPool.maxSampledImage = game::NumMaxEntities;
	ggci.descriptorPool.maxSampler = game::NumMaxEntities;

	if (!m_gameGraphics.CreateDescriptorPool(device, ggci)) return false;
	if (!setupDescriptors()) return false;
	if (!m_gameGraphics.GetShadowPass().CreateDescriptorSetLayout(device)) return false;
	if (!setupEntities()) return false;
	if (!setupPipelines()) return false;
	if (!m_gameGraphics.CreateShadowPass(device, ggci, mEntities)) return false;
	if (!m_gameGraphics.CreateFrameData(device, ggci)) return false;


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

void GameApplication::Render()
{
	updateUniformBuffer();

	auto& render = GetRender();
	auto& swapChain = render.GetSwapChain();
	auto& frame = m_gameGraphics.FrameData(0);
	uint32_t imageIndex = frame.Frame(swapChain);
	vkr::RenderPassPtr renderPass = swapChain.GetRenderPass(imageIndex);
	ASSERT_MSG(!renderPass.IsNull(), "render pass object is null");

	// Build command buffer
	CHECKED_CALL(frame.cmd->Begin());
	{
		m_gameGraphics.GetShadowPass().Draw(frame.cmd, mEntities);

		//  Render scene
		{
			vkr::RenderPassBeginInfo renderPassBeginInfo = {};
			renderPassBeginInfo.pRenderPass = renderPass;
			renderPassBeginInfo.renderArea = renderPass->GetRenderArea();
			renderPassBeginInfo.RTVClearCount = 1;
			renderPassBeginInfo.RTVClearValues[0] = { {0.2f, 0.4f, 1.0f, 0.0f} };
			renderPassBeginInfo.DSVClearValue = { 1.0f, 0xFF };

			frame.cmd->TransitionImageLayout(renderPass->GetRenderTargetImage(0), ALL_SUBRESOURCES, vkr::ResourceState::Present, vkr::ResourceState::RenderTarget);
			frame.cmd->BeginRenderPass(&renderPassBeginInfo);
			{
				frame.cmd->SetScissors(render.GetScissor());
				frame.cmd->SetViewports(render.GetViewport());

				// Draw entities
				frame.cmd->BindGraphicsPipeline(mDrawObjectPipeline);
				for (size_t i = 0; i < mEntities.size(); ++i)
				{
					GameEntity* pEntity = mEntities[i];

					frame.cmd->BindGraphicsDescriptorSets(mDrawObjectPipelineInterface, 1, &pEntity->drawDescriptorSet);
					frame.cmd->BindIndexBuffer(pEntity->mesh);
					frame.cmd->BindVertexBuffers(pEntity->mesh);
					frame.cmd->DrawIndexed(pEntity->mesh->GetIndexCount());
				}

				// Draw light
				m_world.GetMainLight().DrawDebug(frame.cmd);


				// Draw ImGui
				//render.DrawDebugInfo();
				{
					// drawCameraInfo
					{
						ImGui::Columns(2);
						ImGui::Text("Camera position");
						ImGui::NextColumn();
						ImGui::Text("(%.4f, %.4f, %.4f)", m_world.GetPlayer().GetCamera().GetEyePosition()[0], m_world.GetPlayer().GetCamera().GetEyePosition()[1], m_world.GetPlayer().GetCamera().GetEyePosition()[2]);
						ImGui::NextColumn();

						ImGui::Columns(2);
						ImGui::Text("Camera looking at");
						ImGui::NextColumn();
						ImGui::Text("(%.4f, %.4f, %.4f)", m_world.GetPlayer().GetCamera().GetTarget()[0], m_world.GetPlayer().GetCamera().GetTarget()[1], m_world.GetPlayer().GetCamera().GetTarget()[2]);
						ImGui::NextColumn();

						ImGui::Separator();

						ImGui::Columns(2);
						ImGui::Text("Person location");
						ImGui::NextColumn();
						ImGui::Text("(%.4f, %.4f, %.4f)", m_world.GetPlayer().GetPosition()[0], m_world.GetPlayer().GetPosition()[1], m_world.GetPlayer().GetPosition()[2]);
						ImGui::NextColumn();

						ImGui::Columns(2);
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
						ImGui::NextColumn();
					}

					ImGui::Separator();
					ImGui::Checkbox("Use PCF Shadows", &m_gameGraphics.GetShadowPass().UsePCF());
				}
				render.DrawImGui(frame.cmd);
			}
			frame.cmd->EndRenderPass();
			frame.cmd->TransitionImageLayout(renderPass->GetRenderTargetImage(0), ALL_SUBRESOURCES, vkr::ResourceState::RenderTarget, vkr::ResourceState::Present);
		}
	}
	CHECKED_CALL(frame.cmd->End());

	vkr::SubmitInfo submitInfo = frame.SetupSubmitInfo();
	CHECKED_CALL(render.GetGraphicsQueue()->Submit(&submitInfo));
	CHECKED_CALL(swapChain.Present(imageIndex, 1, &frame.renderCompleteSemaphore));
}

void GameApplication::MouseMove(int32_t x, int32_t y, int32_t dx, int32_t dy, MouseButton buttons)
{
	float2 prevPos = GetRender().GetNormalizedDeviceCoordinates(x - dx, y - dy);
	float2 curPos = GetRender().GetNormalizedDeviceCoordinates(x, y);
	float2 deltaPos = prevPos - curPos;
	float  deltaAzimuth = deltaPos[0] * pi<float>() / 4.0f;
	float  deltaAltitude = deltaPos[1] * pi<float>() / 2.0f;
	m_world.GetPlayer().Turn(-deltaAzimuth, deltaAltitude);
}

void GameApplication::KeyDown(KeyCode key)
{
	m_pressedKeys.insert(key);
}

void GameApplication::KeyUp(KeyCode key)
{
	m_pressedKeys.erase(key);
}

bool GameApplication::setupDescriptors()
{
	auto& device = GetRenderDevice();

	// Descriptor set layouts
	{
		// Draw objects
		vkr::DescriptorSetLayoutCreateInfo layoutCreateInfo = {};
		layoutCreateInfo.bindings.push_back(vkr::DescriptorBinding{ 0, vkr::DescriptorType::UniformBuffer, 1, vkr::SHADER_STAGE_ALL_GRAPHICS });
		layoutCreateInfo.bindings.push_back(vkr::DescriptorBinding{ 1, vkr::DescriptorType::SampledImage, 1, vkr::SHADER_STAGE_PS });
		layoutCreateInfo.bindings.push_back(vkr::DescriptorBinding{ 2, vkr::DescriptorType::Sampler, 1, vkr::SHADER_STAGE_PS });
		CHECKED_CALL_AND_RETURN_FALSE(device.CreateDescriptorSetLayout(layoutCreateInfo, &m_drawObjectSetLayout));
	}

	return true;
}

bool GameApplication::setupEntities()
{
	// Setup entities
	{
		vkr::TriMeshOptions options = vkr::TriMeshOptions()
			.Indices()
			.VertexColors()
			.Normals();
		vkr::TriMesh mesh = vkr::TriMesh::CreatePlane(vkr::TRI_MESH_PLANE_POSITIVE_Y, float2(50, 50), 1, 1, vkr::TriMeshOptions(options).ObjectColor(float3(0.7f)));
		mGroundPlane.Setup(GetRenderDevice(), mesh, GetDescriptorPool(), m_drawObjectSetLayout, m_gameGraphics.GetShadowPass().GetDescriptorSetLayout());
		mEntities.push_back(&mGroundPlane);

		mesh = vkr::TriMesh::CreateCube(float3(2, 2, 2), vkr::TriMeshOptions(options).ObjectColor(float3(0.5f, 0.5f, 0.7f)));
		mCube.Setup(GetRenderDevice(), mesh, GetDescriptorPool(), m_drawObjectSetLayout, m_gameGraphics.GetShadowPass().GetDescriptorSetLayout());
		mCube.translate = float3(-2, 1, 0);
		mEntities.push_back(&mCube);

		mesh = vkr::TriMesh::CreateFromOBJ("basic/models/material_sphere.obj", vkr::TriMeshOptions(options).ObjectColor(float3(0.7f, 0.2f, 0.2f)));
		mKnob.Setup(GetRenderDevice(), mesh, GetDescriptorPool(), m_drawObjectSetLayout, m_gameGraphics.GetShadowPass().GetDescriptorSetLayout());
		mKnob.translate = float3(2, 1, 0);
		mKnob.rotate = float3(0, glm::radians(180.0f), 0);
		mKnob.scale = float3(2, 2, 2);
		mEntities.push_back(&mKnob);
	}

	return true;
}

bool GameApplication::setupPipelines()
{
	auto& device = GetRenderDevice();

	// Draw object pipeline interface and pipeline
	{
		// Pipeline interface
		vkr::PipelineInterfaceCreateInfo piCreateInfo = {};
		piCreateInfo.setCount = 1;
		piCreateInfo.sets[0].set = 0;
		piCreateInfo.sets[0].layout = m_drawObjectSetLayout;
		CHECKED_CALL_AND_RETURN_FALSE(device.CreatePipelineInterface(piCreateInfo, &mDrawObjectPipelineInterface));

		// Pipeline
		vkr::ShaderModulePtr VS;
		CHECKED_CALL_AND_RETURN_FALSE(device.CreateShader("GameData/Shaders", "DiffuseShadow.vs", &VS));
		vkr::ShaderModulePtr PS;
		CHECKED_CALL_AND_RETURN_FALSE(device.CreateShader("GameData/Shaders", "DiffuseShadow.ps", &PS));

		vkr::GraphicsPipelineCreateInfo2 gpCreateInfo = {};
		gpCreateInfo.VS = { VS.Get(), "vsmain" };
		gpCreateInfo.PS = { PS.Get(), "psmain" };
		gpCreateInfo.vertexInputState.bindingCount = 3;
		gpCreateInfo.vertexInputState.bindings[0] = mGroundPlane.mesh->GetDerivedVertexBindings()[0];
		gpCreateInfo.vertexInputState.bindings[1] = mGroundPlane.mesh->GetDerivedVertexBindings()[1];
		gpCreateInfo.vertexInputState.bindings[2] = mGroundPlane.mesh->GetDerivedVertexBindings()[2];
		gpCreateInfo.topology = vkr::PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		gpCreateInfo.polygonMode = vkr::POLYGON_MODE_FILL;
		gpCreateInfo.cullMode = vkr::CULL_MODE_BACK;
		gpCreateInfo.frontFace = vkr::FRONT_FACE_CCW;
		gpCreateInfo.depthReadEnable = true;
		gpCreateInfo.depthWriteEnable = true;
		gpCreateInfo.blendModes[0] = vkr::BLEND_MODE_NONE;
		gpCreateInfo.outputState.renderTargetCount = 1;
		gpCreateInfo.outputState.renderTargetFormats[0] = GetRender().GetSwapChain().GetColorFormat();
		gpCreateInfo.outputState.depthStencilFormat = GetRender().GetSwapChain().GetDepthFormat();
		gpCreateInfo.pipelineInterface = mDrawObjectPipelineInterface;

		CHECKED_CALL_AND_RETURN_FALSE(device.CreateGraphicsPipeline(gpCreateInfo, &mDrawObjectPipeline));
		device.DestroyShaderModule(VS);
		device.DestroyShaderModule(PS);
	}

	return true;
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
	m_world.GetPlayer().ProcessInput(m_pressedKeys);
}

void GameApplication::updateUniformBuffer()
{
	// Update uniform buffers
	for (size_t i = 0; i < mEntities.size(); ++i)
	{
		GameEntity* pEntity = mEntities[i];

		float4x4 T = glm::translate(pEntity->translate);
		float4x4 R = 
			glm::rotate(pEntity->rotate.z, float3(0, 0, 1)) *
			glm::rotate(pEntity->rotate.y, float3(0, 1, 0)) *
			glm::rotate(pEntity->rotate.x, float3(1, 0, 0));
		float4x4 S = glm::scale(pEntity->scale);
		float4x4 M = T * R * S;

		// Draw uniform buffers
		struct Scene
		{
			float4x4 ModelMatrix;                // Transforms object space to world space
			float4x4 NormalMatrix;               // Transforms object space to normal space
			float4   Ambient;                    // Object's ambient intensity
			float4x4 CameraViewProjectionMatrix; // Camera's view projection matrix
			float4   LightPosition;              // Light's position
			float4x4 LightViewProjectionMatrix;  // Light's view projection matrix
			uint4    UsePCF;                     // Enable/disable PCF
		};

		Scene scene = {};
		scene.ModelMatrix = M;
		scene.NormalMatrix = glm::inverseTranspose(M);
		scene.Ambient = float4(0.3f);
		scene.CameraViewProjectionMatrix = m_world.GetPlayer().GetCamera().GetViewProjectionMatrix();
		scene.LightPosition = float4(m_world.GetMainLight().GetPosition(), 0);
		scene.LightViewProjectionMatrix = m_world.GetMainLight().GetCamera().GetViewProjectionMatrix();
		scene.UsePCF = uint4(m_gameGraphics.GetShadowPass().UsePCF());

		pEntity->drawUniformBuffer->CopyFromSource(sizeof(scene), &scene);

		// Shadow uniform buffers
		float4x4 PV = m_world.GetMainLight().GetCamera().GetViewProjectionMatrix();
		float4x4 MVP = PV * M; // Yes - the other is reversed

		pEntity->shadowUniformBuffer->CopyFromSource(sizeof(MVP), &MVP);
	}

	// Update light uniform buffer
	{
		float4x4        T = glm::translate(m_world.GetMainLight().GetPosition());
		const float4x4& PV = m_world.GetPlayer().GetCamera().GetViewProjectionMatrix();
		float4x4        MVP = PV * T; // Yes - the other is reversed
		m_world.GetMainLight().UpdateShaderUniform(sizeof(MVP), &MVP);
	}
}