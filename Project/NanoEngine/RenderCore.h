#pragma once

namespace vkr {

#pragma region Decl Class

class RenderDevice;

class Buffer;
class CommandBuffer;
class CommandPool;
class ComputePipeline;
class DescriptorPool;
class DescriptorSet;
class DescriptorSetLayout;
class DrawPass;
class Fence;
class ShadingRatePattern;
class FullscreenQuad;
class GraphicsPipeline;
class Image;
class ImageView;
class Mesh;
class Pipeline;
class PipelineInterface;
class Queue;
class Query;
class RenderPass;
class Sampler;
class SamplerYcbcrConversion;
class Semaphore;
class ShaderModule;
class ShadingRatePattern;
class ShaderProgram;
class TextDraw;
class Texture;
class TextureFont;

class DepthStencilView;
class RenderTargetView;
class SampledImageView;
class StorageImageView;

struct IndexBufferView;
struct VertexBufferView;

class ImageResourceView;

using BufferPtr = ObjPtr<Buffer>;
using CommandBufferPtr = ObjPtr<CommandBuffer>;
using CommandPoolPtr = ObjPtr<CommandPool>;
using ComputePipelinePtr = ObjPtr<ComputePipeline>;
using DescriptorPoolPtr = ObjPtr<DescriptorPool>;
using DescriptorSetPtr = ObjPtr<DescriptorSet>;
using DescriptorSetLayoutPtr = ObjPtr<DescriptorSetLayout>;
using DrawPassPtr = ObjPtr<DrawPass>;
using FencePtr = ObjPtr<Fence>;
using ShadingRatePatternPtr = ObjPtr<ShadingRatePattern>;
using FullscreenQuadPtr = ObjPtr<FullscreenQuad>;
using GraphicsPipelinePtr = ObjPtr<GraphicsPipeline>;
using ImagePtr = ObjPtr<Image>;
using MeshPtr = ObjPtr<Mesh>;
using PipelineInterfacePtr = ObjPtr<PipelineInterface>;
using QueuePtr = ObjPtr<Queue>;
using QueryPtr = ObjPtr<Query>;
using RenderPassPtr = ObjPtr<RenderPass>;
using SamplerPtr = ObjPtr<Sampler>;
using SamplerYcbcrConversionPtr = ObjPtr<SamplerYcbcrConversion>;
using SemaphorePtr = ObjPtr<Semaphore>;
using ShaderModulePtr = ObjPtr<ShaderModule>;
using ShaderProgramPtr = ObjPtr<ShaderProgram>;
using TextDrawPtr = ObjPtr<TextDraw>;
using TexturePtr = ObjPtr<Texture>;
using TextureFontPtr = ObjPtr<TextureFont>;

using DepthStencilViewPtr = ObjPtr<DepthStencilView>;
using RenderTargetViewPtr = ObjPtr<RenderTargetView>;
using SampledImageViewPtr = ObjPtr<SampledImageView>;
using StorageImageViewPtr = ObjPtr<StorageImageView>;

using VkBufferPtr = VkHandlePtr<VkBuffer>;
using VkCommandBufferPtr = VkHandlePtr<VkCommandBuffer>;
using VkCommandPoolPtr = VkHandlePtr<VkCommandPool>;
using VkDebugUtilsMessengerPtr = VkHandlePtr<VkDebugUtilsMessengerEXT>;
using VkDescriptorPoolPtr = VkHandlePtr<VkDescriptorPool>;
using VkDescriptorSetPtr = VkHandlePtr<VkDescriptorSet>;
using VkDescriptorSetLayoutPtr = VkHandlePtr<VkDescriptorSetLayout>;
using VkDevicePtr = VkHandlePtr<VkDevice>;
using VkFencePtr = VkHandlePtr<VkFence>;
using VkFramebufferPtr = VkHandlePtr<VkFramebuffer>;
using VkImagePtr = VkHandlePtr<VkImage>;
using VkImageViewPtr = VkHandlePtr<VkImageView>;
using VkInstancePtr = VkHandlePtr<VkInstance>;
using VkPhysicalDevicePtr = VkHandlePtr<VkPhysicalDevice>;
using VkPipelinePtr = VkHandlePtr<VkPipeline>;
using VkPipelineLayoutPtr = VkHandlePtr<VkPipelineLayout>;
using VkQueryPoolPtr = VkHandlePtr<VkQueryPool>;
using VkQueuePtr = VkHandlePtr<VkQueue>;
using VkRenderPassPtr = VkHandlePtr<VkRenderPass>;
using VkSamplerPtr = VkHandlePtr<VkSampler>;
using VkSamplerYcbcrConversionPtr = VkHandlePtr<VkSamplerYcbcrConversion>;
using VkSemaphorePtr = VkHandlePtr<VkSemaphore>;
using VkShaderModulePtr = VkHandlePtr<VkShaderModule>;
using VkSurfacePtr = VkHandlePtr<VkSurfaceKHR>;
using VkSwapchainPtr = VkHandlePtr<VkSwapchainKHR>;

using VmaAllocationPtr = VkHandlePtr<VmaAllocation>;
using VmaAllocatorPtr = VkHandlePtr<VmaAllocator>;

#pragma endregion

#pragma region Constants

#define VALUE_IGNORED                       UINT32_MAX

#define MAX_SAMPLER_DESCRIPTORS             2048
#define DEFAULT_RESOURCE_DESCRIPTOR_COUNT   8192
#define DEFAULT_SAMPLE_DESCRIPTOR_COUNT     MAX_SAMPLER_DESCRIPTORS

constexpr uint32_t MAX_RENDER_TARGETS = 8;

#define REMAINING_MIP_LEVELS                UINT32_MAX
#define REMAINING_ARRAY_LAYERS              UINT32_MAX
#define ALL_SUBRESOURCES                    0, REMAINING_MIP_LEVELS, 0, REMAINING_ARRAY_LAYERS

#define MAX_VERTEX_BINDINGS                 16
#define APPEND_OFFSET_ALIGNED               UINT32_MAX

#define MAX_VIEWPORTS                       16
#define MAX_SCISSORS                        16

#define MAX_SETS_PER_POOL                   1024
#define MAX_BOUND_DESCRIPTOR_SETS           32

#define WHOLE_SIZE                          UINT64_MAX

// This value is based on what the majority of the GPUs can support in Vulkan. While D3D12 generally allows about 64 DWORDs, a significant amount of Vulkan drivers have a limit of 128 bytes for VkPhysicalDeviceLimits::maxPushConstantsSize. This limits the numberof push constants to 32.
#define MAX_PUSH_CONSTANTS                  32

// Vulkan dynamic uniform/storage buffers requires that offsets are aligned to VkPhysicalDeviceLimits::minUniformBufferOffsetAlignment. Based on vulkan.gpuinfo.org, the range of this value [1, 256] Meaning that 256 should cover all offset cases.
// D3D12 on most(all?) GPUs require that the minimum constant buffer size to be 256.
#define CONSTANT_BUFFER_ALIGNMENT           256
#define UNIFORM_BUFFER_ALIGNMENT            CONSTANT_BUFFER_ALIGNMENT
#define STORAGE_BUFFER_ALIGNMENT            CONSTANT_BUFFER_ALIGNMENT
#define STUCTURED_BUFFER_ALIGNMENT          CONSTANT_BUFFER_ALIGNMENT

#define MINIMUM_CONSTANT_BUFFER_SIZE        CONSTANT_BUFFER_ALIGNMENT
#define MINIMUM_UNIFORM_BUFFER_SIZE         CONSTANT_BUFFER_ALIGNMENT
#define MINIMUM_STORAGE_BUFFER_SIZE         CONSTANT_BUFFER_ALIGNMENT
#define MINIMUM_STRUCTURED_BUFFER_SIZE      CONSTANT_BUFFER_ALIGNMENT

#define MAX_MODEL_TEXTURES_IN_CREATE_INFO 16

// standard attribute semantic names
#define SEMANTIC_NAME_POSITION  "POSITION"
#define SEMANTIC_NAME_NORMAL    "NORMAL"
#define SEMANTIC_NAME_COLOR     "COLOR"
#define SEMANTIC_NAME_TEXCOORD  "TEXCOORD"
#define SEMANTIC_NAME_TANGENT   "TANGENT"
#define SEMANTIC_NAME_BITANGENT "BITANGENT"
#define SEMANTIC_NAME_CUSTOM    "CUSTOM"

#pragma endregion

#pragma region Enums

enum AttachmentLoadOp
{
	ATTACHMENT_LOAD_OP_LOAD = 0,
	ATTACHMENT_LOAD_OP_CLEAR,
	ATTACHMENT_LOAD_OP_DONT_CARE,
};

enum AttachmentStoreOp
{
	ATTACHMENT_STORE_OP_STORE = 0,
	ATTACHMENT_STORE_OP_DONT_CARE,
};

enum BlendFactor
{
	BLEND_FACTOR_ZERO = 0,
	BLEND_FACTOR_ONE = 1,
	BLEND_FACTOR_SRC_COLOR = 2,
	BLEND_FACTOR_ONE_MINUS_SRC_COLOR = 3,
	BLEND_FACTOR_DST_COLOR = 4,
	BLEND_FACTOR_ONE_MINUS_DST_COLOR = 5,
	BLEND_FACTOR_SRC_ALPHA = 6,
	BLEND_FACTOR_ONE_MINUS_SRC_ALPHA = 7,
	BLEND_FACTOR_DST_ALPHA = 8,
	BLEND_FACTOR_ONE_MINUS_DST_ALPHA = 9,
	BLEND_FACTOR_CONSTANT_COLOR = 10,
	BLEND_FACTOR_ONE_MINUS_CONSTANT_COLOR = 11,
	BLEND_FACTOR_CONSTANT_ALPHA = 12,
	BLEND_FACTOR_ONE_MINUS_CONSTANT_ALPHA = 13,
	BLEND_FACTOR_SRC_ALPHA_SATURATE = 14,
	BLEND_FACTOR_SRC1_COLOR = 15,
	BLEND_FACTOR_ONE_MINUS_SRC1_COLOR = 16,
	BLEND_FACTOR_SRC1_ALPHA = 17,
	BLEND_FACTOR_ONE_MINUS_SRC1_ALPHA = 18,
};

// Some basic blend modes to make basic graphics pipelines a bit more manageable.
// Premultipled cases not explicitly handled.
enum BlendMode
{
	BLEND_MODE_NONE = 0,
	BLEND_MODE_ADDITIVE = 1,
	BLEND_MODE_ALPHA = 2,
	BLEND_MODE_OVER = 3,
	BLEND_MODE_UNDER = 4,
	BLEND_MODE_PREMULT_ALPHA = 5,
};

enum BlendOp
{
	BLEND_OP_ADD = 0,
	BLEND_OP_SUBTRACT = 1,
	BLEND_OP_REVERSE_SUBTRACT = 2,
	BLEND_OP_MIN = 3,
	BLEND_OP_MAX = 4,

#if defined(VK_BLEND_OPERATION_ADVANCED)
	// Provdied by VK_blend_operation_advanced
	BLEND_OP_ZERO = 1000148000,
	BLEND_OP_SRC = 1000148001,
	BLEND_OP_DST = 1000148002,
	BLEND_OP_SRC_OVER = 1000148003,
	BLEND_OP_DST_OVER = 1000148004,
	BLEND_OP_SRC_IN = 1000148005,
	BLEND_OP_DST_IN = 1000148006,
	BLEND_OP_SRC_OUT = 1000148007,
	BLEND_OP_DST_OUT = 1000148008,
	BLEND_OP_SRC_ATOP = 1000148009,
	BLEND_OP_DST_ATOP = 1000148010,
	BLEND_OP_XOR = 1000148011,
	BLEND_OP_MULTIPLY = 1000148012,
	BLEND_OP_SCREEN = 1000148013,
	BLEND_OP_OVERLAY = 1000148014,
	BLEND_OP_DARKEN = 1000148015,
	BLEND_OP_LIGHTEN = 1000148016,
	BLEND_OP_COLORDODGE = 1000148017,
	BLEND_OP_COLORBURN = 1000148018,
	BLEND_OP_HARDLIGHT = 1000148019,
	BLEND_OP_SOFTLIGHT = 1000148020,
	BLEND_OP_DIFFERENCE = 1000148021,
	BLEND_OP_EXCLUSION = 1000148022,
	BLEND_OP_INVERT = 1000148023,
	BLEND_OP_INVERT_RGB = 1000148024,
	BLEND_OP_LINEARDODGE = 1000148025,
	BLEND_OP_LINEARBURN = 1000148026,
	BLEND_OP_VIVIDLIGHT = 1000148027,
	BLEND_OP_LINEARLIGHT = 1000148028,
	BLEND_OP_PINLIGHT = 1000148029,
	BLEND_OP_HARDMIX = 1000148030,
	BLEND_OP_HSL_HUE = 1000148031,
	BLEND_OP_HSL_SATURATION = 1000148032,
	BLEND_OP_HSL_COLOR = 1000148033,
	BLEND_OP_HSL_LUMINOSITY = 1000148034,
	BLEND_OP_PLUS = 1000148035,
	BLEND_OP_PLUS_CLAMPED = 1000148036,
	BLEND_OP_PLUS_CLAMPED_ALPHA = 1000148037,
	BLEND_OP_PLUS_DARKER = 1000148038,
	BLEND_OP_MINUS = 1000148039,
	BLEND_OP_MINUS_CLAMPED = 1000148040,
	BLEND_OP_CONTRAST = 1000148041,
	BLEND_OP_INVERT_OVG = 1000148042,
	BLEND_OP_RED = 1000148043,
	BLEND_OP_GREEN = 1000148044,
	BLEND_OP_BLUE = 1000148045,
#endif // defined(VK_BLEND_OPERATION_ADVANCED)
};

enum BorderColor
{
	BORDER_COLOR_FLOAT_TRANSPARENT_BLACK = 0,
	BORDER_COLOR_INT_TRANSPARENT_BLACK = 1,
	BORDER_COLOR_FLOAT_OPAQUE_BLACK = 2,
	BORDER_COLOR_INT_OPAQUE_BLACK = 3,
	BORDER_COLOR_FLOAT_OPAQUE_WHITE = 4,
	BORDER_COLOR_INT_OPAQUE_WHITE = 5,
};

enum BufferUsageFlagBits
{
	BUFFER_USAGE_TRANSFER_SRC = 0x00000001,
	BUFFER_USAGE_TRANSFER_DST = 0x00000002,
	BUFFER_USAGE_UNIFORM_TEXEL_BUFFER = 0x00000004,
	BUFFER_USAGE_STORAGE_TEXEL_BUFFER = 0x00000008,
	BUFFER_USAGE_UNIFORM_BUFFER = 0x00000010,
	BUFFER_USAGE_STORAGE_BUFFER = 0x00000020,
	BUFFER_USAGE_INDEX_BUFFER = 0x00000040,
	BUFFER_USAGE_VERTEX_BUFFER = 0x00000080,
	BUFFER_USAGE_INDIRECT_BUFFER = 0x00000100,
	BUFFER_USAGE_CONDITIONAL_RENDERING = 0x00000200,
	BUFFER_USAGE_RAY_TRACING = 0x00000400,
	BUFFER_USAGE_TRANSFORM_FEEDBACK_BUFFER = 0x00000800,
	BUFFER_USAGE_TRANSFORM_FEEDBACK_COUNTER_BUFFER = 0x00001000,
	BUFFER_USAGE_SHADER_DEVICE_ADDRESS = 0x00002000,
};

enum ChromaLocation
{
	// Specifies that downsampled chroma samples are aligned with even luma sample coordinates.
	CHROMA_LOCATION_COSITED_EVEN = 0,
	// Specifies that downsampled chroma samples are located between even luma sample coordinates and the next higher odd luma sample coordinate.
	CHROMA_LOCATION_MIDPOINT = 1,
};

enum ColorComponentFlagBits
{
	COLOR_COMPONENT_R = 0x00000001,
	COLOR_COMPONENT_G = 0x00000002,
	COLOR_COMPONENT_B = 0x00000004,
	COLOR_COMPONENT_A = 0x00000008,
};

enum CompareOp
{
	COMPARE_OP_NEVER = 0,
	COMPARE_OP_LESS = 1,
	COMPARE_OP_EQUAL = 2,
	COMPARE_OP_LESS_OR_EQUAL = 3,
	COMPARE_OP_GREATER = 4,
	COMPARE_OP_NOT_EQUAL = 5,
	COMPARE_OP_GREATER_OR_EQUAL = 6,
	COMPARE_OP_ALWAYS = 7,
};

enum CommandType
{
	COMMAND_TYPE_UNDEFINED = 0,
	COMMAND_TYPE_GRAPHICS = 1,
	COMMAND_TYPE_COMPUTE = 2,
	COMMAND_TYPE_TRANSFER = 3,
	COMMAND_TYPE_PRESENT = 4,
};

enum ComponentSwizzle
{
	COMPONENT_SWIZZLE_IDENTITY = 0,
	COMPONENT_SWIZZLE_ZERO = 1,
	COMPONENT_SWIZZLE_ONE = 2,
	COMPONENT_SWIZZLE_R = 3,
	COMPONENT_SWIZZLE_G = 4,
	COMPONENT_SWIZZLE_B = 5,
	COMPONENT_SWIZZLE_A = 6,
};

enum CullMode
{
	CULL_MODE_NONE = 0,
	CULL_MODE_FRONT = 1,
	CULL_MODE_BACK = 2,
};

enum ClearFlagBits
{
	CLEAR_FLAG_DEPTH = 0x1,
	CLEAR_FLAG_STENCIL = 0x2,
};

enum DescriptorType
{
	// NOTE: These *DO NOT* match the enums in Vulkan
	DESCRIPTOR_TYPE_UNDEFINED = 0,
	DESCRIPTOR_TYPE_SAMPLER = 1,  // Sampler
	DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER = 2,  // Combined image and sampler
	DESCRIPTOR_TYPE_SAMPLED_IMAGE = 3,  // RO image object
	DESCRIPTOR_TYPE_STORAGE_IMAGE = 4,  // RW image object
	DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER = 5,  // RO texel buffer object
	DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER = 6,  // RW texel buffer object
	DESCRIPTOR_TYPE_UNIFORM_BUFFER = 7,  // constant/uniform buffer object
	DESCRIPTOR_TYPE_RAW_STORAGE_BUFFER = 8,  // RW raw buffer object
	DESCRIPTOR_TYPE_RO_STRUCTURED_BUFFER = 9,  // RO structured buffer object
	DESCRIPTOR_TYPE_RW_STRUCTURED_BUFFER = 10, // RW structured buffer object
	DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC = 11,
	DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC = 12,
	DESCRIPTOR_TYPE_INPUT_ATTACHMENT = 13,
};

enum DrawPassClearFlagBits
{
	DRAW_PASS_CLEAR_FLAG_CLEAR_RENDER_TARGETS = 0x1,
	DRAW_PASS_CLEAR_FLAG_CLEAR_DEPTH = 0x2,
	DRAW_PASS_CLEAR_FLAG_CLEAR_STENCIL = 0x4,
	DRAW_PASS_CLEAR_FLAG_CLEAR_ALL = DRAW_PASS_CLEAR_FLAG_CLEAR_RENDER_TARGETS | DRAW_PASS_CLEAR_FLAG_CLEAR_DEPTH | DRAW_PASS_CLEAR_FLAG_CLEAR_STENCIL,
};

enum Filter
{
	FILTER_NEAREST = 0,
	FILTER_LINEAR = 1,
};

enum ShadingRateMode
{
	SHADING_RATE_NONE = 0,
	SHADING_RATE_FDM = 1, // Fragment Density Map
	SHADING_RATE_VRS = 2, // Variable Rate Shading
};

enum FrontFace
{
	FRONT_FACE_CCW = 0, // Counter clockwise
	FRONT_FACE_CW = 1, // Clockwise
};

enum ImageType
{
	IMAGE_TYPE_UNDEFINED = 0,
	IMAGE_TYPE_1D = 1,
	IMAGE_TYPE_2D = 2,
	IMAGE_TYPE_3D = 3,
	IMAGE_TYPE_CUBE = 6,
};

enum ImageUsageFlagBits
{
	IMAGE_USAGE_TRANSFER_SRC = 0x00000001,
	IMAGE_USAGE_TRANSFER_DST = 0x00000002,
	IMAGE_USAGE_SAMPLED = 0x00000004,
	IMAGE_USAGE_STORAGE = 0x00000008,
	IMAGE_USAGE_COLOR_ATTACHMENT = 0x00000010,
	IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT = 0x00000020,
	IMAGE_USAGE_TRANSIENT_ATTACHMENT = 0x00000040,
	IMAGE_USAGE_INPUT_ATTACHMENT = 0x00000080,
	IMAGE_USAGE_SHADING_RATE_IMAGE_NV = 0x00000100,
	IMAGE_USAGE_FRAGMENT_DENSITY_MAP = 0x00000200,
};

enum ImageViewType
{
	IMAGE_VIEW_TYPE_UNDEFINED = 0,
	IMAGE_VIEW_TYPE_1D = 1,
	IMAGE_VIEW_TYPE_2D = 2,
	IMAGE_VIEW_TYPE_3D = 3,
	IMAGE_VIEW_TYPE_CUBE = 4,
	IMAGE_VIEW_TYPE_1D_ARRAY = 5,
	IMAGE_VIEW_TYPE_2D_ARRAY = 6,
	IMAGE_VIEW_TYPE_CUBE_ARRAY = 7,
};

enum IndexType
{
	INDEX_TYPE_UNDEFINED = 0,
	INDEX_TYPE_UINT16 = 1,
	INDEX_TYPE_UINT32 = 2,
	// Vulkan: UINT8 requires VK_EXT_index_type_uint8
	// DX12: UINT8 is not supported
	INDEX_TYPE_UINT8 = 3,
};

enum LogicOp
{
	LOGIC_OP_CLEAR = 0,
	LOGIC_OP_AND = 1,
	LOGIC_OP_AND_REVERSE = 2,
	LOGIC_OP_COPY = 3,
	LOGIC_OP_AND_INVERTED = 4,
	LOGIC_OP_NO_OP = 5,
	LOGIC_OP_XOR = 6,
	LOGIC_OP_OR = 7,
	LOGIC_OP_NOR = 8,
	LOGIC_OP_EQUIVALENT = 9,
	LOGIC_OP_INVERT = 10,
	LOGIC_OP_OR_REVERSE = 11,
	LOGIC_OP_COPY_INVERTED = 12,
	LOGIC_OP_OR_INVERTED = 13,
	LOGIC_OP_NAND = 14,
	LOGIC_OP_SET = 15,
};

enum MemoryUsage
{
	MEMORY_USAGE_UNKNOWN = 0,
	MEMORY_USAGE_GPU_ONLY = 1,
	MEMORY_USAGE_CPU_ONLY = 2,
	MEMORY_USAGE_CPU_TO_GPU = 3,
	MEMORY_USAGE_GPU_TO_CPU = 4,
};

// VK: Maps to top/bottom of pipeline stages for timestamp queries.
enum PipelineStage
{
	PIPELINE_STAGE_TOP_OF_PIPE_BIT = 0,
	PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT = 1,
};

enum PolygonMode
{
	POLYGON_MODE_FILL = 0,
	POLYGON_MODE_LINE = 1,
	POLYGON_MODE_POINT = 2,
};

enum PresentMode
{
	PRESENT_MODE_UNDEFINED = 0,
	PRESENT_MODE_FIFO,
	PRESENT_MODE_MAILBOX,
	PRESENT_MODE_IMMEDIATE,
};

enum PrimitiveTopology
{
	PRIMITIVE_TOPOLOGY_TRIANGLE_LIST = 0,
	PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP = 1,
	PRIMITIVE_TOPOLOGY_TRIANGLE_FAN = 2,
	PRIMITIVE_TOPOLOGY_POINT_LIST = 3,
	PRIMITIVE_TOPOLOGY_LINE_LIST = 4,
	PRIMITIVE_TOPOLOGY_LINE_STRIP = 5,
	PRIMITIVE_TOPOLOGY_PATCH_LIST = 6,
};

enum QueryType
{
	QUERY_TYPE_UNDEFINED = 0,
	QUERY_TYPE_OCCLUSION = 1,
	QUERY_TYPE_PIPELINE_STATISTICS = 2,
	QUERY_TYPE_TIMESTAMP = 3,
};

enum ResourceState
{
	RESOURCE_STATE_UNDEFINED = 0,
	RESOURCE_STATE_GENERAL,
	RESOURCE_STATE_CONSTANT_BUFFER,
	RESOURCE_STATE_VERTEX_BUFFER,
	RESOURCE_STATE_INDEX_BUFFER,
	RESOURCE_STATE_RENDER_TARGET,
	RESOURCE_STATE_UNORDERED_ACCESS,
	RESOURCE_STATE_DEPTH_STENCIL_READ,        // Depth and stencil READ
	RESOURCE_STATE_DEPTH_STENCIL_WRITE,       // Depth and stencil WRITE
	RESOURCE_STATE_DEPTH_WRITE_STENCIL_READ,  // Depth WRITE and stencil READ
	RESOURCE_STATE_DEPTH_READ_STENCIL_WRITE,  // Depth READ and stencil WRITE
	RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, // VS, HS, DS, GS, CS
	RESOURCE_STATE_PIXEL_SHADER_RESOURCE,     // PS
	RESOURCE_STATE_SHADER_RESOURCE,           // RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE and RESOURCE_STATE_PIXEL_SHADER_RESOURCE
	RESOURCE_STATE_STREAM_OUT,
	RESOURCE_STATE_INDIRECT_ARGUMENT,
	RESOURCE_STATE_COPY_SRC,
	RESOURCE_STATE_COPY_DST,
	RESOURCE_STATE_RESOLVE_SRC,
	RESOURCE_STATE_RESOLVE_DST,
	RESOURCE_STATE_PRESENT,
	RESOURCE_STATE_PREDICATION,
	RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE,
	RESOURCE_STATE_FRAGMENT_DENSITY_MAP_ATTACHMENT,
	RESOURCE_STATE_FRAGMENT_SHADING_RATE_ATTACHMENT,
};

enum SamplerAddressMode
{
	SAMPLER_ADDRESS_MODE_REPEAT = 0,
	SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT = 1,
	SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE = 2,
	SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER = 3,
};

enum SamplerMipmapMode
{
	SAMPLER_MIPMAP_MODE_NEAREST = 0,
	SAMPLER_MIPMAP_MODE_LINEAR = 1,
};

enum SampleCount
{
	SAMPLE_COUNT_1 = 1,
	SAMPLE_COUNT_2 = 2,
	SAMPLE_COUNT_4 = 4,
	SAMPLE_COUNT_8 = 8,
	SAMPLE_COUNT_16 = 16,
	SAMPLE_COUNT_32 = 32,
	SAMPLE_COUNT_64 = 64,
};

enum SemaphoreType
{
	SEMAPHORE_TYPE_BINARY = 0,
	SEMAPHORE_TYPE_TIMELINE = 1,
};

enum ShaderStageBits
{
	SHADER_STAGE_UNDEFINED = 0x00000000,
	SHADER_STAGE_VS = 0x00000001,
	SHADER_STAGE_HS = 0x00000002,
	SHADER_STAGE_DS = 0x00000004,
	SHADER_STAGE_GS = 0x00000008,
	SHADER_STAGE_PS = 0x00000010,
	SHADER_STAGE_CS = 0x00000020,
	SHADER_STAGE_ALL_GRAPHICS = 0x0000001F,
	SHADER_STAGE_ALL = 0x7FFFFFFF,
};

enum StencilOp
{
	STENCIL_OP_KEEP = 0,
	STENCIL_OP_ZERO = 1,
	STENCIL_OP_REPLACE = 2,
	STENCIL_OP_INCREMENT_AND_CLAMP = 3,
	STENCIL_OP_DECREMENT_AND_CLAMP = 4,
	STENCIL_OP_INVERT = 5,
	STENCIL_OP_INCREMENT_AND_WRAP = 6,
	STENCIL_OP_DECREMENT_AND_WRAP = 7,
};

enum TessellationDomainOrigin
{
	TESSELLATION_DOMAIN_ORIGIN_UPPER_LEFT = 0,
	TESSELLATION_DOMAIN_ORIGIN_LOWER_LEFT = 1,
};

enum TransitionFlag
{
	TRANSITION_FLAG_API_REQUIRED = 1, // Indicates a transition that must be executed by the API
	TRANSITION_FALG_API_OPTIONAL = 2, // Indicates a transition that can be optionally ignored by the API
};

enum VendorId
{
	VENDOR_ID_UNKNOWN = 0x0000,
	VENDOR_ID_AMD = 0x1002,
	VENDOR_ID_INTEL = 0x8086,
	VENDOR_ID_NVIDIA = 0x10DE,
};

enum VertexInputRate
{
	VERTEX_INPUT_RATE_VERTEX = 0,
	VERETX_INPUT_RATE_INSTANCE = 1,
};

enum VertexSemantic
{
	VERTEX_SEMANTIC_UNDEFINED = 0,
	VERTEX_SEMANTIC_POSITION = 1, // Object space position
	VERTEX_SEMANTIC_NORMAL = 2, // Object space normals
	VERTEX_SEMANTIC_COLOR = 3, // Vertex color
	VERTEX_SEMANTIC_TANGENT = 4, // Object space tangents
	VERTEX_SEMANTIC_BITANGENT = 5, // Object space bitangents
	VERTEX_SEMANTIC_TEXCOORD = 6,
	VERTEX_SEMANTIC_TEXCOORD0 = VERTEX_SEMANTIC_TEXCOORD + 1,
	VERTEX_SEMANTIC_TEXCOORD1 = VERTEX_SEMANTIC_TEXCOORD0 + 1,
	VERTEX_SEMANTIC_TEXCOORD2 = VERTEX_SEMANTIC_TEXCOORD1 + 1,
	VERTEX_SEMANTIC_TEXCOORD3 = VERTEX_SEMANTIC_TEXCOORD2 + 1,
	VERTEX_SEMANTIC_TEXCOORD4 = VERTEX_SEMANTIC_TEXCOORD3 + 1,
	VERTEX_SEMANTIC_TEXCOORD5 = VERTEX_SEMANTIC_TEXCOORD4 + 1,
	VERTEX_SEMANTIC_TEXCOORD6 = VERTEX_SEMANTIC_TEXCOORD5 + 1,
	VERTEX_SEMANTIC_TEXCOORD7 = VERTEX_SEMANTIC_TEXCOORD6 + 1,
	VERTEX_SEMANTIC_TEXCOORD8 = VERTEX_SEMANTIC_TEXCOORD7 + 1,
	VERTEX_SEMANTIC_TEXCOORD9 = VERTEX_SEMANTIC_TEXCOORD8 + 1,
	VERTEX_SEMANTIC_TEXCOORD10 = VERTEX_SEMANTIC_TEXCOORD9 + 1,
	VERTEX_SEMANTIC_TEXCOORD11 = VERTEX_SEMANTIC_TEXCOORD10 + 1,
	VERTEX_SEMANTIC_TEXCOORD12 = VERTEX_SEMANTIC_TEXCOORD11 + 1,
	VERTEX_SEMANTIC_TEXCOORD13 = VERTEX_SEMANTIC_TEXCOORD12 + 1,
	VERTEX_SEMANTIC_TEXCOORD14 = VERTEX_SEMANTIC_TEXCOORD13 + 1,
	VERTEX_SEMANTIC_TEXCOORD15 = VERTEX_SEMANTIC_TEXCOORD14 + 1,
	VERTEX_SEMANTIC_TEXCOORD16 = VERTEX_SEMANTIC_TEXCOORD15 + 1,
	VERTEX_SEMANTIC_TEXCOORD17 = VERTEX_SEMANTIC_TEXCOORD16 + 1,
	VERTEX_SEMANTIC_TEXCOORD18 = VERTEX_SEMANTIC_TEXCOORD17 + 1,
	VERTEX_SEMANTIC_TEXCOORD19 = VERTEX_SEMANTIC_TEXCOORD18 + 1,
	VERTEX_SEMANTIC_TEXCOORD20 = VERTEX_SEMANTIC_TEXCOORD19 + 1,
	VERTEX_SEMANTIC_TEXCOORD21 = VERTEX_SEMANTIC_TEXCOORD20 + 1,
	VERTEX_SEMANTIC_TEXCOORD22 = VERTEX_SEMANTIC_TEXCOORD21 + 1,
};

enum YcbcrModelConversion
{
	// Specifies that the color space for the image is RGB, and should be kept that way without conversion.
	YCBCR_MODEL_CONVERSION_RGB_IDENTITY = 0,
	// Specifies that the color space for the image is YCbCr, and should be range expanded from a compressed format.
	YCBCR_MODEL_CONVERSION_YCBCR_IDENTITY = 1,
	// Specifies that the color model is YCbCr, but should be converted to RGB based on BT.709 for shader operations.
	YCBCR_MODEL_CONVERSION_YCBCR_709 = 2,
	// Specifies that the color model is YCbCr, but should be converted to RGB based on BT.601 for shader operations.
	YCBCR_MODEL_CONVERSION_YCBCR_601 = 3,
	// Specifies that the color model is YCbCr, but should be converted to RGB based on BT.2020 for shader operations.
	YCBCR_MODEL_CONVERSION_YCBCR_2020 = 4,
};

enum YcbcrRange
{
	YCBCR_RANGE_ITU_FULL = 0,
	YCBCR_RANGE_ITU_NARROW = 1,
};

#pragma endregion

#pragma region Format

enum Format
{
	FORMAT_UNDEFINED = 0,

