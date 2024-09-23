#include "stdafx.h"
#include "010_CubeMap.h"

#define ENABLE_GPU_QUERIES

EngineApplicationCreateInfo Example_010::Config() const
{
	EngineApplicationCreateInfo createInfo{};
	createInfo.render.swapChain.depthFormat = vkr::FORMAT_D32_FLOAT;
	return createInfo;
}

bool Example_010::Setup()
{
	auto& device = GetRenderDevice();

	// vkr::Texture image, view,  and sampler
	{
		vkr::grfx_util::CubeMapCreateInfo createInfo = {};
		createInfo.layout = vkr::grfx_util::CUBE_IMAGE_LAYOUT_CROSS_HORIZONTAL;
		createInfo.posX = ENCODE_CUBE_FACE(3, vkr::grfx_util::CUBE_FACE_OP_NONE, vkr::grfx_util::CUBE_FACE_OP_NONE);
		createInfo.negX = ENCODE_CUBE_FACE(1, vkr::grfx_util::CUBE_FACE_OP_NONE, vkr::grfx_util::CUBE_FACE_OP_NONE);
		createInfo.posY = ENCODE_CUBE_FACE(0, vkr::grfx_util::CUBE_FACE_OP_NONE, vkr::grfx_util::CUBE_FACE_OP_NONE);
		createInfo.negY = ENCODE_CUBE_FACE(5, vkr::grfx_util::CUBE_FACE_OP_NONE, vkr::grfx_util::CUBE_FACE_OP_NONE);
		createInfo.posZ = ENCODE_CUBE_FACE(2, vkr::grfx_util::CUBE_FACE_OP_NONE, vkr::grfx_util::CUBE_FACE_OP_NONE);
		createInfo.negZ = ENCODE_CUBE_FACE(4, vkr::grfx_util::CUBE_FACE_OP_NONE, vkr::grfx_util::CUBE_FACE_OP_NONE);

		CHECKED_CALL(vkr::grfx_util::CreateCubeMapFromFile(device.GetGraphicsQueue(), "basic/textures/hilly_terrain.png", &createInfo, &mCubeMapImage));

		vkr::SampledImageViewCreateInfo viewCreateInfo = vkr::SampledImageViewCreateInfo::GuessFromImage(mCubeMapImage);
		CHECKED_CALL(device.CreateSampledImageView(viewCreateInfo, &mCubeMapImageView));

		vkr::SamplerCreateInfo samplerCreateInfo = {};
		CHECKED_CALL(device.CreateSampler(samplerCreateInfo, &mCubeMapSampler));
	}

	// Descriptor stuff
	{
		vkr::DescriptorPoolCreateInfo poolCreateInfo = {};
		poolCreateInfo.uniformBuffer = 2;
		poolCreateInfo.sampledImage = 2;
		poolCreateInfo.sampler = 2;
		CHECKED_CALL(device.CreateDescriptorPool(poolCreateInfo, &mDescriptorPool));

		vkr::DescriptorSetLayoutCreateInfo layoutCreateInfo = {};
		layoutCreateInfo.bindings.push_back(vkr::DescriptorBinding{ 0, vkr::DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, vkr::SHADER_STAGE_ALL_GRAPHICS });
		layoutCreateInfo.bindings.push_back(vkr::DescriptorBinding{ 1, vkr::DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1, vkr::SHADER_STAGE_ALL_GRAPHICS });
		layoutCreateInfo.bindings.push_back(vkr::DescriptorBinding{ 2, vkr::DESCRIPTOR_TYPE_SAMPLER, 1, vkr::SHADER_STAGE_ALL_GRAPHICS });
		CHECKED_CALL(device.CreateDescriptorSetLayout(layoutCreateInfo, &mDescriptorSetLayout));
	}

	// Entities
	{
		vkr::TriMesh mesh = vkr::TriMesh::CreateCube(float3(8, 8, 8));
		setupEntity(mesh, vkr::GeometryCreateInfo::InterleavedU16().AddColor(), &mSkyBox);

		mesh = vkr::TriMesh::CreateFromOBJ("basic/models/material_sphere.obj", vkr::TriMeshOptions().Normals());
		setupEntity(mesh, vkr::GeometryCreateInfo::InterleavedU16().AddNormal(), &mReflector);
	}

	// Sky box pipeline
	{
		CHECKED_CALL(device.CreateShader("basic/shaders", "SkyBox.vs", &mVS));
		CHECKED_CALL(device.CreateShader("basic/shaders", "SkyBox.ps", &mPS));

		vkr::PipelineInterfaceCreateInfo piCreateInfo = {};
		piCreateInfo.setCount = 1;
		piCreateInfo.sets[0].set = 0;
		piCreateInfo.sets[0].pLayout = mDescriptorSetLayout;
		CHECKED_CALL(device.CreatePipelineInterface(piCreateInfo, &mPipelineInterface));

		vkr::GraphicsPipelineCreateInfo2 gpCreateInfo = {};
		gpCreateInfo.VS = { mVS.Get(), "vsmain" };
		gpCreateInfo.PS = { mPS.Get(), "psmain" };
		gpCreateInfo.vertexInputState.bindingCount = 1;
		gpCreateInfo.vertexInputState.bindings[0] = mReflector.mesh->GetDerivedVertexBindings()[0];
		gpCreateInfo.topology = vkr::PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		gpCreateInfo.polygonMode = vkr::POLYGON_MODE_FILL;
		gpCreateInfo.cullMode = vkr::CULL_MODE_FRONT;
		gpCreateInfo.frontFace = vkr::FRONT_FACE_CCW;
		gpCreateInfo.depthReadEnable = true;
		gpCreateInfo.depthWriteEnable = true;
		gpCreateInfo.blendModes[0] = vkr::BLEND_MODE_NONE;
		gpCreateInfo.outputState.renderTargetCount = 1;
		gpCreateInfo.outputState.renderTargetFormats[0] = GetRender().GetSwapChain().GetColorFormat();
		gpCreateInfo.outputState.depthStencilFormat = GetRender().GetSwapChain().GetDepthFormat();
		gpCreateInfo.pPipelineInterface = mPipelineInterface;

		CHECKED_CALL(device.CreateGraphicsPipeline(gpCreateInfo, &mSkyBoxPipeline));
	}

	// Reflector pipeline
	{
		CHECKED_CALL(device.CreateShader("basic/shaders", "CubeMap.vs", &mVS));
		CHECKED_CALL(device.CreateShader("basic/shaders", "CubeMap.ps", &mPS));

		vkr::PipelineInterfaceCreateInfo piCreateInfo = {};
		piCreateInfo.setCount = 1;
		piCreateInfo.sets[0].set = 0;
		piCreateInfo.sets[0].pLayout = mDescriptorSetLayout;
		CHECKED_CALL(device.CreatePipelineInterface(piCreateInfo, &mPipelineInterface));

		vkr::GraphicsPipelineCreateInfo2 gpCreateInfo = {};
		gpCreateInfo.VS = { mVS.Get(), "vsmain" };
		gpCreateInfo.PS = { mPS.Get(), "psmain" };
		gpCreateInfo.vertexInputState.bindingCount = 1;
		gpCreateInfo.vertexInputState.bindings[0] = mReflector.mesh->GetDerivedVertexBindings()[0];
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
		gpCreateInfo.pPipelineInterface = mPipelineInterface;

		CHECKED_CALL(device.CreateGraphicsPipeline(gpCreateInfo, &mReflectorPipeline));
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

#if defined(ENABLE_GPU_QUERIES)
		// Timestamp query
		vkr::QueryCreateInfo queryCreateInfo = {};
		queryCreateInfo.type = vkr::QUERY_TYPE_TIMESTAMP;
		queryCreateInfo.count = 2;
		CHECKED_CALL(device.CreateQuery(queryCreateInfo, &frame.timestampQuery));
#endif // defined(ENABLE_GPU_QUERIES)

		mPerFrame.push_back(frame);
	}

	return true;
}

