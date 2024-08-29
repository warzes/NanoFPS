#include "Base.h"
#include "Core.h"
#include "RenderCore.h"

#pragma region Format

namespace grfx {

#define UNCOMPRESSED_FORMAT(Name, Type, Aspect, BytesPerTexel, BytesPerComponent, Layout, Component, ComponentOffsets) \
{                                                                                                                  \
"" #Name,                                                                                                      \
FORMAT_DATA_TYPE_##Type,                                                                                   \
FORMAT_ASPECT_##Aspect,                                                                                    \
BytesPerTexel,                                                                                             \
/* blockWidth= */ 1,                                                                                       \
BytesPerComponent,                                                                                         \
FORMAT_LAYOUT_##Layout,                                                                                    \
FORMAT_COMPONENT_##Component,                                                                              \
OFFSETS_##ComponentOffsets                                                                                 \
}

#define COMPRESSED_FORMAT(Name, Type, BytesPerBlock, BlockWidth, Component) \
{                                                                       \
"" #Name,                                                           \
FORMAT_DATA_TYPE_##Type,                                        \
FORMAT_ASPECT_COLOR,                                            \
BytesPerBlock,                                                  \
BlockWidth,                                                     \
-1,                                                             \
FORMAT_LAYOUT_COMPRESSED,                                       \
FORMAT_COMPONENT_##Component,                                   \
OFFSETS_UNDEFINED                                               \
}

#define OFFSETS_UNDEFINED        { -1, -1, -1, -1 }
#define OFFSETS_R(R)             { R, -1, -1, -1 }
#define OFFSETS_RG(R, G)         { R,  G, -1, -1 }
#define OFFSETS_RGB(R, G, B)     { R,  G,  B, -1 }
#define OFFSETS_RGBA(R, G, B, A) { R,  G,  B,  A }