	// 8-bit signed normalized
	FORMAT_R8_SNORM,
	FORMAT_R8G8_SNORM,
	FORMAT_R8G8B8_SNORM,
	FORMAT_R8G8B8A8_SNORM,
	FORMAT_B8G8R8_SNORM,
	FORMAT_B8G8R8A8_SNORM,

	// 8-bit unsigned normalized
	FORMAT_R8_UNORM,
	FORMAT_R8G8_UNORM,
	FORMAT_R8G8B8_UNORM,
	FORMAT_R8G8B8A8_UNORM,
	FORMAT_B8G8R8_UNORM,
	FORMAT_B8G8R8A8_UNORM,

	// 8-bit signed integer
	FORMAT_R8_SINT,
	FORMAT_R8G8_SINT,
	FORMAT_R8G8B8_SINT,
	FORMAT_R8G8B8A8_SINT,
	FORMAT_B8G8R8_SINT,
	FORMAT_B8G8R8A8_SINT,

	// 8-bit unsigned integer
	FORMAT_R8_UINT,
	FORMAT_R8G8_UINT,
	FORMAT_R8G8B8_UINT,
	FORMAT_R8G8B8A8_UINT,
	FORMAT_B8G8R8_UINT,
	FORMAT_B8G8R8A8_UINT,