void Example_010::Shutdown()
{
	mPerFrame.clear();
	mVS.Reset();
	mPS.Reset();
	mPipelineInterface.Reset();

	// TODO: доделать очистку
}

void Example_010::Update()
{
}

void Example_010::Render()
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

#if defined(ENABLE_GPU_QUERIES)
	// Read query results
	//if (GetFrameCount() > 0)
	{
		uint64_t data[2] = { 0 };
		CHECKED_CALL(frame.timestampQuery->GetData(data, 2 * sizeof(uint64_t)));
		mGpuWorkDuration = data[1] - data[0];
	}
	// Reset query
	frame.timestampQuery->Reset(0, 2);
#endif // defined(ENABLE_GPU_QUERIES)

	// Update smoothed rotation
	mRotY += (mTargetRotY - mRotY) * 0.10f;

	// Update uniform buffer
	{
		float3   startEyePos = float3(0, 0, 5);
		float4x4 Reye = glm::rotate(glm::radians(-mRotY), float3(0, 1, 0));
		float3   eyePos = Reye * float4(startEyePos, 0);
		float4x4 P = glm::perspective(glm::radians(60.0f), GetWindowAspect(), 0.001f, 10000.0f);
		float4x4 V = glm::lookAt(eyePos, float3(0, 0, 0), float3(0, 1, 0));
		float4x4 M = glm::translate(float3(0, 0, 0));

		// Sky box
		float4x4 mat = P * V * M;
		mSkyBox.uniformBuffer->CopyFromSource(sizeof(mat), &mat);

		// Reflector
		struct Transform
		{
			float4x4 MVPMatrix;
			float4x4 ModelMatrix;
			float3x3 NormalMatrix;
			float3   EyePos;
		};

		float4x4 T = glm::translate(float3(0, 0, 0));
		float4x4 R = float4x4(1);
		float4x4 S = glm::scale(float3(3, 3, 3));
		M = T * R * S;
		float3x3 N = glm::inverseTranspose(float3x3(M));

		char constantData[MINIMUM_CONSTANT_BUFFER_SIZE] = { 0 };
		// Get offsets to memeber vars
		float4x4* pMVPMatrix = reinterpret_cast<float4x4*>(constantData + 0);
		float4x4* pModelMatrix = reinterpret_cast<float4x4*>(constantData + 64);
		float3* pNormalMatrixR0 = reinterpret_cast<float3*>(constantData + 128);
		float3* pNormalMatrixR1 = reinterpret_cast<float3*>(constantData + 144);
		float3* pNormalMatrixR2 = reinterpret_cast<float3*>(constantData + 160);
		float3* pEyePos = reinterpret_cast<float3*>(constantData + 176);
		// Assign values
		*pMVPMatrix = P * V * M;
		*pModelMatrix = M;
		*pNormalMatrixR0 = N[0];
		*pNormalMatrixR1 = N[1];
		*pNormalMatrixR2 = N[2];
		*pEyePos = eyePos;
		mReflector.uniformBuffer->CopyFromSource(sizeof(constantData), constantData);
	}

	// Build command buffer
	CHECKED_CALL(frame.cmd->Begin());
	{
#if defined(ENABLE_GPU_QUERIES)
		// Write start timestamp
		frame.cmd->WriteTimestamp(frame.timestampQuery, vkr::PIPELINE_STAGE_TOP_OF_PIPE_BIT, 0);
#endif // defined(ENABLE_GPU_QUERIES)

		vkr::RenderPassPtr renderPass = swapChain.GetRenderPass(imageIndex);
		ASSERT_MSG(!renderPass.IsNull(), "render pass object is null");

		vkr::RenderPassBeginInfo beginInfo = {};
		beginInfo.pRenderPass = renderPass;
		beginInfo.renderArea = renderPass->GetRenderArea();
		beginInfo.RTVClearCount = 1;
		beginInfo.RTVClearValues[0] = { {0, 0, 0, 0} };
		beginInfo.DSVClearValue = { 1.0f, 0xFF };

		frame.cmd->TransitionImageLayout(renderPass->GetRenderTargetImage(0), ALL_SUBRESOURCES, vkr::RESOURCE_STATE_PRESENT, vkr::RESOURCE_STATE_RENDER_TARGET);
		frame.cmd->BeginRenderPass(&beginInfo);
		{
			frame.cmd->SetScissors(render.GetScissor());
			frame.cmd->SetViewports(render.GetViewport());

			// Draw reflector
			frame.cmd->BindGraphicsPipeline(mReflectorPipeline);
			frame.cmd->BindGraphicsDescriptorSets(mPipelineInterface, 1, &mReflector.descriptorSet);
			frame.cmd->BindIndexBuffer(mReflector.mesh);
			frame.cmd->BindVertexBuffers(mReflector.mesh);
			frame.cmd->DrawIndexed(mReflector.mesh->GetIndexCount());

			// Draw sky box
			frame.cmd->BindGraphicsPipeline(mSkyBoxPipeline);
			frame.cmd->BindGraphicsDescriptorSets(mPipelineInterface, 1, &mSkyBox.descriptorSet);
			frame.cmd->BindIndexBuffer(mSkyBox.mesh);
			frame.cmd->BindVertexBuffers(mSkyBox.mesh);
			frame.cmd->DrawIndexed(mSkyBox.mesh->GetIndexCount());

			// Draw ImGui
			render.DrawDebugInfo();
			render.DrawImGui(frame.cmd);
		}
		frame.cmd->EndRenderPass();
		frame.cmd->TransitionImageLayout(renderPass->GetRenderTargetImage(0), ALL_SUBRESOURCES, vkr::RESOURCE_STATE_RENDER_TARGET, vkr::RESOURCE_STATE_PRESENT);

#if defined(ENABLE_GPU_QUERIES)
		// Write end timestamp
		frame.cmd->WriteTimestamp(frame.timestampQuery, vkr::PIPELINE_STAGE_TOP_OF_PIPE_BIT, 1);

		// Resolve queries
		frame.cmd->ResolveQueryData(frame.timestampQuery, 0, 2);
#endif // defined(ENABLE_GPU_QUERIES)
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

void Example_010::MouseMove(int32_t x, int32_t y, int32_t dx, int32_t dy, MouseButton buttons)
{
	if (buttons == MouseButton::Left)
	{
		mTargetRotY += 0.25f * dx;
	}
}

void Example_010::setupEntity(const vkr::TriMesh& mesh, const vkr::GeometryCreateInfo& createInfo, Entity* pEntity)
{
	vkr::Geometry geo;
	CHECKED_CALL(vkr::Geometry::Create(createInfo, mesh, &geo));
	CHECKED_CALL(vkr::grfx_util::CreateMeshFromGeometry(GetRenderDevice().GetGraphicsQueue(), &geo, &pEntity->mesh));

	vkr::BufferCreateInfo bufferCreateInfo = {};
	bufferCreateInfo.size = MINIMUM_UNIFORM_BUFFER_SIZE;
	bufferCreateInfo.usageFlags.bits.uniformBuffer = true;
	bufferCreateInfo.memoryUsage = vkr::MEMORY_USAGE_CPU_TO_GPU;
	CHECKED_CALL(GetRenderDevice().CreateBuffer(bufferCreateInfo, &pEntity->uniformBuffer));

	CHECKED_CALL(GetRenderDevice().AllocateDescriptorSet(mDescriptorPool, mDescriptorSetLayout, &pEntity->descriptorSet));

	vkr::WriteDescriptor write = {};
	write.binding = 0;
	write.type = vkr::DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	write.bufferOffset = 0;
	write.bufferRange = WHOLE_SIZE;
	write.pBuffer = pEntity->uniformBuffer;
	CHECKED_CALL(pEntity->descriptorSet->UpdateDescriptors(1, &write));

	write = {};
	write.binding = 1;
	write.type = vkr::DESCRIPTOR_TYPE_SAMPLED_IMAGE;
	write.pImageView = mCubeMapImageView;
	CHECKED_CALL(pEntity->descriptorSet->UpdateDescriptors(1, &write));

	write = {};
	write.binding = 2;
	write.type = vkr::DESCRIPTOR_TYPE_SAMPLER;
	write.pSampler = mCubeMapSampler;
	CHECKED_CALL(pEntity->descriptorSet->UpdateDescriptors(1, &write));
}