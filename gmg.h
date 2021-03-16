#ifndef GMG_H
#define GMG_H

#ifndef DAS_H
#error "GMG depends on DAS, make sure das.h is included before gmg.h"
#endif

#ifdef GMG_ENABLE_VULKAN

#define GMG_VK_DEFINE_HANDLE(object) typedef struct object##_T* object;
GMG_VK_DEFINE_HANDLE(VkInstance);
GMG_VK_DEFINE_HANDLE(VkPhysicalDevice);
GMG_VK_DEFINE_HANDLE(VkDevice);
GMG_VK_DEFINE_HANDLE(VkBuffer);
GMG_VK_DEFINE_HANDLE(VkImage);
GMG_VK_DEFINE_HANDLE(VkImageView);
GMG_VK_DEFINE_HANDLE(VkSampler);
GMG_VK_DEFINE_HANDLE(VkRenderPass);
GMG_VK_DEFINE_HANDLE(VkFramebuffer);
GMG_VK_DEFINE_HANDLE(VkShaderModule);
GMG_VK_DEFINE_HANDLE(VkDescriptorSet);
GMG_VK_DEFINE_HANDLE(VkDescriptorPool);
GMG_VK_DEFINE_HANDLE(VkDescriptorSetLayout);
GMG_VK_DEFINE_HANDLE(VkPipelineLayout);
GMG_VK_DEFINE_HANDLE(VkPipelineCache);
GMG_VK_DEFINE_HANDLE(VkPipeline);
GMG_VK_DEFINE_HANDLE(VkQueue);
GMG_VK_DEFINE_HANDLE(VkDeviceMemory);
GMG_VK_DEFINE_HANDLE(VkSurfaceKHR);
GMG_VK_DEFINE_HANDLE(VkSwapchainKHR);
GMG_VK_DEFINE_HANDLE(VkCommandPool);
GMG_VK_DEFINE_HANDLE(VkCommandBuffer);
GMG_VK_DEFINE_HANDLE(VkFence);
GMG_VK_DEFINE_HANDLE(VkSemaphore);
typedef_DasStk(VkDescriptorPool);

typedef uint64_t VkDeviceSize;

typedef struct VkPhysicalDeviceProperties VkPhysicalDeviceProperties;
typedef struct VkPhysicalDeviceFeatures VkPhysicalDeviceFeatures;
typedef struct VkPhysicalDeviceMemoryProperties VkPhysicalDeviceMemoryProperties;
typedef struct VkQueueFamilyProperties VkQueueFamilyProperties;
typedef struct VkSurfaceCapabilitiesKHR VkSurfaceCapabilitiesKHR;
typedef struct VkSurfaceFormatKHR VkSurfaceFormatKHR;
typedef enum VkPresentModeKHR VkPresentModeKHR;
typedef uint32_t VkFlags;
typedef VkFlags VkQueueFlags;
typedef struct VkLayerProperties VkLayerProperties;
typedef struct VkExtensionProperties VkExtensionProperties;
typedef struct VkDescriptorPoolSize VkDescriptorPoolSize;

#ifndef VK_MAX_MEMORY_TYPES
#define VK_MAX_MEMORY_TYPES 32
#endif

#define gmg_vulkan_assert_result(call_expr) { \
	VkResult _vk_res = call_expr; \
	if (_vk_res < 0) { \
		fprintf(stderr, "file: %s:%u\nerror: call to '%s' failed and returned error code '%u'", __FILE__, __LINE__, #call_expr, _vk_res); \
		abort(); \
	} \
}

#endif // GMG_ENABLE_VULKAN

// ==========================================================
//
//
// Config
//
//
// ==========================================================

#ifndef gmg_vulkan_max_bound_vertex_buffers_count
#define gmg_vulkan_max_bound_vertex_buffers_count 64
#endif

#ifndef gmg_default_object_pool_reserve_cap
#define gmg_default_object_pool_reserve_cap (8 * 1024)
#endif

#ifndef gmg_default_object_pool_grow_count
#define gmg_default_object_pool_grow_count 1024
#endif

#ifndef gmg_frame_allocator_reserve_size
#define gmg_frame_allocator_reserve_size 10 * 1024 * 1024
#endif

#ifndef gmg_frame_allocator_grow_size
#define gmg_frame_allocator_grow_size 1 * 1024 * 1024
#endif

#ifndef gmg_vulkan_allocator_min_block_size_device_local_memory
#define gmg_vulkan_allocator_min_block_size_device_local_memory (128 * 1024 * 1024)
#endif

#ifndef gmg_vulkan_allocator_min_block_size_host_visible_memory
#define gmg_vulkan_allocator_min_block_size_host_visible_memory (64 * 1024 * 1024)
#endif

#ifndef gmg_vulkan_memory_stager_buffer_size
#define gmg_vulkan_memory_stager_buffer_size (64 * 1024 * 1024)
#endif

// ==========================================================
//
//
// General
//
//
// ==========================================================

typedef uint8_t GmgBool;
#define gmg_false 0
#define gmg_true 1

typedef enum GmgResult GmgResult;
enum GmgResult {
	GmgResult_error_ = -1,
	GmgResult_error_unexpected_null_id = -2,
	GmgResult_error_object_use_after_free = -3,
	GmgResult_error_logical_device_object_pool_full = -4,
	GmgResult_error_out_of_memory_host = -5,
	GmgResult_error_out_of_memory_device = -6,
	GmgResult_error_multiple_depth_stencil_attachments = -7,
	GmgResult_error_cannot_find_transfer_queue = -8,
	GmgResult_error_cannot_find_compute_queue = -9,
	GmgResult_error_cannot_find_graphics_queue = -10,
	GmgResult_error_cannot_find_present_queue = -11,
	GmgResult_error_incompatible_shader_format = -12,
	GmgResult_error_vertex_layout_pool_full = -13,
	GmgResult_error_layout_size_mismatch = -14,
	GmgResult_error_incompatible_backend_type = -15,
	GmgResult_error_missing_vertex_shader = -16,
	GmgResult_error_missing_fragment_shader = -17,
	GmgResult_error_missing_mesh_shader = -18,
	GmgResult_error_missing_compute_shader = -20,
	GmgResult_error_binding_idx_out_of_bounds = -21,
	GmgResult_error_descriptor_type_mismatch = -22,
	GmgResult_error_binding_is_not_dynamic = -24,
	GmgResult_error_binding_elmt_idx_out_of_bounds = -25,
	GmgResult_error_expected_vertex_buffer = -26,
	GmgResult_error_incompatible_binding_idx = -27,
	GmgResult_error_expected_index_buffer = -28,
	GmgResult_error_incompatible_render_pass_layout = -29,
	GmgResult_error_incompatible_vertex_layout = -30,
	GmgResult_error_draw_cmd_already_in_process = -31,
	GmgResult_error_uninitialized = -32,
	GmgResult_error_push_constants_out_of_bounds = -33,
	GmgResult_error_vertex_array_has_index_buffer = -34,
	GmgResult_error_vertex_array_does_not_have_index_buffer = -35,
	GmgResult_error_buffer_start_idx_out_of_bounds = -36,
	GmgResult_error_cannot_read_from_gpu_only_resource = -37,
	GmgResult_error_map_out_of_bounds = -38,
	GmgResult_error_attachment_index_out_of_bounds = -39,
	GmgResult_error_width_must_not_be_zero = -40,
	GmgResult_error_height_must_not_be_zero = -41,
	GmgResult_error_depth_must_not_be_zero = -42,
	GmgResult_error_mip_levels_must_not_be_zero = -43,
	GmgResult_error_array_layers_must_not_be_zero = -44,
	GmgResult_error_descriptor_set_layout_not_in_allocator = -45,
	GmgResult_error_shader_does_not_have_a_draw_cmd_descriptor_set = -46,
	GmgResult_error_draw_cmd_descriptor_is_set_twice = -47,
	GmgResult_error_draw_cmd_has_unset_descriptors = -48,
	GmgResult_error_scene_descriptor_set_layout_mismatch = -49,
	GmgResult_error_render_pass_has_more_attachments_than_the_layout = -50,
	GmgResult_error_cannot_submit_zero_render_passes = -51,
	GmgResult_error_END = -51,

	GmgResult_success = 0,
	GmgResult_nothing_changed,
	GmgResult_END,
	GmgResult_COUNT = GmgResult_END + -GmgResult_error_END,
};
typedef_DasStk(GmgResult);

const char* GmgResult_string(GmgResult result);
GmgResult GmgResult_from_DasError(DasError error);
#define gmg_assert_result(call_expr) { \
	GmgResult _gmg_res; \
	das_assert((_gmg_res = call_expr) >= 0, "GMG error '%s'", GmgResult_string(_gmg_res)); \
}

typedef_DasPoolElmtId(GmgVertexLayoutId, 20);

typedef_DasPoolElmtId(_GmgLogicalDeviceObjectId, 20);
#define typedef_GmgLogicalDeviceObjectId(Name) \
typedef union Name Name; \
union Name { \
	_GmgLogicalDeviceObjectId object_id; \
	DasPoolElmtId raw; \
}; \
static Name Name##_null = {0}
typedef_GmgLogicalDeviceObjectId(GmgSamplerId);
typedef_GmgLogicalDeviceObjectId(GmgTextureId);
typedef_GmgLogicalDeviceObjectId(GmgDescriptorSetLayoutId);
typedef_GmgLogicalDeviceObjectId(GmgDescriptorSetId);
typedef_GmgLogicalDeviceObjectId(GmgDescriptorSetAllocatorId);
typedef_GmgLogicalDeviceObjectId(GmgShaderModuleId);
typedef_GmgLogicalDeviceObjectId(GmgShaderId);
typedef_GmgLogicalDeviceObjectId(GmgMaterialSpecCacheId);
typedef_GmgLogicalDeviceObjectId(GmgMaterialSpecId);
typedef_GmgLogicalDeviceObjectId(GmgMaterialId);
typedef_GmgLogicalDeviceObjectId(GmgBufferId);
typedef_GmgLogicalDeviceObjectId(GmgVertexArrayId);
typedef_GmgLogicalDeviceObjectId(GmgRenderPassLayoutId);
typedef_GmgLogicalDeviceObjectId(GmgRenderPassId);
typedef_GmgLogicalDeviceObjectId(GmgFrameBufferId);
typedef_GmgLogicalDeviceObjectId(GmgSwapchainId);
typedef_DasStk(GmgDescriptorSetId);

typedef uint8_t GmgBackendType;
enum {
#ifdef GMG_ENABLE_VULKAN
	GmgBackendType_vulkan,
#endif
#ifdef GMG_ENABLE_DX12
	GmgBackendType_directx12,
#endif
};

