#pragma 

#include "World.h"
#include "GameGraphics.h"

namespace game
{
	constexpr auto NumMaxEntities = 1024u;
} // game

class GameApplication final : public EngineApplication
{
public:
	EngineApplicationCreateInfo Config() const final;

	bool Setup() final;
	void Shutdown() final;
	void Update() final;
	void Render() final;

	void MouseMove(int32_t x, int32_t y, int32_t dx, int32_t dy, MouseButton buttons) final;
	void KeyDown(KeyCode key) final;
	void KeyUp(KeyCode key) final;

	GameGraphics& GetGameGraphics() { return m_gameGraphics; }

private:
	void processInput();

	GameGraphics m_gameGraphics;
	World m_world;
	std::set<KeyCode> m_pressedKeys;

	bool m_cursorVisible = true;
};