	// A static registry of format descriptions.
	// The order must match the order of the grfx::Format enum, so that
	// retrieving the description for a given format can be done in
	// constant time.
	constexpr FormatDesc formatDescs[] = {
		// clang-format off
		//                 +----------------------------------------------------------------------------------------------------------+
		//                 |                                              ,-> bytes per texel                                         |
		//                 |                                              |   ,-> bytes per component                                 |
		//                 | format name      | type     | aspect         |   |   Layout   | Components            | Offsets          |
		UNCOMPRESSED_FORMAT(UNDEFINED,          UNDEFINED, UNDEFINED,     0,  0,  UNDEFINED, UNDEFINED,              UNDEFINED),
		UNCOMPRESSED_FORMAT(R8_SNORM,           SNORM,     COLOR,         1,  1,  LINEAR,    RED,                       R(0)),
		UNCOMPRESSED_FORMAT(R8G8_SNORM,         SNORM,     COLOR,         2,  1,  LINEAR,    RED_GREEN,                RG(0, 1)),
		UNCOMPRESSED_FORMAT(R8G8B8_SNORM,       SNORM,     COLOR,         3,  1,  LINEAR,    RED_GREEN_BLUE,          RGB(0, 1, 2)),
		UNCOMPRESSED_FORMAT(R8G8B8A8_SNORM,     SNORM,     COLOR,         4,  1,  LINEAR,    RED_GREEN_BLUE_ALPHA,   RGBA(0, 1, 2, 3)),
		UNCOMPRESSED_FORMAT(B8G8R8_SNORM,       SNORM,     COLOR,         3,  1,  LINEAR,    RED_GREEN_BLUE,          RGB(2, 1, 0)),
		UNCOMPRESSED_FORMAT(B8G8R8A8_SNORM,     SNORM,     COLOR,         4,  1,  LINEAR,    RED_GREEN_BLUE_ALPHA,   RGBA(2, 1, 0, 3)),

		UNCOMPRESSED_FORMAT(R8_UNORM,           UNORM,     COLOR,         1,  1,  LINEAR,    RED,                       R(0)),
		UNCOMPRESSED_FORMAT(R8G8_UNORM,         UNORM,     COLOR,         2,  1,  LINEAR,    RED_GREEN,                RG(0, 1)),
		UNCOMPRESSED_FORMAT(R8G8B8_UNORM,       UNORM,     COLOR,         3,  1,  LINEAR,    RED_GREEN_BLUE,          RGB(0, 1, 2)),
		UNCOMPRESSED_FORMAT(R8G8B8A8_UNORM,     UNORM,     COLOR,         4,  1,  LINEAR,    RED_GREEN_BLUE_ALPHA,   RGBA(0, 1, 2, 3)),
		UNCOMPRESSED_FORMAT(B8G8R8_UNORM,       UNORM,     COLOR,         3,  1,  LINEAR,    RED_GREEN_BLUE,          RGB(2, 1, 0)),
		UNCOMPRESSED_FORMAT(B8G8R8A8_UNORM,     UNORM,     COLOR,         4,  1,  LINEAR,    RED_GREEN_BLUE_ALPHA,   RGBA(2, 1, 0, 3)),

		UNCOMPRESSED_FORMAT(R8_SINT,            SINT,      COLOR,         1,  1,  LINEAR,    RED,                       R(0)),
		UNCOMPRESSED_FORMAT(R8G8_SINT,          SINT,      COLOR,         2,  1,  LINEAR,    RED_GREEN,                RG(0, 1)),
		UNCOMPRESSED_FORMAT(R8G8B8_SINT,        SINT,      COLOR,         3,  1,  LINEAR,    RED_GREEN_BLUE,          RGB(0, 1, 2)),
		UNCOMPRESSED_FORMAT(R8G8B8A8_SINT,      SINT,      COLOR,         4,  1,  LINEAR,    RED_GREEN_BLUE_ALPHA,   RGBA(0, 1, 2, 3)),
		UNCOMPRESSED_FORMAT(B8G8R8_SINT,        SINT,      COLOR,         3,  1,  LINEAR,    RED_GREEN_BLUE,          RGB(2, 1, 0)),
		UNCOMPRESSED_FORMAT(B8G8R8A8_SINT,      SINT,      COLOR,         4,  1,  LINEAR,    RED_GREEN_BLUE_ALPHA,   RGBA(2, 1, 0, 3)),

		UNCOMPRESSED_FORMAT(R8_UINT,            UINT,      COLOR,         1,  1,  LINEAR,    RED,                       R(0)),
		UNCOMPRESSED_FORMAT(R8G8_UINT,          UINT,      COLOR,         2,  1,  LINEAR,    RED_GREEN,                RG(0, 1)),
		UNCOMPRESSED_FORMAT(R8G8B8_UINT,        UINT,      COLOR,         3,  1,  LINEAR,    RED_GREEN_BLUE,          RGB(0, 1, 2)),
		UNCOMPRESSED_FORMAT(R8G8B8A8_UINT,      UINT,      COLOR,         4,  1,  LINEAR,    RED_GREEN_BLUE_ALPHA,   RGBA(0, 1, 2, 3)),
		UNCOMPRESSED_FORMAT(B8G8R8_UINT,        UINT,      COLOR,         3,  1,  LINEAR,    RED_GREEN_BLUE,          RGB(2, 1, 0)),
		UNCOMPRESSED_FORMAT(B8G8R8A8_UINT,      UINT,      COLOR,         4,  1,  LINEAR,    RED_GREEN_BLUE_ALPHA,   RGBA(2, 1, 0, 3)),

		UNCOMPRESSED_FORMAT(R16_SNORM,          SNORM,     COLOR,         2,  2,  LINEAR,    RED,                       R(0)),
		UNCOMPRESSED_FORMAT(R16G16_SNORM,       SNORM,     COLOR,         4,  2,  LINEAR,    RED_GREEN,                RG(0, 2)),
		UNCOMPRESSED_FORMAT(R16G16B16_SNORM,    SNORM,     COLOR,         6,  2,  LINEAR,    RED_GREEN_BLUE,          RGB(0, 2, 4)),
		UNCOMPRESSED_FORMAT(R16G16B16A16_SNORM, SNORM,     COLOR,         8,  2,  LINEAR,    RED_GREEN_BLUE_ALPHA,   RGBA(0, 2, 4, 6)),

		UNCOMPRESSED_FORMAT(R16_UNORM,          UNORM,     COLOR,         2,  2,  LINEAR,    RED,                       R(0)),
		UNCOMPRESSED_FORMAT(R16G16_UNORM,       UNORM,     COLOR,         4,  2,  LINEAR,    RED_GREEN,                RG(0, 2)),
		UNCOMPRESSED_FORMAT(R16G16B16_UNORM,    UNORM,     COLOR,         6,  2,  LINEAR,    RED_GREEN_BLUE,          RGB(0, 2, 4)),
		UNCOMPRESSED_FORMAT(R16G16B16A16_UNORM, UNORM,     COLOR,         8,  2,  LINEAR,    RED_GREEN_BLUE_ALPHA,   RGBA(0, 2, 4, 6)),

		UNCOMPRESSED_FORMAT(R16_SINT,           SINT,      COLOR,         2,  2,  LINEAR,    RED,                       R(0)),
		UNCOMPRESSED_FORMAT(R16G16_SINT,        SINT,      COLOR,         4,  2,  LINEAR,    RED_GREEN,                RG(0, 2)),
		UNCOMPRESSED_FORMAT(R16G16B16_SINT,     SINT,      COLOR,         6,  2,  LINEAR,    RED_GREEN_BLUE,          RGB(0, 2, 4)),
		UNCOMPRESSED_FORMAT(R16G16B16A16_SINT,  SINT,      COLOR,         8,  2,  LINEAR,    RED_GREEN_BLUE_ALPHA,   RGBA(0, 2, 4, 6)),

		UNCOMPRESSED_FORMAT(R16_UINT,           UINT,      COLOR,         2,  2,  LINEAR,    RED,                       R(0)),
		UNCOMPRESSED_FORMAT(R16G16_UINT,        UINT,      COLOR,         4,  2,  LINEAR,    RED_GREEN,                RG(0, 2)),
		UNCOMPRESSED_FORMAT(R16G16B16_UINT,     UINT,      COLOR,         6,  2,  LINEAR,    RED_GREEN_BLUE,          RGB(0, 2, 4)),
		UNCOMPRESSED_FORMAT(R16G16B16A16_UINT,  UINT,      COLOR,         8,  2,  LINEAR,    RED_GREEN_BLUE_ALPHA,   RGBA(0, 2, 4, 6)),

		UNCOMPRESSED_FORMAT(R16_FLOAT,          FLOAT,     COLOR,         2,  2,  LINEAR,    RED,                       R(0)),
		UNCOMPRESSED_FORMAT(R16G16_FLOAT,       FLOAT,     COLOR,         4,  2,  LINEAR,    RED_GREEN,                RG(0, 2)),
		UNCOMPRESSED_FORMAT(R16G16B16_FLOAT,    FLOAT,     COLOR,         6,  2,  LINEAR,    RED_GREEN_BLUE,          RGB(0, 2, 4)),
		UNCOMPRESSED_FORMAT(R16G16B16A16_FLOAT, FLOAT,     COLOR,         8,  2,  LINEAR,    RED_GREEN_BLUE_ALPHA,   RGBA(0, 2, 4, 6)),

		UNCOMPRESSED_FORMAT(R32_SINT,           SINT,      COLOR,         4,  2,  LINEAR,    RED,                       R(0)),
		UNCOMPRESSED_FORMAT(R32G32_SINT,        SINT,      COLOR,         8,  2,  LINEAR,    RED_GREEN,                RG(0, 4)),
		UNCOMPRESSED_FORMAT(R32G32B32_SINT,     SINT,      COLOR,         12, 2,  LINEAR,    RED_GREEN_BLUE,          RGB(0, 4, 8)),
		UNCOMPRESSED_FORMAT(R32G32B32A32_SINT,  SINT,      COLOR,         16, 2,  LINEAR,    RED_GREEN_BLUE_ALPHA,   RGBA(0, 4, 8, 12)),

		UNCOMPRESSED_FORMAT(R32_UINT,           UINT,      COLOR,         4,  2,  LINEAR,    RED,                       R(0)),
		UNCOMPRESSED_FORMAT(R32G32_UINT,        UINT,      COLOR,         8,  2,  LINEAR,    RED_GREEN,                RG(0, 4)),
		UNCOMPRESSED_FORMAT(R32G32B32_UINT,     UINT,      COLOR,         12, 2,  LINEAR,    RED_GREEN_BLUE,          RGB(0, 4, 8)),
		UNCOMPRESSED_FORMAT(R32G32B32A32_UINT,  UINT,      COLOR,         16, 2,  LINEAR,    RED_GREEN_BLUE_ALPHA,   RGBA(0, 4, 8, 12)),

		UNCOMPRESSED_FORMAT(R32_FLOAT,          FLOAT,     COLOR,         4,  2,  LINEAR,    RED,                       R(0)),
		UNCOMPRESSED_FORMAT(R32G32_FLOAT,       FLOAT,     COLOR,         8,  2,  LINEAR,    RED_GREEN,                RG(0, 4)),
		UNCOMPRESSED_FORMAT(R32G32B32_FLOAT,    FLOAT,     COLOR,         12, 2,  LINEAR,    RED_GREEN_BLUE,          RGB(0, 4, 8)),
		UNCOMPRESSED_FORMAT(R32G32B32A32_FLOAT, FLOAT,     COLOR,         16, 2,  LINEAR,    RED_GREEN_BLUE_ALPHA,   RGBA(0, 4, 8, 12)),

		UNCOMPRESSED_FORMAT(S8_UINT,            UINT,      STENCIL,       1,  1,  LINEAR,    STENCIL,                  RG(-1, 0)),
		UNCOMPRESSED_FORMAT(D16_UNORM,          UNORM,     DEPTH,         2,  2,  LINEAR,    DEPTH,                    RG(0, -1)),
		UNCOMPRESSED_FORMAT(D32_FLOAT,          FLOAT,     DEPTH,         4,  4,  LINEAR,    DEPTH,                    RG(0, -1)),

		UNCOMPRESSED_FORMAT(D16_UNORM_S8_UINT,  UNORM,     DEPTH_STENCIL, 3,  2,  LINEAR,    DEPTH_STENCIL,            RG(0, 2)),
		UNCOMPRESSED_FORMAT(D24_UNORM_S8_UINT,  UNORM,     DEPTH_STENCIL, 4,  3,  LINEAR,    DEPTH_STENCIL,            RG(0, 3)),
		UNCOMPRESSED_FORMAT(D32_FLOAT_S8_UINT,  FLOAT,     DEPTH_STENCIL, 5,  4,  LINEAR,    DEPTH_STENCIL,            RG(0, 4)),

		UNCOMPRESSED_FORMAT(R8_SRGB,            SRGB,     COLOR,          1,  1,  LINEAR,    RED,                       R(0)),
		UNCOMPRESSED_FORMAT(R8G8_SRBG,          SRGB,     COLOR,          2,  1,  LINEAR,    RED_GREEN,                RG(0, 1)),
		UNCOMPRESSED_FORMAT(R8G8B8_SRBG,        SRGB,     COLOR,          3,  1,  LINEAR,    RED_GREEN_BLUE,          RGB(0, 1, 2)),
		UNCOMPRESSED_FORMAT(R8G8B8A8_SRBG,      SRGB,     COLOR,          4,  1,  LINEAR,    RED_GREEN_BLUE_ALPHA,   RGBA(0, 1, 2, 3)),
		UNCOMPRESSED_FORMAT(B8G8R8_SRBG,        SRGB,     COLOR,          3,  1,  LINEAR,    RED_GREEN_BLUE,          RGB(2, 1, 0)),
		UNCOMPRESSED_FORMAT(B8G8R8A8_SRBG,      SRGB,     COLOR,          4,  1,  LINEAR,    RED_GREEN_BLUE_ALPHA,   RGBA(2, 1, 0, 3)),

		// We don't support retrieving component size or byte offsets for packed formats.
		UNCOMPRESSED_FORMAT(R10G10B10A2_UNORM,  UNORM,    COLOR,          4,  -1, PACKED,    RED_GREEN_BLUE_ALPHA,   UNDEFINED),
		UNCOMPRESSED_FORMAT(R11G11B10_FLOAT,    FLOAT,    COLOR,          4,  -1, PACKED,    RED_GREEN_BLUE,         UNDEFINED),

		// We don't support retrieving component size or byte offsets for compressed formats.
		// We don't support non-square blocks for compressed textures.
		//                +-----------------------------------------------------+
		//                |                       ,-> bytes per block           |
		//                |                       |  ,-> block width            |
		//                | format name | type    |  |   Components             |
		COMPRESSED_FORMAT(BC1_RGBA_SRGB , SRGB,   8, 4,  RED_GREEN_BLUE_ALPHA),
		COMPRESSED_FORMAT(BC1_RGBA_UNORM, UNORM,  8, 4,  RED_GREEN_BLUE_ALPHA),
		COMPRESSED_FORMAT(BC1_RGB_SRGB  , SRGB,   8, 4,  RED_GREEN_BLUE),
		COMPRESSED_FORMAT(BC1_RGB_UNORM , UNORM,  8, 4,  RED_GREEN_BLUE),
		COMPRESSED_FORMAT(BC2_SRGB      , SRGB,  16, 4,  RED_GREEN_BLUE_ALPHA),
		COMPRESSED_FORMAT(BC2_UNORM     , UNORM, 16, 4,  RED_GREEN_BLUE_ALPHA),
		COMPRESSED_FORMAT(BC3_SRGB      , SRGB,  16, 4,  RED_GREEN_BLUE_ALPHA),
		COMPRESSED_FORMAT(BC3_UNORM     , UNORM, 16, 4,  RED_GREEN_BLUE_ALPHA),
		COMPRESSED_FORMAT(BC4_UNORM     , UNORM,  8, 4,  RED),
		COMPRESSED_FORMAT(BC4_SNORM     , SNORM,  8, 4,  RED),
		COMPRESSED_FORMAT(BC5_UNORM     , UNORM, 16, 4,  RED_GREEN),
		COMPRESSED_FORMAT(BC5_SNORM     , SNORM, 16, 4,  RED_GREEN),
		COMPRESSED_FORMAT(BC6H_UFLOAT   , FLOAT, 16, 4,  RED_GREEN_BLUE),
		COMPRESSED_FORMAT(BC6H_SFLOAT   , FLOAT, 16, 4,  RED_GREEN_BLUE),
		COMPRESSED_FORMAT(BC7_UNORM     , UNORM, 16, 4,  RED_GREEN_BLUE_ALPHA),
		COMPRESSED_FORMAT(BC7_SRGB      , SRGB,  16, 4,  RED_GREEN_BLUE_ALPHA),

		#undef COMPRESSED_FORMAT
		#undef UNCOMPRESSED_FORMAT
		#undef OFFSETS_UNDEFINED
		#undef OFFSETS_R
		#undef OFFSETS_RG
		#undef OFFSETS_RGB
		#undef OFFSETS_RGBA
	};
	// clang-format on
	constexpr size_t formatDescsSize = sizeof(formatDescs) / sizeof(FormatDesc);
	static_assert(formatDescsSize == FORMAT_COUNT, "Missing format descriptions");