typedef struct GmgSurface GmgSurface;
struct GmgSurface {
	GmgBackendType type;
	union {
#ifdef GMG_ENABLE_VULKAN
		VkSurfaceKHR vulkan;
#endif
	};
};

typedef union GmgVec2u GmgVec2u;
union GmgVec2u {
	struct {
		uint32_t x;
		uint32_t y;
	};
	struct {
		uint32_t width;
		uint32_t height;
	};
};
#define GmgVec2u_init(_x, _y) ((GmgVec2u){ .x = _x, .y = _y })

typedef union GmgRect2u GmgRect2u;
union GmgRect2u {
	struct {
		uint32_t x;
		uint32_t y;
		uint32_t width;
		uint32_t height;
	};
	struct {
		GmgVec2u offset;
		GmgVec2u size;
	};
};
#define GmgRect2u_init(_x, _y, _width, _height) ((GmgRect2u){ .x = _x, .y = _y, .width = _width, .height = _height })
#define GmgRect2u_init_v2(_offset, _size) ((GmgRect2u){ .offset = _offset, .size = _size })

typedef struct GmgViewport GmgViewport;
struct GmgViewport {
	float x;
	float y;
	float width;
	float height;
	float min_depth;
	float max_depth;
};

typedef uint8_t GmgSampleCount;
enum {
	GmgSampleCount_1 = 1,
	GmgSampleCount_2 = 2,
	GmgSampleCount_4 = 4,
	GmgSampleCount_8 = 8,
	GmgSampleCount_16 = 16,
	GmgSampleCount_32 = 32,
	GmgSampleCount_64 = 64,
};

typedef uint8_t GmgCompareOp;
enum {
	GmgCompareOp_never,
	GmgCompareOp_less,
	GmgCompareOp_equal,
	GmgCompareOp_less_or_equal,
	GmgCompareOp_greater,
	GmgCompareOp_not_equal,
	GmgCompareOp_greater_or_equal,
	GmgCompareOp_always,
};

typedef uint8_t GmgTextureFormat;
enum {
	GmgTextureFormat_none,

	GmgTextureFormat_r8_unorm,
	GmgTextureFormat_r8g8_unorm,
	GmgTextureFormat_r8g8b8_unorm,
	GmgTextureFormat_r8g8b8a8_unorm,

	GmgTextureFormat_b8g8r8_unorm,
	GmgTextureFormat_b8g8r8a8_unorm,

	//
	// depth and stencil
	GmgTextureFormat_d16,
	GmgTextureFormat_d32,
	GmgTextureFormat_s8,
	GmgTextureFormat_d16_s8,
	GmgTextureFormat_d24_s8,
	GmgTextureFormat_d32_s8,
	GmgTextureFormat_COUNT,
};

extern uint8_t GmgTextureFormat_bytes_per_pixel[GmgTextureFormat_COUNT];

typedef uint8_t GmgIndexType;
enum {
	GmgIndexType_u8,
	GmgIndexType_u16,
	GmgIndexType_u32,
	GmgIndexType_COUNT,
};

extern uint8_t GmgIndexType_sizes[GmgIndexType_COUNT];

typedef uint8_t GmgMemoryLocation;
enum {
	// memory is local to the GPU and the CPU cannot read from this resource.
	// this is the biggest pool of memory on the GPU.
	// writes to this memory should happen infrequently
	// in vulkan terms: DEVICE_LOCAL_BIT and may have (but not required) HOST_VISIBLE_BIT
	// in dx12 terms: D3D12_HEAP_TYPE_DEFAULT
	GmgMemoryLocation_gpu,

	// memory is optimized for frequent updates to the GPU from the CPU.
	// this is the smallest pool of memory on the GPU.
	// in vulkan terms: HOST_VISIBLE_BIT and may have (but not required) DEVICE_LOCAL_BIT
	// in dx12 terms: D3D12_HEAP_TYPE_UPLOAD
	GmgMemoryLocation_shared,

	// memory is optimized for frequent read to the CPU from the GPU.
	// in vulkan terms: DEVICE_LOCAL_BIT, HOST_VISIBLE_BIT and HOST_CACHED_BIT
	// in dx12 terms: D3D12_HEAP_TYPE_READBACK
	GmgMemoryLocation_shared_cached,
};

// ==========================================================
//
//
// Vertex Layout
//
//
// ==========================================================

typedef uint8_t GmgVertexElmtType;
enum {
	GmgVertexElmtType_u8,
	GmgVertexElmtType_s8,
	GmgVertexElmtType_u8_f32,
	GmgVertexElmtType_s8_f32,
	GmgVertexElmtType_u8_f32_normalize,
	GmgVertexElmtType_s8_f32_normalize,
	GmgVertexElmtType_u16,
	GmgVertexElmtType_s16,
	GmgVertexElmtType_u16_f32,
	GmgVertexElmtType_s16_f32,
	GmgVertexElmtType_u16_f32_normalize,
	GmgVertexElmtType_s16_f32_normalize,
	GmgVertexElmtType_u32,
	GmgVertexElmtType_s32,
	GmgVertexElmtType_u64,
	GmgVertexElmtType_s64,
	GmgVertexElmtType_f16,
	GmgVertexElmtType_f32,
	GmgVertexElmtType_f64,
	GmgVertexElmtType_COUNT,
};

extern uint8_t GmgVertexElmtType_sizes[GmgVertexElmtType_COUNT];
extern uint8_t GmgVertexElmtType_aligns[GmgVertexElmtType_COUNT];

typedef uint8_t GmgVertexVectorType;
enum {
	GmgVertexVectorType_1,
	GmgVertexVectorType_2,
	GmgVertexVectorType_3,
	GmgVertexVectorType_4,
	GmgVertexVectorType_COUNT,
};
#define GmgVertexVectorType_elmts_count(vector_type) ((uint32_t)(vector_type + 1))

typedef struct GmgVertexAttribInfo GmgVertexAttribInfo;
struct GmgVertexAttribInfo {
	uint8_t location_idx;
	GmgVertexElmtType elmt_type: 6;
	GmgVertexVectorType vector_type: 2;
};

typedef struct GmgVertexBindingInfo GmgVertexBindingInfo;
struct GmgVertexBindingInfo {
	GmgVertexAttribInfo* attribs;
	uint16_t attribs_count;
	uint16_t size;
};

typedef struct GmgVertexLayout GmgVertexLayout;
struct GmgVertexLayout {
	uint16_t bindings_count;
	GmgVertexBindingInfo* bindings;
};

GmgResult gmg_vertex_layout_register(GmgVertexLayout* vl, GmgVertexLayoutId* id_out);
GmgResult gmg_vertex_layout_deregister(GmgVertexLayoutId id);
GmgResult gmg_vertex_layout_get(GmgVertexLayoutId id, GmgVertexLayout** out);

// ==========================================================
//
//
// Render State
//
//
// ==========================================================

typedef uint8_t GmgPrimitiveTopology;
enum {
	GmgPrimitiveTopology_point_list,
	GmgPrimitiveTopology_line_list,
	GmgPrimitiveTopology_line_strip,
	GmgPrimitiveTopology_triangle_list,
	GmgPrimitiveTopology_triangle_strip,
	GmgPrimitiveTopology_triangle_fan,
	GmgPrimitiveTopology_line_list_with_adjacency,
	GmgPrimitiveTopology_line_strip_with_adjacency,
	GmgPrimitiveTopology_triangle_list_with_adjacency,
	GmgPrimitiveTopology_triangle_strip_with_adjacency,
	GmgPrimitiveTopology_patch_list,
};

typedef uint8_t GmgRasterizationFlags;
enum {
	GmgRasterizationFlags_enable_depth_clamp = 0x1,
	GmgRasterizationFlags_enable_discard = 0x2,
	GmgRasterizationFlags_enable_depth_bias = 0x4,
};

typedef uint8_t GmgPolygonMode;
enum {
	GmgPolygonMode_fill,
	GmgPolygonMode_line,
	GmgPolygonMode_point,
};

typedef uint8_t GmgCullModeFlags;
enum {
	GmgCullModeFlags_none = 0x0,
	GmgCullModeFlags_front = 0x1,
	GmgCullModeFlags_back = 0x2,
	GmgCullModeFlags_front_and_back = GmgCullModeFlags_front | GmgCullModeFlags_back,
};

typedef uint8_t GmgFrontFace;
enum {
	GmgFrontFace_counter_clockwise,
	GmgFrontFace_clockwise,
};

typedef struct GmgRenderStateRasterization GmgRenderStateRasterization;
struct GmgRenderStateRasterization {
	GmgRasterizationFlags flags;
	GmgPolygonMode polygon_mode;
	GmgCullModeFlags cull_mode_flags;
	GmgFrontFace front_face;
	float depth_bias_constant_factor;
	float depth_bias_clamp;
	float depth_bias_slope_factor;
	float line_width;
};

typedef uint8_t GmgMultisampleFlags;
enum {
	GmgMultisampleFlags_enable_sample_shading = 0x1,
	GmgMultisampleFlags_enable_alpha_to_coverage = 0x2,
	GmgMultisampleFlags_enable_alpha_to_one = 0x4,
};

typedef uint32_t GmgSampleMask;

typedef struct GmgRenderStateMultisample GmgRenderStateMultisample;
struct GmgRenderStateMultisample {
	GmgSampleMask* sample_mask;
	float min_sample_shading;
	GmgMultisampleFlags flags;
	GmgSampleCount rasterization_samples_count;
};

typedef uint8_t GmgDepthStencilFlags;
enum {
	GmgDepthStencilFlags_enable_depth_test = 0x1,
	GmgDepthStencilFlags_enable_depth_write = 0x2,
	GmgDepthStencilFlags_enable_depth_bounds_test = 0x4,
	GmgDepthStencilFlags_enable_stencil_test = 0x8,
};

typedef uint8_t GmgStencilOp;
enum {
	GmgStencilOp_keep,
	GmgStencilOp_zero,
	GmgStencilOp_replace,
	GmgStencilOp_increment_and_clamp,
	GmgStencilOp_decrement_and_clamp,
	GmgStencilOp_invert,
	GmgStencilOp_increment_and_wrap,
	GmgStencilOp_decrement_and_wrap,
};

typedef struct GmgStencilOpState GmgStencilOpState;
struct GmgStencilOpState {
	GmgStencilOp fail_op;
	GmgStencilOp pass_op;
	GmgStencilOp depth_fail_op;
	GmgCompareOp compare_op;
	uint32_t compare_mask;
	uint32_t write_mask;
	uint32_t reference;
};