	// 16-bit signed normalized
	FORMAT_R16_SNORM,
	FORMAT_R16G16_SNORM,
	FORMAT_R16G16B16_SNORM,
	FORMAT_R16G16B16A16_SNORM,

	// 16-bit unsigned normalized
	FORMAT_R16_UNORM,
	FORMAT_R16G16_UNORM,
	FORMAT_R16G16B16_UNORM,
	FORMAT_R16G16B16A16_UNORM,

	// 16-bit signed integer
	FORMAT_R16_SINT,
	FORMAT_R16G16_SINT,
	FORMAT_R16G16B16_SINT,
	FORMAT_R16G16B16A16_SINT,

	// 16-bit unsigned integer
	FORMAT_R16_UINT,
	FORMAT_R16G16_UINT,
	FORMAT_R16G16B16_UINT,
	FORMAT_R16G16B16A16_UINT,

	// 16-bit float
	FORMAT_R16_FLOAT,
	FORMAT_R16G16_FLOAT,
	FORMAT_R16G16B16_FLOAT,
	FORMAT_R16G16B16A16_FLOAT,

	// 32-bit signed integer
	FORMAT_R32_SINT,
	FORMAT_R32G32_SINT,
	FORMAT_R32G32B32_SINT,
	FORMAT_R32G32B32A32_SINT,

	// 32-bit unsigned integer
	FORMAT_R32_UINT,
	FORMAT_R32G32_UINT,
	FORMAT_R32G32B32_UINT,
	FORMAT_R32G32B32A32_UINT,

	// 32-bit float
	FORMAT_R32_FLOAT,
	FORMAT_R32G32_FLOAT,
	FORMAT_R32G32B32_FLOAT,
	FORMAT_R32G32B32A32_FLOAT,

	// 8-bit unsigned integer stencil
	FORMAT_S8_UINT,

	// 16-bit unsigned normalized depth
	FORMAT_D16_UNORM,

	// 32-bit float depth
	FORMAT_D32_FLOAT,

	// Depth/stencil combinations
	FORMAT_D16_UNORM_S8_UINT,
	FORMAT_D24_UNORM_S8_UINT,
	FORMAT_D32_FLOAT_S8_UINT,

	// SRGB
	FORMAT_R8_SRGB,
	FORMAT_R8G8_SRGB,
	FORMAT_R8G8B8_SRGB,
	FORMAT_R8G8B8A8_SRGB,
	FORMAT_B8G8R8_SRGB,
	FORMAT_B8G8R8A8_SRGB,

	// 10-bit RGB, 2-bit A packed
	FORMAT_R10G10B10A2_UNORM,

	// 11-bit R, 11-bit G, 10-bit B packed
	FORMAT_R11G11B10_FLOAT,

	// Compressed formats
	FORMAT_BC1_RGBA_SRGB,
	FORMAT_BC1_RGBA_UNORM,
	FORMAT_BC1_RGB_SRGB,
	FORMAT_BC1_RGB_UNORM,
	FORMAT_BC2_SRGB,
	FORMAT_BC2_UNORM,
	FORMAT_BC3_SRGB,
	FORMAT_BC3_UNORM,
	FORMAT_BC4_UNORM,
	FORMAT_BC4_SNORM,
	FORMAT_BC5_UNORM,
	FORMAT_BC5_SNORM,
	FORMAT_BC6H_UFLOAT,
	FORMAT_BC6H_SFLOAT,
	FORMAT_BC7_UNORM,
	FORMAT_BC7_SRGB,

	FORMAT_COUNT,
};

enum FormatAspectBit
{
	FORMAT_ASPECT_UNDEFINED = 0x0,
	FORMAT_ASPECT_COLOR = 0x1,
	FORMAT_ASPECT_DEPTH = 0x2,
	FORMAT_ASPECT_STENCIL = 0x4,

	FORMAT_ASPECT_DEPTH_STENCIL = FORMAT_ASPECT_DEPTH | FORMAT_ASPECT_STENCIL,
};

enum FormatComponentBit
{
	FORMAT_COMPONENT_UNDEFINED = 0x0,
	FORMAT_COMPONENT_RED = 0x1,
	FORMAT_COMPONENT_GREEN = 0x2,
	FORMAT_COMPONENT_BLUE = 0x4,
	FORMAT_COMPONENT_ALPHA = 0x8,
	FORMAT_COMPONENT_DEPTH = 0x10,
	FORMAT_COMPONENT_STENCIL = 0x20,

	FORMAT_COMPONENT_RED_GREEN = FORMAT_COMPONENT_RED | FORMAT_COMPONENT_GREEN,
	FORMAT_COMPONENT_RED_GREEN_BLUE = FORMAT_COMPONENT_RED | FORMAT_COMPONENT_GREEN | FORMAT_COMPONENT_BLUE,
	FORMAT_COMPONENT_RED_GREEN_BLUE_ALPHA = FORMAT_COMPONENT_RED | FORMAT_COMPONENT_GREEN | FORMAT_COMPONENT_BLUE | FORMAT_COMPONENT_ALPHA,
	FORMAT_COMPONENT_DEPTH_STENCIL = FORMAT_COMPONENT_DEPTH | FORMAT_COMPONENT_STENCIL,
};

enum FormatDataType
{
	FORMAT_DATA_TYPE_UNDEFINED = 0x0,
	FORMAT_DATA_TYPE_UNORM = 0x1,
	FORMAT_DATA_TYPE_SNORM = 0x2,
	FORMAT_DATA_TYPE_UINT = 0x4,
	FORMAT_DATA_TYPE_SINT = 0x8,
	FORMAT_DATA_TYPE_FLOAT = 0x10,
	FORMAT_DATA_TYPE_SRGB = 0x20,
};

enum FormatLayout
{
	FORMAT_LAYOUT_UNDEFINED = 0x0,
	FORMAT_LAYOUT_LINEAR = 0x1,
	FORMAT_LAYOUT_PACKED = 0x2,
	FORMAT_LAYOUT_COMPRESSED = 0x4,
};

struct FormatComponentOffset final
{
	union
	{
		struct
		{
			int32_t red : 8;
			int32_t green : 8;
			int32_t blue : 8;
			int32_t alpha : 8;
		};
		struct
		{
			int32_t depth : 8;
			int32_t stencil : 8;
		};
	};
};

struct FormatDesc final
{
	// specific format name.
	const char* name;

	// The texel data type, e.g. UNORM, SNORM, UINT, etc.
	FormatDataType dataType;

	// The format aspect, i.e. color, depth, stencil, or depth-stencil.
	FormatAspectBit aspect;

	// The number of bytes per texel.
	// For compressed formats, this field is the size of a block.
	uint8_t bytesPerTexel;

	// The size in texels of the smallest supported size.
	// For compressed textures, that's the block size.
	// For uncompressed textures, the value is 1 (a pixel).
	uint8_t blockWidth;

	// The number of bytes per component (channel).
	// In case of combined depth-stencil formats, this is the size of the depth component only.
	// In case of packed or compressed formats, this field is invalid and will be set to -1.
	int8_t bytesPerComponent;

	// The layout of the format (linear, packed, or compressed).
	FormatLayout layout;

