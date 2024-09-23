#include "stdafx.h"
#include "015_CameraMotion.h"

EngineApplicationCreateInfo Example_015::Config() const
{
	EngineApplicationCreateInfo createInfo{};
	createInfo.render.swapChain.depthFormat = vkr::FORMAT_D32_FLOAT;
	createInfo.render.showImgui = true;
	return createInfo;
}

bool Example_015::Setup()
{
	setupDescriptors();
	setupEntities();
	setupPipelines();
	setupPerFrameData();
	setupCamera();

	return true;
}

void Example_015::Shutdown()
{
	mPerFrame.clear();
	// TODO: доделать очистку
}

void Example_015::Update()
{
}

void Example_015::Render()
{
	auto& render = GetRender();
	auto& swapChain = render.GetSwapChain();
	PerFrame& frame = mPerFrame[0];

	// Wait for and reset render complete fence
	CHECKED_CALL(frame.renderCompleteFence->WaitAndReset());

	uint32_t imageIndex = UINT32_MAX;
	CHECKED_CALL(swapChain.AcquireNextImage(UINT64_MAX, frame.imageAcquiredSemaphore, frame.imageAcquiredFence, &imageIndex));

	// Wait for and reset image acquired fence
	CHECKED_CALL(frame.imageAcquiredFence->WaitAndReset());

	// Update uniform buffers
	{
		processInput();

		const float4x4& P = mCurrentCamera->GetProjectionMatrix();
		const float4x4& V = mCurrentCamera->GetViewMatrix();

		for (auto& entity : mEntities) {
			float4x4 T = glm::translate(entity.Location());
			float4x4 mat = P * V * T;
			entity.UniformBuffer()->CopyFromSource(sizeof(mat), &mat);
		}
	}

	// Build command buffer
	CHECKED_CALL(frame.cmd->Begin());
	{
		vkr::RenderPassPtr renderPass = swapChain.GetRenderPass(imageIndex);
		ASSERT_MSG(!renderPass.IsNull(), "render pass object is null");

		vkr::RenderPassBeginInfo beginInfo = {};
		beginInfo.pRenderPass = renderPass;
		beginInfo.renderArea = renderPass->GetRenderArea();
		beginInfo.RTVClearCount = 1;
		beginInfo.RTVClearValues[0] = { {0, 0, 0, 0} };
		beginInfo.DSVClearValue = { 1.0f, 0xFF };

		frame.cmd->TransitionImageLayout(renderPass->GetRenderTargetImage(0), ALL_SUBRESOURCES, vkr::RESOURCE_STATE_PRESENT, vkr::RESOURCE_STATE_RENDER_TARGET);
		frame.cmd->BeginRenderPass(&beginInfo);
		{
			frame.cmd->SetScissors(render.GetScissor());
			frame.cmd->SetViewports(render.GetViewport());

			for (auto& entity : mEntities) {
				frame.cmd->BindGraphicsPipeline(entity.Pipeline());
				frame.cmd->BindGraphicsDescriptorSets(mPipelineInterface, 1, entity.DescriptorSetPtr());
				frame.cmd->BindIndexBuffer(entity.Mesh());
				frame.cmd->BindVertexBuffers(entity.Mesh());
				frame.cmd->DrawIndexed(entity.Mesh()->GetIndexCount());
			}

			// Draw ImGui
			render.DrawDebugInfo();
			drawCameraInfo();
			drawInstructions();
			render.DrawImGui(frame.cmd);
		}
		frame.cmd->EndRenderPass();
		frame.cmd->TransitionImageLayout(renderPass->GetRenderTargetImage(0), ALL_SUBRESOURCES, vkr::RESOURCE_STATE_RENDER_TARGET, vkr::RESOURCE_STATE_PRESENT);
	}
	CHECKED_CALL(frame.cmd->End());

	vkr::SubmitInfo submitInfo = {};
	submitInfo.commandBufferCount = 1;
	submitInfo.ppCommandBuffers = &frame.cmd;
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.ppWaitSemaphores = &frame.imageAcquiredSemaphore;
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.ppSignalSemaphores = &frame.renderCompleteSemaphore;
	submitInfo.pFence = frame.renderCompleteFence;

	CHECKED_CALL(render.GetGraphicsQueue()->Submit(&submitInfo));

	CHECKED_CALL(swapChain.Present(imageIndex, 1, &frame.renderCompleteSemaphore));
}

