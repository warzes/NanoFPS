#pragma once

#include <string>
#include <memory>

#include "app.h"
#include "dialogs.h"

class MenuBar
{
public:
	MenuBar(App::Settings& settings);
	void Update();
	void Draw();
	void OpenSaveMapDialog();
	void OpenOpenMapDialog();
	void SaveMap();

	void DisplayStatusMessage(std::string message, float durationSeconds, int priority);
protected:
	App::Settings& _settings;

	std::unique_ptr<Dialog> _activeDialog;

	std::string _statusMessage;
	float _messageTimer;
	int _messagePriority;
};