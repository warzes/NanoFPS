#include "stdafx.h"
#include "025_BasicMaterial.h"

EngineApplicationCreateInfo Example_025::Config() const
{
	EngineApplicationCreateInfo createInfo{};
	createInfo.render.swapChain.depthFormat = vkr::Format::D32_FLOAT;
	createInfo.render.showImgui = true;
	return createInfo;
}

bool Example_025::Setup()
{
	auto& device = GetRenderDevice();

	CHECKED_CALL(vkr::vkrUtil::CreateTexture1x1<uint8_t>(device.GetGraphicsQueue(), { 0, 0, 0, 0 }, &m1x1BlackTexture));
	CHECKED_CALL(vkr::vkrUtil::CreateTexture1x1<uint8_t>(device.GetGraphicsQueue(), { 255, 255, 255, 255 }, &m1x1WhiteTexture));
	mF0Index = static_cast<uint32_t>(mF0Names.size() - 1);

	// IBL
	SetupIBL();

	// Cameras
	{
		mCamera = PerspCamera(60.0f, GetWindowAspect());
	}

	{
		vkr::DescriptorPoolCreateInfo createInfo = {};
		createInfo.sampler = 1000;
		createInfo.sampledImage = 1000;
		createInfo.uniformBuffer = 1000;
		createInfo.structuredBuffer = 1000;

		CHECKED_CALL(device.CreateDescriptorPool(createInfo, &mDescriptorPool));
	}

	// Meshes
	std::vector<vkr::VertexBinding> vertexBindings;
	{
		vkr::TriMeshOptions options = vkr::TriMeshOptions().Indices().VertexColors().Normals().TexCoords().Tangents();

		{
			vkr::Geometry geo;
			vkr::TriMesh  mesh = vkr::TriMesh::CreateFromOBJ("basic/models/material_sphere.obj", options);
			CHECKED_CALL(vkr::Geometry::Create(mesh, &geo));
			vkr::MeshPtr gpuMesh;
			CHECKED_CALL(vkr::vkrUtil::CreateMeshFromGeometry(device.GetGraphicsQueue(), &geo, &gpuMesh));
			mMeshes.push_back(gpuMesh);

			// Grab the vertex bindings
			vertexBindings = gpuMesh->GetDerivedVertexBindings();
		}

		{
			vkr::Geometry geo;
			vkr::TriMesh  mesh = vkr::TriMesh::CreateSphere(0.75f, 128, 64, vkr::TriMeshOptions(options).TexCoordScale(float2(2)));
			CHECKED_CALL(vkr::Geometry::Create(mesh, &geo));
			vkr::MeshPtr gpuMesh;
			CHECKED_CALL(vkr::vkrUtil::CreateMeshFromGeometry(device.GetGraphicsQueue(), &geo, &gpuMesh));
			mMeshes.push_back(gpuMesh);
		}

		{
			vkr::Geometry geo;
			vkr::TriMesh  mesh = vkr::TriMesh::CreateCube(float3(1.0f), options);
			CHECKED_CALL(vkr::Geometry::Create(mesh, &geo));
			vkr::MeshPtr gpuMesh;
			CHECKED_CALL(vkr::vkrUtil::CreateMeshFromGeometry(device.GetGraphicsQueue(), &geo, &gpuMesh));
			mMeshes.push_back(gpuMesh);
		}

		{
			vkr::Geometry geo;
			vkr::TriMesh  mesh = vkr::TriMesh::CreateFromOBJ("basic/models/monkey.obj", vkr::TriMeshOptions(options).Scale(float3(0.75f)));
			CHECKED_CALL(vkr::Geometry::Create(mesh, &geo));
			vkr::MeshPtr gpuMesh;
			CHECKED_CALL(vkr::vkrUtil::CreateMeshFromGeometry(device.GetGraphicsQueue(), &geo, &gpuMesh));
			mMeshes.push_back(gpuMesh);
		}

		{
			vkr::Geometry geo;
			vkr::TriMesh  mesh = vkr::TriMesh::CreateFromOBJ("poly_haven/models/measuring_tape/measuring_tape_01.obj", vkr::TriMeshOptions(options).Translate(float3(0, -0.4f, 0)).InvertTexCoordsV());
			CHECKED_CALL(vkr::Geometry::Create(mesh, &geo));
			vkr::MeshPtr gpuMesh;
			CHECKED_CALL(vkr::vkrUtil::CreateMeshFromGeometry(device.GetGraphicsQueue(), &geo, &gpuMesh));
			mMeshes.push_back(gpuMesh);
		}

		{
			vkr::Geometry geo;
			vkr::TriMesh  mesh = vkr::TriMesh::CreateFromOBJ("poly_haven/models/food_kiwi/food_kiwi_01.obj", vkr::TriMeshOptions(options).Translate(float3(0, -0.7f, 0)).InvertTexCoordsV());
			CHECKED_CALL(vkr::Geometry::Create(mesh, &geo));
			vkr::MeshPtr gpuMesh;
			CHECKED_CALL(vkr::vkrUtil::CreateMeshFromGeometry(device.GetGraphicsQueue(), &geo, &gpuMesh));
			mMeshes.push_back(gpuMesh);
		}

		{
			vkr::Geometry geo;
			vkr::TriMesh  mesh = vkr::TriMesh::CreateFromOBJ("poly_haven/models/hand_plane/hand_plane_no4_1k.obj", vkr::TriMeshOptions(options).Translate(float3(0, -0.5f, 0)).InvertTexCoordsV());
			CHECKED_CALL(vkr::Geometry::Create(mesh, &geo));
			vkr::MeshPtr gpuMesh;
			CHECKED_CALL(vkr::vkrUtil::CreateMeshFromGeometry(device.GetGraphicsQueue(), &geo, &gpuMesh));
			mMeshes.push_back(gpuMesh);
		}

		{
			vkr::Geometry geo;
			vkr::TriMesh  mesh = vkr::TriMesh::CreateFromOBJ("poly_haven/models/horse_statue/horse_statue_01_1k.obj", vkr::TriMeshOptions(options).Translate(float3(0, -0.725f, 0)).InvertTexCoordsV());
			CHECKED_CALL(vkr::Geometry::Create(mesh, &geo));
			vkr::MeshPtr gpuMesh;
			CHECKED_CALL(vkr::vkrUtil::CreateMeshFromGeometry(device.GetGraphicsQueue(), &geo, &gpuMesh));
			mMeshes.push_back(gpuMesh);
		}
	}

	// Environment draw mesh
	{
		vkr::Geometry geo;
		vkr::TriMesh  mesh = vkr::TriMesh::CreateSphere(15.0f, 128, 64, vkr::TriMeshOptions().Indices().TexCoords());
		CHECKED_CALL(vkr::Geometry::Create(mesh, &geo));
		CHECKED_CALL(vkr::vkrUtil::CreateMeshFromGeometry(device.GetGraphicsQueue(), &geo, &mEnvDrawMesh));
	}

	// Scene data
	{
		vkr::DescriptorSetLayoutCreateInfo createInfo = {};
		createInfo.bindings.push_back({ vkr::DescriptorBinding{SCENE_CONSTANTS_REGISTER, vkr::DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, vkr::SHADER_STAGE_ALL_GRAPHICS} });
		createInfo.bindings.push_back({ vkr::DescriptorBinding{LIGHT_DATA_REGISTER, vkr::DESCRIPTOR_TYPE_RO_STRUCTURED_BUFFER, 1, vkr::SHADER_STAGE_ALL_GRAPHICS} });
		CHECKED_CALL(device.CreateDescriptorSetLayout(createInfo, &mSceneDataLayout));

		CHECKED_CALL(device.AllocateDescriptorSet(mDescriptorPool, mSceneDataLayout, &mSceneDataSet));

		// Scene constants
		vkr::BufferCreateInfo bufferCreateInfo = {};
		bufferCreateInfo.size = vkr::MINIMUM_CONSTANT_BUFFER_SIZE;
		bufferCreateInfo.usageFlags.bits.transferSrc = true;
		bufferCreateInfo.memoryUsage = vkr::MemoryUsage::CPUToGPU;
		CHECKED_CALL(device.CreateBuffer(bufferCreateInfo, &mCpuSceneConstants));

		bufferCreateInfo.usageFlags.bits.transferDst = true;
		bufferCreateInfo.usageFlags.bits.uniformBuffer = true;
		bufferCreateInfo.memoryUsage = vkr::MemoryUsage::GPUOnly;
		CHECKED_CALL(device.CreateBuffer(bufferCreateInfo, &mGpuSceneConstants));

		// HlslLight constants
		bufferCreateInfo = {};
		bufferCreateInfo.size = vkr::MINIMUM_STRUCTURED_BUFFER_SIZE;
		bufferCreateInfo.usageFlags.bits.transferSrc = true;
		bufferCreateInfo.memoryUsage = vkr::MemoryUsage::CPUToGPU;
		CHECKED_CALL(device.CreateBuffer(bufferCreateInfo, &mCpuLightConstants));

		bufferCreateInfo.structuredElementStride = 32;
		bufferCreateInfo.usageFlags.bits.transferDst = true;
		bufferCreateInfo.usageFlags.bits.roStructuredBuffer = true;
		bufferCreateInfo.memoryUsage = vkr::MemoryUsage::GPUOnly;
		CHECKED_CALL(device.CreateBuffer(bufferCreateInfo, &mGpuLightConstants));

		vkr::WriteDescriptor write = {};
		write.binding = SCENE_CONSTANTS_REGISTER;
		write.arrayIndex = 0;
		write.type = vkr::DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		write.bufferOffset = 0;
		write.bufferRange = WHOLE_SIZE;
		write.pBuffer = mGpuSceneConstants;
		CHECKED_CALL(mSceneDataSet->UpdateDescriptors(1, &write));

		write = {};
		write.binding = LIGHT_DATA_REGISTER;
		write.arrayIndex = 0;
		write.type = vkr::DESCRIPTOR_TYPE_RO_STRUCTURED_BUFFER;
		write.bufferOffset = 0;
		write.bufferRange = WHOLE_SIZE;
		write.structuredElementCount = 1;
		write.pBuffer = mGpuLightConstants;
		CHECKED_CALL(mSceneDataSet->UpdateDescriptors(1, &write));
	}

	// Samplers
	SetupSamplers();

	// Env draw data
	{
		vkr::DescriptorSetLayoutCreateInfo createInfo = {};
		createInfo.bindings.push_back({ vkr::DescriptorBinding{0, vkr::DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, vkr::SHADER_STAGE_ALL_GRAPHICS} });
		createInfo.bindings.push_back({ vkr::DescriptorBinding{1, vkr::DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1, vkr::SHADER_STAGE_ALL_GRAPHICS} });
		createInfo.bindings.push_back({ vkr::DescriptorBinding{2, vkr::DESCRIPTOR_TYPE_SAMPLER, 1, vkr::SHADER_STAGE_ALL_GRAPHICS} });
		CHECKED_CALL(device.CreateDescriptorSetLayout(createInfo, &mEnvDrawLayout));

		CHECKED_CALL(device.AllocateDescriptorSet(mDescriptorPool, mEnvDrawLayout, &mEnvDrawSet));

		// Scene constants
		vkr::BufferCreateInfo bufferCreateInfo = {};
		bufferCreateInfo.size = vkr::MINIMUM_CONSTANT_BUFFER_SIZE;
		bufferCreateInfo.usageFlags.bits.transferSrc = true;
		bufferCreateInfo.memoryUsage = vkr::MemoryUsage::CPUToGPU;
		CHECKED_CALL(device.CreateBuffer(bufferCreateInfo, &mCpuEnvDrawConstants));

		bufferCreateInfo.usageFlags.bits.transferDst = true;
		bufferCreateInfo.usageFlags.bits.uniformBuffer = true;
		bufferCreateInfo.memoryUsage = vkr::MemoryUsage::GPUOnly;
		CHECKED_CALL(device.CreateBuffer(bufferCreateInfo, &mGpuEnvDrawConstants));

		vkr::WriteDescriptor writes[3] = {};
		// Constants
		writes[0].binding = 0;
		writes[0].arrayIndex = 0;
		writes[0].type = vkr::DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		writes[0].bufferOffset = 0;
		writes[0].bufferRange = WHOLE_SIZE;
		writes[0].pBuffer = mGpuEnvDrawConstants;
		// IBL texture
		writes[1].binding = 1;
		writes[1].arrayIndex = 0;
		writes[1].type = vkr::DESCRIPTOR_TYPE_SAMPLED_IMAGE;
		writes[1].pImageView = mIBLResources[mCurrentIBLIndex].environmentTexture->GetSampledImageView();
		// Sampler
		writes[2].binding = 2;
		writes[2].arrayIndex = 0;
		writes[2].type = vkr::DESCRIPTOR_TYPE_SAMPLER;
		writes[2].pSampler = mSampler;

		CHECKED_CALL(mEnvDrawSet->UpdateDescriptors(3, writes));
	}

	// Material data resources
	SetupMaterials();

	// MaterialData data
	{
		vkr::DescriptorSetLayoutCreateInfo createInfo = {};
		createInfo.bindings.push_back({ vkr::DescriptorBinding{MATERIAL_CONSTANTS_REGISTER, vkr::DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, vkr::SHADER_STAGE_ALL_GRAPHICS} });
		CHECKED_CALL(device.CreateDescriptorSetLayout(createInfo, &mMaterialDataLayout));

		CHECKED_CALL(device.AllocateDescriptorSet(mDescriptorPool, mMaterialDataLayout, &mMaterialDataSet));

		// MaterialData constants
		vkr::BufferCreateInfo bufferCreateInfo = {};
		bufferCreateInfo.size = vkr::MINIMUM_CONSTANT_BUFFER_SIZE;
		bufferCreateInfo.usageFlags.bits.transferSrc = true;
		bufferCreateInfo.memoryUsage = vkr::MemoryUsage::CPUToGPU;
		CHECKED_CALL(device.CreateBuffer(bufferCreateInfo, &mCpuMaterialConstants));

		bufferCreateInfo.usageFlags.bits.transferDst = true;
		bufferCreateInfo.usageFlags.bits.uniformBuffer = true;
		bufferCreateInfo.memoryUsage = vkr::MemoryUsage::GPUOnly;
		CHECKED_CALL(device.CreateBuffer(bufferCreateInfo, &mGpuMaterialConstants));

		vkr::WriteDescriptor write = {};
		write.binding = MATERIAL_CONSTANTS_REGISTER;
		write.arrayIndex = 0;
		write.type = vkr::DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		write.bufferOffset = 0;
		write.bufferRange = WHOLE_SIZE;
		write.pBuffer = mGpuMaterialConstants;
		CHECKED_CALL(mMaterialDataSet->UpdateDescriptors(1, &write));
	}

	// Model data
	{
		vkr::DescriptorSetLayoutCreateInfo createInfo = {};
		createInfo.bindings.push_back({ vkr::DescriptorBinding{MODEL_CONSTANTS_REGISTER, vkr::DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, vkr::SHADER_STAGE_ALL_GRAPHICS} });
		CHECKED_CALL(device.CreateDescriptorSetLayout(createInfo, &mModelDataLayout));

		CHECKED_CALL(device.AllocateDescriptorSet(mDescriptorPool, mModelDataLayout, &mModelDataSet));

		// Model constants
		vkr::BufferCreateInfo bufferCreateInfo = {};
		bufferCreateInfo.size = vkr::MINIMUM_CONSTANT_BUFFER_SIZE;
		bufferCreateInfo.usageFlags.bits.transferSrc = true;
		bufferCreateInfo.memoryUsage = vkr::MemoryUsage::CPUToGPU;
		CHECKED_CALL(device.CreateBuffer(bufferCreateInfo, &mCpuModelConstants));

		bufferCreateInfo.usageFlags.bits.transferDst = true;
		bufferCreateInfo.usageFlags.bits.uniformBuffer = true;
		bufferCreateInfo.memoryUsage = vkr::MemoryUsage::GPUOnly;
		CHECKED_CALL(device.CreateBuffer(bufferCreateInfo, &mGpuModelConstants));

		vkr::WriteDescriptor write = {};
		write.binding = MODEL_CONSTANTS_REGISTER;
		write.arrayIndex = 0;
		write.type = vkr::DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		write.bufferOffset = 0;
		write.bufferRange = WHOLE_SIZE;
		write.pBuffer = mGpuModelConstants;
		CHECKED_CALL(mModelDataSet->UpdateDescriptors(1, &write));
	}

	// Pipeline Interfaces
	{
		vkr::PipelineInterfaceCreateInfo createInfo = {};
		createInfo.setCount = 4;
		createInfo.sets[0].set = 0;
		createInfo.sets[0].pLayout = mSceneDataLayout;
		createInfo.sets[1].set = 1;
		createInfo.sets[1].pLayout = mMaterialResourcesLayout;
		createInfo.sets[2].set = 2;
		createInfo.sets[2].pLayout = mMaterialDataLayout;
		createInfo.sets[3].set = 3;
		createInfo.sets[3].pLayout = mModelDataLayout;

		CHECKED_CALL(device.CreatePipelineInterface(createInfo, &mPipelineInterface));

		// Env Draw
		createInfo = {};
		createInfo.setCount = 1;
		createInfo.sets[0].set = 0;
		createInfo.sets[0].pLayout = mEnvDrawLayout;

		CHECKED_CALL(device.CreatePipelineInterface(createInfo, &mEnvDrawPipelineInterface));
	}

	// Pipeline
	{
		vkr::GraphicsPipelineCreateInfo2 gpCreateInfo = {};
		gpCreateInfo.vertexInputState.bindingCount = CountU32(vertexBindings);
		gpCreateInfo.vertexInputState.bindings[0] = vertexBindings[0];
		gpCreateInfo.vertexInputState.bindings[1] = vertexBindings[1];
		gpCreateInfo.vertexInputState.bindings[2] = vertexBindings[2];
		gpCreateInfo.vertexInputState.bindings[3] = vertexBindings[3];
		gpCreateInfo.vertexInputState.bindings[4] = vertexBindings[4];
		gpCreateInfo.vertexInputState.bindings[5] = vertexBindings[5];
		gpCreateInfo.topology = vkr::PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
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

		vkr::ShaderModulePtr VS;

		std::vector<char> bytecode = GetRenderDevice().LoadShader("materials/shaders", "VertexShader.vs");
		ASSERT_MSG(!bytecode.empty(), "VS shader bytecode load failed");
		vkr::ShaderModuleCreateInfo shaderCreateInfo = { static_cast<uint32_t>(bytecode.size()), bytecode.data() };
		CHECKED_CALL(device.CreateShaderModule(shaderCreateInfo, &VS));

		// Gouraud
		{
			vkr::ShaderModulePtr PS;

			bytecode = GetRenderDevice().LoadShader("materials/shaders", "Gouraud.ps");
			ASSERT_MSG(!bytecode.empty(), "PS shader bytecode load failed");
			shaderCreateInfo = { static_cast<uint32_t>(bytecode.size()), bytecode.data() };
			CHECKED_CALL(device.CreateShaderModule(shaderCreateInfo, &PS));

			gpCreateInfo.VS = { VS.Get(), "vsmain" };
			gpCreateInfo.PS = { PS.Get(), "psmain" };

			CHECKED_CALL(device.CreateGraphicsPipeline(gpCreateInfo, &mGouraudPipeline));
			device.DestroyShaderModule(PS);

			mShaderPipelines.push_back(mGouraudPipeline);
		}

		// Phong
		{
			vkr::ShaderModulePtr PS;

			bytecode = GetRenderDevice().LoadShader("materials/shaders", "Phong.ps");
			ASSERT_MSG(!bytecode.empty(), "PS shader bytecode load failed");
			shaderCreateInfo = { static_cast<uint32_t>(bytecode.size()), bytecode.data() };
			CHECKED_CALL(device.CreateShaderModule(shaderCreateInfo, &PS));

			gpCreateInfo.VS = { VS.Get(), "vsmain" };
			gpCreateInfo.PS = { PS.Get(), "psmain" };

			CHECKED_CALL(device.CreateGraphicsPipeline(gpCreateInfo, &mPhongPipeline));
			device.DestroyShaderModule(PS);

			mShaderPipelines.push_back(mPhongPipeline);
		}

		// BlinnPhong
		{
			vkr::ShaderModulePtr PS;

			bytecode = GetRenderDevice().LoadShader("materials/shaders", "BlinnPhong.ps");
			ASSERT_MSG(!bytecode.empty(), "PS shader bytecode load failed");
			shaderCreateInfo = { static_cast<uint32_t>(bytecode.size()), bytecode.data() };
			CHECKED_CALL(device.CreateShaderModule(shaderCreateInfo, &PS));

			gpCreateInfo.VS = { VS.Get(), "vsmain" };
			gpCreateInfo.PS = { PS.Get(), "psmain" };

			CHECKED_CALL(device.CreateGraphicsPipeline(gpCreateInfo, &mBlinnPhongPipeline));
			device.DestroyShaderModule(PS);

			mShaderPipelines.push_back(mBlinnPhongPipeline);
		}

		// PBR
		{
			vkr::ShaderModulePtr PS;

			bytecode = GetRenderDevice().LoadShader("materials/shaders", "PBR.ps");
			ASSERT_MSG(!bytecode.empty(), "PS shader bytecode load failed");
			shaderCreateInfo = { static_cast<uint32_t>(bytecode.size()), bytecode.data() };
			CHECKED_CALL(device.CreateShaderModule(shaderCreateInfo, &PS));

			gpCreateInfo.VS = { VS.Get(), "vsmain" };
			gpCreateInfo.PS = { PS.Get(), "psmain" };

			CHECKED_CALL(device.CreateGraphicsPipeline(gpCreateInfo, &mPBRPipeline));
			device.DestroyShaderModule(PS);

			mShaderPipelines.push_back(mPBRPipeline);
		}

		// Env Draw
		{
			bytecode = GetRenderDevice().LoadShader("materials/shaders", "EnvDraw.vs");
			ASSERT_MSG(!bytecode.empty(), "VS shader bytecode load failed");
			vkr::ShaderModuleCreateInfo shaderCreateInfo = { static_cast<uint32_t>(bytecode.size()), bytecode.data() };
			CHECKED_CALL(device.CreateShaderModule(shaderCreateInfo, &VS));

			vkr::ShaderModulePtr PS;
			bytecode = GetRenderDevice().LoadShader("materials/shaders", "EnvDraw.ps");
			ASSERT_MSG(!bytecode.empty(), "PS shader bytecode load failed");
			shaderCreateInfo = { static_cast<uint32_t>(bytecode.size()), bytecode.data() };
			CHECKED_CALL(device.CreateShaderModule(shaderCreateInfo, &PS));

			gpCreateInfo.vertexInputState.bindingCount = CountU32(mEnvDrawMesh->GetDerivedVertexBindings());
			gpCreateInfo.vertexInputState.bindings[0] = mEnvDrawMesh->GetDerivedVertexBindings()[0];
			gpCreateInfo.vertexInputState.bindings[1] = mEnvDrawMesh->GetDerivedVertexBindings()[1];
			gpCreateInfo.cullMode = vkr::CULL_MODE_FRONT;
			gpCreateInfo.pPipelineInterface = mEnvDrawPipelineInterface;

			gpCreateInfo.VS = { VS.Get(), "vsmain" };
			gpCreateInfo.PS = { PS.Get(), "psmain" };

			CHECKED_CALL(device.CreateGraphicsPipeline(gpCreateInfo, &mEnvDrawPipeline));
			device.DestroyShaderModule(PS);
		}
	}

	// Per frame data
	{
		PerFrame frame = {};

		CHECKED_CALL(device.GetGraphicsQueue()->CreateCommandBuffer(&frame.cmd));

		vkr::SemaphoreCreateInfo semaCreateInfo = {};
		CHECKED_CALL(device.CreateSemaphore(semaCreateInfo, &frame.imageAcquiredSemaphore));

		vkr::FenceCreateInfo fenceCreateInfo = {};
		CHECKED_CALL(device.CreateFence(fenceCreateInfo, &frame.imageAcquiredFence));

		CHECKED_CALL(device.CreateSemaphore(semaCreateInfo, &frame.renderCompleteSemaphore));

		fenceCreateInfo = { true }; // Create signaled
		CHECKED_CALL(device.CreateFence(fenceCreateInfo, &frame.renderCompleteFence));

#ifdef ENABLE_GPU_QUERIES
		vkr::QueryCreateInfo queryCreateInfo = {};
		queryCreateInfo.type = vkr::QueryType::Timestamp;
		queryCreateInfo.count = 2;
		CHECKED_CALL(device.CreateQuery(queryCreateInfo, &frame.timestampQuery));

		// Pipeline statistics query pool
		if (device.PipelineStatsAvailable()) {
			queryCreateInfo = {};
			queryCreateInfo.type = vkr::QueryType::PipelineStatistics;
			queryCreateInfo.count = 1;
			CHECKED_CALL(device.CreateQuery(queryCreateInfo, &frame.pipelineStatsQuery));
		}
#endif

		mPerFrame.push_back(frame);
	}

	return true;
}