typedef struct GmgRenderStateDepthStencil GmgRenderStateDepthStencil;
struct GmgRenderStateDepthStencil {
	GmgDepthStencilFlags flags;
	GmgCompareOp depth_compare_op;
	GmgStencilOpState front;
	GmgStencilOpState back;
};

typedef uint8_t GmgBlendFactor;
enum {
	GmgBlendFactor_zero,
	GmgBlendFactor_one,
	GmgBlendFactor_src_color,
	GmgBlendFactor_one_minus_src_color,
	GmgBlendFactor_dst_color,
	GmgBlendFactor_one_minus_dst_color,
	GmgBlendFactor_src_alpha,
	GmgBlendFactor_one_minus_src_alpha,
	GmgBlendFactor_dst_alpha,
	GmgBlendFactor_one_minus_dst_alpha,
	GmgBlendFactor_constant_color,
	GmgBlendFactor_one_minus_constant_color,
	GmgBlendFactor_constant_alpha,
	GmgBlendFactor_one_minus_constant_alpha,
	GmgBlendFactor_src_alpha_saturate,
	GmgBlendFactor_src1_color,
	GmgBlendFactor_one_minus_src1_color,
	GmgBlendFactor_src1_alpha,
	GmgBlendFactor_one_minus_src1_alpha,
};

typedef uint8_t GmgBlendOp;
enum {
	GmgBlendOp_add,
	GmgBlendOp_subtract,
	GmgBlendOp_reverse_subtract,
	GmgBlendOp_min,
	GmgBlendOp_max,
};

typedef uint8_t GmgColorComponentFlags;
enum {
	GmgColorComponentFlags_r = 0x1,
	GmgColorComponentFlags_g = 0x2,
	GmgColorComponentFlags_b = 0x4,
	GmgColorComponentFlags_a = 0x8,
	GmgColorComponentFlags_all = 0xf,
};

typedef uint8_t GmgLogicOp;
enum {
	GmgLogicOp_clear,
	GmgLogicOp_and,
	GmgLogicOp_and_reverse,
	GmgLogicOp_copy,
	GmgLogicOp_and_inverted,
	GmgLogicOp_no_op,
	GmgLogicOp_xor,
	GmgLogicOp_or,
	GmgLogicOp_nor,
	GmgLogicOp_equivalent,
	GmgLogicOp_invert,
	GmgLogicOp_or_reverse,
	GmgLogicOp_copy_inverted,
	GmgLogicOp_or_inverted,
	GmgLogicOp_nand,
	GmgLogicOp_set,
};

typedef struct GmgRenderStateBlendAttachment GmgRenderStateBlendAttachment;
struct GmgRenderStateBlendAttachment {
	GmgBool blend_enable;
	GmgBlendFactor src_color_blend_factor;
	GmgBlendFactor dst_color_blend_factor;
	GmgBlendOp color_blend_op;
	GmgBlendFactor src_alpha_blend_factor;
	GmgBlendFactor dst_alpha_blend_factor;
	GmgBlendOp alpha_blend_op;
	GmgColorComponentFlags color_write_mask;
};

typedef struct GmgRenderStateBlend GmgRenderStateBlend;
struct GmgRenderStateBlend {
	GmgRenderStateBlendAttachment* attachments;
	uint32_t attachments_count;
	GmgBool enable_logic_op;
	GmgLogicOp logic_op;
	float blend_constants[4];
};

typedef struct GmgRenderState GmgRenderState;
struct GmgRenderState {
	GmgBool enable_primitive_restart;
	GmgPrimitiveTopology topology;
	uint32_t tessellation_patch_control_points;

	uint32_t viewports_count;

	GmgRenderStateRasterization rasterization;
	GmgRenderStateMultisample multisample;
	GmgRenderStateDepthStencil depth_stencil;
	GmgRenderStateBlend blend;
};

typedef uint8_t GmgDescriptorType;
enum {
	GmgDescriptorType_sampler,
	GmgDescriptorType_texture,
	GmgDescriptorType_uniform_buffer,
	GmgDescriptorType_storage_buffer,
	GmgDescriptorType_uniform_buffer_dynamic,
	GmgDescriptorType_storage_buffer_dynamic,
	GmgDescriptorType_COUNT,
};

typedef uint8_t GmgShaderStageFlags;
enum {
	GmgShaderStageFlags_vertex = 0x1,
	GmgShaderStageFlags_tessellation_control = 0x2,
	GmgShaderStageFlags_tessellation_evaluation = 0x4,
	GmgShaderStageFlags_geometry = 0x8,
	GmgShaderStageFlags_fragment = 0x10,
	GmgShaderStageFlags_compute = 0x20,
	GmgShaderStageFlags_all_graphics = 0x1f,
};

// ==========================================================
//
//
// Physical Device
//
//
// ==========================================================

typedef struct GmgPhysicalDevice GmgPhysicalDevice;
struct GmgPhysicalDevice {
	union {
#ifdef GMG_ENABLE_VULKAN
		struct {
			VkPhysicalDevice handle;
			VkPhysicalDeviceProperties* properties;
			VkPhysicalDeviceFeatures* features;
			VkPhysicalDeviceMemoryProperties* memory_properties;
			VkQueueFamilyProperties* queue_family_properties;
			VkSurfaceCapabilitiesKHR* surface_capabilities;
			VkSurfaceFormatKHR* surface_formats;
			VkPresentModeKHR* surface_present_modes;
			uint32_t queue_families_count;
			uint32_t surface_formats_count;
			uint32_t surface_present_modes_count;
		} vulkan;
#endif
	};
};

typedef uint8_t GmgPhysicalDeviceType;
enum {
	GmgPhysicalDeviceType_other,
	GmgPhysicalDeviceType_gpu_integrated,
	GmgPhysicalDeviceType_gpu_discrete,
	GmgPhysicalDeviceType_gpu_virtual,
	GmgPhysicalDeviceType_cpu,
};

typedef uint8_t GmgPhysicalDeviceFeature;
enum {
	GmgPhysicalDeviceFeature_robust_buffer_access,
	GmgPhysicalDeviceFeature_full_draw_index_uint32,
	GmgPhysicalDeviceFeature_image_cube_array,
	GmgPhysicalDeviceFeature_independent_blend,
	GmgPhysicalDeviceFeature_geometry_shader,
	GmgPhysicalDeviceFeature_tessellation_shader,
	GmgPhysicalDeviceFeature_sample_rate_shading,
	GmgPhysicalDeviceFeature_dual_src_blend,
	GmgPhysicalDeviceFeature_logic_op,
	GmgPhysicalDeviceFeature_multi_draw_indirect,
	GmgPhysicalDeviceFeature_draw_indirect_first_instance,
	GmgPhysicalDeviceFeature_depth_clamp,
	GmgPhysicalDeviceFeature_depth_bias_clamp,
	GmgPhysicalDeviceFeature_fill_mode_non_solid,
	GmgPhysicalDeviceFeature_depth_bounds,
	GmgPhysicalDeviceFeature_wide_lines,
	GmgPhysicalDeviceFeature_large_points,
	GmgPhysicalDeviceFeature_alpha_to_one,
	GmgPhysicalDeviceFeature_multi_viewport,
	GmgPhysicalDeviceFeature_sampler_anisotropy,
	GmgPhysicalDeviceFeature_texture_compression_etc2,
	GmgPhysicalDeviceFeature_texture_compression_astc_ldr,
	GmgPhysicalDeviceFeature_texture_compression_bc,
	GmgPhysicalDeviceFeature_occlusion_query_precise,
	GmgPhysicalDeviceFeature_pipeline_statistics_query,
	GmgPhysicalDeviceFeature_vertex_pipeline_stores_and_atomics,
	GmgPhysicalDeviceFeature_fragment_stores_and_atomics,
	GmgPhysicalDeviceFeature_shader_tessellation_and_geometry_point_size,
	GmgPhysicalDeviceFeature_shader_image_gather_extended,
	GmgPhysicalDeviceFeature_shader_storage_image_extended_formats,
	GmgPhysicalDeviceFeature_shader_storage_image_multisample,
	GmgPhysicalDeviceFeature_shader_storage_image_read_withoutFormat,
	GmgPhysicalDeviceFeature_shader_storage_image_write_without_format,
	GmgPhysicalDeviceFeature_shader_uniform_buffer_array_dynamic_indexing,
	GmgPhysicalDeviceFeature_shader_sampled_image_array_dynamic_indexing,
	GmgPhysicalDeviceFeature_shader_storage_buffer_array_dynamic_indexing,
	GmgPhysicalDeviceFeature_shader_storage_image_array_dynamic_indexing,
	GmgPhysicalDeviceFeature_shader_clip_distance,
	GmgPhysicalDeviceFeature_shader_cull_distance,
	GmgPhysicalDeviceFeature_shader_float64,
	GmgPhysicalDeviceFeature_shader_int64,
	GmgPhysicalDeviceFeature_shader_int16,
	GmgPhysicalDeviceFeature_shader_resource_residency,
	GmgPhysicalDeviceFeature_shader_resource_min_lod,
	GmgPhysicalDeviceFeature_sparse_binding,
	GmgPhysicalDeviceFeature_sparse_residency_buffer,
	GmgPhysicalDeviceFeature_sparse_residency_image2_d,
	GmgPhysicalDeviceFeature_sparse_residency_image3_d,
	GmgPhysicalDeviceFeature_sparse_residency2_samples,
	GmgPhysicalDeviceFeature_sparse_residency4_samples,
	GmgPhysicalDeviceFeature_sparse_residency8_samples,
	GmgPhysicalDeviceFeature_sparse_residency16_samples,
	GmgPhysicalDeviceFeature_sparse_residency_aliased,
	GmgPhysicalDeviceFeature_variable_multisample_rate,
	GmgPhysicalDeviceFeature_inherited_queries,
	GmgPhysicalDeviceFeature_COUNT,
};

typedef struct GmgPhysicalDeviceProperties GmgPhysicalDeviceProperties;
struct GmgPhysicalDeviceProperties {
	union {
#ifdef GMG_ENABLE_VULKAN
		struct {
			uint32_t api_version;
		} vulkan;
#endif
	};
	uint32_t driver_version;
	uint32_t vendor_id;
	uint32_t device_id;
	GmgPhysicalDeviceType device_type;
	char* device_name;
};

typedef uint64_t GmgPhysicalDeviceFeatureFlags;
#define GmgPhysicalDeviceFeatureFlags_has(flags, feature) ((flags & (1 << feature)) != 0)
#define GmgPhysicalDeviceFeatureFlags_insert(flags, feature) (flags |= (1 << feature))