void Example_015::MouseMove(int32_t x, int32_t y, int32_t dx, int32_t dy, MouseButton buttons)
{
	float2 prevPos = GetRender().GetNormalizedDeviceCoordinates(x - dx, y - dy);
	float2 curPos = GetRender().GetNormalizedDeviceCoordinates(x, y);
	float2 deltaPos = prevPos - curPos;
	float  deltaAzimuth = deltaPos[0] * pi<float>() / 4.0f;
	float  deltaAltitude = deltaPos[1] * pi<float>() / 2.0f;
	mPerson.Turn(-deltaAzimuth, deltaAltitude);
	updateCamera(mCurrentCamera);
}

void Example_015::KeyDown(KeyCode key)
{
	mPressedKeys.insert(key);
}

void Example_015::KeyUp(KeyCode key)
{
	mPressedKeys.erase(key);
}

void Example_015::setupEntity(const vkr::TriMesh& mesh, const vkr::GeometryCreateInfo& createInfo, Entity* pEntity)
{
	CHECKED_CALL(vkr::grfx_util::CreateMeshFromTriMesh(GetRender().GetGraphicsQueue(), &mesh, pEntity->MeshPtr()));

	vkr::BufferCreateInfo bufferCreateInfo = {};
	bufferCreateInfo.size = RoundUp(512, CONSTANT_BUFFER_ALIGNMENT);
	bufferCreateInfo.usageFlags.bits.uniformBuffer = true;
	bufferCreateInfo.memoryUsage = vkr::MEMORY_USAGE_CPU_TO_GPU;
	CHECKED_CALL(GetRenderDevice().CreateBuffer(bufferCreateInfo, pEntity->UniformBufferPtr()));

	CHECKED_CALL(GetRenderDevice().AllocateDescriptorSet(mDescriptorPool, mDescriptorSetLayout, pEntity->DescriptorSetPtr()));

	vkr::WriteDescriptor write = {};
	write.binding = 0;
	write.type = vkr::DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	write.bufferOffset = 0;
	write.bufferRange = WHOLE_SIZE;
	write.pBuffer = pEntity->UniformBuffer();
	CHECKED_CALL(pEntity->DescriptorSet()->UpdateDescriptors(1, &write));
}

void Example_015::setupEntity(const vkr::WireMesh& mesh, const vkr::GeometryCreateInfo& createInfo, Entity* pEntity)
{
	CHECKED_CALL(vkr::grfx_util::CreateMeshFromWireMesh(GetRender().GetGraphicsQueue(), &mesh, pEntity->MeshPtr()));

	vkr::BufferCreateInfo bufferCreateInfo = {};
	bufferCreateInfo.size = MINIMUM_UNIFORM_BUFFER_SIZE;
	bufferCreateInfo.usageFlags.bits.uniformBuffer = true;
	bufferCreateInfo.memoryUsage = vkr::MEMORY_USAGE_CPU_TO_GPU;
	CHECKED_CALL(GetRenderDevice().CreateBuffer(bufferCreateInfo, pEntity->UniformBufferPtr()));

	CHECKED_CALL(GetRenderDevice().AllocateDescriptorSet(mDescriptorPool, mDescriptorSetLayout, pEntity->DescriptorSetPtr()));

	vkr::WriteDescriptor write = {};
	write.binding = 0;
	write.type = vkr::DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	write.bufferOffset = 0;
	write.bufferRange = WHOLE_SIZE;
	write.pBuffer = pEntity->UniformBuffer();
	CHECKED_CALL(pEntity->DescriptorSet()->UpdateDescriptors(1, &write));
}

