#include "stdafx.h"
#include "LoaderMapData.h"

#define DEG2RAD (pi<float>()/180.0f) /*TODO: удалить*/

fileMapData::TileGrid::TileGrid(size_t width, size_t height, size_t length, float spacing, Tile fill)
	: Grid<Tile>(width, height, length, spacing, fill)
{
}

fileMapData::Tile fileMapData::TileGrid::GetTile(int i, int j, int k) const
{
	return getCel(i, j, k);
}

fileMapData::Tile fileMapData::TileGrid::GetTile(int flatIndex) const
{
	return m_grid[flatIndex];
}

fileMapData::TileGrid fileMapData::TileGrid::Subsection(int i, int j, int k, int w, int h, int l) const
{
	assert(i >= 0 && j >= 0 && k >= 0);
	assert(i + w <= int(m_width) && j + h <= int(m_height) && k + l <= int(m_length));

	TileGrid newGrid(w, h, l);

	subsectionCopy(i, j, k, w, h, l, newGrid);

	return newGrid;
}

void fileMapData::TileGrid::SetTileDataBase64(std::string data)
{
	std::vector<uint8_t> bin = base64::decode(data);
	size_t gridIndex = 0;
	for (size_t b = 0; b < bin.size(); b += sizeof(Tile))
	{
		Tile loadedTile = *(reinterpret_cast<Tile*>(&bin[b]));
		if (loadedTile.shape < 0)
		{
			int j = 0;
			for (j = 0; j < -loadedTile.shape; ++j)
			{
				m_grid[gridIndex + j] = Tile{ NO_MODEL, 0, NO_TEX, 0 };
			}
			gridIndex += j;
		}
		else
		{
			m_grid[gridIndex] = loadedTile;
			++gridIndex;
		}
	}
}

fileMapData::Ent::Ent(float radius)
{
	active = true;
	this->radius = radius;
}

void fileMapData::from_json(const nlohmann::json& j, fileMapData::Ent& ent)
{
	ent.active     = true;
	ent.radius     = j.at("radius");
	ent.color      = Color{ j.at("color").at(0), j.at("color").at(1), j.at("color").at(2), 255 };
	ent.position   = glm::vec3{ j.at("position").at(0), j.at("position").at(1), j.at("position").at(2) };
	ent.pitch      = j.at("angles").at(0);
	ent.yaw        = j.at("angles").at(1);
	ent.properties = j.at("properties");

	if (j.contains("display")) ent.display = j.at("display");
	else ent.display = Ent::DisplayMode::SPHERE;

	switch (ent.display)
	{
	case Ent::DisplayMode::MODEL:
		if (j.contains("model")) ent.model = j.at("model");
		[[fallthrough]];
	case Ent::DisplayMode::SPRITE:
		if (j.contains("texture")) ent.texture = j.at("texture");
		break;
	}
}

//Creates ent grid of given dimensions, default spacing.
fileMapData::EntGrid::EntGrid(size_t width, size_t height, size_t length)
	: Grid<Ent>(width, height, length, ENT_SPACING_DEFAULT, Ent())
{
	m_labelsToDraw.reserve(m_grid.size());
}

bool LoaderMapData::Setup(std::filesystem::path filePath)
{
	using namespace nlohmann;

	filePath = "GameData/Maps/" / filePath;

	std::ifstream file(filePath);
	json jsonData;
	file >> jsonData;

	auto tilesData = jsonData.at("tiles");
	{
		m_texturePaths = tilesData.at("textures");
		m_modelPaths = tilesData.at("shapes");

		m_tileGrid = fileMapData::TileGrid(tilesData.at("width"), tilesData.at("height"), tilesData.at("length"), fileMapData::TILE_SPACING_DEFAULT, fileMapData::Tile());
		m_tileGrid.SetTileDataBase64(tilesData.at("data"));

		//for (size_t x = 0; x < m_tileGrid.GetWidth(); x++)
		//{
		//	std::string line = "";
		//	for (size_t y = 0; y < m_tileGrid.GetLength(); y++)
		//	{
		//		auto t = m_tileGrid.GetTile(x, 1, y);

		//		line += std::to_string(t.texture);
		//	}
		//	puts(line.c_str());
		//}
	}

	m_entGrid = fileMapData::EntGrid(m_tileGrid.GetWidth(), m_tileGrid.GetHeight(), m_tileGrid.GetLength());
	auto entData = jsonData.at("ents");
	{
		for (const fileMapData::Ent& e : entData.get<std::vector<fileMapData::Ent>>())
		{
			glm::vec3 gridPos = m_entGrid.WorldToGridPos(e.position);
			m_entGrid.AddEnt((int)gridPos.x, (int)gridPos.y, (int)gridPos.z, e);
		}
	}

	if (jsonData.contains("editorCamera"))
	{
		json::array_t posArr = jsonData["editorCamera"]["position"];
		m_defaultCameraPosition = glm::vec3{(float)posArr[0], (float)posArr[1], (float)posArr[2]};
		json::array_t rotArr = jsonData["editorCamera"]["eulerAngles"];
		m_defaultCameraAngles = glm::vec3{(float)rotArr[0] * DEG2RAD, (float)rotArr[1] * DEG2RAD, (float)rotArr[2] * DEG2RAD};
	}
	return true;
}

void LoaderMapData::Shutdown()
{
}
