#include "stdafx.h"
#include "World.h"
#include "GameApp.h"

bool World::Setup(GameApplication* game, const WorldCreateInfo& createInfo)
{
	m_game = game;
	if (!m_player.Setup(m_game, createInfo.player)) return false;
	if (!m_mainLight.Setup(m_game)) return false;

	if (!setupDescriptorSetLayout()) return false;
	if (!addTestEntities()) return false;
	if (!setupEntities()) return false;

	if (!m_mapData.Setup(createInfo.startMapName)) return false;

	return true;
}

void World::Shutdown()
{
	m_mapData.Shutdown();
	m_mainLight.Shutdown();
	m_player.Shutdown();
}

void World::Update(float deltaTime)
{
	m_player.Update(deltaTime);
	m_mainLight.Update(deltaTime);
}

void World::Draw(vkr::CommandBufferPtr cmd)
{
	// Draw entities
	cmd->BindGraphicsPipeline(m_drawObjectPipeline);
	for (size_t i = 0; i < m_entities.size(); ++i)
	{
		GameEntity& pEntity = m_entities[i];

		cmd->BindGraphicsDescriptorSets(m_drawObjectPipelineInterface, 1, &pEntity.drawDescriptorSet);
		cmd->BindIndexBuffer(pEntity.mesh);
		cmd->BindVertexBuffers(pEntity.mesh);
		cmd->DrawIndexed(pEntity.mesh->GetIndexCount());
	}

	// Draw light
	GetMainLight().DrawDebug(cmd);
}

void World::UpdateUniformBuffer()
{
	// Update uniform buffers
	for (size_t i = 0; i < m_entities.size(); ++i)
	{
		GameEntity& pEntity = m_entities[i];

		float4x4 T = glm::translate(pEntity.translate);
		float4x4 R =
			glm::rotate(pEntity.rotate.z, float3(0, 0, 1)) *
			glm::rotate(pEntity.rotate.y, float3(0, 1, 0)) *
			glm::rotate(pEntity.rotate.x, float3(1, 0, 0));
		float4x4 S = glm::scale(pEntity.scale);
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
		scene.CameraViewProjectionMatrix = GetPlayer().GetCamera().GetViewProjectionMatrix();
		scene.LightPosition = float4(GetMainLight().GetPosition(), 0);
		scene.LightViewProjectionMatrix = GetMainLight().GetCamera().GetViewProjectionMatrix();
		scene.UsePCF = uint4(m_game->GetGameGraphics().GetShadowPass().UsePCF());

		pEntity.drawUniformBuffer->CopyFromSource(sizeof(scene), &scene);

		// Shadow uniform buffers
		float4x4 PV = GetMainLight().GetCamera().GetViewProjectionMatrix();
		float4x4 MVP = PV * M; // Yes - the other is reversed

		pEntity.shadowUniformBuffer->CopyFromSource(sizeof(MVP), &MVP);
	}

	// Update light uniform buffer
	{
		float4x4        T = glm::translate(GetMainLight().GetPosition());
		const float4x4& PV = GetPlayer().GetCamera().GetViewProjectionMatrix();
		float4x4        MVP = PV * T; // Yes - the other is reversed
		GetMainLight().UpdateShaderUniform(sizeof(MVP), &MVP);
	}
}

