#include "stdafx.h"
#include "TestPhysicalBox.h"
#include "GameApp.h"

bool TestPhysicalBox::Setup(GameApplication* game)
{
	auto& device = game->GetRenderDevice();
	auto phsystem = game->GetPhysicsSystem();
	auto phscene = game->GetPhysicsScene();

	{
		// Descriptor set layt
		vkr::DescriptorSetLayoutCreateInfo layoutCreateInfo = {};
		layoutCreateInfo.bindings.push_back(vkr::DescriptorBinding{ 0, vkr::DescriptorType::UniformBuffer, 1, vkr::SHADER_STAGE_ALL_GRAPHICS });
		CHECKED_CALL_AND_RETURN_FALSE(device.CreateDescriptorSetLayout(layoutCreateInfo, &m_setLayout));

		// Model
		vkr::TriMeshOptions options = vkr::TriMeshOptions()
			.Indices()
			.ObjectColor(float3(0, 1, 1));
		vkr::TriMesh        mesh = vkr::TriMesh::CreateCube(float3(1.0f, 1.0f, 1.0f), options);

		vkr::Geometry geo;
		CHECKED_CALL_AND_RETURN_FALSE(vkr::Geometry::Create(mesh, &geo));
		CHECKED_CALL_AND_RETURN_FALSE(vkr::vkrUtil::CreateMeshFromGeometry(device.GetGraphicsQueue(), &geo, &m_model.mesh));

		// Uniform buffer
		vkr::BufferCreateInfo bufferCreateInfo = {};
		bufferCreateInfo.size = vkr::MINIMUM_UNIFORM_BUFFER_SIZE;
		bufferCreateInfo.usageFlags.bits.uniformBuffer = true;
		bufferCreateInfo.memoryUsage = vkr::MemoryUsage::CPUToGPU;
		CHECKED_CALL_AND_RETURN_FALSE(device.CreateBuffer(bufferCreateInfo, &m_model.drawUniformBuffer));

		// Descriptor set
		CHECKED_CALL_AND_RETURN_FALSE(device.AllocateDescriptorSet(game->GetGameGraphics().GetDescriptorPool(), m_setLayout, &m_model.drawDescriptorSet));

		// Update descriptor set
		vkr::WriteDescriptor write = {};
		write.binding = 0;
		write.type = vkr::DescriptorType::UniformBuffer;
		write.bufferOffset = 0;
		write.bufferRange = WHOLE_SIZE;
		write.buffer = m_model.drawUniformBuffer;
		CHECKED_CALL_AND_RETURN_FALSE(m_model.drawDescriptorSet->UpdateDescriptors(1, &write));

		// Pipeline interface
		vkr::PipelineInterfaceCreateInfo piCreateInfo = {};
		piCreateInfo.setCount = 1;
		piCreateInfo.sets[0].set = 0;
		piCreateInfo.sets[0].layout = m_setLayout;
		CHECKED_CALL_AND_RETURN_FALSE(device.CreatePipelineInterface(piCreateInfo, &m_pipelineInterface));

		// Pipeline
		vkr::ShaderModulePtr VS;
		CHECKED_CALL_AND_RETURN_FALSE(device.CreateShader("basic/shaders", "VertexColors.vs", &VS));
		vkr::ShaderModulePtr PS;
		CHECKED_CALL_AND_RETURN_FALSE(device.CreateShader("basic/shaders", "VertexColors.ps", &PS));

		vkr::GraphicsPipelineCreateInfo2 gpCreateInfo = {};
		gpCreateInfo.VS = { VS.Get(), "vsmain" };
		gpCreateInfo.PS = { PS.Get(), "psmain" };
		gpCreateInfo.vertexInputState.bindingCount = 2;
		gpCreateInfo.vertexInputState.bindings[0] = m_model.mesh->GetDerivedVertexBindings()[0];
		gpCreateInfo.vertexInputState.bindings[1] = m_model.mesh->GetDerivedVertexBindings()[1];
		gpCreateInfo.topology = vkr::PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		gpCreateInfo.polygonMode = vkr::POLYGON_MODE_FILL;
		gpCreateInfo.cullMode = vkr::CULL_MODE_BACK;
		gpCreateInfo.frontFace = vkr::FRONT_FACE_CCW;
		gpCreateInfo.depthReadEnable = true;
		gpCreateInfo.depthWriteEnable = true;
		gpCreateInfo.blendModes[0] = vkr::BLEND_MODE_NONE;
		gpCreateInfo.outputState.renderTargetCount = 1;
		gpCreateInfo.outputState.renderTargetFormats[0] = game->GetRender().GetSwapChain().GetColorFormat();
		gpCreateInfo.outputState.depthStencilFormat = game->GetRender().GetSwapChain().GetDepthFormat();
		gpCreateInfo.pipelineInterface = m_pipelineInterface;

		CHECKED_CALL_AND_RETURN_FALSE(device.CreateGraphicsPipeline(gpCreateInfo, &m_pipeline));
		device.DestroyShaderModule(VS);
		device.DestroyShaderModule(PS);
	}

	m_material = phsystem.CreateMaterial(0.8f, 0.8f, 0.25f);

	physx::PxRigidStatic* groundPlane = physx::PxCreatePlane(*phsystem.GetPhysics(), physx::PxPlane(0, 1, 0, 0), *m_material);
	phscene.GetPxScene()->addActor(*groundPlane);

	physx::PxTransform t = physx::PxTransform(physx::PxVec3(20, 20, 20));
	auto geometry = physx::PxSphereGeometry(1);
	physx::PxVec3 velocity = physx::PxVec3(0, -1, -2);
	m_dynamic = physx::PxCreateDynamic(*phsystem.GetPhysics(), t, geometry, *m_material, 5.0f);
	m_dynamic->setAngularDamping(0.5f);
	m_dynamic->setLinearVelocity(velocity);
	phscene.GetPxScene()->addActor(*m_dynamic);

	return true;
}

void TestPhysicalBox::Shutdown()
{
}

void TestPhysicalBox::DrawDebug(vkr::CommandBufferPtr cmd, const float4x4& matPV)
{
	{
		const physx::PxTransform transform = m_dynamic->getGlobalPose();
		const glm::vec3 position = { transform.p.x, transform.p.y, transform.p.z };
		//m_velocity = (m_position - lastPosition) / fixedDeltaTime; // от текущей отнять пред
		//auto rotationMatrix = glm::mat4_cast(glm::quat{ transform.q.w, transform.q.x, transform.q.y, transform.q.z });

		float4x4 T = glm::translate(position);
		float4x4 MVP = matPV * T;
		m_model.drawUniformBuffer->CopyFromSource(sizeof(MVP), &MVP);
	}

	cmd->BindGraphicsPipeline(m_pipeline);
	cmd->BindGraphicsDescriptorSets(m_pipelineInterface, 1, &m_model.drawDescriptorSet);
	cmd->BindIndexBuffer(m_model.mesh);
	cmd->BindVertexBuffers(m_model.mesh);
	cmd->DrawIndexed(m_model.mesh->GetIndexCount());
}