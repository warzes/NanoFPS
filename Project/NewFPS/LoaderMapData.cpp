#include "stdafx.h"
#include "LoaderMapData.h"

#define DEG2RAD (pi<float>()/180.0f) /*TODO: удалить*/

fileMapData::TileGrid::TileGrid(LoaderMapData* mapMan, size_t width, size_t height, size_t length, float spacing, Tile fill)
	: Grid<Tile>(width, height, length, spacing, fill)
{
	m_mapMan = mapMan;
	m_batchFromY = 0;
	m_batchToY = height - 1;
	m_batchPosition = glm::vec3(0.0f);
	m_regenBatches = true;
	m_regenModel = true;
	m_modelCulled = false;
}

fileMapData::Tile fileMapData::TileGrid::GetTile(int i, int j, int k) const
{
	return getCel(i, j, k);
}

fileMapData::Tile fileMapData::TileGrid::GetTile(int flatIndex) const
{
	return m_grid[flatIndex];
}

void fileMapData::TileGrid::SetTile(int i, int j, int k, const Tile& tile)
{
	setCel(i, j, k, tile);
	m_regenBatches = true;
	m_regenModel = true;
}

void fileMapData::TileGrid::SetTile(int flatIndex, const Tile& tile)
{
	m_grid[flatIndex] = tile;
	m_regenBatches = true;
	m_regenModel = true;
}

std::string fileMapData::TileGrid::GetTileDataBase64() const
{
	std::vector<uint8_t> bin;
	bin.reserve(m_grid.size() * sizeof(Tile));
	for (size_t i = 0; i < m_grid.size(); ++i)
	{
		Tile savedTile = m_grid[i];

		//Reinterpret each tile as a series of bytes and push them onto the vector.
		const char* tileBin = reinterpret_cast<const char*>(&savedTile);
		for (size_t b = 0; b < sizeof(Tile); ++b)
		{
			bin.push_back(tileBin[b]);
		}
	}

	return base64::encode(bin);
}

std::string fileMapData::TileGrid::GetOptimizedTileDataBase64() const
{
	std::vector<uint8_t> bin;
	bin.reserve(m_grid.size() * sizeof(Tile));

	int runLength = 0;
	for (size_t i = 0; i < m_grid.size(); ++i)
	{
		Tile savedTile = m_grid[i];

		if (!savedTile && i < m_grid.size() - 1)
		{
			// Blank tiles (except for the last tile in the grid) are represented as runs.
			++runLength;
		}
		else
		{
			if (runLength > 0)
			{
				//Insert a special tile signifying the number of empty tiles preceding this one.
				Tile runTile = { -runLength, runLength, -runLength, runLength };
				const char* tileBin = reinterpret_cast<const char*>(&runTile);
				for (size_t b = 0; b < sizeof(Tile); ++b)
				{
					bin.push_back(tileBin[b]);
				}
				runLength = 0;
			}

			//Reinterpret the tile as a series of bytes and push them onto the vector.
			const char* tileBin = reinterpret_cast<const char*>(&savedTile);
			for (size_t b = 0; b < sizeof(Tile); ++b)
			{
				bin.push_back(tileBin[b]);
			}
		}
	}

	return base64::encode(bin);
}