	// The components (channels) represented by the format, e.g. RGBA, depth-stencil, or a subset of those.
	FormatComponentBit componentBits;

	// The offset, in bytes, of each component within the texel.
	// In case of packed or compressed formats, this field is invalid and the offsets will be set to -1.
	FormatComponentOffset componentOffset;
};

// Gets a description of the given /b format.
const FormatDesc* GetFormatDescription(Format format);

const char* ToString(Format format);

#pragma endregion

#pragma region Helper

struct BufferUsageFlags final
{
	union
	{
		struct
		{
			bool transferSrc : 1;
			bool transferDst : 1;
			bool uniformTexelBuffer : 1;
			bool storageTexelBuffer : 1;
			bool uniformBuffer : 1;
			bool rawStorageBuffer : 1;
			bool roStructuredBuffer : 1;
			bool rwStructuredBuffer : 1;
			bool indexBuffer : 1;
			bool vertexBuffer : 1;
			bool indirectBuffer : 1;
			bool conditionalRendering : 1;
			bool rayTracing : 1;
			bool transformFeedbackBuffer : 1;
			bool transformFeedbackCounterBuffer : 1;
			bool shaderDeviceAddress : 1;
		} bits;
		uint32_t flags;
	};

	BufferUsageFlags()
		: flags(0) {}

	BufferUsageFlags(uint32_t _flags)
		: flags(_flags) {}

	BufferUsageFlags& operator=(uint32_t rhs)
	{
		this->flags = rhs;
		return *this;
	}

	operator uint32_t() const
	{
		return flags;
	}
};

struct ColorComponentFlags final
{
	union
	{
		struct
		{
			bool r : 1;
			bool g : 1;
			bool b : 1;
			bool a : 1;
		} bits;
		uint32_t flags;
	};

	ColorComponentFlags()
		: flags(0) {}

	ColorComponentFlags(uint32_t _flags)
		: flags(_flags) {}

	ColorComponentFlags& operator=(uint32_t rhs)
	{
		this->flags = rhs;
		return *this;
	}

	operator uint32_t() const
	{
		return flags;
	}

	static ColorComponentFlags RGBA();
};

struct DescriptorBindingFlags final
{
	union
	{
		struct
		{
			bool updatable : 1;
			bool partiallyBound : 1;
		} bits;
		uint32_t flags;
	};

	DescriptorBindingFlags()
		: flags(0) {}

	DescriptorBindingFlags(uint32_t flags_)
		: flags(flags_) {}

	DescriptorBindingFlags& operator=(uint32_t rhs)
	{
		this->flags = rhs;
		return *this;
	}

	operator uint32_t() const
	{
		return flags;
	}
};

struct DescriptorSetLayoutFlags final
{
	union
	{
		struct
		{
			bool pushable : 1;

		} bits;
		uint32_t flags;
	};

	DescriptorSetLayoutFlags()
		: flags(0) {}

	DescriptorSetLayoutFlags(uint32_t flags_)
		: flags(flags_) {}

	DescriptorSetLayoutFlags& operator=(uint32_t rhs)
	{
		this->flags = rhs;
		return *this;
	}

	operator uint32_t() const
	{
		return flags;
	}
};

struct DrawPassClearFlags final
{
	union
	{
		struct
		{
			bool clearRenderTargets : 1;
			bool clearDepth : 1;
			bool clearStencil : 1;

		} bits;
		uint32_t flags;
	};

	DrawPassClearFlags()
		: flags(0) {}

	DrawPassClearFlags(uint32_t flags_)
		: flags(flags_) {}

	DrawPassClearFlags& operator=(uint32_t rhs)
	{
		this->flags = rhs;
		return *this;
	}

	operator uint32_t() const
	{
		return flags;
	}
};

struct BeginRenderingFlags final
{
	union
	{
		struct
		{
			bool suspending : 1;
			bool resuming : 1;
		} bits;
		uint32_t flags;
	};

	BeginRenderingFlags()
		: flags(0) {}

	BeginRenderingFlags(uint32_t flags_)
		: flags(flags_) {}

	BeginRenderingFlags& operator=(uint32_t rhs)
	{
		this->flags = rhs;
		return *this;
	}

	operator uint32_t() const
	{
		return flags;
	}
};

struct ImageUsageFlags final
{
	union
	{
		struct
		{
			bool transferSrc : 1;
			bool transferDst : 1;
			bool sampled : 1;
			bool storage : 1;
			bool colorAttachment : 1;
			bool depthStencilAttachment : 1;
			bool transientAttachment : 1;
			bool inputAttachment : 1;
			bool fragmentDensityMap : 1;
			bool fragmentShadingRateAttachment : 1;
		} bits;
		uint32_t flags;
	};

	ImageUsageFlags()
		: flags(0) {}

	ImageUsageFlags(uint32_t flags_)
		: flags(flags_) {}

	ImageUsageFlags& operator=(uint32_t rhs)
	{
		this->flags = rhs;
		return *this;
	}

	ImageUsageFlags& operator|=(const ImageUsageFlags& rhs)
	{
		this->flags |= rhs.flags;
		return *this;
	}

	ImageUsageFlags& operator|=(uint32_t rhs)
	{
		this->flags |= rhs;
		return *this;
	}

	operator uint32_t() const
	{
		return flags;
	}

	static ImageUsageFlags SampledImage();
};

struct ImageCreateFlags final
{
	union
	{
		struct
		{
			bool subsampledFormat : 1;
		} bits;
		uint32_t flags;
	};

	ImageCreateFlags()
		: flags(0) {}

	ImageCreateFlags(uint32_t flags_)
		: flags(flags_) {}

	ImageCreateFlags& operator=(uint32_t rhs)
	{
		this->flags = rhs;
		return *this;
	}

	ImageCreateFlags& operator|=(const ImageCreateFlags& rhs)
	{
		this->flags |= rhs.flags;
		return *this;
	}

	ImageCreateFlags& operator|=(uint32_t rhs)
	{
		this->flags |= rhs;
		return *this;
	}

	operator uint32_t() const
	{
		return flags;
	}
};

struct SamplerCreateFlags final
{
	union
	{
		struct
		{
			bool subsampledFormat : 1;
		} bits;
		uint32_t flags;
	};

	SamplerCreateFlags()
		: flags(0) {}

	SamplerCreateFlags(uint32_t flags_)
		: flags(flags_) {}

	SamplerCreateFlags& operator=(uint32_t rhs)
	{
		this->flags = rhs;
		return *this;
	}

	SamplerCreateFlags& operator|=(const SamplerCreateFlags& rhs)
	{
		this->flags |= rhs.flags;
		return *this;
	}

	SamplerCreateFlags& operator|=(uint32_t rhs)
	{
		this->flags |= rhs;
		return *this;
	}

	operator uint32_t() const
	{
		return flags;
	}
};

struct Range final
{
	uint32_t start = 0;
	uint32_t end = 0;
};

struct ShaderStageFlags final
{
	union
	{
		struct
		{
			bool VS : 1;
			bool HS : 1;
			bool DS : 1;
			bool GS : 1;
			bool PS : 1;
			bool CS : 1;

		} bits;
		uint32_t flags;
	};

	ShaderStageFlags()
		: flags(0) {}

	ShaderStageFlags(uint32_t _flags)
		: flags(_flags) {}

	ShaderStageFlags& operator=(uint32_t rhs)
	{
		this->flags = rhs;
		return *this;
	}

	operator uint32_t() const
	{
		return flags;
	}

	static ShaderStageFlags SampledImage();
};

struct VertexAttribute final
{
	std::string     semanticName = "";                    // Semantic name (no effect in Vulkan currently)
	uint32_t        location = 0;                         // @TODO: Find a way to handle between DX and VK
	Format          format = FORMAT_UNDEFINED;
	uint32_t        binding = 0;                          // Valid range is [0, 15]
	uint32_t        offset = APPEND_OFFSET_ALIGNED;       // Use APPEND_OFFSET_ALIGNED to auto calculate offsets
	VertexInputRate inputRate = VERTEX_INPUT_RATE_VERTEX;
	VertexSemantic  semantic = VERTEX_SEMANTIC_UNDEFINED;
};

struct MultiViewState final
{
	uint32_t viewMask = 0;
	uint32_t correlationMask = 0;
};

// Storage class for binding number, vertex data stride, and vertex attributes for a vertex buffer binding.
// ** WARNING **
// Adding an attribute updates the stride information based on the current set of attributes.
// If a custom stride is required, add all the attributes first then call VertexBinding::SetStride() to set the stride.
class VertexBinding final
{
public:
	VertexBinding() {}
	VertexBinding(uint32_t binding, VertexInputRate inputRate)
		: m_binding(binding), m_inputRate(inputRate) {}

	VertexBinding(const VertexAttribute& attribute)
		: m_binding(attribute.binding), m_inputRate(attribute.inputRate)
	{
		AppendAttribute(attribute);
	}

	uint32_t        GetBinding() const { return m_binding; }
	void            SetBinding(uint32_t binding);
	const uint32_t& GetStride() const { return m_stride; } // Return a reference to member var so apps can take address of it
	void            SetStride(uint32_t stride);
	VertexInputRate GetInputRate() const { return m_inputRate; }
	uint32_t        GetAttributeCount() const { return static_cast<uint32_t>(m_attributes.size()); }
	Result          GetAttribute(uint32_t index, const VertexAttribute** ppAttribute) const;
	uint32_t        GetAttributeIndex(VertexSemantic semantic) const;
	VertexBinding&  AppendAttribute(const VertexAttribute& attribute);

	VertexBinding& operator+=(const VertexAttribute& rhs);

private:
	static const VertexInputRate kInvalidVertexInputRate = static_cast<VertexInputRate>(~0);

	uint32_t                     m_binding = 0;
	uint32_t                     m_stride = 0;
	VertexInputRate              m_inputRate = kInvalidVertexInputRate;
	std::vector<VertexAttribute> m_attributes;
};

class VertexDescription final
{
public:
	uint32_t             GetBindingCount() const { return CountU32(m_bindings); }
	Result               GetBinding(uint32_t index, const VertexBinding** ppBinding) const;
	const VertexBinding* GetBinding(uint32_t index) const;
	uint32_t             GetBindingIndex(uint32_t binding) const;
	Result               AppendBinding(const VertexBinding& binding);

private:
	std::vector<VertexBinding> m_bindings;
};

#pragma endregion

#pragma region Utils

std::string ToString(DescriptorType value);
std::string ToString(VertexSemantic value);
std::string ToString(IndexType value);

uint32_t    IndexTypeSize(IndexType value);
Format      VertexSemanticFormat(VertexSemantic value);

std::string ToString(const gli::target& target);
std::string ToString(const gli::format& format);

#pragma endregion

#pragma region Core Struct



struct Extent2D final
{
	uint32_t width;
	uint32_t height;
};

struct ComponentMapping final
{
	ComponentSwizzle r = COMPONENT_SWIZZLE_IDENTITY;
	ComponentSwizzle g = COMPONENT_SWIZZLE_IDENTITY;
	ComponentSwizzle b = COMPONENT_SWIZZLE_IDENTITY;
	ComponentSwizzle a = COMPONENT_SWIZZLE_IDENTITY;
};

struct DepthStencilClearValue final
{
	float    depth = 0;
	uint32_t stencil = 0;
};

union RenderTargetClearValue final
{
	struct
	{
		float r;
		float g;
		float b;
		float a;
	};
	float rgba[4];
};

struct Rect final
{
	Rect() = default;
	Rect(int32_t x_, int32_t y_, uint32_t width_, uint32_t height_) : x(x_), y(y_), width(width_), height(height_) {}

	int32_t  x = 0;
	int32_t  y = 0;
	uint32_t width = 0;
	uint32_t height = 0;
};

struct Viewport final
{
	Viewport() = default;
	Viewport(float x_, float y_, float width_, float height_, float minDepth_ = 0, float maxDepth_ = 1)
		: x(x_), y(y_), width(width_), height(height_), minDepth(minDepth_), maxDepth(maxDepth_) {}

	float x;
	float y;
	float width;
	float height;
	float minDepth;
	float maxDepth;
};

//! @enum Ownership
//!
//! The purpose of this enum is to help grfx objects manage the lifetime
//! of their member objects. All grfx objects are created with ownership
//! set to OWNERSHIP_REFERENCE. This means that the object lifetime is
//! left up to either Device or Instance unless the application
//! explicitly destroys it.
//!
//! If a member object's ownership is set to OWNERSHIP_EXCLUSIVE or
//! OWNERSHIP_RESTRICTED, this means that the containing object must
//! destroy it during the destruction process.
//!
//! If the containing object fails to destroy OWNERSHIP_EXCLUSIVE and
//! OWNERSHIP_RESTRICTED objects, then either Device or Instance
//! will destroy it in their destruction proces.
//!
//! If an object's ownership is set to OWNERSHIP_RESTRICTED then its
//! ownership cannot be changed. Calling SetOwnership() will have no effect.
//!
//! Examples of objects with OWNERSHIP_EXCLUSIVE ownership:
//!   - Draw passes and render passes have create infos where only the
//!     format of the render target and/or depth stencil are known.
//!     In these cases draw passes and render passes will create the
//!     necessary backing images and views. These objects will be created
//!     with ownership set to EXCLUSIVE. The render pass will destroy
//!     these objects when it itself is destroyed.
//!
//!   - Model's buffers and textures typically have OWNERSHIP_REFERENCE
//!     ownership. However, the application is free to change ownership
//!     to EXCLUSIVE as it sees fit.
enum Ownership
{
	OWNERSHIP_REFERENCE = 0,
	OWNERSHIP_EXCLUSIVE = 1,
	OWNERSHIP_RESTRICTED = 2,
};

class OwnershipTrait
{
public:
	Ownership GetOwnership() const { return m_ownership; }

