#include "stdafx.h"
#include "LoaderMapData.h"

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

	return true;
}

void LoaderMapData::Shutdown()
{
}
