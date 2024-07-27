#include "GameApp.h"
#include "PhysicsSimulationEventCallback.h"
#include "Scene.h"
#include "MapData.h"
#include "GameHUD.h"
#include "GameLua.h"
#include "Mouse.h"

std::unique_ptr<PbrRenderer> renderer;
std::unique_ptr<PhysicsSystem> physicsSystem;
std::unique_ptr<PhysicsSimulationEventCallback> physicsCallback;

std::unique_ptr<TempMouse> mouse;

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

	mouse->Recalibrate();
	//m_audio->StopAllEvents();

	physicsScene = std::make_unique<PhysicsScene>(physicsSystem.get());
	physicsScene->SetSimulationEventCallback(physicsCallback.get());
	scene = std::make_unique<Scene>();
	lua = std::make_unique<GameLua>();
	hud = std::make_unique<GameHUD>();

	LoadEntities(mapName);
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
	ScheduleMapLoad("maps/puzzle/level0.haru");

	renderer = std::make_unique<PbrRenderer>(Window::GetWindow());
	mouse = std::make_unique<TempMouse>();
	physicsSystem = std::make_unique<PhysicsSystem>();
	physicsCallback = std::make_unique<PhysicsSimulationEventCallback>();

	return true;
}

void GameApp::Destroy()
{
	physicsCallback.reset();
	physicsSystem.reset();
	renderer.reset();
	mouse.reset();
}

void GameApp::Update()
{
}

void Update(float deltaTime)
{
	mouse->Update();
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

	lua->Update(deltaTime);

	const float timeScale = slowMotion ? 0.2f : 1.0f;
	if (physicsScene->Update(deltaTime, timeScale))
	{
		scene->FixedUpdate(physicsScene->GetFixedTimestep() * timeScale);
	}
	scene->Update(deltaTime * timeScale);

	hud->Update(deltaTime);
}

void Draw() 
{
	scene->Draw();
	hud->Draw();

	renderer->FinishDrawing();
}

void GameApp::Frame()
{
	if (!nextMap.empty())
	{
		CleanupMap();
		LoadMap(nextMap);
		currentMap = std::move(nextMap);
	}

	::Update(EngineApp::GetDeltaTime());
	Draw();
}