void Example_015::Entity::Place(int32_t subGridIx, Random& random, const int2& gridDim, const int2& subGridDim)
{
	// The original grid has been split in equal-sized sub-grids that preserve the same ratio. There are as many sub-grids as entities to place.  The entity will be placed at random inside the sub-grid with index @param subGridIx.
	// Each sub-grid is assumed to have its origin at top-left.
	int32_t sgx = random.UInt32() % subGridDim[0];
	int32_t sgz = random.UInt32() % subGridDim[1];
	::Print("Object location in grid #" + std::to_string(subGridIx) + ": (" + std::to_string(sgx) + ", " + std::to_string(sgz) + ")");

	// Translate the location relative to the sub-grid location to the main grid coordinates.
	int32_t xDisplacement = subGridDim[0] * subGridIx;
	int32_t x = (xDisplacement + sgx) % gridDim[0];
	int32_t z = sgz + (subGridDim[1] * (xDisplacement / gridDim[0]));
	::Print("xDisplacement: " + std::to_string(xDisplacement));
	::Print("Object location in main grid: (" + std::to_string(x) + ", " + std::to_string(z) + ")");

	// All the calculations above assume that the main grid has its origin at the top-left corner, but grids have their origin in the center.  So, we need to shift the location accordingly.
	int32_t adjX = x - gridDim[0] / 2;
	int32_t adjZ = z - gridDim[1] / 2;
	::Print("Adjusted object location in main grid: (" + std::to_string(adjX) + ", " + std::to_string(adjZ) + ")");
	location = float3(adjX, 1, adjZ);
}

void Example_015::setupEntities()
{
	vkr::GeometryCreateInfo geometryCreateInfo = vkr::GeometryCreateInfo::Planar().AddColor();

	// Each object will live in a square region on the grid.  The size of each grid depends on how many objects we need to place.  Note that since the first entity is the grid itself, we ignore it here.
	int numObstacles = (kNumEntities > 1) ? kNumEntities - 1 : 0;
	ASSERT_MSG(numObstacles > 0, "There should be at least 1 obstacle in the grid");

	// Using the total area of the main grid and the grid ratio, compute the height and width of each sub-grid where each object will be placed at random. Each sub-grid has the same ratio as the original grid.
	//
	// To compute the depth (SGD) and width (SGW) of each sub-grid, we start with:
	//
	// Grid area:  A = kGridWidth * kGridDepth
	// Grid ratio: R = kGridWidth / kGridDepth
	// Number of objects: N
	// Sub-grid area: SGA = A / N
	//
	// SGA = SGW * SGD
	// R = SGW / SGD
	//
	// Solving for SGW and SGD, we get:
	//
	// SGD = sqrt(SGA / R)
	// SGW = SGA / SGD
	float gridRatio = static_cast<float>(kGridWidth) / static_cast<float>(kGridDepth);
	float subGridArea = (kGridWidth * kGridDepth) / static_cast<float>(numObstacles);
	float subGridDepth = std::sqrt(subGridArea / gridRatio);
	float subGridWidth = subGridArea / subGridDepth;

	Random random;
	for (int32_t i = 0; i < kNumEntities; ++i)
	{
		float3 location, dimension;

		if (i == 0) {
			// The first object is the mesh plane where all the other entities are placed.
			vkr::WireMeshOptions wireMeshOptions = vkr::WireMeshOptions().Indices().VertexColors();
			vkr::WireMesh        wireMesh = vkr::WireMesh::CreatePlane(vkr::WIRE_MESH_PLANE_POSITIVE_Y, float2(kGridWidth, kGridDepth), 100, 100, wireMeshOptions);
			dimension = float3(kGridWidth, 0, kGridDepth);
			location = float3(0, 0, 0);
			auto& entity = mEntities.emplace_back(location, dimension, Entity::EntityKind::FLOOR);
			setupEntity(wireMesh, geometryCreateInfo, &entity);
		}
		else {
			vkr::TriMesh            triMesh;
			uint32_t           distribution = random.UInt32() % 100;
			Entity::EntityKind kind = Entity::EntityKind::INVALID;

			// NOTE: vkr::TriMeshOptions added here must match the number of bindings when creating this entity's pipeline.
			// See the handling of different entities in ProjApp::SetupPipelines.
			if (distribution <= 60) {
				dimension = float3(2, 2, 2);
				vkr::TriMeshOptions options = vkr::TriMeshOptions().Indices().VertexColors();
				triMesh = (distribution <= 30) ? vkr::TriMesh::CreateCube(dimension, options) : vkr::TriMesh::CreateSphere(dimension[0] / 2, 100, 100, options);
				kind = Entity::EntityKind::TRI_MESH;
			}
			else {
				float3         lb = { 0, 0, 0 };
				float3         ub = { 1, 1, 1 };
				vkr::TriMeshOptions options = vkr::TriMeshOptions().Indices().ObjectColor(random.Float3(lb, ub));
				triMesh = vkr::TriMesh::CreateFromOBJ("basic/models/monkey.obj", options);
				kind = Entity::EntityKind::OBJECT;
				dimension = triMesh.GetBoundingBoxMax();
				Print("Object dimension: (" + std::to_string(dimension[0]) + ", " + std::to_string(dimension[1]) + ", " + std::to_string(dimension[2]) + ")");
			}

			// Create the entity and compute a random location for it.  The location is computed within the boundaries of the object's home grid.
			auto& entity = mEntities.emplace_back(float3(0, 0, 0), dimension, kind);
			entity.Place(i - 1, random, int2(kGridWidth, kGridDepth), int2(subGridWidth, subGridDepth));
			setupEntity(triMesh, geometryCreateInfo, &entity);
		}
	}
}

