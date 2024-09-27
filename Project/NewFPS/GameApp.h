#pragma 

#include "World.h"
#include "PerFrame.h"

namespace game
{
	constexpr auto NumMaxEntities = 512u;

	class Player
	{
	public:
		enum class MovementDirection
		{
			FORWARD,
			LEFT,
			RIGHT,
			BACKWARD
		};

		Player()
		{
			Setup();
		}

		// Initialize the location of this person.
		void Setup()
		{
			location = float3(0, 1, 0);
			azimuth = pi<float>() / 2.0f;
			altitude = pi<float>() / 2.0f;
			rateOfMove = 0.2f;
			rateOfTurn = 0.02f;
		}

		// Move the location of this person in @param dir direction for @param distance units.
		// All the symbolic directions are computed using the current direction where the person is looking at (azimuth).
		void Move(MovementDirection dir, float distance);

		// Change the location where the person is looking at by turning @param deltaAzimuth radians and looking up @param deltaAltitude radians. @param deltaAzimuth is an angle in the range [0, 2pi].  @param deltaAltitude is an angle in the range [0, pi].
		void Turn(float deltaAzimuth, float deltaAltitude);

		// Return the coordinates in world space that the person is looking at.
		const float3 GetLookAt() const { return location + SphericalToCartesian(azimuth, altitude); }

		// Return the location of the person in world space.
		const float3& GetLocation() const { return location; }

		float GetAzimuth() const { return azimuth; }
		float GetAltitude() const { return altitude; }
		float GetRateOfMove() const { return rateOfMove; }
		float GetRateOfTurn() const { return rateOfTurn; }

	private:
		// Coordinate in world space where the person is standing.
		float3 location;

		// Spherical coordinates in world space where the person is looking at.
		// azimuth is an angle in the range [0, 2pi].
		// altitude is an angle in the range [0, pi].
		float azimuth;
		float altitude;

		// Rate of motion (grid units) and turning (radians).
		float rateOfMove;
		float rateOfTurn;
	};
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

private:
	World m_world;

private:
	bool setupDescriptors();
	bool setupEntities();
	bool setupPipelines();
	bool setupShadowRenderPass();
	bool setupShadowInfo();
	bool setupLight();
	bool setupCamera();
	void updateCamera(PerspCamera* camera);

	void updateLight();
	void processInput();

	void updateUniformBuffer();

	PerspCamera m_perspCamera;
	std::set<KeyCode> m_pressedKeys;
	game::Player m_oldPlayer;

	struct Entity
	{
		float3           translate = float3(0, 0, 0);
		float3           rotate = float3(0, 0, 0);
		float3           scale = float3(1, 1, 1);
		vkr::MeshPtr          mesh;
		vkr::DescriptorSetPtr drawDescriptorSet;
		vkr::BufferPtr        drawUniformBuffer;
		vkr::DescriptorSetPtr shadowDescriptorSet;
		vkr::BufferPtr        shadowUniformBuffer;
	};

	void setupEntity(
		const vkr::TriMesh& mesh,
		vkr::DescriptorPool* pDescriptorPool,
		const vkr::DescriptorSetLayout* pDrawSetLayout,
		const vkr::DescriptorSetLayout* pShadowSetLayout,
		Entity* pEntity);

	std::vector<VulkanPerFrameData> mPerFrame;

	vkr::DescriptorPoolPtr      m_descriptorPool;

	vkr::DescriptorSetLayoutPtr m_drawObjectSetLayout;
	vkr::PipelineInterfacePtr   mDrawObjectPipelineInterface;
	vkr::GraphicsPipelinePtr    mDrawObjectPipeline;
	Entity                 mGroundPlane;
	Entity                 mCube;
	Entity                 mKnob;
	std::vector<Entity*>   mEntities;


	vkr::DescriptorSetLayoutPtr m_shadowSetLayout;
	vkr::PipelineInterfacePtr   mShadowPipelineInterface;
	vkr::GraphicsPipelinePtr    mShadowPipeline;
	vkr::RenderPassPtr          mShadowRenderPass;
	vkr::SampledImageViewPtr    mShadowImageView;
	vkr::SamplerPtr             mShadowSampler;

	vkr::DescriptorSetLayoutPtr mLightSetLayout;
	vkr::PipelineInterfacePtr   mLightPipelineInterface;
	vkr::GraphicsPipelinePtr    mLightPipeline;
	Entity                 mLight;
	float3                 mLightPosition = float3(0, 5, 5);
	PerspCamera            mLightCamera;
	bool                   mUsePCF = false;

	bool m_cursorVisible = true;
};