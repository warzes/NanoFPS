#include "stdafx.h"
#include "World.h"
#include "GameApp.h"

bool World::Setup(GameApplication* game, const WorldCreateInfo& createInfo)
{
	m_game = game;
	if (!m_player.Setup(m_game, createInfo.player)) return false;
	if (!m_mainLight.Setup(m_game)) return false;
	if (!m_phBox.Setup(m_game)) return false;

	if (!setupPipelineEntities()) return false;

	if (!loadMap(createInfo.startMapName)) return false;

	return true;
}

void World::Shutdown()
{
	m_mapData.Shutdown();
	m_phBox.Shutdown();
	m_mainLight.Shutdown();
	m_player.Shutdown();
}

void World::Update(float deltaTime)
{
	// TODO: не нужно каждый вызов это делать
	{
		m_aspect = m_game->GetWindowAspect();

		float horizFovRadians = glm::radians(m_horizFovDegrees);
		float vertFovRadians = 2.0f * atan(tan(horizFovRadians / 2.0f) / m_aspect);
		float vertFovDegrees = glm::degrees(vertFovRadians);

		m_projectionMatrix = glm::perspective(vertFovRadians, m_aspect, CAMERA_DEFAULT_NEAR_CLIP, CAMERA_DEFAULT_FAR_CLIP);
	}

	m_player.Update(deltaTime);
	m_mainLight.Update(deltaTime);
}

void World::FixedUpdate(float fixedDeltaTime)
{
	m_player.FixedUpdate(fixedDeltaTime);
}

void World::Draw(vkr::CommandBufferPtr cmd)
{
	// Draw entities
	cmd->BindGraphicsPipeline(m_drawObjectPipeline);
	for (size_t i = 0; i < m_entities.size(); ++i)
	{
		GameEntity& entity = m_entities[i];
		cmd->BindGraphicsDescriptorSets(m_drawObjectPipelineInterface, 1, &entity.drawDescriptorSet);
		cmd->BindIndexBuffer(entity.mesh);
		cmd->BindVertexBuffers(entity.mesh);
		cmd->DrawIndexed(entity.mesh->GetIndexCount());
	}

	// Draw light
	m_mainLight.DrawDebug(cmd);
	m_phBox.DrawDebug(cmd, GetViewProjectionMatrix());
}

void World::UpdateUniformBuffer()
{
	for (size_t i = 0; i < m_entities.size(); ++i)
	{
		GameEntity& entity = m_entities[i];
		entity.UniformBuffer(GetViewProjectionMatrix(), m_mainLight, m_game->GetGameGraphics().GetShadowPass().UsePCF());
	}

	// Update light uniform buffer
	{
		float4x4        T = glm::translate(m_mainLight.GetPosition());
		const float4x4& PV = GetViewProjectionMatrix();
		float4x4        MVP = PV * T; // Yes - the other is reversed
		m_mainLight.UpdateShaderUniform(sizeof(MVP), &MVP);
	}
}

glm::mat4 World::GetViewProjectionMatrix()
{
	Transform& transform = m_player.GetTransform();
	const glm::vec3 position = transform.GetTranslation();

	glm::vec3 forward = transform.GetForwardVector();
	glm::vec3 up = transform.GetUpVector();
	//glm::mat4 view = glm::inverse(transform.GetTranslationMatrix() * transform.GetRotationMatrix());
	glm::mat4 view = glm::lookAt(position, position + forward, up);

	return m_projectionMatrix * view;

}

