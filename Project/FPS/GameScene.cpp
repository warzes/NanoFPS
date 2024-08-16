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

	scene->CreateActor<APlayerNoClip>(origin);
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
