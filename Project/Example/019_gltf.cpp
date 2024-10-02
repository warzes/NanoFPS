#include "stdafx.h"
#include "019_gltf.h"

Bitmap ColorToBitmap(const float3& color)
{
	Bitmap bitmap;
	CHECKED_CALL(Bitmap::Create(1, 1, Bitmap::Format::FORMAT_RGBA_FLOAT, &bitmap));
	float* ptr = bitmap.GetPixel32f(0, 0);
	ptr[0] = color.r;
	ptr[1] = color.g;
	ptr[2] = color.b;
	ptr[3] = 1.f;
	return bitmap;
}

inline size_t CountPrimitives(const cgltf_mesh* array, size_t count)
{
	size_t total = 0;
	for (size_t i = 0; i < count; i++) {
		total += array[i].primitives_count;
	}
	return total;
}

void GetAccessorsForPrimitive(const cgltf_primitive& ppPrimitive, const cgltf_accessor** ppPosition, const cgltf_accessor** ppUv, const cgltf_accessor** ppNormal, const cgltf_accessor** ppTangent)
{
	ASSERT_NULL_ARG(ppPosition);
	ASSERT_NULL_ARG(ppUv);
	ASSERT_NULL_ARG(ppNormal);
	ASSERT_NULL_ARG(ppTangent);

	*ppPosition = nullptr;
	*ppUv = nullptr;
	*ppNormal = nullptr;
	*ppTangent = nullptr;

	for (size_t i = 0; i < ppPrimitive.attributes_count; i++) {
		const auto& type = ppPrimitive.attributes[i].type;
		const auto* data = ppPrimitive.attributes[i].data;
		if (type == cgltf_attribute_type_position) {
			*ppPosition = data;
		}
		else if (type == cgltf_attribute_type_normal) {
			*ppNormal = data;
		}
		else if (type == cgltf_attribute_type_tangent) {
			*ppTangent = data;
		}
		else if (type == cgltf_attribute_type_texcoord && *ppUv == nullptr) {
			// For UV we only load the first TEXCOORDs (FIXME: support multiple tex coordinates).
			*ppUv = data;
		}
	}

	ASSERT_MSG(*ppPosition != nullptr && *ppUv != nullptr && *ppNormal != nullptr && *ppTangent != nullptr, "For now, only supports model with position, normal, tangent and UV attributes");
}

float4x4 ComputeObjectMatrix(const cgltf_node* node)
{
	float4x4 output(1.f);
	while (node != nullptr) {
		if (node->has_matrix) {
			output = glm::make_mat4(node->matrix) * output;
		}
		else {
			const float4x4 T = node->has_translation ? glm::translate(glm::make_vec3(node->translation)) : glm::mat4(1.f);
			const float4x4 R = node->has_rotation
				? glm::mat4_cast(glm::quat(node->rotation[3], node->rotation[0], node->rotation[1], node->rotation[2]))
				: glm::mat4(1.f);
			const float4x4 S = node->has_scale ? glm::scale(glm::make_vec3(node->scale)) : glm::mat4(1.f);
			const float4x4 M = T * R * S;
			output = M * output;
		}
		node = node->parent;
	}
	return output;
}

EngineApplicationCreateInfo Example_019::Config() const
{
	EngineApplicationCreateInfo createInfo{};
	createInfo.render.swapChain.depthFormat = vkr::Format::D32_FLOAT;
	createInfo.render.showImgui = true;
	return createInfo;
}

