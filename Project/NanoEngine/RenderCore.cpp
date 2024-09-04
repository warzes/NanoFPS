#include "stdafx.h"
#include "Core.h"
#include "RenderCore.h"

#pragma region Format

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
#define OFFSETS_R(R)             { R,  -1, -1, -1 }
#define OFFSETS_RG(R, G)         { R,   G, -1, -1 }
#define OFFSETS_RGB(R, G, B)     { R,   G,  B, -1 }
#define OFFSETS_RGBA(R, G, B, A) { R,   G,  B,  A }

// A static registry of format descriptions.
// The order must match the order of the Format enum, so that retrieving the description for a given format can be done in constant time.
constexpr FormatDesc formatDescs[] =
{
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

constexpr size_t formatDescsSize = sizeof(formatDescs) / sizeof(FormatDesc);
static_assert(formatDescsSize == FORMAT_COUNT, "Missing format descriptions");

const FormatDesc* GetFormatDescription(Format format)
{
	uint32_t formatIndex = static_cast<uint32_t>(format);
	ASSERT_MSG(format != FORMAT_UNDEFINED && formatIndex < formatDescsSize, "invalid format");
	return &formatDescs[formatIndex];
}

const char* ToString(Format format)
{
	uint32_t formatIndex = static_cast<uint32_t>(format);
	ASSERT_MSG(formatIndex < formatDescsSize, "invalid format");
	return formatDescs[formatIndex].name;
}

#pragma endregion

#pragma region Helper

ColorComponentFlags ColorComponentFlags::RGBA()
{
	ColorComponentFlags flags = ColorComponentFlags(COLOR_COMPONENT_R | COLOR_COMPONENT_G | COLOR_COMPONENT_B | COLOR_COMPONENT_A);
	return flags;
}

ImageUsageFlags ImageUsageFlags::SampledImage()
{
	ImageUsageFlags flags = ImageUsageFlags(IMAGE_USAGE_SAMPLED);
	return flags;
}

void VertexBinding::SetBinding(uint32_t binding)
{
	m_binding = binding;
	for (auto& elem : m_attributes) {
		elem.binding = binding;
	}
}

void VertexBinding::SetStride(uint32_t stride)
{
	m_stride = stride;
}

Result VertexBinding::GetAttribute(uint32_t index, const VertexAttribute** ppAttribute) const
{
	if (!IsIndexInRange(index, m_attributes)) {
		return ERROR_OUT_OF_RANGE;
	}
	*ppAttribute = &m_attributes[index];
	return SUCCESS;
}

uint32_t VertexBinding::GetAttributeIndex(VertexSemantic semantic) const
{
	auto it = FindIf(
		m_attributes,
		[semantic](const VertexAttribute& elem) -> bool {
			bool isMatch = (elem.semantic == semantic);
			return isMatch; });
	if (it == std::end(m_attributes)) {
		return VALUE_IGNORED;
	}
	uint32_t index = static_cast<uint32_t>(std::distance(std::begin(m_attributes), it));
	return index;
}

VertexBinding& VertexBinding::AppendAttribute(const VertexAttribute& attribute)
{
	m_attributes.push_back(attribute);

	if (m_inputRate == VertexBinding::kInvalidVertexInputRate)
		m_inputRate = attribute.inputRate;

	// Calculate offset for inserted attribute
	if (m_attributes.size() > 1)
	{
		size_t i1 = m_attributes.size() - 1;
		size_t i0 = i1 - 1;
		if (m_attributes[i1].offset == APPEND_OFFSET_ALIGNED)
		{
			uint32_t prevOffset = m_attributes[i0].offset;
			uint32_t prevSize = GetFormatDescription(m_attributes[i0].format)->bytesPerTexel;
			m_attributes[i1].offset = prevOffset + prevSize;
		}
	}
	// Size of mAttributes should be 1
	else
	{
		if (m_attributes[0].offset == APPEND_OFFSET_ALIGNED)
		{
			m_attributes[0].offset = 0;
		}
	}

	// Calculate stride
	m_stride = 0;
	for (auto& elem : m_attributes)
	{
		uint32_t size = GetFormatDescription(elem.format)->bytesPerTexel;
		m_stride += size;
	}

	return *this;
}

VertexBinding& VertexBinding::operator+=(const VertexAttribute& rhs)
{
	AppendAttribute(rhs);
	return *this;
}

Result VertexDescription::GetBinding(uint32_t index, const VertexBinding** ppBinding) const
{
	if (!IsIndexInRange(index, m_bindings))
		return ERROR_OUT_OF_RANGE;

	if (!IsNull(ppBinding))
		*ppBinding = &m_bindings[index];

	return SUCCESS;
}

const VertexBinding* VertexDescription::GetBinding(uint32_t index) const
{
	const VertexBinding* ptr = nullptr;
	GetBinding(index, &ptr);
	return ptr;
}

uint32_t VertexDescription::GetBindingIndex(uint32_t binding) const
{
	auto it = FindIf(
		m_bindings,
		[binding](const VertexBinding& elem) -> bool {
			bool isMatch = (elem.GetBinding() == binding);
			return isMatch; });
	if (it == std::end(m_bindings)) {
		return VALUE_IGNORED;
	}
	uint32_t index = static_cast<uint32_t>(std::distance(std::begin(m_bindings), it));
	return index;
}

Result VertexDescription::AppendBinding(const VertexBinding& binding)
{
	auto it = FindIf(
		m_bindings,
		[binding](const VertexBinding& elem) -> bool {
			bool isSame = (elem.GetBinding() == binding.GetBinding());
			return isSame; });
	if (it != std::end(m_bindings)) {
		return ERROR_DUPLICATE_ELEMENT;
	}
	m_bindings.push_back(binding);
	return SUCCESS;
}
#pragma endregion

#pragma region Utils

const char* ToString(DescriptorType value)
{
	switch (value) {
	default: break;
	case DESCRIPTOR_TYPE_SAMPLER: return "DESCRIPTOR_TYPE_SAMPLER"; break;
	case DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER: return "DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER"; break;
	case DESCRIPTOR_TYPE_SAMPLED_IMAGE: return "DESCRIPTOR_TYPE_SAMPLED_IMAGE"; break;
	case DESCRIPTOR_TYPE_STORAGE_IMAGE: return "DESCRIPTOR_TYPE_STORAGE_IMAGE"; break;
	case DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER: return "DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER  "; break;
	case DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER: return "DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER  "; break;
	case DESCRIPTOR_TYPE_UNIFORM_BUFFER: return "DESCRIPTOR_TYPE_UNIFORM_BUFFER"; break;
	case DESCRIPTOR_TYPE_RAW_STORAGE_BUFFER: return "DESCRIPTOR_TYPE_RAW_STORAGE_BUFFER"; break;
	case DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC: return "DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC"; break;
	case DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC: return "DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC"; break;
	case DESCRIPTOR_TYPE_INPUT_ATTACHMENT: return "DESCRIPTOR_TYPE_INPUT_ATTACHMENT"; break;
	}
	return "<unknown descriptor type>";
}

const char* ToString(VertexSemantic value)
{
	switch (value) {
	default: break;
	case VERTEX_SEMANTIC_POSITION: return "POSITION"; break;
	case VERTEX_SEMANTIC_NORMAL: return "NORMAL"; break;
	case VERTEX_SEMANTIC_COLOR: return "COLOR"; break;
	case VERTEX_SEMANTIC_TANGENT: return "TANGENT"; break;
	case VERTEX_SEMANTIC_BITANGENT: return "BITANGENT"; break;
	case VERTEX_SEMANTIC_TEXCOORD: return "TEXCOORD"; break;
	case VERTEX_SEMANTIC_TEXCOORD0: return "TEXCOORD0"; break;
	case VERTEX_SEMANTIC_TEXCOORD1: return "TEXCOORD1"; break;
	case VERTEX_SEMANTIC_TEXCOORD2: return "TEXCOORD2"; break;
	case VERTEX_SEMANTIC_TEXCOORD3: return "TEXCOORD3"; break;
	case VERTEX_SEMANTIC_TEXCOORD4: return "TEXCOORD4"; break;
	case VERTEX_SEMANTIC_TEXCOORD5: return "TEXCOORD5"; break;
	case VERTEX_SEMANTIC_TEXCOORD6: return "TEXCOORD6"; break;
	case VERTEX_SEMANTIC_TEXCOORD7: return "TEXCOORD7"; break;
	case VERTEX_SEMANTIC_TEXCOORD8: return "TEXCOORD8"; break;
	case VERTEX_SEMANTIC_TEXCOORD9: return "TEXCOORD9"; break;
	case VERTEX_SEMANTIC_TEXCOORD10: return "TEXCOORD10"; break;
	case VERTEX_SEMANTIC_TEXCOORD11: return "TEXCOORD11"; break;
	case VERTEX_SEMANTIC_TEXCOORD12: return "TEXCOORD12"; break;
	case VERTEX_SEMANTIC_TEXCOORD13: return "TEXCOORD13"; break;
	case VERTEX_SEMANTIC_TEXCOORD14: return "TEXCOORD14"; break;
	case VERTEX_SEMANTIC_TEXCOORD15: return "TEXCOORD15"; break;
	case VERTEX_SEMANTIC_TEXCOORD16: return "TEXCOORD16"; break;
	case VERTEX_SEMANTIC_TEXCOORD17: return "TEXCOORD17"; break;
	case VERTEX_SEMANTIC_TEXCOORD18: return "TEXCOORD18"; break;
	case VERTEX_SEMANTIC_TEXCOORD19: return "TEXCOORD19"; break;
	case VERTEX_SEMANTIC_TEXCOORD20: return "TEXCOORD20"; break;
	case VERTEX_SEMANTIC_TEXCOORD21: return "TEXCOORD21"; break;
	case VERTEX_SEMANTIC_TEXCOORD22: return "TEXCOORD22"; break;
	}
	return "";
}

const char* ToString(IndexType value)
{
	switch (value) {
	default: break;
	case INDEX_TYPE_UNDEFINED: return "INDEX_TYPE_UNDEFINED";
	case INDEX_TYPE_UINT16: return "INDEX_TYPE_UINT16";
	case INDEX_TYPE_UINT32: return "INDEX_TYPE_UINT32";
	}
	return "<unknown IndexType>";
}

uint32_t IndexTypeSize(IndexType value)
{
	switch (value) {
	default: break;
	case INDEX_TYPE_UINT16: return sizeof(uint16_t); break;
	case INDEX_TYPE_UINT32: return sizeof(uint32_t); break;
	}
	return 0;
}

Format VertexSemanticFormat(VertexSemantic value)
{
	switch (value) {
	default: break;
	case VERTEX_SEMANTIC_POSITION: return FORMAT_R32G32B32_FLOAT; break;
	case VERTEX_SEMANTIC_NORMAL: return FORMAT_R32G32B32_FLOAT; break;
	case VERTEX_SEMANTIC_COLOR: return FORMAT_R32G32B32_FLOAT; break;
	case VERTEX_SEMANTIC_TANGENT: return FORMAT_R32G32B32_FLOAT; break;
	case VERTEX_SEMANTIC_BITANGENT: return FORMAT_R32G32B32_FLOAT; break;
	case VERTEX_SEMANTIC_TEXCOORD: return FORMAT_R32G32_FLOAT; break;
	case VERTEX_SEMANTIC_TEXCOORD0: return FORMAT_R32G32_FLOAT; break;
	case VERTEX_SEMANTIC_TEXCOORD1: return FORMAT_R32G32_FLOAT; break;
	case VERTEX_SEMANTIC_TEXCOORD2: return FORMAT_R32G32_FLOAT; break;
	case VERTEX_SEMANTIC_TEXCOORD3: return FORMAT_R32G32_FLOAT; break;
	case VERTEX_SEMANTIC_TEXCOORD4: return FORMAT_R32G32_FLOAT; break;
	case VERTEX_SEMANTIC_TEXCOORD5: return FORMAT_R32G32_FLOAT; break;
	case VERTEX_SEMANTIC_TEXCOORD6: return FORMAT_R32G32_FLOAT; break;
	case VERTEX_SEMANTIC_TEXCOORD7: return FORMAT_R32G32_FLOAT; break;
	case VERTEX_SEMANTIC_TEXCOORD8: return FORMAT_R32G32_FLOAT; break;
	case VERTEX_SEMANTIC_TEXCOORD9: return FORMAT_R32G32_FLOAT; break;
	case VERTEX_SEMANTIC_TEXCOORD10: return FORMAT_R32G32_FLOAT; break;
	case VERTEX_SEMANTIC_TEXCOORD11: return FORMAT_R32G32_FLOAT; break;
	case VERTEX_SEMANTIC_TEXCOORD12: return FORMAT_R32G32_FLOAT; break;
	case VERTEX_SEMANTIC_TEXCOORD13: return FORMAT_R32G32_FLOAT; break;
	case VERTEX_SEMANTIC_TEXCOORD14: return FORMAT_R32G32_FLOAT; break;
	case VERTEX_SEMANTIC_TEXCOORD15: return FORMAT_R32G32_FLOAT; break;
	case VERTEX_SEMANTIC_TEXCOORD16: return FORMAT_R32G32_FLOAT; break;
	case VERTEX_SEMANTIC_TEXCOORD17: return FORMAT_R32G32_FLOAT; break;
	case VERTEX_SEMANTIC_TEXCOORD18: return FORMAT_R32G32_FLOAT; break;
	case VERTEX_SEMANTIC_TEXCOORD19: return FORMAT_R32G32_FLOAT; break;
	case VERTEX_SEMANTIC_TEXCOORD20: return FORMAT_R32G32_FLOAT; break;
	case VERTEX_SEMANTIC_TEXCOORD21: return FORMAT_R32G32_FLOAT; break;
	case VERTEX_SEMANTIC_TEXCOORD22: return FORMAT_R32G32_FLOAT; break;
	}
	return FORMAT_UNDEFINED;
}

const char* ToString(const gli::target& target)
{
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

#pragma endregion

#pragma region Vk Utils

const char* ToString(VkResult value)
{
	return string_VkResult(value);
}

const char* ToString(VkDescriptorType value)
{
	return string_VkDescriptorType(value);	
}

const char* ToString(VkPresentModeKHR value)
{
	return string_VkPresentModeKHR(value);
}

VkAttachmentLoadOp ToVkAttachmentLoadOp(AttachmentLoadOp value)
{
	switch (value) {
	default: break;
	case ATTACHMENT_LOAD_OP_LOAD: return VK_ATTACHMENT_LOAD_OP_LOAD; break;
	case ATTACHMENT_LOAD_OP_CLEAR: return VK_ATTACHMENT_LOAD_OP_CLEAR; break;
	case ATTACHMENT_LOAD_OP_DONT_CARE: return VK_ATTACHMENT_LOAD_OP_DONT_CARE; break;
	}
	return InvalidValue<VkAttachmentLoadOp>();
}

VkAttachmentStoreOp ToVkAttachmentStoreOp(AttachmentStoreOp value)
{
	switch (value) {
	default: break;
	case ATTACHMENT_STORE_OP_STORE: return VK_ATTACHMENT_STORE_OP_STORE; break;
	case ATTACHMENT_STORE_OP_DONT_CARE: return VK_ATTACHMENT_STORE_OP_DONT_CARE; break;
	}
	return InvalidValue<VkAttachmentStoreOp>();
}

VkBlendFactor ToVkBlendFactor(BlendFactor value)
{
	switch (value) {
	default: break;
	case BLEND_FACTOR_ZERO: return VK_BLEND_FACTOR_ZERO; break;
	case BLEND_FACTOR_ONE: return VK_BLEND_FACTOR_ONE; break;
	case BLEND_FACTOR_SRC_COLOR: return VK_BLEND_FACTOR_SRC_COLOR; break;
	case BLEND_FACTOR_ONE_MINUS_SRC_COLOR: return VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR; break;
	case BLEND_FACTOR_DST_COLOR: return VK_BLEND_FACTOR_DST_COLOR; break;
	case BLEND_FACTOR_ONE_MINUS_DST_COLOR: return VK_BLEND_FACTOR_ONE_MINUS_DST_COLOR; break;
	case BLEND_FACTOR_SRC_ALPHA: return VK_BLEND_FACTOR_SRC_ALPHA; break;
	case BLEND_FACTOR_ONE_MINUS_SRC_ALPHA: return VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA; break;
	case BLEND_FACTOR_DST_ALPHA: return VK_BLEND_FACTOR_DST_ALPHA; break;
	case BLEND_FACTOR_ONE_MINUS_DST_ALPHA: return VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA; break;
	case BLEND_FACTOR_CONSTANT_COLOR: return VK_BLEND_FACTOR_CONSTANT_COLOR; break;
	case BLEND_FACTOR_ONE_MINUS_CONSTANT_COLOR: return VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_COLOR; break;
	case BLEND_FACTOR_CONSTANT_ALPHA: return VK_BLEND_FACTOR_CONSTANT_ALPHA; break;
	case BLEND_FACTOR_ONE_MINUS_CONSTANT_ALPHA: return VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_ALPHA; break;
	case BLEND_FACTOR_SRC_ALPHA_SATURATE: return VK_BLEND_FACTOR_SRC_ALPHA_SATURATE; break;
	case BLEND_FACTOR_SRC1_COLOR: return VK_BLEND_FACTOR_SRC1_COLOR; break;
	case BLEND_FACTOR_ONE_MINUS_SRC1_COLOR: return VK_BLEND_FACTOR_ONE_MINUS_SRC1_COLOR; break;
	case BLEND_FACTOR_SRC1_ALPHA: return VK_BLEND_FACTOR_SRC1_ALPHA; break;
	case BLEND_FACTOR_ONE_MINUS_SRC1_ALPHA: return VK_BLEND_FACTOR_ONE_MINUS_SRC1_ALPHA; break;
	}
	return InvalidValue<VkBlendFactor>();
}

VkBlendOp ToVkBlendOp(BlendOp value)
{
	switch (value) {
	default: break;
	case BLEND_OP_ADD: return VK_BLEND_OP_ADD; break;
	case BLEND_OP_SUBTRACT: return VK_BLEND_OP_SUBTRACT; break;
	case BLEND_OP_REVERSE_SUBTRACT: return VK_BLEND_OP_REVERSE_SUBTRACT; break;
	case BLEND_OP_MIN: return VK_BLEND_OP_MIN; break;
	case BLEND_OP_MAX: return VK_BLEND_OP_MAX; break;

#if defined(VK_BLEND_OPERATION_ADVANCED)
	// Provdied by VK_blend_operation_advanced
	case BLEND_OP_ZERO: return  VK_BLEND_OP_ZERO_EXT; break;
	case BLEND_OP_SRC: return  VK_BLEND_OP_SRC_EXT; break;
	case BLEND_OP_DST: return  VK_BLEND_OP_DST_EXT; break;
	case BLEND_OP_SRC_OVER: return  VK_BLEND_OP_SRC_OVER_EXT; break;
	case BLEND_OP_DST_OVER: return  VK_BLEND_OP_DST_OVER_EXT; break;
	case BLEND_OP_SRC_IN: return  VK_BLEND_OP_SRC_IN_EXT; break;
	case BLEND_OP_DST_IN: return  VK_BLEND_OP_DST_IN_EXT; break;
	case BLEND_OP_SRC_OUT: return  VK_BLEND_OP_SRC_OUT_EXT; break;
	case BLEND_OP_DST_OUT: return  VK_BLEND_OP_DST_OUT_EXT; break;
	case BLEND_OP_SRC_ATOP: return  VK_BLEND_OP_SRC_ATOP_EXT; break;
	case BLEND_OP_DST_ATOP: return  VK_BLEND_OP_DST_ATOP_EXT; break;
	case BLEND_OP_XOR: return  VK_BLEND_OP_XOR_EXT; break;
	case BLEND_OP_MULTIPLY: return  VK_BLEND_OP_MULTIPLY_EXT; break;
	case BLEND_OP_SCREEN: return  VK_BLEND_OP_SCREEN_EXT; break;
	case BLEND_OP_OVERLAY: return  VK_BLEND_OP_OVERLAY_EXT; break;
	case BLEND_OP_DARKEN: return  VK_BLEND_OP_DARKEN_EXT; break;
	case BLEND_OP_LIGHTEN: return  VK_BLEND_OP_LIGHTEN_EXT; break;
	case BLEND_OP_COLORDODGE: return  VK_BLEND_OP_COLORDODGE_EXT; break;
	case BLEND_OP_COLORBURN: return  VK_BLEND_OP_COLORBURN_EXT; break;
	case BLEND_OP_HARDLIGHT: return  VK_BLEND_OP_HARDLIGHT_EXT; break;
	case BLEND_OP_SOFTLIGHT: return  VK_BLEND_OP_SOFTLIGHT_EXT; break;
	case BLEND_OP_DIFFERENCE: return  VK_BLEND_OP_DIFFERENCE_EXT; break;
	case BLEND_OP_EXCLUSION: return  VK_BLEND_OP_EXCLUSION_EXT; break;
	case BLEND_OP_INVERT: return  VK_BLEND_OP_INVERT_EXT; break;
	case BLEND_OP_INVERT_RGB: return  VK_BLEND_OP_INVERT_RGB_EXT; break;
	case BLEND_OP_LINEARDODGE: return  VK_BLEND_OP_LINEARDODGE_EXT; break;
	case BLEND_OP_LINEARBURN: return  VK_BLEND_OP_LINEARBURN_EXT; break;
	case BLEND_OP_VIVIDLIGHT: return  VK_BLEND_OP_VIVIDLIGHT_EXT; break;
	case BLEND_OP_LINEARLIGHT: return  VK_BLEND_OP_LINEARLIGHT_EXT; break;
	case BLEND_OP_PINLIGHT: return  VK_BLEND_OP_PINLIGHT_EXT; break;
	case BLEND_OP_HARDMIX: return  VK_BLEND_OP_HARDMIX_EXT; break;
	case BLEND_OP_HSL_HUE: return  VK_BLEND_OP_HSL_HUE_EXT; break;
	case BLEND_OP_HSL_SATURATION: return  VK_BLEND_OP_HSL_SATURATION_EXT; break;
	case BLEND_OP_HSL_COLOR: return  VK_BLEND_OP_HSL_COLOR_EXT; break;
	case BLEND_OP_HSL_LUMINOSITY: return  VK_BLEND_OP_HSL_LUMINOSITY_EXT; break;
	case BLEND_OP_PLUS: return  VK_BLEND_OP_PLUS_EXT; break;
	case BLEND_OP_PLUS_CLAMPED: return  VK_BLEND_OP_PLUS_CLAMPED_EXT; break;
	case BLEND_OP_PLUS_CLAMPED_ALPHA: return  VK_BLEND_OP_PLUS_CLAMPED_ALPHA_EXT; break;
	case BLEND_OP_PLUS_DARKER: return  VK_BLEND_OP_PLUS_DARKER_EXT; break;
	case BLEND_OP_MINUS: return  VK_BLEND_OP_MINUS_EXT; break;
	case BLEND_OP_MINUS_CLAMPED: return  VK_BLEND_OP_MINUS_CLAMPED_EXT; break;
	case BLEND_OP_CONTRAST: return  VK_BLEND_OP_CONTRAST_EXT; break;
	case BLEND_OP_INVERT_OVG: return  VK_BLEND_OP_INVERT_OVG_EXT; break;
	case BLEND_OP_RED: return  VK_BLEND_OP_RED_EXT; break;
	case BLEND_OP_GREEN: return  VK_BLEND_OP_GREEN_EXT; break;
	case BLEND_OP_BLUE: return  VK_BLEND_OP_BLUE_EXT; break;
#endif // defined(PPX_VK_BLEND_OPERATION_ADVANCED)
	}
	return InvalidValue<VkBlendOp>();
}

VkBorderColor ToVkBorderColor(BorderColor value)
{
	switch (value) {
	default: break;
	case BORDER_COLOR_FLOAT_TRANSPARENT_BLACK: return VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK; break;
	case BORDER_COLOR_INT_TRANSPARENT_BLACK: return VK_BORDER_COLOR_INT_TRANSPARENT_BLACK; break;
	case BORDER_COLOR_FLOAT_OPAQUE_BLACK: return VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK; break;
	case BORDER_COLOR_INT_OPAQUE_BLACK: return VK_BORDER_COLOR_INT_OPAQUE_BLACK; break;
	case BORDER_COLOR_FLOAT_OPAQUE_WHITE: return VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE; break;
	case BORDER_COLOR_INT_OPAQUE_WHITE: return VK_BORDER_COLOR_INT_OPAQUE_WHITE; break;
	}
	return InvalidValue<VkBorderColor>();
}

VkBufferUsageFlags ToVkBufferUsageFlags(const BufferUsageFlags& value)
{
	VkBufferUsageFlags flags = 0;
	if (value.bits.transferSrc) flags |= VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
	if (value.bits.transferDst) flags |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;
	if (value.bits.uniformTexelBuffer) flags |= VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT;
	if (value.bits.storageTexelBuffer) flags |= VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT;
	if (value.bits.uniformBuffer) flags |= VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
	if (value.bits.rawStorageBuffer) flags |= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
	if (value.bits.roStructuredBuffer) flags |= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
	if (value.bits.rwStructuredBuffer) flags |= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
	if (value.bits.indexBuffer) flags |= VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
	if (value.bits.vertexBuffer) flags |= VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
	if (value.bits.indirectBuffer) flags |= VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT;
	if (value.bits.conditionalRendering) flags |= VK_BUFFER_USAGE_CONDITIONAL_RENDERING_BIT_EXT;
	if (value.bits.transformFeedbackBuffer) flags |= VK_BUFFER_USAGE_TRANSFORM_FEEDBACK_BUFFER_BIT_EXT;
	if (value.bits.transformFeedbackCounterBuffer) flags |= VK_BUFFER_USAGE_TRANSFORM_FEEDBACK_COUNTER_BUFFER_BIT_EXT;
	if (value.bits.shaderDeviceAddress) flags |= VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT_KHR;
	return flags;
}

VkChromaLocation ToVkChromaLocation(ChromaLocation value)
{
	switch (value) {
	default: break;
	case CHROMA_LOCATION_COSITED_EVEN: return VK_CHROMA_LOCATION_COSITED_EVEN;
	case CHROMA_LOCATION_MIDPOINT: return VK_CHROMA_LOCATION_MIDPOINT;
	}
	return InvalidValue<VkChromaLocation>();
}

VkClearColorValue ToVkClearColorValue(const RenderTargetClearValue& value)
{
	VkClearColorValue res = {};
	res.float32[0] = value.rgba[0];
	res.float32[1] = value.rgba[1];
	res.float32[2] = value.rgba[2];
	res.float32[3] = value.rgba[3];
	return res;
}

VkClearDepthStencilValue ToVkClearDepthStencilValue(const DepthStencilClearValue& value)
{
	VkClearDepthStencilValue res = {};
	res.depth = value.depth;
	res.stencil = value.stencil;
	return res;
}

VkColorComponentFlags ToVkColorComponentFlags(const ColorComponentFlags& value)
{
	VkColorComponentFlags flags = 0;
	if (value.bits.r) flags |= VK_COLOR_COMPONENT_R_BIT;
	if (value.bits.g) flags |= VK_COLOR_COMPONENT_G_BIT;
	if (value.bits.b) flags |= VK_COLOR_COMPONENT_B_BIT;
	if (value.bits.a) flags |= VK_COLOR_COMPONENT_A_BIT;
	return flags;
}

VkCompareOp ToVkCompareOp(CompareOp value)
{
	switch (value) {
	default: break;
	case COMPARE_OP_NEVER: return VK_COMPARE_OP_NEVER; break;
	case COMPARE_OP_LESS: return VK_COMPARE_OP_LESS; break;
	case COMPARE_OP_EQUAL: return VK_COMPARE_OP_EQUAL; break;
	case COMPARE_OP_LESS_OR_EQUAL: return VK_COMPARE_OP_LESS_OR_EQUAL; break;
	case COMPARE_OP_GREATER: return VK_COMPARE_OP_GREATER; break;
	case COMPARE_OP_NOT_EQUAL: return VK_COMPARE_OP_NOT_EQUAL; break;
	case COMPARE_OP_GREATER_OR_EQUAL: return VK_COMPARE_OP_GREATER_OR_EQUAL; break;
	case COMPARE_OP_ALWAYS: return VK_COMPARE_OP_ALWAYS; break;
	}
	return InvalidValue<VkCompareOp>();
}

VkComponentSwizzle ToVkComponentSwizzle(ComponentSwizzle value)
{
	switch (value) {
	default: break;
	case COMPONENT_SWIZZLE_IDENTITY: return VK_COMPONENT_SWIZZLE_IDENTITY; break;
	case COMPONENT_SWIZZLE_ZERO: return VK_COMPONENT_SWIZZLE_ZERO; break;
	case COMPONENT_SWIZZLE_ONE: return VK_COMPONENT_SWIZZLE_ONE; break;
	case COMPONENT_SWIZZLE_R: return VK_COMPONENT_SWIZZLE_R; break;
	case COMPONENT_SWIZZLE_G: return VK_COMPONENT_SWIZZLE_G; break;
	case COMPONENT_SWIZZLE_B: return VK_COMPONENT_SWIZZLE_B; break;
	case COMPONENT_SWIZZLE_A: return VK_COMPONENT_SWIZZLE_A; break;
	}
	return InvalidValue<VkComponentSwizzle>();
}

VkComponentMapping ToVkComponentMapping(const ComponentMapping& value)
{
	VkComponentMapping res = {};
	res.r = ToVkComponentSwizzle(value.r);
	res.g = ToVkComponentSwizzle(value.g);
	res.b = ToVkComponentSwizzle(value.b);
	res.a = ToVkComponentSwizzle(value.a);
	return res;
}

VkCullModeFlagBits ToVkCullMode(CullMode value)
{
	switch (value) {
	default: break;
	case CULL_MODE_NONE: return VK_CULL_MODE_NONE; break;
	case CULL_MODE_FRONT: return VK_CULL_MODE_FRONT_BIT; break;
	case CULL_MODE_BACK: return VK_CULL_MODE_BACK_BIT; break;
	}
	return InvalidValue<VkCullModeFlagBits>();
}

VkDescriptorBindingFlags ToVkDescriptorBindingFlags(const DescriptorBindingFlags& value)
{
	VkDescriptorBindingFlags flags = 0;
	if (value.bits.updatable) {
		flags |= VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT;
	}
	if (value.bits.partiallyBound) {
		flags |= VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT;
	}
	return flags;
}

VkDescriptorType ToVkDescriptorType(DescriptorType value)
{
	switch (value) {
	default: break;
	case DESCRIPTOR_TYPE_SAMPLER: return VK_DESCRIPTOR_TYPE_SAMPLER; break;
	case DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER: return VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER; break;
	case DESCRIPTOR_TYPE_SAMPLED_IMAGE: return VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE; break;
	case DESCRIPTOR_TYPE_STORAGE_IMAGE: return VK_DESCRIPTOR_TYPE_STORAGE_IMAGE; break;
	case DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER: return VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER; break;
	case DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER: return VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER; break;
	case DESCRIPTOR_TYPE_UNIFORM_BUFFER: return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER; break;
	case DESCRIPTOR_TYPE_RAW_STORAGE_BUFFER: return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER; break;
	case DESCRIPTOR_TYPE_RO_STRUCTURED_BUFFER: return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER; break;
	case DESCRIPTOR_TYPE_RW_STRUCTURED_BUFFER: return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER; break;
	case DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC: return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC; break;
	case DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC: return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC; break;
	case DESCRIPTOR_TYPE_INPUT_ATTACHMENT: return VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT; break;
	}
	return InvalidValue<VkDescriptorType>();
}

VkFilter ToVkFilter(Filter value)
{
	switch (value) {
	default: break;
	case FILTER_NEAREST: return VK_FILTER_NEAREST; break;
	case FILTER_LINEAR: return VK_FILTER_LINEAR; break;
	}
	return InvalidValue<VkFilter>();
}

VkFormat ToVkFormat(Format value)
{
	switch (value) {
	default: break;

	// 8-bit signed normalized
	case FORMAT_R8_SNORM: return VK_FORMAT_R8_SNORM; break;
	case FORMAT_R8G8_SNORM: return VK_FORMAT_R8G8_SNORM; break;
	case FORMAT_R8G8B8_SNORM: return VK_FORMAT_R8G8B8_SNORM; break;
	case FORMAT_R8G8B8A8_SNORM: return VK_FORMAT_R8G8B8A8_SNORM; break;
	case FORMAT_B8G8R8_SNORM: return VK_FORMAT_B8G8R8_SNORM; break;
	case FORMAT_B8G8R8A8_SNORM: return VK_FORMAT_B8G8R8A8_SNORM; break;

		// 8-bit unsigned normalized
	case FORMAT_R8_UNORM: return VK_FORMAT_R8_UNORM; break;
	case FORMAT_R8G8_UNORM: return VK_FORMAT_R8G8_UNORM; break;
	case FORMAT_R8G8B8_UNORM: return VK_FORMAT_R8G8B8_UNORM; break;
	case FORMAT_R8G8B8A8_UNORM: return VK_FORMAT_R8G8B8A8_UNORM; break;
	case FORMAT_B8G8R8_UNORM: return VK_FORMAT_B8G8R8_UNORM; break;
	case FORMAT_B8G8R8A8_UNORM: return VK_FORMAT_B8G8R8A8_UNORM; break;

		// 8-bit signed integer
	case FORMAT_R8_SINT: return VK_FORMAT_R8_SINT; break;
	case FORMAT_R8G8_SINT: return VK_FORMAT_R8G8_SINT; break;
	case FORMAT_R8G8B8_SINT: return VK_FORMAT_R8G8B8_SINT; break;
	case FORMAT_R8G8B8A8_SINT: return VK_FORMAT_R8G8B8A8_SINT; break;
	case FORMAT_B8G8R8_SINT: return VK_FORMAT_B8G8R8_SINT; break;
	case FORMAT_B8G8R8A8_SINT: return VK_FORMAT_B8G8R8A8_SINT; break;

		// 8-bit unsigned integer
	case FORMAT_R8_UINT: return VK_FORMAT_R8_UINT; break;
	case FORMAT_R8G8_UINT: return VK_FORMAT_R8G8_UINT; break;
	case FORMAT_R8G8B8_UINT: return VK_FORMAT_R8G8B8_UINT; break;
	case FORMAT_R8G8B8A8_UINT: return VK_FORMAT_R8G8B8A8_UINT; break;
	case FORMAT_B8G8R8_UINT: return VK_FORMAT_B8G8R8_UINT; break;
	case FORMAT_B8G8R8A8_UINT: return VK_FORMAT_B8G8R8A8_UINT; break;

		// 16-bit signed normalized
	case FORMAT_R16_SNORM: return VK_FORMAT_R16_SNORM; break;
	case FORMAT_R16G16_SNORM: return VK_FORMAT_R16G16_SNORM; break;
	case FORMAT_R16G16B16_SNORM: return VK_FORMAT_R16G16B16_SNORM; break;
	case FORMAT_R16G16B16A16_SNORM: return VK_FORMAT_R16G16B16A16_SNORM; break;

		// 16-bit unsigned normalized
	case FORMAT_R16_UNORM: return VK_FORMAT_R16_UNORM; break;
	case FORMAT_R16G16_UNORM: return VK_FORMAT_R16G16_UNORM; break;
	case FORMAT_R16G16B16_UNORM: return VK_FORMAT_R16G16B16_UNORM; break;
	case FORMAT_R16G16B16A16_UNORM: return VK_FORMAT_R16G16B16A16_UNORM; break;

		// 16-bit signed integer
	case FORMAT_R16_SINT: return VK_FORMAT_R16_SINT; break;
	case FORMAT_R16G16_SINT: return VK_FORMAT_R16G16_SINT; break;
	case FORMAT_R16G16B16_SINT: return VK_FORMAT_R16G16B16_SINT; break;
	case FORMAT_R16G16B16A16_SINT: return VK_FORMAT_R16G16B16A16_SINT; break;

		// 16-bit unsigned integer
	case FORMAT_R16_UINT: return VK_FORMAT_R16_UINT; break;
	case FORMAT_R16G16_UINT: return VK_FORMAT_R16G16_UINT; break;
	case FORMAT_R16G16B16_UINT: return VK_FORMAT_R16G16B16_UINT; break;
	case FORMAT_R16G16B16A16_UINT: return VK_FORMAT_R16G16B16A16_UINT; break;

		// 16-bit float
	case FORMAT_R16_FLOAT: return VK_FORMAT_R16_SFLOAT; break;
	case FORMAT_R16G16_FLOAT: return VK_FORMAT_R16G16_SFLOAT; break;
	case FORMAT_R16G16B16_FLOAT: return VK_FORMAT_R16G16B16_SFLOAT; break;
	case FORMAT_R16G16B16A16_FLOAT: return VK_FORMAT_R16G16B16A16_SFLOAT; break;

		// 32-bit signed integer
	case FORMAT_R32_SINT: return VK_FORMAT_R32_SINT; break;
	case FORMAT_R32G32_SINT: return VK_FORMAT_R32G32_SINT; break;
	case FORMAT_R32G32B32_SINT: return VK_FORMAT_R32G32B32_SINT; break;
	case FORMAT_R32G32B32A32_SINT: return VK_FORMAT_R32G32B32A32_SINT; break;

		// 32-bit unsigned integer
	case FORMAT_R32_UINT: return VK_FORMAT_R32_UINT; break;
	case FORMAT_R32G32_UINT: return VK_FORMAT_R32G32_UINT; break;
	case FORMAT_R32G32B32_UINT: return VK_FORMAT_R32G32B32_UINT; break;
	case FORMAT_R32G32B32A32_UINT: return VK_FORMAT_R32G32B32A32_UINT; break;

		// 32-bit float
	case FORMAT_R32_FLOAT: return VK_FORMAT_R32_SFLOAT; break;
	case FORMAT_R32G32_FLOAT: return VK_FORMAT_R32G32_SFLOAT; break;
	case FORMAT_R32G32B32_FLOAT: return VK_FORMAT_R32G32B32_SFLOAT; break;
	case FORMAT_R32G32B32A32_FLOAT: return VK_FORMAT_R32G32B32A32_SFLOAT; break;

		// 8-bit unsigned integer stencil
	case FORMAT_S8_UINT: return VK_FORMAT_S8_UINT; break;

		// 16-bit unsigned normalized depth
	case FORMAT_D16_UNORM: return VK_FORMAT_D16_UNORM; break;

		// 32-bit float depth
	case FORMAT_D32_FLOAT: return VK_FORMAT_D32_SFLOAT; break;

		// Depth/stencil combinations
	case FORMAT_D16_UNORM_S8_UINT: return VK_FORMAT_D16_UNORM_S8_UINT; break;
	case FORMAT_D24_UNORM_S8_UINT: return VK_FORMAT_D24_UNORM_S8_UINT; break;
	case FORMAT_D32_FLOAT_S8_UINT: return VK_FORMAT_D32_SFLOAT_S8_UINT; break;

		// SRGB
	case FORMAT_R8_SRGB: return VK_FORMAT_R8_SRGB; break;
	case FORMAT_R8G8_SRGB: return VK_FORMAT_R8G8_SRGB; break;
	case FORMAT_R8G8B8_SRGB: return VK_FORMAT_R8G8B8_SRGB; break;
	case FORMAT_R8G8B8A8_SRGB: return VK_FORMAT_R8G8B8A8_SRGB; break;
	case FORMAT_B8G8R8_SRGB: return VK_FORMAT_B8G8R8_SRGB; break;
	case FORMAT_B8G8R8A8_SRGB: return VK_FORMAT_B8G8R8A8_SRGB; break;

		// 10-bit
	case FORMAT_R10G10B10A2_UNORM: return VK_FORMAT_A2R10G10B10_UNORM_PACK32; break;

		// 11-bit R, 11-bit G, 10-bit B packed
	case FORMAT_R11G11B10_FLOAT: return VK_FORMAT_B10G11R11_UFLOAT_PACK32; break;

		// Compressed images
	case FORMAT_BC1_RGBA_SRGB: return VK_FORMAT_BC1_RGBA_SRGB_BLOCK; break;
	case FORMAT_BC1_RGBA_UNORM: return VK_FORMAT_BC1_RGBA_UNORM_BLOCK; break;
	case FORMAT_BC1_RGB_SRGB: return VK_FORMAT_BC1_RGB_SRGB_BLOCK; break;
	case FORMAT_BC1_RGB_UNORM: return VK_FORMAT_BC1_RGB_UNORM_BLOCK; break;
	case FORMAT_BC2_SRGB: return VK_FORMAT_BC2_SRGB_BLOCK; break;
	case FORMAT_BC2_UNORM: return VK_FORMAT_BC2_UNORM_BLOCK; break;
	case FORMAT_BC3_SRGB: return VK_FORMAT_BC3_SRGB_BLOCK; break;
	case FORMAT_BC3_UNORM: return VK_FORMAT_BC3_UNORM_BLOCK; break;
	case FORMAT_BC4_UNORM: return VK_FORMAT_BC4_UNORM_BLOCK; break;
	case FORMAT_BC4_SNORM: return VK_FORMAT_BC4_SNORM_BLOCK; break;
	case FORMAT_BC5_UNORM: return VK_FORMAT_BC5_UNORM_BLOCK; break;
	case FORMAT_BC5_SNORM: return VK_FORMAT_BC5_SNORM_BLOCK; break;
	case FORMAT_BC6H_UFLOAT: return VK_FORMAT_BC6H_UFLOAT_BLOCK; break;
	case FORMAT_BC6H_SFLOAT: return VK_FORMAT_BC6H_SFLOAT_BLOCK; break;
	case FORMAT_BC7_UNORM: return VK_FORMAT_BC7_UNORM_BLOCK; break;
	case FORMAT_BC7_SRGB: return VK_FORMAT_BC7_SRGB_BLOCK; break;
	}
	return VK_FORMAT_UNDEFINED;
}

VkFrontFace ToVkFrontFace(FrontFace value)
{
	switch (value) {
	default: break;
	case FRONT_FACE_CCW: return VK_FRONT_FACE_COUNTER_CLOCKWISE; break;
	case FRONT_FACE_CW: return VK_FRONT_FACE_CLOCKWISE; break;
	}
	return InvalidValue<VkFrontFace>();
}

VkImageType ToVkImageType(ImageType value)
{
	switch (value) {
	default: break;
	case IMAGE_TYPE_1D: return VK_IMAGE_TYPE_1D; break;
	case IMAGE_TYPE_2D: return VK_IMAGE_TYPE_2D; break;
	case IMAGE_TYPE_3D: return VK_IMAGE_TYPE_3D; break;
	case IMAGE_TYPE_CUBE: return VK_IMAGE_TYPE_2D; break;
	}
	return InvalidValue<VkImageType>();
}

VkImageUsageFlags ToVkImageUsageFlags(const ImageUsageFlags& value)
{
	VkImageUsageFlags flags = 0;
	if (value.bits.transferSrc) flags |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
	if (value.bits.transferDst) flags |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
	if (value.bits.sampled) flags |= VK_IMAGE_USAGE_SAMPLED_BIT;
	if (value.bits.storage) flags |= VK_IMAGE_USAGE_STORAGE_BIT;
	if (value.bits.colorAttachment) flags |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	if (value.bits.depthStencilAttachment) flags |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
	if (value.bits.transientAttachment) flags |= VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT;
	if (value.bits.inputAttachment) flags |= VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT;
	if (value.bits.fragmentDensityMap) flags |= VK_IMAGE_USAGE_FRAGMENT_DENSITY_MAP_BIT_EXT;
	if (value.bits.fragmentShadingRateAttachment) flags |= VK_IMAGE_USAGE_FRAGMENT_SHADING_RATE_ATTACHMENT_BIT_KHR;
	return flags;
}

VkImageViewType ToVkImageViewType(ImageViewType value)
{
	switch (value) {
	default: break;
	case IMAGE_VIEW_TYPE_1D: return VK_IMAGE_VIEW_TYPE_1D; break;
	case IMAGE_VIEW_TYPE_2D: return VK_IMAGE_VIEW_TYPE_2D; break;
	case IMAGE_VIEW_TYPE_3D: return VK_IMAGE_VIEW_TYPE_3D; break;
	case IMAGE_VIEW_TYPE_CUBE: return VK_IMAGE_VIEW_TYPE_CUBE; break;
	case IMAGE_VIEW_TYPE_1D_ARRAY: return VK_IMAGE_VIEW_TYPE_1D_ARRAY; break;
	case IMAGE_VIEW_TYPE_2D_ARRAY: return VK_IMAGE_VIEW_TYPE_2D_ARRAY; break;
	case IMAGE_VIEW_TYPE_CUBE_ARRAY: return VK_IMAGE_VIEW_TYPE_CUBE_ARRAY; break;
	}
	return InvalidValue<VkImageViewType>();
}

VkIndexType ToVkIndexType(IndexType value)
{
	switch (value) {
	default: break;
	case INDEX_TYPE_UINT16: return VK_INDEX_TYPE_UINT16; break;
	case INDEX_TYPE_UINT32: return VK_INDEX_TYPE_UINT32; break;
	}
	return InvalidValue<VkIndexType>();
}

VkLogicOp ToVkLogicOp(LogicOp value)
{
	switch (value) {
	default: break;
	case LOGIC_OP_CLEAR: return VK_LOGIC_OP_CLEAR; break;
	case LOGIC_OP_AND: return VK_LOGIC_OP_AND; break;
	case LOGIC_OP_AND_REVERSE: return VK_LOGIC_OP_AND_REVERSE; break;
	case LOGIC_OP_COPY: return VK_LOGIC_OP_COPY; break;
	case LOGIC_OP_AND_INVERTED: return VK_LOGIC_OP_AND_INVERTED; break;
	case LOGIC_OP_NO_OP: return VK_LOGIC_OP_NO_OP; break;
	case LOGIC_OP_XOR: return VK_LOGIC_OP_XOR; break;
	case LOGIC_OP_OR: return VK_LOGIC_OP_OR; break;
	case LOGIC_OP_NOR: return VK_LOGIC_OP_NOR; break;
	case LOGIC_OP_EQUIVALENT: return VK_LOGIC_OP_EQUIVALENT; break;
	case LOGIC_OP_INVERT: return VK_LOGIC_OP_INVERT; break;
	case LOGIC_OP_OR_REVERSE: return VK_LOGIC_OP_OR_REVERSE; break;
	case LOGIC_OP_COPY_INVERTED: return VK_LOGIC_OP_COPY_INVERTED; break;
	case LOGIC_OP_OR_INVERTED: return VK_LOGIC_OP_OR_INVERTED; break;
	case LOGIC_OP_NAND: return VK_LOGIC_OP_NAND; break;
	case LOGIC_OP_SET: return VK_LOGIC_OP_SET; break;
	}
	return InvalidValue<VkLogicOp>();
}

VkPipelineStageFlagBits ToVkPipelineStage(PipelineStage value)
{
	switch (value) {
	default: break;
	case PIPELINE_STAGE_TOP_OF_PIPE_BIT: return VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT; break;
	case PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT: return VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT; break;
	}
	return InvalidValue<VkPipelineStageFlagBits>();
}

VkPolygonMode ToVkPolygonMode(PolygonMode value)
{
	switch (value) {
	default: break;
	case POLYGON_MODE_FILL: return VK_POLYGON_MODE_FILL; break;
	case POLYGON_MODE_LINE: return VK_POLYGON_MODE_LINE; break;
	case POLYGON_MODE_POINT: return VK_POLYGON_MODE_POINT; break;
	}
	return InvalidValue<VkPolygonMode>();
}

VkPresentModeKHR ToVkPresentMode(PresentMode value)
{
	switch (value) {
	default: break;
	case PRESENT_MODE_FIFO: return VK_PRESENT_MODE_FIFO_KHR; break;
	case PRESENT_MODE_MAILBOX: return VK_PRESENT_MODE_MAILBOX_KHR; break;
	case PRESENT_MODE_IMMEDIATE: return VK_PRESENT_MODE_IMMEDIATE_KHR; break;
	}
	return InvalidValue<VkPresentModeKHR>();
}

VkPrimitiveTopology ToVkPrimitiveTopology(PrimitiveTopology value)
{
	switch (value) {
	default: break;
	case PRIMITIVE_TOPOLOGY_TRIANGLE_LIST: return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST; break;
	case PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP: return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP; break;
	case PRIMITIVE_TOPOLOGY_TRIANGLE_FAN: return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN; break;
	case PRIMITIVE_TOPOLOGY_POINT_LIST: return VK_PRIMITIVE_TOPOLOGY_POINT_LIST; break;
	case PRIMITIVE_TOPOLOGY_LINE_LIST: return VK_PRIMITIVE_TOPOLOGY_LINE_LIST; break;
	case PRIMITIVE_TOPOLOGY_LINE_STRIP: return VK_PRIMITIVE_TOPOLOGY_LINE_STRIP; break;
	}
	return InvalidValue<VkPrimitiveTopology>();
}

VkQueryType ToVkQueryType(QueryType value)
{
	switch (value) {
	default: break;
	case QUERY_TYPE_OCCLUSION: return VK_QUERY_TYPE_OCCLUSION; break;
	case QUERY_TYPE_TIMESTAMP: return VK_QUERY_TYPE_TIMESTAMP; break;
	case QUERY_TYPE_PIPELINE_STATISTICS: return VK_QUERY_TYPE_PIPELINE_STATISTICS; break;
	}
	return InvalidValue<VkQueryType>();
}

VkSamplerAddressMode ToVkSamplerAddressMode(SamplerAddressMode value)
{
	switch (value) {
	default: break;
	case SAMPLER_ADDRESS_MODE_REPEAT: return VK_SAMPLER_ADDRESS_MODE_REPEAT; break;
	case SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT: return VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT; break;
	case SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE: return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE; break;
	case SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER: return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER; break;
	}
	return InvalidValue<VkSamplerAddressMode>();
}

VkSamplerMipmapMode ToVkSamplerMipmapMode(SamplerMipmapMode value)
{
	switch (value) {
	default: break;
	case SAMPLER_MIPMAP_MODE_NEAREST: return VK_SAMPLER_MIPMAP_MODE_NEAREST; break;
	case SAMPLER_MIPMAP_MODE_LINEAR: return VK_SAMPLER_MIPMAP_MODE_LINEAR; break;
	}
	return InvalidValue<VkSamplerMipmapMode>();
}

VkSampleCountFlagBits ToVkSampleCount(SampleCount value)
{
	switch (value) {
	default: break;
	case SAMPLE_COUNT_1: return VK_SAMPLE_COUNT_1_BIT; break;
	case SAMPLE_COUNT_2: return VK_SAMPLE_COUNT_2_BIT; break;
	case SAMPLE_COUNT_4: return VK_SAMPLE_COUNT_4_BIT; break;
	case SAMPLE_COUNT_8: return VK_SAMPLE_COUNT_8_BIT; break;
	case SAMPLE_COUNT_16: return VK_SAMPLE_COUNT_16_BIT; break;
	case SAMPLE_COUNT_32: return VK_SAMPLE_COUNT_32_BIT; break;
	case SAMPLE_COUNT_64: return VK_SAMPLE_COUNT_64_BIT; break;
	}
	return InvalidValue<VkSampleCountFlagBits>();
}

VkShaderStageFlags ToVkShaderStageFlags(const ShaderStageFlags& value)
{
	VkShaderStageFlags flags = 0;
	if (value.bits.VS) flags |= VK_SHADER_STAGE_VERTEX_BIT;
	if (value.bits.HS) flags |= VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
	if (value.bits.DS) flags |= VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
	if (value.bits.GS) flags |= VK_SHADER_STAGE_GEOMETRY_BIT;
	if (value.bits.PS) flags |= VK_SHADER_STAGE_FRAGMENT_BIT;
	if (value.bits.CS) flags |= VK_SHADER_STAGE_COMPUTE_BIT;
	return flags;
}

VkStencilOp ToVkStencilOp(StencilOp value)
{
	switch (value) {
	default: break;
	case STENCIL_OP_KEEP: return VK_STENCIL_OP_KEEP; break;
	case STENCIL_OP_ZERO: return VK_STENCIL_OP_ZERO; break;
	case STENCIL_OP_REPLACE: return VK_STENCIL_OP_REPLACE; break;
	case STENCIL_OP_INCREMENT_AND_CLAMP: return VK_STENCIL_OP_INCREMENT_AND_CLAMP; break;
	case STENCIL_OP_DECREMENT_AND_CLAMP: return VK_STENCIL_OP_DECREMENT_AND_CLAMP; break;
	case STENCIL_OP_INVERT: return VK_STENCIL_OP_INVERT; break;
	case STENCIL_OP_INCREMENT_AND_WRAP: return VK_STENCIL_OP_INCREMENT_AND_WRAP; break;
	case STENCIL_OP_DECREMENT_AND_WRAP: return VK_STENCIL_OP_DECREMENT_AND_WRAP; break;
	}
	return InvalidValue<VkStencilOp>();
}

VkTessellationDomainOrigin ToVkTessellationDomainOrigin(TessellationDomainOrigin value)
{
	switch (value) {
	default: break;
	case TESSELLATION_DOMAIN_ORIGIN_UPPER_LEFT: return VK_TESSELLATION_DOMAIN_ORIGIN_UPPER_LEFT; break;
	case TESSELLATION_DOMAIN_ORIGIN_LOWER_LEFT: return VK_TESSELLATION_DOMAIN_ORIGIN_LOWER_LEFT; break;
	}
	return InvalidValue<VkTessellationDomainOrigin>();
}

VkVertexInputRate ToVkVertexInputRate(VertexInputRate value)
{
	switch (value) {
	default: break;
	case VERTEX_INPUT_RATE_VERTEX: return VK_VERTEX_INPUT_RATE_VERTEX; break;
	case VERETX_INPUT_RATE_INSTANCE: return VK_VERTEX_INPUT_RATE_INSTANCE; break;
	}
	return InvalidValue<VkVertexInputRate>();
}

static Result ToVkBarrier(
	ResourceState                   state,
	CommandType               commandType,
	const VkPhysicalDeviceFeatures& features,
	bool                            isSource,
	VkPipelineStageFlags& stageMask,
	VkAccessFlags& accessMask,
	VkImageLayout& layout)
{
	VkPipelineStageFlags PIPELINE_STAGE_ALL_SHADER_STAGES = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
	if (commandType == CommandType::COMMAND_TYPE_GRAPHICS) {
		PIPELINE_STAGE_ALL_SHADER_STAGES |= VK_PIPELINE_STAGE_VERTEX_SHADER_BIT |
			VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	}

	VkPipelineStageFlags PIPELINE_STAGE_NON_PIXEL_SHADER_STAGES = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
	if (commandType == CommandType::COMMAND_TYPE_GRAPHICS) {
		PIPELINE_STAGE_NON_PIXEL_SHADER_STAGES |= VK_PIPELINE_STAGE_VERTEX_SHADER_BIT;
	}

	if (commandType == CommandType::COMMAND_TYPE_GRAPHICS && features.geometryShader) {
		PIPELINE_STAGE_ALL_SHADER_STAGES |= VK_PIPELINE_STAGE_GEOMETRY_SHADER_BIT;
		PIPELINE_STAGE_NON_PIXEL_SHADER_STAGES |= VK_PIPELINE_STAGE_GEOMETRY_SHADER_BIT;
	}
	if (commandType == CommandType::COMMAND_TYPE_GRAPHICS && features.tessellationShader) {
		PIPELINE_STAGE_ALL_SHADER_STAGES |=
			VK_PIPELINE_STAGE_TESSELLATION_CONTROL_SHADER_BIT |
			VK_PIPELINE_STAGE_TESSELLATION_EVALUATION_SHADER_BIT;
		PIPELINE_STAGE_NON_PIXEL_SHADER_STAGES |=
			VK_PIPELINE_STAGE_TESSELLATION_CONTROL_SHADER_BIT |
			VK_PIPELINE_STAGE_TESSELLATION_EVALUATION_SHADER_BIT;
	}

	switch (state) {
	default: return ERROR_FAILED; break;

	case RESOURCE_STATE_UNDEFINED: {
		stageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
		accessMask = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT;
		layout = VK_IMAGE_LAYOUT_UNDEFINED;
	} break;

	case RESOURCE_STATE_GENERAL: {
		stageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
		accessMask = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT;
		layout = VK_IMAGE_LAYOUT_GENERAL;
	} break;

	case RESOURCE_STATE_CONSTANT_BUFFER:
	case RESOURCE_STATE_VERTEX_BUFFER: {
		stageMask = VK_PIPELINE_STAGE_VERTEX_INPUT_BIT | PIPELINE_STAGE_ALL_SHADER_STAGES;
		accessMask = VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT | VK_ACCESS_UNIFORM_READ_BIT;
		layout = InvalidValue<VkImageLayout>();
	} break;

	case RESOURCE_STATE_INDEX_BUFFER: {
		stageMask = VK_PIPELINE_STAGE_VERTEX_INPUT_BIT;
		accessMask = VK_ACCESS_INDEX_READ_BIT;
		layout = InvalidValue<VkImageLayout>();
	} break;

	case RESOURCE_STATE_RENDER_TARGET: {
		stageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		accessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	} break;

	case RESOURCE_STATE_UNORDERED_ACCESS: {
		stageMask = PIPELINE_STAGE_ALL_SHADER_STAGES;
		accessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
		layout = VK_IMAGE_LAYOUT_GENERAL;
	} break;

	case RESOURCE_STATE_DEPTH_STENCIL_READ: {
		stageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
		accessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
	} break;

	case RESOURCE_STATE_DEPTH_STENCIL_WRITE: {
		stageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
		accessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	} break;

	case RESOURCE_STATE_DEPTH_WRITE_STENCIL_READ: {
		stageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
		accessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		layout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL;
	} break;

	case RESOURCE_STATE_DEPTH_READ_STENCIL_WRITE: {
		stageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
		accessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		layout = VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_STENCIL_ATTACHMENT_OPTIMAL;
	} break;

	case RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE: {
		stageMask = PIPELINE_STAGE_NON_PIXEL_SHADER_STAGES;
		accessMask = VK_ACCESS_SHADER_READ_BIT;
		layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	} break;

	case RESOURCE_STATE_SHADER_RESOURCE: {
		stageMask = PIPELINE_STAGE_NON_PIXEL_SHADER_STAGES | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		accessMask = VK_ACCESS_SHADER_READ_BIT;
		layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	} break;

	case RESOURCE_STATE_PIXEL_SHADER_RESOURCE: {
		stageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		accessMask = VK_ACCESS_SHADER_READ_BIT;
		layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	} break;

	case RESOURCE_STATE_STREAM_OUT: {
		stageMask = VK_PIPELINE_STAGE_TRANSFORM_FEEDBACK_BIT_EXT;
		accessMask = VK_ACCESS_TRANSFORM_FEEDBACK_WRITE_BIT_EXT;
		layout = InvalidValue<VkImageLayout>();
	} break;

	case RESOURCE_STATE_INDIRECT_ARGUMENT: {
		stageMask = VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT;
		accessMask = VK_ACCESS_INDIRECT_COMMAND_READ_BIT;
		layout = InvalidValue<VkImageLayout>();
	} break;

	case RESOURCE_STATE_COPY_SRC: {
		stageMask = VK_PIPELINE_STAGE_TRANSFER_BIT;
		accessMask = VK_ACCESS_TRANSFER_READ_BIT;
		layout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
	} break;

	case RESOURCE_STATE_COPY_DST: {
		stageMask = VK_PIPELINE_STAGE_TRANSFER_BIT;
		accessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		layout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	} break;

	case RESOURCE_STATE_RESOLVE_SRC: {
		stageMask = VK_PIPELINE_STAGE_TRANSFER_BIT;
		accessMask = VK_ACCESS_TRANSFER_READ_BIT;
		layout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
	} break;

	case RESOURCE_STATE_RESOLVE_DST: {
		stageMask = VK_PIPELINE_STAGE_TRANSFER_BIT;
		accessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		layout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	} break;

	case RESOURCE_STATE_PRESENT: {
		stageMask = isSource ? VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT : VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
		accessMask = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT;
		layout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
	} break;

	case RESOURCE_STATE_PREDICATION: {
		stageMask = VK_PIPELINE_STAGE_CONDITIONAL_RENDERING_BIT_EXT;
		accessMask = VK_ACCESS_CONDITIONAL_RENDERING_READ_BIT_EXT;
		layout = InvalidValue<VkImageLayout>();
	} break;

	case RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE: {
		stageMask = InvalidValue<VkPipelineStageFlags>();
		accessMask = InvalidValue<VkAccessFlags>();
		layout = InvalidValue<VkImageLayout>();
	} break;

	case RESOURCE_STATE_FRAGMENT_DENSITY_MAP_ATTACHMENT: {
		stageMask = VK_PIPELINE_STAGE_FRAGMENT_DENSITY_PROCESS_BIT_EXT;
		accessMask = VK_ACCESS_FRAGMENT_DENSITY_MAP_READ_BIT_EXT;
		layout = VK_IMAGE_LAYOUT_FRAGMENT_DENSITY_MAP_OPTIMAL_EXT;
	} break;

	case RESOURCE_STATE_FRAGMENT_SHADING_RATE_ATTACHMENT: {
		stageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADING_RATE_ATTACHMENT_BIT_KHR;
		accessMask = VK_ACCESS_FRAGMENT_SHADING_RATE_ATTACHMENT_READ_BIT_KHR;
		layout = VK_IMAGE_LAYOUT_FRAGMENT_SHADING_RATE_ATTACHMENT_OPTIMAL_KHR;
	} break;
	}

	return SUCCESS;
}

Result ToVkBarrierSrc(
	ResourceState                   state,
	CommandType               commandType,
	const VkPhysicalDeviceFeatures& features,
	VkPipelineStageFlags& stageMask,
	VkAccessFlags& accessMask,
	VkImageLayout& layout)
{
	return ToVkBarrier(state, commandType, features, true, stageMask, accessMask, layout);
}

Result ToVkBarrierDst(
	ResourceState                   state,
	CommandType               commandType,
	const VkPhysicalDeviceFeatures& features,
	VkPipelineStageFlags& stageMask,
	VkAccessFlags& accessMask,
	VkImageLayout& layout)
{
	return ToVkBarrier(state, commandType, features, false, stageMask, accessMask, layout);
}

VkImageAspectFlags DetermineAspectMask(VkFormat format)
{
	switch (format) {
	// Depth
	case VK_FORMAT_D16_UNORM:
	case VK_FORMAT_X8_D24_UNORM_PACK32:
	case VK_FORMAT_D32_SFLOAT: {
		return VK_IMAGE_ASPECT_DEPTH_BIT;
	} break;

	 // Stencil
	case VK_FORMAT_S8_UINT: {
		return VK_IMAGE_ASPECT_STENCIL_BIT;
	} break;

	// Depth/stencil
	case VK_FORMAT_D16_UNORM_S8_UINT:
	case VK_FORMAT_D24_UNORM_S8_UINT:
	case VK_FORMAT_D32_SFLOAT_S8_UINT: {
		return VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
	} break;

	// Assume everything else is color
	default: break;
	}
	return VK_IMAGE_ASPECT_COLOR_BIT;
}

VmaMemoryUsage ToVmaMemoryUsage(MemoryUsage value)
{
	switch (value) {
	default: break;
	case MEMORY_USAGE_GPU_ONLY: return VMA_MEMORY_USAGE_GPU_ONLY; break;
	case MEMORY_USAGE_CPU_ONLY: return VMA_MEMORY_USAGE_CPU_ONLY; break;
	case MEMORY_USAGE_CPU_TO_GPU: return VMA_MEMORY_USAGE_CPU_TO_GPU; break;
	case MEMORY_USAGE_GPU_TO_CPU: return VMA_MEMORY_USAGE_GPU_TO_CPU; break;
	}
	return VMA_MEMORY_USAGE_UNKNOWN;
}

VkSamplerYcbcrModelConversion ToVkYcbcrModelConversion(YcbcrModelConversion value)
{
	switch (value) {
	default: break;
	case YCBCR_MODEL_CONVERSION_RGB_IDENTITY: return VK_SAMPLER_YCBCR_MODEL_CONVERSION_RGB_IDENTITY;
	case YCBCR_MODEL_CONVERSION_YCBCR_IDENTITY: return VK_SAMPLER_YCBCR_MODEL_CONVERSION_YCBCR_IDENTITY;
	case YCBCR_MODEL_CONVERSION_YCBCR_709: return VK_SAMPLER_YCBCR_MODEL_CONVERSION_YCBCR_709;
	case YCBCR_MODEL_CONVERSION_YCBCR_601: return VK_SAMPLER_YCBCR_MODEL_CONVERSION_YCBCR_601;
	case YCBCR_MODEL_CONVERSION_YCBCR_2020: return VK_SAMPLER_YCBCR_MODEL_CONVERSION_YCBCR_2020;
	}
	return InvalidValue<VkSamplerYcbcrModelConversion>();
}

VkSamplerYcbcrRange ToVkYcbcrRange(YcbcrRange value)
{
	switch (value) {
	default: break;
	case YCBCR_RANGE_ITU_FULL: return VK_SAMPLER_YCBCR_RANGE_ITU_FULL;
	case YCBCR_RANGE_ITU_NARROW: return VK_SAMPLER_YCBCR_RANGE_ITU_NARROW;
	}
	return InvalidValue<VkSamplerYcbcrRange>();
}

#pragma endregion

#pragma region DeviceQueue

bool DeviceQueue::init(vkb::Device& vkbDevice, vkb::QueueType type)
{
	switch (type)
	{
	case vkb::QueueType::present:  CommandType = COMMAND_TYPE_PRESENT; break;
	case vkb::QueueType::graphics: CommandType = COMMAND_TYPE_GRAPHICS; break;
	case vkb::QueueType::compute:  CommandType = COMMAND_TYPE_COMPUTE; break;
	case vkb::QueueType::transfer: CommandType = COMMAND_TYPE_TRANSFER; break;
	}

	auto queueRet = vkbDevice.get_queue(type);
	if (!queueRet.has_value())
	{
		Fatal("failed to get queue: " + queueRet.error().message());
		return false;
	}
	Queue = queueRet.value();

	auto queueFamilyRet = vkbDevice.get_queue_index(type);
	if (!queueFamilyRet.has_value())
	{
		Fatal("failed to get queue index: " + queueFamilyRet.error().message());
		return false;
	}
	QueueFamily = queueFamilyRet.value();

	return true;
}

#pragma endregion
