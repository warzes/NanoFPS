#pragma once

// Number of entities. Add one more for the floor.
#define kNumEntities (45 + 1)

// Size of the world grid.
#define kGridDepth 100
#define kGridWidth 100

class Example_015 final : public EngineApplication
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
	class Entity
	{
	public:
		enum class EntityKind
		{
			INVALID = -1,
			FLOOR = 0,
			TRI_MESH = 1,
			OBJECT = 2
		};

		Entity()
			: mesh(nullptr),
			descriptorSet(nullptr),
			uniformBuffer(nullptr),
			pipeline(nullptr),
			location(0, 0, 0),
			dimension(0, 0, 0),
			kind(EntityKind::INVALID) {
		}

		Entity(float3 location, float3 dimension, EntityKind kind)
			: mesh(nullptr),
			descriptorSet(nullptr),
			uniformBuffer(nullptr),
			pipeline(nullptr),
			location(location),
			dimension(dimension),
			kind(kind) {
		}

		// Place this entity in a random location within the given sub-grid index.
		// @param subGridIx - Index identifying the sub-grid where the object should be randomly positioned.
		// @param subGridWidth - Width of the sub-grid.
		// @param subGridDepth - Depth of the sub-grid.
		void Place(int32_t subGridIx, Random& random, const int2& gridDim, const int2& subGridDim);

		EntityKind Kind() const { return kind; }
		bool       IsMesh() const { return Kind() == EntityKind::TRI_MESH; }
		bool       IsFloor() const { return Kind() == EntityKind::FLOOR; }
		bool       IsObject() const { return Kind() == EntityKind::OBJECT; }

		DescriptorSet** DescriptorSetPtr() { return &descriptorSet; }
		const ::DescriptorSetPtr DescriptorSet() const { return descriptorSet; }

		const GraphicsPipelinePtr Pipeline() const { return pipeline; }
		GraphicsPipeline** PipelinePtr() { return &pipeline; }

		const MeshPtr& Mesh() const { return mesh; }
		::Mesh** MeshPtr() { return &mesh; }

		const BufferPtr& UniformBuffer() const { return uniformBuffer; }
		Buffer** UniformBufferPtr() { return &uniformBuffer; }

		const float3& Location() const { return location; }

	private:
		::MeshPtr             mesh;
		::DescriptorSetPtr    descriptorSet;
		BufferPtr           uniformBuffer;
		GraphicsPipelinePtr pipeline;
		float3                    location;
		float3                    dimension;
		EntityKind                kind;
	};

	class Person
	{
	public:
		enum class MovementDirection
		{
			FORWARD,
			LEFT,
			RIGHT,
			BACKWARD
		};

		Person()
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

	struct PerFrame
	{
		CommandBufferPtr cmd;
		SemaphorePtr     imageAcquiredSemaphore;
		FencePtr         imageAcquiredFence;
		SemaphorePtr     renderCompleteSemaphore;
		FencePtr         renderCompleteFence;
	};

	std::vector<PerFrame>  mPerFrame;
	ShaderModulePtr        mVS;
	ShaderModulePtr        mPS;
	PipelineInterfacePtr   mPipelineInterface;
	DescriptorPoolPtr      mDescriptorPool;
	DescriptorSetLayoutPtr mDescriptorSetLayout;
	std::vector<Entity>    mEntities;
	PerspCamera            mPerspCamera;
	ArcballCamera          mArcballCamera;
	PerspCamera*           mCurrentCamera;
	std::set<KeyCode>      mPressedKeys;
	Person                 mPerson;

	void setupDescriptors();
	void setupPipelines();
	void setupPerFrameData();
	void setupCamera();
	void updateCamera(PerspCamera* camera);
	void setupEntities();
	void setupEntity(const TriMesh& mesh, const GeometryCreateInfo& createInfo, Entity* pEntity);
	void setupEntity(const WireMesh& mesh, const GeometryCreateInfo& createInfo, Entity* pEntity);
	void processInput();
	void drawCameraInfo();
	void drawInstructions();
};