GmgResult gmg_physical_devices_get(GmgPhysicalDevice** physical_devices_out, uint32_t* physical_devices_count_out);
GmgResult gmg_physical_device_get(uint32_t physical_device_idx, GmgPhysicalDevice** out);
GmgResult gmg_physical_device_features(uint32_t physical_device_idx, GmgPhysicalDeviceFeatureFlags* flags_out);
GmgResult gmg_physical_device_properties(uint32_t physical_device_idx, GmgPhysicalDeviceProperties* properties_out);
GmgResult gmg_physical_device_surface_texture_format(uint32_t physical_device_idx, GmgSurface surface, GmgTextureFormat* format_out);

// ==========================================================
//
//
// Internal: Vulkan Allocator
//
//
// ==========================================================

#ifdef GMG_ENABLE_VULKAN

typedef uint8_t _GmgVulkanAllocType;
enum _GmgVulkanAllocType {
	_GmgVulkanAllocType_buffer,
	_GmgVulkanAllocType_image,
	_GmgVulkanAllocType_image_linear,
	_GmgVulkanAllocType_image_optimal,
	_GmgVulkanAllocType_COUNT,
};

typedef struct _GmgVulkanAlloc _GmgVulkanAlloc;
struct _GmgVulkanAlloc {
	uint64_t start: 48;
	uint64_t block_idx: 16; // used to get the block in the pool
	uint64_t end: 56;
	uint64_t memory_type_idx: 8; // used to get the pool
};

typedef_DasPoolElmtId(_GmgVulkanAllocId, 20);
typedef_DasPool(_GmgVulkanAllocId, _GmgVulkanAlloc);

typedef struct _GmgVulkanAllocResult _GmgVulkanAllocResult;
struct _GmgVulkanAllocResult {
	_GmgVulkanAllocId id;
	VkDeviceMemory device_memory;
	uint64_t device_memory_offset;
	void* memory_mapped_data;
};

typedef struct _GmgVulkanFreeRange _GmgVulkanFreeRange;
struct _GmgVulkanFreeRange {
	uint64_t start;
	uint64_t end; // exclusive
};
typedef_DasStk(_GmgVulkanFreeRange);

typedef struct _GmgVulkanBlock _GmgVulkanBlock;
struct _GmgVulkanBlock {
	DasStk(_GmgVulkanFreeRange) free_ranges;
	VkDeviceMemory device_memory;
	uint64_t remaining_size;
	uint64_t size;
	void* mapped_memory;
};

#define _GmgVulkanBlockKey_memory_location_MASK  0x0000000000000003
#define _GmgVulkanBlockKey_memory_location_SHIFT 0
#define _GmgVulkanBlockKey_type_MASK             0x000000000000000c
#define _GmgVulkanBlockKey_type_SHIFT            2
#define _GmgVulkanBlockKey_remaining_size_MASK   0x7ffffffffffffff0
#define _GmgVulkanBlockKey_remaining_size_SHIFT  4
#define _GmgVulkanBlockKey_is_allocated_MASK     0x8000000000000000
#define _GmgVulkanBlockKey_is_allocated_SHIFT    63

typedef uint64_t _GmgVulkanBlockKey;
/*
struct {
	uint64_t memory_location: 2; // GmgMemoryLocation
	uint64_t type: 2; // _GmgVulkanAllocType
	uint64_t remaining_size: 59;
	uint64_t is_allocated: 1;
}
*/

typedef struct _GmgVulkanPool _GmgVulkanPool;
struct _GmgVulkanPool {
	_GmgVulkanBlockKey* keys;
	_GmgVulkanBlock* blocks;
	uint16_t blocks_count;
	uint16_t blocks_cap;
};

typedef struct _GmgVulkanAllocator _GmgVulkanAllocator;
struct _GmgVulkanAllocator {
	_GmgVulkanPool pools[VK_MAX_MEMORY_TYPES];
	DasPool(_GmgVulkanAllocId, _GmgVulkanAlloc) allocations;
	uint64_t min_block_size_device_local_memory;
	uint64_t min_block_size_host_visible_memory;
};

typedef struct _GmgVulkanAllocArgs _GmgVulkanAllocArgs;
struct _GmgVulkanAllocArgs {
	uint64_t size;
	uint64_t align;
	uint32_t memory_type_bits;
	uint16_t _block_idx;
	GmgMemoryLocation* memory_location;
	_GmgVulkanAllocType type;
	uint8_t _memory_type_idx;
};

// ==========================================================
//
//
// Internal: Vulkan Memory Stager
//
//
// ==========================================================

typedef struct GmgTextureArea GmgTextureArea;
struct GmgTextureArea {
	uint32_t offset_x;
	uint32_t offset_y;
	uint32_t offset_z;
	uint32_t width;
	uint32_t height;
	uint32_t depth;
	uint32_t mip_level;
	uint32_t array_layer;
	uint32_t array_layers_count;
};

typedef struct _GmgVulkanStagedBuffer _GmgVulkanStagedBuffer;
struct _GmgVulkanStagedBuffer {
	VkBuffer dst_buffer;
	uint64_t dst_buffer_offset;
	uint32_t size;
	uint32_t src_buffer_offset;
};
typedef_DasStk(_GmgVulkanStagedBuffer);

typedef struct _GmgVulkanStagedTexture _GmgVulkanStagedTexture;
struct _GmgVulkanStagedTexture {
	VkImage dst_image;
	uint32_t src_buffer_offset;
	GmgTextureArea area;
};
typedef_DasStk(_GmgVulkanStagedTexture);

typedef struct _GmgVulkanStagingBuffer _GmgVulkanStagingBuffer;
struct _GmgVulkanStagingBuffer {
	VkBuffer buffer;
	void* memory_mapped_data;
	_GmgVulkanAllocId device_memory_alloc_id;
	uint32_t position;
	GmgBool needs_flush;
	DasStk(_GmgVulkanStagedBuffer) staged_buffers;
	DasStk(_GmgVulkanStagedTexture) staged_textures;
};
typedef_DasStk(_GmgVulkanStagingBuffer);

typedef struct _GmgVulkanMemoryStager _GmgVulkanMemoryStager;
struct _GmgVulkanMemoryStager {
	DasStk(_GmgVulkanStagingBuffer) buffers;
	uint32_t buffer_size;
	uint32_t filled_buffers_count;
};

#endif // GMG_ENABLE_VULKAN

// ==========================================================
//
//
// Logical Device
//
//
// ==========================================================

typedef union _GmgLogicalDeviceObject _GmgLogicalDeviceObject;
typedef_DasPool(_GmgLogicalDeviceObjectId, _GmgLogicalDeviceObject);
typedef struct GmgLogicalDevice GmgLogicalDevice;
typedef struct GmgRenderPass GmgRenderPass;

typedef struct _GmgDescriptor _GmgDescriptor;
struct _GmgDescriptor {
	uint16_t binding_idx;
	uint16_t elmt_idx: 13;
	uint16_t type: 3; // GmgDescriptorType
	_GmgLogicalDeviceObjectId object_id;
	uint64_t buffer_start_idx;
};
typedef_DasStk(_GmgDescriptor);

typedef struct _GmgSetDescriptor _GmgSetDescriptor;
struct _GmgSetDescriptor {
	_GmgDescriptor base;
	union {
#ifdef GMG_ENABLE_VULKAN
		struct {
			VkDescriptorSet descriptor_set;
		} vulkan;
#endif
	};
};
typedef_DasStk(_GmgSetDescriptor);

typedef uint8_t _GmgVulkanQueueType;
enum {
	//
	// these queue types at worst only have a single queue for them selves. Or at best they share a queue with all the queue types.
	// WARNING: these must match the same bit index as there vulkan VkQueueFlags counterpart.
	_GmgVulkanQueueType_graphics = 0,
	_GmgVulkanQueueType_compute = 1,
	_GmgVulkanQueueType_transfer = 2,

	//
	// these are for asynchronous queue submission but are not required.
	// these are only populated if the primary queues all exist and there are still more queues of this type available.
	_GmgVulkanQueueType_async_compute,
	_GmgVulkanQueueType_async_transfer,

	_GmgVulkanQueueType_COUNT,
};

#define _GmgVulkanQueueType_primary_end _GmgVulkanQueueType_async_compute

typedef uint8_t _GmgVulkanFoundQueueFlags;
#define _GmgVulkanQueueType_to_flags(v) (1 << v)

typedef struct _GmgVulkanCommandBufferBuildTransfer _GmgVulkanCommandBufferBuildTransfer;
struct _GmgVulkanCommandBufferBuildTransfer {
	GmgLogicalDevice* logical_device;
	VkCommandBuffer command_buffer;
};

typedef struct _GmgVulkanCommandBufferBuildGraphics _GmgVulkanCommandBufferBuildGraphics;
struct _GmgVulkanCommandBufferBuildGraphics {
	GmgLogicalDevice* logical_device;
	GmgRenderPass* render_pass;
	VkCommandBuffer command_buffer;
};

typedef struct _GmgVulkanSubmit _GmgVulkanSubmit;
struct _GmgVulkanSubmit {
	VkFence fence;
	uint32_t command_buffers_count;
};

typedef_DasStk(VkCommandBuffer);
typedef_DasStk(VkFence);
typedef_DasDeque(VkCommandBuffer);
typedef_DasDeque(_GmgVulkanSubmit);

typedef struct _GmgVulkanFlushMemoryRange _GmgVulkanFlushMemoryRange;
struct _GmgVulkanFlushMemoryRange {
	VkDeviceMemory memory;
	VkDeviceSize offset;
	VkDeviceSize size;
};
typedef_DasStk(_GmgVulkanFlushMemoryRange);

typedef struct GmgLogicalDevice GmgLogicalDevice;
struct GmgLogicalDevice {
	uint32_t physical_device_idx;
	DasPool(_GmgLogicalDeviceObjectId, _GmgLogicalDeviceObject) object_pool;
	DasStk(_GmgSetDescriptor) set_descriptors_queue;
	DasLinearAlctor temp_alctor;
	DasStk(GmgResult) job_results;
	union {
#ifdef GMG_ENABLE_VULKAN
		struct {
			VkDevice handle;
			VkSemaphore semaphore_present;
			VkSemaphore semaphore_render;
			VkDescriptorSetLayout dummy_descriptor_set_layout;
			VkQueue queue_type_to_queue[_GmgVulkanQueueType_COUNT];
			uint32_t queue_type_to_queue_family_idx[_GmgVulkanQueueType_COUNT];
			VkCommandPool graphics_command_pool;
			DasDeque(VkCommandBuffer) graphics_command_buffers_being_executed;
			DasDeque(_GmgVulkanSubmit) graphics_submits;
			DasStk(VkCommandBuffer) graphics_command_buffers_available;
			DasStk(VkFence) graphics_fences_available;
			_GmgVulkanAllocator allocator;
			_GmgVulkanMemoryStager memory_stager;
			DasStk(_GmgVulkanFlushMemoryRange) flush_memory_ranges;
		} vulkan;
#endif
	};
};