bool World::setupPipelineEntities()
{
	auto& device = m_game->GetRenderDevice();
	auto& swapChain = m_game->GetRender().GetSwapChain();

	// Descriptor set layouts entities
	{
		// Draw objects
		vkr::DescriptorSetLayoutCreateInfo layoutCreateInfo = {};
		layoutCreateInfo.bindings.push_back(vkr::DescriptorBinding{ 0, vkr::DescriptorType::UniformBuffer, 1, vkr::SHADER_STAGE_ALL_GRAPHICS });
		layoutCreateInfo.bindings.push_back(vkr::DescriptorBinding{ 1, vkr::DescriptorType::SampledImage, 1, vkr::SHADER_STAGE_PS });
		layoutCreateInfo.bindings.push_back(vkr::DescriptorBinding{ 2, vkr::DescriptorType::Sampler, 1, vkr::SHADER_STAGE_PS });
		layoutCreateInfo.bindings.push_back(vkr::DescriptorBinding{ 3, vkr::DescriptorType::SampledImage, 1, vkr::SHADER_STAGE_PS });
		layoutCreateInfo.bindings.push_back(vkr::DescriptorBinding{ 4, vkr::DescriptorType::Sampler, 1, vkr::SHADER_STAGE_PS });

		CHECKED_CALL_AND_RETURN_FALSE(device.CreateDescriptorSetLayout(layoutCreateInfo, &m_drawObjectSetLayout));
	}

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
				
		vkr::VertexAttribute vertexAttribute0 = {
			.semanticName = "POSITION",
			.location = 0,
			.format = vkr::Format::R32G32B32_FLOAT,
			.binding = 0,
			.offset = 0,
			.inputRate = vkr::VertexInputRate::Vertex,
			.semantic = vkr::VertexSemantic::Position
		};

		vkr::VertexAttribute vertexAttribute1 = {
			.semanticName = "COLOR",
			.location = 1,
			.format = vkr::Format::R32G32B32_FLOAT,
			.binding = 1,
			.offset = 0,
			.inputRate = vkr::VertexInputRate::Vertex,
			.semantic = vkr::VertexSemantic::Color
		};

		vkr::VertexAttribute vertexAttribute2 = {
			.semanticName = "NORMAL",
			.location = 2,
			.format = vkr::Format::R32G32B32_FLOAT,
			.binding = 2,
			.offset = 0,
			.inputRate = vkr::VertexInputRate::Vertex,
			.semantic = vkr::VertexSemantic::Normal
		};

		vkr::VertexAttribute vertexAttribute3 = {
			.semanticName = "TEXCOORD",
			.location = 3,
			.format = vkr::Format::R32G32_FLOAT,
			.binding = 3,
			.offset = 0,
			.inputRate = vkr::VertexInputRate::Vertex,
			.semantic = vkr::VertexSemantic::Texcoord
		};

		vkr::VertexBinding vertexBinding0{};
		vertexBinding0.SetBinding(0);
		vertexBinding0.SetStride(12);
		vertexBinding0.AppendAttribute(vertexAttribute0);

		vkr::VertexBinding vertexBinding1{};
		vertexBinding1.SetBinding(1);
		vertexBinding1.SetStride(12);
		vertexBinding1.AppendAttribute(vertexAttribute1);

		vkr::VertexBinding vertexBinding2{};
		vertexBinding2.SetBinding(2);
		vertexBinding2.SetStride(12);
		vertexBinding2.AppendAttribute(vertexAttribute2);

		vkr::VertexBinding vertexBinding3{};
		vertexBinding3.SetBinding(3);
		vertexBinding3.SetStride(8);
		vertexBinding3.AppendAttribute(vertexAttribute3);

		vkr::GraphicsPipelineCreateInfo2 gpCreateInfo   = {};
		gpCreateInfo.VS                                 = { VS.Get(), "vsmain" };
		gpCreateInfo.PS                                 = { PS.Get(), "psmain" };
		gpCreateInfo.vertexInputState.bindingCount      = 4;
		gpCreateInfo.vertexInputState.bindings[0]       = vertexBinding0;
		gpCreateInfo.vertexInputState.bindings[1]       = vertexBinding1;
		gpCreateInfo.vertexInputState.bindings[2]       = vertexBinding2;
		gpCreateInfo.vertexInputState.bindings[3]       = vertexBinding3;
		gpCreateInfo.topology                           = vkr::PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		gpCreateInfo.polygonMode                        = vkr::POLYGON_MODE_FILL;
		gpCreateInfo.cullMode                           = vkr::CULL_MODE_BACK;
		gpCreateInfo.frontFace                          = vkr::FRONT_FACE_CCW;
		gpCreateInfo.depthReadEnable                    = true;
		gpCreateInfo.depthWriteEnable                   = true;
		gpCreateInfo.blendModes[0]                      = vkr::BLEND_MODE_NONE;
		gpCreateInfo.outputState.renderTargetCount      = 1;
		gpCreateInfo.outputState.renderTargetFormats[0] = swapChain.GetColorFormat(); // TODO: передавать через креатор
		gpCreateInfo.outputState.depthStencilFormat     = swapChain.GetDepthFormat(); // TODO: передавать через креатор
		gpCreateInfo.pipelineInterface                  = m_drawObjectPipelineInterface;

		CHECKED_CALL_AND_RETURN_FALSE(device.CreateGraphicsPipeline(gpCreateInfo, &m_drawObjectPipeline));
		device.DestroyShaderModule(VS);
		device.DestroyShaderModule(PS);
	}

	return true;
}