void fileMapData::TileGrid::SetTileDataBase64(std::string data)
{
	std::vector<uint8_t> bin = base64::decode(data);
	size_t gridIndex = 0;
	for (size_t b = 0; b < bin.size(); b += sizeof(Tile))
	{
		// Reinterpret groups of bytes as tiles and place them into the grid.
		Tile loadedTile = *(reinterpret_cast<Tile*>(&bin[b]));
		if (loadedTile.shape < 0)
		{
			// This tile represents a run of blank tiles
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

// Default, inactive entity
fileMapData::Ent::Ent()
{
	active = false;
	display = DisplayMode::SPHERE;
	color = Color::White;
	radius = 0.0f;
	position = glm::vec3(0.0f);
	yaw = pitch = 0;
	properties = std::map<std::string, std::string>();
}

fileMapData::Ent::Ent(float radius)
{
	active = true;
	display = DisplayMode::SPHERE;
	color = Color::White;
	this->radius = radius;
	position = glm::vec3(0.0f);
	yaw = pitch = 0;
	properties = std::map<std::string, std::string>();
}

void fileMapData::to_json(nlohmann::json& j, const fileMapData::Ent& ent)
{
	j["radius"] = ent.radius;
	j["color"] = nlohmann::json::array({ ent.color.r, ent.color.g, ent.color.b });
	j["position"] = nlohmann::json::array({ ent.position.x, ent.position.y, ent.position.z });
	j["angles"] = nlohmann::json::array({ ent.pitch, ent.yaw, 0.0f });
	j["display"] = ent.display;
	//if (ent.model != nullptr) j["model"] = ent.model->GetPath();
	//if (ent.texture != nullptr) j["texture"] = ent.texture->GetPath();
	j["properties"] = ent.properties;
}

void fileMapData::from_json(const nlohmann::json& j, fileMapData::Ent& ent)
{
	ent.active = true;
	ent.radius = j.at("radius");
	ent.color = Color{ j.at("color").at(0), j.at("color").at(1), j.at("color").at(2), 255 };
	ent.position = glm::vec3{ j.at("position").at(0), j.at("position").at(1), j.at("position").at(2) };
	ent.pitch = j.at("angles").at(0);
	ent.yaw = j.at("angles").at(1);
	ent.properties = j.at("properties");

	if (j.contains("display"))
		ent.display = j.at("display");
	else
		ent.display = Ent::DisplayMode::SPHERE;

	//switch (ent.display)
	//{
	//case Ent::DisplayMode::MODEL:
	//	if (j.contains("model")) ent.model = Assets::GetModel(j.at("model"));
	//	//fallthrough
	//case Ent::DisplayMode::SPRITE:
	//	if (j.contains("texture")) ent.texture = Assets::GetTexture(j.at("texture"));
	//	break;
	//}
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
	json jData;
	file >> jData;

	auto tData = jData.at("tiles");

	//Replace our textures with the listed ones
	std::vector<std::string> texturePaths = tData.at("textures");
	m_texturePaths.clear();
	m_texturePaths.reserve(texturePaths.size());
	for (const auto& path : texturePaths)
	{
		m_texturePaths.push_back(std::filesystem::path(path));
	}

	//Same with models
	std::vector<std::string> shapePaths = tData.at("shapes");
	m_modelPaths.clear();
	m_modelPaths.reserve(shapePaths.size());
	for (const auto& path : shapePaths)
	{
		m_modelPaths.push_back(std::filesystem::path(path));
	}

	m_tileGrid = fileMapData::TileGrid(this, tData.at("width"), tData.at("height"), tData.at("length"), TILE_SPACING_DEFAULT, fileMapData::Tile());
	m_tileGrid.SetTileDataBase64(tData.at("data"));

	m_entGrid = fileMapData::EntGrid(m_tileGrid.GetWidth(), m_tileGrid.GetHeight(), m_tileGrid.GetLength());
	for (const fileMapData::Ent& e : jData.at("ents").get<std::vector<fileMapData::Ent>>())
	{
		glm::vec3 gridPos = m_entGrid.WorldToGridPos(e.position);
		m_entGrid.AddEnt((int)gridPos.x, (int)gridPos.y, (int)gridPos.z, e);
	}

	if (jData.contains("editorCamera"))
	{
		json::array_t posArr = jData["editorCamera"]["position"];
		m_defaultCameraPosition = glm::vec3{
		(float)posArr[0], (float)posArr[1], (float)posArr[2]
		};
		json::array_t rotArr = jData["editorCamera"]["eulerAngles"];
		m_defaultCameraAngles = glm::vec3{
		(float)rotArr[0] * DEG2RAD, (float)rotArr[1] * DEG2RAD, (float)rotArr[2] * DEG2RAD
		};
	}
	return true;
}

void LoaderMapData::Shutdown()
{
}