	void SetOwnership(Ownership ownership)
	{
		// Cannot change to or from OWNERSHIP_RESTRICTED
		if ((ownership == OWNERSHIP_RESTRICTED) || (m_ownership == OWNERSHIP_RESTRICTED)) return;
		m_ownership = ownership;
	}

private:
	Ownership m_ownership = OWNERSHIP_REFERENCE;
};

template <typename CreatInfoT>
class CreateDestroyTraits : public OwnershipTrait
{
	friend class RenderDevice;
protected:
	virtual Result create(const CreatInfoT& createInfo)
	{
		// Copy create info
		m_createInfo = createInfo;
		// Create API objects
		Result ppxres = createApiObjects(createInfo);
		if (ppxres != SUCCESS)
		{
			destroyApiObjects();
			return ppxres;
		}
		// Success
		return SUCCESS;
	}

	virtual void destroy()
	{
		destroyApiObjects();
	}

	virtual Result createApiObjects(const CreatInfoT& createInfo) = 0;
	virtual void   destroyApiObjects() = 0;

	CreatInfoT m_createInfo = {};
};

class NamedObjectTrait
{
public:
	const std::string& GetName() const { return m_name; }
	void               SetName(const std::string& name) { m_name = name; }

private:
	std::string m_name;
};

template <typename CreatInfoT>
class DeviceObject : public CreateDestroyTraits<CreatInfoT>, public NamedObjectTrait
{
	friend class RenderDevice;
public:
	RenderDevice* GetDevice() const
	{
		return m_device;
	}

private:
	void setParent(RenderDevice* device)
	{
		m_device = device;
	}

	RenderDevice* m_device{ nullptr };
};

#pragma endregion

#pragma region Vk Utils

const uint32_t kAllQueueMask      = VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT | VK_QUEUE_TRANSFER_BIT;
const uint32_t kGraphicsQueueMask = VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT | VK_QUEUE_TRANSFER_BIT;
const uint32_t kComputeQueueMask  = VK_QUEUE_COMPUTE_BIT | VK_QUEUE_TRANSFER_BIT;
const uint32_t kTransferQueueMask = VK_QUEUE_TRANSFER_BIT;

std::string ToString(VkResult value);
std::string ToString(VkDescriptorType value);
std::string ToString(VkPresentModeKHR value);

VkAttachmentLoadOp            ToVkAttachmentLoadOp(AttachmentLoadOp value);
VkAttachmentStoreOp           ToVkAttachmentStoreOp(AttachmentStoreOp value);
VkBlendFactor                 ToVkBlendFactor(BlendFactor value);
VkBlendOp                     ToVkBlendOp(BlendOp value);
VkBorderColor                 ToVkBorderColor(BorderColor value);
VkBufferUsageFlags            ToVkBufferUsageFlags(const BufferUsageFlags& value);
VkChromaLocation              ToVkChromaLocation(ChromaLocation value);
VkClearColorValue             ToVkClearColorValue(const RenderTargetClearValue& value);
VkClearDepthStencilValue      ToVkClearDepthStencilValue(const DepthStencilClearValue& value);
VkColorComponentFlags         ToVkColorComponentFlags(const ColorComponentFlags& value);
VkCompareOp                   ToVkCompareOp(CompareOp value);
VkComponentSwizzle            ToVkComponentSwizzle(ComponentSwizzle value);
VkComponentMapping            ToVkComponentMapping(const ComponentMapping& value);
VkCullModeFlagBits            ToVkCullMode(CullMode value);
VkDescriptorBindingFlags      ToVkDescriptorBindingFlags(const DescriptorBindingFlags& value);
VkDescriptorType              ToVkDescriptorType(DescriptorType value);
VkFilter                      ToVkFilter(Filter value);
VkFormat                      ToVkFormat(Format value);
VkFrontFace                   ToVkFrontFace(FrontFace value);
VkImageType                   ToVkImageType(ImageType value);
VkImageUsageFlags             ToVkImageUsageFlags(const ImageUsageFlags& value);
VkImageViewType               ToVkImageViewType(ImageViewType value);
VkIndexType                   ToVkIndexType(IndexType value);
VkLogicOp                     ToVkLogicOp(LogicOp value);
VkPipelineStageFlagBits       ToVkPipelineStage(PipelineStage value);
VkPolygonMode                 ToVkPolygonMode(PolygonMode value);
VkPresentModeKHR              ToVkPresentMode(PresentMode value);
VkPrimitiveTopology           ToVkPrimitiveTopology(PrimitiveTopology value);
VkQueryType                   ToVkQueryType(QueryType value);
VkSamplerAddressMode          ToVkSamplerAddressMode(SamplerAddressMode value);
VkSamplerMipmapMode           ToVkSamplerMipmapMode(SamplerMipmapMode value);
VkSampleCountFlagBits         ToVkSampleCount(SampleCount value);
VkShaderStageFlags            ToVkShaderStageFlags(const ShaderStageFlags& value);
VkStencilOp                   ToVkStencilOp(StencilOp value);
VkTessellationDomainOrigin    ToVkTessellationDomainOrigin(TessellationDomainOrigin value);
VkVertexInputRate             ToVkVertexInputRate(VertexInputRate value);
VkSamplerYcbcrModelConversion ToVkYcbcrModelConversion(YcbcrModelConversion value);
VkSamplerYcbcrRange           ToVkYcbcrRange(YcbcrRange value);

Result ToVkBarrierSrc(
	ResourceState                   state,
	CommandType               commandType,
	const VkPhysicalDeviceFeatures& features,
	VkPipelineStageFlags& stageMask,
	VkAccessFlags& accessMask,
	VkImageLayout& layout);
Result ToVkBarrierDst(
	ResourceState                   state,
	CommandType               commandType,
	const VkPhysicalDeviceFeatures& features,
	VkPipelineStageFlags& stageMask,
	VkAccessFlags& accessMask,
	VkImageLayout& layout);

VkImageAspectFlags DetermineAspectMask(VkFormat format);

VmaMemoryUsage ToVmaMemoryUsage(MemoryUsage value);

// Inserts nextStruct into the pNext chain of baseStruct.
template <typename TVkStruct1, typename TVkStruct2>
void InsertPNext(TVkStruct1& baseStruct, TVkStruct2& nextStruct)
{
	nextStruct.pNext = baseStruct.pNext;
	baseStruct.pNext = &nextStruct;
}

#pragma endregion

#pragma region DeviceQueue

class DeviceQueue final
{
	friend class VulkanInstance;
public:
	VkQueue     Queue{ nullptr };
	uint32_t    QueueFamily{ 0 };
	CommandType CommandType{ COMMAND_TYPE_UNDEFINED };
private:
	bool init(vkb::Device& vkbDevice, vkb::QueueType type);
};
using DeviceQueuePtr = std::shared_ptr<DeviceQueue>;

#pragma endregion

#pragma region Bitmap

class Bitmap final
{
public:
	enum DataType
	{
		DATA_TYPE_UNDEFINED = 0,
		DATA_TYPE_UINT8,
		DATA_TYPE_UINT16,
		DATA_TYPE_UINT32,
		DATA_TYPE_FLOAT,
	};

	enum Format
	{
		FORMAT_UNDEFINED = 0,
		FORMAT_R_UINT8,
		FORMAT_RG_UINT8,
		FORMAT_RGB_UINT8,
		FORMAT_RGBA_UINT8,
		FORMAT_R_UINT16,
		FORMAT_RG_UINT16,
		FORMAT_RGB_UINT16,
		FORMAT_RGBA_UINT16,
		FORMAT_R_UINT32,
		FORMAT_RG_UINT32,
		FORMAT_RGB_UINT32,
		FORMAT_RGBA_UINT32,
		FORMAT_R_FLOAT,
		FORMAT_RG_FLOAT,
		FORMAT_RGB_FLOAT,
		FORMAT_RGBA_FLOAT,
	};

	Bitmap();
	Bitmap(const Bitmap& obj);
	~Bitmap();

	Bitmap& operator=(const Bitmap& rhs);

	//! Creates a bitmap with internal storage.
	static Result Create(uint32_t width, uint32_t height, Bitmap::Format format, Bitmap* pBitmap);
	//! Creates a bitmap with external storage. If \b rowStride is 0, default row stride for format is used.
	static Result Create(uint32_t width, uint32_t height, Bitmap::Format format, uint32_t rowStride, char* pExternalStorage, Bitmap* pBitmap);
	//! Creates a bitmap with external storage.
	static Result Create(uint32_t width, uint32_t height, Bitmap::Format format, char* pExternalStorage, Bitmap* pBitmap);
	//! Returns a bitmap with internal storage.
	static Bitmap Create(uint32_t width, uint32_t height, Bitmap::Format format, Result* pResult = nullptr);
	//! Returns a bitmap with external storage. If \b rowStride is 0, default row stride for format is used.
	static Bitmap Create(uint32_t width, uint32_t height, Bitmap::Format format, uint32_t rowStride, char* pExternalStorage, Result* pResult = nullptr);
	//! Returns a bitmap with external storage.
	static Bitmap Create(uint32_t width, uint32_t height, Bitmap::Format format, char* pExternalStorage, Result* pResult = nullptr);

	// Returns true if dimensions are greater than one, format is valid, and storage is valid
	bool IsOk() const;

	uint32_t       GetWidth() const { return mWidth; }
	uint32_t       GetHeight() const { return mHeight; }
	Bitmap::Format GetFormat() const { return mFormat; };
	uint32_t       GetChannelCount() const { return mChannelCount; }
	uint32_t       GetPixelStride() const { return mPixelStride; }
	uint32_t       GetRowStride() const { return mRowStride; }
	char* GetData() const { return mData; }
	uint64_t       GetFootprintSize(uint32_t rowStrideAlignment = 1) const;

	Result Resize(uint32_t width, uint32_t height);
	Result ScaleTo(Bitmap* pTargetBitmap) const;
	Result ScaleTo(Bitmap* pTargetBitmap, stbir_filter filterType) const;

	template <typename PixelDataType>
	void Fill(PixelDataType r, PixelDataType g, PixelDataType b, PixelDataType a);

	// Returns byte address of pixel at (x,y)
	char* GetPixelAddress(uint32_t x, uint32_t y);
	const char* GetPixelAddress(uint32_t x, uint32_t y) const;

	// These functions will return null if the Bitmap's format doesn't the function. For example, GetPixel8u() returns null if the format is FORMAT_RGBA_FLOAT.
	uint8_t* GetPixel8u(uint32_t x, uint32_t y);
	const uint8_t* GetPixel8u(uint32_t x, uint32_t y) const;
	uint16_t* GetPixel16u(uint32_t x, uint32_t y);
	const uint16_t* GetPixel16u(uint32_t x, uint32_t y) const;
	uint32_t* GetPixel32u(uint32_t x, uint32_t y);
	const uint32_t* GetPixel32u(uint32_t x, uint32_t y) const;
	float* GetPixel32f(uint32_t x, uint32_t y);
	const float* GetPixel32f(uint32_t x, uint32_t y) const;

	static uint32_t         ChannelSize(Bitmap::Format value);
	static uint32_t         ChannelCount(Bitmap::Format value);
	static Bitmap::DataType ChannelDataType(Bitmap::Format value);
	static uint32_t         FormatSize(Bitmap::Format value);
	static uint64_t         StorageFootprint(uint32_t width, uint32_t height, Bitmap::Format format);

	static Result GetFileProperties(const std::filesystem::path& path, uint32_t* pWidth, uint32_t* pHeight, Bitmap::Format* pFormat);
	static Result LoadFile(const std::filesystem::path& path, Bitmap* pBitmap);
	//static Result SaveFilePNG(const std::filesystem::path& path, const Bitmap* pBitmap);
	static bool   IsBitmapFile(const std::filesystem::path& path);

	static Result LoadFromMemory(const size_t dataSize, const void* pData, Bitmap* pBitmap);

	class PixelIterator
	{
	private:
		friend class Bitmap;

		PixelIterator(Bitmap* pBitmap)
			: mBitmap(pBitmap)
		{
			Reset();
		}

	public:
		void Reset()
		{
			mX = 0;
			mY = 0;
			mPixelAddress = mBitmap->GetPixelAddress(mX, mY);
		}

		bool Done() const
		{
			bool done = (mY >= mBitmap->GetHeight());
			return done;
		}

		bool Next()
		{
			if (Done()) {
				return false;
			}

			mX += 1;
			mPixelAddress += mBitmap->GetPixelStride();
			if (mX == mBitmap->GetWidth()) {
				mY += 1;
				mX = 0;
				mPixelAddress = mBitmap->GetPixelAddress(mX, mY);
			}

			return Done() ? false : true;
		}

		uint32_t       GetX() const { return mX; }
		uint32_t       GetY() const { return mY; }
		Bitmap::Format GetFormat() const { return mBitmap->GetFormat(); }
		uint32_t       GetChannelCount() const { return Bitmap::ChannelCount(GetFormat()); }

		template <typename T>
		T* GetPixelAddress() const { return reinterpret_cast<T*>(mPixelAddress); }

	private:
		Bitmap* mBitmap = nullptr;
		uint32_t mX = 0;
		uint32_t mY = 0;
		char* mPixelAddress = nullptr;
	};

	PixelIterator GetPixelIterator() { return PixelIterator(this); }

private:
	void   InternalCtor();
	Result InternalInitialize(uint32_t width, uint32_t height, Bitmap::Format format, uint32_t rowStride, char* pExternalStorage);
	Result InternalCopy(const Bitmap& obj);
	void   FreeStbiDataIfNeeded();

