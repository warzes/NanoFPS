﻿#pragma once

#include "NanoEngineVK.h"

class GameHUD final
{
public:
	GameHUD();
	GameHUD(GameHUD&&) = delete;
	~GameHUD();
	GameHUD(const GameHUD&) = delete;

	GameHUD& operator=(const GameHUD&) = delete;
	GameHUD& operator=(GameHUD&&) = delete;

	void Update(float deltaTime);

	void Draw();

	void FadeToColor(float duration, const glm::vec3& color)
	{
		m_fadeDuration = duration;
		m_fadeTimer = duration;
		m_fadeTo = true;
		m_fadeColor = color;
	}

	void FadeFromColor(float duration, const glm::vec3& color)
	{
		m_fadeDuration = duration;
		m_fadeTimer = duration;
		m_fadeTo = false;
		m_fadeColor = color;
	}

	void ShowObjective(std::string objective)
	{
		m_objective = std::move(objective);
		m_objectiveNumDisplayedChars = 0;
		m_objectivePrintTimer = 0.0f;
	}

private:
	BitmapTextRenderer*    m_textRenderer;
	SingleTextureMaterial* m_whiteTexture;

	float       m_fadeDuration = 0.0f;
	float       m_fadeTimer = 0.0f;
	bool        m_fadeTo = false; // true: transparent->color; false: color->transparent
	glm::vec3   m_fadeColor{};

	std::string m_objective;
	int         m_objectiveNumDisplayedChars = 0;
	float       m_objectivePrintTimer = 0.0f;
	std::string m_objectiveDisplayedText;
};