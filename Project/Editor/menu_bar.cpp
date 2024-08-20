#include "menu_bar.h"

#include "imgui/imgui.h"

#include <filesystem>
namespace fs = std::filesystem;

#include "map_man.h"

MenuBar::MenuBar(App::Settings& settings)
	: _settings(settings),
	_activeDialog(nullptr),
	_messageTimer(0.0f),
	_messagePriority(0)
{
}

void MenuBar::DisplayStatusMessage(std::string message, float durationSeconds, int priority)
{
	if (priority >= _messagePriority)
	{
		_statusMessage = message;
		_messageTimer = durationSeconds;
		_messagePriority = priority;
	}
}

void MenuBar::OpenOpenMapDialog()
{
	auto callback = [](fs::path path)
		{
			App::Get()->TryOpenMap(path);
		};
	_activeDialog.reset(new FileDialog("Open Map (*.te3, *.ti)", { ".te3", ".ti" }, callback, false));
}

void MenuBar::OpenSaveMapDialog()
{
	auto callback = [](fs::path path)
		{
			App::Get()->TrySaveMap(path);
		};
	_activeDialog.reset(new FileDialog("Save Map (*.te3)", { ".te3" }, callback, true));
}

void MenuBar::SaveMap()
{
	if (!App::Get()->GetLastSavedPath().empty())
	{
		App::Get()->TrySaveMap(App::Get()->GetLastSavedPath());
	}
	else
	{
		OpenSaveMapDialog();
	}
}

void MenuBar::Update()
{
	if (_messageTimer > 0.0f)
	{
		_messageTimer -= GetFrameTime();
		if (_messageTimer <= 0.0f)
		{
			_messageTimer = 0.0f;
			_messagePriority = 0;
		}
	}

	//Show exit confirmation dialog
	if (WindowShouldClose())
	{
		_activeDialog.reset(new CloseDialog());
	}
}

void MenuBar::Draw()
{
	if (ImGui::BeginMainMenuBar())
	{
		if (ImGui::BeginMenu("MAP"))
		{
			if (ImGui::MenuItem("NEW")) _activeDialog.reset(new NewMapDialog());
			if (ImGui::MenuItem("OPEN")) OpenOpenMapDialog();
			if (ImGui::MenuItem("SAVE")) SaveMap();
			if (ImGui::MenuItem("SAVE AS")) OpenSaveMapDialog();
			if (ImGui::MenuItem("EXPORT")) _activeDialog.reset(new ExportDialog(_settings));
			if (ImGui::MenuItem("EXPAND GRID")) _activeDialog.reset(new ExpandMapDialog());
			if (ImGui::MenuItem("SHRINK GRID")) _activeDialog.reset(new ShrinkMapDialog());
			ImGui::EndMenu();
		}
		if (ImGui::BeginMenu("VIEW"))
		{
			if (ImGui::MenuItem("MAP EDITOR"))     App::Get()->ChangeEditorMode(App::Mode::PLACE_TILE);
			if (ImGui::MenuItem("TEXTURE PICKER")) App::Get()->ChangeEditorMode(App::Mode::PICK_TEXTURE);
			if (ImGui::MenuItem("SHAPE PICKER"))   App::Get()->ChangeEditorMode(App::Mode::PICK_SHAPE);
			if (ImGui::MenuItem("ENTITY EDITOR"))  App::Get()->ChangeEditorMode(App::Mode::EDIT_ENT);
			if (ImGui::MenuItem("RESET CAMERA"))   App::Get()->ResetEditorCamera();
			if (ImGui::MenuItem("TOGGLE PREVIEW")) App::Get()->TogglePreviewing();
			ImGui::EndMenu();
		}
		if (ImGui::BeginMenu("CONFIG"))
		{
			if (ImGui::MenuItem("ASSET PATHS")) _activeDialog.reset(new AssetPathDialog(_settings));
			if (ImGui::MenuItem("SETTINGS"))    _activeDialog.reset(new SettingsDialog(_settings));
			ImGui::EndMenu();
		}
		if (ImGui::BeginMenu("INFO"))
		{
			if (ImGui::MenuItem("ABOUT"))          _activeDialog.reset(new AboutDialog());
			if (ImGui::MenuItem("KEYS/SHORTCUTS")) _activeDialog.reset(new ShortcutsDialog());
			if (ImGui::MenuItem("INSTRUCTIONS"))   _activeDialog.reset(new InstructionsDialog());
			ImGui::EndMenu();
		}

		ImGui::SameLine();
		ImGui::TextUnformatted(" | ");
		if (_messageTimer > 0.0f)
		{
			ImGui::SameLine();
			ImGui::TextUnformatted(_statusMessage.c_str());
		}

		ImGui::EndMainMenuBar();
	}

	//Draw modal dialogs
	if (_activeDialog.get())
	{
		if (!_activeDialog->Draw()) _activeDialog.reset(nullptr);
	}
}