void Example_015::setupDescriptors()
{
	vkr::DescriptorPoolCreateInfo poolCreateInfo = {};
	poolCreateInfo.uniformBuffer = kNumEntities;
	CHECKED_CALL(GetRenderDevice().CreateDescriptorPool(poolCreateInfo, &mDescriptorPool));

	vkr::DescriptorSetLayoutCreateInfo layoutCreateInfo = {};
	layoutCreateInfo.bindings.push_back(vkr::DescriptorBinding{ 0, vkr::DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, vkr::SHADER_STAGE_ALL_GRAPHICS });
	CHECKED_CALL(GetRenderDevice().CreateDescriptorSetLayout(layoutCreateInfo, &mDescriptorSetLayout));
}

void Example_015::setupPipelines()
{
	CHECKED_CALL(GetRenderDevice().CreateShader("basic/shaders", "VertexColors.vs", &mVS));
	CHECKED_CALL(GetRenderDevice().CreateShader("basic/shaders", "VertexColors.ps", &mPS));

	vkr::PipelineInterfaceCreateInfo piCreateInfo = {};
	piCreateInfo.setCount = 1;
	piCreateInfo.sets[0].set = 0;
	piCreateInfo.sets[0].pLayout = mDescriptorSetLayout;
	CHECKED_CALL(GetRenderDevice().CreatePipelineInterface(piCreateInfo, &mPipelineInterface));

	vkr::GraphicsPipelineCreateInfo2 gpCreateInfo = {};
	gpCreateInfo.VS = { mVS.Get(), "vsmain" };
	gpCreateInfo.PS = { mPS.Get(), "psmain" };
	gpCreateInfo.polygonMode = vkr::POLYGON_MODE_FILL;
	gpCreateInfo.cullMode = vkr::CULL_MODE_BACK;
	gpCreateInfo.frontFace = vkr::FRONT_FACE_CCW;
	gpCreateInfo.depthReadEnable = true;
	gpCreateInfo.depthWriteEnable = true;
	gpCreateInfo.blendModes[0] = vkr::BLEND_MODE_NONE;
	gpCreateInfo.outputState.renderTargetCount = 1;
	gpCreateInfo.outputState.renderTargetFormats[0] = GetRender().GetSwapChain().GetColorFormat();
	gpCreateInfo.outputState.depthStencilFormat = GetRender().GetSwapChain().GetDepthFormat();
	gpCreateInfo.pPipelineInterface = mPipelineInterface;

	for (auto& entity : mEntities) {
		// NOTE: Number of vertex input bindings here must match the number of options added to each entity in ProjApp::SetupEntities.
		if (entity.IsFloor() || entity.IsMesh()) {
			gpCreateInfo.topology = (entity.IsFloor()) ? vkr::PRIMITIVE_TOPOLOGY_LINE_LIST : vkr::PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
			gpCreateInfo.vertexInputState.bindingCount = 2;
			gpCreateInfo.vertexInputState.bindings[0] = entity.Mesh()->GetDerivedVertexBindings()[0];
			gpCreateInfo.vertexInputState.bindings[1] = entity.Mesh()->GetDerivedVertexBindings()[1];
		}
		else if (entity.IsObject()) {
			gpCreateInfo.topology = vkr::PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
			gpCreateInfo.vertexInputState.bindingCount = 2;
			gpCreateInfo.vertexInputState.bindings[0] = entity.Mesh()->GetDerivedVertexBindings()[0];
			gpCreateInfo.vertexInputState.bindings[1] = entity.Mesh()->GetDerivedVertexBindings()[1];
		}
		else {
			ASSERT_MSG(false, "Unrecognized entity kind: " + std::to_string(static_cast<int>(entity.Kind())));
		}
		CHECKED_CALL(GetRenderDevice().CreateGraphicsPipeline(gpCreateInfo, entity.PipelinePtr()));
	}
}

