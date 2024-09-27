#pragma once

class GameApplication;

struct PlayerCreateInfo final
{
	glm::vec3 position = glm::vec3(0.0f, 2.0f, -5.0f);
};

class Player final
{
public:
	bool Setup(GameApplication* game, const PlayerCreateInfo& createInfo);
	void Shutdown();

	void Update(float deltaTime);

	bool IsAlive() const;

	void ComputeCameraOrientation();

	glm::vec3 GetPos();
	void SetPos(const glm::vec3& pos);

	glm::vec3 GetCameraLook();
	glm::vec3 GetCameraSide();
	glm::vec3 GetCameraUp();

private:
	void controlMouseInput(float deltaTime);      // turns mouse motion into camera angles
	void computeWalkingVectors();                 // computes the vectors used for walking
	void controlLooking(float deltaTime);         // places limits on the player's viewing angles
	void controlMovingAndFiring(float deltaTime); // controls player motion, firing, and reloading

	GameApplication* m_game;

	glm::vec3 m_pos;           // global position of player
	glm::vec3 m_gunPos;        // position of gun relative to player
	glm::vec3 m_forward;       // forwards vector of player
	glm::vec3 m_side;          // sideway vector of player (used for strafing)
	glm::vec3 m_up;            // up vector of player

	float m_jumpTimer;         // jumping uses a short timed burst of upwards motion; this timer controls it
	float m_gravity;           // current downward pull experienced by player---zero when touching ground
	bool m_touchingGround;     // is the player touching the ground currently?
	bool m_isMoving;           // is the player walking at all, in any direction?

	double m_oldMouseX;        // used to determine mouse motion
	double m_oldMouseY;        // used to determine mouse motion

	float m_targetLookAngleX;  // used to compute smooth camera motion
	float m_targetLookAngleY;  // used to compute smooth camera motion

	float m_lookAngleX;        // is computed using targetLookAngleX for smooth camera motion
	float m_lookAngleY;        // is computed using targetLookAngleY for smooth camera motion

	glm::vec3 m_cameraForward; // forward look direction of camera
	glm::vec3 m_cameraSide;    // sideways vector of camera
	glm::vec3 m_cameraUp;      // cross of cameraForward and cameraSide
	glm::vec3 m_cameraLook;    // the global 3D position where the camera is focused (used for glm::lookAt())

	bool m_alive;              // one hit, one kill---is the player alive?
};