#pragma once

class ShadowPass;
class DirectionalLight;
class GameApplication;

struct GameEntity
{
	bool Setup(GameApplication* game, vkr::RenderDevice& device, const vkr::TriMesh& mesh, const std::filesystem::path& diffuseTextureFileName, vkr::DescriptorPool* pDescriptorPool, const vkr::DescriptorSetLayout* pDrawSetLayout, ShadowPass& shadowPass);

	void UniformBuffer(const float4x4& viewProj, const DirectionalLight& mainLight, bool UsePCF);

	float3                   translate = float3(0, 0, 0);
	float3                   rotate = float3(0, 0, 0);
	float3                   scale = float3(1, 1, 1);
	vkr::MeshPtr             mesh;
	vkr::DescriptorSetPtr    drawDescriptorSet;
	vkr::BufferPtr           drawUniformBuffer;
	vkr::DescriptorSetPtr    shadowDescriptorSet;
	vkr::BufferPtr           shadowUniformBuffer;

	vkr::ImagePtr            image;
	vkr::SampledImageViewPtr sampledImageView;
	vkr::SamplerPtr          sampler;

	std::vector<glm::vec3> rawVertex;
	std::vector<uint32_t>  rawIndex;
	ph::StaticBodyPtr      phBody;
};