bool Example_019::Setup()
{
	auto& device = GetRenderDevice();

	// Cameras
	{
		mCamera = PerspectiveCamera(60.0f, GetWindowAspect());
	}

	// Create descriptor pool large enough for this project
	{
		vkr::DescriptorPoolCreateInfo poolCreateInfo = {};
		poolCreateInfo.uniformBuffer = 1024;
		poolCreateInfo.sampledImage = 1024;
		poolCreateInfo.sampler = 1024;
		CHECKED_CALL(device.CreateDescriptorPool(poolCreateInfo, &mDescriptorPool));
	}

	CHECKED_CALL(device.CreateShader("basic/shaders", "PbrMetallicRoughness.vs", &mVertexShader));
	CHECKED_CALL(device.CreateShader("basic/shaders", "PbrMetallicRoughness.ps", &mPbrPixelShader));
	CHECKED_CALL(device.CreateShader("basic/shaders", "Unlit.ps", &mUnlitPixelShader));

	{
		vkr::DescriptorSetLayoutCreateInfo layoutCreateInfo = {};
		layoutCreateInfo.bindings.push_back(vkr::DescriptorBinding{
			/* binding= */ 0,
			/* type= */ vkr::DescriptorType::UniformBuffer,
			/* array_count= */ 1,
			/* shader_visibility= */ vkr::SHADER_STAGE_ALL_GRAPHICS });

		// Albedo
		layoutCreateInfo.bindings.push_back(vkr::DescriptorBinding{
			/* binding= */ 1,
			/* type= */ vkr::DescriptorType::SampledImage,
			/* array_count= */ 1,
			/* shader_visibility= */ vkr::SHADER_STAGE_PS });
		layoutCreateInfo.bindings.push_back(vkr::DescriptorBinding{
			/* binding= */ 2,
			/* type= */ vkr::DescriptorType::Sampler,
			/* array_count= */ 1,
			/* shader_visibility= */ vkr::SHADER_STAGE_PS });

		// Normal
		layoutCreateInfo.bindings.push_back(vkr::DescriptorBinding{
			/* binding= */ 3,
			/* type= */ vkr::DescriptorType::SampledImage,
			/* array_count= */ 1,
			/* shader_visibility= */ vkr::SHADER_STAGE_PS });
		layoutCreateInfo.bindings.push_back(vkr::DescriptorBinding{
			/* binding= */ 4,
			/* type= */ vkr::DescriptorType::Sampler,
			/* array_count= */ 1,
			/* shader_visibility= */ vkr::SHADER_STAGE_PS });

		// Metallic/Roughness
		layoutCreateInfo.bindings.push_back(vkr::DescriptorBinding{
			/* binding= */ 5,
			/* type= */ vkr::DescriptorType::SampledImage,
			/* array_count= */ 1,
			/* shader_visibility= */ vkr::SHADER_STAGE_PS });
		layoutCreateInfo.bindings.push_back(vkr::DescriptorBinding{
			/* binding= */ 6,
			/* type= */ vkr::DescriptorType::Sampler,
			/* array_count= */ 1,
			/* shader_visibility= */ vkr::SHADER_STAGE_PS });

		CHECKED_CALL(device.CreateDescriptorSetLayout(layoutCreateInfo, &mSetLayout));
	}

	loadScene(
		"basic/models/altimeter/altimeter.gltf",
		device.GetGraphicsQueue(),
		mDescriptorPool,
		&mTextureCache,
		&mObjects,
		&mPrimitives,
		&mMaterials);

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

		mPerFrame.push_back(frame);
	}

	device.DestroyShaderModule(mVertexShader);
	device.DestroyShaderModule(mPbrPixelShader);
	device.DestroyShaderModule(mUnlitPixelShader);

	return true;
}

void Example_019::Shutdown()
{
	mPerFrame.clear();
	// TODO: доделать очистку
}

void Example_019::Update()
{
}