GmgResult _GmgLogicalDevice_vulkan_queue_family_indices_get(GmgLogicalDevice* logical_device, VkQueueFlags queue_flags, uint32_t queue_family_indices_out[3], uint16_t* queue_family_indices_count_out);

typedef struct GmgLogicalDeviceCreateArgs GmgLogicalDeviceCreateArgs;
struct GmgLogicalDeviceCreateArgs {
	GmgPhysicalDeviceFeatureFlags feature_flags;
	uint32_t object_pool_reserve_cap;
	uint32_t object_pool_grow_count;
	union {
#ifdef GMG_ENABLE_VULKAN
		struct {
			uint64_t allocator_min_block_size_device_local_memory;
			uint64_t allocator_min_block_size_host_visible_memory;
			uint32_t memory_stager_buffer_size;
		} vulkan;
#endif
	};
};

GmgResult gmg_logical_device_init(uint32_t physical_device_idx, GmgLogicalDeviceCreateArgs* create_args, GmgLogicalDevice** out);
GmgResult gmg_logical_device_deinit(GmgLogicalDevice* logical_device);

GmgResult gmg_logical_device_submit(GmgLogicalDevice* logical_device, GmgRenderPassId* ordered_render_pass_ids, uint32_t render_passes_count);

DasAlctor gmg_logical_device_temp_alctor(GmgLogicalDevice* logical_device);

// ==========================================================
//
//
// Sampler
//
//
// ==========================================================

typedef uint8_t GmgFilter;
enum {
	GmgFilter_nearest,
	GmgFilter_linear,
	GmgFilter_cubic_img,
};

typedef uint8_t GmgSamplerMipmapMode;
enum {
	GmgSamplerMipmapMode_nearest,
	GmgSamplerMipmapMode_linear,
};

typedef uint8_t GmgSamplerAddressMode;
enum {
	GmgSamplerAddressMode_repeat,
	GmgSamplerAddressMode_mirrored_repeat,
	GmgSamplerAddressMode_clamp_to_edge,
	GmgSamplerAddressMode_clamp_to_border,
	GmgSamplerAddressMode_mirror_clamp_to_edge,
};

typedef uint8_t GmgBorderColor;
enum {
	GmgBorderColor_float_transparent_black,
	GmgBorderColor_int_transparent_black,
	GmgBorderColor_float_opaque_black,
	GmgBorderColor_int_opaque_black,
	GmgBorderColor_float_opaque_white,
	GmgBorderColor_int_opaque_white,
};

typedef struct GmgSampler GmgSampler;
struct GmgSampler {
	union {
#ifdef GMG_ENABLE_VULKAN
		struct {
			VkSampler handle;
		} vulkan;
#endif
	};
	GmgFilter mag_filter;
	GmgFilter min_filter;
	GmgSamplerMipmapMode mipmap_mode;
	GmgSamplerAddressMode address_mode_u;
	GmgSamplerAddressMode address_mode_v;
	GmgSamplerAddressMode address_mode_w;
	float mip_lod_bias;
	float max_anisotropy;
	float min_lod;
	float max_lod;
	GmgCompareOp compare_op;
	GmgBorderColor border_color;
};

typedef uint8_t GmgSamplerCreateFlags;
enum {
	GmgSamplerCreateFlags_anisotropy_enable = 0x1,
	GmgSamplerCreateFlags_compare_enable = 0x2,
	GmgSamplerCreateFlags_unnormalized_coordinates = 0x4,
};

typedef struct GmgSamplerCreateArgs GmgSamplerCreateArgs;
struct GmgSamplerCreateArgs {
	GmgSamplerCreateFlags flags;
	GmgFilter mag_filter;
	GmgFilter min_filter;
	GmgSamplerMipmapMode mipmap_mode;
	GmgSamplerAddressMode address_mode_u;
	GmgSamplerAddressMode address_mode_v;
	GmgSamplerAddressMode address_mode_w;
	float mip_lod_bias;
	float max_anisotropy;
	float min_lod;
	float max_lod;
	GmgCompareOp compare_op;
	GmgBorderColor border_color;
};

GmgResult gmg_sampler_init(GmgLogicalDevice* logical_device, GmgSamplerCreateArgs* create_args, GmgSamplerId* out);
GmgResult gmg_sampler_deinit(GmgLogicalDevice* logical_device, GmgSamplerId id);
GmgResult gmg_sampler_get(GmgLogicalDevice* logical_device, GmgSamplerId id, GmgSampler** out);

// ==========================================================
//
//
// Texture
//
//
// ==========================================================

typedef uint8_t GmgTextureType;
enum {
	GmgTextureType_1d,
	GmgTextureType_1d_array,
	GmgTextureType_2d,
	GmgTextureType_2d_array,
	GmgTextureType_3d,
	GmgTextureType_cube,
	GmgTextureType_cube_array,
};

typedef uint8_t GmgTextureFlags;
enum {
	GmgTextureFlags_sampled = 0x1,
	GmgTextureFlags_color_attachment = 0x2,
	GmgTextureFlags_depth_stencil_attachment = 0x4,
	GmgTextureFlags_input_attachment = 0x8,
	GmgTextureFlags_used_for_graphics = 0x10,
	GmgTextureFlags_used_for_compute = 0x20,
	GmgTextureFlags_swapchain = 0x40,
};

typedef struct GmgTexture GmgTexture;
struct GmgTexture {
	GmgTextureType type;
	GmgTextureFormat format;
	GmgTextureFlags flags;
	uint32_t width;
	uint32_t height;
	uint32_t depth;
	uint32_t mip_levels;
	uint32_t array_layers_count;
	GmgSampleCount samples;
	union {
#ifdef GMG_ENABLE_VULKAN
		struct {
			VkImage image;
			VkImageView image_view;
			void* memory_mapped_data;
			_GmgVulkanAllocId device_memory_alloc_id;
		} vulkan;
#endif
	};
};

typedef struct GmgTextureCreateArgs GmgTextureCreateArgs;
struct GmgTextureCreateArgs {
	GmgTextureType type;
	GmgTextureFormat format;
	GmgTextureFlags flags;
	GmgSampleCount samples;
};

GmgResult gmg_texture_init(GmgLogicalDevice* logical_device, GmgTextureCreateArgs* create_args, GmgTextureId* out);
GmgResult gmg_texture_deinit(GmgLogicalDevice* logical_device, GmgTextureId id);
GmgResult gmg_texture_get(GmgLogicalDevice* logical_device, GmgTextureId id, GmgTexture** out);

GmgResult gmg_texture_resize(GmgLogicalDevice* logical_device, GmgTextureId id, uint32_t width, uint32_t height, uint32_t depth, uint32_t mip_levels, uint32_t array_layers_count);
GmgResult gmg_texture_get_write_buffer(GmgLogicalDevice* logical_device, GmgTextureId id, GmgTextureArea* area, void** data_out);

// ==========================================================
//
//
// Descriptors
//
//
// ==========================================================

typedef struct GmgDescriptorBinding GmgDescriptorBinding;
struct GmgDescriptorBinding {
	GmgDescriptorType type;
	GmgShaderStageFlags shader_stage_flags;
	uint16_t binding_idx;
	uint16_t count;
	uint16_t size;
	uint16_t _dynamic_idx; // internally set
	uint16_t _start_idx; // internally set
};

typedef struct GmgDescriptorSetLayout GmgDescriptorSetLayout;
struct GmgDescriptorSetLayout {
	GmgDescriptorBinding* descriptor_bindings;
	uint16_t descriptor_bindings_count;
	uint16_t descriptors_count;
	uint16_t dynamic_descriptors_count;
	union {
#ifdef GMG_ENABLE_VULKAN
		struct {
			VkDescriptorSetLayout handle;
			VkDescriptorPoolSize* pool_sizes;
			uint32_t pool_sizes_count;
		} vulkan;
#endif
	};
};

typedef struct GmgDescriptorSetLayoutCreateArgs GmgDescriptorSetLayoutCreateArgs;
struct GmgDescriptorSetLayoutCreateArgs {
	GmgDescriptorBinding* descriptor_bindings;
	uint16_t descriptor_bindings_count;
};

typedef struct GmgDescriptorSet GmgDescriptorSet;
struct GmgDescriptorSet {
	GmgDescriptorSetAllocatorId allocator_id;
	GmgDescriptorSetLayoutId layout_id;
	union {
#ifdef GMG_ENABLE_VULKAN
		struct {
			VkDescriptorSet handle;
		} vulkan;
#endif
	};
};

typedef struct _GmgDescriptorSetLayoutAllocator _GmgDescriptorSetLayoutAllocator;
struct _GmgDescriptorSetLayoutAllocator {
	GmgDescriptorSetLayoutId layout_id;
	uint32_t cap;
	uint32_t count;
	uint32_t pool_idx;
	DasStk(GmgDescriptorSetId) allocated_set_ids;
	union {
#ifdef GMG_ENABLE_VULKAN
		struct {
			DasStk(VkDescriptorPool) pools;
		} vulkan;
#endif
	};
};

typedef struct GmgDescriptorSetAllocator GmgDescriptorSetAllocator;
struct GmgDescriptorSetAllocator {
	GmgDescriptorSetLayoutId* layout_ids;
	_GmgDescriptorSetLayoutAllocator* layout_allocators;
	uint32_t layouts_count;
};

typedef struct GmgDescriptorSetLayoutAllocatorCreateArgs GmgDescriptorSetLayoutAllocatorCreateArgs;
struct GmgDescriptorSetLayoutAllocatorCreateArgs {
	GmgDescriptorSetLayoutId id;
	uint32_t pool_cap;
};

typedef struct GmgDescriptorSetAllocatorCreateArgs GmgDescriptorSetAllocatorCreateArgs;
struct GmgDescriptorSetAllocatorCreateArgs {
	GmgDescriptorSetLayoutAllocatorCreateArgs* layouts;
	uint32_t layouts_count;
};

GmgResult gmg_descriptor_set_layout_init(GmgLogicalDevice* logical_device, GmgDescriptorSetLayoutCreateArgs* create_args, GmgDescriptorSetLayoutId* id_out);
GmgResult gmg_descriptor_set_layout_deinit(GmgLogicalDevice* logical_device, GmgDescriptorSetLayoutId id);
GmgResult gmg_descriptor_set_layout_get(GmgLogicalDevice* logical_device, GmgDescriptorSetLayoutId id, GmgDescriptorSetLayout** out);

