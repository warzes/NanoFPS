#include "stdafx.h"
#include "012_Shadows.h"

#define kShadowMapSize 1024

EngineApplicationCreateInfo Example_012::Config() const
{
	EngineApplicationCreateInfo createInfo{};
	createInfo.render.swapChain.depthFormat = vkr::Format::D32_FLOAT;
	createInfo.render.showImgui = true;
	return createInfo;
}

bool Example_012::Setup()
{
	mUsePCF = false;


	auto& device = GetRenderDevice();

	// Cameras
	{
		mCamera = PerspCamera(60.0f, GetWindowAspect());
		mLightCamera = PerspCamera(60.0f, 1.0f, 1.0f, 100.0f);
	}

	// Create descriptor pool large enough for this project
	{
		vkr::DescriptorPoolCreateInfo poolCreateInfo = {};
		poolCreateInfo.uniformBuffer = 512;
		poolCreateInfo.sampledImage = 512;
		poolCreateInfo.sampler = 512;
		CHECKED_CALL(device.CreateDescriptorPool(poolCreateInfo, &mDescriptorPool));
	}

	// Descriptor set layouts
	{
		// Draw objects
		vkr::DescriptorSetLayoutCreateInfo layoutCreateInfo = {};
		layoutCreateInfo.bindings.push_back(vkr::DescriptorBinding{ 0, vkr::DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, vkr::SHADER_STAGE_ALL_GRAPHICS });
		layoutCreateInfo.bindings.push_back(vkr::DescriptorBinding{ 1, vkr::DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1, vkr::SHADER_STAGE_PS });
		layoutCreateInfo.bindings.push_back(vkr::DescriptorBinding{ 2, vkr::DESCRIPTOR_TYPE_SAMPLER, 1, vkr::SHADER_STAGE_PS });
		CHECKED_CALL(device.CreateDescriptorSetLayout(layoutCreateInfo, &mDrawObjectSetLayout));

		// Shadow
		layoutCreateInfo = {};
		layoutCreateInfo.bindings.push_back(vkr::DescriptorBinding{ 0, vkr::DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, vkr::SHADER_STAGE_ALL_GRAPHICS });
		CHECKED_CALL(device.CreateDescriptorSetLayout(layoutCreateInfo, &mShadowSetLayout));
	}

	// Setup entities
	{
		vkr::TriMeshOptions options = vkr::TriMeshOptions().Indices().VertexColors().Normals();
		vkr::TriMesh        mesh = vkr::TriMesh::CreatePlane(vkr::TRI_MESH_PLANE_POSITIVE_Y, float2(50, 50), 1, 1, vkr::TriMeshOptions(options).ObjectColor(float3(0.7f)));
		setupEntity(mesh, mDescriptorPool, mDrawObjectSetLayout, mShadowSetLayout, &mGroundPlane);
		mEntities.push_back(&mGroundPlane);

		mesh = vkr::TriMesh::CreateCube(float3(2, 2, 2), vkr::TriMeshOptions(options).ObjectColor(float3(0.5f, 0.5f, 0.7f)));
		setupEntity(mesh, mDescriptorPool, mDrawObjectSetLayout, mShadowSetLayout, &mCube);
		mCube.translate = float3(-2, 1, 0);
		mEntities.push_back(&mCube);

		mesh = vkr::TriMesh::CreateFromOBJ("basic/models/material_sphere.obj", vkr::TriMeshOptions(options).ObjectColor(float3(0.7f, 0.2f, 0.2f)));
		setupEntity(mesh, mDescriptorPool, mDrawObjectSetLayout, mShadowSetLayout, &mKnob);
		mKnob.translate = float3(2, 1, 0);
		mKnob.rotate = float3(0, glm::radians(180.0f), 0);
		mKnob.scale = float3(2, 2, 2);
		mEntities.push_back(&mKnob);
	}

	// Draw object pipeline interface and pipeline
	{
		// Pipeline interface
		vkr::PipelineInterfaceCreateInfo piCreateInfo = {};
		piCreateInfo.setCount = 1;
		piCreateInfo.sets[0].set = 0;
		piCreateInfo.sets[0].pLayout = mDrawObjectSetLayout;
		CHECKED_CALL(device.CreatePipelineInterface(piCreateInfo, &mDrawObjectPipelineInterface));

		// Pipeline
		vkr::ShaderModulePtr VS;
		CHECKED_CALL(device.CreateShader("basic/shaders", "DiffuseShadow.vs", &VS));
		vkr::ShaderModulePtr PS;
		CHECKED_CALL(device.CreateShader("basic/shaders", "DiffuseShadow.ps", &PS));

		vkr::GraphicsPipelineCreateInfo2 gpCreateInfo = {};
		gpCreateInfo.VS = { VS.Get(), "vsmain" };
		gpCreateInfo.PS = { PS.Get(), "psmain" };
		gpCreateInfo.vertexInputState.bindingCount = 3;
		gpCreateInfo.vertexInputState.bindings[0] = mGroundPlane.mesh->GetDerivedVertexBindings()[0];
		gpCreateInfo.vertexInputState.bindings[1] = mGroundPlane.mesh->GetDerivedVertexBindings()[1];
		gpCreateInfo.vertexInputState.bindings[2] = mGroundPlane.mesh->GetDerivedVertexBindings()[2];
		gpCreateInfo.topology = vkr::PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		gpCreateInfo.polygonMode = vkr::POLYGON_MODE_FILL;
		gpCreateInfo.cullMode = vkr::CULL_MODE_BACK;
		gpCreateInfo.frontFace = vkr::FRONT_FACE_CCW;
		gpCreateInfo.depthReadEnable = true;
		gpCreateInfo.depthWriteEnable = true;
		gpCreateInfo.blendModes[0] = vkr::BLEND_MODE_NONE;
		gpCreateInfo.outputState.renderTargetCount = 1;
		gpCreateInfo.outputState.renderTargetFormats[0] = GetRender().GetSwapChain().GetColorFormat();
		gpCreateInfo.outputState.depthStencilFormat = GetRender().GetSwapChain().GetDepthFormat();
		gpCreateInfo.pPipelineInterface = mDrawObjectPipelineInterface;

		CHECKED_CALL(device.CreateGraphicsPipeline(gpCreateInfo, &mDrawObjectPipeline));
		device.DestroyShaderModule(VS);
		device.DestroyShaderModule(PS);
	}

	// Shadow pipeline interface and pipeline
	{
		// Pipeline interface
		vkr::PipelineInterfaceCreateInfo piCreateInfo = {};
		piCreateInfo.setCount = 1;
		piCreateInfo.sets[0].set = 0;
		piCreateInfo.sets[0].pLayout = mShadowSetLayout;
		CHECKED_CALL(device.CreatePipelineInterface(piCreateInfo, &mShadowPipelineInterface));

		// Pipeline
		vkr::ShaderModulePtr VS;
		CHECKED_CALL(device.CreateShader("basic/shaders", "Depth.vs", &VS));

		vkr::GraphicsPipelineCreateInfo2 gpCreateInfo = {};
		gpCreateInfo.VS = { VS.Get(), "vsmain" };
		gpCreateInfo.vertexInputState.bindingCount = 1;
		gpCreateInfo.vertexInputState.bindings[0] = mGroundPlane.mesh->GetDerivedVertexBindings()[0];
		gpCreateInfo.topology = vkr::PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		gpCreateInfo.polygonMode = vkr::POLYGON_MODE_FILL;
		gpCreateInfo.cullMode = vkr::CULL_MODE_BACK;
		gpCreateInfo.frontFace = vkr::FRONT_FACE_CCW;
		gpCreateInfo.depthReadEnable = true;
		gpCreateInfo.depthWriteEnable = true;
		gpCreateInfo.blendModes[0] = vkr::BLEND_MODE_NONE;
		gpCreateInfo.outputState.renderTargetCount = 0;
		gpCreateInfo.outputState.depthStencilFormat = vkr::Format::D32_FLOAT;
		gpCreateInfo.pPipelineInterface = mShadowPipelineInterface;

		CHECKED_CALL(device.CreateGraphicsPipeline(gpCreateInfo, &mShadowPipeline));
		device.DestroyShaderModule(VS);
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

		CHECKED_CALL(device.CreateRenderPass(createInfo, &mShadowRenderPass));
	}

	// Update draw objects with shadow information
	{
		vkr::SampledImageViewCreateInfo ivCreateInfo = vkr::SampledImageViewCreateInfo::GuessFromImage(mShadowRenderPass->GetDepthStencilImage());
		CHECKED_CALL(device.CreateSampledImageView(ivCreateInfo, &mShadowImageView));

		vkr::SamplerCreateInfo samplerCreateInfo = {};
		samplerCreateInfo.addressModeU = vkr::SamplerAddressMode::ClampToEdge;
		samplerCreateInfo.addressModeV = vkr::SamplerAddressMode::ClampToEdge;
		samplerCreateInfo.addressModeW = vkr::SamplerAddressMode::ClampToEdge;
		samplerCreateInfo.compareEnable = true;
		samplerCreateInfo.compareOp = vkr::CompareOp::LessOrEqual;
		samplerCreateInfo.borderColor = vkr::BorderColor::FloatOpaqueWhite;
		CHECKED_CALL(device.CreateSampler(samplerCreateInfo, &mShadowSampler));

		vkr::WriteDescriptor writes[2] = {};
		writes[0].binding = 1; // Shadow texture
		writes[0].type = vkr::DESCRIPTOR_TYPE_SAMPLED_IMAGE;
		writes[0].pImageView = mShadowImageView;
		writes[1].binding = 2; // Shadow sampler
		writes[1].type = vkr::DESCRIPTOR_TYPE_SAMPLER;
		writes[1].pSampler = mShadowSampler;

		for (size_t i = 0; i < mEntities.size(); ++i)
		{
			Entity* pEntity = mEntities[i];
			CHECKED_CALL(pEntity->drawDescriptorSet->UpdateDescriptors(2, writes));
		}
	}

	// Light
	{
		// Descriptor set layt
		vkr::DescriptorSetLayoutCreateInfo layoutCreateInfo = {};
		layoutCreateInfo.bindings.push_back(vkr::DescriptorBinding{ 0, vkr::DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, vkr::SHADER_STAGE_ALL_GRAPHICS });
		CHECKED_CALL(device.CreateDescriptorSetLayout(layoutCreateInfo, &mLightSetLayout));

		// Model
		vkr::TriMeshOptions options = vkr::TriMeshOptions().Indices().ObjectColor(float3(1, 1, 1));
		vkr::TriMesh        mesh = vkr::TriMesh::CreateCube(float3(0.25f, 0.25f, 0.25f), options);

		vkr::Geometry geo;
		CHECKED_CALL(vkr::Geometry::Create(mesh, &geo));
		CHECKED_CALL(vkr::vkrUtil::CreateMeshFromGeometry(GetRenderDevice().GetGraphicsQueue(), &geo, &mLight.mesh));

		// Uniform buffer
		vkr::BufferCreateInfo bufferCreateInfo = {};
		bufferCreateInfo.size = vkr::MINIMUM_UNIFORM_BUFFER_SIZE;
		bufferCreateInfo.usageFlags.bits.uniformBuffer = true;
		bufferCreateInfo.memoryUsage = vkr::MemoryUsage::CPUToGPU;
		CHECKED_CALL(device.CreateBuffer(bufferCreateInfo, &mLight.drawUniformBuffer));

		// Descriptor set
		CHECKED_CALL(device.AllocateDescriptorSet(mDescriptorPool, mLightSetLayout, &mLight.drawDescriptorSet));

		// Update descriptor set
		vkr::WriteDescriptor write = {};
		write.binding = 0;
		write.type = vkr::DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		write.bufferOffset = 0;
		write.bufferRange = WHOLE_SIZE;
		write.pBuffer = mLight.drawUniformBuffer;
		CHECKED_CALL(mLight.drawDescriptorSet->UpdateDescriptors(1, &write));

		// Pipeline interface
		vkr::PipelineInterfaceCreateInfo piCreateInfo = {};
		piCreateInfo.setCount = 1;
		piCreateInfo.sets[0].set = 0;
		piCreateInfo.sets[0].pLayout = mLightSetLayout;
		CHECKED_CALL(device.CreatePipelineInterface(piCreateInfo, &mLightPipelineInterface));

		// Pipeline
		vkr::ShaderModulePtr VS;
		CHECKED_CALL(device.CreateShader("basic/shaders", "VertexColors.vs", &VS));
		vkr::ShaderModulePtr PS;
		CHECKED_CALL(device.CreateShader("basic/shaders", "VertexColors.ps", &PS));

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
		gpCreateInfo.outputState.renderTargetFormats[0] = GetRender().GetSwapChain().GetColorFormat();
		gpCreateInfo.outputState.depthStencilFormat = GetRender().GetSwapChain().GetDepthFormat();
		gpCreateInfo.pPipelineInterface = mLightPipelineInterface;

		CHECKED_CALL(device.CreateGraphicsPipeline(gpCreateInfo, &mLightPipeline));
		device.DestroyShaderModule(VS);
		device.DestroyShaderModule(PS);
	}

	// Per frame data
	{
		PerFrame frame = {};

		CHECKED_CALL(device.GetGraphicsQueue()->CreateCommandBuffer(&frame.cmd));

		vkr::SemaphoreCreateInfo semaCreateInfo = {};
		CHECKED_CALL(device.CreateSemaphore(semaCreateInfo, &frame.imageAcquiredSemaphore));

		vkr::FenceCreateInfo fenceCreateInfo = {};
		CHECKED_CALL(device.CreateFence(fenceCreateInfo, &frame.imageAcquiredFence));

		CHECKED_CALL(device.CreateSemaphore(semaCreateInfo, &frame.renderCompleteSemaphore));

		fenceCreateInfo = { true }; // Create signaled
		CHECKED_CALL(device.CreateFence(fenceCreateInfo, &frame.renderCompleteFence));

		mPerFrame.push_back(frame);
	}

	return true;
}

