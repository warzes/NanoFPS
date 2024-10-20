#include "stdafx.h"
#include "026_Text.h"

EngineApplicationCreateInfo Example_026::Config() const
{
	EngineApplicationCreateInfo createInfo{};
	return createInfo;
}

bool Example_026::Setup()
{
	auto& device = GetRenderDevice();

	mCamera = PerspectiveCamera(GetWindowWidth(), GetWindowHeight());

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

	// Texture font
	{
		Font font;
		CHECKED_CALL(Font::CreateFromFile("basic/fonts/Roboto/Roboto-Regular.ttf", &font));

		vkr::TextureFontCreateInfo createInfo = {};
		createInfo.font = font;
		createInfo.size = 48.0f;
		createInfo.characters = vkr::TextureFont::GetDefaultCharacters();

		CHECKED_CALL(device.CreateTextureFont(createInfo, &mRoboto));
	}

	// Text draw
	{
		vkr::ShaderModulePtr VS;
		CHECKED_CALL(device.CreateShader("basic/shaders", "TextDraw.vs", &VS));
		vkr::ShaderModulePtr PS;
		CHECKED_CALL(device.CreateShader("basic/shaders", "TextDraw.ps", &PS));

		vkr::TextDrawCreateInfo createInfo = {};
		createInfo.pFont = mRoboto;
		createInfo.maxTextLength = 4096;
		createInfo.VS = { VS.Get(), "vsmain" };
		createInfo.PS = { PS.Get(), "psmain" };
		createInfo.renderTargetFormat = GetRender().GetSwapChain().GetColorFormat();

		CHECKED_CALL(device.CreateTextDraw(createInfo, &mStaticText));
		CHECKED_CALL(device.CreateTextDraw(createInfo, &mDynamicText));

		device.DestroyShaderModule(VS);
		device.DestroyShaderModule(PS);
	}

	mStaticText->AddString(float2(50, 100), "Diego brazenly plots pixels for\nmaking, very quirky, images with just code!", float3(0.7f, 0.7f, 0.8f));
	mStaticText->AddString(float2(50, 200), "RED: 0xFF0000", float3(1, 0, 0));
	mStaticText->AddString(float2(50, 240), "GREEN: 0x00FF00", float3(0, 1, 0));
	mStaticText->AddString(float2(50, 280), "BLUE: 0x0000FF", float3(0, 0, 1));
	mStaticText->AddString(float2(50, 330), "This string has\tsome\ttabs that are 400% the size of a space!", 4.0f, 1.0f, float3(1), 1);
	mStaticText->AddString(float2(50, 370), "This string has 70%\nline\nspacing!", 3.0f, 0.7f, float3(1), 1);

	CHECKED_CALL(mStaticText->UploadToGpu(device.GetGraphicsQueue()));

	return true;
}

void Example_026::Shutdown()
{
	mPerFrame.clear();
	// TODO: �������� �������
}

void Example_026::Update()
{
}

void Example_026::Render()
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

	// Build command buffer
	CHECKED_CALL(frame.cmd->Begin());
	{
		// Add some dynamic text
		{
			std::stringstream ss;
			//ss << "Frame: " << GetFrameCount() << "\n";
			//ss << "FPS: " << std::setw(6) << std::setprecision(6) << GetAverageFPS();

			ss << "Test";

			mDynamicText->Clear();
			mDynamicText->AddString(float2(50, 500), ss.str());

			mDynamicText->UploadToGpu(frame.cmd);
		}

		// Update constnat buffer
		mStaticText->PrepareDraw(mCamera.GetViewProjectionMatrix(), frame.cmd);
		mDynamicText->PrepareDraw(mCamera.GetViewProjectionMatrix(), frame.cmd);

		vkr::RenderPassPtr renderPass = swapChain.GetRenderPass(imageIndex);
		ASSERT_MSG(!renderPass.IsNull(), "render pass object is null");

		vkr::RenderPassBeginInfo beginInfo = {};
		beginInfo.pRenderPass = renderPass;
		beginInfo.renderArea = renderPass->GetRenderArea();
		beginInfo.RTVClearCount = 1;
		beginInfo.RTVClearValues[0] = {0.25f, 0.3f, 0.33f, 1};

		frame.cmd->TransitionImageLayout(renderPass->GetRenderTargetImage(0), ALL_SUBRESOURCES, vkr::ResourceState::Present, vkr::ResourceState::RenderTarget);
		frame.cmd->BeginRenderPass(&beginInfo);
		{
			vkr::Rect     scissorRect = renderPass->GetScissor();
			vkr::Viewport viewport = renderPass->GetViewport();
			frame.cmd->SetScissors(1, &scissorRect);
			frame.cmd->SetViewports(1, &viewport);

			mStaticText->Draw(frame.cmd);
			mDynamicText->Draw(frame.cmd);
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

	CHECKED_CALL(GetRenderDevice().GetGraphicsQueue()->Submit(&submitInfo));

	CHECKED_CALL(swapChain.Present(imageIndex, 1, &frame.renderCompleteSemaphore));
}