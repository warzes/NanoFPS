#pragma once

class Example_011 final : public EngineApplication
{
public:
	EngineApplicationCreateInfo Config() const final;
	bool Setup() final;
	void Shutdown() final;
	void Update() final;
	void Render() final;

private:
	struct ShapeDesc
	{
		const char* texturePath;
		float3      homeLoc;
	};

	std::vector<ShapeDesc> textures = {
		ShapeDesc{"basic/textures/box_panel_bc1.dds", float3(-6, 2, 0)},
		ShapeDesc{"basic/textures/box_panel_bc2.dds", float3(-2, 2, 0)},
		ShapeDesc{"basic/textures/box_panel_bc3.dds", float3(2, 2, 0)},
		ShapeDesc{"basic/textures/box_panel_bc4.dds", float3(6, 2, 0)},
		ShapeDesc{"basic/textures/box_panel_bc5.dds", float3(-6, -2, 0)},
		ShapeDesc{"basic/textures/box_panel_bc6h.dds", float3(-2, -2, 0)},
		ShapeDesc{"basic/textures/box_panel_bc6h_sf.dds", float3(2, -2, 0)},
		ShapeDesc{"basic/textures/box_panel_bc7.dds", float3(6, -2, 0)},
	};

	struct PerFrame
	{
		vkr::CommandBufferPtr cmd;
		vkr::SemaphorePtr     imageAcquiredSemaphore;
		vkr::FencePtr         imageAcquiredFence;
		vkr::SemaphorePtr     renderCompleteSemaphore;
		vkr::FencePtr         renderCompleteFence;
	};

	struct TexturedShape
	{
		int                 id;
		vkr::DescriptorSetPtr    descriptorSet;
		vkr::BufferPtr           uniformBuffer;
		vkr::ImagePtr            image;
		vkr::SamplerPtr          sampler;
		vkr::SampledImageViewPtr sampledImageView;
		float3              homeLoc;
	};

	std::vector<PerFrame>      mPerFrame;
	vkr::ShaderModulePtr            mVS;
	vkr::ShaderModulePtr            mPS;
	vkr::PipelineInterfacePtr       mPipelineInterface;
	vkr::GraphicsPipelinePtr        mPipeline;
	vkr::BufferPtr                  mVertexBuffer;
	vkr::DescriptorPoolPtr          mDescriptorPool;
	vkr::DescriptorSetLayoutPtr     mDescriptorSetLayout;
	vkr::VertexBinding              mVertexBinding;
	std::vector<TexturedShape> mShapes;
};