	const FormatDesc* GetFormatDescription(grfx::Format format)
	{
		uint32_t formatIndex = static_cast<uint32_t>(format);
		ASSERT_MSG(format != grfx::FORMAT_UNDEFINED && formatIndex < formatDescsSize, "invalid format");
		return &formatDescs[formatIndex];
	}

	const char* ToString(grfx::Format format)
	{
		uint32_t formatIndex = static_cast<uint32_t>(format);
		ASSERT_MSG(formatIndex < formatDescsSize, "invalid format");
		return formatDescs[formatIndex].name;
	}

} // namespace grfx

#pragma endregion

#pragma region Helper

namespace grfx {

	// -------------------------------------------------------------------------------------------------
	// ColorComponentFlags
	// -------------------------------------------------------------------------------------------------
	ColorComponentFlags ColorComponentFlags::RGBA()
	{
		ColorComponentFlags flags = ColorComponentFlags(COLOR_COMPONENT_R | COLOR_COMPONENT_G | COLOR_COMPONENT_B | COLOR_COMPONENT_A);
		return flags;
	}

	// -------------------------------------------------------------------------------------------------
	// ImageUsageFlags
	// -------------------------------------------------------------------------------------------------
	ImageUsageFlags ImageUsageFlags::SampledImage()
	{
		ImageUsageFlags flags = ImageUsageFlags(grfx::IMAGE_USAGE_SAMPLED);
		return flags;
	}

	// -------------------------------------------------------------------------------------------------
	// VertexBinding
	// -------------------------------------------------------------------------------------------------
	void VertexBinding::SetBinding(uint32_t binding)
	{
		mBinding = binding;
		for (auto& elem : mAttributes) {
			elem.binding = binding;
		}
	}

	void VertexBinding::SetStride(uint32_t stride)
	{
		mStride = stride;
	}

	Result VertexBinding::GetAttribute(uint32_t index, const grfx::VertexAttribute** ppAttribute) const
	{
		if (!IsIndexInRange(index, mAttributes)) {
			return ERROR_OUT_OF_RANGE;
		}
		*ppAttribute = &mAttributes[index];
		return SUCCESS;
	}

	uint32_t VertexBinding::GetAttributeIndex(grfx::VertexSemantic semantic) const
	{
		auto it = FindIf(
			mAttributes,
			[semantic](const grfx::VertexAttribute& elem) -> bool {
				bool isMatch = (elem.semantic == semantic);
				return isMatch; });
		if (it == std::end(mAttributes)) {
			return VALUE_IGNORED;
		}
		uint32_t index = static_cast<uint32_t>(std::distance(std::begin(mAttributes), it));
		return index;
	}

	VertexBinding& VertexBinding::AppendAttribute(const grfx::VertexAttribute& attribute)
	{
		mAttributes.push_back(attribute);

		if (mInputRate == grfx::VertexBinding::kInvalidVertexInputRate) {
			mInputRate = attribute.inputRate;
		}

		// Caluclate offset for inserted attribute
		if (mAttributes.size() > 1) {
			size_t i1 = mAttributes.size() - 1;
			size_t i0 = i1 - 1;
			if (mAttributes[i1].offset == APPEND_OFFSET_ALIGNED) {
				uint32_t prevOffset = mAttributes[i0].offset;
				uint32_t prevSize = grfx::GetFormatDescription(mAttributes[i0].format)->bytesPerTexel;
				mAttributes[i1].offset = prevOffset + prevSize;
			}
		}
		// Size of mAttributes should be 1
		else {
			if (mAttributes[0].offset == APPEND_OFFSET_ALIGNED) {
				mAttributes[0].offset = 0;
			}
		}

		// Calculate stride
		mStride = 0;
		for (auto& elem : mAttributes) {
			uint32_t size = grfx::GetFormatDescription(elem.format)->bytesPerTexel;
			mStride += size;
		}

		return *this;
	}

	grfx::VertexBinding& VertexBinding::operator+=(const grfx::VertexAttribute& rhs)
	{
		AppendAttribute(rhs);
		return *this;
	}

	// -------------------------------------------------------------------------------------------------
	// VertexDescription
	// -------------------------------------------------------------------------------------------------
	Result VertexDescription::GetBinding(uint32_t index, const grfx::VertexBinding** ppBinding) const
	{
		if (!IsIndexInRange(index, mBindings)) {
			return ERROR_OUT_OF_RANGE;
		}
		if (!IsNull(ppBinding)) {
			*ppBinding = &mBindings[index];
		}
		return SUCCESS;
	}

	const grfx::VertexBinding* VertexDescription::GetBinding(uint32_t index) const
	{
		const VertexBinding* ptr = nullptr;
		GetBinding(index, &ptr);
		return ptr;
	}

	uint32_t VertexDescription::GetBindingIndex(uint32_t binding) const
	{
		auto it = FindIf(
			mBindings,
			[binding](const grfx::VertexBinding& elem) -> bool {
				bool isMatch = (elem.GetBinding() == binding);
				return isMatch; });
		if (it == std::end(mBindings)) {
			return VALUE_IGNORED;
		}
		uint32_t index = static_cast<uint32_t>(std::distance(std::begin(mBindings), it));
		return index;
	}

