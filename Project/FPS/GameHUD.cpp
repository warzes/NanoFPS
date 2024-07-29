#include "GameHUD.h"

extern std::unique_ptr<PbrRenderer> renderer;

GameHUD::GameHUD()
{
	m_textRenderer = new BitmapTextRenderer{ renderer.get(),
		"textures/bitmap_fonts/share_tech_mono_shadow.png",
		{ 23.0f, 48.0f }
	};

	m_whiteTexture = renderer->LoadSingleTextureMaterial("textures/dev/white.png");
}

GameHUD::~GameHUD()
{
	delete m_textRenderer;
}

void GameHUD::Update(float deltaTime)
{
	if (m_fadeTimer > 0.0f)
		m_fadeTimer = glm::max(0.0f, m_fadeTimer - deltaTime);

	if (m_objectiveNumDisplayedChars < m_objective.length())
	{
		if (m_objectivePrintTimer <= 0.0f)
		{
			m_objectivePrintTimer += 0.05f;
			m_objectiveNumDisplayedChars++;
			m_objectiveDisplayedText = m_objective.substr(0, m_objectiveNumDisplayedChars);
			if (!std::isspace(m_objectiveDisplayedText.back()))
			{
				//Audio->PlayOneShot("event:/ui/objective/type");
			}
		}
		m_objectivePrintTimer -= deltaTime;
	}
}

void GameHUD::Draw()
{
	const glm::vec2 screenExtent = renderer->GetScreenExtent();

	if (m_fadeTimer > 0.0f)
	{
		const float fade = m_fadeTimer / m_fadeDuration;
		renderer->DrawScreenRect(
			{ 0.0f, 0.0f },
			screenExtent,
			{ 0.0f, 0.0f },
			{ 1.0f, 1.0f },
			{ m_fadeColor.x,
			m_fadeColor.y,
			m_fadeColor.z,
			m_fadeTo ? 1.0f - fade : fade },
			m_whiteTexture
		);
	}

	m_textRenderer->DrawText(
		m_objectiveDisplayedText,
		{ 48.0f, screenExtent.y - 48.0f - m_textRenderer->GetCharSize().y },
		{ 1.0f, 1.0f, 1.0f, 1.0f }
	);
}