bool World::addTestEntities()
{
	/*auto& device = m_game->GetRenderDevice();
	auto& gameGraphics = m_game->GetGameGraphics();
	vkr::DescriptorPoolPtr descriptorPool = gameGraphics.GetDescriptorPool();
	ShadowPass& shadowPassData = gameGraphics.GetShadowPass();

	vkr::TriMeshOptions options = vkr::TriMeshOptions()
		.Indices()
		.VertexColors()
		.Normals()
		.TexCoords();

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
	m_entities.emplace_back(mKnob);*/

	return true;
}

bool World::loadMap(std::string_view mapFileName)
{
	if (!m_mapData.Setup(mapFileName)) return false;
	if (!addTestEntities()) return false;

	auto& device = m_game->GetRenderDevice();
	auto& gameGraphics = m_game->GetGameGraphics();
	vkr::DescriptorPoolPtr descriptorPool = gameGraphics.GetDescriptorPool();
	ShadowPass& shadowPassData = gameGraphics.GetShadowPass();

	vkr::TriMeshOptions options = vkr::TriMeshOptions()
		.Indices()
		.VertexColors()
		.Normals()
		.TexCoords();

	const auto& tileGrid = m_mapData.GetTileGrid();
	const auto& tileModelFileName = m_mapData.GetModelPaths();
	const auto& tileTextureFileName = m_mapData.GetTexturePaths();
			
	for (size_t x = 0; x < tileGrid.GetWidth(); x++)
	{
		for (size_t y = 0; y < tileGrid.GetLength(); y++)
		{
			for (size_t z = 0; z < tileGrid.GetHeight(); z++)
			{
				auto tile = tileGrid.GetTile(x, z, y);
				if (!tile) continue;

				auto mesh = vkr::TriMesh::CreateFromOBJ(
					tileModelFileName[tile.shape],
					vkr::TriMeshOptions(options)
					.ObjectColor(float3(1.0f, 1.0f, 1.0f))
					.Translate(float3{ x, z, y } * tileGrid.GetSpacing())
					.RotateX(glm::radians(float(-tile.pitch)))
					.RotateY(glm::radians(float(-tile.angle)))
				);

				std::string textureName = tileTextureFileName[tile.texture].string();
				bool isFind = false;
				for (size_t i = 0; i < m_mapMeshes.size(); i++)
				{
					if (textureName == m_mapMeshes[i].diffuseTextureFileName)
					{
						isFind = true;
						m_mapMeshes[i].mesh += mesh;
						break;
					}
				}

				if (!isFind)
				{
					MeshBild mb{};
					mb.diffuseTextureFileName = textureName;
					mb.mesh = mesh;
					m_mapMeshes.emplace_back(mb);
				}
			}
		}
	}

	for (size_t i = 0; i < m_mapMeshes.size(); i++)
	{
		GameEntity entity;
		entity.Setup(m_game, device, m_mapMeshes[i].mesh, m_mapMeshes[i].diffuseTextureFileName, descriptorPool, m_drawObjectSetLayout, shadowPassData);
		m_entities.emplace_back(entity);
	}

	return true;
}