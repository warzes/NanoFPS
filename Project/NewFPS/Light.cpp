#include "stdafx.h"
#include "Light.h"
#include "GameApp.h"

bool DirectionalLight::Setup(GameApplication* game)
{
	mLightCamera = PerspectiveCamera(60.0f, 1.0f, 1.0f, 100.0f);

	auto& device = game->GetRenderDevice();

	// Light
	{
		// Descriptor set layt
		vkr::DescriptorSetLayoutCreateInfo layoutCreateInfo = {};
		layoutCreateInfo.bindings.push_back(vkr::DescriptorBinding{ 0, vkr::DescriptorType::UniformBuffer, 1, vkr::SHADER_STAGE_ALL_GRAPHICS });
		CHECKED_CALL_AND_RETURN_FALSE(device.CreateDescriptorSetLayout(layoutCreateInfo, &mLightSetLayout));

		// Model
		vkr::TriMeshOptions options = vkr::TriMeshOptions().Indices().ObjectColor(float3(1, 1, 1));
		vkr::TriMesh        mesh = vkr::TriMesh::CreateCube(float3(0.25f, 0.25f, 0.25f), options);

		vkr::Geometry geo;
		CHECKED_CALL_AND_RETURN_FALSE(vkr::Geometry::Create(mesh, &geo));
		CHECKED_CALL_AND_RETURN_FALSE(vkr::vkrUtil::CreateMeshFromGeometry(device.GetGraphicsQueue(), &geo, &mLight.mesh));

		// Uniform buffer
		vkr::BufferCreateInfo bufferCreateInfo = {};
		bufferCreateInfo.size = vkr::MINIMUM_UNIFORM_BUFFER_SIZE;
		bufferCreateInfo.usageFlags.bits.uniformBuffer = true;
		bufferCreateInfo.memoryUsage = vkr::MemoryUsage::CPUToGPU;
		CHECKED_CALL_AND_RETURN_FALSE(device.CreateBuffer(bufferCreateInfo, &mLight.drawUniformBuffer));

		// Descriptor set
		CHECKED_CALL_AND_RETURN_FALSE(device.AllocateDescriptorSet(game->GetGameGraphics().GetDescriptorPool(), mLightSetLayout, &mLight.drawDescriptorSet));

		// Update descriptor set
		vkr::WriteDescriptor write = {};
		write.binding = 0;
		write.type = vkr::DescriptorType::UniformBuffer;
		write.bufferOffset = 0;
		write.bufferRange = WHOLE_SIZE;
		write.buffer = mLight.drawUniformBuffer;
		CHECKED_CALL_AND_RETURN_FALSE(mLight.drawDescriptorSet->UpdateDescriptors(1, &write));

		// Pipeline interface
		vkr::PipelineInterfaceCreateInfo piCreateInfo = {};
		piCreateInfo.setCount = 1;
		piCreateInfo.sets[0].set = 0;
		piCreateInfo.sets[0].layout = mLightSetLayout;
		CHECKED_CALL_AND_RETURN_FALSE(device.CreatePipelineInterface(piCreateInfo, &mLightPipelineInterface));

		// Pipeline
		vkr::ShaderModulePtr VS;
		CHECKED_CALL_AND_RETURN_FALSE(device.CreateShader("basic/shaders", "VertexColors.vs", &VS));
		vkr::ShaderModulePtr PS;
		CHECKED_CALL_AND_RETURN_FALSE(device.CreateShader("basic/shaders", "VertexColors.ps", &PS));

		vkr::GraphicsPipelineCreateInfo2 gpCreateInfo = {};
		gpCreateInfo.VS = { VS.Get(), "vsmain" };
		gpCreateInfo.PS = { PS.Get(), "psmain" };
		gpCreateInfo.vertexInputState.bindingCount = 2;
		gpCreateInfo.vertexInputState.bindings[0] = mLight.mesh->GetDerivedVertexBindings()[0];
		gpCreateInfo.vertexInputState.bindings[1] = mLight.mesh->GetDerivedVertexBindings()[1];
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
		gpCreateInfo.pipelineInterface = mLightPipelineInterface;

		CHECKED_CALL_AND_RETURN_FALSE(device.CreateGraphicsPipeline(gpCreateInfo, &mLightPipeline));
		device.DestroyShaderModule(VS);
		device.DestroyShaderModule(PS);
	}

	Update(0.001f); // TODO: может лишнее?

	return true;
}

void DirectionalLight::Shutdown()
{
	// TODO: очистка
}

void DirectionalLight::Update(float deltaTime)
{
	// Update light position
	float t = deltaTime / 2.0f;
	float r = 7.0f;
	mLightPosition = float3(r * cos(t), 5.0f, r * sin(t));

	// Update camera(s)
	mLightCamera.LookAt(mLightPosition, float3(0, 0, 0));
}

void DirectionalLight::DrawDebug(vkr::CommandBufferPtr cmd)
{
	cmd->BindGraphicsPipeline(mLightPipeline);
	cmd->BindGraphicsDescriptorSets(mLightPipelineInterface, 1, &mLight.drawDescriptorSet);
	cmd->BindIndexBuffer(mLight.mesh);
	cmd->BindVertexBuffers(mLight.mesh);
	cmd->DrawIndexed(mLight.mesh->GetIndexCount());
}

void DirectionalLight::UpdateShaderUniform(uint32_t dataSize, const void* srcData)
{
	mLight.drawUniformBuffer->CopyFromSource(dataSize, srcData);
}