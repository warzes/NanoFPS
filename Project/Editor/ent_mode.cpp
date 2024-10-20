#include "ent_mode.h"


#include "c_helpers.h"
#include "raylib.h"
#include "imgui/imgui.h"

#include <cstring>

EntMode::EntMode()
{
	memset(_texturePathBuffer, 0, TEXT_FIELD_MAX * sizeof(char));
	memset(_modelPathBuffer, 0, TEXT_FIELD_MAX * sizeof(char));
	memset(_newKeyBuffer, 0, TEXT_FIELD_MAX * sizeof(char));
	memset(_newValBuffer, 0, TEXT_FIELD_MAX * sizeof(char));
}

EntMode::~EntMode()
{
}

void EntMode::OnEnter()
{
	if (_ent.texture) strcpy(_texturePathBuffer, _ent.texture->GetPath().string().c_str());
	if (_ent.model) strcpy(_modelPathBuffer, _ent.model->GetPath().string().c_str());
	memset(_newKeyBuffer, 0, TEXT_FIELD_MAX * sizeof(char));
	memset(_newValBuffer, 0, TEXT_FIELD_MAX * sizeof(char));

	// Allocate buffers for all of the entity's properties to be edited
	_valBuffers.clear();
	for (auto [key, value] : _ent.properties)
	{
		_valBuffers[key] = SAFE_MALLOC(char, TEXT_FIELD_MAX);
		strcpy(_valBuffers[key], _ent.properties[key].c_str());
	}
}

void EntMode::OnExit()
{
	// Try to load the entity's model and texture
	if (strlen(_modelPathBuffer) > 0)
	{
		_ent.model = Assets::GetModel(fs::path(std::string(_modelPathBuffer)));
	}
	else
	{
		_ent.model = nullptr;
	}

	if (strlen(_texturePathBuffer) > 0)
	{
		_ent.texture = Assets::GetTexture(fs::path(std::string(_texturePathBuffer)));
	}
	else
	{
		_ent.texture = nullptr;
	}

	for (auto [k, v] : _valBuffers)
	{
		free(v);
	}
}

void EntMode::Update()
{

}

