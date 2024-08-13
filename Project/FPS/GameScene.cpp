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

bool parseProperty(std::map<std::string, std::string>& prop, const char* propString)
{
	char kvp[1024];
	char key[512];
	char value[512];

	strcpy_s(kvp, propString);
	char* next_token;
	rsize_t kvpmax = sizeof kvp;
	char* tok = strtok_s(kvp, "\"", &next_token);

	int i = 0;

	while (tok != NULL)
	{
		if (i == 0)
		{
			strcpy_s(key, tok);
		}
		else if (i == 2)
		{
			strcpy_s(value, tok);
		}
		i++;
		tok = strtok_s(NULL, "\"", &next_token);
	}
	if (i == 4)
	{
		prop[key] = value;
		return true;
	}
	return false;
}

class Side
{
public:
	glm::vec3 p1, p2, p3;
	float xScale, yScale, rotation, xShift, yShift;
	char texture[256];
	int pointCount = 0;
	std::vector<glm::vec3> points;

	Side() {}

	~Side() {}

	glm::vec3 normal()
	{
		glm::vec3 ab = p2 - p1;
		glm::vec3 ac = p3 - p1;
		return glm::cross(ab, ac);
	}

	glm::vec3 center()
	{
		return (p1 + p2 + p3) / 3.0f;
	}

	float distance()
	{
		glm::vec3 n = normal();
		return ((p1.x * n.x) + (p1.y * n.y) + (p1.z * n.z)) / sqrtf(powf(n.x, 2) + powf(n.y, 2) + powf(n.z, 2));
	}

	// calculate the center of a brush face by averaging the positions of each vertex
	glm::vec3 pointCenter()
	{
		glm::vec3 c;
		for (int i = 0; i < pointCount; i++)
			c = c + points[i];
		return c / (float)pointCount;
	}

	// sort vertices clockwise by comparing their angles to the center
	void sortVertices()
	{
		glm::vec3 c = pointCenter() + 1e-5f; // in case the angle of every vertex is the same, the center  should be slightly off
		glm::vec3 n = normal();
		std::stable_sort(points.begin(), points.end(), [&](glm::vec3  lhs, glm::vec3 rhs)
			{
				if (lhs == rhs)
					return false; // this line makes this an unstable sort

				glm::vec3 ca = c - lhs;
				glm::vec3 cb = c - rhs;
				glm::vec3 caXcb = glm::cross(ca, cb);

				return glm::dot(n, caXcb) >= 0;
			});
	}
};

bool parseSide(Side& res, const char* sideStr)
{
	int count = sscanf_s(sideStr, "( %f %f %f ) ( %f %f %f ) ( %f %f %f ) %s %f %f %f %f %f",
		&res.p1.x, &res.p1.y, &res.p1.z,
		&res.p2.x, &res.p2.y, &res.p2.z,
		&res.p3.x, &res.p3.y, &res.p3.z,
		res.texture, (unsigned)_countof(res.texture),
		&res.xScale, &res.yScale,
		&res.rotation,
		&res.yShift, &res.yShift
	);
	if (count == 15)
	{
		return true;
	}
	return false;
}

// calculate the intersection points of three planes
bool getPlaneIntersection(Side side1, Side side2, Side side3, glm::vec3& out)
{
	glm::vec3 normal1 = glm::normalize(side1.normal());
	glm::vec3 normal2 = glm::normalize(side2.normal());
	glm::vec3 normal3 = glm::normalize(side3.normal());
	float determinant = glm::dot(normal1, glm::cross(normal2, normal3));

	// can't intersect parallel planes
	if ((determinant <= 1e-5 and determinant >= -1e-5))
		return false;

	out = (
		glm::cross(normal2, normal3) * side1.distance() +
		glm::cross(normal3, normal1) * side2.distance() +
		glm::cross(normal1, normal2) * side3.distance()
		) / determinant;

	return true;
}

// avoid adding duplicate verts
bool inList(glm::vec3 vec, std::vector<glm::vec3> vecs)
{
	for (glm::vec3 v : vecs)
	{
		if (v == vec) return true;
	}
	return false;
}

// check if a point is a part of the convex shape
bool pointIsLegal(std::vector<Side>& sides, glm::vec3 v)
{
	int sideCount = sides.size();
	for (int i = 0; i < sideCount; i++)
	{
		glm::vec3 facing = glm::normalize(v - sides[i].center());
		if (glm::dot(facing, glm::normalize(sides[i].normal())) < -1e-5)
			return false;
	}
	return true;
}

void getIntersectionPoints(std::vector<Side>& sides)
{
	int sideCount = sides.size();
	glm::vec3 point;
	for (int i = 0; i < sideCount - 2; i++)
	{
		for (int j = 0; j < sideCount - 1; j++)
		{
			for (int k = 0; k < sideCount; k++)
			{
				if (i != j && i != k && j != k)
				{
					if (getPlaneIntersection(sides[i], sides[j], sides[k], point))
					{
						if (pointIsLegal(sides, point))
						{
							if (!inList(point, sides[i].points))
							{
								sides[i].points.push_back(point);
								sides[i].pointCount++;
							}

							if (!inList(point, sides[j].points))
							{
								sides[j].points.push_back(point);
								sides[j].pointCount++;
							}

							if (!inList(point, sides[k].points))
							{
								sides[k].points.push_back(point);
								sides[k].pointCount++;
							}
						}
					}
				}
			}
		}
	}
}