void Example_012::Shutdown()
{
	mPerFrame.clear();
	// TODO: доделать очистку
}

void Example_012::Update()
{
}

void Example_012::Render()
{
	auto& render = GetRender();
	auto& swapChain = render.GetSwapChain();
	PerFrame& frame = mPerFrame[0];

	// Wait for and reset render complete fence
	CHECKED_CALL(frame.renderCompleteFence->WaitAndReset());

	uint32_t imageIndex = UINT32_MAX;
	CHECKED_CALL(swapChain.AcquireNextImage(UINT64_MAX, frame.imageAcquiredSemaphore, frame.imageAcquiredFence, &imageIndex));

	// Wait for and reset image acquired fence
	CHECKED_CALL(frame.imageAcquiredFence->WaitAndReset());

	// Update light position
	//float t = GetElapsedSeconds() / 2.0f;
	static float t = 0.0;
	t += 0.001f;
	float r = 7.0f;
	mLightPosition = float3(r * cos(t), 5.0f, r * sin(t));

	// Update camera(s)
	mCamera.LookAt(float3(5, 7, 7), float3(0, 1, 0));
	mLightCamera.LookAt(mLightPosition, float3(0, 0, 0));

	// Update uniform buffers
	for (size_t i = 0; i < mEntities.size(); ++i)
	{
		Entity* pEntity = mEntities[i];

		float4x4 T = glm::translate(pEntity->translate);
		float4x4 R = glm::rotate(pEntity->rotate.z, float3(0, 0, 1)) *
			glm::rotate(pEntity->rotate.y, float3(0, 1, 0)) *
			glm::rotate(pEntity->rotate.x, float3(1, 0, 0));
		float4x4 S = glm::scale(pEntity->scale);
		float4x4 M = T * R * S;

		// Draw uniform buffers
		struct Scene
		{
			float4x4 ModelMatrix;                // Transforms object space to world space
			float4x4 NormalMatrix;               // Transforms object space to normal space
			float4   Ambient;                    // Object's ambient intensity
			float4x4 CameraViewProjectionMatrix; // Camera's view projection matrix
			float4   LightPosition;              // Light's position
			float4x4 LightViewProjectionMatrix;  // Light's view projection matrix
			uint4    UsePCF;                     // Enable/disable PCF
		};

		Scene scene = {};
		scene.ModelMatrix = M;
		scene.NormalMatrix = glm::inverseTranspose(M);
		scene.Ambient = float4(0.3f);
		scene.CameraViewProjectionMatrix = mCamera.GetViewProjectionMatrix();
		scene.LightPosition = float4(mLightPosition, 0);
		scene.LightViewProjectionMatrix = mLightCamera.GetViewProjectionMatrix();
		scene.UsePCF = uint4(mUsePCF);

		pEntity->drawUniformBuffer->CopyFromSource(sizeof(scene), &scene);

		// Shadow uniform buffers
		float4x4 PV = mLightCamera.GetViewProjectionMatrix();
		float4x4 MVP = PV * M; // Yes - the other is reversed

		pEntity->shadowUniformBuffer->CopyFromSource(sizeof(MVP), &MVP);
	}

	// Update light uniform buffer
	{
		float4x4        T = glm::translate(mLightPosition);
		const float4x4& PV = mCamera.GetViewProjectionMatrix();
		float4x4        MVP = PV * T; // Yes - the other is reversed

		mLight.drawUniformBuffer->CopyFromSource(sizeof(MVP), &MVP);
	}

	// Build command buffer
	CHECKED_CALL(frame.cmd->Begin());
	{
		vkr::RenderPassPtr renderPass = swapChain.GetRenderPass(imageIndex);
		ASSERT_MSG(!renderPass.IsNull(), "render pass object is null");

		// =====================================================================
		//  Render shadow pass
		// =====================================================================
		frame.cmd->TransitionImageLayout(mShadowRenderPass->GetDepthStencilImage(), ALL_SUBRESOURCES, vkr::ResourceState::PixelShaderResource, vkr::ResourceState::DepthStencilWrite);
		frame.cmd->BeginRenderPass(mShadowRenderPass);
		{
			frame.cmd->SetScissors(mShadowRenderPass->GetScissor());
			frame.cmd->SetViewports(mShadowRenderPass->GetViewport());

			// Draw entities
			frame.cmd->BindGraphicsPipeline(mShadowPipeline);
			for (size_t i = 0; i < mEntities.size(); ++i) {
				Entity* pEntity = mEntities[i];

				frame.cmd->BindGraphicsDescriptorSets(mShadowPipelineInterface, 1, &pEntity->shadowDescriptorSet);
				frame.cmd->BindIndexBuffer(pEntity->mesh);
				frame.cmd->BindVertexBuffers(pEntity->mesh);
				frame.cmd->DrawIndexed(pEntity->mesh->GetIndexCount());
			}
		}
		frame.cmd->EndRenderPass();
		frame.cmd->TransitionImageLayout(mShadowRenderPass->GetDepthStencilImage(), ALL_SUBRESOURCES, vkr::ResourceState::DepthStencilWrite, vkr::ResourceState::PixelShaderResource);

		// =====================================================================
		//  Render scene
		// =====================================================================
		frame.cmd->TransitionImageLayout(renderPass->GetRenderTargetImage(0), ALL_SUBRESOURCES, vkr::ResourceState::Present, vkr::ResourceState::RenderTarget);
		frame.cmd->BeginRenderPass(renderPass);
		{
			frame.cmd->SetScissors(render.GetScissor());
			frame.cmd->SetViewports(render.GetViewport());

			// Draw entities
			frame.cmd->BindGraphicsPipeline(mDrawObjectPipeline);
			for (size_t i = 0; i < mEntities.size(); ++i)
			{
				Entity* pEntity = mEntities[i];

				frame.cmd->BindGraphicsDescriptorSets(mDrawObjectPipelineInterface, 1, &pEntity->drawDescriptorSet);
				frame.cmd->BindIndexBuffer(pEntity->mesh);
				frame.cmd->BindVertexBuffers(pEntity->mesh);
				frame.cmd->DrawIndexed(pEntity->mesh->GetIndexCount());
			}

			// Draw light
			frame.cmd->BindGraphicsPipeline(mLightPipeline);
			frame.cmd->BindGraphicsDescriptorSets(mLightPipelineInterface, 1, &mLight.drawDescriptorSet);
			frame.cmd->BindIndexBuffer(mLight.mesh);
			frame.cmd->BindVertexBuffers(mLight.mesh);
			frame.cmd->DrawIndexed(mLight.mesh->GetIndexCount());

			// Draw ImGui
			render.DrawDebugInfo();
			{
				ImGui::Separator();
				ImGui::Checkbox("Use PCF Shadows", &mUsePCF);
			}
			render.DrawImGui(frame.cmd);
		}
		frame.cmd->EndRenderPass();
		frame.cmd->TransitionImageLayout(renderPass->GetRenderTargetImage(0), ALL_SUBRESOURCES, vkr::ResourceState::RenderTarget, vkr::ResourceState::Present);
	}
	CHECKED_CALL(frame.cmd->End());

	vkr::SubmitInfo submitInfo = {};
	submitInfo.commandBufferCount = 1;
	submitInfo.ppCommandBuffers = &frame.cmd;
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.ppWaitSemaphores = &frame.imageAcquiredSemaphore;
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.ppSignalSemaphores = &frame.renderCompleteSemaphore;
	submitInfo.pFence = frame.renderCompleteFence;

	CHECKED_CALL(render.GetGraphicsQueue()->Submit(&submitInfo));

	CHECKED_CALL(swapChain.Present(imageIndex, 1, &frame.renderCompleteSemaphore));
}