GmgResult gmg_descriptor_set_allocator_init(GmgLogicalDevice* logical_device, GmgDescriptorSetAllocatorCreateArgs* create_args, GmgDescriptorSetAllocatorId* id_out);
GmgResult gmg_descriptor_set_allocator_deinit(GmgLogicalDevice* logical_device, GmgDescriptorSetAllocatorId id);
GmgResult gmg_descriptor_set_allocator_get(GmgLogicalDevice* logical_device, GmgDescriptorSetAllocatorId id, GmgDescriptorSetAllocator** out);
GmgResult gmg_descriptor_set_allocator_reset(GmgLogicalDevice* logical_device, GmgDescriptorSetAllocatorId id, uint32_t keep_pools_count);
GmgResult gmg_descriptor_set_allocator_alloc(GmgLogicalDevice* logical_device, GmgDescriptorSetAllocatorId id, GmgDescriptorSetLayoutId layout_id, GmgDescriptorSetId* out);

GmgResult gmg_descriptor_set_get(GmgLogicalDevice* logical_device, GmgDescriptorSetId id, GmgDescriptorSet** out);

GmgResult gmg_descriptor_set_set_descriptor(GmgLogicalDevice* logical_device, GmgDescriptorSetId id, uint16_t binding_idx, uint16_t elmt_idx, _GmgLogicalDeviceObjectId object_id, GmgDescriptorType type, uint64_t buffer_start_idx);
GmgResult gmg_descriptor_set_set_sampler(GmgLogicalDevice* logical_device, GmgDescriptorSetId id, uint16_t binding_idx, uint16_t elmt_idx, GmgSamplerId sampler_id);
GmgResult gmg_descriptor_set_set_texture(GmgLogicalDevice* logical_device, GmgDescriptorSetId id, uint16_t binding_idx, uint16_t elmt_idx, GmgTextureId texture_id);
GmgResult gmg_descriptor_set_set_uniform_buffer(GmgLogicalDevice* logical_device, GmgDescriptorSetId id, uint16_t binding_idx, uint16_t elmt_idx, GmgBufferId buffer_id, uint64_t buffer_start_idx);
GmgResult gmg_descriptor_set_set_storage_buffer(GmgLogicalDevice* logical_device, GmgDescriptorSetId id, uint16_t binding_idx, uint16_t elmt_idx, GmgBufferId buffer_id, uint64_t buffer_start_idx);

// ==========================================================
//
//
// Shader
//
//
// ==========================================================

typedef uint8_t GmgShaderFormat;
enum {
	GmgShaderFormat_none,
	GmgShaderFormat_spir_v,
	GmgShaderFormat_hlsl,
	GmgShaderFormat_msl,
};

typedef struct GmgShaderModule GmgShaderModule;
struct GmgShaderModule {
	union {
#ifdef GMG_ENABLE_VULKAN
		struct {
			VkShaderModule handle;
		} vulkan;
#endif
	};
	uint8_t* code;
	uint32_t code_size;
	GmgShaderFormat format;
};

typedef struct GmgShaderStage GmgShaderStage;
struct GmgShaderStage {
	union {
		struct {
			GmgShaderModuleId module_id;
			const char* entry_point_name;
		} spir_v;
		struct {
			uint8_t* code;
			uint32_t code_size;
		} hlsl;
		struct {
			const char* shader_name;
		} msl;
	};
	GmgShaderFormat format;
};

typedef uint8_t GmgShaderType;
enum {
	GmgShaderType_graphics,
	GmgShaderType_graphics_mesh,
	GmgShaderType_compute,
	GmgShaderType_COUNT,
};
extern uint8_t GmgShaderType_stages_counts[GmgShaderType_COUNT];

#define GmgShader_stages_count 5
#define GmgShader_stages_count_graphics 5
#define GmgShader_stages_count_graphics_mesh 3
#define GmgShader_stages_count_compute 1
typedef struct GmgShader GmgShader;
struct GmgShader {
	GmgDescriptorSetLayoutId scene_descriptor_set_layout_id;
	GmgDescriptorSetLayoutId material_descriptor_set_layout_id;
	GmgDescriptorSetLayoutId draw_cmd_descriptor_set_layout_id;
	uint16_t push_constants_size;
	GmgShaderStageFlags push_constants_shader_stage_flags;
	GmgShaderType type;
	GmgShaderStage stages[GmgShader_stages_count];
	union {
#ifdef GMG_ENABLE_VULKAN
		struct {
			VkPipelineLayout pipeline_layout;
		} vulkan;
#endif
	};
};

typedef struct GmgShaderModuleCreateArgs GmgShaderModuleCreateArgs;
struct GmgShaderModuleCreateArgs {
	GmgShaderFormat format;
	uint8_t* code;
	uint32_t code_size;
};

typedef struct GmgShaderCreateArgs GmgShaderCreateArgs;
struct GmgShaderCreateArgs {
	GmgDescriptorSetLayoutId scene_descriptor_set_layout_id;
	GmgDescriptorSetLayoutId material_descriptor_set_layout_id;
	GmgDescriptorSetLayoutId draw_cmd_descriptor_set_layout_id; // nullable
	uint16_t push_constants_size;
	GmgShaderStageFlags push_constants_shader_stage_flags;
	GmgShaderType type;
	union {
		GmgShaderStage compute;
		struct {
			GmgShaderStage vertex;
			GmgShaderStage tessellation_control;
			GmgShaderStage tessellation_evaluation;
			GmgShaderStage geometry;
			GmgShaderStage fragment;
		} graphics;
		struct {
			GmgShaderStage task;
			GmgShaderStage mesh;
			GmgShaderStage fragment;
		} graphics_mesh;
		GmgShaderStage array[GmgShader_stages_count];
	} stages;
};

GmgResult gmg_shader_module_init(GmgLogicalDevice* logical_device, GmgShaderModuleCreateArgs* create_args, GmgShaderModuleId* id_out);
GmgResult gmg_shader_module_deinit(GmgLogicalDevice* logical_device, GmgShaderModuleId id);
GmgResult gmg_shader_module_get(GmgLogicalDevice* logical_device, GmgShaderModuleId id, GmgShaderModule** out);

GmgResult gmg_shader_init(GmgLogicalDevice* logical_device, GmgShaderCreateArgs* create_args, GmgShaderId* id_out);
GmgResult gmg_shader_deinit(GmgLogicalDevice* logical_device, GmgShaderId id);
GmgResult gmg_shader_get(GmgLogicalDevice* logical_device, GmgShaderId id, GmgShader** out);

// ==========================================================
//
//
// Material Spec Cache
//
//
// ==========================================================

typedef struct GmgMaterialSpecCache GmgMaterialSpecCache;
struct GmgMaterialSpecCache {
	union {
#ifdef GMG_ENABLE_VULKAN
		struct {
			VkPipelineCache handle;
		} vulkan;
#endif
	};
};

typedef struct GmgMaterialSpecCacheCreateArgs GmgMaterialSpecCacheCreateArgs;
struct GmgMaterialSpecCacheCreateArgs {
	void* data;
	size_t size;
};

GmgResult gmg_material_spec_cache_init(GmgLogicalDevice* logical_device, GmgMaterialSpecCacheCreateArgs* create_args, GmgMaterialSpecCacheId* id_out);
GmgResult gmg_material_spec_cache_deinit(GmgLogicalDevice* logical_device, GmgMaterialSpecCacheId id);
GmgResult gmg_material_spec_cache_get(GmgLogicalDevice* logical_device, GmgMaterialSpecCacheId id, GmgMaterialSpecCache** out);
GmgResult gmg_material_spec_cache_get_data(GmgLogicalDevice* logical_device, GmgMaterialSpecCacheId id, void** data_out, uintptr_t* size_out);

// ==========================================================
//
//
// Material Spec
//
//
// ==========================================================

typedef struct GmgMaterialSpec GmgMaterialSpec;
struct GmgMaterialSpec {
	GmgShaderId shader_id;
	GmgRenderPassId render_pass_id;
	GmgVertexLayoutId vertex_layout_id;
	union {
#ifdef GMG_ENABLE_VULKAN
		struct {
			VkPipeline pipeline;
		} vulkan;
#endif
	};
};

typedef struct GmgMaterialSpecCreateArgs GmgMaterialSpecCreateArgs;
struct GmgMaterialSpecCreateArgs {
	GmgShaderId shader_id;
	GmgRenderPassId render_pass_id;
	GmgRenderState* render_state;
	GmgVertexLayoutId vertex_layout_id;
	GmgMaterialSpecCacheId cache_id;
};

GmgResult gmg_material_spec_init(GmgLogicalDevice* logical_device, GmgMaterialSpecCreateArgs* create_args, GmgMaterialSpecId* id_out);
GmgResult gmg_material_spec_deinit(GmgLogicalDevice* logical_device, GmgMaterialSpecId id);
GmgResult gmg_material_spec_get(GmgLogicalDevice* logical_device, GmgMaterialSpecId id, GmgMaterialSpec** out);

// ==========================================================
//
//
// Material
//
//
// ==========================================================

typedef struct GmgMaterial GmgMaterial;
struct GmgMaterial {
	GmgMaterialSpecId spec_id;
	uint32_t* dynamic_descriptor_offsets;
	uint16_t dynamic_descriptors_count;
	GmgDescriptorSetId descriptor_set_id;
};

typedef struct GmgMaterialCreateArgs GmgMaterialCreateArgs;
struct GmgMaterialCreateArgs {
	GmgMaterialSpecId spec_id;
	GmgDescriptorSetAllocatorId descriptor_set_allocator_id;
};

GmgResult gmg_material_init(GmgLogicalDevice* logical_device, GmgMaterialCreateArgs* create_args, GmgMaterialId* id_out);
GmgResult gmg_material_deinit(GmgLogicalDevice* logical_device, GmgMaterialId id);
GmgResult gmg_material_get(GmgLogicalDevice* logical_device, GmgMaterialId id, GmgMaterial** out);

