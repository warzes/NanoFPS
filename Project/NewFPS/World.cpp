#include "stdafx.h"
#include "World.h"
#include "GameApp.h"

bool World::Setup(GameApplication* game, const WorldCreateInfo& createInfo)
{
	m_game = game;
	if (!m_player.Setup(m_game, createInfo.player)) return false;
	if (!m_mainLight.Setup(m_game)) return false;

	if (!setupPipelineEntities()) return false;

	if (!loadMap(createInfo.startMapName)) return false;

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
		GameEntity& entity = m_entities[i];
		cmd->BindGraphicsDescriptorSets(m_drawObjectPipelineInterface, 1, &entity.drawDescriptorSet);
		cmd->BindIndexBuffer(entity.mesh);
		cmd->BindVertexBuffers(entity.mesh);
		cmd->DrawIndexed(entity.mesh->GetIndexCount());
	}

	// Draw light
	GetMainLight().DrawDebug(cmd);
}

void World::UpdateUniformBuffer()
{
	for (size_t i = 0; i < m_entities.size(); ++i)
	{
		GameEntity& entity = m_entities[i];
		entity.UniformBuffer(GetPlayer().GetCamera().GetViewProjectionMatrix(), GetMainLight(), m_game->GetGameGraphics().GetShadowPass().UsePCF());
	}

	// Update light uniform buffer
	{
		float4x4        T = glm::translate(GetMainLight().GetPosition());
		const float4x4& PV = GetPlayer().GetCamera().GetViewProjectionMatrix();
		float4x4        MVP = PV * T; // Yes - the other is reversed
		GetMainLight().UpdateShaderUniform(sizeof(MVP), &MVP);
	}
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

		vkr::GraphicsPipelineCreateInfo2 gpCreateInfo = {};
		gpCreateInfo.VS = { VS.Get(), "vsmain" };
		gpCreateInfo.PS = { PS.Get(), "psmain" };
		gpCreateInfo.vertexInputState.bindingCount = 4;
		gpCreateInfo.vertexInputState.bindings[0] = vertexBinding0;
		gpCreateInfo.vertexInputState.bindings[1] = vertexBinding1;
		gpCreateInfo.vertexInputState.bindings[2] = vertexBinding2;
		gpCreateInfo.vertexInputState.bindings[3] = vertexBinding3;
		gpCreateInfo.topology = vkr::PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		gpCreateInfo.polygonMode = vkr::POLYGON_MODE_FILL;
		gpCreateInfo.cullMode = vkr::CULL_MODE_BACK;
		gpCreateInfo.frontFace = vkr::FRONT_FACE_CCW;
		gpCreateInfo.depthReadEnable = true;
		gpCreateInfo.depthWriteEnable = true;
		gpCreateInfo.blendModes[0] = vkr::BLEND_MODE_NONE;
		gpCreateInfo.outputState.renderTargetCount = 1;
		gpCreateInfo.outputState.renderTargetFormats[0] = swapChain.GetColorFormat(); // TODO: передавать через креатор
		gpCreateInfo.outputState.depthStencilFormat = swapChain.GetDepthFormat(); // TODO: передавать через креатор
		gpCreateInfo.pipelineInterface = m_drawObjectPipelineInterface;

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

	auto& tileGrid = m_mapData.GetTileGrid();
	auto& tileModelFileName = m_mapData.GetModelPaths();

	bool first = true;
		
	for (size_t x = 0; x < tileGrid.GetWidth(); x++)
	{
		for (size_t y = 0; y < tileGrid.GetLength(); y++)
		{
			for (size_t z = 0; z < tileGrid.GetHeight(); z++)
			{
				auto tile = tileGrid.GetTile(x, z, y);
				if (tile.shape == fileMapData::NO_MODEL) continue;

				if (first)
				{
					m_mapMeshes = vkr::TriMesh::CreateFromOBJ(
						tileModelFileName[tile.shape],
						vkr::TriMeshOptions(options)
						.ObjectColor(float3(0.3f, 0.6f, 1.0f)));
					first = false;
				}
				else
				{
					auto mesh = vkr::TriMesh::CreateFromOBJ(
						tileModelFileName[tile.shape],
						vkr::TriMeshOptions(options)
						.ObjectColor(float3(0.3f, 0.6f, 1.0f))
						.Translate(float3{ x, z, y } * tileGrid.GetSpacing())
						.RotateX(glm::radians(float(-tile.pitch)))
						.RotateY(glm::radians(float(-tile.angle)))
					);

					// TODO: оптимизации
					// - загрузить меши в вектор, а потом уже мержить (функция мержа меша и вектора)
					// - возможно вмето += написать метод Merge в который передается как вектор, так и размеры всех мешей для ресерва (размеры подсчитать при загрузке меша)
					// - не грузить в этот вектор те меши, которые уже там есть

					m_mapMeshes += mesh;
				}
			}
		}
	}


	GameEntity entity;
	entity.Setup(device, m_mapMeshes, descriptorPool, m_drawObjectSetLayout, shadowPassData);
	m_entities.emplace_back(entity);

	auto tt = entity.mesh->GetDerivedVertexBindings();


	return true;
}