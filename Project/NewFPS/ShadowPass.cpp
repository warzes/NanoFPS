#include "stdafx.h"
#include "ShadowPass.h"
#include "Entity.h"

#define kShadowMapSize 1024

bool ShadowPass::Setup(vkr::RenderDevice& device)
{
	// Create Descriptor Set Layout
	{
		vkr::DescriptorSetLayoutCreateInfo layoutCreateInfo = {};
		layoutCreateInfo.bindings.push_back(vkr::DescriptorBinding{ 0, vkr::DescriptorType::UniformBuffer, 1, vkr::SHADER_STAGE_ALL_GRAPHICS });
		CHECKED_CALL_AND_RETURN_FALSE(device.CreateDescriptorSetLayout(layoutCreateInfo, &m_shadowSetLayout));
	}

	// Shadow render pass
	{
		vkr::RenderPassCreateInfo2 createInfo = {};
		createInfo.width = kShadowMapSize;
		createInfo.height = kShadowMapSize;
		createInfo.depthStencilFormat = vkr::Format::D32_FLOAT;
		createInfo.depthStencilUsageFlags.bits.depthStencilAttachment = true;
		createInfo.depthStencilUsageFlags.bits.sampled = true;
		createInfo.depthStencilClearValue = { 1.0f, 0xFF };
		createInfo.depthLoadOp = vkr::AttachmentLoadOp::Clear;
		createInfo.depthStoreOp = vkr::AttachmentStoreOp::Store;
		createInfo.depthStencilInitialState = vkr::ResourceState::PixelShaderResource;

		CHECKED_CALL_AND_RETURN_FALSE(device.CreateRenderPass(createInfo, &mShadowRenderPass));
	}

	// Update draw objects with shadow information
	{
		vkr::SampledImageViewCreateInfo ivCreateInfo = vkr::SampledImageViewCreateInfo::GuessFromImage(mShadowRenderPass->GetDepthStencilImage());
		CHECKED_CALL_AND_RETURN_FALSE(device.CreateSampledImageView(ivCreateInfo, &mShadowImageView));

		vkr::SamplerCreateInfo samplerCreateInfo = {};
		samplerCreateInfo.addressModeU = vkr::SamplerAddressMode::ClampToEdge;
		samplerCreateInfo.addressModeV = vkr::SamplerAddressMode::ClampToEdge;
		samplerCreateInfo.addressModeW = vkr::SamplerAddressMode::ClampToEdge;
		samplerCreateInfo.compareEnable = true;
		samplerCreateInfo.compareOp = vkr::CompareOp::LessOrEqual;
		samplerCreateInfo.borderColor = vkr::BorderColor::FloatOpaqueWhite;
		CHECKED_CALL_AND_RETURN_FALSE(device.CreateSampler(samplerCreateInfo, &mShadowSampler));
	}

	// Shadow pipeline interface and pipeline
	{
		// Pipeline interface
		vkr::PipelineInterfaceCreateInfo piCreateInfo = {};
		piCreateInfo.setCount = 1;
		piCreateInfo.sets[0].set = 0;
		piCreateInfo.sets[0].layout = m_shadowSetLayout;
		CHECKED_CALL_AND_RETURN_FALSE(device.CreatePipelineInterface(piCreateInfo, &mShadowPipelineInterface));

		// Pipeline
		vkr::ShaderModulePtr VS;
		CHECKED_CALL_AND_RETURN_FALSE(device.CreateShader("GameData/Shaders", "Depth.vs", &VS));

		vkr::VertexAttribute vertexAttribute = {
			.semanticName = "POSITION",
			.format = vkr::Format::R32G32B32_FLOAT,
			.inputRate = vkr::VertexInputRate::Vertex,
			.semantic = vkr::VertexSemantic::Position
		};

		vkr::GraphicsPipelineCreateInfo2 gpCreateInfo = {};
		gpCreateInfo.VS = { VS.Get(), "vsmain" };
		gpCreateInfo.vertexInputState.bindingCount = 1;
		gpCreateInfo.vertexInputState.bindings[0] = vkr::VertexBinding(vertexAttribute);
		gpCreateInfo.topology = vkr::PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		gpCreateInfo.polygonMode = vkr::POLYGON_MODE_FILL;
		gpCreateInfo.cullMode = vkr::CULL_MODE_BACK;
		gpCreateInfo.frontFace = vkr::FRONT_FACE_CCW;
		gpCreateInfo.depthReadEnable = true;
		gpCreateInfo.depthWriteEnable = true;
		gpCreateInfo.blendModes[0] = vkr::BLEND_MODE_NONE;
		gpCreateInfo.outputState.renderTargetCount = 0;
		gpCreateInfo.outputState.depthStencilFormat = vkr::Format::D32_FLOAT;
		gpCreateInfo.pipelineInterface = mShadowPipelineInterface;

		CHECKED_CALL_AND_RETURN_FALSE(device.CreateGraphicsPipeline(gpCreateInfo, &mShadowPipeline));
		device.DestroyShaderModule(VS);
	}

	return true;
}

void ShadowPass::Shutdown()
{
}

void ShadowPass::Draw(vkr::CommandBufferPtr cmd, const std::vector<GameEntity>& entities)
{
	//  Render shadow pass
	{
		cmd->TransitionImageLayout(mShadowRenderPass->GetDepthStencilImage(), ALL_SUBRESOURCES, vkr::ResourceState::PixelShaderResource, vkr::ResourceState::DepthStencilWrite);
		cmd->BeginRenderPass(mShadowRenderPass);
		{
			cmd->SetScissors(mShadowRenderPass->GetScissor());
			cmd->SetViewports(mShadowRenderPass->GetViewport());

			// Draw entities
			cmd->BindGraphicsPipeline(mShadowPipeline);
			for (size_t i = 0; i < entities.size(); ++i)
			{
				auto entity = entities[i];

				cmd->BindGraphicsDescriptorSets(mShadowPipelineInterface, 1, &entity.shadowDescriptorSet);
				cmd->BindIndexBuffer(entity.mesh);
				cmd->BindVertexBuffers(entity.mesh);
				cmd->DrawIndexed(entity.mesh->GetIndexCount());
			}
		}
		cmd->EndRenderPass();
		cmd->TransitionImageLayout(mShadowRenderPass->GetDepthStencilImage(), ALL_SUBRESOURCES, vkr::ResourceState::DepthStencilWrite, vkr::ResourceState::PixelShaderResource);
	}
}