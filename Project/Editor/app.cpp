#include "app.h"

#include "raylib.h"
#include "raymath.h"

#include "imgui/rlImGui.h"
#include "imgui/imgui.h"

#include <stdlib.h>
#include <vector>
#include <string>
#include <memory>
#include <fstream>
#include <iostream>
#include <filesystem>

#include "assets.h"
#include "menu_bar.h"
#include "place_mode.h"
#include "pick_mode.h"
#include "ent_mode.h"

#define SETTINGS_FILE_PATH "settings.json"

static App* _appInstance = nullptr;

App* App::Get()
{
	if (!_appInstance)
	{
		_appInstance = new App();
	}
	return _appInstance;
}

App::App()
	: _settings{
	.texturesDir = "assets/textures/tiles/",
	.shapesDir = "assets/models/shapes/",
	.undoMax = 30UL,
	.mouseSensitivity = 0.5f,
	.exportSeparateGeometry = false,
	.cullFaces = true,
	.defaultTexturePath = "assets/textures/tiles/brickwall.png",
	.defaultShapePath = "assets/models/shapes/cube.obj",
	},
	_mapMan(std::make_unique<MapMan>()),
	_tilePlaceMode(std::make_unique<PlaceMode>(*_mapMan.get())),
	_texPickMode(std::make_unique<PickMode>(PickMode::Mode::TEXTURES)),
	_shapePickMode(std::make_unique<PickMode>(PickMode::Mode::SHAPES)),
	_entMode(std::make_unique<EntMode>()),
	_editorMode(_tilePlaceMode.get()),
	_previewDraw(false),
	_lastSavedPath(),
	_menuBar(std::make_unique<MenuBar>(_settings)),
	_quit(false)
{
	std::filesystem::directory_entry entry{ SETTINGS_FILE_PATH };
	if (entry.exists())
	{
		LoadSettings();
	}
	else
	{
		SaveSettings();
	}
}

void App::ChangeEditorMode(const App::Mode newMode)
{
	_editorMode->OnExit();

	if (_editorMode == _texPickMode.get() && _texPickMode->GetPickedTexture())
	{
		_tilePlaceMode->SetCursorTexture(_texPickMode->GetPickedTexture());
	}
	else if (_editorMode == _shapePickMode.get() && _shapePickMode->GetPickedShape())
	{
		_tilePlaceMode->SetCursorShape(_shapePickMode->GetPickedShape());
	}

	switch (newMode)
	{
	case App::Mode::PICK_SHAPE:
	{
		_editorMode = _shapePickMode.get();
	}
	break;
	case App::Mode::PICK_TEXTURE:
	{
		_editorMode = _texPickMode.get();
	}
	break;
	case App::Mode::PLACE_TILE:
	{
		if (_editorMode == _entMode.get())
		{
			_tilePlaceMode->SetCursorEnt(_entMode->GetEnt());
		}
		_editorMode = _tilePlaceMode.get();
	}
	break;
	case App::Mode::EDIT_ENT:
	{
		if (_editorMode == _tilePlaceMode.get()) _entMode->SetEnt(_tilePlaceMode->GetCursorEnt());
		_editorMode = _entMode.get();
	}
	break;
	}
	_editorMode->OnEnter();
}

void App::Update()
{
	_menuBar->Update();

	//Mode switching hotkeys
	if (IsKeyPressed(KEY_TAB))
	{
		if (IsKeyDown(KEY_LEFT_SHIFT))
		{
			if (_editorMode == _tilePlaceMode.get()) ChangeEditorMode(Mode::PICK_SHAPE);
			else ChangeEditorMode(Mode::PLACE_TILE);
		}
		else if (IsKeyDown(KEY_LEFT_CONTROL))
		{
			if (_editorMode == _tilePlaceMode.get()) ChangeEditorMode(Mode::EDIT_ENT);
			else if (_editorMode == _entMode.get()) ChangeEditorMode(Mode::PLACE_TILE);
		}
		else
		{
			if (_editorMode == _tilePlaceMode.get()) ChangeEditorMode(Mode::PICK_TEXTURE);
			else ChangeEditorMode(Mode::PLACE_TILE);
		}
	}

	//Save hotkey
	if (IsKeyDown(KEY_LEFT_CONTROL) && IsKeyPressed(KEY_S))
	{
		if (!GetLastSavedPath().empty())
		{
			TrySaveMap(GetLastSavedPath());
		}
		else
		{
			_menuBar->OpenSaveMapDialog();
		}
	}

	_editorMode->Update();

	//Draw
	BeginDrawing();

	ClearBackground(GetBackgroundColor());

	rlImGuiBegin();
	_editorMode->Draw();
	_menuBar->Draw();
	rlImGuiEnd();

	if (!_previewDraw) DrawFPS(GetScreenWidth() - 24, 4);

	EndDrawing();
}

