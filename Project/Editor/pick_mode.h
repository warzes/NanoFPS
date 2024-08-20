#pragma once

#include "raylib.h"
#include "imgui/imgui.h"

#include <vector>
#include <string>
#include <assert.h>
#include <mutex>
#include <future>
#include <map>
#include <set>

#include "app.h"

#define SEARCH_BUFFER_SIZE 256

class PickMode : public App::ModeImpl
{
public:
	//Represents a selectable frame in the list or grid of the picker
	struct Frame
	{
		fs::path        filePath;
		std::string     label;
		Texture         texture;

		Frame();
		Frame(const fs::path filePath, const fs::path rootDir);
	};

	enum class Mode { TEXTURES, SHAPES };

	PickMode(Mode mode);
	virtual void Update() override;
	virtual void Draw() override;
	virtual void OnEnter() override;
	virtual void OnExit() override;

	inline Mode GetMode() const { return _mode; }

	std::shared_ptr<Assets::TexHandle> GetPickedTexture() const;
	std::shared_ptr<Assets::ModelHandle> GetPickedShape() const;

protected:
	//Retrieves files, recursively, and generates frames for each.
	void _GetFrames();

	//Load or retrieve cached texture
	Texture2D       _GetTexture(const fs::path path);
	//Load or retrieve cached model
	Model           _GetModel(const fs::path path);
	//Load or retrieve cached render texture
	RenderTexture2D _GetIcon(const fs::path path);

	std::map<fs::path, Texture2D> _loadedTextures;
	std::map<fs::path, Model> _loadedModels;
	std::map<fs::path, RenderTexture2D> _loadedIcons;
	std::set<fs::path> _foundFiles;
	std::vector<Frame> _frames;

	Frame _selectedFrame;
	Camera _iconCamera; //Camera for rendering 3D shape preview icons

	char _searchFilterBuffer[SEARCH_BUFFER_SIZE];
	char _searchFilterPrevious[SEARCH_BUFFER_SIZE];

	Mode _mode;
	fs::path _rootDir;
};