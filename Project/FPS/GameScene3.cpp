#include "GameScene.h"
#include "GameLua.h"

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
		&res.p1.x, &res.p1.z, &res.p1.y,
		&res.p2.x, &res.p2.z, &res.p2.y,
		&res.p3.x, &res.p3.z, &res.p3.y,
		res.texture, (unsigned)_countof(res.texture),
		&res.xScale, &res.yScale,
		&res.rotation,
		&res.yShift, &res.yShift
	);

	// TODO: скейлинг от квайк
	res.p1 = res.p1 / 32.0f;
	res.p2 = res.p2 / 32.0f;
	res.p3 = res.p3 / 32.0f;

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

void LoadEntities3(const std::string& mapFilename)
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