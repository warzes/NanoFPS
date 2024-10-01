#pragma once

class ShadowPass;

struct GameEntity
{
	bool Setup(vkr::RenderDevice& device, const vkr::TriMesh& mesh, vkr::DescriptorPool* pDescriptorPool, const vkr::DescriptorSetLayout* pDrawSetLayout, ShadowPass& shadowPass);

	float3           translate = float3(0, 0, 0);
	float3           rotate = float3(0, 0, 0);
	float3           scale = float3(1, 1, 1);
	vkr::MeshPtr          mesh;
	vkr::DescriptorSetPtr drawDescriptorSet;
	vkr::BufferPtr        drawUniformBuffer;
	vkr::DescriptorSetPtr shadowDescriptorSet;
	vkr::BufferPtr        shadowUniformBuffer;
};