void Example_012::setupEntity(const vkr::TriMesh& mesh, vkr::DescriptorPool* pDescriptorPool, const vkr::DescriptorSetLayout* pDrawSetLayout, const vkr::DescriptorSetLayout* pShadowSetLayout, Entity* pEntity)
{
	vkr::Geometry geo;
	CHECKED_CALL(vkr::Geometry::Create(mesh, &geo));
	CHECKED_CALL(vkr::vkrUtil::CreateMeshFromGeometry(GetRenderDevice().GetGraphicsQueue(), &geo, &pEntity->mesh));

	// Draw uniform buffer
	vkr::BufferCreateInfo bufferCreateInfo = {};
	bufferCreateInfo.size = RoundUp(512, vkr::CONSTANT_BUFFER_ALIGNMENT);
	bufferCreateInfo.usageFlags.bits.uniformBuffer = true;
	bufferCreateInfo.memoryUsage = vkr::MemoryUsage::CPUToGPU;
	CHECKED_CALL(GetRenderDevice().CreateBuffer(bufferCreateInfo, &pEntity->drawUniformBuffer));

	// Shadow uniform buffer
	bufferCreateInfo = {};
	bufferCreateInfo.size = vkr::MINIMUM_UNIFORM_BUFFER_SIZE;
	bufferCreateInfo.usageFlags.bits.uniformBuffer = true;
	bufferCreateInfo.memoryUsage = vkr::MemoryUsage::CPUToGPU;
	CHECKED_CALL(GetRenderDevice().CreateBuffer(bufferCreateInfo, &pEntity->shadowUniformBuffer));

	// Draw descriptor set
	CHECKED_CALL(GetRenderDevice().AllocateDescriptorSet(pDescriptorPool, pDrawSetLayout, &pEntity->drawDescriptorSet));

	// Shadow descriptor set
	CHECKED_CALL(GetRenderDevice().AllocateDescriptorSet(pDescriptorPool, pShadowSetLayout, &pEntity->shadowDescriptorSet));

	// Update draw descriptor set
	vkr::WriteDescriptor write = {};
	write.binding = 0;
	write.type = vkr::DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	write.bufferOffset = 0;
	write.bufferRange = WHOLE_SIZE;
	write.pBuffer = pEntity->drawUniformBuffer;
	CHECKED_CALL(pEntity->drawDescriptorSet->UpdateDescriptors(1, &write));

	// Update shadow descriptor set
	write = {};
	write.binding = 0;
	write.type = vkr::DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	write.bufferOffset = 0;
	write.bufferRange = WHOLE_SIZE;
	write.pBuffer = pEntity->shadowUniformBuffer;
	CHECKED_CALL(pEntity->shadowDescriptorSet->UpdateDescriptors(1, &write));
}