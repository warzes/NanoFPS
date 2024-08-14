#include "GameScene.h"
#include "GameLua.h"

extern std::unique_ptr<Scene> scene;

#pragma region EntityLoaders

void Register(Actor* actor, const MapData::Entity& entity)
{
	std::string name;
	if (entity.GetPropertyString("name", name))
	{
		scene->Register(name, actor);
	}
}

void LoadWorldSpawn(const MapData::Entity& entity)
{
	glm::vec3 lightDirection;
	if(!entity.GetPropertyVector("light_direction", lightDirection))
		Fatal("worldspawn doesn't have a valid light_direction!");

	glm::vec3 lightColor;
	if(!entity.GetPropertyColor("light_color", lightColor))
		Fatal("worldspawn doesn't have a valid light_color!");

	std::string script;
	if(!entity.GetPropertyString("script", script))
		Error("worldspawn doesn't have a valid script!");

	scene->CreateActor<AWorldSpawn>(entity.Brushes, lightDirection, lightColor, script);
}

void LoadInfoPlayerStart(const MapData::Entity& entity)
{
	glm::vec3 origin;
	if(!entity.GetPropertyVector("origin", origin))
		Fatal("info_player_start doesn't have a valid origin!");

	scene->CreateActor<APlayer>(origin);
}

void LoadFuncBrush(const MapData::Entity& entity)
{
	scene->CreateActor<AFuncBrush>(entity.Brushes);
}

void LoadFuncGroup(const MapData::Entity& entity)
{
	scene->CreateActor<AFuncBrush>(entity.Brushes);
}

void LoadFuncMove(const MapData::Entity& entity)
{
	glm::vec3 moveSpeed;
	if (!entity.GetPropertyVector("move_speed", moveSpeed))
		Fatal("func_move doesn't have a valid move_speed!");

	float moveTime;
	if(!entity.GetPropertyFloat("move_time", moveTime))
		Fatal("func_move doesn't have a valid move_time!");

	Register(scene->CreateActor<AFuncMove>(entity.Brushes, moveSpeed, moveTime, "event:/device/func_move/move_stone"), entity);
}

void LoadFuncButton(const MapData::Entity& entity)
{
	glm::vec3 moveSpeed;
	if(!entity.GetPropertyVector("move_speed", moveSpeed))
		Fatal("func_button doesn't have a valid move_speed!");

	float moveTime;
	if(!entity.GetPropertyFloat("move_time", moveTime))
		Fatal("func_button doesn't have a valid move_time!");

	std::string event;
	if(!entity.GetPropertyString("event", event))
		Fatal("func_button doesn't have a valid event!");

	scene->CreateActor<AFuncButton>(entity.Brushes, moveSpeed, moveTime, event);
}

void LoadFuncPhys(const MapData::Entity& entity)
{
	scene->CreateActor<AFuncPhys>(entity.Brushes);
}

void LoadTriggerOnce(const MapData::Entity& entity)
{
	std::string event;
	if(!entity.GetPropertyString("event", event))
		Fatal("trigger_once doesn't have a valid event!");

	int type;
	if(!entity.GetPropertyInteger("type", type))
		Fatal("trigger_once doesn't have a valid type!");

	scene->CreateActor<ATriggerOnce>(entity.Brushes, type, event);
}

void LoadTriggerMultiple(const MapData::Entity& entity)
{
	std::string event;
	if(!entity.GetPropertyString("event", event))
		Fatal("trigger_multiple doesn't have a valid event!");

	int type;
	if(!entity.GetPropertyInteger("type", type))
		Fatal("trigger_multiple doesn't have a valid type!");

	scene->CreateActor<ATrigger>(entity.Brushes, type, event);
}

void LoadLightPoint(const MapData::Entity& entity)
{
	glm::vec3 origin;
	if(!entity.GetPropertyVector("origin", origin))
		Fatal("light_point doesn't have a valid origin!");

	glm::vec3 color;
	if(!entity.GetPropertyColor("color", color))
		Fatal("light_point doesn't have a valid color!");

	float radius;
	if(!entity.GetPropertyFloat("radius", radius))
		Fatal("light_point doesn't have a valid radius!");

	scene->CreateActor<ALightPoint>(origin, color, radius);
}