void Example_019::Render()
{
	auto& render = GetRender();
	auto& device = GetRenderDevice();
	auto& swapChain = render.GetSwapChain();
	PerFrame& frame = mPerFrame[0];

	// Wait for and reset render complete fence
	CHECKED_CALL(frame.renderCompleteFence->WaitAndReset());
	uint32_t imageIndex = UINT32_MAX;
	CHECKED_CALL(GetRender().GetSwapChain().AcquireNextImage(UINT64_MAX, frame.imageAcquiredSemaphore, frame.imageAcquiredFence, &imageIndex));
	// Wait for and reset image acquired fence
	CHECKED_CALL(frame.imageAcquiredFence->WaitAndReset());

	// Update camera(s)
	mCamera.LookAt(float3(2, 2, 2), float3(0, 0, 0));

	// Update uniform buffers
	for (auto& object : mObjects) {
		struct Scene
		{
			float4x4 modelMatrix;                // Transforms object space to world space
			float4x4 ITModelMatrix;              // Inverse-transpose of the model matrix.
			float4   ambient;                    // Object's ambient intensity
			float4x4 cameraViewProjectionMatrix; // Camera's view projection matrix
			float4   lightPosition;              // Light's position
			float4   eyePosition;
		};

		Scene scene = {};
		scene.modelMatrix = object.modelMatrix;
		scene.ITModelMatrix = object.ITModelMatrix;
		scene.ambient = float4(0.3f);
		scene.cameraViewProjectionMatrix = mCamera.GetViewProjectionMatrix();
		scene.lightPosition = float4(mLightPosition, 0);
		scene.eyePosition = float4(mCamera.GetEyePosition(), 0.f);

		object.pUniformBuffer->CopyFromSource(sizeof(scene), &scene);
	}

	{
		// FIXME: this assumes we have only PBR, and with 3 textures per materials. Needs to be revisited.
		constexpr size_t                                    TEXTURE_COUNT = 3;
		constexpr size_t                                    DESCRIPTOR_COUNT = 1 + TEXTURE_COUNT * 2 /* uniform + 3 * (sampler + texture) */;
		std::array<vkr::WriteDescriptor, DESCRIPTOR_COUNT> write;
		for (auto& object : mObjects) {
			for (auto& renderable : object.renderables) {
				auto* pPrimitive = renderable.pPrimitive;
				auto* pMaterial = renderable.pMaterial;
				auto* pDescriptorSet = renderable.pDescriptorSet;

				write[0].binding = 0;
				write[0].type = vkr::DescriptorType::UniformBuffer;
				write[0].bufferOffset = 0;
				write[0].bufferRange = WHOLE_SIZE;
				write[0].buffer = object.pUniformBuffer;

				for (size_t i = 0; i < TEXTURE_COUNT; i++) {
					write[1 + i * 2 + 0].binding = static_cast<uint32_t>(1 + i * 2 + 0);
					write[1 + i * 2 + 0].type = vkr::DescriptorType::SampledImage;
					write[1 + i * 2 + 0].imageView = pMaterial->textures[i].pTexture;
					write[1 + i * 2 + 1].binding = static_cast<uint32_t>(1 + i * 2 + 1);
					write[1 + i * 2 + 1].type = vkr::DescriptorType::Sampler;
					write[1 + i * 2 + 1].sampler = pMaterial->textures[i].pSampler;
				}
				CHECKED_CALL(pDescriptorSet->UpdateDescriptors(static_cast<uint32_t>(write.size()), write.data()));
			}
		}
	}

	// Build command buffer
	CHECKED_CALL(frame.cmd->Begin());
	{
		vkr::RenderPassPtr renderPass = GetRender().GetSwapChain().GetRenderPass(imageIndex);
		ASSERT_MSG(!renderPass.IsNull(), "render pass object is null");

		// =====================================================================
		//  Render scene
		// =====================================================================
		frame.cmd->TransitionImageLayout(renderPass->GetRenderTargetImage(0), ALL_SUBRESOURCES, vkr::ResourceState::Present, vkr::ResourceState::RenderTarget);
		frame.cmd->BeginRenderPass(renderPass);
		{
			frame.cmd->SetScissors(render.GetScissor());
			frame.cmd->SetViewports(render.GetViewport());

			// Draw entities
			for (auto& object : mObjects) {
				for (auto& renderable : object.renderables) {
					frame.cmd->BindGraphicsPipeline(renderable.pMaterial->pPipeline);
					frame.cmd->BindGraphicsDescriptorSets(renderable.pMaterial->pInterface, 1, &renderable.pDescriptorSet);

					frame.cmd->BindIndexBuffer(renderable.pPrimitive->mesh);
					frame.cmd->BindVertexBuffers(renderable.pPrimitive->mesh);
					frame.cmd->DrawIndexed(renderable.pPrimitive->mesh->GetIndexCount());
				}
			}

			render.DrawImGui(frame.cmd);
		}
		frame.cmd->EndRenderPass();
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

	CHECKED_CALL(GetRender().GetSwapChain().Present(imageIndex, 1, &frame.renderCompleteSemaphore));
}

void Example_019::loadTexture(
	const std::filesystem::path& gltfFolder,
	const cgltf_texture_view& textureView,
	vkr::Queue* pQueue,
	TextureCache* pTextureCache,
	Texture* pOutput)
{
	auto& device = GetRenderDevice();

	const auto& texture = *textureView.texture;
	ASSERT_MSG(textureView.texture != nullptr, "Texture with no image are not supported.");
	ASSERT_MSG(textureView.has_transform == false, "Texture transforms are not supported yet.");
	ASSERT_MSG(texture.image != nullptr, "image pointer is null.");
	ASSERT_MSG(texture.image->uri != nullptr, "image uri is null.");

	auto it = pTextureCache->find(texture.image->uri);
	if (it == pTextureCache->end()) {
		vkr::vkrUtil::ImageOptions options = vkr::vkrUtil::ImageOptions().MipLevelCount(RemainingMipLevels);
		CHECKED_CALL(vkr::vkrUtil::CreateImageFromFile(pQueue, gltfFolder / texture.image->uri, &pOutput->pImage, options, false));
		pTextureCache->emplace(texture.image->uri, pOutput->pImage);
	}
	else {
		pOutput->pImage = it->second;
	}

	vkr::SampledImageViewCreateInfo sivCreateInfo = vkr::SampledImageViewCreateInfo::GuessFromImage(pOutput->pImage);
	CHECKED_CALL(device.CreateSampledImageView(sivCreateInfo, &pOutput->pTexture));

	// FIXME: read sampler info from GLTF.
	vkr::SamplerCreateInfo samplerCreateInfo = {};
	samplerCreateInfo.magFilter = vkr::Filter::Linear;
	samplerCreateInfo.minFilter = vkr::Filter::Linear;
	samplerCreateInfo.anisotropyEnable = true;
	samplerCreateInfo.maxAnisotropy = 16;
	samplerCreateInfo.mipmapMode = vkr::SamplerMipmapMode::Linear;
	samplerCreateInfo.minLod = 0.f;
	samplerCreateInfo.maxLod = FLT_MAX;
	CHECKED_CALL(device.CreateSampler(samplerCreateInfo, &pOutput->pSampler));
}

void Example_019::loadTexture(const Bitmap& bitmap, vkr::Queue* pQueue, Texture* pOutput)
{
	auto& device = GetRenderDevice();

	vkr::vkrUtil::ImageOptions options = vkr::vkrUtil::ImageOptions().MipLevelCount(1);
	CHECKED_CALL(vkr::vkrUtil::CreateImageFromBitmap(pQueue, &bitmap, &pOutput->pImage, options));

	vkr::SampledImageViewCreateInfo sivCreateInfo = vkr::SampledImageViewCreateInfo::GuessFromImage(pOutput->pImage);
	CHECKED_CALL(device.CreateSampledImageView(sivCreateInfo, &pOutput->pTexture));
	vkr::SamplerCreateInfo samplerCreateInfo = {};
	samplerCreateInfo.magFilter = vkr::Filter::Linear;
	samplerCreateInfo.minFilter = vkr::Filter::Linear;
	samplerCreateInfo.anisotropyEnable = true;
	samplerCreateInfo.maxAnisotropy = 1.f;
	samplerCreateInfo.mipmapMode = vkr::SamplerMipmapMode::Linear;
	CHECKED_CALL(device.CreateSampler(samplerCreateInfo, &pOutput->pSampler));
}

void Example_019::loadMaterial(
	const std::filesystem::path& gltfFolder,
	const cgltf_material& material,
	vkr::Queue* pQueue,
	vkr::DescriptorPool* pDescriptorPool,
	Example_019::TextureCache* pTextureCache,
	Example_019::Material* pOutput)
{
	auto& device = GetRenderDevice();
	auto& render = GetRender();
	if (material.extensions_count != 0) {
		printf("Material %s has extensions, but they are ignored. Rendered result may vary.\n", material.name);
	}
	// This is to simplify the pipeline creation for now. Need to revisit later.
	ASSERT_MSG(material.has_pbr_metallic_roughness, "Only PBR metallic roughness supported for now.");

	vkr::PipelineInterfaceCreateInfo piCreateInfo = {};
	piCreateInfo.setCount = 1;
	piCreateInfo.sets[0].set = 0;
	piCreateInfo.sets[0].layout = mSetLayout;
	CHECKED_CALL(device.CreatePipelineInterface(piCreateInfo, &pOutput->pInterface));

	vkr::GraphicsPipelineCreateInfo2 gpCreateInfo = {};
	gpCreateInfo.VS = { mVertexShader.Get(), "vsmain" };
	vkr::ShaderModule* psModule = material.unlit ? mUnlitPixelShader.Get() : mPbrPixelShader.Get();
	gpCreateInfo.PS = { psModule, "psmain" };
	// FIXME: assuming all primitives provides POSITION, UV, NORMAL and TANGENT. Might not be the case.
	gpCreateInfo.vertexInputState.bindingCount = 4;
	gpCreateInfo.vertexInputState.bindings[0] = mPrimitives[0].mesh->GetDerivedVertexBindings()[0];
	gpCreateInfo.vertexInputState.bindings[1] = mPrimitives[0].mesh->GetDerivedVertexBindings()[1];
	gpCreateInfo.vertexInputState.bindings[2] = mPrimitives[0].mesh->GetDerivedVertexBindings()[2];
	gpCreateInfo.vertexInputState.bindings[3] = mPrimitives[0].mesh->GetDerivedVertexBindings()[3];
	gpCreateInfo.topology = vkr::PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	gpCreateInfo.polygonMode = vkr::POLYGON_MODE_FILL;
	gpCreateInfo.cullMode = vkr::CULL_MODE_BACK;
	gpCreateInfo.frontFace = vkr::FRONT_FACE_CCW;
	gpCreateInfo.depthReadEnable = true;
	gpCreateInfo.depthWriteEnable = true;
	gpCreateInfo.blendModes[0] = vkr::BLEND_MODE_NONE;
	gpCreateInfo.outputState.renderTargetCount = 1;
	gpCreateInfo.outputState.renderTargetFormats[0] = render.GetSwapChain().GetColorFormat();
	gpCreateInfo.outputState.depthStencilFormat = render.GetSwapChain().GetDepthFormat();
	gpCreateInfo.pipelineInterface = pOutput->pInterface;

	CHECKED_CALL(device.CreateGraphicsPipeline(gpCreateInfo, &pOutput->pPipeline));

	pOutput->textures.resize(3);
	if (material.pbr_metallic_roughness.base_color_texture.texture == nullptr) {
		float3 color = glm::make_vec3(material.pbr_metallic_roughness.base_color_factor);
		loadTexture(ColorToBitmap(color), pQueue, &pOutput->textures[0]);
	}
	else {
		const auto& texture_path = material.pbr_metallic_roughness.base_color_texture;
		loadTexture(gltfFolder, texture_path, pQueue, pTextureCache, &pOutput->textures[0]);
	}

	if (material.normal_texture.texture == nullptr) {
		loadTexture(ColorToBitmap(float3(0.f, 0.f, 1.f)), pQueue, &pOutput->textures[1]);
	}
	else {
		loadTexture(gltfFolder, material.normal_texture, pQueue, pTextureCache, &pOutput->textures[1]);
	}

	if (material.pbr_metallic_roughness.metallic_roughness_texture.texture == nullptr) {
		const auto& mtl = material.pbr_metallic_roughness;
		float3      color = float3(mtl.metallic_factor, mtl.roughness_factor, 0.f);
		loadTexture(ColorToBitmap(color), pQueue, &pOutput->textures[2]);
	}
	else {
		const auto& texture_path = material.pbr_metallic_roughness.metallic_roughness_texture;
		loadTexture(gltfFolder, texture_path, pQueue, pTextureCache, &pOutput->textures[2]);
	}
}

void Example_019::loadPrimitive(const cgltf_primitive& primitive, vkr::BufferPtr pStagingBuffer, vkr::Queue* pQueue, Primitive* pOutput)
{
	auto& device = GetRenderDevice();

	vkr::ScopeDestroyer SCOPED_DESTROYER(pQueue->GetDevice());
	ASSERT_MSG(primitive.type == cgltf_primitive_type_triangles, "only supporting tri primitives for now.");
	ASSERT_MSG(!primitive.has_draco_mesh_compression, "draco compression not supported yet.");
	ASSERT_MSG(primitive.indices != nullptr, "only primitives with indices are supported for now.");

	// Attribute accessors.
	constexpr size_t                                   POSITION_INDEX = 0;
	constexpr size_t                                   UV_INDEX = 1;
	constexpr size_t                                   NORMAL_INDEX = 2;
	constexpr size_t                                   TANGENT_INDEX = 3;
	constexpr size_t                                   ATTRIBUTE_COUNT = 4;
	std::array<const cgltf_accessor*, ATTRIBUTE_COUNT> accessors;
	GetAccessorsForPrimitive(primitive, &accessors[POSITION_INDEX], &accessors[UV_INDEX], &accessors[NORMAL_INDEX], &accessors[TANGENT_INDEX]);

	const cgltf_accessor& indices = *primitive.indices;
	const cgltf_component_type indicesTypes = indices.component_type;

	vkr::MeshPtr targetMesh;
	{
		// Indices.
		ASSERT_MSG(indicesTypes == cgltf_component_type_r_16u || indicesTypes == cgltf_component_type_r_32u, "only 32u or 16u are supported for indices.");

		// Create mesh.
		vkr::MeshCreateInfo ci;

		ci.indexType = indicesTypes == cgltf_component_type_r_16u
			? vkr::IndexType::Uint16
			: vkr::IndexType::Uint32;
		ci.indexCount = static_cast<uint32_t>(indices.count);
		ci.vertexCount = static_cast<uint32_t>(accessors[POSITION_INDEX]->count);
		ci.memoryUsage = vkr::MemoryUsage::GPUOnly;
		ci.vertexBufferCount = 4;

		for (size_t i = 0; i < accessors.size(); i++) {
			const auto& a = *accessors[i];
			const auto& bv = *a.buffer_view;
			ASSERT_MSG(a.type == cgltf_type_vec2 || a.type == cgltf_type_vec3 || a.type == cgltf_type_vec4, "Non supported accessor type.");
			ASSERT_MSG(a.component_type == cgltf_component_type_r_32f, "only float for POS, NORM, TEX are supported.");

			ci.vertexBuffers[i].attributeCount = 1;
			ci.vertexBuffers[i].vertexInputRate = vkr::VertexInputRate::Vertex;
			ci.vertexBuffers[i].attributes[0].format = a.type == cgltf_type_vec2 ? vkr::Format::R32G32_FLOAT
				: a.type == cgltf_type_vec3 ? vkr::Format::R32G32B32_FLOAT
				: vkr::Format::R32G32B32A32_FLOAT;
			ci.vertexBuffers[i].attributes[0].stride = static_cast<uint32_t>(bv.stride == 0 ? a.stride : bv.stride);

			constexpr std::array<vkr::VertexSemantic, ATTRIBUTE_COUNT> semantics = {
			vkr::VertexSemantic::Position,
			vkr::VertexSemantic::Texcoord,
			vkr::VertexSemantic::Normal,
			vkr::VertexSemantic::Tangent };
			ci.vertexBuffers[i].attributes[0].vertexSemantic = semantics[i];
		}
		CHECKED_CALL(device.CreateMesh(ci, &targetMesh));
		SCOPED_DESTROYER.AddObject(targetMesh);
	}

	// Copy geometry data to mesh.
	{
		const auto& bufferView = *indices.buffer_view;
		ASSERT_MSG(indicesTypes == cgltf_component_type_r_16u || indicesTypes == cgltf_component_type_r_32u, "only 32u or 16u are supported for indices.");
		ASSERT_MSG(bufferView.data == nullptr, "Doesn't support extra data");

		vkr::BufferToBufferCopyInfo copyInfo = {};
		copyInfo.size = targetMesh->GetIndexBuffer()->GetSize();
		copyInfo.srcBuffer.offset = indices.offset + bufferView.offset;
		copyInfo.dstBuffer.offset = 0;
		CHECKED_CALL(pQueue->CopyBufferToBuffer(&copyInfo, pStagingBuffer, targetMesh->GetIndexBuffer(), vkr::ResourceState::IndexBuffer, vkr::ResourceState::IndexBuffer));
		for (size_t i = 0; i < accessors.size(); i++) {
			const auto& bufferView = *accessors[i]->buffer_view;

			const auto& vertexBuffer = targetMesh->GetVertexBuffer(static_cast<uint32_t>(i));
			vkr::BufferToBufferCopyInfo copyInfo = {};
			copyInfo.size = vertexBuffer->GetSize();
			copyInfo.srcBuffer.offset = accessors[i]->offset + bufferView.offset;
			copyInfo.dstBuffer.offset = 0;
			CHECKED_CALL(pQueue->CopyBufferToBuffer(&copyInfo, pStagingBuffer, vertexBuffer, vkr::ResourceState::VertexBuffer, vkr::ResourceState::VertexBuffer));
		}
	}

	targetMesh->SetOwnership(vkr::Ownership::Reference);
	pOutput->mesh = targetMesh;
}

void Example_019::loadScene(
	const std::filesystem::path& filename,
	vkr::Queue* pQueue,
	vkr::DescriptorPool* pDescriptorPool,
	TextureCache* pTextureCache,
	std::vector<Object>* pObjects,
	std::vector<Primitive>* pPrimitives,
	std::vector<Material>* pMaterials)
{
	auto& device = GetRenderDevice();

	//Timer timerGlobal;
	//timerGlobal.Start();

	//Timer timerModelLoading;
	//timerModelLoading.Start();
	const std::filesystem::path gltfFolder = std::filesystem::path(filename).remove_filename();
	cgltf_data* data = nullptr;
	{
		const std::filesystem::path gltfFilePath = filename;
		ASSERT_MSG(gltfFilePath != "", "Cannot resolve asset path.");
		cgltf_options options = {};
		cgltf_result  result = cgltf_parse_file(&options, gltfFilePath.string().c_str(), &data);
		ASSERT_MSG(result == cgltf_result_success, "Failure while loading GLB file.");
		result = cgltf_validate(data);
		ASSERT_MSG(result == cgltf_result_success, "Failure while validating GLB file.");
		result = cgltf_load_buffers(&options, data, gltfFilePath.string().c_str());
		ASSERT_MSG(result == cgltf_result_success, "Failure while loading buffers.");

		ASSERT_MSG(data->buffers_count == 1, "Only supports one buffer for now.");
		ASSERT_MSG(data->buffers[0].data != nullptr, "Data not loaded. Was cgltf_load_buffer called?");
	}
	//const double timerModelLoadingElapsed = timerModelLoading.SecondsSinceStart();

	//Timer timerStagingBufferLoading;
	//timerStagingBufferLoading.Start();
	vkr::ScopeDestroyer SCOPED_DESTROYER(pQueue->GetDevice());
	// Copy main buffer data to staging buffer.
	vkr::BufferPtr stagingBuffer;
	{
		vkr::BufferCreateInfo ci = {};
		ci.size = data->buffers[0].size;
		ci.usageFlags.bits.transferSrc = true;
		ci.memoryUsage = vkr::MemoryUsage::CPUToGPU;

		CHECKED_CALL(device.CreateBuffer(ci, &stagingBuffer));
		SCOPED_DESTROYER.AddObject(stagingBuffer);
		CHECKED_CALL(stagingBuffer->CopyFromSource(static_cast<uint32_t>(data->buffers[0].size), data->buffers[0].data));
	}
	//const double timerStagingBufferLoadingElapsed = timerStagingBufferLoading.SecondsSinceStart();

	//Timer timerPrimitiveLoading;
	//timerPrimitiveLoading.Start();
	std::unordered_map<const cgltf_primitive*, size_t> primitiveToIndex;
	pPrimitives->resize(CountPrimitives(data->meshes, data->meshes_count));
	{
		size_t nextSlot = 0;
		for (size_t i = 0; i < data->meshes_count; i++)
		{
			const auto& mesh = data->meshes[i];
			for (size_t j = 0; j < mesh.primitives_count; j++)
			{
				loadPrimitive(mesh.primitives[j], stagingBuffer, pQueue, &(*pPrimitives)[nextSlot]);
				primitiveToIndex.insert({ &mesh.primitives[j], nextSlot });
				nextSlot++;
			}
		}
	}
	//const double timerPrimitiveLoadingElapsed = timerPrimitiveLoading.SecondsSinceStart();

	//Timer timerMaterialLoading;
	//timerMaterialLoading.Start();
	pMaterials->resize(data->materials_count);
	for (size_t i = 0; i < data->materials_count; i++)
	{
		loadMaterial(gltfFolder, data->materials[i], pQueue, pDescriptorPool, pTextureCache, &(*pMaterials)[i]);
	}
	//const double timerMaterialLoadingElapsed = timerMaterialLoading.SecondsSinceStart();

	//Timer timerNodeLoading;
	//timerNodeLoading.Start();
	loadNodes(data, pQueue, pDescriptorPool, pObjects, primitiveToIndex, pPrimitives, pMaterials);
	//const double timerNodeLoadingElapsed = timerNodeLoading.SecondsSinceStart();

	//printf("Scene loading time breakdown for '%s':\n", filename.u8string().c_str());
	//printf("\t             total: %lfs\n", timerGlobal.SecondsSinceStart());
	//printf("\t      GLtf parsing: %lfs\n", timerModelLoadingElapsed);
	//printf("\t    staging buffer: %lfs\n", timerStagingBufferLoadingElapsed);
	//printf("\tprimitives loading: %lfs\n", timerPrimitiveLoadingElapsed);
	//printf("\t materials loading: %lfs\n", timerMaterialLoadingElapsed);
	//printf("\t     nodes loading: %lfs\n", timerNodeLoadingElapsed);
}

void Example_019::loadNodes(
	const cgltf_data* data,
	vkr::Queue* pQueue,
	vkr::DescriptorPool* pDescriptorPool,
	std::vector<Object>* objects,
	const std::unordered_map<const cgltf_primitive*, size_t>& primitiveToIndex,
	std::vector<Primitive>* pPrimitives,
	std::vector<Material>* pMaterials)
{
	auto& device = GetRenderDevice();

	const size_t nodeCount = data->nodes_count;
	for (size_t i = 0; i < nodeCount; i++) {
		const auto& node = data->nodes[i];
		if (node.mesh == nullptr) {
			continue;
		}

		Object item;
		item.modelMatrix = ComputeObjectMatrix(&node);
		item.ITModelMatrix = glm::inverse(glm::transpose(item.modelMatrix));

		for (size_t j = 0; j < node.mesh->primitives_count; j++) {
			const size_t primitive_index = primitiveToIndex.at(&node.mesh->primitives[j]);
			// FIXME: support meshes with no material.
			// For now, assign the first available material.
			ASSERT_MSG(pMaterials->size() != 0, "Doesn't support GLTF files with no materials.");
			cgltf_material* mtl = node.mesh->primitives[j].material;
			const size_t    material_index = mtl == nullptr ? 0 : std::distance(data->materials, mtl);

			ASSERT_MSG(primitive_index < pPrimitives->size(), "Invalid GLB file. Primitive index out of range.");
			ASSERT_MSG(material_index < pMaterials->size(), "Invalid GLB file. Material index out of range.");
			Primitive* pPrimitive = &(*pPrimitives)[primitive_index];
			Material* pMaterial = &(*pMaterials)[material_index];

			vkr::DescriptorSet* pDescriptorSet = nullptr;
			CHECKED_CALL(device.AllocateDescriptorSet(pDescriptorPool, mSetLayout, &pDescriptorSet));
			item.renderables.emplace_back(pMaterial, pPrimitive, pDescriptorSet);
		}

		// Create uniform buffer.
		vkr::BufferCreateInfo bufferCreateInfo = {};
		bufferCreateInfo.size = RoundUp(512, vkr::CONSTANT_BUFFER_ALIGNMENT);
		bufferCreateInfo.usageFlags.bits.uniformBuffer = true;
		bufferCreateInfo.memoryUsage = vkr::MemoryUsage::CPUToGPU;
		CHECKED_CALL(device.CreateBuffer(bufferCreateInfo, &item.pUniformBuffer));

		objects->emplace_back(std::move(item));
	}
}