bool World::setupDescriptorSetLayout()
{
	auto& device = m_game->GetRenderDevice();

	// Descriptor set layouts entities
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

bool World::addTestEntities()
{
	auto& device = m_game->GetRenderDevice();
	auto& gameGraphics = m_game->GetGameGraphics();
	vkr::DescriptorPoolPtr descriptorPool = gameGraphics.GetDescriptorPool();
	ShadowPass& shadowPassData = gameGraphics.GetShadowPass();

	vkr::TriMeshOptions options = vkr::TriMeshOptions()
		.Indices()
		.VertexColors()
		.Normals();

	GameEntity mGroundPlane;
	vkr::TriMesh mesh = vkr::TriMesh::CreatePlane(vkr::TRI_MESH_PLANE_POSITIVE_Y, float2(50, 50), 1, 1, vkr::TriMeshOptions(options).ObjectColor(float3(0.7f)));
	mGroundPlane.Setup(device, mesh, descriptorPool, m_drawObjectSetLayout, shadowPassData);
	m_entities.emplace_back(mGroundPlane);

	GameEntity mCube;
	mesh = vkr::TriMesh::CreateCube(float3(2, 2, 2), vkr::TriMeshOptions(options).ObjectColor(float3(0.5f, 0.5f, 0.7f)));
	mCube.Setup(device, mesh, descriptorPool, m_drawObjectSetLayout, shadowPassData);
	mCube.translate = float3(-2, 1, 0);
	m_entities.emplace_back(mCube);

	GameEntity mKnob;
	mesh = vkr::TriMesh::CreateFromOBJ("basic/models/material_sphere.obj", vkr::TriMeshOptions(options).ObjectColor(float3(0.7f, 0.2f, 0.2f)));
	mKnob.Setup(device, mesh, descriptorPool, m_drawObjectSetLayout, shadowPassData);
	mKnob.translate = float3(2, 1, 0);
	mKnob.rotate = float3(0, glm::radians(180.0f), 0);
	mKnob.scale = float3(2, 2, 2);
	m_entities.emplace_back(mKnob);

	return true;
}

bool World::setupEntities()
{
	auto& device = m_game->GetRenderDevice();

	// Draw object pipeline interface and pipeline
	{
		// Pipeline interface
		vkr::PipelineInterfaceCreateInfo piCreateInfo = {};
		piCreateInfo.setCount = 1;
		piCreateInfo.sets[0].set = 0;
		piCreateInfo.sets[0].layout = m_drawObjectSetLayout;
		CHECKED_CALL_AND_RETURN_FALSE(device.CreatePipelineInterface(piCreateInfo, &m_drawObjectPipelineInterface));

		// Pipeline
		vkr::ShaderModulePtr VS;
		CHECKED_CALL_AND_RETURN_FALSE(device.CreateShader("GameData/Shaders", "DiffuseShadow.vs", &VS));
		vkr::ShaderModulePtr PS;
		CHECKED_CALL_AND_RETURN_FALSE(device.CreateShader("GameData/Shaders", "DiffuseShadow.ps", &PS));

		vkr::GraphicsPipelineCreateInfo2 gpCreateInfo = {};
		gpCreateInfo.VS = { VS.Get(), "vsmain" };
		gpCreateInfo.PS = { PS.Get(), "psmain" };
		gpCreateInfo.vertexInputState.bindingCount = 3;
		gpCreateInfo.vertexInputState.bindings[0] = m_entities[0].mesh->GetDerivedVertexBindings()[0];
		gpCreateInfo.vertexInputState.bindings[1] = m_entities[0].mesh->GetDerivedVertexBindings()[1];
		gpCreateInfo.vertexInputState.bindings[2] = m_entities[0].mesh->GetDerivedVertexBindings()[2];
		gpCreateInfo.topology = vkr::PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		gpCreateInfo.polygonMode = vkr::POLYGON_MODE_FILL;
		gpCreateInfo.cullMode = vkr::CULL_MODE_BACK;
		gpCreateInfo.frontFace = vkr::FRONT_FACE_CCW;
		gpCreateInfo.depthReadEnable = true;
		gpCreateInfo.depthWriteEnable = true;
		gpCreateInfo.blendModes[0] = vkr::BLEND_MODE_NONE;
		gpCreateInfo.outputState.renderTargetCount = 1;
		gpCreateInfo.outputState.renderTargetFormats[0] = m_game->GetRender().GetSwapChain().GetColorFormat(); // TODO: передавать через креатор
		gpCreateInfo.outputState.depthStencilFormat = m_game->GetRender().GetSwapChain().GetDepthFormat(); // TODO: передавать через креатор
		gpCreateInfo.pipelineInterface = m_drawObjectPipelineInterface;

		CHECKED_CALL_AND_RETURN_FALSE(device.CreateGraphicsPipeline(gpCreateInfo, &m_drawObjectPipeline));
		device.DestroyShaderModule(VS);
		device.DestroyShaderModule(PS);
	}
	
	return true;
}
