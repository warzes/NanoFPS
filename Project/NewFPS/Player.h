#pragma once

class GameApplication;

enum class MovementDirection
{
	Forward,
	Left,
	Right,
	Backward,
	Up,
	Down,
};

struct PlayerCreateInfo final
{
	glm::vec3 position = glm::vec3(0.0f, 1.0f, 0.0f);
};

class Player final
{
public:
	bool Setup(GameApplication* game, const PlayerCreateInfo& createInfo);
	void Shutdown();

	void ProcessInput(const std::set<KeyCode>& pressedKeys);
	void Update(float deltaTime);

	// Change the location where the person is looking at by turning @param deltaAzimuth radians and looking up @param deltaAltitude radians. @param deltaAzimuth is an angle in the range [0, 2pi].  @param deltaAltitude is an angle in the range [0, pi].
	void Turn(float deltaAzimuth, float deltaAltitude);

	// Return the coordinates in world space that the person is looking at.
	const float3 GetLookAt() const { return m_position + SphericalToCartesian(m_azimuth, m_altitude); }

	// Return the location of the person in world space.
	const float3& GetPosition() const { return m_position; }

	float GetAzimuth() const { return m_azimuth; }
	float GetAltitude() const { return m_altitude; }
	float GetRateOfMove() const { return m_rateOfMove; }
	float GetRateOfTurn() const { return m_rateOfTurn; }

	PerspectiveCamera& GetCamera() { return m_perspCamera; }

private:
	void setupCamera();
	void updateCamera();
	// Move the location of this person in @param dir direction for @param distance units.
	// All the symbolic directions are computed using the current direction where the person is looking at (azimuth).
	void move(MovementDirection dir, float distance);

	GameApplication* m_game;

	PerspectiveCamera m_perspCamera;

	float3 m_position = float3(0.0f);

	// Spherical coordinates in world space where the person is looking at.
	// azimuth is an angle in the range [0, 2pi].
	// altitude is an angle in the range [0, pi].
	float m_azimuth = pi<float>() / 2.0f;
	float m_altitude = pi<float>() / 2.0f;

	// Rate of motion (grid units) and turning (radians).
	float m_rateOfMove = 0.2f;
	float m_rateOfTurn = 0.02f;


};