	Result VertexDescription::AppendBinding(const grfx::VertexBinding& binding)
	{
		auto it = FindIf(
			mBindings,
			[binding](const grfx::VertexBinding& elem) -> bool {
				bool isSame = (elem.GetBinding() == binding.GetBinding());
				return isSame; });
		if (it != std::end(mBindings)) {
			return ERROR_DUPLICATE_ELEMENT;
		}
		mBindings.push_back(binding);
		return SUCCESS;
	}

} // namespace grfx

#pragma endregion

#pragma region Utils

namespace grfx {

	const char* ToString(grfx::Api value)
	{
		switch (value) {
		default: break;
		case grfx::API_VK_1_1: return "Vulkan 1.1"; break;
		case grfx::API_VK_1_2: return "Vulkan 1.2"; break;
		case grfx::API_VK_1_3: return "Vulkan 1.3"; break;
		case grfx::API_DX_12_0: return "Direct3D 12.0"; break;
		case grfx::API_DX_12_1: return "Direct3D 12.1"; break;
		}
		return "<unknown graphics API>";
	}

	const char* ToString(grfx::DescriptorType value)
	{
		// clang-format off
		switch (value) {
		default: break;
		case grfx::DESCRIPTOR_TYPE_SAMPLER: return "grfx::DESCRIPTOR_TYPE_SAMPLER"; break;
		case grfx::DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER: return "grfx::DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER"; break;
		case grfx::DESCRIPTOR_TYPE_SAMPLED_IMAGE: return "grfx::DESCRIPTOR_TYPE_SAMPLED_IMAGE"; break;
		case grfx::DESCRIPTOR_TYPE_STORAGE_IMAGE: return "grfx::DESCRIPTOR_TYPE_STORAGE_IMAGE"; break;
		case grfx::DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER: return "grfx::DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER  "; break;
		case grfx::DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER: return "grfx::DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER  "; break;
		case grfx::DESCRIPTOR_TYPE_UNIFORM_BUFFER: return "grfx::DESCRIPTOR_TYPE_UNIFORM_BUFFER"; break;
		case grfx::DESCRIPTOR_TYPE_RAW_STORAGE_BUFFER: return "grfx::DESCRIPTOR_TYPE_RAW_STORAGE_BUFFER"; break;
		case grfx::DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC: return "grfx::DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC"; break;
		case grfx::DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC: return "grfx::DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC"; break;
		case grfx::DESCRIPTOR_TYPE_INPUT_ATTACHMENT: return "grfx::DESCRIPTOR_TYPE_INPUT_ATTACHMENT"; break;
		}
		// clang-format on
		return "<unknown descriptor type>";
	}

	const char* ToString(grfx::VertexSemantic value)
	{
		// clang-format off
		switch (value) {
		default: break;
		case grfx::VERTEX_SEMANTIC_POSITION: return "POSITION"; break;
		case grfx::VERTEX_SEMANTIC_NORMAL: return "NORMAL"; break;
		case grfx::VERTEX_SEMANTIC_COLOR: return "COLOR"; break;
		case grfx::VERTEX_SEMANTIC_TANGENT: return "TANGENT"; break;
		case grfx::VERTEX_SEMANTIC_BITANGENT: return "BITANGENT"; break;
		case grfx::VERTEX_SEMANTIC_TEXCOORD: return "TEXCOORD"; break;
		case grfx::VERTEX_SEMANTIC_TEXCOORD0: return "TEXCOORD0"; break;
		case grfx::VERTEX_SEMANTIC_TEXCOORD1: return "TEXCOORD1"; break;
		case grfx::VERTEX_SEMANTIC_TEXCOORD2: return "TEXCOORD2"; break;
		case grfx::VERTEX_SEMANTIC_TEXCOORD3: return "TEXCOORD3"; break;
		case grfx::VERTEX_SEMANTIC_TEXCOORD4: return "TEXCOORD4"; break;
		case grfx::VERTEX_SEMANTIC_TEXCOORD5: return "TEXCOORD5"; break;
		case grfx::VERTEX_SEMANTIC_TEXCOORD6: return "TEXCOORD6"; break;
		case grfx::VERTEX_SEMANTIC_TEXCOORD7: return "TEXCOORD7"; break;
		case grfx::VERTEX_SEMANTIC_TEXCOORD8: return "TEXCOORD8"; break;
		case grfx::VERTEX_SEMANTIC_TEXCOORD9: return "TEXCOORD9"; break;
		case grfx::VERTEX_SEMANTIC_TEXCOORD10: return "TEXCOORD10"; break;
		case grfx::VERTEX_SEMANTIC_TEXCOORD11: return "TEXCOORD11"; break;
		case grfx::VERTEX_SEMANTIC_TEXCOORD12: return "TEXCOORD12"; break;
		case grfx::VERTEX_SEMANTIC_TEXCOORD13: return "TEXCOORD13"; break;
		case grfx::VERTEX_SEMANTIC_TEXCOORD14: return "TEXCOORD14"; break;
		case grfx::VERTEX_SEMANTIC_TEXCOORD15: return "TEXCOORD15"; break;
		case grfx::VERTEX_SEMANTIC_TEXCOORD16: return "TEXCOORD16"; break;
		case grfx::VERTEX_SEMANTIC_TEXCOORD17: return "TEXCOORD17"; break;
		case grfx::VERTEX_SEMANTIC_TEXCOORD18: return "TEXCOORD18"; break;
		case grfx::VERTEX_SEMANTIC_TEXCOORD19: return "TEXCOORD19"; break;
		case grfx::VERTEX_SEMANTIC_TEXCOORD20: return "TEXCOORD20"; break;
		case grfx::VERTEX_SEMANTIC_TEXCOORD21: return "TEXCOORD21"; break;
		case grfx::VERTEX_SEMANTIC_TEXCOORD22: return "TEXCOORD22"; break;
		}
		// clang-format on
		return "";
	}

	const char* ToString(grfx::IndexType value)
	{
		switch (value) {
		default: break;
		case grfx::INDEX_TYPE_UNDEFINED: return "INDEX_TYPE_UNDEFINED";
		case grfx::INDEX_TYPE_UINT16: return "INDEX_TYPE_UINT16";
		case grfx::INDEX_TYPE_UINT32: return "INDEX_TYPE_UINT32";
		}
		return "<unknown grfx::IndexType>";
	}

	uint32_t IndexTypeSize(grfx::IndexType value)
	{
		// clang-format off
		switch (value) {
		default: break;
		case grfx::INDEX_TYPE_UINT16: return sizeof(uint16_t); break;
		case grfx::INDEX_TYPE_UINT32: return sizeof(uint32_t); break;
		}
		// clang-format on
		return 0;
	}