void LoadEntities2(const std::string& mapFilename)
{
	MapData::Map map;
	{
		FILE* file{};
		auto err = fopen_s(&file, mapFilename.c_str(), "r");
		if (err || file == NULL)
		{
			Fatal("Could not open file " + mapFilename);
			return;
		}

		Print("Loading map file " + mapFilename);

		enum
		{
			NONE = 0,
			ENTITY,
			BRUSH,
			PATCH
		};

		// проверить какие из временных объектов не нужны
		MapData::Entity tempEnt;
		MapData::Brush tempBrush;
		Side tempSide;
		std::vector<Side> tempSides;

		int l = 0;
		int CURRENT = NONE;
		char line[256];
		while (fgets(line, sizeof(line), file))
		{
			l++;

			// check for what to expect first
			if (line[0] == '/')
			{ // it's a comment
				continue;
			}
			else if (line[0] == '{')
			{ // open bracket
				if (CURRENT == NONE)
				{
					CURRENT = ENTITY;
					continue;
				}
				else if (CURRENT == ENTITY)
				{
					CURRENT = BRUSH;
					continue;
				}
				else
				{
					Fatal("Parse error. Unexpected { on line" + std::to_string(l));
					fclose(file);
					return;
				}
			}
			else if (line[0] == '}')
			{ // close bracket
				if (CURRENT == BRUSH)
				{
					getIntersectionPoints(tempSides);

					typedef struct Tri 
					{
						glm::vec3 p1, p2, p3;
					} Tri;
					std::vector<Tri> tris;

					for (Side side : tempSides)
					{
						if (side.pointCount < 3)
						{
							continue;
						}
						side.sortVertices();
						tris.push_back({
							{side.points[0].x / 30.f, side.points[0].z / 30.f, side.points[0].y / -30.f},
							{side.points[side.pointCount - 1].x / 30.f, side.points[side.pointCount - 1].z / 30.f, side.points[side.pointCount - 1].y / -30.f},
							{side.points[1].x / 30.f, side.points[1].z / 30.f, side.points[1].y / -30.f},
							});
						for (int i = 1; i < (side.pointCount / 2); i++) {
							tris.push_back({
								{side.points[i].x / 30.f, side.points[i].z / 30.f, side.points[i].y / -30.f},
								{side.points[side.pointCount - i].x / 30.f, side.points[side.pointCount - i].z / 30.f, side.points[side.pointCount - i].y / -30.f},
								{side.points[i + 1].x / 30.f, side.points[i + 1].z / 30.f, side.points[i + 1].y / -30.f},
								});

							tris.push_back({
								{side.points[side.pointCount - i].x / 30.f, side.points[side.pointCount - i].z / 30.f, side.points[side.pointCount - i].y / -30.f},
								{side.points[side.pointCount - i - 1].x / 30.f, side.points[side.pointCount - i - 1].z / 30.f, side.points[side.pointCount - i - 1].y / -30.f},
								{side.points[i + 1].x / 30.f, side.points[i + 1].z / 30.f, side.points[i + 1].y / -30.f},
								});
						}
					}

					// TODO: тут

					tempEnt.Brushes.push_back(tempBrush);
					tempBrush = {};
					tempSides.clear();
					CURRENT = ENTITY;
					continue;
				}
				else if (CURRENT == ENTITY)
				{
					map.Entities.push_back(tempEnt);
					tempEnt = {};
					CURRENT = NONE;
					continue;
				}
				else if (CURRENT == NONE)
				{
					Fatal("Parse error. Unexpected } on line " + std::to_string(l));
					fclose(file);
					return;
				}
			}
			else if (line[0] == '"')
			{ // property
				if (CURRENT != ENTITY)
				{
					Fatal("Parse error. Unexpected \" on line " + std::to_string(l));
					fclose(file);
					return;
				}
				if (parseProperty(tempEnt.Properties, line))
				{
					continue;
				}
				else
				{
					Fatal("Parse error. Invalid KVP syntax on line " + std::to_string(l));
					fclose(file);
					return;
				}
			}
			else if (line[0] == '(')
			{ // brush side
				if (CURRENT != BRUSH)
				{
					Fatal("Parse error. Unexpected ( on line " + std::to_string(l));
					fclose(file);
					return;
				}
				if (parseSide(tempSide, line))
				{
					tempSides.push_back(tempSide);
					// TODO:
				}
				else
				{
					Fatal("Parse error. Invalid plane syntax on line " + std::to_string(l));
					fclose(file);
					return;
				}
			}
		}
	}

	// TODO: parse side
	{

	}


	LoadEntities(map);
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