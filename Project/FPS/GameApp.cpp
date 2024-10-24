﻿#include "GameApp.h"
#include "GameScene.h"
#include "MapData.h"
#include "GameHUD.h"
#include "GameLua.h"

std::unique_ptr<PbrRenderer> renderer;
std::unique_ptr<PhysicsSystem> physicsSystem;
std::unique_ptr<PhysicsSimulationEventCallback> physicsCallback;

std::string nextMap;
std::string currentMap;

bool slowMotion = false;
bool showTriggers = false;
bool prevR = false;

// recreated per map
std::unique_ptr<PhysicsScene> physicsScene;
std::unique_ptr<Scene> scene;
std::unique_ptr<GameLua> lua;
std::unique_ptr<GameHUD> hud;

void ScheduleMapLoad(const std::string& mapName)
{ 
	nextMap = mapName;
}

void LoadMap(const std::string& mapName)
{
	Print("Loading map " + mapName);

	//m_audio->StopAllEvents();

	physicsScene = std::make_unique<PhysicsScene>(physicsSystem.get());
	physicsScene->SetSimulationEventCallback(physicsCallback.get());
	scene = std::make_unique<Scene>();
	lua = std::make_unique<GameLua>();
	hud = std::make_unique<GameHUD>();

	LoadEntities(mapName);
	scene->CreateActor<APropTestModel>("models/sm210_radiatoryang2.obj", "materials/dev_10.json", glm::vec3(5));

	scene->CreateActor<ALightPoint>(glm::vec3(1), glm::vec3(0.9f, 0.2f, 0.4f), 1000);

}

void CleanupMap()
{
	renderer->WaitDeviceIdle();
	hud.reset();
	lua.reset();
	scene.reset();
	physicsScene.reset();
}

bool GameApp::Create()
{
	renderer = std::make_unique<PbrRenderer>(Window::GetWindow());
	physicsSystem = std::make_unique<PhysicsSystem>();
	physicsCallback = std::make_unique<PhysicsSimulationEventCallback>();

	ScheduleMapLoad("maps/puzzle/level0.mp");

	return true;
}

void GameApp::Destroy()
{
	physicsCallback.reset();
	physicsSystem.reset();
	renderer.reset();
}

void loadMap()
{
	CleanupMap();
	LoadMap(nextMap);
	currentMap = std::move(nextMap);
}

void GameApp::Update()
{
	if (!nextMap.empty())
	{
		loadMap();
		return;
	}

	float deltatime = EngineApp::GetDeltaTime();

	//m_audio->Update();

	if (glfwGetKey(Window::GetWindow(), GLFW_KEY_ESCAPE))
	{
		glfwSetWindowShouldClose(Window::GetWindow(), GLFW_TRUE);
	}
	const bool currentR = glfwGetKey(Window::GetWindow(), GLFW_KEY_R);
	if (currentR && !prevR)
	{
		nextMap = std::move(currentMap);
	}
	prevR = currentR;

	slowMotion = glfwGetKey(Window::GetWindow(), GLFW_KEY_TAB);
	showTriggers = glfwGetKey(Window::GetWindow(), GLFW_KEY_CAPS_LOCK);

	lua->Update(deltatime);

	const float timeScale = slowMotion ? 0.2f : 1.0f;
	if (physicsScene->Update(deltatime, timeScale))
	{
		scene->FixedUpdate(physicsScene->GetFixedTimestep() * timeScale);
	}
	scene->Update(deltatime * timeScale);

	hud->Update(deltatime);
}

void Draw() 
{
	scene->Draw();
	hud->Draw();
	renderer->FinishDrawing();
}

void GameApp::Frame()
{
	Draw();
}