#include "stdafx.h"
#include "Entity.h"

void GameEntity::Setup(vkr::RenderDevice& device, const vkr::TriMesh& mesh, vkr::DescriptorPool* pDescriptorPool, const vkr::DescriptorSetLayout* pDrawSetLayout, const vkr::DescriptorSetLayout* pShadowSetLayout)
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
	CHECKED_CALL(device.AllocateDescriptorSet(pDescriptorPool, pShadowSetLayout, &shadowDescriptorSet));

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
}