void EntMode::Draw()
{
	if (_fileDialog)
	{
		if (!_fileDialog->Draw())
		{
			_fileDialog.reset(nullptr);
		}
		return;
	}

	const float WINDOW_UPPER_MARGIN = 24.0f;
	bool open = true;
	ImGui::SetNextWindowSize(ImVec2((float)GetScreenWidth(), (float)GetScreenHeight() - WINDOW_UPPER_MARGIN));
	ImVec2 center = ImGui::GetMainViewport()->GetCenter();
	ImGui::SetNextWindowPos(ImVec2(center.x, center.y + WINDOW_UPPER_MARGIN), ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
	if (ImGui::Begin("##Entity Mode View", &open, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove))
	{
		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8.0f, 32.0f));
		// Entity display mode
		ImGui::Combo("Display mode", (int*)(&_ent.display), "Sphere\0Model\0Sprite\0");
		ImGui::Separator();

		ImGui::PopStyleVar(1);

		// Color picker
		float entColorf[3] = { _ent.color.r / 255.0f, _ent.color.g / 255.0f, _ent.color.b / 255.0f };
		ImGui::SetNextItemWidth(256.0f);
		ImGui::ColorPicker3("Color", entColorf, ImGuiColorEditFlags_PickerHueBar | ImGuiColorEditFlags_NoAlpha | ImGuiColorEditFlags_InputRGB);
		_ent.color = Color{
		(unsigned char)(entColorf[0] * 255.0f),
		(unsigned char)(entColorf[1] * 255.0f),
		(unsigned char)(entColorf[2] * 255.0f),
		255
		};

		ImGui::SameLine();
		ImGui::BeginGroup();
		// Display mode configuration
		switch (_ent.display)
		{
		case Ent::DisplayMode::SPHERE:
		{
			// Radius box
			ImGui::TextUnformatted("Radius");
			ImGui::SetNextItemWidth(256.0f);
			ImGui::InputFloat("##Radius", &_ent.radius, 0.1f, 0.1f, "%.1f");
			break;
		}
		case Ent::DisplayMode::MODEL:
		{
			ImGui::TextUnformatted("Model path");
			if (ImGui::Button("Browse##Model"))
			{
				_fileDialog.reset(new FileDialog("Select model", { std::string(".obj") },
					[&](fs::path path)
					{
						fs::path relativePath = fs::relative(path);
						strcpy(_modelPathBuffer, relativePath.string().c_str());
					},
					false));
			}
			ImGui::SameLine();
			ImGui::InputText("##Model path", _modelPathBuffer, TEXT_FIELD_MAX);

			// Intentional fallthrough
		}
		case Ent::DisplayMode::SPRITE:
		{
			ImGui::TextUnformatted("Texture path");
			if (ImGui::Button("Browse##Sprite"))
			{
				_fileDialog.reset(new FileDialog("Select texture", { std::string(".png") },
					[&](fs::path path)
					{
						fs::path relativePath = fs::relative(path);
						strcpy(_texturePathBuffer, relativePath.string().c_str());
					},
					false));
			}
			ImGui::SameLine();
			ImGui::InputText("##Texture path", _texturePathBuffer, TEXT_FIELD_MAX);

			ImGui::TextUnformatted("Scale");
			ImGui::SetNextItemWidth(256.0f);
			ImGui::InputFloat("##Scale", &_ent.radius, 0.1f, 0.1f, "%.1f");

			break;
		}
		}
		ImGui::EndGroup();
		ImGui::Separator();

		// Properties editor
		ImGui::Text("Entity properties");
		if (ImGui::BeginTable("Entity properties", 3, ImGuiTableFlags_Resizable | ImGuiTableFlags_BordersV))
		{
			ImGui::TableSetupColumn("Key");
			ImGui::TableSetupColumn("Value");
			ImGui::TableSetupColumn(" ");
			ImGui::TableHeadersRow();

			std::vector<std::string> keysToErase;
			keysToErase.reserve(_ent.properties.size());
			for (auto [key, val] : _ent.properties)
			{
				ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(0.0f, 0.0f));
				ImGui::TableNextRow();

				ImGui::TableNextColumn();
				ImGui::Text(key.c_str());

				ImGui::TableNextColumn();
				std::string valID = std::string("##Val") + key;
				ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
				ImGui::InputText(valID.c_str(), _valBuffers[key], TEXT_FIELD_MAX);

				ImGui::TableNextColumn();

				std::string xButtonStr = std::string("Remove##Erase") + key;
				if (ImGui::Button(xButtonStr.c_str()))
				{
					keysToErase.push_back(key);
				}
				ImGui::PopStyleVar(1);
			}

			// Remove keys
			for (std::string key : keysToErase)
			{
				_ent.properties.erase(key);
				free(_valBuffers[key]);
				_valBuffers.erase(key);
			}

			// Update the entity's properties to reflect the changed values in the buffers
			for (const auto& [key, val] : _valBuffers)
			{
				_ent.properties[key] = std::string(val);
			}

			ImGui::TableNextRow();
			ImGui::TableNextColumn();
			ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
			ImGui::InputText("##New key", _newKeyBuffer, TEXT_FIELD_MAX);
			ImGui::TableNextColumn();
			ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
			ImGui::InputText("##New val", _newValBuffer, TEXT_FIELD_MAX);
			ImGui::TableNextColumn();
			std::string pButtonStr = std::string("Add##AddKey");
			if (ImGui::Button(pButtonStr.c_str()))
			{
				std::string key = std::string(_newKeyBuffer);
				_ent.properties[key] = std::string(_newValBuffer);

				_valBuffers[key] = SAFE_REALLOC(char, _valBuffers[key], TEXT_FIELD_MAX);
				strcpy(_valBuffers[key], _newValBuffer);

				memset(_newKeyBuffer, 0, sizeof(char) * TEXT_FIELD_MAX);
				memset(_newValBuffer, 0, sizeof(char) * TEXT_FIELD_MAX);
			}

			ImGui::EndTable();
		}

		ImGui::End();
	}
}
