#include "stdafx.h"
#include "028_Primitives.h"

EngineApplicationCreateInfo Example_028::Config() const
{
	EngineApplicationCreateInfo createInfo{};
	createInfo.render.swapChain.depthFormat = vkr::Format::D32_FLOAT;
	return createInfo;
}

bool Example_028::Setup()
{
	auto& device = GetRenderDevice();

	// Descriptor stuff
	{
		vkr::DescriptorPoolCreateInfo poolCreateInfo = {};
		poolCreateInfo.uniformBuffer = 6;
		CHECKED_CALL(device.CreateDescriptorPool(poolCreateInfo, &mDescriptorPool));

		vkr::DescriptorSetLayoutCreateInfo layoutCreateInfo = {};
		layoutCreateInfo.bindings.push_back(vkr::DescriptorBinding{ 0, vkr::DescriptorType::UniformBuffer, 1, vkr::SHADER_STAGE_ALL_GRAPHICS });
		CHECKED_CALL(device.CreateDescriptorSetLayout(layoutCreateInfo, &mDescriptorSetLayout));
	}

	// Entities
	{
		vkr::GeometryCreateInfo geometryCreateInfo = vkr::GeometryCreateInfo::Planar().AddColor();
		vkr::TriMeshOptions     triMeshOptions = vkr::TriMeshOptions().Indices().VertexColors();
		vkr::WireMeshOptions    wireMeshOptions = vkr::WireMeshOptions().Indices().VertexColors();

		vkr::TriMesh triMesh = vkr::TriMesh::CreateCube(float3(2, 2, 2), triMeshOptions);
		setupEntity(triMesh, geometryCreateInfo, &mCube);

		triMesh = vkr::TriMesh::CreateSphere(1.0f, 16, 8, triMeshOptions);
		setupEntity(triMesh, geometryCreateInfo, &mSphere);

		triMesh = vkr::TriMesh::CreatePlane(vkr::TRI_MESH_PLANE_POSITIVE_Y, float2(2, 2), 1, 1, triMeshOptions);
		setupEntity(triMesh, geometryCreateInfo, &mPlane);

		vkr::WireMesh wireMesh = vkr::WireMesh::CreateCube(float3(2, 2, 2), wireMeshOptions);
		setupEntity(wireMesh, geometryCreateInfo, &mWireCube);

		wireMesh = vkr::WireMesh::CreateSphere(1.0f, 16, 8, wireMeshOptions);
		setupEntity(wireMesh, geometryCreateInfo, &mWireSphere);

		wireMesh = vkr::WireMesh::CreatePlane(vkr::WIRE_MESH_PLANE_POSITIVE_Y, float2(2, 2), 4, 4, wireMeshOptions);
		setupEntity(wireMesh, geometryCreateInfo, &mWirePlane);
	}

	// Pipelines
	{
		CHECKED_CALL(device.CreateShader("basic/shaders", "VertexColors.vs", &mVS));
		CHECKED_CALL(device.CreateShader("basic/shaders", "VertexColors.ps", &mPS));

		vkr::PipelineInterfaceCreateInfo piCreateInfo = {};
		piCreateInfo.setCount = 1;
		piCreateInfo.sets[0].set = 0;
		piCreateInfo.sets[0].layout = mDescriptorSetLayout;
		CHECKED_CALL(device.CreatePipelineInterface(piCreateInfo, &mPipelineInterface));

		vkr::GraphicsPipelineCreateInfo2 gpCreateInfo = {};
		gpCreateInfo.VS = { mVS.Get(), "vsmain" };
		gpCreateInfo.PS = { mPS.Get(), "psmain" };
		gpCreateInfo.vertexInputState.bindingCount = 2;
		gpCreateInfo.vertexInputState.bindings[0] = mSphere.mesh->GetDerivedVertexBindings()[0];
		gpCreateInfo.vertexInputState.bindings[1] = mSphere.mesh->GetDerivedVertexBindings()[1];
		gpCreateInfo.topology = vkr::PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		gpCreateInfo.polygonMode = vkr::POLYGON_MODE_FILL;
		gpCreateInfo.cullMode = vkr::CULL_MODE_NONE;
		gpCreateInfo.frontFace = vkr::FRONT_FACE_CCW;
		gpCreateInfo.depthReadEnable = true;
		gpCreateInfo.depthWriteEnable = true;
		gpCreateInfo.blendModes[0] = vkr::BLEND_MODE_NONE;
		gpCreateInfo.outputState.renderTargetCount = 1;
		gpCreateInfo.outputState.renderTargetFormats[0] = GetRender().GetSwapChain().GetColorFormat();
		gpCreateInfo.outputState.depthStencilFormat = GetRender().GetSwapChain().GetDepthFormat();
		gpCreateInfo.pipelineInterface = mPipelineInterface;

		// Triange pipeline
		CHECKED_CALL(device.CreateGraphicsPipeline(gpCreateInfo, &mTrianglePipeline));

		// Wire pipeline
		gpCreateInfo.topology = vkr::PRIMITIVE_TOPOLOGY_LINE_LIST;
		CHECKED_CALL(device.CreateGraphicsPipeline(gpCreateInfo, &mWirePipeline));		
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

void Example_028::Shutdown()
{
	mPerFrame.clear();
	mVS.Reset();
	mPS.Reset();
	mPipelineInterface.Reset();

	// TODO: доделать очистку
}

void Example_028::Update()
{
}

void Example_028::Render()
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

		float4x4 T = glm::translate(float3(-4, 2, 0));
		float4x4 mat = P * V * T * M;
		mCube.uniformBuffer->CopyFromSource(sizeof(mat), &mat);

		T = glm::translate(float3(0, 2, 0));
		mat = P * V * T * M;
		mSphere.uniformBuffer->CopyFromSource(sizeof(mat), &mat);

		T = glm::translate(float3(4, 2, 0));
		mat = P * V * T * M;
		mPlane.uniformBuffer->CopyFromSource(sizeof(mat), &mat);

		T = glm::translate(float3(-4, -2, 0));
		mat = P * V * T * M;
		mWireCube.uniformBuffer->CopyFromSource(sizeof(mat), &mat);

		T = glm::translate(float3(0, -2, 0));
		mat = P * V * T * M;
		mWireSphere.uniformBuffer->CopyFromSource(sizeof(mat), &mat);

		T = glm::translate(float3(4, -2, 0));
		mat = P * V * T * M;
		mWirePlane.uniformBuffer->CopyFromSource(sizeof(mat), &mat);
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
		beginInfo.RTVClearValues[0] = { 0, 0, 0, 0 };
		beginInfo.DSVClearValue = { 1.0f, 0xFF };

		frame.cmd->TransitionImageLayout(renderPass->GetRenderTargetImage(0), ALL_SUBRESOURCES, vkr::ResourceState::Present, vkr::ResourceState::RenderTarget);
		frame.cmd->BeginRenderPass(&beginInfo);
		{
			frame.cmd->SetScissors(render.GetScissor());
			frame.cmd->SetViewports(render.GetViewport());

			// Triangle pipeline
			frame.cmd->BindGraphicsPipeline(mTrianglePipeline);

			// Cube
			frame.cmd->BindGraphicsDescriptorSets(mPipelineInterface, 1, &mCube.descriptorSet);
			frame.cmd->BindIndexBuffer(mCube.mesh);
			frame.cmd->BindVertexBuffers(mCube.mesh);
			frame.cmd->DrawIndexed(mCube.mesh->GetIndexCount());

			// Sphere
			frame.cmd->BindGraphicsDescriptorSets(mPipelineInterface, 1, &mSphere.descriptorSet);
			frame.cmd->BindIndexBuffer(mSphere.mesh);
			frame.cmd->BindVertexBuffers(mSphere.mesh);
			frame.cmd->DrawIndexed(mSphere.mesh->GetIndexCount());

			// Plane
			frame.cmd->BindGraphicsDescriptorSets(mPipelineInterface, 1, &mPlane.descriptorSet);
			frame.cmd->BindIndexBuffer(mPlane.mesh);
			frame.cmd->BindVertexBuffers(mPlane.mesh);
			frame.cmd->DrawIndexed(mPlane.mesh->GetIndexCount());

			// -------------------------------------------------------------------------------------
			//
			// Wire pipeline
			frame.cmd->BindGraphicsPipeline(mWirePipeline);

			// Wire cube
			frame.cmd->BindGraphicsDescriptorSets(mPipelineInterface, 1, &mWireCube.descriptorSet);
			frame.cmd->BindIndexBuffer(mWireCube.mesh);
			frame.cmd->BindVertexBuffers(mWireCube.mesh);
			frame.cmd->DrawIndexed(mWireCube.mesh->GetIndexCount());

			// Wire sphere
			frame.cmd->BindGraphicsDescriptorSets(mPipelineInterface, 1, &mWireSphere.descriptorSet);
			frame.cmd->BindIndexBuffer(mWireSphere.mesh);
			frame.cmd->BindVertexBuffers(mWireSphere.mesh);
			frame.cmd->DrawIndexed(mWireSphere.mesh->GetIndexCount());

			// Wire plane
			frame.cmd->BindGraphicsDescriptorSets(mPipelineInterface, 1, &mWirePlane.descriptorSet);
			frame.cmd->BindIndexBuffer(mWirePlane.mesh);
			frame.cmd->BindVertexBuffers(mWirePlane.mesh);
			frame.cmd->DrawIndexed(mWirePlane.mesh->GetIndexCount());

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

void Example_028::setupEntity(const vkr::TriMesh& mesh, const vkr::GeometryCreateInfo& createInfo, Entity* pEntity)
{
	CHECKED_CALL(vkr::vkrUtil::CreateMeshFromTriMesh(GetRenderDevice().GetGraphicsQueue(), &mesh, &pEntity->mesh));

	vkr::BufferCreateInfo bufferCreateInfo = {};
	bufferCreateInfo.size = vkr::MINIMUM_UNIFORM_BUFFER_SIZE;
	bufferCreateInfo.usageFlags.bits.uniformBuffer = true;
	bufferCreateInfo.memoryUsage = vkr::MemoryUsage::CPUToGPU;
	CHECKED_CALL(GetRenderDevice().CreateBuffer(bufferCreateInfo, &pEntity->uniformBuffer));

	CHECKED_CALL(GetRenderDevice().AllocateDescriptorSet(mDescriptorPool, mDescriptorSetLayout, &pEntity->descriptorSet));

	vkr::WriteDescriptor write = {};
	write.binding = 0;
	write.type = vkr::DescriptorType::UniformBuffer;
	write.bufferOffset = 0;
	write.bufferRange = WHOLE_SIZE;
	write.buffer = pEntity->uniformBuffer;
	CHECKED_CALL(pEntity->descriptorSet->UpdateDescriptors(1, &write));
}

void Example_028::setupEntity(const vkr::WireMesh& mesh, const vkr::GeometryCreateInfo& createInfo, Entity* pEntity)
{
	CHECKED_CALL(vkr::vkrUtil::CreateMeshFromWireMesh(GetRenderDevice().GetGraphicsQueue(), &mesh, &pEntity->mesh));

	vkr::BufferCreateInfo bufferCreateInfo = {};
	bufferCreateInfo.size = vkr::MINIMUM_UNIFORM_BUFFER_SIZE;
	bufferCreateInfo.usageFlags.bits.uniformBuffer = true;
	bufferCreateInfo.memoryUsage = vkr::MemoryUsage::CPUToGPU;
	CHECKED_CALL(GetRenderDevice().CreateBuffer(bufferCreateInfo, &pEntity->uniformBuffer));

	CHECKED_CALL(GetRenderDevice().AllocateDescriptorSet(mDescriptorPool, mDescriptorSetLayout, &pEntity->descriptorSet));

	vkr::WriteDescriptor write = {};
	write.binding = 0;
	write.type = vkr::DescriptorType::UniformBuffer;
	write.bufferOffset = 0;
	write.bufferRange = WHOLE_SIZE;
	write.buffer = pEntity->uniformBuffer;
	CHECKED_CALL(pEntity->descriptorSet->UpdateDescriptors(1, &write));
}