void App::DisplayStatusMessage(std::string message, float durationSeconds, int priority)
{
	_menuBar->DisplayStatusMessage(message, durationSeconds, priority);
}

void App::ResetEditorCamera()
{
	if (_editorMode == _tilePlaceMode.get())
	{
		_tilePlaceMode->ResetCamera();
		_tilePlaceMode->ResetGrid();
	}
}

void App::NewMap(int width, int height, int length)
{
	_mapMan->NewMap(width, height, length);
	_tilePlaceMode->ResetCamera();
	_tilePlaceMode->ResetGrid();
	_lastSavedPath = "";
}

void App::ExpandMap(Direction axis, int amount)
{
	_mapMan->ExpandMap(axis, amount);
	_tilePlaceMode->ResetGrid();
}

void App::ShrinkMap()
{
	_mapMan->ShrinkMap();
	_tilePlaceMode->ResetGrid();
	_tilePlaceMode->ResetCamera();
}

void App::TryOpenMap(fs::path path)
{
	fs::directory_entry entry{ path };
	if (entry.exists() && entry.is_regular_file())
	{
		if (path.extension() == ".te3")
		{
			if (_mapMan->LoadTE3Map(path))
			{
				_lastSavedPath = path;
				DisplayStatusMessage("Loaded .te3 map '" + path.filename().string() + "'.", 5.0f, 100);
			}
			else
			{
				DisplayStatusMessage("ERROR: Failed to load .te3 map. Check the console.", 5.0f, 100);
			}
			_tilePlaceMode->ResetCamera();
			// Set editor camera to saved position
			_tilePlaceMode->SetCameraOrientation(_mapMan->GetDefaultCameraPosition(), _mapMan->GetDefaultCameraAngles());
			_tilePlaceMode->ResetGrid();
		}
		else if (path.extension() == ".ti")
		{
			if (_mapMan->LoadTE2Map(path))
			{
				_lastSavedPath = "";
				DisplayStatusMessage("Loaded .ti map '" + path.filename().string() + "'.", 5.0f, 100);
			}
			else
			{
				DisplayStatusMessage("ERROR: Failed to load .ti map. Check the console.", 5.0f, 100);
			}
			_tilePlaceMode->ResetCamera();
			_tilePlaceMode->ResetGrid();
		}
		else
		{
			DisplayStatusMessage("ERROR: Invalid file extension.", 5.0f, 100);
		}
	}
	else
	{
		DisplayStatusMessage("ERROR: Invalid file path.", 5.0f, 100);
	}
}

void App::TrySaveMap(fs::path path)
{
	//Add correct extension if no extension is given.
	if (path.extension().empty())
	{
		path += ".te3";
	}

	fs::directory_entry entry{ path };

	if (path.extension() == ".te3")
	{
		if (_mapMan->SaveTE3Map(path))
		{
			_lastSavedPath = path;
			std::string msg = "Saved .te3 map '";
			msg += path.filename().string();
			msg += "'.";
			DisplayStatusMessage(msg, 5.0f, 100);
		}
		else
		{
			DisplayStatusMessage("ERROR: Map could not be saved. Check the console.", 5.0f, 100);
		}
	}
	else
	{
		DisplayStatusMessage("ERROR: Invalid file extension.", 5.0f, 100);
	}
}

void App::TryExportMap(fs::path path, bool separateGeometry)
{
	//Add correct extension if no extension is given.
	if (path.extension().empty())
	{
		path += ".gltf";
	}

	fs::directory_entry entry{ path };

	if (path.extension() == ".gltf" || path.extension() == ".glb")
	{
		if (_mapMan->ExportGLTFScene(path, separateGeometry))
		{
			DisplayStatusMessage(std::string("Exported map as ") + path.filename().string(), 5.0f, 100);
		}
		else
		{
			DisplayStatusMessage("ERROR: Map could not be exported. Check the console.", 5.0f, 100);
		}
	}
	else
	{
		DisplayStatusMessage("ERROR: Invalid file extension.", 5.0f, 100);
	}
}

void App::SaveSettings()
{
	try
	{
		nlohmann::json jData;
		App::to_json(jData, _settings);
		std::ofstream file(SETTINGS_FILE_PATH);
		file << jData;
	}
	catch (std::exception e)
	{
		std::cerr << "Error saving settings: " << e.what() << std::endl;
	}
}

void App::LoadSettings()
{
	try
	{
		nlohmann::json jData;
		std::ifstream file(SETTINGS_FILE_PATH);
		file >> jData;
		App::from_json(jData, _settings);
	}
	catch (std::exception e)
	{
		std::cerr << "Error loading settings: " << e.what() << std::endl;
	}
}