GmgResult gmg_material_set_descriptor(GmgLogicalDevice* logical_device, GmgMaterialId id, uint16_t binding_idx, uint16_t elmt_idx, _GmgLogicalDeviceObjectId object_id, GmgDescriptorType type, uint64_t buffer_start_idx);
GmgResult gmg_material_set_sampler(GmgLogicalDevice* logical_device, GmgMaterialId id, uint16_t binding_idx, uint16_t elmt_idx, GmgSamplerId sampler_id);
GmgResult gmg_material_set_texture(GmgLogicalDevice* logical_device, GmgMaterialId id, uint16_t binding_idx, uint16_t elmt_idx, GmgTextureId texture_id);
GmgResult gmg_material_set_uniform_buffer(GmgLogicalDevice* logical_device, GmgMaterialId id, uint16_t binding_idx, uint16_t elmt_idx, GmgBufferId buffer_id, uint64_t buffer_start_idx);
GmgResult gmg_material_set_storage_buffer(GmgLogicalDevice* logical_device, GmgMaterialId id, uint16_t binding_idx, uint16_t elmt_idx, GmgBufferId buffer_id, uint64_t buffer_start_idx);
GmgResult gmg_material_set_dynamic_descriptor_offset(GmgLogicalDevice* logical_device, GmgMaterialId id, uint16_t binding_idx, uint16_t elmt_idx, uint32_t dynamic_offset);

// ==========================================================
//
//
// Buffer
//
//
// ==========================================================

typedef uint8_t GmgBufferType;
enum {
	GmgBufferType_vertex,
	GmgBufferType_index,
	GmgBufferType_uniform,
	GmgBufferType_storage,
};

typedef uint32_t GmgBufferFlags;
enum {
	GmgBufferFlags_used_for_graphics = 0x1,
	GmgBufferFlags_used_for_compute = 0x2,
	GmgBufferFlags_indirect_draw = 0x4,
};

typedef struct GmgBuffer GmgBuffer;
struct GmgBuffer {
	GmgBufferFlags flags;
	GmgBufferType type;
	GmgMemoryLocation memory_location;
	uint64_t elmts_count;
	uint32_t elmt_size;
	union {
#ifdef GMG_ENABLE_VULKAN
		struct {
			VkBuffer handle;
			void* memory_mapped_data;
			_GmgVulkanAllocId device_memory_alloc_id;
			GmgBool needs_flush;
		} vulkan;
#endif
	};
	union {
		struct {
			GmgVertexLayoutId layout_id;
			uint16_t binding_idx;
		} vertex;
		struct {
			GmgIndexType type;
		} index;
	};
};

typedef struct GmgBufferCreateArgs GmgBufferCreateArgs;
struct GmgBufferCreateArgs {
	GmgBufferFlags flags;
	GmgBufferType type;
	GmgMemoryLocation memory_location;
	union {
		struct {
			GmgVertexLayoutId layout_id;
			uint16_t binding_idx;
		} vertex;
		struct {
			GmgIndexType type;
		} index;
		struct {
			uint32_t elmt_size;
		} uniform;
		struct {
			uint32_t elmt_size;
		} storage;
	};
};

GmgResult gmg_buffer_init(GmgLogicalDevice* logical_device, GmgBufferCreateArgs* create_args, GmgBufferId* id_out);
GmgResult gmg_buffer_deinit(GmgLogicalDevice* logical_device, GmgBufferId id);
GmgResult gmg_buffer_get(GmgLogicalDevice* logical_device, GmgBufferId id, GmgBuffer** id_out);

GmgResult gmg_buffer_resize(GmgLogicalDevice* logical_device, GmgBufferId id, uint64_t elmts_count);
GmgResult gmg_buffer_get_read_buffer(GmgLogicalDevice* logical_device, GmgBufferId id, uint64_t start_idx, uintptr_t elmts_count, const void** data_out);
GmgResult gmg_buffer_get_write_buffer(GmgLogicalDevice* logical_device, GmgBufferId id, uint64_t start_idx, uintptr_t elmts_count, void** data_out);

// ==========================================================
//
//
// Vertex Array
//
//
// ==========================================================

typedef struct GmgVertexArray GmgVertexArray;
struct GmgVertexArray {
	GmgVertexLayoutId layout_id;
	GmgBufferId* vertex_buffer_ids;
	uint16_t vertex_buffers_count;
	GmgBufferId index_buffer_id; // optional
};


typedef struct GmgVertexArrayCreateArgs GmgVertexArrayCreateArgs;
struct GmgVertexArrayCreateArgs {
	GmgVertexLayoutId layout_id;
	GmgBufferId* vertex_buffer_ids;
	uint16_t vertex_buffers_count;
	GmgBufferId index_buffer_id; // optional
};

GmgResult gmg_vertex_array_init(GmgLogicalDevice* logical_device, GmgVertexArrayCreateArgs* create_args, GmgVertexArrayId* id_out);
GmgResult gmg_vertex_array_deinit(GmgLogicalDevice* logical_device, GmgVertexArrayId id);
GmgResult gmg_vertex_array_get(GmgLogicalDevice* logical_device, GmgVertexArrayId id, GmgVertexArray** out);

// ==========================================================
//
//
// Render Pass Layout
//
//
// ==========================================================

typedef struct GmgAttachmentLayout GmgAttachmentLayout;
struct GmgAttachmentLayout {
	GmgTextureFormat format;
	GmgSampleCount samples_count;
	GmgBool present;
};

typedef struct GmgRenderPassLayout GmgRenderPassLayout;
struct GmgRenderPassLayout {
	GmgAttachmentLayout* attachments;
	uint32_t attachments_count;
	union {
#ifdef GMG_ENABLE_VULKAN
		struct {
			VkRenderPass handle;
		} vulkan;
#endif
	};
};

typedef struct GmgRenderPassLayoutCreateArgs GmgRenderPassLayoutCreateArgs;
struct GmgRenderPassLayoutCreateArgs {
	GmgAttachmentLayout* attachments;
	uint32_t attachments_count;
};

GmgResult gmg_render_pass_layout_init(GmgLogicalDevice* logical_device, GmgRenderPassLayoutCreateArgs* create_args, GmgRenderPassLayoutId* id_out);
GmgResult gmg_render_pass_layout_deinit(GmgLogicalDevice* logical_device, GmgRenderPassLayoutId id);
GmgResult gmg_render_pass_layout_get(GmgLogicalDevice* logical_device, GmgRenderPassLayoutId id, GmgRenderPassLayout** out);

// ==========================================================
//
//
// Render Pass
//
//
// ==========================================================

typedef struct _GmgDrawCmd _GmgDrawCmd;
struct _GmgDrawCmd {
	void* push_constants_data;
	uint16_t push_constants_offset;
	uint16_t push_constants_size;
	uint32_t depth;
	GmgMaterialId material_id;
	GmgVertexArrayId vertex_array_id;
	GmgDescriptorSetId descriptor_set_id;
	uint32_t dynamic_descriptor_offsets_start_idx;
	uint16_t dynamic_descriptor_offsets_count;
	uint32_t instances_start_idx;
	uint32_t instances_count;
	union {
		struct {
			uint32_t vertices_start_idx;
			uint32_t vertices_count;
		} vertexed;
		struct {
			uint32_t vertices_start_idx;
			uint32_t indices_start_idx;
			uint32_t indices_count;
		} indexed;
	};
};
typedef_DasStk(_GmgDrawCmd);

typedef uint8_t GmgDrawOrder;
enum {
	GmgDrawOrder_material_spec_cmd_idx,
	GmgDrawOrder_depth_cmd_idx,
	GmgDrawOrder_cmd_idx,
};
#define GmgRenderPass_draw_cmd_sort_key_cmd_idx_MASK 0xffffffff

typedef union GmgClearValue GmgClearValue;
union GmgClearValue {
	union {
		float float32[4];
		int32_t int32[4];
		uint32_t uint32[4];
	} color;
	struct {
		float depth;
		uint8_t stencil;
	};
};

typedef struct _GmgRenderPassDescriptorSet _GmgRenderPassDescriptorSet;
struct _GmgRenderPassDescriptorSet {
	GmgDescriptorSetId id;
	uint32_t descriptors_start_idx;
	uint16_t descriptors_count;
};
typedef_DasStk(_GmgRenderPassDescriptorSet);

typedef struct GmgRenderPass GmgRenderPass;
struct GmgRenderPass {
	GmgRenderPassLayoutId layout_id;
	GmgFrameBufferId frame_buffer_id;
	GmgClearValue* attachment_clear_values;
	uint16_t attachments_count;
	GmgDrawOrder draw_order;
	GmgViewport viewport;
	GmgRect2u scissor;
	uint32_t* scene_dynamic_descriptor_offsets;
	uint16_t scene_dynamic_descriptors_count;
	GmgDescriptorSetLayoutId scene_descriptor_set_layout_id;
	GmgDescriptorSetId scene_descriptor_set_id;
	GmgDescriptorSetAllocatorId draw_cmd_descriptor_set_allocator_id;
	DasStk(uint64_t) draw_cmd_descriptor_set_keys;
	DasStk(_GmgRenderPassDescriptorSet) draw_cmd_descriptor_sets;
	DasStk(_GmgDescriptor) draw_cmd_descriptors;
	DasStk(uint32_t) draw_cmd_dynamic_descriptor_offsets;
	uint32_t last_written_draw_cmd_descriptors;
	union {
#ifdef GMG_ENABLE_VULKAN
		struct {
			VkRenderPass handle;
		} vulkan;
#endif
	};
	DasStk(uint64_t) draw_cmd_sort_keys;
	DasStk(_GmgDrawCmd) draw_cmds;
};

typedef struct GmgDrawCmdBuilder GmgDrawCmdBuilder;
struct GmgDrawCmdBuilder {
	_GmgDrawCmd draw_cmd;
	DasStk(_GmgDescriptor) descriptors;
	DasStk(uint32_t) dynamic_descriptor_offsets;
	GmgDescriptorBinding* descriptor_bindings;
	uint16_t descriptor_bindings_count;
	uint16_t descriptors_count;
	uint16_t descriptors_been_set_count;
	GmgDescriptorSetLayoutId descriptor_set_layout_id;
	GmgLogicalDevice* logical_device;
	GmgRenderPass* render_pass;
};

typedef uint8_t GmgAttachmentLoadOp;
enum {
	GmgAttachmentLoadOp_preserve,
	GmgAttachmentLoadOp_clear,
	GmgAttachmentLoadOp_uninitialized,
};

typedef uint8_t GmgAttachmentStoreOp;
enum {
	GmgAttachmentStoreOp_preserve,
	GmgAttachmentStoreOp_discard,
};

typedef struct GmgAttachmentInfo GmgAttachmentInfo;
struct GmgAttachmentInfo {
	GmgAttachmentLoadOp load_op;
	GmgAttachmentStoreOp store_op;
	GmgAttachmentLoadOp stencil_load_op;
	GmgAttachmentStoreOp stencil_store_op;
};

typedef struct GmgRenderPassCreateArgs GmgRenderPassCreateArgs;
struct GmgRenderPassCreateArgs {
	GmgRenderPassLayoutId layout_id;
	GmgAttachmentInfo* attachments;
	GmgClearValue* attachment_clear_values;
	uint16_t attachments_count;

