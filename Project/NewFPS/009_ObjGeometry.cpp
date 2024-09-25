#include "stdafx.h"
#include "009_ObjGeometry.h"

EngineApplicationCreateInfo Example_009::Config() const
{
	EngineApplicationCreateInfo createInfo{};
	createInfo.render.swapChain.depthFormat = vkr::FORMAT_D32_FLOAT;
	return createInfo;
}

bool Example_009::Setup()
{
	auto& device = GetRenderDevice();

	// Descriptor stuff
	{
		vkr::DescriptorPoolCreateInfo poolCreateInfo = {};
		poolCreateInfo.uniformBuffer = 6;
		CHECKED_CALL(device.CreateDescriptorPool(poolCreateInfo, &mDescriptorPool));

		vkr::DescriptorSetLayoutCreateInfo layoutCreateInfo = {};
		layoutCreateInfo.bindings.push_back(vkr::DescriptorBinding{ 0, vkr::DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, vkr::SHADER_STAGE_ALL_GRAPHICS });
		CHECKED_CALL(device.CreateDescriptorSetLayout(layoutCreateInfo, &mDescriptorSetLayout));
	}

	// Entities
	{
		vkr::TriMesh mesh = vkr::TriMesh::CreateFromOBJ("basic/models/material_sphere.obj", vkr::TriMeshOptions().VertexColors());
		setupEntity(mesh, vkr::GeometryCreateInfo::InterleavedU32().AddColor(), &mInterleavedU32);
		setupEntity(mesh, vkr::GeometryCreateInfo::Interleaved().AddColor(), &mInterleaved);
		setupEntity(mesh, vkr::GeometryCreateInfo::PlanarU32().AddColor(), &mPlanarU32);
		setupEntity(mesh, vkr::GeometryCreateInfo::Planar().AddColor(), &mPlanar);
	}

	// Pipelines
	{
		CHECKED_CALL(device.CreateShader("basic/shaders", "VertexColors.vs", &mVS));
		CHECKED_CALL(device.CreateShader("basic/shaders", "VertexColors.ps", &mPS));

		vkr::PipelineInterfaceCreateInfo piCreateInfo = {};
		piCreateInfo.setCount = 1;
		piCreateInfo.sets[0].set = 0;
		piCreateInfo.sets[0].pLayout = mDescriptorSetLayout;
		CHECKED_CALL(device.CreatePipelineInterface(piCreateInfo, &mPipelineInterface));

		vkr::GraphicsPipelineCreateInfo2 gpCreateInfo = {};
		gpCreateInfo.VS = { mVS.Get(), "vsmain" };
		gpCreateInfo.PS = { mPS.Get(), "psmain" };
		gpCreateInfo.vertexInputState.bindingCount = 1;
		gpCreateInfo.vertexInputState.bindings[0] = mInterleaved.mesh->GetDerivedVertexBindings()[0];
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

		// Interleaved pipeline
		CHECKED_CALL(device.CreateGraphicsPipeline(gpCreateInfo, &mInterleavedPipeline));

		// Planar pipeline
		gpCreateInfo.vertexInputState.bindingCount = 2;
		gpCreateInfo.vertexInputState.bindings[0] = mPlanar.mesh->GetDerivedVertexBindings()[0];
		gpCreateInfo.vertexInputState.bindings[1] = mPlanar.mesh->GetDerivedVertexBindings()[1];
		CHECKED_CALL(device.CreateGraphicsPipeline(gpCreateInfo, &mPlanarPipeline));
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

void Example_009::Shutdown()
{
	mPerFrame.clear();
	mVS.Reset();
	mPS.Reset();
	mPipelineInterface.Reset();

	// TODO: доделать очистку
}

void Example_009::Update()
{
}

void Example_009::Render()
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
		float4x4 M = glm::rotate(t, float3(0, 0, 1)) * glm::rotate(2 * t, float3(0, 1, 0)) * glm::rotate(t, float3(1, 0, 0)) * glm::scale(float3(2));

		float4x4 T = glm::translate(float3(-3, 2, 0));
		float4x4 mat = P * V * T * M;
		mInterleavedU32.uniformBuffer->CopyFromSource(sizeof(mat), &mat);

		T = glm::translate(float3(3, 2, 0));
		mat = P * V * T * M;
		mInterleaved.uniformBuffer->CopyFromSource(sizeof(mat), &mat);

		T = glm::translate(float3(-3, -2, 0));
		mat = P * V * T * M;
		mPlanarU32.uniformBuffer->CopyFromSource(sizeof(mat), &mat);

		T = glm::translate(float3(3, -2, 0));
		mat = P * V * T * M;
		mPlanar.uniformBuffer->CopyFromSource(sizeof(mat), &mat);
	}

	// Build command buffer
	CHECKED_CALL(frame.cmd->Begin());
	{
		vkr::RenderPassPtr renderPass = swapChain.GetRenderPass(imageIndex);
		ASSERT_MSG(!renderPass.IsNull(), "render pass object is null");

		vkr::RenderPassBeginInfo beginInfo = {};
		beginInfo.pRenderPass = renderPass;
		beginInfo.renderArea = renderPass->GetRenderArea();
		beginInfo.RTVClearCount = 1;
		beginInfo.RTVClearValues[0] = { {0, 0, 0, 0} };
		beginInfo.DSVClearValue = { 1.0f, 0xFF };

		frame.cmd->TransitionImageLayout(renderPass->GetRenderTargetImage(0), ALL_SUBRESOURCES, vkr::ResourceState::Present, vkr::ResourceState::RenderTarget);
		frame.cmd->BeginRenderPass(&beginInfo);
		{
			frame.cmd->SetScissors(render.GetScissor());
			frame.cmd->SetViewports(render.GetViewport());

			// Interleaved pipeline
			frame.cmd->BindGraphicsPipeline(mInterleavedPipeline);

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

			// Planar U32
			frame.cmd->BindGraphicsDescriptorSets(mPipelineInterface, 1, &mPlanarU32.descriptorSet);
			frame.cmd->BindIndexBuffer(mPlanarU32.mesh);
			frame.cmd->BindVertexBuffers(mPlanarU32.mesh);
			frame.cmd->DrawIndexed(mPlanarU32.mesh->GetIndexCount());

			// Planar
			frame.cmd->BindGraphicsDescriptorSets(mPipelineInterface, 1, &mPlanar.descriptorSet);
			frame.cmd->BindVertexBuffers(mPlanar.mesh);
			frame.cmd->Draw(mPlanar.mesh->GetVertexCount());

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

void Example_009::setupEntity(const vkr::TriMesh& mesh, const vkr::GeometryCreateInfo& createInfo, Entity* pEntity)
{
	vkr::Geometry geo;
	CHECKED_CALL(vkr::Geometry::Create(createInfo, mesh, &geo));
	CHECKED_CALL(vkr::vkrUtil::CreateMeshFromGeometry(GetRenderDevice().GetGraphicsQueue(), &geo, &pEntity->mesh));

	vkr::BufferCreateInfo bufferCreateInfo = {};
	bufferCreateInfo.size = vkr::MINIMUM_UNIFORM_BUFFER_SIZE;
	bufferCreateInfo.usageFlags.bits.uniformBuffer = true;
	bufferCreateInfo.memoryUsage = vkr::MemoryUsage::CPUToGPU;
	CHECKED_CALL(GetRenderDevice().CreateBuffer(bufferCreateInfo, &pEntity->uniformBuffer));

	CHECKED_CALL(GetRenderDevice().AllocateDescriptorSet(mDescriptorPool, mDescriptorSetLayout, &pEntity->descriptorSet));

	vkr::WriteDescriptor write = {};
	write.binding = 0;
	write.type = vkr::DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	write.bufferOffset = 0;
	write.bufferRange = WHOLE_SIZE;
	write.pBuffer = pEntity->uniformBuffer;
	CHECKED_CALL(pEntity->descriptorSet->UpdateDescriptors(1, &write));
}