	// Stbi-specific functions/wrappers.
	// These arguments generally mirror those for stbi_load, except format which is used to determine whether the call should be made to stbi_load or stbi_loadf (that is, whether the file to be read is in integer or floating point format).
	static char* StbiLoad(const std::filesystem::path& path, Bitmap::Format format, int* pWidth, int* pHeight, int* pChannels, int desiredChannels);
	// These arugments mirror those for stbi_info.
	static Result StbiInfo(const std::filesystem::path& path, int* pX, int* pY, int* pComp);

private:
	uint32_t          mWidth = 0;
	uint32_t          mHeight = 0;
	Bitmap::Format    mFormat = Bitmap::FORMAT_UNDEFINED;
	uint32_t          mChannelCount = 0;
	uint32_t          mPixelStride = 0;
	uint32_t          mRowStride = 0;
	char* mData = nullptr;
	bool              mDataIsFromStbi = false;
	std::vector<char> mInternalStorage = {};
};

template <typename PixelDataType>
void Bitmap::Fill(PixelDataType r, PixelDataType g, PixelDataType b, PixelDataType a)
{
	ASSERT_MSG(mData != nullptr, "data is null");
	ASSERT_MSG(mFormat != Bitmap::FORMAT_UNDEFINED, "format is undefined");

	PixelDataType rgba[4] = { r, g, b, a };

	const uint32_t channelCount = Bitmap::ChannelCount(mFormat);

	char* pData = mData;
	for (uint32_t y = 0; y < mHeight; ++y)
	{
		for (uint32_t x = 0; x < mWidth; ++x)
		{
			PixelDataType* pPixelData = reinterpret_cast<PixelDataType*>(pData);
			for (uint32_t c = 0; c < channelCount; ++c)
			{
				pPixelData[c] = rgba[c];
			}
			pData += mPixelStride;
		}
	}
}

#pragma endregion

#pragma region Font

struct FontMetrics
{
	float ascent = 0;
	float descent = 0;
	float lineGap = 0;
};

struct GlyphBox
{
	int32_t x0 = 0;
	int32_t y0 = 0;
	int32_t x1 = 0;
	int32_t y1 = 0;
};

struct GlyphMetrics
{
	float    advance = 0;
	float    leftBearing = 0;
	GlyphBox box = {};
};

class Font
{
public:
	Font();
	virtual ~Font();

	static Result CreateFromFile(const std::filesystem::path& path, Font* pFont);
	static Result CreateFromMemory(size_t size, const char* pData, Font* pFont);

	float GetScale(float fontSizeInPixels) const;

	void GetFontMetrics(float fontSizeInPixels, FontMetrics* pMetrics) const;

	void GetGlyphMetrics(
		float         fontSizeInPixels,
		uint32_t      codepoint,
		float         subpixelShiftX,
		float         subpixelShiftY,
		GlyphMetrics* pMetrics) const;

	void RenderGlyphBitmap(
		float          fontSizeInPixels,
		uint32_t       codepoint,
		float          subpixelShiftX,
		float          subpixelShiftY,
		uint32_t       glyphWidth,
		uint32_t       glyphHeight,
		uint32_t       rowStride,
		unsigned char* pOutput) const;

private:
	void AcquireFontMetrics();

private:
	struct Object
	{
		std::vector<unsigned char> fontData;
		stbtt_fontinfo             fontInfo;
		int                        ascent = 0;
		int                        descent = 0;
		int                        lineGap = 0;
	};
	std::shared_ptr<Object> mObject;
};

#pragma endregion

#pragma region TriMesh

enum TriMeshAttributeDim
{
	TRI_MESH_ATTRIBUTE_DIM_UNDEFINED = 0,
	TRI_MESH_ATTRIBUTE_DIM_2 = 2,
	TRI_MESH_ATTRIBUTE_DIM_3 = 3,
	TRI_MESH_ATTRIBUTE_DIM_4 = 4,
};

enum TriMeshPlane
{
	TRI_MESH_PLANE_POSITIVE_X = 0,
	TRI_MESH_PLANE_NEGATIVE_X = 1,
	TRI_MESH_PLANE_POSITIVE_Y = 2,
	TRI_MESH_PLANE_NEGATIVE_Y = 3,
	TRI_MESH_PLANE_POSITIVE_Z = 4,
	TRI_MESH_PLANE_NEGATIVE_Z = 5,
};

struct TriMeshVertexData
{
	float3 position;
	float3 color;
	float3 normal;
	float2 texCoord;
	float4 tangent;
	float3 bitangent;
};

struct TriMeshVertexDataCompressed
{
	half4  position;
	half3  color;
	i8vec4 normal;
	half2  texCoord;
	i8vec4 tangent;
	i8vec3 bitangent;
};

class TriMeshOptions
{
public:
	TriMeshOptions() {}
	~TriMeshOptions() {}
	// clang-format off
	//! Enable/disable indices
	TriMeshOptions& Indices(bool value = true) { mEnableIndices = value; return *this; }
	//! Enable/disable vertex colors
	TriMeshOptions& VertexColors(bool value = true) { mEnableVertexColors = value; return *this; }
	//! Enable/disable normals
	TriMeshOptions& Normals(bool value = true) { mEnableNormals = value; return *this; }
	//! Enable/disable texture coordinates, most geometry will have 2-dimensional texture coordinates
	TriMeshOptions& TexCoords(bool value = true) { mEnableTexCoords = value; return *this; }
	//! Enable/disable tangent and bitangent creation flag
	TriMeshOptions& Tangents(bool value = true) { mEnableTangents = value; return *this; }
	//! Set and/or enable/disable object color, object color will override vertex colors
	TriMeshOptions& ObjectColor(const float3& color, bool enable = true) { mObjectColor = color; mEnableObjectColor = enable; return *this; }
	//! Set the translate of geometry position, default is (0, 0, 0)
	TriMeshOptions& Translate(const float3& translate) { mTranslate = translate; return *this; }
	//! Set the scale of geometry position, default is (1, 1, 1)
	TriMeshOptions& Scale(const float3& scale) { mScale = scale; return *this; }
	//! Sets the UV texture coordinate scale, default is (1, 1)
	TriMeshOptions& TexCoordScale(const float2& scale) { mTexCoordScale = scale; return *this; }
	//! Enable all attributes
	TriMeshOptions& AllAttributes() { mEnableVertexColors = true; mEnableNormals = true; mEnableTexCoords = true; mEnableTangents = true; return *this; }
	//! Inverts tex coords vertically
	TriMeshOptions& InvertTexCoordsV() { mInvertTexCoordsV = true; return *this; }
	//! Inverts winding order of ONLY indices
	TriMeshOptions& InvertWinding() { mInvertWinding = true; return *this; }
	// clang-format on
private:
	bool   mEnableIndices = false;
	bool   mEnableVertexColors = false;
	bool   mEnableNormals = false;
	bool   mEnableTexCoords = false;
	bool   mEnableTangents = false;
	bool   mEnableObjectColor = false;
	bool   mInvertTexCoordsV = false;
	bool   mInvertWinding = false;
	float3 mObjectColor = float3(0.7f);
	float3 mTranslate = float3(0, 0, 0);
	float3 mScale = float3(1, 1, 1);
	float2 mTexCoordScale = float2(1, 1);
	friend class TriMesh;
};

class TriMesh
{
public:
	TriMesh();
	TriMesh(IndexType indexType);
	TriMesh(TriMeshAttributeDim texCoordDim);
	TriMesh(IndexType indexType, TriMeshAttributeDim texCoordDim);
	~TriMesh();

	IndexType     GetIndexType() const { return mIndexType; }
	TriMeshAttributeDim GetTexCoordDim() const { return mTexCoordDim; }

	bool HasColors() const { return GetCountColors() > 0; }
	bool HasNormals() const { return GetCountNormals() > 0; }
	bool HasTexCoords() const { return GetCountTexCoords() > 0; }
	bool HasTangents() const { return GetCountTangents() > 0; }
	bool HasBitangents() const { return GetCountBitangents() > 0; }

	uint32_t GetCountTriangles() const;
	uint32_t GetCountIndices() const;
	uint32_t GetCountPositions() const;
	uint32_t GetCountColors() const;
	uint32_t GetCountNormals() const;
	uint32_t GetCountTexCoords() const;
	uint32_t GetCountTangents() const;
	uint32_t GetCountBitangents() const;

	uint64_t GetDataSizeIndices() const;
	uint64_t GetDataSizePositions() const;
	uint64_t GetDataSizeColors() const;
	uint64_t GetDataSizeNormalls() const;
	uint64_t GetDataSizeTexCoords() const;
	uint64_t GetDataSizeTangents() const;
	uint64_t GetDataSizeBitangents() const;

	const uint16_t* GetDataIndicesU16(uint32_t index = 0) const;
	const uint32_t* GetDataIndicesU32(uint32_t index = 0) const;
	const float3* GetDataPositions(uint32_t index = 0) const;
	const float3* GetDataColors(uint32_t index = 0) const;
	const float3* GetDataNormalls(uint32_t index = 0) const;
	const float2* GetDataTexCoords2(uint32_t index = 0) const;
	const float3* GetDataTexCoords3(uint32_t index = 0) const;
	const float4* GetDataTexCoords4(uint32_t index = 0) const;
	const float4* GetDataTangents(uint32_t index = 0) const;
	const float3* GetDataBitangents(uint32_t index = 0) const;

	const float3& GetBoundingBoxMin() const { return mBoundingBoxMin; }
	const float3& GetBoundingBoxMax() const { return mBoundingBoxMax; }

	// Preallocates triangle, position, color, normal, texture and tangent data (as desired) based on the provided triangle count.
	// Using this avoids doing those allocations (potentially multiple times) during the data load.
	void     PreallocateForTriangleCount(size_t triangleCount, bool enableColors, bool enableNormals, bool enableTexCoords, bool enableTangents);
	uint32_t AppendTriangle(uint32_t v0, uint32_t v1, uint32_t v2);
	uint32_t AppendPosition(const float3& value);
	uint32_t AppendColor(const float3& value);
	uint32_t AppendTexCoord(const float2& value);
	uint32_t AppendTexCoord(const float3& value);
	uint32_t AppendTexCoord(const float4& value);
	uint32_t AppendNormal(const float3& value);
	uint32_t AppendTangent(const float4& value);
	uint32_t AppendBitangent(const float3& value);

	Result GetTriangle(uint32_t triIndex, uint32_t& v0, uint32_t& v1, uint32_t& v2) const;
	Result GetVertexData(uint32_t vtxIndex, TriMeshVertexData* pVertexData) const;

	static TriMesh CreatePlane(TriMeshPlane plane, const float2& size, uint32_t usegs, uint32_t vsegs, const TriMeshOptions& options = TriMeshOptions());
	static TriMesh CreateCube(const float3& size, const TriMeshOptions& options = TriMeshOptions());
	static TriMesh CreateSphere(float radius, uint32_t usegs, uint32_t vsegs, const TriMeshOptions& options = TriMeshOptions());

	static Result  CreateFromOBJ(const std::filesystem::path& path, const TriMeshOptions& options, TriMesh* pTriMesh);
	static TriMesh CreateFromOBJ(const std::filesystem::path& path, const TriMeshOptions& options = TriMeshOptions());

private:
	void AppendIndexU16(uint16_t value);
	void AppendIndexU32(uint32_t value);

	static void AppendIndexAndVertexData(
		std::vector<uint32_t>& indexData,
		const std::vector<float>& vertexData,
		const uint32_t            expectedVertexCount,
		const TriMeshOptions& options,
		TriMesh& mesh);

private:
	IndexType      mIndexType = INDEX_TYPE_UNDEFINED;
	TriMeshAttributeDim  mTexCoordDim = TRI_MESH_ATTRIBUTE_DIM_UNDEFINED;
	std::vector<uint8_t> mIndices;        // Stores both 16 and 32 bit indices
	std::vector<float3>  mPositions;      // Vertex positions
	std::vector<float3>  mColors;         // Vertex colors
	std::vector<float3>  mNormals;        // Vertex normals
	std::vector<float>   mTexCoords;      // Vertex texcoords, dimension can be 2, 3, or 4
	std::vector<float4>  mTangents;       // Vertex tangents
	std::vector<float3>  mBitangents;     // Vertex bitangents
	float3               mBoundingBoxMin; // Bounding box min
	float3               mBoundingBoxMax; // Bounding box max
};

#pragma endregion

#pragma region WireMesh

enum WireMeshPlane
{
	WIRE_MESH_PLANE_POSITIVE_X = 0,
	WIRE_MESH_PLANE_NEGATIVE_X = 1,
	WIRE_MESH_PLANE_POSITIVE_Y = 2,
	WIRE_MESH_PLANE_NEGATIVE_Y = 3,
	WIRE_MESH_PLANE_POSITIVE_Z = 4,
	WIRE_MESH_PLANE_NEGATIVE_Z = 5,
};

struct WireMeshVertexData
{
	float3 position;
	float3 color;
};

class WireMeshOptions
{
public:
	WireMeshOptions() {}
	~WireMeshOptions() {}
	//! Enable/disable indices
	WireMeshOptions& Indices(bool value = true) { mEnableIndices = value; return *this; }
	//! Enable/disable vertex colors
	WireMeshOptions& VertexColors(bool value = true) { mEnableVertexColors = value; return *this; }
	//! Set and/or enable/disable object color, object color will override vertex colors
	WireMeshOptions& ObjectColor(const float3& color, bool enable = true) { mObjectColor = color; mEnableObjectColor = enable; return *this; }
	//! Set the scale of geometry position, default is (1, 1, 1)
	WireMeshOptions& Scale(const float3& scale) { mScale = scale; return *this; }

private:
	bool   mEnableIndices = false;
	bool   mEnableVertexColors = false;
	bool   mEnableObjectColor = false;
	float3 mObjectColor = float3(0.7f);
	float3 mScale = float3(1, 1, 1);
	friend class WireMesh;
};

class WireMesh
{
public:
	WireMesh();
	WireMesh(IndexType indexType);
	~WireMesh();

	IndexType GetIndexType() const { return mIndexType; }

	bool HasColors() const { return GetCountColors() > 0; }

	uint32_t GetCountEdges() const;
	uint32_t GetCountIndices() const;
	uint32_t GetCountPositions() const;
	uint32_t GetCountColors() const;

	uint64_t GetDataSizeIndices() const;
	uint64_t GetDataSizePositions() const;
	uint64_t GetDataSizeColors() const;

	const uint16_t* GetDataIndicesU16(uint32_t index = 0) const;
	const uint32_t* GetDataIndicesU32(uint32_t index = 0) const;
	const float3* GetDataPositions(uint32_t index = 0) const;
	const float3* GetDataColors(uint32_t index = 0) const;

	const float3& GetBoundingBoxMin() const { return mBoundingBoxMin; }
	const float3& GetBoundingBoxMax() const { return mBoundingBoxMax; }

