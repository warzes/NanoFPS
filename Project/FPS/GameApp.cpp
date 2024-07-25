#include "GameApp.h"

std::unique_ptr<PbrRenderer> Renderer;

bool GameApp::Create()
{
	Renderer = std::make_unique<PbrRenderer>(Window::GetWindow());

	return true;
}

void GameApp::Destroy()
{
	Renderer.reset();
}

void GameApp::Update()
{
}

void GameApp::Frame()
{
}