void Example_015::setupPerFrameData()
{
	PerFrame frame = {};

	CHECKED_CALL(GetRender().GetGraphicsQueue()->CreateCommandBuffer(&frame.cmd));

	vkr::SemaphoreCreateInfo semaCreateInfo = {};
	CHECKED_CALL(GetRenderDevice().CreateSemaphore(semaCreateInfo, &frame.imageAcquiredSemaphore));

	vkr::FenceCreateInfo fenceCreateInfo = {};
	CHECKED_CALL(GetRenderDevice().CreateFence(fenceCreateInfo, &frame.imageAcquiredFence));

	CHECKED_CALL(GetRenderDevice().CreateSemaphore(semaCreateInfo, &frame.renderCompleteSemaphore));

	fenceCreateInfo = { true };
	CHECKED_CALL(GetRenderDevice().CreateFence(fenceCreateInfo, &frame.renderCompleteFence));

	mPerFrame.push_back(frame);
}

void Example_015::setupCamera()
{
	mPerson.Setup();
	mCurrentCamera = &mPerspCamera;
	updateCamera(&mPerspCamera);
	updateCamera(&mArcballCamera);
}

void Example_015::updateCamera(PerspCamera* camera)
{
	float3 cameraPosition(0, 0, 0);
	if (camera == &mPerspCamera) {
		cameraPosition = mPerson.GetLocation();
	}
	else {
		cameraPosition = mPerson.GetLocation() + float3(0, 1, -5);
	}
	camera->LookAt(cameraPosition, mPerson.GetLookAt(), CAMERA_DEFAULT_WORLD_UP);
	camera->SetPerspective(60.f, GetWindowAspect());
}

void Example_015::Person::Move(MovementDirection dir, float distance)
{
	if (dir == MovementDirection::FORWARD) {
		location += float3(distance * std::cos(azimuth), 0, distance * std::sin(azimuth));
	}
	else if (dir == MovementDirection::LEFT) {
		float perpendicularDir = azimuth - pi<float>() / 2.0f;
		location += float3(distance * std::cos(perpendicularDir), 0, distance * std::sin(perpendicularDir));
	}
	else if (dir == MovementDirection::RIGHT) {
		float perpendicularDir = azimuth + pi<float>() / 2.0f;
		location += float3(distance * std::cos(perpendicularDir), 0, distance * std::sin(perpendicularDir));
	}
	else if (dir == MovementDirection::BACKWARD) {
		location += float3(-distance * std::cos(azimuth), 0, -distance * std::sin(azimuth));
	}
	else {
		//ASSERT_MSG(false, "unhandled direction code ");
	}
}

void Example_015::Person::Turn(float deltaAzimuth, float deltaAltitude)
{
	azimuth += deltaAzimuth;
	altitude += deltaAltitude;

	// Saturate azimuth values by making wrap around.
	if (azimuth < 0) {
		azimuth = 2 * pi<float>();
	}
	else if (azimuth > 2 * pi<float>()) {
		azimuth = 0;
	}

	// Altitude is saturated by making it stop, so the world doesn't turn upside down.
	if (altitude < 0) {
		altitude = 0;
	}
	else if (altitude > pi<float>()) {
		altitude = pi<float>();
	}
}

