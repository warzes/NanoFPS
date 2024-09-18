#include "stdafx.h"
#include "008_BasicGeometry.h"

EngineApplicationCreateInfo Example_008::Config() const
{
	EngineApplicationCreateInfo createInfo{};
	createInfo.render.swapChain.depthFormat = FORMAT_D32_FLOAT;
	return createInfo;
}

bool Example_008::Setup()
{
	auto& device = GetRenderDevice();

	// Descriptor stuff
	{
		DescriptorPoolCreateInfo poolCreateInfo = {};
		// For the 9 entities created below.
		poolCreateInfo.uniformBuffer = 9;
		CHECKED_CALL(device.CreateDescriptorPool(poolCreateInfo, &mDescriptorPool));

		DescriptorSetLayoutCreateInfo layoutCreateInfo = {};
		layoutCreateInfo.bindings.push_back(DescriptorBinding{ 0, DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, SHADER_STAGE_ALL_GRAPHICS });
		CHECKED_CALL(device.CreateDescriptorSetLayout(layoutCreateInfo, &mDescriptorSetLayout));
	}

	// Entities
	{
		TriMesh mesh = TriMesh::CreateCube(float3(2, 2, 2), TriMeshOptions().VertexColors().Normals());
		// 9 total entities. Each uses a descriptor with a uniform buffer allocated from the descriptor pool created above.
		setupEntity(mesh, GeometryCreateInfo::InterleavedU16().AddColor().AddNormal(), &mInterleavedU16);
		setupEntity(mesh, GeometryCreateInfo::InterleavedU32().AddColor().AddNormal(), &mInterleavedU32);
		setupEntity(mesh, GeometryCreateInfo::Interleaved().AddColor().AddNormal(), &mInterleaved);
		setupEntity(mesh, GeometryCreateInfo::PlanarU16().AddColor().AddNormal(), &mPlanarU16);
		setupEntity(mesh, GeometryCreateInfo::PlanarU32().AddColor().AddNormal(), &mPlanarU32);
		setupEntity(mesh, GeometryCreateInfo::Planar().AddColor().AddNormal(), &mPlanar);
		setupEntity(mesh, GeometryCreateInfo::PositionPlanarU16().AddColor().AddNormal(), &mPositionPlanarU16);
		setupEntity(mesh, GeometryCreateInfo::PositionPlanarU32().AddColor().AddNormal(), &mPositionPlanarU32);
		setupEntity(mesh, GeometryCreateInfo::PositionPlanar().AddColor().AddNormal(), &mPositionPlanar);
	}

	// Pipelines
	{
		CHECKED_CALL(device.CreateShader("basic/shaders", "VertexLayoutTest.vs", &mVS));
		CHECKED_CALL(device.CreateShader("basic/shaders", "VertexLayoutTest.ps", &mPS));

		PipelineInterfaceCreateInfo piCreateInfo = {};
		piCreateInfo.setCount = 1;
		piCreateInfo.sets[0].set = 0;
		piCreateInfo.sets[0].pLayout = mDescriptorSetLayout;
		CHECKED_CALL(device.CreatePipelineInterface(piCreateInfo, &mPipelineInterface));

		// -----------------------------------------------------------------------------------------
		// Interleaved pipeline

		auto interleavedVertexBindings = mInterleavedU16.mesh->GetDerivedVertexBindings();

		GraphicsPipelineCreateInfo2 gpCreateInfo = {};
		gpCreateInfo.VS = { mVS.Get(), "vsmain" };
		gpCreateInfo.PS = { mPS.Get(), "psmain" };
		gpCreateInfo.vertexInputState.bindingCount = 1;
		gpCreateInfo.vertexInputState.bindings[0] = interleavedVertexBindings[0];
		gpCreateInfo.topology = PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		gpCreateInfo.polygonMode = POLYGON_MODE_FILL;
		gpCreateInfo.cullMode = CULL_MODE_BACK;
		gpCreateInfo.frontFace = FRONT_FACE_CCW;
		gpCreateInfo.depthReadEnable = true;
		gpCreateInfo.depthWriteEnable = true;
		gpCreateInfo.blendModes[0] = BLEND_MODE_NONE;
		gpCreateInfo.outputState.renderTargetCount = 1;
		gpCreateInfo.outputState.renderTargetFormats[0] = GetRender().GetSwapChain().GetColorFormat();
		gpCreateInfo.outputState.depthStencilFormat = GetRender().GetSwapChain().GetDepthFormat();
		gpCreateInfo.pPipelineInterface = mPipelineInterface;

		CHECKED_CALL(device.CreateGraphicsPipeline(gpCreateInfo, &mInterleavedPipeline));

		// -----------------------------------------------------------------------------------------
		// Planar pipeline

		auto planarVertexBindings = mPlanarU16.mesh->GetDerivedVertexBindings();

		ASSERT_MSG(mPlanarU16.mesh->GetVertexBufferCount() == 3, "vertex buffer count should be 3: position, color, normal");
		gpCreateInfo.vertexInputState.bindingCount = 3;
		gpCreateInfo.vertexInputState.bindings[0] = planarVertexBindings[0];
		gpCreateInfo.vertexInputState.bindings[1] = planarVertexBindings[1];
		gpCreateInfo.vertexInputState.bindings[2] = planarVertexBindings[2];
		CHECKED_CALL(device.CreateGraphicsPipeline(gpCreateInfo, &mPlanarPipeline));

		// -----------------------------------------------------------------------------------------
		// Planar pipeline

		auto positionPlanarVertexBindings = mPositionPlanarU16.mesh->GetDerivedVertexBindings();

		ASSERT_MSG(mPositionPlanarU16.mesh->GetVertexBufferCount() == 2, "vertex buffer count should be 2: position, non-position");
		gpCreateInfo.vertexInputState.bindingCount = 2;
		gpCreateInfo.vertexInputState.bindings[0] = positionPlanarVertexBindings[0];
		gpCreateInfo.vertexInputState.bindings[1] = positionPlanarVertexBindings[1];
		CHECKED_CALL(device.CreateGraphicsPipeline(gpCreateInfo, &mPositionPlanarPipeline));
	}
	
	// Per frame data
	{
		PerFrame frame = {};

		CHECKED_CALL(device.GetGraphicsQueue()->CreateCommandBuffer(&frame.cmd));

		SemaphoreCreateInfo semaCreateInfo = {};
		CHECKED_CALL(device.CreateSemaphore(semaCreateInfo, &frame.imageAcquiredSemaphore));

		FenceCreateInfo fenceCreateInfo = {};
		CHECKED_CALL(device.CreateFence(fenceCreateInfo, &frame.imageAcquiredFence));

		CHECKED_CALL(device.CreateSemaphore(semaCreateInfo, &frame.renderCompleteSemaphore));

		fenceCreateInfo = { true }; // Create signaled
		CHECKED_CALL(device.CreateFence(fenceCreateInfo, &frame.renderCompleteFence));

		mPerFrame.push_back(frame);
	}

	return true;
}

