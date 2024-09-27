#include "stdafx.h"
#include "013_NormalMap.h"

EngineApplicationCreateInfo Example_013::Config() const
{
	EngineApplicationCreateInfo createInfo{};
	createInfo.render.swapChain.depthFormat = vkr::Format::D32_FLOAT;
	return createInfo;
}

bool Example_013::Setup()
{
	auto& device = GetRenderDevice();

	// Cameras
	{
		mCamera = PerspCamera(60.0f, GetWindowAspect());
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
		layoutCreateInfo.bindings.push_back(vkr::DescriptorBinding{ 0, vkr::DescriptorType::UniformBuffer, 1, vkr::SHADER_STAGE_ALL_GRAPHICS });
		layoutCreateInfo.bindings.push_back(vkr::DescriptorBinding{ 1, vkr::DescriptorType::SampledImage, 1, vkr::SHADER_STAGE_PS });
		layoutCreateInfo.bindings.push_back(vkr::DescriptorBinding{ 2, vkr::DescriptorType::SampledImage, 1, vkr::SHADER_STAGE_PS });
		layoutCreateInfo.bindings.push_back(vkr::DescriptorBinding{ 3, vkr::DescriptorType::Sampler, 1, vkr::SHADER_STAGE_PS });
		CHECKED_CALL(device.CreateDescriptorSetLayout(layoutCreateInfo, &mDrawObjectSetLayout));
	}

	// Textures, views, and samplers
	{
		vkr::vkrUtil::ImageOptions options = vkr::vkrUtil::ImageOptions().MipLevelCount(RemainingMipLevels);
		CHECKED_CALL(vkr::vkrUtil::CreateImageFromFile(device.GetGraphicsQueue(), "basic/textures/normal_map/albedo.jpg", &mAlbedoTexture, options, false));
		CHECKED_CALL(vkr::vkrUtil::CreateImageFromFile(device.GetGraphicsQueue(), "basic/textures/normal_map/normal.jpg", &mNormalMap, options, false));

		vkr::SampledImageViewCreateInfo sivCreateInfo = vkr::SampledImageViewCreateInfo::GuessFromImage(mAlbedoTexture);
		CHECKED_CALL(device.CreateSampledImageView(sivCreateInfo, &mAlbedoTextureView));

		sivCreateInfo = vkr::SampledImageViewCreateInfo::GuessFromImage(mNormalMap);
		CHECKED_CALL(device.CreateSampledImageView(sivCreateInfo, &mNormalMapView));

		vkr::SamplerCreateInfo samplerCreateInfo = {};
		CHECKED_CALL(device.CreateSampler(samplerCreateInfo, &mSampler));
	}

	// Setup entities
	{
		vkr::TriMeshOptions options = vkr::TriMeshOptions().Indices().Normals().TexCoords().Tangents();

		vkr::TriMesh mesh = vkr::TriMesh::CreateCube(float3(2, 2, 2), vkr::TriMeshOptions(options).ObjectColor(float3(0.7f)));
		setupEntity(mesh, mDescriptorPool, mDrawObjectSetLayout, mDrawObjectSetLayout, &mCube);
		mEntities.push_back(&mCube);

		mesh = vkr::TriMesh::CreateSphere(2, 16, 8, vkr::TriMeshOptions(options).ObjectColor(float3(0.7f)).TexCoordScale(float2(3)));
		setupEntity(mesh, mDescriptorPool, mDrawObjectSetLayout, mDrawObjectSetLayout, &mSphere);
		mEntities.push_back(&mSphere);
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
		CHECKED_CALL(device.CreateShader("basic/shaders", "NormalMap.vs", &VS));

		vkr::ShaderModulePtr PS;
		CHECKED_CALL(device.CreateShader("basic/shaders", "NormalMap.ps", &PS));

		vkr::GraphicsPipelineCreateInfo2 gpCreateInfo = {};
		gpCreateInfo.VS = { VS.Get(), "vsmain" };
		gpCreateInfo.PS = { PS.Get(), "psmain" };
		gpCreateInfo.vertexInputState.bindingCount = CountU32(mCube.mesh->GetDerivedVertexBindings());
		gpCreateInfo.vertexInputState.bindings[0] = mCube.mesh->GetDerivedVertexBindings()[0];
		gpCreateInfo.vertexInputState.bindings[1] = mCube.mesh->GetDerivedVertexBindings()[1];
		gpCreateInfo.vertexInputState.bindings[2] = mCube.mesh->GetDerivedVertexBindings()[2];
		gpCreateInfo.vertexInputState.bindings[3] = mCube.mesh->GetDerivedVertexBindings()[3];
		gpCreateInfo.vertexInputState.bindings[4] = mCube.mesh->GetDerivedVertexBindings()[4];
		gpCreateInfo.vertexInputState.bindings[5] = mCube.mesh->GetDerivedVertexBindings()[5];
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

	// Light
	{
		// Descriptor set layt
		vkr::DescriptorSetLayoutCreateInfo layoutCreateInfo = {};
		layoutCreateInfo.bindings.push_back(vkr::DescriptorBinding{ 0, vkr::DescriptorType::UniformBuffer, 1, vkr::SHADER_STAGE_ALL_GRAPHICS });
		CHECKED_CALL(device.CreateDescriptorSetLayout(layoutCreateInfo, &mLightSetLayout));

		// Model
		vkr::TriMeshOptions options = vkr::TriMeshOptions().Indices().ObjectColor(float3(1, 1, 1));
		vkr::TriMesh        mesh = vkr::TriMesh::CreateCube(float3(0.25f, 0.25f, 0.25f), options);

		vkr::Geometry geo;
		CHECKED_CALL(vkr::Geometry::Create(mesh, &geo));
		CHECKED_CALL(vkr::vkrUtil::CreateMeshFromGeometry(device.GetGraphicsQueue(), &geo, &mLight.mesh));

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
		write.type = vkr::DescriptorType::UniformBuffer;
		write.bufferOffset = 0;
		write.bufferRange = WHOLE_SIZE;
		write.buffer = mLight.drawUniformBuffer;
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

void Example_013::Shutdown()
{
	mPerFrame.clear();
	// TODO: доделать очистку
}

void Example_013::Update()
{
}

void Example_013::Render()
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
	
	float r = 3.0f;
	mLightPosition = float3(2, 2, 2);

	// Update camera(s)
	mCamera.LookAt(float3(0, 0, 5), float3(0, 0, 0));

	// Update uniform buffer(s)
	{
		Entity* pEntity = mEntities[mEntityIndex];

		pEntity->translate = float3(0, 0, -10 * (1 + sin(t / 2)));
		pEntity->rotate = float3(t, t, 2 * t);

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
			float4   EyePosition;                // Eye position
		};

		Scene scene = {};
		scene.ModelMatrix = M;
		scene.NormalMatrix = glm::inverseTranspose(M);
		scene.Ambient = float4(0.3f);
		scene.CameraViewProjectionMatrix = mCamera.GetViewProjectionMatrix();
		scene.LightPosition = float4(mLightPosition, 0);
		scene.EyePosition = float4(mCamera.GetEyePosition(), 1);

		pEntity->drawUniformBuffer->CopyFromSource(sizeof(scene), &scene);
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
	   //  Render scene
	   // =====================================================================
		frame.cmd->TransitionImageLayout(renderPass->GetRenderTargetImage(0), ALL_SUBRESOURCES, vkr::ResourceState::Present, vkr::ResourceState::RenderTarget);
		frame.cmd->BeginRenderPass(renderPass);
		{
			frame.cmd->SetScissors(render.GetScissor());
			frame.cmd->SetViewports(render.GetViewport());

			// Draw entities
			frame.cmd->BindGraphicsPipeline(mDrawObjectPipeline);
			frame.cmd->BindGraphicsDescriptorSets(mDrawObjectPipelineInterface, 1, &mEntities[mEntityIndex]->drawDescriptorSet);
			frame.cmd->BindIndexBuffer(mEntities[mEntityIndex]->mesh);
			frame.cmd->BindVertexBuffers(mEntities[mEntityIndex]->mesh);
			frame.cmd->DrawIndexed(mEntities[mEntityIndex]->mesh->GetIndexCount());

			// Draw light
			frame.cmd->BindGraphicsPipeline(mLightPipeline);
			frame.cmd->BindGraphicsDescriptorSets(mLightPipelineInterface, 1, &mLight.drawDescriptorSet);
			frame.cmd->BindIndexBuffer(mLight.mesh);
			frame.cmd->BindVertexBuffers(mLight.mesh);
			frame.cmd->DrawIndexed(mLight.mesh->GetIndexCount());

			// Draw ImGui
			render.DrawDebugInfo();
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

void Example_013::setupEntity(const vkr::TriMesh& mesh, vkr::DescriptorPool* pDescriptorPool, const vkr::DescriptorSetLayout* pDrawSetLayout, const vkr::DescriptorSetLayout* pShadowSetLayout, Entity* pEntity)
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

	// Draw descriptor set
	CHECKED_CALL(GetRenderDevice().AllocateDescriptorSet(pDescriptorPool, pDrawSetLayout, &pEntity->drawDescriptorSet));

	// Update draw descriptor set
	vkr::WriteDescriptor writes[4] = {};
	writes[0].binding = 0; // Uniform buffer
	writes[0].type = vkr::DescriptorType::UniformBuffer;
	writes[0].bufferOffset = 0;
	writes[0].bufferRange = WHOLE_SIZE;
	writes[0].buffer = pEntity->drawUniformBuffer;
	writes[1].binding = 1; // Albedo texture
	writes[1].type = vkr::DescriptorType::SampledImage;
	writes[1].imageView = mAlbedoTextureView;
	writes[2].binding = 2; // Normal map
	writes[2].type = vkr::DescriptorType::SampledImage;
	writes[2].imageView = mNormalMapView;
	writes[3].binding = 3; // Sampler
	writes[3].type = vkr::DescriptorType::Sampler;
	writes[3].sampler = mSampler;

	CHECKED_CALL(pEntity->drawDescriptorSet->UpdateDescriptors(4, writes));
}