	uint32_t AppendEdge(uint32_t v0, uint32_t v1);
	uint32_t AppendPosition(const float3& value);
	uint32_t AppendColor(const float3& value);

	Result GetEdge(uint32_t lineIndex, uint32_t& v0, uint32_t& v1) const;
	Result GetVertexData(uint32_t vtxIndex, WireMeshVertexData* pVertexData) const;

	static WireMesh CreatePlane(WireMeshPlane plane, const float2& size, uint32_t usegs, uint32_t vsegs, const WireMeshOptions& options = WireMeshOptions());
	static WireMesh CreateCube(const float3& size, const WireMeshOptions& options = WireMeshOptions());
	static WireMesh CreateSphere(float radius, uint32_t usegs, uint32_t vsegs, const WireMeshOptions& options = WireMeshOptions());

private:
	void AppendIndexU16(uint16_t value);
	void AppendIndexU32(uint32_t value);

	static void AppendIndexAndVertexData(
		std::vector<uint32_t>& indexData,
		const std::vector<float>& vertexData,
		const uint32_t            expectedVertexCount,
		const WireMeshOptions& options,
		WireMesh& mesh);

private:
	IndexType      mIndexType = INDEX_TYPE_UNDEFINED;
	std::vector<uint8_t> mIndices;        // Stores both 16 and 32 bit indices
	std::vector<float3>  mPositions;      // Vertex positions
	std::vector<float3>  mColors;         // Vertex colors
	float3               mBoundingBoxMin; // Bounding box min
	float3               mBoundingBoxMax; // Bounding box max
};

#pragma endregion

#pragma region Geometry

template <typename T>
class VertexDataProcessorBase;

enum GeometryVertexAttributeLayout
{
	GEOMETRY_VERTEX_ATTRIBUTE_LAYOUT_INTERLEAVED = 1,
	GEOMETRY_VERTEX_ATTRIBUTE_LAYOUT_PLANAR = 2,
	GEOMETRY_VERTEX_ATTRIBUTE_LAYOUT_POSITION_PLANAR = 3,
};

//! @struct GeometryCreateInfo
//!
//! primitiveTopology
//!   - only TRIANGLE_LIST is currently supported
//!
//! indexType
//!   - use UNDEFINED to indicate there's not index data
//!
//! attributeLayout
//!   - if INTERLEAVED only bindings[0] is used
//!   - if PLANAR bindings[0..bindingCount] are used
//!   - if PLANAR bindings can only have 1 attribute
//!
//! Supported vertex semantics:
//!    VERTEX_SEMANTIC_POSITION
//!    VERTEX_SEMANTIC_NORMAL
//!    VERTEX_SEMANTIC_COLOR
//!    VERTEX_SEMANTIC_TANGENT
//!    VERTEX_SEMANTIC_BITANGEN
//!    VERTEX_SEMANTIC_TEXCOORD
//!
struct GeometryCreateInfo
{
	IndexType               indexType = INDEX_TYPE_UNDEFINED;
	GeometryVertexAttributeLayout vertexAttributeLayout = GEOMETRY_VERTEX_ATTRIBUTE_LAYOUT_INTERLEAVED;
	uint32_t                      vertexBindingCount = 0;
	VertexBinding           vertexBindings[MAX_VERTEX_BINDINGS] = {};
	PrimitiveTopology       primitiveTopology = PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

	// Creates a create info objects with UINT8, UINT16 or UINT32 index type and position vertex attribute.
	static GeometryCreateInfo InterleavedU8(Format format = FORMAT_R32G32B32_FLOAT);
	static GeometryCreateInfo InterleavedU16(Format format = FORMAT_R32G32B32_FLOAT);
	static GeometryCreateInfo InterleavedU32(Format format = FORMAT_R32G32B32_FLOAT);
	static GeometryCreateInfo PlanarU8(Format format = FORMAT_R32G32B32_FLOAT);
	static GeometryCreateInfo PlanarU16(Format format = FORMAT_R32G32B32_FLOAT);
	static GeometryCreateInfo PlanarU32(Format format = FORMAT_R32G32B32_FLOAT);
	static GeometryCreateInfo PositionPlanarU8(Format format = FORMAT_R32G32B32_FLOAT);
	static GeometryCreateInfo PositionPlanarU16(Format format = FORMAT_R32G32B32_FLOAT);
	static GeometryCreateInfo PositionPlanarU32(Format format = FORMAT_R32G32B32_FLOAT);

	// Create a create info with a position vertex attribute.
	//
	static GeometryCreateInfo Interleaved();
	static GeometryCreateInfo Planar();
	static GeometryCreateInfo PositionPlanar();

	GeometryCreateInfo& IndexType(vkr::IndexType indexType_);
	GeometryCreateInfo& IndexTypeU8();
	GeometryCreateInfo& IndexTypeU16();
	GeometryCreateInfo& IndexTypeU32();

	// NOTE: Vertex input locations (Vulkan) are based on the order of
	//       when the attribute is added.
	//
	//       Example (assumes no attributes exist):
	//         AddPosition(); // location = 0
	//         AddColor();    // location = 1
	//
	//       Example (2 attributes exist):
	//         AddTexCoord(); // location = 2
	//         AddTangent();  // location = 3
	//
	// WARNING: Changing the attribute layout, index type, or messing
	//          with the vertex bindings after or in between calling
	//          these functions can result in undefined behavior.
	//
	GeometryCreateInfo& AddPosition(Format format = FORMAT_R32G32B32_FLOAT);
	GeometryCreateInfo& AddNormal(Format format = FORMAT_R32G32B32_FLOAT);
	GeometryCreateInfo& AddColor(Format format = FORMAT_R32G32B32_FLOAT);
	GeometryCreateInfo& AddTexCoord(Format format = FORMAT_R32G32_FLOAT);
	GeometryCreateInfo& AddTangent(Format format = FORMAT_R32G32B32A32_FLOAT);
	GeometryCreateInfo& AddBitangent(Format format = FORMAT_R32G32B32_FLOAT);

private:
	GeometryCreateInfo& AddAttribute(VertexSemantic semantic, Format format);
};

//! @class Geometry
//!
//! Implementation Notes:
//!   - Recommended to avoid modifying the index/vertex buffers directly and to use
//!     the Append* functions instead (for smaller geometries especially)
//!
class Geometry
{
	template <typename T>
	friend class VertexDataProcessorBase;
	enum BufferType
	{
		BUFFER_TYPE_VERTEX = 1,
		BUFFER_TYPE_INDEX = 2,
	};

public:
	//! @class Buffer
	//!
	//! Element count is data size divided by element size.
	//! Example:
	//!   - If  buffer is storing 16 bit indices then element size
	//!     is 2 bytes. Dividing the value of GetSize() by 2 will
	//!     return the element count, i.e. the number of indices.
	//!
	//! There's two different ways to use Buffer. Mixing them will most
	//! likely lead to undefined behavior.
	//!
	//! Use #1:
	//!   - Create a buffer object with type and element size
	//!   - Call SetSize() to allocate storage
	//!   - Grab the pointer with GetData() and memcpy to buffer object
	//!
	//! Use #2:
	//!   - Create a buffer object with type and element size
	//!   - Call Append<T>() to append data to it
	//!
	class Buffer
	{
	public:
		Buffer() {}
		Buffer(BufferType type, uint32_t elementSize)
			: mType(type), mElementSize(elementSize) {}
		~Buffer() {}

		BufferType  GetType() const { return mType; }
		uint32_t    GetElementSize() const { return mElementSize; }
		uint32_t    GetSize() const { return static_cast<uint32_t>(mUsedSize); }
		char* GetData() { return DataPtr(mData); }
		const char* GetData() const { return DataPtr(mData); }
		uint32_t    GetElementCount() const;

		// Changes the buffer size to `size`.
		// If this operations grows the vector, this class considers all the elements
		// to be initialized. It is the caller responsibility to initialize those elements.
		void SetSize(uint32_t size)
		{
			mUsedSize = size;
			mData.resize(size);
		}

		// Trusts that calling code is well behaved :)
		template <typename T>
		void Append(const T& value)
		{
			Append(1, &value);
		}

		template <typename T>
		void Append(uint32_t count, const T* pValues)
		{
			uint32_t sizeOfValues = count * static_cast<uint32_t>(sizeof(T));
			if ((mUsedSize + sizeOfValues) > mData.size()) {
				mData.resize(std::max<size_t>(mUsedSize + sizeOfValues, mData.size() * 2));
			}

			//  Copy data
			const void* pSrc = pValues;
			void* pDst = mData.data() + mUsedSize;
			memcpy(pDst, pSrc, sizeOfValues);
			mUsedSize += sizeOfValues;
		}

	private:
		BufferType        mType = BUFFER_TYPE_VERTEX;
		uint32_t          mElementSize = 0;
		size_t            mUsedSize = 0;
		std::vector<char> mData;
	};

public:
	Geometry() {}
	virtual ~Geometry() {}

private:
	Result InternalCtor();

public:
	// Create object using parameters from createInfo
	static Result Create(const GeometryCreateInfo& createInfo, Geometry* pGeometry);

	// Create object using parameters from createInfo using data from mesh
	static Result Create(
		const GeometryCreateInfo& createInfo,
		const TriMesh& mesh,
		Geometry* pGeometry);

	// Create object using parameters from createInfo using data from tri mesh
	static Result Create(
		const GeometryCreateInfo& createInfo,
		const WireMesh& mesh,
		Geometry* pGeometry);

	// Create object with a create info derived from mesh
	static Result Create(const TriMesh& mesh, Geometry* pGeometry);
	static Result Create(const WireMesh& mesh, Geometry* pGeometry);

	IndexType         GetIndexType() const { return mCreateInfo.indexType; }
	const Geometry::Buffer* GetIndexBuffer() const { return &mIndexBuffer; }
	void                    SetIndexBuffer(const Geometry::Buffer& newIndexBuffer);
	uint32_t                GetIndexCount() const;

	GeometryVertexAttributeLayout GetVertexAttributeLayout() const { return mCreateInfo.vertexAttributeLayout; }
	uint32_t                      GetVertexBindingCount() const { return mCreateInfo.vertexBindingCount; }
	const VertexBinding* GetVertexBinding(uint32_t index) const;

	uint32_t                GetVertexCount() const;
	uint32_t                GetVertexBufferCount() const { return CountU32(mVertexBuffers); }
	const Geometry::Buffer* GetVertexBuffer(uint32_t index) const;
	Geometry::Buffer* GetVertexBuffer(uint32_t index);
	uint32_t                GetLargestBufferSize() const;

	// Appends single index, triangle, or edge vertex indices to index buffer
	//
	// Will cast to uint16_t if geometry index type is UINT16.
	// Will cast to uint8_t if geometry index type is UINT8.
	// NOOP if index type is UNDEFINED (geometry does not have index data).
	void AppendIndex(uint32_t idx);
	void AppendIndicesTriangle(uint32_t idx0, uint32_t idx1, uint32_t idx2);
	void AppendIndicesEdge(uint32_t idx0, uint32_t idx1);

	// Append a chunk of UINT32 indices
	void AppendIndicesU32(uint32_t count, const uint32_t* pIndices);

	// Append multiple attributes at once
	uint32_t AppendVertexData(const TriMeshVertexData& vtx);
	uint32_t AppendVertexData(const TriMeshVertexDataCompressed& vtx);
	uint32_t AppendVertexData(const WireMeshVertexData& vtx);

	// Appends triangle or edge vertex data and indices (if present)
	void AppendTriangle(const TriMeshVertexData& vtx0, const TriMeshVertexData& vtx1, const TriMeshVertexData& vtx2);
	void AppendEdge(const WireMeshVertexData& vtx0, const WireMeshVertexData& vtx1);

private:
	// This is intialized to point to a static var of derived class of VertexDataProcessorBase
	// which is shared by geometry objects, it is not supposed to be deleted
	VertexDataProcessorBase<TriMeshVertexData>* mVDProcessor = nullptr;
	VertexDataProcessorBase<TriMeshVertexDataCompressed>* mVDProcessorCompressed = nullptr;
	GeometryCreateInfo                                    mCreateInfo = {};
	Geometry::Buffer                                      mIndexBuffer;
	std::vector<Geometry::Buffer>                         mVertexBuffers;
	uint32_t                                              mPositionBufferIndex = VALUE_IGNORED;
	uint32_t                                              mNormaBufferIndex = VALUE_IGNORED;
	uint32_t                                              mColorBufferIndex = VALUE_IGNORED;
	uint32_t                                              mTexCoordBufferIndex = VALUE_IGNORED;
	uint32_t                                              mTangentBufferIndex = VALUE_IGNORED;
	uint32_t                                              mBitangentBufferIndex = VALUE_IGNORED;
};

#pragma endregion

#pragma region Mipmap

// Stores a mipmap as a linear chunk of memory with each mip level accessible as a Bitmap.
//! The expected disk format used by Mipmap::LoadFile is an vertically tailed mip map:
//!   +---------------------+
//!   | MIP 0               |
//!   |                     |
//!   +---------------------+
//!   | MIP 1    |          |
//!   +----------+----------+
//!   | ... |               |
//!   +-----+---------------+
class Mipmap
{
public:
	Mipmap() {}
	// Using the static, shared-memory pool is currently only safe in single-threaded applications!
	// This should only be used for temporary mipmaps which will be destroyed prior to the creation of any new mipmap.
	Mipmap(uint32_t width, uint32_t height, Bitmap::Format format, uint32_t levelCount, bool useStaticPool);
	Mipmap(uint32_t width, uint32_t height, Bitmap::Format format, uint32_t levelCount);
	// Using the static, shared-memory pool is currently only safe in single-threaded applications!
	// This should only be used for temporary mipmaps which will be destroyed prior to the creation of any new mipmap.
	Mipmap(const Bitmap& bitmap, uint32_t levelCount, bool useStaticPool);
	Mipmap(const Bitmap& bitmap, uint32_t levelCount);
	~Mipmap() {}

	// Returns true if there's at least one mip level, format is valid, and storage is valid
	bool IsOk() const;