void Example_025::Shutdown()
{
	mPerFrame.clear();
	// TODO: доделать очистку
}

void Example_025::Update()
{
}

void Example_025::Render()
{
	auto& render = GetRender();
	auto& device = GetRenderDevice();
	auto& swapChain = render.GetSwapChain();
	PerFrame& frame = mPerFrame[0];

	//uint32_t  iffIndex = GetInFlightFrameIndex();

	// Wait for and reset render complete fence
	CHECKED_CALL(frame.renderCompleteFence->WaitAndReset());

	uint32_t imageIndex = UINT32_MAX;
	CHECKED_CALL(swapChain.AcquireNextImage(UINT64_MAX, frame.imageAcquiredSemaphore, frame.imageAcquiredFence, &imageIndex));

	// Wait for and reset image acquired fence
	CHECKED_CALL(frame.imageAcquiredFence->WaitAndReset());

	// ---------------------------------------------------------------------------------------------

	// Smooth out the rotation on Y
	mModelRotY += (mTargetModelRotY - mModelRotY) * 0.1f;
	mCameraRotY += (mTargetCameraRotY - mCameraRotY) * 0.1f;

	// ---------------------------------------------------------------------------------------------

	// Update camera(s)
	float3   startEyePos = float3(0, 0, 8);
	float4x4 Reye = glm::rotate(glm::radians(-mCameraRotY), float3(0, 1, 0));
	float3   eyePos = Reye * float4(startEyePos, 0);
	mCamera.LookAt(eyePos, float3(0, 0, 0));

	// Update scene constants
	{
		HLSL_PACK_BEGIN();
		struct HlslSceneData
		{
			hlsl_uint<4>      frameNumber;
			hlsl_float<12>    time;
			hlsl_float4x4<64> viewProjectionMatrix;
			hlsl_float3<12>   eyePosition;
			hlsl_uint<4>      lightCount;
			hlsl_float<4>     ambient;
			hlsl_float<4>     envLevelCount;
			hlsl_uint<4>      useBRDFLUT;
		};
		HLSL_PACK_END();

		void* pMappedAddress = nullptr;
		CHECKED_CALL(mCpuSceneConstants->MapMemory(0, &pMappedAddress));

		HlslSceneData* pSceneData = static_cast<HlslSceneData*>(pMappedAddress);
		pSceneData->viewProjectionMatrix = mCamera.GetViewProjectionMatrix();
		pSceneData->eyePosition = mCamera.GetEyePosition();
		pSceneData->lightCount = 4;
		pSceneData->ambient = mAmbient;
		pSceneData->envLevelCount = static_cast<float>(mIBLResources[mCurrentIBLIndex].environmentTexture->GetMipLevelCount());
		pSceneData->useBRDFLUT = mUseBRDFLUT;

		mCpuSceneConstants->UnmapMemory();

		vkr::BufferToBufferCopyInfo copyInfo = { mCpuSceneConstants->GetSize() };
		device.GetGraphicsQueue()->CopyBufferToBuffer(&copyInfo, mCpuSceneConstants, mGpuSceneConstants, vkr::ResourceState::ConstantBuffer, vkr::ResourceState::ConstantBuffer);
	}

	// Lights
	{
		HLSL_PACK_BEGIN();
		struct HlslLight
		{
			hlsl_uint<4>    type;
			hlsl_float3<12> position;
			hlsl_float3<12> color;
			hlsl_float<4>   intensity;
		};
		HLSL_PACK_END();

		void* pMappedAddress = nullptr;
		CHECKED_CALL(mCpuLightConstants->MapMemory(0, &pMappedAddress));

		HlslLight* pLight = static_cast<HlslLight*>(pMappedAddress);
		pLight[0].position = float3(10, 5, 10);
		pLight[1].position = float3(-10, 0, 5);
		pLight[2].position = float3(1, 10, 3);
		pLight[3].position = float3(-1, 0, 15);

		pLight[0].color = float3(1.0f, 1.0f, 1.0f);
		pLight[1].color = float3(1.0f, 1.0f, 1.0f);
		pLight[2].color = float3(1.0f, 1.0f, 1.0f);
		pLight[3].color = float3(1.0f, 1.0f, 1.0f);

		// These values favor PBR and will look a bit overblown using Phong or Blinn
		pLight[0].intensity = 0.37f;
		pLight[1].intensity = 0.30f;
		pLight[2].intensity = 0.45f;
		pLight[3].intensity = 0.37f;

		mCpuLightConstants->UnmapMemory();

		vkr::BufferToBufferCopyInfo copyInfo = { mCpuLightConstants->GetSize() };
		device.GetGraphicsQueue()->CopyBufferToBuffer(&copyInfo, mCpuLightConstants, mGpuLightConstants, vkr::ResourceState::ShaderResource, vkr::ResourceState::ShaderResource);
	}

	// MaterialData constatns
	{
		HLSL_PACK_BEGIN();
		struct HlslMaterial
		{
			hlsl_float3<16> F0;
			hlsl_float3<12> albedo;
			hlsl_float<4>   roughness;
			hlsl_float<4>   metalness;
			hlsl_float<4>   iblStrength;
			hlsl_float<4>   envStrength;
			hlsl_uint<4>    albedoSelect;
			hlsl_uint<4>    roughnessSelect;
			hlsl_uint<4>    metalnessSelect;
			hlsl_uint<4>    normalSelect;
			hlsl_uint<4>    iblSelect;
			hlsl_uint<4>    envSelect;
		};
		HLSL_PACK_END();

		void* pMappedAddress = nullptr;
		CHECKED_CALL(mCpuMaterialConstants->MapMemory(0, &pMappedAddress));

		HlslMaterial* pMaterial = static_cast<HlslMaterial*>(pMappedAddress);
		pMaterial->F0 = mF0[mF0Index];
		pMaterial->albedo = (mF0Index <= 10) ? mF0[mF0Index] : mAlbedoColor;
		pMaterial->roughness = mMaterialData.roughness;
		pMaterial->metalness = mMaterialData.metalness;
		pMaterial->iblStrength = mMaterialData.iblStrength;
		pMaterial->envStrength = mMaterialData.envStrength;
		pMaterial->albedoSelect = mMaterialData.albedoSelect;
		pMaterial->roughnessSelect = mMaterialData.roughnessSelect;
		pMaterial->metalnessSelect = mMaterialData.metalnessSelect;
		pMaterial->normalSelect = mMaterialData.normalSelect;
		pMaterial->iblSelect = mMaterialData.iblSelect;
		pMaterial->envSelect = mMaterialData.envSelect;

		mCpuMaterialConstants->UnmapMemory();

		vkr::BufferToBufferCopyInfo copyInfo = { mCpuMaterialConstants->GetSize() };
		device.GetGraphicsQueue()->CopyBufferToBuffer(&copyInfo, mCpuMaterialConstants, mGpuMaterialConstants, vkr::ResourceState::ConstantBuffer, vkr::ResourceState::ConstantBuffer);
	}

	// Update model constants
	{
		float4x4 R = glm::rotate(glm::radians(mModelRotY + 180.0f), float3(0, 1, 0));
		float4x4 S = glm::scale(float3(3.0f));
		float4x4 M = R * S;

		HLSL_PACK_BEGIN();
		struct HlslModelData
		{
			hlsl_float4x4<64> modelMatrix;
			hlsl_float4x4<64> normalMatrix;
			hlsl_float3<12>   debugColor;
		};
		HLSL_PACK_END();

		void* pMappedAddress = nullptr;
		CHECKED_CALL(mCpuModelConstants->MapMemory(0, &pMappedAddress));

		HlslModelData* pModelData = static_cast<HlslModelData*>(pMappedAddress);
		pModelData->modelMatrix = M;
		pModelData->normalMatrix = glm::inverseTranspose(M);

		mCpuModelConstants->UnmapMemory();

		vkr::BufferToBufferCopyInfo copyInfo = { mCpuModelConstants->GetSize() };
		device.GetGraphicsQueue()->CopyBufferToBuffer(&copyInfo, mCpuModelConstants, mGpuModelConstants, vkr::ResourceState::ConstantBuffer, vkr::ResourceState::ConstantBuffer);
	}

	// Update env draw constants
	{
		float4x4 MVP = mCamera.GetViewProjectionMatrix();

		void* pMappedAddress = nullptr;
		CHECKED_CALL(mCpuEnvDrawConstants->MapMemory(0, &pMappedAddress));

		std::memcpy(pMappedAddress, &MVP, sizeof(float4x4));

		mCpuEnvDrawConstants->UnmapMemory();

		vkr::BufferToBufferCopyInfo copyInfo = { mCpuEnvDrawConstants->GetSize() };
		device.GetGraphicsQueue()->CopyBufferToBuffer(&copyInfo, mCpuEnvDrawConstants, mGpuEnvDrawConstants, vkr::ResourceState::ConstantBuffer, vkr::ResourceState::ConstantBuffer);
	}

	// Update descriptors if IBL selection changed
	if (mIBLIndex != mCurrentIBLIndex) {
		mCurrentIBLIndex = mIBLIndex;

		for (auto& materialResources : mMaterialResourcesSets) {
			// Irradiance map
			vkr::WriteDescriptor write = {};
			write.binding = IRR_MAP_TEXTURE_REGISTER;
			write.arrayIndex = 0;
			write.type = vkr::DESCRIPTOR_TYPE_SAMPLED_IMAGE;
			write.pImageView = mIBLResources[mCurrentIBLIndex].irradianceTexture->GetSampledImageView();
			CHECKED_CALL(materialResources->UpdateDescriptors(1, &write));

			// Environment map
			write = {};
			write.binding = ENV_MAP_TEXTURE_REGISTER;
			write.arrayIndex = 0;
			write.type = vkr::DESCRIPTOR_TYPE_SAMPLED_IMAGE;
			write.pImageView = mIBLResources[mCurrentIBLIndex].environmentTexture->GetSampledImageView();
			CHECKED_CALL(materialResources->UpdateDescriptors(1, &write));
		}

		// Env Draw
		vkr::WriteDescriptor write = {};
		write.binding = 1;
		write.arrayIndex = 0;
		write.type = vkr::DESCRIPTOR_TYPE_SAMPLED_IMAGE;
		write.pImageView = mIBLResources[mCurrentIBLIndex].environmentTexture->GetSampledImageView();
		CHECKED_CALL(mEnvDrawSet->UpdateDescriptors(1, &write));
	}

#ifdef ENABLE_GPU_QUERIES
	// Read query results
	//if (GetFrameCount() > 0)
	{
		uint64_t data[2] = { 0 };
		CHECKED_CALL(frame.timestampQuery->GetData(data, 2 * sizeof(uint64_t)));
		mTotalGpuFrameTime = data[1] - data[0];
		if (device.PipelineStatsAvailable()) {
			CHECKED_CALL(frame.pipelineStatsQuery->GetData(&mPipelineStatistics, sizeof(vkr::PipelineStatistics)));
		}
	}

	// Reset query
	frame.timestampQuery->Reset(0, 2);
	if (device.PipelineStatsAvailable()) {
		frame.pipelineStatsQuery->Reset(0, 1);
	}
#endif

	// Build command buffer
	CHECKED_CALL(frame.cmd->Begin());
	{
		vkr::RenderPassPtr renderPass = swapChain.GetRenderPass(imageIndex);
		ASSERT_MSG(!renderPass.IsNull(), "render pass object is null");

		// =====================================================================
		//  Render scene
		// =====================================================================
		frame.cmd->TransitionImageLayout(renderPass->GetRenderTargetImage(0), ALL_SUBRESOURCES, vkr::ResourceState::Present, vkr::ResourceState::RenderTarget);
		frame.cmd->BeginRenderPass(renderPass);
		{
#ifdef ENABLE_GPU_QUERIES
			frame.cmd->WriteTimestamp(frame.timestampQuery, vkr::PIPELINE_STAGE_TOP_OF_PIPE_BIT, 0);
#endif
			frame.cmd->SetScissors(render.GetScissor());
			frame.cmd->SetViewports(render.GetViewport());

#ifdef ENABLE_GPU_QUERIES
			if (device.PipelineStatsAvailable()) {
				frame.cmd->BeginQuery(frame.pipelineStatsQuery, 0);
			}
#endif
			// Draw model
			vkr::DescriptorSet* sets[4] = { nullptr };
			sets[0] = mSceneDataSet;
			sets[1] = mMaterialResourcesSets[mMaterialIndex];
			sets[2] = mMaterialDataSet;
			sets[3] = mModelDataSet;
			frame.cmd->BindGraphicsDescriptorSets(mPipelineInterface, 4, sets);
			frame.cmd->BindGraphicsPipeline(mShaderPipelines[mShaderIndex]);

			frame.cmd->BindIndexBuffer(mMeshes[mMeshIndex]);
			frame.cmd->BindVertexBuffers(mMeshes[mMeshIndex]);
			frame.cmd->DrawIndexed(mMeshes[mMeshIndex]->GetIndexCount());

			// Draw environnment
			sets[0] = mEnvDrawSet;
			frame.cmd->BindGraphicsDescriptorSets(mEnvDrawPipelineInterface, 1, sets);
			frame.cmd->BindGraphicsPipeline(mEnvDrawPipeline);

			frame.cmd->BindIndexBuffer(mEnvDrawMesh);
			frame.cmd->BindVertexBuffers(mEnvDrawMesh);
			frame.cmd->DrawIndexed(mEnvDrawMesh->GetIndexCount());

#ifdef ENABLE_GPU_QUERIES
			if (device.PipelineStatsAvailable()) {
				frame.cmd->EndQuery(frame.pipelineStatsQuery, 0);
			}
#endif

			// Draw ImGui
			render.DrawDebugInfo();
			{
				ImGui::Separator();

				ImGui::SliderFloat("Ambient", &mAmbient, 0.0f, 1.0f, "%.03f");

				ImGui::Separator();

				static const char* currentShaderName = "PBR";
				if (ImGui::BeginCombo("Shader Pipeline", currentShaderName)) {
					for (size_t i = 0; i < mShaderNames.size(); ++i) {
						bool isSelected = (currentShaderName == mShaderNames[i]);
						if (ImGui::Selectable(mShaderNames[i], isSelected)) {
							currentShaderName = mShaderNames[i];
							mShaderIndex = static_cast<uint32_t>(i);
						}
						if (isSelected) {
							ImGui::SetItemDefaultFocus();
						}
					}
					ImGui::EndCombo();
				}

				ImGui::Separator();

				static const char* currentModelName = mMeshNames[0];
				if (ImGui::BeginCombo("vkr::Geometry", currentModelName)) {
					for (size_t i = 0; i < mMeshNames.size(); ++i) {
						bool isSelected = (currentModelName == mMeshNames[i]);
						if (ImGui::Selectable(mMeshNames[i], isSelected)) {
							currentModelName = mMeshNames[i];
							mMeshIndex = static_cast<uint32_t>(i);
						}
						if (isSelected) {
							ImGui::SetItemDefaultFocus();
						}
					}
					ImGui::EndCombo();
				}

				ImGui::Separator();

				static const char* currentMaterialName = mMaterialNames[0];
				if (ImGui::BeginCombo("Material Textures", currentMaterialName)) {
					for (size_t i = 0; i < mMaterialNames.size(); ++i) {
						bool isSelected = (currentMaterialName == mMaterialNames[i]);
						if (ImGui::Selectable(mMaterialNames[i], isSelected)) {
							currentMaterialName = mMaterialNames[i];
							mMaterialIndex = static_cast<uint32_t>(i);
						}
						if (isSelected) {
							ImGui::SetItemDefaultFocus();
						}
					}
					ImGui::EndCombo();
				}

				ImGui::Separator();

				static const char* currentIBLName = mIBLNames[0];
				if (ImGui::BeginCombo("IBL Selection", currentIBLName)) {
					for (size_t i = 0; i < mIBLNames.size(); ++i) {
						bool isSelected = (currentIBLName == mIBLNames[i]);
						if (ImGui::Selectable(mIBLNames[i], isSelected)) {
							currentIBLName = mIBLNames[i];
							mIBLIndex = static_cast<uint32_t>(i);
						}
						if (isSelected) {
							ImGui::SetItemDefaultFocus();
						}
					}
					ImGui::EndCombo();
				}

				ImGui::Separator();

				ImGui::Checkbox("PBR Use Albedo Texture", &mMaterialData.albedoSelect);
				ImGui::Checkbox("PBR Use Roughness Texture", &mMaterialData.roughnessSelect);
				ImGui::Checkbox("PBR Use Metalness Texture", &mMaterialData.metalnessSelect);
				ImGui::Checkbox("PBR Use Normal Map", &mMaterialData.normalSelect);
				ImGui::Checkbox("PBR Use Reflection Map", &mMaterialData.envSelect);
				ImGui::Checkbox("PBR Use BRDF LUT", &mUseBRDFLUT);

				ImGui::Separator();

				ImGui::Columns(2);

				uint64_t frequency = 0;
				device.GetGraphicsQueue()->GetTimestampFrequency(&frequency);
				ImGui::Text("Previous GPU Frame Time");
				ImGui::NextColumn();
				ImGui::Text("%f ms ", static_cast<float>(mTotalGpuFrameTime / static_cast<double>(frequency)) * 1000.0f);
				ImGui::NextColumn();

				ImGui::Separator();

				ImGui::Text("IAVertices");
				ImGui::NextColumn();
				ImGui::Text("%" PRIu64, mPipelineStatistics.IAVertices);
				ImGui::NextColumn();

				ImGui::Text("IAPrimitives");
				ImGui::NextColumn();
				ImGui::Text("%" PRIu64, mPipelineStatistics.IAPrimitives);
				ImGui::NextColumn();

				ImGui::Text("VSInvocations");
				ImGui::NextColumn();
				ImGui::Text("%" PRIu64, mPipelineStatistics.VSInvocations);
				ImGui::NextColumn();

				ImGui::Text("CInvocations");
				ImGui::NextColumn();
				ImGui::Text("%" PRIu64, mPipelineStatistics.CInvocations);
				ImGui::NextColumn();

				ImGui::Text("CPrimitives");
				ImGui::NextColumn();
				ImGui::Text("%" PRIu64, mPipelineStatistics.CPrimitives);
				ImGui::NextColumn();

				ImGui::Text("PSInvocations");
				ImGui::NextColumn();
				ImGui::Text("%" PRIu64, mPipelineStatistics.PSInvocations);
				ImGui::NextColumn();

				ImGui::Columns(1);
			}
			render.DrawImGui(frame.cmd);
		}
		frame.cmd->EndRenderPass();
#ifdef ENABLE_GPU_QUERIES
		frame.cmd->WriteTimestamp(frame.timestampQuery, vkr::PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 1);

		// Resolve queries
		frame.cmd->ResolveQueryData(frame.timestampQuery, 0, 2);
		if (device.PipelineStatsAvailable()) {
			frame.cmd->ResolveQueryData(frame.pipelineStatsQuery, 0, 1);
		}
#endif
		frame.cmd->TransitionImageLayout(renderPass->GetRenderTargetImage(0), ALL_SUBRESOURCES, vkr::ResourceState::RenderTarget, vkr::ResourceState::Present);
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

	CHECKED_CALL(device.GetGraphicsQueue()->Submit(&submitInfo));

	CHECKED_CALL(swapChain.Present(imageIndex, 1, &frame.renderCompleteSemaphore));
}

void Example_025::MouseMove(int32_t x, int32_t y, int32_t dx, int32_t dy, MouseButton buttons)
{
	if (buttons == MouseButton::Left) {
		if (GetKeyState(KEY_LEFT_CONTROL).down || GetKeyState(KEY_RIGHT_CONTROL).down) {
			mTargetCameraRotY += 0.25f * dx;
		}
		else {
			mTargetModelRotY += 0.25f * dx;
		}
	}
}

void Example_025::SetupSamplers()
{
	// Sampler
	vkr::SamplerCreateInfo samplerCreateInfo = {};
	samplerCreateInfo.magFilter = vkr::Filter::Linear;
	samplerCreateInfo.minFilter = vkr::Filter::Linear;
	samplerCreateInfo.mipmapMode = vkr::SamplerMipmapMode::Linear;
	samplerCreateInfo.minLod = 0.0f;
	samplerCreateInfo.maxLod = FLT_MAX;
	CHECKED_CALL(GetRenderDevice().CreateSampler(samplerCreateInfo, &mSampler));
}

void Example_025::SetupMaterialResources(
	const std::filesystem::path& albedoPath,
	const std::filesystem::path& roughnessPath,
	const std::filesystem::path& metalnessPath,
	const std::filesystem::path& normalMapPath,
	MaterialResources& materialResources)
{
	CHECKED_CALL(GetRenderDevice().AllocateDescriptorSet(mDescriptorPool, mMaterialResourcesLayout, &materialResources.set));

	// Albedo
	{
		CHECKED_CALL(vkr::vkrUtil::CreateTextureFromFile(GetRenderDevice().GetGraphicsQueue(), albedoPath, &materialResources.albedoTexture));

		vkr::WriteDescriptor write = {};
		write.binding = ALBEDO_TEXTURE_REGISTER;
		write.arrayIndex = 0;
		write.type = vkr::DESCRIPTOR_TYPE_SAMPLED_IMAGE;
		write.pImageView = materialResources.albedoTexture->GetSampledImageView();
		CHECKED_CALL(materialResources.set->UpdateDescriptors(1, &write));
	}

	// Roughness
	{
		CHECKED_CALL(vkr::vkrUtil::CreateTextureFromFile(GetRenderDevice().GetGraphicsQueue(), roughnessPath, &materialResources.roughnessTexture));

		vkr::WriteDescriptor write = {};
		write.binding = ROUGHNESS_TEXTURE_REGISTER;
		write.arrayIndex = 0;
		write.type = vkr::DESCRIPTOR_TYPE_SAMPLED_IMAGE;
		write.pImageView = materialResources.roughnessTexture->GetSampledImageView();
		CHECKED_CALL(materialResources.set->UpdateDescriptors(1, &write));
	}

	// Metalness
	{
		CHECKED_CALL(vkr::vkrUtil::CreateTextureFromFile(GetRenderDevice().GetGraphicsQueue(), metalnessPath, &materialResources.metalnessTexture));

		vkr::WriteDescriptor write = {};
		write.binding = METALNESS_TEXTURE_REGISTER;
		write.arrayIndex = 0;
		write.type = vkr::DESCRIPTOR_TYPE_SAMPLED_IMAGE;
		write.pImageView = materialResources.metalnessTexture->GetSampledImageView();
		CHECKED_CALL(materialResources.set->UpdateDescriptors(1, &write));
	}

	// Normal map
	{
		CHECKED_CALL(vkr::vkrUtil::CreateTextureFromFile(GetRenderDevice().GetGraphicsQueue(), normalMapPath, &materialResources.normalMapTexture));

		vkr::WriteDescriptor write = {};
		write.binding = NORMAL_MAP_TEXTURE_REGISTER;
		write.arrayIndex = 0;
		write.type = vkr::DESCRIPTOR_TYPE_SAMPLED_IMAGE;
		write.pImageView = materialResources.normalMapTexture->GetSampledImageView();
		CHECKED_CALL(materialResources.set->UpdateDescriptors(1, &write));
	}

	// Irradiance map
	{
		vkr::WriteDescriptor write = {};
		write.binding = IRR_MAP_TEXTURE_REGISTER;
		write.arrayIndex = 0;
		write.type = vkr::DESCRIPTOR_TYPE_SAMPLED_IMAGE;
		write.pImageView = mIBLResources[mCurrentIBLIndex].irradianceTexture->GetSampledImageView();
		CHECKED_CALL(materialResources.set->UpdateDescriptors(1, &write));
	}

	// Environment map
	{
		vkr::WriteDescriptor write = {};
		write.binding = ENV_MAP_TEXTURE_REGISTER;
		write.arrayIndex = 0;
		write.type = vkr::DESCRIPTOR_TYPE_SAMPLED_IMAGE;
		write.pImageView = mIBLResources[mCurrentIBLIndex].environmentTexture->GetSampledImageView();
		CHECKED_CALL(materialResources.set->UpdateDescriptors(1, &write));
	}

	// BRDF LUT
	{
		vkr::WriteDescriptor write = {};
		write.binding = BRDF_LUT_TEXTURE_REGISTER;
		write.arrayIndex = 0;
		write.type = vkr::DESCRIPTOR_TYPE_SAMPLED_IMAGE;
		write.pImageView = mBRDFLUTTexture->GetSampledImageView();
		CHECKED_CALL(materialResources.set->UpdateDescriptors(1, &write));
	}

	// Sampler
	{
		vkr::WriteDescriptor write = {};
		write.binding = CLAMPED_SAMPLER_REGISTER;
		write.arrayIndex = 0;
		write.type = vkr::DESCRIPTOR_TYPE_SAMPLER;
		write.pSampler = mSampler;
		CHECKED_CALL(materialResources.set->UpdateDescriptors(1, &write));
	}
}

void Example_025::SetupMaterials()
{
	// Layout
	vkr::DescriptorSetLayoutCreateInfo createInfo = {};
	createInfo.bindings.push_back({ vkr::DescriptorBinding{ALBEDO_TEXTURE_REGISTER, vkr::DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1, vkr::SHADER_STAGE_ALL_GRAPHICS} });
	createInfo.bindings.push_back({ vkr::DescriptorBinding{ROUGHNESS_TEXTURE_REGISTER, vkr::DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1, vkr::SHADER_STAGE_ALL_GRAPHICS} });
	createInfo.bindings.push_back({ vkr::DescriptorBinding{METALNESS_TEXTURE_REGISTER, vkr::DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1, vkr::SHADER_STAGE_ALL_GRAPHICS} });
	createInfo.bindings.push_back({ vkr::DescriptorBinding{NORMAL_MAP_TEXTURE_REGISTER, vkr::DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1, vkr::SHADER_STAGE_ALL_GRAPHICS} });
	createInfo.bindings.push_back({ vkr::DescriptorBinding{IRR_MAP_TEXTURE_REGISTER, vkr::DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1, vkr::SHADER_STAGE_ALL_GRAPHICS} });
	createInfo.bindings.push_back({ vkr::DescriptorBinding{ENV_MAP_TEXTURE_REGISTER, vkr::DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1, vkr::SHADER_STAGE_ALL_GRAPHICS} });
	createInfo.bindings.push_back({ vkr::DescriptorBinding{BRDF_LUT_TEXTURE_REGISTER, vkr::DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1, vkr::SHADER_STAGE_ALL_GRAPHICS} });
	createInfo.bindings.push_back({ vkr::DescriptorBinding{CLAMPED_SAMPLER_REGISTER, vkr::DESCRIPTOR_TYPE_SAMPLER, 1, vkr::SHADER_STAGE_ALL_GRAPHICS} });
	CHECKED_CALL(GetRenderDevice().CreateDescriptorSetLayout(createInfo, &mMaterialResourcesLayout));

	// Green metal rust
	{
		SetupMaterialResources(
			"poly_haven/textures/green_metal_rust/diffuse.png",
			"poly_haven/textures/green_metal_rust/roughness.png",
			"poly_haven/textures/green_metal_rust/metalness.png",
			"poly_haven/textures/green_metal_rust/normal.png",
			mMetalMaterial);
		mMaterialResourcesSets.push_back(mMetalMaterial.set);
	};

	// Wood
	{
		SetupMaterialResources(
			"poly_haven/textures/weathered_planks/diffuse.png",
			"poly_haven/textures/weathered_planks/roughness.png",
			"poly_haven/textures/weathered_planks/metalness.png",
			"poly_haven/textures/weathered_planks/normal.png",
			mWoodMaterial);
		mMaterialResourcesSets.push_back(mWoodMaterial.set);
	};

	// Tiles
	{
		SetupMaterialResources(
			"poly_haven/textures/square_floor_tiles/diffuse.png",
			"poly_haven/textures/square_floor_tiles/roughness.png",
			"poly_haven/textures/square_floor_tiles/metalness.png",
			"poly_haven/textures/square_floor_tiles/normal.png",
			mTilesMaterial);
		mMaterialResourcesSets.push_back(mTilesMaterial.set);
	};

	// Stone Wall
	{
		SetupMaterialResources(
			"poly_haven/textures/yellow_stone_wall/diffuse.png",
			"poly_haven/textures/yellow_stone_wall/roughness.png",
			"poly_haven/textures/yellow_stone_wall/metalness.png",
			"poly_haven/textures/yellow_stone_wall/normal.png",
			mStoneWallMaterial);
		mMaterialResourcesSets.push_back(mStoneWallMaterial.set);
	}

	// Measuring Tape
	{
		SetupMaterialResources(
			"poly_haven/models/measuring_tape/textures/diffuse.png",
			"poly_haven/models/measuring_tape/textures/roughness.png",
			"poly_haven/models/measuring_tape/textures/metalness.png",
			"poly_haven/models/measuring_tape/textures/normal.png",
			mMeasuringTapeMaterial);
		mMaterialResourcesSets.push_back(mMeasuringTapeMaterial.set);
	}

	// Kiwi
	{
		SetupMaterialResources(
			"poly_haven/models/food_kiwi/textures/diffuse.png",
			"poly_haven/models/food_kiwi/textures/roughness.png",
			"poly_haven/models/food_kiwi/textures/metalness.png",
			"poly_haven/models/food_kiwi/textures/normal.png",
			mKiwiMaterial);
		mMaterialResourcesSets.push_back(mKiwiMaterial.set);
	}

	// Hand Plane
	{
		SetupMaterialResources(
			"poly_haven/models/hand_plane/textures/diffuse.png",
			"poly_haven/models/hand_plane/textures/roughness.png",
			"poly_haven/models/hand_plane/textures/metalness.png",
			"poly_haven/models/hand_plane/textures/normal.png",
			mHandPlaneMaterial);
		mMaterialResourcesSets.push_back(mHandPlaneMaterial.set);
	}

	// Horse Statue
	{
		SetupMaterialResources(
			"poly_haven/models/horse_statue/textures/diffuse.png",
			"poly_haven/models/horse_statue/textures/roughness.png",
			"poly_haven/models/horse_statue/textures/metalness.png",
			"poly_haven/models/horse_statue/textures/normal.png",
			mHorseStatueMaterial);
		mMaterialResourcesSets.push_back(mHorseStatueMaterial.set);
	}
}

void Example_025::SetupIBL()
{
	// BRDF LUT
	CHECKED_CALL(vkr::vkrUtil::CreateTextureFromFile(
		GetRenderDevice().GetGraphicsQueue(),
		"basic/textures/ppx/brdf_lut.hdr",
		&mBRDFLUTTexture));

	// Old Depot - good mix of diffused over head and bright exterior lighting from windows
	{
		IBLResources reses = {};
		CHECKED_CALL(vkr::vkrUtil::CreateIBLTexturesFromFile(
			GetRenderDevice().GetGraphicsQueue(),
			"poly_haven/ibl/old_depot_4k.ibl",
			&reses.irradianceTexture,
			&reses.environmentTexture));
		mIBLResources.push_back(reses);
	}

	// Palermo Square - almost fully difuse exterior lighting
	{
		IBLResources reses = {};
		CHECKED_CALL(vkr::vkrUtil::CreateIBLTexturesFromFile(
			GetRenderDevice().GetGraphicsQueue(),
			"poly_haven/ibl/palermo_square_4k.ibl",
			&reses.irradianceTexture,
			&reses.environmentTexture));
		mIBLResources.push_back(reses);
	}

	// Venice Sunset - Golden Hour at beach
	{
		IBLResources reses = {};
		CHECKED_CALL(vkr::vkrUtil::CreateIBLTexturesFromFile(
			GetRenderDevice().GetGraphicsQueue(),
			"poly_haven/ibl/venice_sunset_4k.ibl",
			&reses.irradianceTexture,
			&reses.environmentTexture));
		mIBLResources.push_back(reses);
	}

	// Hilly Terrain - Clear blue sky on hills
	{
		IBLResources reses = {};
		CHECKED_CALL(vkr::vkrUtil::CreateIBLTexturesFromFile(
			GetRenderDevice().GetGraphicsQueue(),
			"poly_haven/ibl/hilly_terrain_01_4k.ibl",
			&reses.irradianceTexture,
			&reses.environmentTexture));
		mIBLResources.push_back(reses);
	}

	// Neon Photo Studio - interior artificial lighting
	{
		IBLResources reses = {};
		CHECKED_CALL(vkr::vkrUtil::CreateIBLTexturesFromFile(
			GetRenderDevice().GetGraphicsQueue(),
			"poly_haven/ibl/neon_photostudio_4k.ibl",
			&reses.irradianceTexture,
			&reses.environmentTexture));
		mIBLResources.push_back(reses);
	}

	// Sky Lit Garage - diffused overhead exterior lighting
	{
		IBLResources reses = {};
		CHECKED_CALL(vkr::vkrUtil::CreateIBLTexturesFromFile(
			GetRenderDevice().GetGraphicsQueue(),
			"poly_haven/ibl/skylit_garage_4k.ibl",
			&reses.irradianceTexture,
			&reses.environmentTexture));
		mIBLResources.push_back(reses);
	}

	// Noon Grass - harsh overhead exterior lighting
	{
		IBLResources reses = {};
		CHECKED_CALL(vkr::vkrUtil::CreateIBLTexturesFromFile(
			GetRenderDevice().GetGraphicsQueue(),
			"poly_haven/ibl/noon_grass_4k.ibl",
			&reses.irradianceTexture,
			&reses.environmentTexture));
		mIBLResources.push_back(reses);
	}
}