void Example_015::processInput()
{
	if (mPressedKeys.empty()) {
		return;
	}

	if (mPressedKeys.count(KEY_W) > 0) {
		mPerson.Move(Person::MovementDirection::FORWARD, mPerson.GetRateOfMove());
	}

	if (mPressedKeys.count(KEY_A) > 0) {
		mPerson.Move(Person::MovementDirection::LEFT, mPerson.GetRateOfMove());
	}

	if (mPressedKeys.count(KEY_S) > 0) {
		mPerson.Move(Person::MovementDirection::BACKWARD, mPerson.GetRateOfMove());
	}

	if (mPressedKeys.count(KEY_D) > 0) {
		mPerson.Move(Person::MovementDirection::RIGHT, mPerson.GetRateOfMove());
	}

	if (mPressedKeys.count(KEY_SPACE) > 0) {
		setupCamera();
		return;
	}

	if (mPressedKeys.count(KEY_1) > 0) {
		mCurrentCamera = &mPerspCamera;
	}

	if (mPressedKeys.count(KEY_2) > 0) {
		mCurrentCamera = &mArcballCamera;
	}

	if (mPressedKeys.count(KEY_LEFT) > 0) {
		mPerson.Turn(-mPerson.GetRateOfTurn(), 0);
	}

	if (mPressedKeys.count(KEY_RIGHT) > 0) {
		mPerson.Turn(mPerson.GetRateOfTurn(), 0);
	}

	if (mPressedKeys.count(KEY_UP) > 0) {
		mPerson.Turn(0, -mPerson.GetRateOfTurn());
	}

	if (mPressedKeys.count(KEY_DOWN) > 0) {
		mPerson.Turn(0, mPerson.GetRateOfTurn());
	}

	updateCamera(mCurrentCamera);
}

void Example_015::drawInstructions()
{
	if (ImGui::Begin("Instructions")) {
		ImGui::Columns(2);

		ImGui::Text("Movement keys");
		ImGui::NextColumn();
		ImGui::Text("W, A, S, D ");
		ImGui::NextColumn();

		ImGui::Text("Turn and look");
		ImGui::NextColumn();
		ImGui::Text("Arrow keys and mouse");
		ImGui::NextColumn();

		ImGui::Text("Cameras");
		ImGui::NextColumn();
		ImGui::Text("1 (perspective), 2 (arcball)");
		ImGui::NextColumn();

		ImGui::Text("Reset view");
		ImGui::NextColumn();
		ImGui::Text("space");
		ImGui::NextColumn();
	}
	ImGui::End();
}

void Example_015::drawCameraInfo()
{
	ImGui::Separator();

	ImGui::Columns(2);
	ImGui::Text("Camera position");
	ImGui::NextColumn();
	ImGui::Text("(%.4f, %.4f, %.4f)", mCurrentCamera->GetEyePosition()[0], mCurrentCamera->GetEyePosition()[1], mCurrentCamera->GetEyePosition()[2]);
	ImGui::NextColumn();

	ImGui::Columns(2);
	ImGui::Text("Camera looking at");
	ImGui::NextColumn();
	ImGui::Text("(%.4f, %.4f, %.4f)", mCurrentCamera->GetTarget()[0], mCurrentCamera->GetTarget()[1], mCurrentCamera->GetTarget()[2]);
	ImGui::NextColumn();

	ImGui::Separator();

	ImGui::Columns(2);
	ImGui::Text("Person location");
	ImGui::NextColumn();
	ImGui::Text("(%.4f, %.4f, %.4f)", mPerson.GetLocation()[0], mPerson.GetLocation()[1], mPerson.GetLocation()[2]);
	ImGui::NextColumn();

	ImGui::Columns(2);
	ImGui::Text("Person looking at");
	ImGui::NextColumn();
	ImGui::Text("(%.4f, %.4f, %.4f)", mPerson.GetLookAt()[0], mPerson.GetLookAt()[1], mPerson.GetLookAt()[2]);
	ImGui::NextColumn();

	ImGui::Columns(2);
	ImGui::Text("Azimuth");
	ImGui::NextColumn();
	ImGui::Text("%.4f", mPerson.GetAzimuth());
	ImGui::NextColumn();

	ImGui::Columns(2);
	ImGui::Text("Altitude");
	ImGui::NextColumn();
	ImGui::Text("%.4f", mPerson.GetAltitude());
	ImGui::NextColumn();
}