	Bitmap::Format GetFormat() const;
	uint32_t       GetLevelCount() const { return CountU32(mMips); }
	Bitmap* GetMip(uint32_t level);
	const Bitmap* GetMip(uint32_t level) const;

	uint32_t GetWidth(uint32_t level) const;
	uint32_t GetHeight(uint32_t level) const;

	static uint32_t CalculateLevelCount(uint32_t width, uint32_t height);
	static Result   LoadFile(const std::filesystem::path& path, uint32_t baseWidth, uint32_t baseHeight, Mipmap* pMipmap, uint32_t levelCount = REMAINING_MIP_LEVELS);
	static Result   SaveFile(const std::filesystem::path& path, const Mipmap* pMipmap, uint32_t levelCount = REMAINING_MIP_LEVELS);

private:
	std::vector<char>   mData;
	std::vector<Bitmap> mMips;

	// Static, shared-memory pool for temporary mipmap generation.
	// NOTE: This is designed for single-threaded use ONLY as it's an unprotected memory block!
	// This will need locks if the consuming paths ever become multi-threaded.
	static std::vector<char> mStaticData;
	bool                     mUseStaticPool = false;
};

#pragma endregion

#pragma region grfx util

namespace grfx_util
{

	class ImageOptions
	{
	public:
		ImageOptions() {}
		~ImageOptions() {}

		ImageOptions& AdditionalUsage(ImageUsageFlags flags) { mAdditionalUsage = flags; return *this; }
		ImageOptions& MipLevelCount(uint32_t levelCount) { mMipLevelCount = levelCount; return *this; }

	private:
		ImageUsageFlags mAdditionalUsage = ImageUsageFlags();
		uint32_t              mMipLevelCount = REMAINING_MIP_LEVELS;

		friend Result CreateImageFromBitmap(
			Queue* pQueue,
			const Bitmap* pBitmap,
			Image** ppImage,
			const ImageOptions& options);

		friend Result CreateImageFromCompressedImage(
			Queue* pQueue,
			const gli::texture& image,
			Image** ppImage,
			const ImageOptions& options);

		friend Result CreateImageFromFile(
			Queue* pQueue,
			const std::filesystem::path& path,
			Image** ppImage,
			const ImageOptions& options,
			bool                         useGpu);

		friend Result CreateImageFromBitmapGpu(
			Queue* pQueue,
			const Bitmap* pBitmap,
			Image** ppImage,
			const ImageOptions& options);
	};

	Result CopyBitmapToImage(
		Queue* pQueue,
		const Bitmap* pBitmap,
		Image* pImage,
		uint32_t            mipLevel,
		uint32_t            arrayLayer,
		ResourceState stateBefore,
		ResourceState stateAfter);

	Result CreateImageFromBitmap(
		Queue* pQueue,
		const Bitmap* pBitmap,
		Image** ppImage,
		const ImageOptions& options = ImageOptions());

	Result CreateImageFromFile(
		Queue* pQueue,
		const std::filesystem::path& path,
		Image** ppImage,
		const ImageOptions& options = ImageOptions(),
		bool                         useGpu = false);

	Result CreateImageFromBitmapGpu(
		Queue* pQueue,
		const Bitmap* pBitmap,
		Image** ppImage,
		const ImageOptions& options = ImageOptions());

	class TextureOptions
	{
	public:
		TextureOptions() {}
		~TextureOptions() {}

		TextureOptions& AdditionalUsage(ImageUsageFlags flags) { mAdditionalUsage = flags; return *this; }
		TextureOptions& InitialState(ResourceState state) { mInitialState = state; return *this; }
		TextureOptions& MipLevelCount(uint32_t levelCount) { mMipLevelCount = levelCount; return *this; }
		TextureOptions& SamplerYcbcrConversion(SamplerYcbcrConversion* pYcbcrConversion) { mYcbcrConversion = pYcbcrConversion; return *this; }

	private:
		ImageUsageFlags         mAdditionalUsage = ImageUsageFlags();
		ResourceState           mInitialState = ResourceState::RESOURCE_STATE_SHADER_RESOURCE;
		uint32_t                      mMipLevelCount = 1;
		vkr::SamplerYcbcrConversion* mYcbcrConversion = nullptr;

		friend Result CreateTextureFromBitmap(
			Queue* pQueue,
			const Bitmap* pBitmap,
			Texture** ppTexture,
			const TextureOptions& options);

		friend Result CreateTextureFromMipmap(
			Queue* pQueue,
			const Mipmap* pMipmap,
			Texture** ppTexture,
			const TextureOptions& options);

		friend Result CreateTextureFromFile(
			Queue* pQueue,
			const std::filesystem::path& path,
			Texture** ppTexture,
			const TextureOptions& options);
	};

	Result CopyBitmapToTexture(
		Queue* pQueue,
		const Bitmap* pBitmap,
		Texture* pTexture,
		uint32_t            mipLevel,
		uint32_t            arrayLayer,
		ResourceState stateBefore,
		ResourceState stateAfter);

	Result CreateTextureFromBitmap(
		Queue* pQueue,
		const Bitmap* pBitmap,
		Texture** ppTexture,
		const TextureOptions& options = TextureOptions());

	// Mip level count from pMipmap is used. Mip level count from options is ignored.
	Result CreateTextureFromMipmap(
		Queue* pQueue,
		const Mipmap* pMipmap,
		Texture** ppTexture,
		const TextureOptions& options = TextureOptions());

	Result CreateTextureFromFile(
		Queue* pQueue,
		const std::filesystem::path& path,
		Texture** ppTexture,
		const TextureOptions& options = TextureOptions());

	// Create a 1x1 texture with the specified pixel data. The format for the texture is derived from the pixel data type, which can be one of uint8, uint16, uint32 or float.
	template <typename PixelDataType>
	Result CreateTexture1x1(
		Queue* pQueue,
		const std::array<PixelDataType, 4> color,
		Texture** ppTexture,
		const TextureOptions& options = TextureOptions())
	{
		ASSERT_NULL_ARG(pQueue);
		ASSERT_NULL_ARG(ppTexture);

		Bitmap::Format format = Bitmap::FORMAT_UNDEFINED;
		if constexpr (std::is_same_v<PixelDataType, float>) {
			format = Bitmap::FORMAT_RGBA_FLOAT;
		}
		else if constexpr (std::is_same_v<PixelDataType, uint32_t>) {
			format = Bitmap::FORMAT_RGBA_UINT32;
		}
		else if constexpr (std::is_same_v<PixelDataType, uint16_t>) {
			format = Bitmap::FORMAT_RGBA_UINT16;
		}
		else if constexpr (std::is_same_v<PixelDataType, uint8_t>) {
			format = Bitmap::FORMAT_RGBA_UINT8;
		}
		else {
			ASSERT_MSG(false, "Invalid pixel data type: must be one of uint8, uint16, uint32 or float.")
				return ERROR_INVALID_CREATE_ARGUMENT;
		}

		// Create bitmap
		Result ppxres = SUCCESS;
		Bitmap bitmap = Bitmap::Create(1, 1, format, &ppxres);
		if (Failed(ppxres)) {
			return ERROR_BITMAP_CREATE_FAILED;
		}

		// Fill color
		bitmap.Fill(color[0], color[1], color[2], color[3]);

		return CreateTextureFromBitmap(pQueue, &bitmap, ppTexture, options);
	}

	// Creates an irradiance and an environment texture from an *.ibl file
	Result CreateIBLTexturesFromFile(
		Queue* pQueue,
		const std::filesystem::path& path,
		Texture** ppIrradianceTexture,
		Texture** ppEnvironmentTexture);
	/*

	Cross Horizontal Left:
			   _____
			  |  0  |
		 _____|_____|_____ _____
		|  1  |  2  |  3  |  4  |
		|_____|_____|_____|_____|
			  |  5  |
			  |_____|

	Cross Horizontal Right:
					 _____
					|  0  |
		 ___________|_____|_____
		|  1  |  2  |  3  |  4  |
		|_____|_____|_____|_____|
					|  5  |
					|_____|

	Cross Vertical Top:
			   _____
			  |  0  |
		 _____|_____|_____
		|  1  |  2  |  3  |
		|_____|_____|_____|
			  |  4  |
			  |_____|
			  |  5  |
			  |_____|

	Cross Vertical Bottom:
			   _____
			  |  0  |
			  |_____|
			  |  1  |
		 _____|_____|_____
		|  2  |  3  |  4  |
		|_____|_____|_____|
			  |  5  |
			  |_____|

	Lat Long Horizontal:
		 _____ _____ _____
		|  0  |  1  |  2  |
		|_____|_____|_____|
		|  3  |  4  |  5  |
		|_____|_____|_____|

	Lat Long Vertical:
		 _____ _____
		|  0  |  1  |
		|_____|_____|
		|  2  |  3  |
		|_____|_____|
		|  4  |  5  |
		|_____|_____|

	Strip Horizontal:
		 _____ _____ _____ _____ _____ _____
		|  0  |  1  |  2  |  3  |  4  |  5  |
		|_____|_____|_____|_____|_____|_____|


	Strip Vertical:
		 _____
		|  0  |
		|_____|
		|  1  |
		|_____|
		|  2  |
		|_____|
		|  3  |
		|_____|
		|  4  |
		|_____|
		|  5  |
		|_____|

	*/
	enum CubeImageLayout
	{
		CUBE_IMAGE_LAYOUT_UNDEFINED = 0,
		CUBE_IMAGE_LAYOUT_CROSS_HORIZONTAL_LEFT = 1,
		CUBE_IMAGE_LAYOUT_CROSS_HORIZONTAL_RIGHT = 2,
		CUBE_IMAGE_LAYOUT_CROSS_VERTICAL_TOP = 3,
		CUBE_IMAGE_LAYOUT_CROSS_VERTICAL_BOTTOM = 4,
		CUBE_IMAGE_LAYOUT_LAT_LONG_HORIZONTAL = 5,
		CUBE_IMAGE_LAYOUT_LAT_LONG_VERTICAL = 6,
		CUBE_IMAGE_LAYOUT_STRIP_HORIZONTAL = 7,
		CUBE_IMAGE_LAYOUT_STRIP_VERTICAL = 8,
		CUBE_IMAGE_LAYOUT_CROSS_HORIZONTAL = CUBE_IMAGE_LAYOUT_CROSS_HORIZONTAL_LEFT,
		CUBE_IMAGE_LAYOUT_CROSS_VERTICAL = CUBE_IMAGE_LAYOUT_CROSS_VERTICAL_TOP,
		CUBE_IMAGE_LAYOUT_LAT_LONG = CUBE_IMAGE_LAYOUT_LAT_LONG_HORIZONTAL,
		CUBE_IMAGE_LAYOUT_STRIP = CUBE_IMAGE_LAYOUT_STRIP_HORIZONTAL,
	};

	// Rotation is always clockwise.
	enum CubeFaceOp
	{
		CUBE_FACE_OP_NONE = 0,
		CUBE_FACE_OP_ROTATE_90 = 1,
		CUBE_FACE_OP_ROTATE_180 = 2,
		CUBE_FACE_OP_ROTATE_270 = 3,
		CUBE_FACE_OP_INVERT_HORIZONTAL = 4,
		CUBE_FACE_OP_INVERT_VERTICAL = 5,
	};

	// See enum CubeImageLayout for explanation of layouts
	// Example - Use subimage 0 with 90 degrees CW rotation for posX face:
	//   layout = CUBE_IMAGE_LAYOUT_CROSS_HORIZONTAL;
	//   posX   = PPX_ENCODE_CUBE_FACE(0, CUBE_FACE_OP_ROTATE_90, :CUBE_FACE_OP_NONE);
#define CUBE_OP_MASK           0xFF
#define CUBE_OP_SUBIMAGE_SHIFT 0
#define CUBE_OP_OP1_SHIFT      8
#define CUBE_OP_OP2_SHIFT      16

#define ENCODE_CUBE_FACE(SUBIMAGE, OP1, OP2)              \
    (SUBIMAGE & CUBE_OP_MASK) |                           \
        ((OP1 & CUBE_OP_MASK) << CUBE_OP_OP1_SHIFT) | \
        ((OP1 & CUBE_OP_MASK) << CUBE_OP_OP2_SHIFT)

#define DECODE_CUBE_FACE_SUBIMAGE(FACE) (FACE >> CUBE_OP_SUBIMAGE_SHIFT) & CUBE_OP_MASK
#define DECODE_CUBE_FACE_OP1(FACE)      (FACE >> CUBE_OP_OP1_SHIFT) & CUBE_OP_MASK
#define DECODE_CUBE_FACE_OP2(FACE)      (FACE >> CUBE_OP_OP2_SHIFT) & CUBE_OP_MASK

	struct CubeMapCreateInfo
	{
		CubeImageLayout layout = CUBE_IMAGE_LAYOUT_UNDEFINED;
		uint32_t        posX = VALUE_IGNORED;
		uint32_t        negX = VALUE_IGNORED;
		uint32_t        posY = VALUE_IGNORED;
		uint32_t        negY = VALUE_IGNORED;
		uint32_t        posZ = VALUE_IGNORED;
		uint32_t        negZ = VALUE_IGNORED;
	};

	Result CreateCubeMapFromFile(
		Queue* pQueue,
		const std::filesystem::path& path,
		const CubeMapCreateInfo* pCreateInfo,
		Image** ppImage,
		const ImageUsageFlags& additionalImageUsage = ImageUsageFlags());

	Result CreateMeshFromGeometry(
		Queue* pQueue,
		const Geometry* pGeometry,
		Mesh** ppMesh);

	Result CreateMeshFromTriMesh(
		Queue* pQueue,
		const TriMesh* pTriMesh,
		Mesh** ppMesh);

	Result CreateMeshFromWireMesh(
		Queue* pQueue,
		const WireMesh* pWireMesh,
		Mesh** ppMesh);

	Result CreateMeshFromFile(
		Queue* pQueue,
		const std::filesystem::path& path,
		Mesh** ppMesh,
		const TriMeshOptions& options = TriMeshOptions());

	Format ToGrfxFormat(Bitmap::Format value);

}

#pragma endregion

} // namespace vkr