void LoadPropTestModel(const MapData::Entity& entity)
{
	glm::vec3 origin;
	if(!entity.GetPropertyVector("origin", origin))
		Fatal("prop_test_model doesn't have a valid origin!");

	std::string model;
	if(!entity.GetPropertyString("model", model))
		Fatal("prop_test_model doesn't have a valid model!");

	scene->CreateActor<APropTestModel>(
		"models/" + model + ".obj", //
		"materials/" + model + ".json",
		origin
	);
}

void LoadPropPowerSphere(const MapData::Entity& entity)
{
	glm::vec3 origin;
	if(!entity.GetPropertyVector("origin", origin))
		Fatal("prop_power_sphere doesn't have a valid origin!");

	scene->CreateActor<APropPowerSphere>(origin);
}

#pragma endregion

#pragma region LoadEntities

void LoadEntities(const std::string& mapFilename)
{
	Print("Loading entities in map " + mapFilename);
	const MapParser mapParser(mapFilename);
	LoadEntities(mapParser.GetMap());
}

typedef void (*EntityLoader)(const MapData::Entity& entity);

static const std::map<std::string, EntityLoader> s_EntityLoaders
{
	{"worldspawn",        LoadWorldSpawn     },
	{"info_player_start", LoadInfoPlayerStart},
	{"func_brush",        LoadFuncBrush      },
	{"func_group",        LoadFuncGroup      },
	{"func_move",         LoadFuncMove       },
	{"func_button",       LoadFuncButton     },
	{"func_phys",         LoadFuncPhys       },
	{"trigger_once",      LoadTriggerOnce    },
	{"trigger_multiple",  LoadTriggerMultiple},
	{"light_point",       LoadLightPoint     },
	{"prop_test_model",   LoadPropTestModel  },
	{"prop_power_sphere", LoadPropPowerSphere}
};

void LoadEntity(const MapData::Entity& entity)
{
	std::string className;
	if(!entity.GetPropertyString("classname", className))
		Fatal("Entity doesn't have a class!");

	auto loader = s_EntityLoaders.find(className);
	if(loader == s_EntityLoaders.end())
		Fatal("Unknown entity type: " + className);

	loader->second(entity);
#if defined(_DEBUG)
	Print("Load Entity: " + loader->first);
#endif
}

void LoadEntities(const MapData::Map& map)
{
	for (const MapData::Entity& entity : map.Entities)
	{
		LoadEntity(entity);
	}
}

#pragma endregion

#pragma region Scene

void Scene::Update(const float deltaTime)
{
	// put pending destroy actors from last frame into a temp queue
	const std::vector<std::unique_ptr<Actor>> pendingDestroyActorsFromLastFrame = std::move(m_pendingDestroyActors);

	// update all alive actors
	std::vector<std::unique_ptr<Actor>> aliveActors;
	aliveActors.reserve(m_actors.size());
	for (auto& actor : m_actors)
	{
		actor->Update(deltaTime);
		if (actor->IsPendingDestroy())
		{
			// put pending destroy actors in this frame into m_pendingDestroyActors
			m_pendingDestroyActors.push_back(std::move(actor));
		}
		else
		{
			aliveActors.push_back(std::move(actor));
		}
	}
	m_actors = std::move(aliveActors);

	// pendingDestroyActorsFromLastFrame deconstructs and releases all pending destroy actors destroy pending destroy actors from last frame this strategy will give other actors one frame to cleanup their references
}

void Scene::FixedUpdate(float fixedDeltaTime)
{
	for (auto& actor : m_actors)
		actor->FixedUpdate(fixedDeltaTime);
}

void Scene::Draw()
{
	for (auto& actor : m_actors)
		actor->Draw();
}

Actor* Scene::findFirstActorOfClassImpl(const std::string& className) const
{
	for (auto& actor : m_actors)
	{
		if (actor->GetActorClassName() == className)
		{
			return actor.get();
		}
	}
	return nullptr;
}

void Scene::Register(const std::string& name, Actor* actor)
{
	auto pair = m_registeredActors.find(name);
	if (pair == m_registeredActors.end())
	{
		m_registeredActors.emplace(name, actor);
	}
	else
	{
		Error("Failed to register actor \"" + name + "\" because the name is already registered.");
	}
}

Actor* Scene::FindActorWithName(const std::string& name) const
{
	auto pair = m_registeredActors.find(name);
	if (pair == m_registeredActors.end())
	{
		Error("Failed to find actor \"" + name + "\"");
		return nullptr;
	}
	return pair->second;
}

#pragma endregion