void Example_008::Shutdown()
{
	mPerFrame.clear();
	mVS.Reset();
	mPS.Reset();
	mPipelineInterface.Reset();

	// TODO: доделать очистку
}

void Example_008::Update()
{
}

void Example_008::Render()
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

	// Update uniform buffer
	{
		//float    t = GetElapsedSeconds();
		static float t = 0.0;
		t += 0.001f;
		float4x4 P = glm::perspective(glm::radians(60.0f), GetWindowAspect(), 0.001f, 10000.0f);
		float4x4 V = glm::lookAt(float3(0, 0, 8), float3(0, 0, 0), float3(0, 1, 0));
		float4x4 M = glm::rotate(t, float3(0, 0, 1)) * glm::rotate(2 * t, float3(0, 1, 0)) * glm::rotate(t, float3(1, 0, 0));

		// Top 3 cubes are interleaved
		float4x4 T = glm::translate(float3(-4, 2, 0));
		float4x4 mat = P * V * T * M;
		mInterleavedU16.uniformBuffer->CopyFromSource(sizeof(mat), &mat);

		T = glm::translate(float3(0, 2, 0));
		mat = P * V * T * M;
		mInterleavedU32.uniformBuffer->CopyFromSource(sizeof(mat), &mat);

		T = glm::translate(float3(4, 2, 0));
		mat = P * V * T * M;
		mInterleaved.uniformBuffer->CopyFromSource(sizeof(mat), &mat);

		// Middle 3 cubes are planar
		T = glm::translate(float3(-4, 0, 0));
		mat = P * V * T * M;
		mPlanarU16.uniformBuffer->CopyFromSource(sizeof(mat), &mat);

		T = glm::translate(float3(0, 0, 0));
		mat = P * V * T * M;
		mPlanarU32.uniformBuffer->CopyFromSource(sizeof(mat), &mat);

		T = glm::translate(float3(4, 0, 0));
		mat = P * V * T * M;
		mPlanar.uniformBuffer->CopyFromSource(sizeof(mat), &mat);

		// Bottom 3 cubes are position planar
		T = glm::translate(float3(-4, -2, 0));
		mat = P * V * T * M;
		mPositionPlanarU16.uniformBuffer->CopyFromSource(sizeof(mat), &mat);

		T = glm::translate(float3(0, -2, 0));
		mat = P * V * T * M;
		mPositionPlanarU32.uniformBuffer->CopyFromSource(sizeof(mat), &mat);

		T = glm::translate(float3(4, -2, 0));
		mat = P * V * T * M;
		mPositionPlanar.uniformBuffer->CopyFromSource(sizeof(mat), &mat);
	}

	// Build command buffer
	CHECKED_CALL(frame.cmd->Begin());
	{
		RenderPassPtr renderPass = swapChain.GetRenderPass(imageIndex);
		ASSERT_MSG(!renderPass.IsNull(), "render pass object is null");

		RenderPassBeginInfo beginInfo = {};
		beginInfo.pRenderPass = renderPass;
		beginInfo.renderArea = renderPass->GetRenderArea();
		beginInfo.RTVClearCount = 1;
		beginInfo.RTVClearValues[0] = { {0, 0, 0, 0} };
		beginInfo.DSVClearValue = { 1.0f, 0xFF };

		frame.cmd->TransitionImageLayout(renderPass->GetRenderTargetImage(0), ALL_SUBRESOURCES, RESOURCE_STATE_PRESENT, RESOURCE_STATE_RENDER_TARGET);
		frame.cmd->BeginRenderPass(&beginInfo);
		{
			frame.cmd->SetScissors(render.GetScissor());
			frame.cmd->SetViewports(render.GetViewport());

			// Interleaved pipeline
			frame.cmd->BindGraphicsPipeline(mInterleavedPipeline);

			// Interleaved U16
			frame.cmd->BindGraphicsDescriptorSets(mPipelineInterface, 1, &mInterleavedU16.descriptorSet);
			frame.cmd->BindIndexBuffer(mInterleavedU16.mesh);
			frame.cmd->BindVertexBuffers(mInterleavedU16.mesh);
			frame.cmd->DrawIndexed(mInterleavedU16.mesh->GetIndexCount());

			// Interleaved U32
			frame.cmd->BindGraphicsDescriptorSets(mPipelineInterface, 1, &mInterleavedU32.descriptorSet);
			frame.cmd->BindIndexBuffer(mInterleavedU32.mesh);
			frame.cmd->BindVertexBuffers(mInterleavedU32.mesh);
			frame.cmd->DrawIndexed(mInterleavedU32.mesh->GetIndexCount());

			// Interleaved
			frame.cmd->BindGraphicsDescriptorSets(mPipelineInterface, 1, &mInterleaved.descriptorSet);
			frame.cmd->BindVertexBuffers(mInterleaved.mesh);
			frame.cmd->Draw(mInterleaved.mesh->GetVertexCount());

			// -------------------------------------------------------------------------------------

			// Planar pipeline
			frame.cmd->BindGraphicsPipeline(mPlanarPipeline);

			// Planar U16
			frame.cmd->BindGraphicsDescriptorSets(mPipelineInterface, 1, &mPlanarU16.descriptorSet);
			frame.cmd->BindIndexBuffer(mPlanarU16.mesh);
			frame.cmd->BindVertexBuffers(mPlanarU16.mesh);
			frame.cmd->DrawIndexed(mPlanarU16.mesh->GetIndexCount());

			// Planar U32
			frame.cmd->BindGraphicsDescriptorSets(mPipelineInterface, 1, &mPlanarU32.descriptorSet);
			frame.cmd->BindIndexBuffer(mPlanarU32.mesh);
			frame.cmd->BindVertexBuffers(mPlanarU32.mesh);
			frame.cmd->DrawIndexed(mPlanarU32.mesh->GetIndexCount());

			// Planar
			frame.cmd->BindGraphicsDescriptorSets(mPipelineInterface, 1, &mPlanar.descriptorSet);
			frame.cmd->BindVertexBuffers(mPlanar.mesh);
			frame.cmd->Draw(mPlanar.mesh->GetVertexCount());

			// -------------------------------------------------------------------------------------

			// Position Planar pipeline
			frame.cmd->BindGraphicsPipeline(mPositionPlanarPipeline);

			// Position Planar U16
			frame.cmd->BindGraphicsDescriptorSets(mPipelineInterface, 1, &mPositionPlanarU16.descriptorSet);
			frame.cmd->BindIndexBuffer(mPositionPlanarU16.mesh);
			frame.cmd->BindVertexBuffers(mPositionPlanarU16.mesh);
			frame.cmd->DrawIndexed(mPositionPlanarU16.mesh->GetIndexCount());

			// Position Planar U32
			frame.cmd->BindGraphicsDescriptorSets(mPipelineInterface, 1, &mPositionPlanarU32.descriptorSet);
			frame.cmd->BindIndexBuffer(mPositionPlanarU32.mesh);
			frame.cmd->BindVertexBuffers(mPositionPlanarU32.mesh);
			frame.cmd->DrawIndexed(mPositionPlanarU32.mesh->GetIndexCount());

			// Position Planar
			frame.cmd->BindGraphicsDescriptorSets(mPipelineInterface, 1, &mPositionPlanar.descriptorSet);
			frame.cmd->BindVertexBuffers(mPositionPlanar.mesh);
			frame.cmd->Draw(mPositionPlanar.mesh->GetVertexCount());

			// Draw ImGui
			render.DrawDebugInfo();
			render.DrawImGui(frame.cmd);
		}
		frame.cmd->EndRenderPass();
		frame.cmd->TransitionImageLayout(renderPass->GetRenderTargetImage(0), ALL_SUBRESOURCES, RESOURCE_STATE_RENDER_TARGET, RESOURCE_STATE_PRESENT);
	}
	CHECKED_CALL(frame.cmd->End());

	SubmitInfo submitInfo = {};
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

