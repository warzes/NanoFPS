#include "stdafx.h"
#include "Entity.h"
#include "ShadowPass.h"
#include "Light.h"

// Draw uniform buffers
struct GameEntityScene
{
	float4x4 ModelMatrix;                // Transforms object space to world space
	float4x4 NormalMatrix;               // Transforms object space to normal space
	float4   Ambient;                    // Object's ambient intensity
	float4x4 CameraViewProjectionMatrix; // Camera's view projection matrix
	float4   LightPosition;              // Light's position
	float4x4 LightViewProjectionMatrix;  // Light's view projection matrix
	uint4    UsePCF;                     // Enable/disable PCF
};

bool GameEntity::Setup(vkr::RenderDevice& device, const vkr::TriMesh& mesh, vkr::DescriptorPool* pDescriptorPool, const vkr::DescriptorSetLayout* pDrawSetLayout, ShadowPass& shadowPass)
{
	vkr::Geometry geo;
	CHECKED_CALL(vkr::Geometry::Create(mesh, &geo));
	CHECKED_CALL(vkr::vkrUtil::CreateMeshFromGeometry(device.GetGraphicsQueue(), &geo, &this->mesh));

	// Draw uniform buffer
	vkr::BufferCreateInfo bufferCreateInfo = {};
	bufferCreateInfo.size = RoundUp(512, vkr::CONSTANT_BUFFER_ALIGNMENT);
	bufferCreateInfo.usageFlags.bits.uniformBuffer = true;
	bufferCreateInfo.memoryUsage = vkr::MemoryUsage::CPUToGPU;
	CHECKED_CALL(device.CreateBuffer(bufferCreateInfo, &drawUniformBuffer));

	// Shadow uniform buffer
	bufferCreateInfo = {};
	bufferCreateInfo.size = vkr::MINIMUM_UNIFORM_BUFFER_SIZE;
	bufferCreateInfo.usageFlags.bits.uniformBuffer = true;
	bufferCreateInfo.memoryUsage = vkr::MemoryUsage::CPUToGPU;
	CHECKED_CALL(device.CreateBuffer(bufferCreateInfo, &shadowUniformBuffer));

	// Draw descriptor set
	CHECKED_CALL(device.AllocateDescriptorSet(pDescriptorPool, pDrawSetLayout, &drawDescriptorSet));

	// Shadow descriptor set
	CHECKED_CALL(device.AllocateDescriptorSet(pDescriptorPool, shadowPass.GetDescriptorSetLayout(), &shadowDescriptorSet));

	// Update draw descriptor set
	vkr::WriteDescriptor write = {};
	write.binding = 0;
	write.type = vkr::DescriptorType::UniformBuffer;
	write.bufferOffset = 0;
	write.bufferRange = WHOLE_SIZE;
	write.buffer = drawUniformBuffer;
	CHECKED_CALL(drawDescriptorSet->UpdateDescriptors(1, &write));

	// Update shadow descriptor set
	write = {};
	write.binding = 0;
	write.type = vkr::DescriptorType::UniformBuffer;
	write.bufferOffset = 0;
	write.bufferRange = WHOLE_SIZE;
	write.buffer = shadowUniformBuffer;
	CHECKED_CALL(shadowDescriptorSet->UpdateDescriptors(1, &write));

	{
		vkr::WriteDescriptor writes[2] = {};
		writes[0].binding = 1; // Shadow texture
		writes[0].type = vkr::DescriptorType::SampledImage;
		writes[0].imageView = shadowPass.GetSampledImageView();;
		writes[1].binding = 2; // Shadow sampler
		writes[1].type = vkr::DescriptorType::Sampler;
		writes[1].sampler = shadowPass.GetSampler();

		CHECKED_CALL_AND_RETURN_FALSE(drawDescriptorSet->UpdateDescriptors(2, writes));
	}

	return true;
}

void GameEntity::UniformBuffer(const float4x4& viewProj, const DirectionalLight& mainLight, bool UsePCF)
{
	// TODO: сделать методом ентити
	float4x4 T = glm::translate(glm::mat4(1.0), translate);
	float4x4 R =
		glm::rotate(rotate.z, float3(0, 0, 1)) *
		glm::rotate(rotate.y, float3(0, 1, 0)) *
		glm::rotate(rotate.x, float3(1, 0, 0));
	float4x4 S = glm::scale(scale);
	float4x4 M = T * R * S;

	GameEntityScene scene = {};
	scene.ModelMatrix = M;
	scene.NormalMatrix = glm::inverseTranspose(M);
	scene.Ambient = float4(0.3f);
	scene.CameraViewProjectionMatrix = viewProj;
	scene.LightPosition = float4(mainLight.GetPosition(), 0);
	scene.LightViewProjectionMatrix = mainLight.GetCamera().GetViewProjectionMatrix();
	scene.UsePCF = uint4(UsePCF);

	drawUniformBuffer->CopyFromSource(sizeof(scene), &scene);

	// Shadow uniform buffers
	float4x4 PV = mainLight.GetCamera().GetViewProjectionMatrix();
	float4x4 MVP = PV * M; // Yes - the other is reversed

	shadowUniformBuffer->CopyFromSource(sizeof(MVP), &MVP);
}