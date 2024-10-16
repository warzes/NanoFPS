#include "stdafx.h"
#include "018_DynamicRendering.h"

EngineApplicationCreateInfo Example_018::Config() const
{
	EngineApplicationCreateInfo createInfo{};
	createInfo.render.swapChain.depthFormat = vkr::Format::D32_FLOAT;
	createInfo.render.showImgui = true;
	createInfo.render.enableImGuiDynamicRendering = true;
	return createInfo;
}

bool Example_018::Setup()
{
	auto& device = GetRenderDevice();

	// Vertex buffer and geometry data
	{
		vkr::GeometryCreateInfo geometryCreateInfo = vkr::GeometryCreateInfo::Planar().AddColor();
		vkr::TriMeshOptions     triMeshOptions = vkr::TriMeshOptions().Indices().VertexColors();
		vkr::TriMesh            sphereTriMesh = vkr::TriMesh::CreateSphere(/* radius */ 1.0f, 16, 8, triMeshOptions);
		CHECKED_CALL(vkr::vkrUtil::CreateMeshFromTriMesh(device.GetGraphicsQueue(), &sphereTriMesh, &mSphereMesh));
	}

	// Pipeline
	{
		vkr::ShaderModulePtr vs;
		CHECKED_CALL(device.CreateShader("basic/shaders", "VertexColorsPushConstants.vs", &vs));

		vkr::ShaderModulePtr ps;
		CHECKED_CALL(device.CreateShader("basic/shaders", "VertexColorsPushConstants.ps", &ps));

		vkr::PipelineInterfaceCreateInfo piCreateInfo = {};
		piCreateInfo.setCount = 0;
		piCreateInfo.pushConstants.count = (sizeof(float4x4) + sizeof(uint32_t)) / sizeof(uint32_t);
		piCreateInfo.pushConstants.binding = 0;
		piCreateInfo.pushConstants.set = 0;
		CHECKED_CALL(device.CreatePipelineInterface(piCreateInfo, &mPipelineInterface));

		vkr::GraphicsPipelineCreateInfo2 gpCreateInfo = {};
		gpCreateInfo.dynamicRenderPass = true;
		gpCreateInfo.VS = { vs.Get(), "vsmain" };
		gpCreateInfo.PS = { ps.Get(), "psmain" };
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
		gpCreateInfo.pipelineInterface = mPipelineInterface;

		const std::vector<vkr::VertexBinding> bindings = mSphereMesh->GetDerivedVertexBindings();
		gpCreateInfo.vertexInputState.bindingCount = static_cast<uint32_t>(bindings.size());
		for (size_t i = 0; i < bindings.size(); i++) {
			gpCreateInfo.vertexInputState.bindings[i] = bindings[i];
		}

		CHECKED_CALL(device.CreateGraphicsPipeline(gpCreateInfo, &mPipeline));

		device.DestroyShaderModule(vs);
		device.DestroyShaderModule(ps);
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

	for (uint32_t imageIndex = 0; imageIndex < GetRender().GetSwapChain().GetImageCount(); imageIndex++)
	{
		vkr::CommandBufferPtr preRecordedCmd;
		CHECKED_CALL(device.GetGraphicsQueue()->CreateCommandBuffer(&preRecordedCmd));
		mPreRecordedCmds.push_back(preRecordedCmd);

		CHECKED_CALL(preRecordedCmd->Begin());
		{
			preRecordedCmd->TransitionImageLayout(GetRender().GetSwapChain().GetColorImage(imageIndex), ALL_SUBRESOURCES, vkr::ResourceState::Present, vkr::ResourceState::RenderTarget);
			vkr::RenderingInfo renderingInfo = {};
			renderingInfo.flags.bits.suspending = true;
			renderingInfo.renderArea = { 0, 0, GetRender().GetSwapChain().GetWidth(), GetRender().GetSwapChain().GetHeight() };
			renderingInfo.renderTargetCount = 1;
			// There will be an explicit clear inside a renderpass.
			renderingInfo.pRenderTargetViews[0] = GetRender().GetSwapChain().GetRenderTargetView(imageIndex, vkr::AttachmentLoadOp::Load);
			renderingInfo.pDepthStencilView = GetRender().GetSwapChain().GetDepthStencilView(imageIndex);

			float4x4 P = glm::perspective(glm::radians(60.0f), GetWindowAspect(), 0.001f, 10000.0f);
			float4x4 V = glm::lookAt(float3(0, 0, 5), float3(0, 0, 0), float3(0, 1, 0));
			float4x4 M = glm::translate(float3(0.0, 0.0, -2.0)) * glm::scale(float3(2.0, 2.0, 2.0));
			float4x4 mat = P * V * M;

			preRecordedCmd->BeginRendering(&renderingInfo);
			{
				float4 rtvClearValue = { 0.7f, 0.7f, 0.7f, 1.0f };
				vkr::DepthStencilClearValue dsvClearValue = { 1.0f, 0xFF };
				preRecordedCmd->ClearRenderTarget(GetRender().GetSwapChain().GetColorImage(imageIndex), rtvClearValue);
				preRecordedCmd->ClearDepthStencil(dsvClearValue, vkr::CLEAR_FLAG_DEPTH);
				preRecordedCmd->SetScissors(GetRender().GetScissor());
				preRecordedCmd->SetViewports(GetRender().GetViewport());
				preRecordedCmd->PushGraphicsConstants(mPipelineInterface, 16, &mat);
				preRecordedCmd->BindGraphicsPipeline(mPipeline);
				preRecordedCmd->BindIndexBuffer(mSphereMesh);
				preRecordedCmd->BindVertexBuffers(mSphereMesh);
				preRecordedCmd->DrawIndexed(mSphereMesh->GetIndexCount());
			}
			preRecordedCmd->EndRendering();
		}
		CHECKED_CALL(preRecordedCmd->End());
	}

	return true;
}

void Example_018::Shutdown()
{
	mPerFrame.clear();
	// TODO: �������� �������
}

void Example_018::Update()
{
}

void Example_018::Render()
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

	CHECKED_CALL(frame.cmd->Begin());
	{
		vkr::RenderingInfo renderingInfo = {};
		renderingInfo.flags.bits.resuming = true;
		renderingInfo.renderArea = { 0, 0, swapChain.GetWidth(), swapChain.GetHeight() };
		renderingInfo.renderTargetCount = 1;
		renderingInfo.pRenderTargetViews[0] = swapChain.GetRenderTargetView(imageIndex, vkr::AttachmentLoadOp::Load);
		renderingInfo.pDepthStencilView = swapChain.GetDepthStencilView(imageIndex);

		//float t = GetElapsedSeconds();
		static float t = 0.0;
		t += 0.001f;

		float4x4 P = glm::perspective(glm::radians(60.0f), GetWindowAspect(), 0.001f, 10000.0f);
		float4x4 V = glm::lookAt(float3(0, 0, 5), float3(0, 0, 0), float3(0, 1, 0));
		float4x4 M = glm::rotate(t, float3(0, 1, 0)) * glm::translate(float3(0.0, 0.0, -3.0)) * glm::scale(float3(0.5, 0.5, 0.5));
		float4x4 mat = P * V * M;

		frame.cmd->BeginRendering(&renderingInfo);
		{
			frame.cmd->SetScissors(render.GetScissor());
			frame.cmd->SetViewports(render.GetViewport());
			frame.cmd->PushGraphicsConstants(mPipelineInterface, 16, &mat);
			frame.cmd->BindGraphicsPipeline(mPipeline);
			frame.cmd->BindIndexBuffer(mSphereMesh);
			frame.cmd->BindVertexBuffers(mSphereMesh);
			frame.cmd->DrawIndexed(mSphereMesh->GetIndexCount());
		}
		frame.cmd->EndRendering();

		{
			// ImGui expects the rendering info without depth attachment.
			renderingInfo.pDepthStencilView = nullptr;

			frame.cmd->BeginRendering(&renderingInfo);
			{
				// Draw ImGui
				render.DrawDebugInfo();
				render.DrawImGui(frame.cmd);
			}
			frame.cmd->EndRendering();
		}

		frame.cmd->TransitionImageLayout(swapChain.GetColorImage(imageIndex), ALL_SUBRESOURCES, vkr::ResourceState::RenderTarget, vkr::ResourceState::Present);
	}
	CHECKED_CALL(frame.cmd->End());

	const vkr::CommandBuffer* commands[2] = { mPreRecordedCmds[imageIndex], frame.cmd };

	vkr::SubmitInfo submitInfo = {};
	submitInfo.commandBufferCount = 2;
	submitInfo.ppCommandBuffers = commands;
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.ppWaitSemaphores = &frame.imageAcquiredSemaphore;
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.ppSignalSemaphores = &frame.renderCompleteSemaphore;
	submitInfo.pFence = frame.renderCompleteFence;

	CHECKED_CALL(render.GetGraphicsQueue()->Submit(&submitInfo));

	CHECKED_CALL(swapChain.Present(imageIndex, 1, &frame.renderCompleteSemaphore));
}