	grfx::Format VertexSemanticFormat(grfx::VertexSemantic value)
	{
		// clang-format off
		switch (value) {
		default: break;
		case grfx::VERTEX_SEMANTIC_POSITION: return grfx::FORMAT_R32G32B32_FLOAT; break;
		case grfx::VERTEX_SEMANTIC_NORMAL: return grfx::FORMAT_R32G32B32_FLOAT; break;
		case grfx::VERTEX_SEMANTIC_COLOR: return grfx::FORMAT_R32G32B32_FLOAT; break;
		case grfx::VERTEX_SEMANTIC_TANGENT: return grfx::FORMAT_R32G32B32_FLOAT; break;
		case grfx::VERTEX_SEMANTIC_BITANGENT: return grfx::FORMAT_R32G32B32_FLOAT; break;
		case grfx::VERTEX_SEMANTIC_TEXCOORD: return grfx::FORMAT_R32G32_FLOAT; break;
		case grfx::VERTEX_SEMANTIC_TEXCOORD0: return grfx::FORMAT_R32G32_FLOAT; break;
		case grfx::VERTEX_SEMANTIC_TEXCOORD1: return grfx::FORMAT_R32G32_FLOAT; break;
		case grfx::VERTEX_SEMANTIC_TEXCOORD2: return grfx::FORMAT_R32G32_FLOAT; break;
		case grfx::VERTEX_SEMANTIC_TEXCOORD3: return grfx::FORMAT_R32G32_FLOAT; break;
		case grfx::VERTEX_SEMANTIC_TEXCOORD4: return grfx::FORMAT_R32G32_FLOAT; break;
		case grfx::VERTEX_SEMANTIC_TEXCOORD5: return grfx::FORMAT_R32G32_FLOAT; break;
		case grfx::VERTEX_SEMANTIC_TEXCOORD6: return grfx::FORMAT_R32G32_FLOAT; break;
		case grfx::VERTEX_SEMANTIC_TEXCOORD7: return grfx::FORMAT_R32G32_FLOAT; break;
		case grfx::VERTEX_SEMANTIC_TEXCOORD8: return grfx::FORMAT_R32G32_FLOAT; break;
		case grfx::VERTEX_SEMANTIC_TEXCOORD9: return grfx::FORMAT_R32G32_FLOAT; break;
		case grfx::VERTEX_SEMANTIC_TEXCOORD10: return grfx::FORMAT_R32G32_FLOAT; break;
		case grfx::VERTEX_SEMANTIC_TEXCOORD11: return grfx::FORMAT_R32G32_FLOAT; break;
		case grfx::VERTEX_SEMANTIC_TEXCOORD12: return grfx::FORMAT_R32G32_FLOAT; break;
		case grfx::VERTEX_SEMANTIC_TEXCOORD13: return grfx::FORMAT_R32G32_FLOAT; break;
		case grfx::VERTEX_SEMANTIC_TEXCOORD14: return grfx::FORMAT_R32G32_FLOAT; break;
		case grfx::VERTEX_SEMANTIC_TEXCOORD15: return grfx::FORMAT_R32G32_FLOAT; break;
		case grfx::VERTEX_SEMANTIC_TEXCOORD16: return grfx::FORMAT_R32G32_FLOAT; break;
		case grfx::VERTEX_SEMANTIC_TEXCOORD17: return grfx::FORMAT_R32G32_FLOAT; break;
		case grfx::VERTEX_SEMANTIC_TEXCOORD18: return grfx::FORMAT_R32G32_FLOAT; break;
		case grfx::VERTEX_SEMANTIC_TEXCOORD19: return grfx::FORMAT_R32G32_FLOAT; break;
		case grfx::VERTEX_SEMANTIC_TEXCOORD20: return grfx::FORMAT_R32G32_FLOAT; break;
		case grfx::VERTEX_SEMANTIC_TEXCOORD21: return grfx::FORMAT_R32G32_FLOAT; break;
		case grfx::VERTEX_SEMANTIC_TEXCOORD22: return grfx::FORMAT_R32G32_FLOAT; break;
		}
		// clang-format on
		return grfx::FORMAT_UNDEFINED;
	}

	const char* ToString(const gli::target& target)
	{
		// clang-format off
		switch (target) {
		case gli::TARGET_1D:         return "TARGET_1D";
		case gli::TARGET_1D_ARRAY:   return "TARGET_1D_ARRAY";
		case gli::TARGET_2D:         return "TARGET_2D";
		case gli::TARGET_2D_ARRAY:   return "TARGET_2D_ARRAY";
		case gli::TARGET_3D:         return "TARGET_3D";
		case gli::TARGET_RECT:       return "TARGET_RECT";
		case gli::TARGET_RECT_ARRAY: return "TARGET_RECT_ARRAY";
		case gli::TARGET_CUBE:       return "TARGET_CUBE";
		case gli::TARGET_CUBE_ARRAY: return "TARGET_CUBE_ARRAY";
		}
		// clang-format on
		return "TARGET_UNKNOWN";
	}