	GmgDescriptorSetLayoutId scene_descriptor_set_layout_id;
	GmgDescriptorSetLayoutAllocatorCreateArgs* draw_cmd_descriptor_set_layouts;
	uint32_t draw_cmd_descriptor_set_layouts_count;
};

GmgResult gmg_render_pass_init(GmgLogicalDevice* logical_device, GmgRenderPassCreateArgs* create_args, GmgRenderPassId* out);
GmgResult gmg_render_pass_deinit(GmgLogicalDevice* logical_device, GmgRenderPassId id);
GmgResult gmg_render_pass_get(GmgLogicalDevice* logical_device, GmgRenderPassId id, GmgRenderPass** out);

GmgResult gmg_render_pass_set_frame_buffer(GmgLogicalDevice* logical_device, GmgRenderPassId id, GmgFrameBufferId frame_buffer_id);
GmgResult gmg_render_pass_set_scene_descriptor_set(GmgLogicalDevice* logical_device, GmgRenderPassId id, GmgDescriptorSetId set_id);
GmgResult gmg_render_pass_set_scene_dynamic_descriptor_offset(GmgLogicalDevice* logical_device, GmgRenderPassId id, uint16_t binding_idx, uint16_t elmt_idx, uint32_t dynamic_offset);
GmgResult gmg_render_pass_set_draw_order(GmgLogicalDevice* logical_device, GmgRenderPassId id, GmgDrawOrder draw_order);
GmgResult gmg_render_pass_set_viewport(GmgLogicalDevice* logical_device, GmgRenderPassId id, GmgViewport* viewport);
GmgResult gmg_render_pass_set_scissor(GmgLogicalDevice* logical_device, GmgRenderPassId id, GmgRect2u* scissor);
GmgResult gmg_render_pass_set_attachment_clear_value(GmgLogicalDevice* logical_device, GmgRenderPassId id, uint16_t attachment_idx, GmgClearValue* clear_value);
GmgResult gmg_render_pass_clear_draw_cmds(GmgLogicalDevice* logical_device, GmgRenderPassId id, uint32_t keep_descriptor_pools_count);

GmgResult gmg_render_pass_draw_cmd_start(GmgLogicalDevice* logical_device, GmgRenderPassId id, GmgMaterialId material_id, GmgVertexArrayId vertex_array_id, GmgDrawCmdBuilder* builder_in_out);

GmgResult gmg_draw_cmd_set_push_constants(GmgDrawCmdBuilder* builder, void* data, uint32_t offset, uint32_t size);
GmgResult gmg_draw_cmd_set_depth(GmgDrawCmdBuilder* builder, uint32_t depth);
GmgResult gmg_draw_cmd_set_instances(GmgDrawCmdBuilder* builder, uint32_t instances_start_idx, uint32_t instances_count);

GmgResult gmg_draw_cmd_set_descriptor(GmgDrawCmdBuilder* builder, uint16_t binding_idx, uint16_t elmt_idx, _GmgLogicalDeviceObjectId object_id, GmgDescriptorType type, uint64_t buffer_start_idx);
GmgResult gmg_draw_cmd_set_sampler(GmgDrawCmdBuilder* builder, uint16_t binding_idx, uint16_t elmt_idx, GmgSamplerId sampler_id);
GmgResult gmg_draw_cmd_set_texture(GmgDrawCmdBuilder* builder, uint16_t binding_idx, uint16_t elmt_idx, GmgTextureId texture_id);
GmgResult gmg_draw_cmd_set_uniform_buffer(GmgDrawCmdBuilder* builder, uint16_t binding_idx, uint16_t elmt_idx, GmgBufferId buffer_id, uint64_t buffer_start_idx);
GmgResult gmg_draw_cmd_set_storage_buffer(GmgDrawCmdBuilder* builder, uint16_t binding_idx, uint16_t elmt_idx, GmgBufferId buffer_id, uint64_t buffer_start_idx);
GmgResult gmg_draw_cmd_set_dynamic_descriptor_offset(GmgDrawCmdBuilder* builder, uint16_t binding_idx, uint16_t elmt_idx, uint32_t dynamic_offset);

GmgResult gmg_draw_cmd_queue_vertexed(GmgDrawCmdBuilder* builder, uint32_t vertices_start_idx, uint32_t vertices_count);
GmgResult gmg_draw_cmd_queue_indexed(GmgDrawCmdBuilder* builder, uint32_t vertices_start_idx, uint32_t indices_start_idx, uint32_t indices_count);

// ==========================================================
//
//
// Frame Buffer
//
//
// ==========================================================

typedef struct GmgFrameBuffer GmgFrameBuffer;
struct GmgFrameBuffer {
	GmgTextureId* attachments;
	uint16_t attachments_count;
	uint16_t layers;
	GmgRenderPassLayoutId render_pass_layout_id;
	uint32_t width;
	uint32_t height;
	union {
#ifdef GMG_ENABLE_VULKAN
		struct {
			VkFramebuffer handle;
		} vulkan;
#endif
	};
};

typedef struct GmgFrameBufferCreateArgs GmgFrameBufferCreateArgs;
struct GmgFrameBufferCreateArgs {
	GmgTextureId* attachments;
	uint16_t attachments_count;
	uint16_t layers;
	GmgRenderPassLayoutId render_pass_layout_id;
	uint32_t width;
	uint32_t height;
};

GmgResult gmg_frame_buffer_init(GmgLogicalDevice* logical_device, GmgFrameBufferCreateArgs* create_args, GmgFrameBufferId* out);
GmgResult gmg_frame_buffer_deinit(GmgLogicalDevice* logical_device, GmgFrameBufferId id);
GmgResult gmg_frame_buffer_get(GmgLogicalDevice* logical_device, GmgFrameBufferId id, GmgFrameBuffer** out);

// ==========================================================
//
//
// Swapchain
//
//
// ==========================================================

typedef struct GmgSwapchain GmgSwapchain;
struct GmgSwapchain {
	union {
#ifdef GMG_ENABLE_VULKAN
		struct {
			VkSwapchainKHR handle;
			VkQueue present_queue;
			uint32_t image_idx;
		} vulkan;
#endif
	};
	GmgSurface surface;
	uint32_t width;
	uint32_t height;
	uint32_t array_layers_count;
	uint16_t min_textures_count;
	uint16_t textures_count;
	GmgBool vsync;
	GmgBool fifo;
	GmgTextureFormat format;
	GmgTextureId* texture_ids;
};

typedef struct GmgSwapchainCreateArgs GmgSwapchainCreateArgs;
struct GmgSwapchainCreateArgs {
	GmgSurface surface;
	uint32_t array_layers_count;
	uint16_t min_textures_count;
	GmgTextureFormat format;
};

GmgResult gmg_swapchain_init(GmgLogicalDevice* logical_device, GmgSwapchainCreateArgs* create_args, GmgSwapchainId* id_out);
GmgResult gmg_swapchain_deinit(GmgLogicalDevice* logical_device, GmgSwapchainId id);
GmgResult gmg_swapchain_get(GmgLogicalDevice* logical_device, GmgSwapchainId id, GmgSwapchain** out);

GmgResult gmg_swapchain_resize(GmgLogicalDevice* logical_device, GmgSwapchainId id, uint32_t width, uint32_t height, GmgBool vsync, GmgBool fifo);
GmgResult gmg_swapchain_get_next_texture_idx(GmgLogicalDevice* logical_device, GmgSwapchainId id, uint32_t* next_texture_idx_out);
GmgResult gmg_swapchain_present(GmgLogicalDevice* logical_device, GmgSwapchainId* swapchain_ids, uint32_t swapchains_count);

// ==========================================================
//
//
// Gmg
//
//
// ==========================================================

typedef enum GmgCallbackFn GmgCallbackFn;
enum GmgCallbackFn {
	GmgCallbackFn_execute_job,
	GmgCallbackFn_wait_for_jobs,
};

typedef int (*GmgJobStartFn)(void* args);
typedef union GmgCallbackArgs GmgCallbackArgs;
union GmgCallbackArgs {
	struct {
		GmgJobStartFn start_fn;
		void* args;
	} execute_job;
};

typedef uint8_t GmgDisplayManagerType;
enum {
	GmgDisplayManagerType_windows,
	GmgDisplayManagerType_macos,
	GmgDisplayManagerType_ios,
	GmgDisplayManagerType_xlib,
	GmgDisplayManagerType_xcb,
	GmgDisplayManagerType_wayland,
	GmgDisplayManagerType_android,
};

typedef GmgResult (*GmgCallbackImplFn)(GmgLogicalDevice* logical_device, GmgCallbackFn fn, GmgCallbackArgs* args, void* userdata);

typedef struct GmgSetup GmgSetup;
struct GmgSetup {
	GmgBackendType backend_type;
	char* application_name;
	uint32_t application_version;
	char* engine_name;
	uint32_t engine_version;
	GmgCallbackImplFn callback_impl_fn;
	void* callback_userdata;
	GmgDisplayManagerType display_manager_type;
	GmgBool debug_backend;
};

GmgResult gmg_init(GmgSetup* setup);
GmgResult gmg_vulkan_instance(VkInstance* out);

typedef_DasPool(GmgVertexLayoutId, GmgVertexLayout);

typedef GmgResult (*_GmgBackendImplFn)();

typedef struct _Gmg _Gmg;
struct _Gmg {
	GmgBackendType backend_type;
	GmgCallbackImplFn callback_impl_fn;
	void* callback_userdata;
	DasPool(GmgVertexLayoutId, GmgVertexLayout) vertex_layout_pool;
	union {
#ifdef GMG_ENABLE_VULKAN
		struct {
			VkInstance instance;
		} vulkan;
#endif
	};

	_GmgBackendImplFn* backend_impl_fns;

	GmgPhysicalDevice* physical_devices;
	uint16_t physical_devices_count;
};

extern _Gmg _gmg;

union _GmgLogicalDeviceObject {
	GmgSampler sampler;
	GmgTexture texture;
	GmgDescriptorSetLayout descriptor_set_layout;
	GmgDescriptorSet descriptor_set;
	GmgDescriptorSetAllocator descriptor_set_allocator;
	GmgShaderModule shader_module;
	GmgShader shader;
	GmgMaterialSpecCache material_spec_cache;
	GmgMaterialSpec material_spec;
	GmgMaterial material;
	GmgBuffer buffer;
	GmgVertexArray vertex_array;
	GmgRenderPassLayout render_pass_layout;
	GmgRenderPass render_pass;
	GmgFrameBuffer frame_buffer;
	GmgSwapchain swapchain;
};

#endif // GMG_H