void Example_008::setupEntity(const TriMesh& mesh, const GeometryCreateInfo& createInfo, Entity* pEntity)
{
	Geometry geo;
	CHECKED_CALL(Geometry::Create(createInfo, mesh, &geo));
	CHECKED_CALL(grfx_util::CreateMeshFromGeometry(GetRenderDevice().GetGraphicsQueue(), &geo, &pEntity->mesh));

	BufferCreateInfo bufferCreateInfo = {};
	bufferCreateInfo.size = MINIMUM_UNIFORM_BUFFER_SIZE;
	bufferCreateInfo.usageFlags.bits.uniformBuffer = true;
	bufferCreateInfo.memoryUsage = MEMORY_USAGE_CPU_TO_GPU;
	CHECKED_CALL(GetRenderDevice().CreateBuffer(bufferCreateInfo, &pEntity->uniformBuffer));

	CHECKED_CALL(GetRenderDevice().AllocateDescriptorSet(mDescriptorPool, mDescriptorSetLayout, &pEntity->descriptorSet));

	WriteDescriptor write = {};
	write.binding = 0;
	write.type = DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	write.bufferOffset = 0;
	write.bufferRange = WHOLE_SIZE;
	write.pBuffer = pEntity->uniformBuffer;
	CHECKED_CALL(pEntity->descriptorSet->UpdateDescriptors(1, &write));
}