	const char* ToString(const gli::format& format)
	{
		switch (format) {
		case gli::FORMAT_UNDEFINED: return "FORMAT_UNDEFINED";
		case gli::FORMAT_RG4_UNORM_PACK8: return "FORMAT_RG4_UNORM_PACK8";
		case gli::FORMAT_RGBA4_UNORM_PACK16: return "FORMAT_RGBA4_UNORM_PACK16";
		case gli::FORMAT_BGRA4_UNORM_PACK16: return "FORMAT_BGRA4_UNORM_PACK16";
		case gli::FORMAT_R5G6B5_UNORM_PACK16: return "FORMAT_R5G6B5_UNORM_PACK16";
		case gli::FORMAT_B5G6R5_UNORM_PACK16: return "FORMAT_B5G6R5_UNORM_PACK16";
		case gli::FORMAT_RGB5A1_UNORM_PACK16: return "FORMAT_RGB5A1_UNORM_PACK16";
		case gli::FORMAT_BGR5A1_UNORM_PACK16: return "FORMAT_BGR5A1_UNORM_PACK16";
		case gli::FORMAT_A1RGB5_UNORM_PACK16: return "FORMAT_A1RGB5_UNORM_PACK16";

		case gli::FORMAT_R8_UNORM_PACK8: return "FORMAT_R8_UNORM_PACK8";
		case gli::FORMAT_R8_SNORM_PACK8: return "FORMAT_R8_SNORM_PACK8";
		case gli::FORMAT_R8_USCALED_PACK8: return "FORMAT_R8_USCALED_PACK8";
		case gli::FORMAT_R8_SSCALED_PACK8: return "FORMAT_R8_SSCALED_PACK8";
		case gli::FORMAT_R8_UINT_PACK8: return "FORMAT_R8_UINT_PACK8";
		case gli::FORMAT_R8_SINT_PACK8: return "FORMAT_R8_SINT_PACK8";
		case gli::FORMAT_R8_SRGB_PACK8: return "FORMAT_R8_SRGB_PACK8";

		case gli::FORMAT_RG8_UNORM_PACK8: return "FORMAT_RG8_UNORM_PACK8";
		case gli::FORMAT_RG8_SNORM_PACK8: return "FORMAT_RG8_SNORM_PACK8";
		case gli::FORMAT_RG8_USCALED_PACK8: return "FORMAT_RG8_USCALED_PACK8";
		case gli::FORMAT_RG8_SSCALED_PACK8: return "FORMAT_RG8_SSCALED_PACK8";
		case gli::FORMAT_RG8_UINT_PACK8: return "FORMAT_RG8_UINT_PACK8";
		case gli::FORMAT_RG8_SINT_PACK8: return "FORMAT_RG8_SINT_PACK8";
		case gli::FORMAT_RG8_SRGB_PACK8: return "FORMAT_RG8_SRGB_PACK8";

		case gli::FORMAT_RGB8_UNORM_PACK8: return "FORMAT_RGB8_UNORM_PACK8";
		case gli::FORMAT_RGB8_SNORM_PACK8: return "FORMAT_RGB8_SNORM_PACK8";
		case gli::FORMAT_RGB8_USCALED_PACK8: return "FORMAT_RGB8_USCALED_PACK8";
		case gli::FORMAT_RGB8_SSCALED_PACK8: return "FORMAT_RGB8_SSCALED_PACK8";
		case gli::FORMAT_RGB8_UINT_PACK8: return "FORMAT_RGB8_UINT_PACK8";
		case gli::FORMAT_RGB8_SINT_PACK8: return "FORMAT_RGB8_SINT_PACK8";
		case gli::FORMAT_RGB8_SRGB_PACK8: return "FORMAT_RGB8_SRGB_PACK8";

		case gli::FORMAT_BGR8_UNORM_PACK8: return "FORMAT_BGR8_UNORM_PACK8";
		case gli::FORMAT_BGR8_SNORM_PACK8: return "FORMAT_BGR8_SNORM_PACK8";
		case gli::FORMAT_BGR8_USCALED_PACK8: return "FORMAT_BGR8_USCALED_PACK8";
		case gli::FORMAT_BGR8_SSCALED_PACK8: return "FORMAT_BGR8_SSCALED_PACK8";
		case gli::FORMAT_BGR8_UINT_PACK8: return "FORMAT_BGR8_UINT_PACK8";
		case gli::FORMAT_BGR8_SINT_PACK8: return "FORMAT_BGR8_SINT_PACK8";
		case gli::FORMAT_BGR8_SRGB_PACK8: return "FORMAT_BGR8_SRGB_PACK8";

		case gli::FORMAT_RGBA8_UNORM_PACK8: return "FORMAT_RGBA8_UNORM_PACK8";
		case gli::FORMAT_RGBA8_SNORM_PACK8: return "FORMAT_RGBA8_SNORM_PACK8";
		case gli::FORMAT_RGBA8_USCALED_PACK8: return "FORMAT_RGBA8_USCALED_PACK8";
		case gli::FORMAT_RGBA8_SSCALED_PACK8: return "FORMAT_RGBA8_SSCALED_PACK8";
		case gli::FORMAT_RGBA8_UINT_PACK8: return "FORMAT_RGBA8_UINT_PACK8";
		case gli::FORMAT_RGBA8_SINT_PACK8: return "FORMAT_RGBA8_SINT_PACK8";
		case gli::FORMAT_RGBA8_SRGB_PACK8: return "FORMAT_RGBA8_SRGB_PACK8";

		case gli::FORMAT_BGRA8_UNORM_PACK8: return "FORMAT_BGRA8_UNORM_PACK8";
		case gli::FORMAT_BGRA8_SNORM_PACK8: return "FORMAT_BGRA8_SNORM_PACK8";
		case gli::FORMAT_BGRA8_USCALED_PACK8: return "FORMAT_BGRA8_USCALED_PACK8";
		case gli::FORMAT_BGRA8_SSCALED_PACK8: return "FORMAT_BGRA8_SSCALED_PACK8";
		case gli::FORMAT_BGRA8_UINT_PACK8: return "FORMAT_BGRA8_UINT_PACK8";
		case gli::FORMAT_BGRA8_SINT_PACK8: return "FORMAT_BGRA8_SINT_PACK8";
		case gli::FORMAT_BGRA8_SRGB_PACK8: return "FORMAT_BGRA8_SRGB_PACK8";

		case gli::FORMAT_RGBA8_UNORM_PACK32: return "FORMAT_RGBA8_UNORM_PACK32";
		case gli::FORMAT_RGBA8_SNORM_PACK32: return "FORMAT_RGBA8_SNORM_PACK32";
		case gli::FORMAT_RGBA8_USCALED_PACK32: return "FORMAT_RGBA8_USCALED_PACK32";
		case gli::FORMAT_RGBA8_SSCALED_PACK32: return "FORMAT_RGBA8_SSCALED_PACK32";
		case gli::FORMAT_RGBA8_UINT_PACK32: return "FORMAT_RGBA8_UINT_PACK32";
		case gli::FORMAT_RGBA8_SINT_PACK32: return "FORMAT_RGBA8_SINT_PACK32";
		case gli::FORMAT_RGBA8_SRGB_PACK32: return "FORMAT_RGBA8_SRGB_PACK32";

		case gli::FORMAT_RGB10A2_UNORM_PACK32: return "FORMAT_RGB10A2_UNORM_PACK32";
		case gli::FORMAT_RGB10A2_SNORM_PACK32: return "FORMAT_RGB10A2_SNORM_PACK32";
		case gli::FORMAT_RGB10A2_USCALED_PACK32: return "FORMAT_RGB10A2_USCALED_PACK32";
		case gli::FORMAT_RGB10A2_SSCALED_PACK32: return "FORMAT_RGB10A2_SSCALED_PACK32";
		case gli::FORMAT_RGB10A2_UINT_PACK32: return "FORMAT_RGB10A2_UINT_PACK32";
		case gli::FORMAT_RGB10A2_SINT_PACK32: return "FORMAT_RGB10A2_SINT_PACK32";

		case gli::FORMAT_BGR10A2_UNORM_PACK32: return "FORMAT_BGR10A2_UNORM_PACK32";
		case gli::FORMAT_BGR10A2_SNORM_PACK32: return "FORMAT_BGR10A2_SNORM_PACK32";
		case gli::FORMAT_BGR10A2_USCALED_PACK32: return "FORMAT_BGR10A2_USCALED_PACK32";
		case gli::FORMAT_BGR10A2_SSCALED_PACK32: return "FORMAT_BGR10A2_SSCALED_PACK32";
		case gli::FORMAT_BGR10A2_UINT_PACK32: return "FORMAT_BGR10A2_UINT_PACK32";
		case gli::FORMAT_BGR10A2_SINT_PACK32: return "FORMAT_BGR10A2_SINT_PACK32";

		case gli::FORMAT_R16_UNORM_PACK16: return "FORMAT_R16_UNORM_PACK16";
		case gli::FORMAT_R16_SNORM_PACK16: return "FORMAT_R16_SNORM_PACK16";
		case gli::FORMAT_R16_USCALED_PACK16: return "FORMAT_R16_USCALED_PACK16";
		case gli::FORMAT_R16_SSCALED_PACK16: return "FORMAT_R16_SSCALED_PACK16";
		case gli::FORMAT_R16_UINT_PACK16: return "FORMAT_R16_UINT_PACK16";
		case gli::FORMAT_R16_SINT_PACK16: return "FORMAT_R16_SINT_PACK16";
		case gli::FORMAT_R16_SFLOAT_PACK16: return "FORMAT_R16_SFLOAT_PACK16";

		case gli::FORMAT_RG16_UNORM_PACK16: return "FORMAT_RG16_UNORM_PACK16";
		case gli::FORMAT_RG16_SNORM_PACK16: return "FORMAT_RG16_SNORM_PACK16";
		case gli::FORMAT_RG16_USCALED_PACK16: return "FORMAT_RG16_USCALED_PACK16";
		case gli::FORMAT_RG16_SSCALED_PACK16: return "FORMAT_RG16_SSCALED_PACK16";
		case gli::FORMAT_RG16_UINT_PACK16: return "FORMAT_RG16_UINT_PACK16";
		case gli::FORMAT_RG16_SINT_PACK16: return "FORMAT_RG16_SINT_PACK16";
		case gli::FORMAT_RG16_SFLOAT_PACK16: return "FORMAT_RG16_SFLOAT_PACK16";

		case gli::FORMAT_RGB16_UNORM_PACK16: return "FORMAT_RGB16_UNORM_PACK16";
		case gli::FORMAT_RGB16_SNORM_PACK16: return "FORMAT_RGB16_SNORM_PACK16";
		case gli::FORMAT_RGB16_USCALED_PACK16: return "FORMAT_RGB16_USCALED_PACK16";
		case gli::FORMAT_RGB16_SSCALED_PACK16: return "FORMAT_RGB16_SSCALED_PACK16";
		case gli::FORMAT_RGB16_UINT_PACK16: return "FORMAT_RGB16_UINT_PACK16";
		case gli::FORMAT_RGB16_SINT_PACK16: return "FORMAT_RGB16_SINT_PACK16";
		case gli::FORMAT_RGB16_SFLOAT_PACK16: return "FORMAT_RGB16_SFLOAT_PACK16";

		case gli::FORMAT_RGBA16_UNORM_PACK16: return "FORMAT_RGBA16_UNORM_PACK16";
		case gli::FORMAT_RGBA16_SNORM_PACK16: return "FORMAT_RGBA16_SNORM_PACK16";
		case gli::FORMAT_RGBA16_USCALED_PACK16: return "FORMAT_RGBA16_USCALED_PACK16";
		case gli::FORMAT_RGBA16_SSCALED_PACK16: return "FORMAT_RGBA16_SSCALED_PACK16";
		case gli::FORMAT_RGBA16_UINT_PACK16: return "FORMAT_RGBA16_UINT_PACK16";
		case gli::FORMAT_RGBA16_SINT_PACK16: return "FORMAT_RGBA16_SINT_PACK16";
		case gli::FORMAT_RGBA16_SFLOAT_PACK16: return "FORMAT_RGBA16_SFLOAT_PACK16";

		case gli::FORMAT_R32_UINT_PACK32: return "FORMAT_R32_UINT_PACK32";
		case gli::FORMAT_R32_SINT_PACK32: return "FORMAT_R32_SINT_PACK32";
		case gli::FORMAT_R32_SFLOAT_PACK32: return "FORMAT_R32_SFLOAT_PACK32";

		case gli::FORMAT_RG32_UINT_PACK32: return "FORMAT_RG32_UINT_PACK32";
		case gli::FORMAT_RG32_SINT_PACK32: return "FORMAT_RG32_SINT_PACK32";
		case gli::FORMAT_RG32_SFLOAT_PACK32: return "FORMAT_RG32_SFLOAT_PACK32";

		case gli::FORMAT_RGB32_UINT_PACK32: return "FORMAT_RGB32_UINT_PACK32";
		case gli::FORMAT_RGB32_SINT_PACK32: return "FORMAT_RGB32_SINT_PACK32";
		case gli::FORMAT_RGB32_SFLOAT_PACK32: return "FORMAT_RGB32_SFLOAT_PACK32";

		case gli::FORMAT_RGBA32_UINT_PACK32: return "FORMAT_RGBA32_UINT_PACK32";
		case gli::FORMAT_RGBA32_SINT_PACK32: return "FORMAT_RGBA32_SINT_PACK32";
		case gli::FORMAT_RGBA32_SFLOAT_PACK32: return "FORMAT_RGBA32_SFLOAT_PACK32";

		case gli::FORMAT_R64_UINT_PACK64: return "FORMAT_R64_UINT_PACK64";
		case gli::FORMAT_R64_SINT_PACK64: return "FORMAT_R64_SINT_PACK64";
		case gli::FORMAT_R64_SFLOAT_PACK64: return "FORMAT_R64_SFLOAT_PACK64";

		case gli::FORMAT_RG64_UINT_PACK64: return "FORMAT_RG64_UINT_PACK64";
		case gli::FORMAT_RG64_SINT_PACK64: return "FORMAT_RG64_SINT_PACK64";
		case gli::FORMAT_RG64_SFLOAT_PACK64: return "FORMAT_RG64_SFLOAT_PACK64";

		case gli::FORMAT_RGB64_UINT_PACK64: return "FORMAT_RGB64_UINT_PACK64";
		case gli::FORMAT_RGB64_SINT_PACK64: return "FORMAT_RGB64_SINT_PACK64";
		case gli::FORMAT_RGB64_SFLOAT_PACK64: return "FORMAT_RGB64_SFLOAT_PACK64";

		case gli::FORMAT_RGBA64_UINT_PACK64: return "FORMAT_RGBA64_UINT_PACK64";
		case gli::FORMAT_RGBA64_SINT_PACK64: return "FORMAT_RGBA64_SINT_PACK64";
		case gli::FORMAT_RGBA64_SFLOAT_PACK64: return "FORMAT_RGBA64_SFLOAT_PACK64";

		case gli::FORMAT_RG11B10_UFLOAT_PACK32: return "FORMAT_RG11B10_UFLOAT_PACK32";
		case gli::FORMAT_RGB9E5_UFLOAT_PACK32: return "FORMAT_RGB9E5_UFLOAT_PACK32";

		case gli::FORMAT_D16_UNORM_PACK16: return "FORMAT_D16_UNORM_PACK16";
		case gli::FORMAT_D24_UNORM_PACK32: return "FORMAT_D24_UNORM_PACK32";
		case gli::FORMAT_D32_SFLOAT_PACK32: return "FORMAT_D32_SFLOAT_PACK32";
		case gli::FORMAT_S8_UINT_PACK8: return "FORMAT_S8_UINT_PACK8";
		case gli::FORMAT_D16_UNORM_S8_UINT_PACK32: return "FORMAT_D16_UNORM_S8_UINT_PACK32";
		case gli::FORMAT_D24_UNORM_S8_UINT_PACK32: return "FORMAT_D24_UNORM_S8_UINT_PACK32";
		case gli::FORMAT_D32_SFLOAT_S8_UINT_PACK64: return "FORMAT_D32_SFLOAT_S8_UINT_PACK64";

		case gli::FORMAT_RGB_DXT1_UNORM_BLOCK8: return "FORMAT_RGB_DXT1_UNORM_BLOCK8";
		case gli::FORMAT_RGB_DXT1_SRGB_BLOCK8: return "FORMAT_RGB_DXT1_SRGB_BLOCK8";
		case gli::FORMAT_RGBA_DXT1_UNORM_BLOCK8: return "FORMAT_RGBA_DXT1_UNORM_BLOCK8";
		case gli::FORMAT_RGBA_DXT1_SRGB_BLOCK8: return "FORMAT_RGBA_DXT1_SRGB_BLOCK8";
		case gli::FORMAT_RGBA_DXT3_UNORM_BLOCK16: return "FORMAT_RGBA_DXT3_UNORM_BLOCK16";
		case gli::FORMAT_RGBA_DXT3_SRGB_BLOCK16: return "FORMAT_RGBA_DXT3_SRGB_BLOCK16";
		case gli::FORMAT_RGBA_DXT5_UNORM_BLOCK16: return "FORMAT_RGBA_DXT5_UNORM_BLOCK16";
		case gli::FORMAT_RGBA_DXT5_SRGB_BLOCK16: return "FORMAT_RGBA_DXT5_SRGB_BLOCK16";
		case gli::FORMAT_R_ATI1N_UNORM_BLOCK8: return "FORMAT_R_ATI1N_UNORM_BLOCK8";
		case gli::FORMAT_R_ATI1N_SNORM_BLOCK8: return "FORMAT_R_ATI1N_SNORM_BLOCK8";
		case gli::FORMAT_RG_ATI2N_UNORM_BLOCK16: return "FORMAT_RG_ATI2N_UNORM_BLOCK16";
		case gli::FORMAT_RG_ATI2N_SNORM_BLOCK16: return "FORMAT_RG_ATI2N_SNORM_BLOCK16";
		case gli::FORMAT_RGB_BP_UFLOAT_BLOCK16: return "FORMAT_RGB_BP_UFLOAT_BLOCK16";
		case gli::FORMAT_RGB_BP_SFLOAT_BLOCK16: return "FORMAT_RGB_BP_SFLOAT_BLOCK16";
		case gli::FORMAT_RGBA_BP_UNORM_BLOCK16: return "FORMAT_RGBA_BP_UNORM_BLOCK16";
		case gli::FORMAT_RGBA_BP_SRGB_BLOCK16: return "FORMAT_RGBA_BP_SRGB_BLOCK16";

		case gli::FORMAT_RGB_ETC2_UNORM_BLOCK8: return "FORMAT_RGB_ETC2_UNORM_BLOCK8";
		case gli::FORMAT_RGB_ETC2_SRGB_BLOCK8: return "FORMAT_RGB_ETC2_SRGB_BLOCK8";
		case gli::FORMAT_RGBA_ETC2_UNORM_BLOCK8: return "FORMAT_RGBA_ETC2_UNORM_BLOCK8";
		case gli::FORMAT_RGBA_ETC2_SRGB_BLOCK8: return "FORMAT_RGBA_ETC2_SRGB_BLOCK8";
		case gli::FORMAT_RGBA_ETC2_UNORM_BLOCK16: return "FORMAT_RGBA_ETC2_UNORM_BLOCK16";
		case gli::FORMAT_RGBA_ETC2_SRGB_BLOCK16: return "FORMAT_RGBA_ETC2_SRGB_BLOCK16";
		case gli::FORMAT_R_EAC_UNORM_BLOCK8: return "FORMAT_R_EAC_UNORM_BLOCK8";
		case gli::FORMAT_R_EAC_SNORM_BLOCK8: return "FORMAT_R_EAC_SNORM_BLOCK8";
		case gli::FORMAT_RG_EAC_UNORM_BLOCK16: return "FORMAT_RG_EAC_UNORM_BLOCK16";
		case gli::FORMAT_RG_EAC_SNORM_BLOCK16: return "FORMAT_RG_EAC_SNORM_BLOCK16";

		case gli::FORMAT_RGBA_ASTC_4X4_UNORM_BLOCK16: return "FORMAT_RGBA_ASTC_4X4_UNORM_BLOCK16";
		case gli::FORMAT_RGBA_ASTC_4X4_SRGB_BLOCK16: return "FORMAT_RGBA_ASTC_4X4_SRGB_BLOCK16";
		case gli::FORMAT_RGBA_ASTC_5X4_UNORM_BLOCK16: return "FORMAT_RGBA_ASTC_5X4_UNORM_BLOCK16";
		case gli::FORMAT_RGBA_ASTC_5X4_SRGB_BLOCK16: return "FORMAT_RGBA_ASTC_5X4_SRGB_BLOCK16";
		case gli::FORMAT_RGBA_ASTC_5X5_UNORM_BLOCK16: return "FORMAT_RGBA_ASTC_5X5_UNORM_BLOCK16";
		case gli::FORMAT_RGBA_ASTC_5X5_SRGB_BLOCK16: return "FORMAT_RGBA_ASTC_5X5_SRGB_BLOCK16";
		case gli::FORMAT_RGBA_ASTC_6X5_UNORM_BLOCK16: return "FORMAT_RGBA_ASTC_6X5_UNORM_BLOCK16";
		case gli::FORMAT_RGBA_ASTC_6X5_SRGB_BLOCK16: return "FORMAT_RGBA_ASTC_6X5_SRGB_BLOCK16";
		case gli::FORMAT_RGBA_ASTC_6X6_UNORM_BLOCK16: return "FORMAT_RGBA_ASTC_6X6_UNORM_BLOCK16";
		case gli::FORMAT_RGBA_ASTC_6X6_SRGB_BLOCK16: return "FORMAT_RGBA_ASTC_6X6_SRGB_BLOCK16";
		case gli::FORMAT_RGBA_ASTC_8X5_UNORM_BLOCK16: return "FORMAT_RGBA_ASTC_8X5_UNORM_BLOCK16";
		case gli::FORMAT_RGBA_ASTC_8X5_SRGB_BLOCK16: return "FORMAT_RGBA_ASTC_8X5_SRGB_BLOCK16";
		case gli::FORMAT_RGBA_ASTC_8X6_UNORM_BLOCK16: return "FORMAT_RGBA_ASTC_8X6_UNORM_BLOCK16";
		case gli::FORMAT_RGBA_ASTC_8X6_SRGB_BLOCK16: return "FORMAT_RGBA_ASTC_8X6_SRGB_BLOCK16";
		case gli::FORMAT_RGBA_ASTC_8X8_UNORM_BLOCK16: return "FORMAT_RGBA_ASTC_8X8_UNORM_BLOCK16";
		case gli::FORMAT_RGBA_ASTC_8X8_SRGB_BLOCK16: return "FORMAT_RGBA_ASTC_8X8_SRGB_BLOCK16";
		case gli::FORMAT_RGBA_ASTC_10X5_UNORM_BLOCK16: return "FORMAT_RGBA_ASTC_10X5_UNORM_BLOCK16";
		case gli::FORMAT_RGBA_ASTC_10X5_SRGB_BLOCK16: return "FORMAT_RGBA_ASTC_10X5_SRGB_BLOCK16";
		case gli::FORMAT_RGBA_ASTC_10X6_UNORM_BLOCK16: return "FORMAT_RGBA_ASTC_10X6_UNORM_BLOCK16";
		case gli::FORMAT_RGBA_ASTC_10X6_SRGB_BLOCK16: return "FORMAT_RGBA_ASTC_10X6_SRGB_BLOCK16";
		case gli::FORMAT_RGBA_ASTC_10X8_UNORM_BLOCK16: return "FORMAT_RGBA_ASTC_10X8_UNORM_BLOCK16";
		case gli::FORMAT_RGBA_ASTC_10X8_SRGB_BLOCK16: return "FORMAT_RGBA_ASTC_10X8_SRGB_BLOCK16";
		case gli::FORMAT_RGBA_ASTC_10X10_UNORM_BLOCK16: return "FORMAT_RGBA_ASTC_10X10_UNORM_BLOCK16";
		case gli::FORMAT_RGBA_ASTC_10X10_SRGB_BLOCK16: return "FORMAT_RGBA_ASTC_10X10_SRGB_BLOCK16";
		case gli::FORMAT_RGBA_ASTC_12X10_UNORM_BLOCK16: return "FORMAT_RGBA_ASTC_12X10_UNORM_BLOCK16";
		case gli::FORMAT_RGBA_ASTC_12X10_SRGB_BLOCK16: return "FORMAT_RGBA_ASTC_12X10_SRGB_BLOCK16";
		case gli::FORMAT_RGBA_ASTC_12X12_UNORM_BLOCK16: return "FORMAT_RGBA_ASTC_12X12_UNORM_BLOCK16";
		case gli::FORMAT_RGBA_ASTC_12X12_SRGB_BLOCK16: return "FORMAT_RGBA_ASTC_12X12_SRGB_BLOCK16";

		case gli::FORMAT_RGB_PVRTC1_8X8_UNORM_BLOCK32: return "FORMAT_RGB_PVRTC1_8X8_UNORM_BLOCK32";
		case gli::FORMAT_RGB_PVRTC1_8X8_SRGB_BLOCK32: return "FORMAT_RGB_PVRTC1_8X8_SRGB_BLOCK32";
		case gli::FORMAT_RGB_PVRTC1_16X8_UNORM_BLOCK32: return "FORMAT_RGB_PVRTC1_16X8_UNORM_BLOCK32";
		case gli::FORMAT_RGB_PVRTC1_16X8_SRGB_BLOCK32: return "FORMAT_RGB_PVRTC1_16X8_SRGB_BLOCK32";
		case gli::FORMAT_RGBA_PVRTC1_8X8_UNORM_BLOCK32: return "FORMAT_RGBA_PVRTC1_8X8_UNORM_BLOCK32";
		case gli::FORMAT_RGBA_PVRTC1_8X8_SRGB_BLOCK32: return "FORMAT_RGBA_PVRTC1_8X8_SRGB_BLOCK32";
		case gli::FORMAT_RGBA_PVRTC1_16X8_UNORM_BLOCK32: return "FORMAT_RGBA_PVRTC1_16X8_UNORM_BLOCK32";
		case gli::FORMAT_RGBA_PVRTC1_16X8_SRGB_BLOCK32: return "FORMAT_RGBA_PVRTC1_16X8_SRGB_BLOCK32";
		case gli::FORMAT_RGBA_PVRTC2_4X4_UNORM_BLOCK8: return "FORMAT_RGBA_PVRTC2_4X4_UNORM_BLOCK8";
		case gli::FORMAT_RGBA_PVRTC2_4X4_SRGB_BLOCK8: return "FORMAT_RGBA_PVRTC2_4X4_SRGB_BLOCK8";
		case gli::FORMAT_RGBA_PVRTC2_8X4_UNORM_BLOCK8: return "FORMAT_RGBA_PVRTC2_8X4_UNORM_BLOCK8";
		case gli::FORMAT_RGBA_PVRTC2_8X4_SRGB_BLOCK8: return "FORMAT_RGBA_PVRTC2_8X4_SRGB_BLOCK8";

		case gli::FORMAT_RGB_ETC_UNORM_BLOCK8: return "FORMAT_RGB_ETC_UNORM_BLOCK8";
		case gli::FORMAT_RGB_ATC_UNORM_BLOCK8: return "FORMAT_RGB_ATC_UNORM_BLOCK8";
		case gli::FORMAT_RGBA_ATCA_UNORM_BLOCK16: return "FORMAT_RGBA_ATCA_UNORM_BLOCK16";
		case gli::FORMAT_RGBA_ATCI_UNORM_BLOCK16: return "FORMAT_RGBA_ATCI_UNORM_BLOCK16";

		case gli::FORMAT_L8_UNORM_PACK8: return "FORMAT_L8_UNORM_PACK8";
		case gli::FORMAT_A8_UNORM_PACK8: return "FORMAT_A8_UNORM_PACK8";
		case gli::FORMAT_LA8_UNORM_PACK8: return "FORMAT_LA8_UNORM_PACK8";
		case gli::FORMAT_L16_UNORM_PACK16: return "FORMAT_L16_UNORM_PACK16";
		case gli::FORMAT_A16_UNORM_PACK16: return "FORMAT_A16_UNORM_PACK16";
		case gli::FORMAT_LA16_UNORM_PACK16: return "FORMAT_LA16_UNORM_PACK16";

		case gli::FORMAT_BGR8_UNORM_PACK32: return "FORMAT_BGR8_UNORM_PACK32";
		case gli::FORMAT_BGR8_SRGB_PACK32: return "FORMAT_BGR8_SRGB_PACK32";

		case gli::FORMAT_RG3B2_UNORM_PACK8: return "FORMAT_RG3B2_UNORM_PACK8";
		}
		return "FORMAT_UNKNOWN";
	}

} // namespace grfx

#pragma endregion
