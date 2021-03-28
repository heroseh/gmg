
#ifndef DAS_H
#include "das.h"
#endif

#ifndef GMG_H
#include "gmg.h"
#endif

#define GMG_TODO_INTERNAL_ALLOCATOR DasAlctor_default
#define GMG_TODO_VULKAN_ALLOCATOR NULL

#ifdef GMG_ENABLE_VULKAN

#include <vulkan/vulkan_core.h>

#ifdef _WIN32
#include <vulkan/vulkan_win32.h>
#endif

#ifdef __APPLE__
#include <vulkan/vulkan_macos.h>
#include <vulkan/vulkan_ios.h>
#endif

#ifdef __unix__

#ifdef GMG_ENABLE_XLIB
typedef struct _XDisplay Display;
typedef uintptr_t Window;
typedef uintptr_t VisualID;
#include <vulkan/vulkan_xlib.h>
#endif

#ifdef GMG_ENABLE_XCB
typedef struct xcb_connection_t xcb_connection_t;
typedef uint32_t xcb_window_t;
typedef uint32_t xcb_visualid_t;
#include <vulkan/vulkan_xcb.h>
#endif

#ifdef GMG_ENABLE_WAYLAND
struct wl_display;
struct wl_surface;
#include <vulkan/vulkan_wayland.h>
#endif

#endif // __unix__

#ifdef __ANDROID__
#include <vulkan/vulkan_android.h>
#endif

#endif // GMG_ENABLE_VULKAN

// ==========================================================
//
//
// Internal: General
//
//
// ==========================================================

_Gmg _gmg;

#define _gmg_backend_fn_call(name, ...) _gmg.backend_impl_fns[_GmgBackendImplFnType_##name](__VA_ARGS__)

static GmgResult _gmg_backend_none() {
	return GmgResult_success;
}

#define _GMG_BACKEND_IMPL_FN_LIST \
	X(init) \
	X(physical_device_features) \
	X(physical_device_properties) \
	X(physical_device_surface_texture_format) \
	X(logical_device_init) \
	X(logical_device_deinit) \
	X(logical_device_submit) \
	\
	X(sampler_init) \
	X(sampler_deinit) \
	\
	X(texture_reinit) \
	X(texture_deinit) \
	X(texture_get_write_buffer) \
	\
	X(descriptor_set_layout_init) \
	X(descriptor_set_layout_deinit) \
	X(descriptor_set_allocator_init) \
	X(descriptor_set_allocator_deinit) \
	X(descriptor_set_allocator_reset) \
	\
	X(shader_module_init) \
	X(shader_module_deinit) \
	\
	X(shader_init) \
	X(shader_deinit) \
	\
	X(material_spec_cache_init) \
	X(material_spec_cache_deinit) \
	X(material_spec_cache_get_data) \
	\
	X(material_spec_init) \
	X(material_spec_deinit) \
	\
	X(buffer_reinit) \
	X(buffer_deinit) \
	X(buffer_get_write_buffer) \
	\
	X(render_pass_layout_init) \
	X(render_pass_layout_deinit) \
	\
	X(render_pass_init) \
	X(render_pass_deinit) \
	X(render_pass_clear_draw_cmds) \
	X(render_pass_queue_descriptors) \
	\
	X(frame_buffer_init) \
	X(frame_buffer_deinit) \
	\
	X(swapchain_reinit) \
	X(swapchain_deinit) \
	X(swapchain_get_next_texture_idx) \
	X(swapchain_present) \
	/* end */

typedef enum _GmgBackendImplFnType _GmgBackendImplFnType;
enum _GmgBackendImplFnType {
	_GmgBackendImplFnType_none,

#define X(name) _GmgBackendImplFnType_##name,
_GMG_BACKEND_IMPL_FN_LIST
#undef X

	_GmgBackendImplFnType_COUNT,
};

static GmgResult _gmg_stacktrace_add_(GmgResultType type, uint32_t ignore_levels_count) {
	das_assert(type < 0, "error type must be an error but got '%s'\n", GmgResultType_string(type));
	GmgStacktraceId trace_id;
	GmgStacktrace* trace = DasPool_alloc(GmgStacktraceId, &_gmg.stacktrace_pool, &trace_id);
	das_assert(trace, "cannot store the error as the error pool is full");
	das_zero_elmt(trace);

	if (!das_stacktrace(ignore_levels_count + 2, &trace->string))
		return (GmgResult) { .type = GmgResultType_error_out_of_memory_host, .stacktrace_id = 0 };

	GmgResult result = { .type = type, .stacktrace_id = trace_id };
	return result;
}
#define _gmg_stacktrace_add(type) _gmg_stacktrace_add_(type, 0)

static inline GmgResult _gmg_logical_device_object_get(GmgLogicalDevice* logical_device, _GmgLogicalDeviceObjectId id, _GmgLogicalDeviceObject** out) {
	if (id.raw == 0) return _gmg_stacktrace_add(GmgResultType_error_unexpected_null_id);
	if (!DasPool_is_id_valid(_GmgLogicalDeviceObjectId, &logical_device->object_pool, id))
		return _gmg_stacktrace_add(GmgResultType_error_object_use_after_free);

	*out = DasPool_id_to_ptr(_GmgLogicalDeviceObjectId, &logical_device->object_pool, id);
	return GmgResult_success;
}

static GmgResult _gmg_logical_device_object_init(GmgLogicalDevice* logical_device, void* create_args, _GmgLogicalDeviceObjectId* id_out, _GmgBackendImplFnType fn_type, _GmgLogicalDeviceObject** object_out) {
	_GmgLogicalDeviceObjectId object_id;
	_GmgLogicalDeviceObject* object = DasPool_alloc(_GmgLogicalDeviceObjectId, &logical_device->object_pool, &object_id);
	if (object == NULL) return _gmg_stacktrace_add(GmgResultType_error_logical_device_object_pool_full);
	*id_out = object_id;
	if (object_out) *object_out = object;

	return _gmg.backend_impl_fns[fn_type](object, logical_device, create_args);
}

static GmgResult _gmg_logical_device_object_deinit(GmgLogicalDevice* logical_device, _GmgLogicalDeviceObjectId object_id, _GmgBackendImplFnType fn_type) {
	_GmgLogicalDeviceObject* object;
	GmgResult result = _gmg_logical_device_object_get(logical_device, object_id, &object);
	if (result.type < 0) return result;

	result = _gmg.backend_impl_fns[fn_type](object, logical_device);
	if (result.type < 0) return result;

	DasPool_dealloc(_GmgLogicalDeviceObjectId, &logical_device->object_pool, object_id);

	return GmgResult_success;
}

static int _gmg_render_pass_sort_cmp_fn(const void* a_, const void* b_) {
	uint64_t a = *(const uint64_t*)a_;
	uint64_t b = *(const uint64_t*)b_;
	if (a < b) return -1;
	if (a > b) return 1;
	return 0;
}

static GmgResult _gmg_render_pass_sort(GmgRenderPass* render_pass) {
	uint32_t count = DasStk_count(&render_pass->draw_cmd_sort_keys);
	if (count) {
		void* data = DasStk_data(&render_pass->draw_cmd_sort_keys);
		qsort(
			data,
			count,
			DasStk_elmt_size(&render_pass->draw_cmd_sort_keys),
			_gmg_render_pass_sort_cmp_fn);
	}

	return GmgResult_success;
}

static void _gmg_logical_device_add_job_result(GmgLogicalDevice* logical_device, GmgResult result) {
	//
	// TODO: handle mulitple threads calling this function at the same time.
	//
	das_assert(DasStk_push(&logical_device->job_results, &result), "TODO handle this allocation error");
}

GmgResult _gmg_check_set_descriptor(GmgLogicalDevice* logical_device, GmgDescriptorBinding* bindings, uint16_t bindings_count, _GmgDescriptor* descriptor, uint32_t* set_binding_idx_out) {
	if (bindings_count <= descriptor->binding_idx) {
		return _gmg_stacktrace_add(GmgResultType_error_binding_idx_out_of_bounds);
	}

	GmgDescriptorBinding* binding = &bindings[descriptor->binding_idx];
	if (binding->type != descriptor->type) {
		GmgBool dynamic_uniform = descriptor->type == GmgDescriptorType_uniform_buffer && binding->type == GmgDescriptorType_uniform_buffer_dynamic;
		GmgBool dynamic_storage = descriptor->type == GmgDescriptorType_storage_buffer && binding->type == GmgDescriptorType_storage_buffer_dynamic;

		if (dynamic_uniform) descriptor->type = GmgDescriptorType_uniform_buffer_dynamic;
		else if (dynamic_storage) descriptor->type = GmgDescriptorType_storage_buffer_dynamic;
		else return _gmg_stacktrace_add(GmgResultType_error_descriptor_type_mismatch);
	}
	if (binding->count <= descriptor->elmt_idx) {
		return _gmg_stacktrace_add(GmgResultType_error_binding_elmt_idx_out_of_bounds);
	}

	if (descriptor->object_id.raw == 0) {
		return _gmg_stacktrace_add(GmgResultType_error_unexpected_null_id);
	}

	if (descriptor->type == GmgDescriptorType_uniform_buffer || descriptor->type == GmgDescriptorType_storage_buffer) {
		GmgBuffer* buffer;
		GmgResult result = gmg_buffer_get(logical_device, ((GmgBufferId) { .object_id = descriptor->object_id }), &buffer);
		if (result.type < 0) return result;

		if (descriptor->buffer_start_idx >= buffer->elmts_count) {
			return _gmg_stacktrace_add(GmgResultType_error_buffer_start_idx_out_of_bounds);
		}
	}

	*set_binding_idx_out = binding->_start_idx + descriptor->elmt_idx;
	return GmgResult_success;
}

GmgResult _gmg_check_set_dynamic_descriptor_offset(GmgLogicalDevice* logical_device, _GmgDescriptor* descriptors, GmgDescriptorBinding* bindings, uint16_t bindings_count, uint16_t binding_idx, uint16_t elmt_idx, uint32_t* dynamic_offset, uint16_t* out) {
	if (bindings_count <= binding_idx) {
		return _gmg_stacktrace_add(GmgResultType_error_binding_idx_out_of_bounds);
	}

	GmgDescriptorBinding* binding = &bindings[binding_idx];
	if (binding->type != GmgDescriptorType_uniform_buffer_dynamic && binding->type != GmgDescriptorType_storage_buffer_dynamic) {
		return _gmg_stacktrace_add(GmgResultType_error_binding_is_not_dynamic);
	}
	if (binding->count <= elmt_idx) {
		return _gmg_stacktrace_add(GmgResultType_error_binding_elmt_idx_out_of_bounds);
	}

	GmgBuffer* buffer;
	GmgResult result = gmg_buffer_get(logical_device, ((GmgBufferId) { .object_id = descriptors[binding->_start_idx + elmt_idx].object_id }), &buffer);
	if (result.type < 0) return result;

	*dynamic_offset *= (uint32_t)buffer->elmt_size;

	*out = binding->_dynamic_idx;
	return GmgResult_success;
}

#define _gmg_fnv_hash_32_initial 0x811c9dc5
uint32_t _gmg_fnv_hash_32(uint8_t* bytes, uint32_t byte_count, uint32_t hash) {
	uint8_t* bytes_end = bytes + byte_count;
	while (bytes < bytes_end) {
		hash = hash ^ *bytes;
		hash = hash * 0x01000193;
		bytes += 1;
	}
	return hash;
}

static GmgResult _gmg_descriptor_set_allocator_get_layout_idx(GmgLogicalDevice* logical_device, GmgDescriptorSetAllocator* allocator, GmgDescriptorSetLayoutId layout_id, uint32_t* idx_out) {
	uint32_t layout_idx = UINT32_MAX;
	for (uint32_t i = 0; i < allocator->layouts_count; i += 1) {
		if (allocator->layout_ids[i].raw == layout_id.raw) {
			layout_idx = i;
			break;
		}
	}

	if (layout_idx == UINT32_MAX) {
		return _gmg_stacktrace_add(GmgResultType_error_descriptor_set_layout_not_in_allocator);
	}

	*idx_out = layout_idx;
	return GmgResult_success;
}

#ifdef GMG_ENABLE_VULKAN

// ==========================================================
//
//
// Internal: Vulkan Allocator
//
//
// ==========================================================

static GmgResult _gmg_vulkan_convert_from_result(VkResult result);

static GmgResult _gmg_vulkan_device_memory_block_init(GmgLogicalDevice* logical_device, _GmgVulkanBlock* block, _GmgVulkanMemoryLocation memory_location, uint8_t memory_type_idx, uint64_t size) {
	block->remaining_size = size;
	block->size = size;

	VkMemoryAllocateInfo alloc_info = {
		.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
		.pNext = NULL,
		.allocationSize = size,
		.memoryTypeIndex = memory_type_idx,
	};
	VkResult vk_result = vkAllocateMemory(logical_device->vulkan.handle, &alloc_info, GMG_TODO_VULKAN_ALLOCATOR, &block->device_memory);
	if (vk_result) return _gmg_vulkan_convert_from_result(vk_result);

	if (memory_location != _GmgVulkanMemoryLocation_gpu) {
		vk_result = vkMapMemory(logical_device->vulkan.handle, block->device_memory, 0, size, 0, &block->mapped_memory);
		if (vk_result) return _gmg_vulkan_convert_from_result(vk_result);
	}

	_GmgVulkanFreeRange* free_range = DasStk_push(&block->free_ranges, NULL);
	if (free_range == NULL) return _gmg_stacktrace_add(GmgResultType_error_out_of_memory_host);
	free_range->start = 0;
	free_range->end = size;

	return GmgResult_success;
}

static GmgResult _gmg_vulkan_device_memory_block_deinit(GmgLogicalDevice* logical_device, _GmgVulkanMemoryLocation memory_location, _GmgVulkanBlock* block) {
	DasStk_deinit(&block->free_ranges);
	if (memory_location != _GmgVulkanMemoryLocation_gpu) {
		vkUnmapMemory(logical_device->vulkan.handle, block->device_memory);
	}
	vkFreeMemory(logical_device->vulkan.handle, block->device_memory, GMG_TODO_VULKAN_ALLOCATOR);
	return GmgResult_success;
}

static GmgResult _gmg_vulkan_device_memory_block_alloc(GmgLogicalDevice* logical_device, _GmgVulkanBlock* block, _GmgVulkanAllocArgs* args, _GmgVulkanAllocResult* out) {
	uint32_t smallest_found_range_idx = UINT32_MAX;
	uint64_t smallest_found_range_size = UINT64_MAX;
	DasStk_foreach(&block->free_ranges, i) {
		//
		// find the free range that can hold the requested allocation
		_GmgVulkanFreeRange* free_range = DasStk_get(&block->free_ranges, i);
		uint64_t allocation_start = das_round_up_nearest_multiple_u(free_range->start, args->align);
		uint64_t free_size = free_range->end - allocation_start;
		if (args->size <= free_size && free_size < smallest_found_range_size) {
			smallest_found_range_idx = i;
			smallest_found_range_size = free_size;
#ifdef GMG_VULKAN_ENABLE_ALLOCATOR_BLOCK_FIND_FIRST_FIT
			break;
#endif
		}
	}

	if (smallest_found_range_idx != UINT32_MAX) {
		_GmgVulkanFreeRange* free_range = DasStk_get(&block->free_ranges, smallest_found_range_idx);

		uint64_t allocation_start = das_round_up_nearest_multiple_u(free_range->start, args->align);
		//
		// this free range can fit so lets allocate an allocation structure
		// to keep track of all the memory that is used (including the alignment padding)
		_GmgVulkanAlloc* allocation = DasPool_alloc(_GmgVulkanAllocId, &logical_device->vulkan.allocator.allocations, &out->id);
		if (allocation == NULL) return _gmg_stacktrace_add(GmgResultType_error_out_of_memory_host);
		uint64_t allocation_end = allocation_start + args->size;
		allocation->memory_type_idx = args->_memory_type_idx;
		allocation->block_idx = args->_block_idx;
		allocation->start = free_range->start;
		allocation->end = allocation_end;

		uint32_t whole_allocation_size = allocation_end - free_range->start;

		// now store the rest of the user needs out to bind memory.
		out->memory_mapped_data = block->mapped_memory ? das_ptr_add(block->mapped_memory, allocation_start) : NULL;
		out->device_memory = block->device_memory;
		out->device_memory_offset = allocation_start;

		// shrink the free range or remove it if it has been completely
		// used by this allocation.
		if (free_range->end != allocation_end) {
			free_range->start = allocation_end;
		} else {
			DasStk_remove_shift(&block->free_ranges, smallest_found_range_idx);
		}

		block->remaining_size -= whole_allocation_size;
		return GmgResult_success;
	}

	return _gmg_stacktrace_add(GmgResultType_error_out_of_memory_device);
}

GmgResult _gmg_vulkan_device_memory_block_dealloc(GmgLogicalDevice* logical_device, _GmgVulkanBlock* block, uint64_t start, uint64_t end) {
	//
	// try to add the memory to a free range that directly neighbours the deallocation.
	// if the deallocation joins two free ranges then collapse these into a single one.
	DasStk_foreach(&block->free_ranges, i) {
		_GmgVulkanFreeRange* free_range = DasStk_get(&block->free_ranges, i);
		if (free_range->start == end) {
			free_range->start = start;
			goto SUCCESS;
		} else if (free_range->end == start) {
			free_range->end = end;
			// we only have to check for joins on the tail end to
			// the block that follows this one.
			if (i + 1 < _end_) {
				_GmgVulkanFreeRange* next = DasStk_get(&block->free_ranges, i + 1);
				if (next->start == end) {
					free_range->end = next->end;
					DasStk_remove_shift(&block->free_ranges, i + 1);
				}
			}
			goto SUCCESS;
		}
	}

	_GmgVulkanFreeRange* free_range = DasStk_push(&block->free_ranges, NULL);
	if (free_range == NULL) return _gmg_stacktrace_add(GmgResultType_error_out_of_memory_host);
	free_range->start = start;
	free_range->end = end;

SUCCESS: {}
	block->remaining_size += end - start;
	return GmgResult_success;
}

static GmgResult _gmg_vulkan_device_memory_pool_deinit(GmgLogicalDevice* logical_device, _GmgVulkanPool* pool) {
	for (uint32_t i = 0; i < pool->blocks_count; i += 1) {
		_GmgVulkanBlockKey key = pool->keys[i];
		if (key != 0) {
			_GmgVulkanMemoryLocation memory_location = (key & _GmgVulkanBlockKey_memory_location_MASK) >> _GmgVulkanBlockKey_memory_location_SHIFT;
			GmgResult result = _gmg_vulkan_device_memory_block_deinit(logical_device, memory_location, &pool->blocks[i]);
			if (result.type < 0) return result;
		}
	}

	das_dealloc_array(_GmgVulkanBlockKey, GMG_TODO_INTERNAL_ALLOCATOR, pool->keys, pool->blocks_cap);
	das_dealloc_array(_GmgVulkanBlock, GMG_TODO_INTERNAL_ALLOCATOR, pool->blocks, pool->blocks_cap);
	return GmgResult_success;
}

static GmgResult _gmg_vulkan_device_memory_pool_alloc(GmgLogicalDevice* logical_device, _GmgVulkanPool* pool, _GmgVulkanAllocArgs* args, _GmgVulkanAllocResult* out) {
	uint32_t free_block_idx = UINT32_MAX;
	_GmgVulkanBlock* block;
	_GmgVulkanBlockKey* block_key;
	_GmgVulkanMemoryLocation memory_location = args->memory_location;
	for (uint32_t i = 0; i < pool->blocks_count; i += 1) {
		_GmgVulkanBlockKey key = pool->keys[i];
		if (key == 0) {
			if (free_block_idx == UINT32_MAX) {
				free_block_idx = i;
			}
			continue;
		}

		if (((key & _GmgVulkanBlockKey_memory_location_MASK) >> _GmgVulkanBlockKey_memory_location_SHIFT) != memory_location) {
			continue;
		}
		if (((key & _GmgVulkanBlockKey_type_MASK) >> _GmgVulkanBlockKey_type_SHIFT) != args->type) {
			continue;
		}
		if (((key & _GmgVulkanBlockKey_remaining_size_MASK) >> _GmgVulkanBlockKey_remaining_size_SHIFT) < args->size) {
			continue;
		}

		block = &pool->blocks[i];
		block_key = &pool->keys[i];
		args->_block_idx = i;
		GmgResult result = _gmg_vulkan_device_memory_block_alloc(logical_device, block, args, out);
		if (result.type == GmgResultType_error_out_of_memory_device) {
			continue;
		}
		if (result.type < 0) return result;
		goto SUCCESS;
	}

	if (free_block_idx == UINT32_MAX) {
		if (pool->blocks_count == pool->blocks_cap) {
			uint16_t new_cap = pool->blocks_cap == 0 ? 16 : pool->blocks_cap * 2;
			_GmgVulkanBlockKey* keys = das_realloc_array(_GmgVulkanBlockKey, GMG_TODO_INTERNAL_ALLOCATOR, pool->keys, pool->blocks_cap, new_cap);
			if (keys == NULL) return _gmg_stacktrace_add(GmgResultType_error_out_of_memory_host);
			_GmgVulkanBlock* blocks = das_realloc_array(_GmgVulkanBlock, GMG_TODO_INTERNAL_ALLOCATOR, pool->blocks, pool->blocks_cap, new_cap);
			if (blocks == NULL) return _gmg_stacktrace_add(GmgResultType_error_out_of_memory_host);
			pool->keys = keys;
			pool->blocks = blocks;
			pool->blocks_cap = new_cap;
		}

		free_block_idx = pool->blocks_count;
		pool->blocks_count += 1;
	}

	uint64_t block_size = memory_location == _GmgVulkanMemoryLocation_gpu
		? logical_device->vulkan.allocator.min_block_size_device_local_memory
		: logical_device->vulkan.allocator.min_block_size_host_visible_memory;

	if (block_size < args->size) {
		block_size = args->size;
	}

	block = &pool->blocks[free_block_idx];
	das_zero_elmt(block);
	GmgResult result = _gmg_vulkan_device_memory_block_init(logical_device, block, memory_location, args->_memory_type_idx, block_size);
	if (result.type < 0) return result;

	args->_block_idx = free_block_idx;
	result = _gmg_vulkan_device_memory_block_alloc(logical_device, block, args, out);
	das_assert(result.type != GmgResultType_error_out_of_memory_device, "a new memory block in a pool must be able to allocate the initial allocation");
	if (result.type < 0) return result;

	block_key = &pool->keys[free_block_idx];
	*block_key = 0;
	*block_key |= _GmgVulkanBlockKey_is_allocated_MASK;
	*block_key |= ((uint64_t)memory_location << _GmgVulkanBlockKey_memory_location_SHIFT) & _GmgVulkanBlockKey_memory_location_MASK;
	*block_key |= ((uint64_t)args->type << _GmgVulkanBlockKey_type_SHIFT) & _GmgVulkanBlockKey_type_MASK;
SUCCESS: {}
	*block_key &= ~_GmgVulkanBlockKey_remaining_size_MASK;
	*block_key |= (block->remaining_size << _GmgVulkanBlockKey_remaining_size_SHIFT) & _GmgVulkanBlockKey_remaining_size_MASK;
	return GmgResult_success;
}

static GmgResult _gmg_vulkan_device_memory_pool_dealloc(GmgLogicalDevice* logical_device, _GmgVulkanPool* pool, uint32_t block_idx, uint64_t start, uint64_t end) {
	_GmgVulkanBlock* block = &pool->blocks[block_idx];
	GmgResult result = _gmg_vulkan_device_memory_block_dealloc(logical_device, block, start, end);
	if (result.type < 0) return result;

	_GmgVulkanBlockKey* key = &pool->keys[block_idx];
	if (DasStk_count(&block->free_ranges)) {
		*key &= ~_GmgVulkanBlockKey_remaining_size_MASK;
		*key |= (block->remaining_size << _GmgVulkanBlockKey_remaining_size_SHIFT) & _GmgVulkanBlockKey_remaining_size_MASK;
	} else {
		_GmgVulkanMemoryLocation memory_location = (*key & _GmgVulkanBlockKey_memory_location_MASK) >> _GmgVulkanBlockKey_memory_location_SHIFT;
		result = _gmg_vulkan_device_memory_block_deinit(logical_device, memory_location, block);
		if (result.type < 0) return result;
		*key = 0;
	}

	return GmgResult_success;
}

static GmgResult _gmg_vulkan_device_memory_allocator_init(GmgLogicalDevice* logical_device, uint64_t min_block_size_device_local_memory, uint64_t min_block_size_host_visible_memory) {
	_GmgVulkanAllocator* allocator = &logical_device->vulkan.allocator;
	if (min_block_size_device_local_memory == 0) {
		min_block_size_device_local_memory = gmg_vulkan_allocator_min_block_size_device_local_memory;
	}
	if (min_block_size_host_visible_memory == 0) {
		min_block_size_host_visible_memory = gmg_vulkan_allocator_min_block_size_host_visible_memory;
	}
	allocator->min_block_size_device_local_memory = min_block_size_device_local_memory;
	allocator->min_block_size_host_visible_memory = min_block_size_host_visible_memory;

	das_zero_array(allocator->pools);

	DasError das_error = DasPool_init(_GmgVulkanAllocId, &allocator->allocations, 32 * 1024, 1024);
	if (das_error) return GmgResult_from_DasError(das_error);

	return GmgResult_success;
}

static GmgResult _gmg_vulkan_device_memory_allocator_deinit(GmgLogicalDevice* logical_device) {
	_GmgVulkanAllocator* allocator = &logical_device->vulkan.allocator;
	for (uint32_t memory_type_idx = 0; memory_type_idx < VK_MAX_MEMORY_TYPES; memory_type_idx += 1) {
		GmgResult result = _gmg_vulkan_device_memory_pool_deinit(logical_device, &allocator->pools[memory_type_idx]);
		if (result.type < 0) return result;
	}

	DasError das_error = DasPool_deinit(_GmgVulkanAllocId, &allocator->allocations);
	if (das_error) return GmgResult_from_DasError(das_error);
	return GmgResult_success;
}

GmgResult _gmg_vulkan_device_memory_alloc(GmgLogicalDevice* logical_device, _GmgVulkanAllocArgs* args, _GmgVulkanAllocResult* out, GmgBool* needs_flush_out) {
	VkMemoryPropertyFlags memory_properties;
	{
		GmgPhysicalDevice* physical_device;
		GmgResult result = gmg_physical_device_get(logical_device->physical_device_idx, &physical_device);
		if (result.type < 0) return result;

		if (physical_device->vulkan.memory_properties == NULL) {
			physical_device->vulkan.memory_properties = das_alloc_elmt(VkPhysicalDeviceMemoryProperties, GMG_TODO_INTERNAL_ALLOCATOR);
			if (physical_device->vulkan.memory_properties == NULL)
				return _gmg_stacktrace_add(GmgResultType_error_out_of_memory_host);
			vkGetPhysicalDeviceMemoryProperties(physical_device->vulkan.handle, physical_device->vulkan.memory_properties);
		}

		VkPhysicalDeviceMemoryProperties* vk_props = physical_device->vulkan.memory_properties;

		VkMemoryPropertyFlags required = 0;
		VkMemoryPropertyFlags preferred = 0;
		VkMemoryPropertyFlags not_preferred = 0;

		switch (args->memory_location) {
		case _GmgVulkanMemoryLocation_gpu:
			preferred |= VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
			not_preferred |= VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
			break;
		case _GmgVulkanMemoryLocation_shared:
			required |= VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
			preferred |= VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
			not_preferred |= VK_MEMORY_PROPERTY_HOST_CACHED_BIT;
			break;
		case _GmgVulkanMemoryLocation_shared_cached:
			required |= VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
			preferred |= VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_CACHED_BIT;
			break;
		}

		uint32_t memory_type_bits = args->memory_type_bits;
		for (uint32_t i = 0; i < vk_props->memoryTypeCount; i += 1) {
			if (((memory_type_bits >> i) & 1) == 0) {
				continue;
			}

			VkMemoryPropertyFlags properties = vk_props->memoryTypes[i].propertyFlags;
			if ((properties & required) != required) {
				continue;
			}

			if ((properties & preferred) != preferred) {
				continue;
			}

			if (properties & not_preferred) {
				continue;
			}

			args->_memory_type_idx = i;
			memory_properties = properties;
			goto MEMORY_TYPE_FOUND;
		}

		for (uint32_t i = 0; i < vk_props->memoryTypeCount; i += 1) {
			if (((memory_type_bits >> i) & 1) == 0) {
				continue;
			}

			VkMemoryPropertyFlags properties = vk_props->memoryTypes[i].propertyFlags;
			if ((properties & required) != required) {
				continue;
			}

			args->_memory_type_idx = i;
			memory_properties = properties;
			goto MEMORY_TYPE_FOUND;
		}

		das_abort("could not find a memory type for the allocation request");
	}
MEMORY_TYPE_FOUND: {}
	if (needs_flush_out) {
		*needs_flush_out = (memory_properties & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) == 0;
	}

	_GmgVulkanAllocator* allocator = &logical_device->vulkan.allocator;
	_GmgVulkanPool* pool = &allocator->pools[args->_memory_type_idx];
	return _gmg_vulkan_device_memory_pool_alloc(logical_device, pool, args, out);
}

GmgResult _gmg_vulkan_device_memory_dealloc(GmgLogicalDevice* logical_device, _GmgVulkanAllocId allocation_id) {
	_GmgVulkanAllocator* allocator = &logical_device->vulkan.allocator;

	_GmgVulkanAlloc* alloc = DasPool_id_to_ptr(_GmgVulkanAllocId, &allocator->allocations, allocation_id);

	_GmgVulkanPool* pool = &allocator->pools[alloc->memory_type_idx];
	GmgResult result =  _gmg_vulkan_device_memory_pool_dealloc(logical_device, pool, alloc->block_idx, alloc->start, alloc->end);
	if (result.type < 0) return result;

	DasPool_dealloc(_GmgVulkanAllocId, &allocator->allocations, allocation_id);

	return GmgResult_success;
}

GmgResult _gmg_vulkan_device_memory_allocator_flush(GmgLogicalDevice* logical_device, _GmgVulkanAllocId alloc_id, void* memory_mapped_start, uint64_t offset, uintptr_t size) {
	_GmgVulkanAllocator* allocator = &logical_device->vulkan.allocator;
	_GmgVulkanAlloc* alloc = DasPool_id_to_ptr(_GmgVulkanAllocId, &allocator->allocations, alloc_id);

	_GmgVulkanPool* pool = &allocator->pools[alloc->memory_type_idx];
	_GmgVulkanBlock* block = &pool->blocks[alloc->block_idx];

	uintptr_t block_offset = memory_mapped_start - block->mapped_memory;

	_GmgVulkanFlushMemoryRange* range = DasStk_push(&logical_device->vulkan.flush_memory_ranges, NULL);
	if (range == NULL) return _gmg_stacktrace_add(GmgResultType_error_out_of_memory_host);
	range->memory = block->device_memory;
	range->offset = block_offset + offset;
	range->size = size;

	return GmgResult_success;
}

// ==========================================================
//
//
// Internal: Vulkan Memory Stager
//
//
// ==========================================================

static GmgResult _gmg_vulkan_memory_stager_init(GmgLogicalDevice* logical_device, uint32_t buffer_size) {
	_GmgVulkanMemoryStager* stager = &logical_device->vulkan.memory_stager;
	if (buffer_size == 0) buffer_size = gmg_vulkan_memory_stager_buffer_size;

	stager->buffer_size = buffer_size;

	return GmgResult_success;
}

static GmgResult _gmg_vulkan_memory_stager_stage(GmgLogicalDevice* logical_device, uint32_t size, uint32_t align, _GmgVulkanStagingBuffer** staging_buffer_out, uint32_t* src_buffer_offset_out, void** src_buffer_data_out) {
	_GmgVulkanMemoryStager* stager = &logical_device->vulkan.memory_stager;
	das_assert(size <= stager->buffer_size, "cannot stage memory more that '%u' bytes but got '%u'", size, stager->buffer_size);

	//
	// see if we have a current buffer that can fit the size of the data being staged.
	// if does not then user the next buffer if there is one.
	// the while loop will only execute a single time more, as the next buffer
	// is clear so will definitely store the data (thanks to the assertion above).
	//
	_GmgVulkanStagingBuffer* buffer = NULL;
	while (stager->filled_buffers_count < DasStk_count(&stager->buffers)) {
		buffer = DasStk_get(&stager->buffers, stager->filled_buffers_count);
		uint32_t aligned_pos = (uintptr_t)das_ptr_round_up_align((void*)(uintptr_t)buffer->position, align);
		if (aligned_pos + size > stager->buffer_size) {
			stager->filled_buffers_count += 1;
			buffer = NULL;
			continue;
		}
		break;
	}

	if (buffer == NULL) {
		buffer = DasStk_push(&stager->buffers, NULL);
		if (buffer == NULL) return _gmg_stacktrace_add(GmgResultType_error_out_of_memory_host);
		das_zero_elmt(buffer);
	}

	if (buffer->core.buffer == VK_NULL_HANDLE) {
		if (DasStk_count(&logical_device->vulkan.staging_buffers_available)) {
			buffer->core = *DasStk_get_last(&logical_device->vulkan.staging_buffers_available);
			DasStk_pop(&logical_device->vulkan.staging_buffers_available);
		} else {
			{
				VkBufferUsageFlags vk_usage_flags = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

				VkBufferCreateInfo vk_create_info = {
					.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
					.pNext = NULL,
					.flags = 0,
					.size = stager->buffer_size,
					.usage = vk_usage_flags,
					.sharingMode = VK_SHARING_MODE_EXCLUSIVE,
					.queueFamilyIndexCount = 0,
					.pQueueFamilyIndices = NULL,
				};

				VkResult vk_result = vkCreateBuffer(logical_device->vulkan.handle, &vk_create_info, GMG_TODO_VULKAN_ALLOCATOR, &buffer->core.buffer);
				if (vk_result) return _gmg_vulkan_convert_from_result(vk_result);


				VkMemoryRequirements mem_req;
				vkGetBufferMemoryRequirements(logical_device->vulkan.handle, buffer->core.buffer, &mem_req);

				_GmgVulkanAllocResult alloc_result;
				_GmgVulkanAllocArgs args = {
					.size = mem_req.size,
					.align = mem_req.alignment,
					.memory_type_bits = mem_req.memoryTypeBits,
					.memory_location = _GmgVulkanMemoryLocation_shared,
					.type = _GmgVulkanAllocType_buffer,
				};
				GmgResult result = _gmg_vulkan_device_memory_alloc(logical_device, &args, &alloc_result, &buffer->needs_flush);
				if (result.type < 0) return result;

				buffer->core.memory_mapped_data = alloc_result.memory_mapped_data;
				buffer->core.device_memory_alloc_id = alloc_result.id;

				vk_result = vkBindBufferMemory(logical_device->vulkan.handle, buffer->core.buffer, alloc_result.device_memory, alloc_result.device_memory_offset);
				if (vk_result) return _gmg_vulkan_convert_from_result(vk_result);
			}
		}
	}

	uint32_t aligned_pos = (uintptr_t)das_ptr_round_up_align((void*)(uintptr_t)buffer->position, align);
	*staging_buffer_out = buffer;
	*src_buffer_offset_out = aligned_pos;
	*src_buffer_data_out = das_ptr_add(buffer->core.memory_mapped_data, aligned_pos);

	buffer->position = aligned_pos + size;

	return GmgResult_success;
}

static GmgResult _gmg_vulkan_memory_stager_stage_buffer(GmgLogicalDevice* logical_device, VkBuffer dst_buffer, uint64_t dst_buffer_offset, uint32_t size, void** src_buffer_data_out) {
	_GmgVulkanStagingBuffer* staging_buffer;
	uint32_t src_buffer_offset;
	GmgResult result = _gmg_vulkan_memory_stager_stage(logical_device, size, 1, &staging_buffer, &src_buffer_offset, src_buffer_data_out);
	if (result.type < 0) return result;

	_GmgVulkanStagedBuffer* staged = DasStk_push(&staging_buffer->staged_buffers, NULL);
	if (staged == NULL) return _gmg_stacktrace_add(GmgResultType_error_out_of_memory_host);

	staged->dst_buffer = dst_buffer;
	staged->dst_buffer_offset = dst_buffer_offset;
	staged->size = size;
	staged->src_buffer_offset = src_buffer_offset;

	return GmgResult_success;
}

static GmgResult _gmg_vulkan_memory_stager_stage_texture(GmgLogicalDevice* logical_device, GmgTextureFormat texture_format, VkImage dst_image, GmgTextureArea* area, void** src_buffer_data_out) {
	uint32_t size = area->width * area->height * area->depth * area->array_layers_count * (uint32_t)GmgTextureFormat_bytes_per_pixel[texture_format];

	_GmgVulkanStagingBuffer* staging_buffer;
	uint32_t src_buffer_offset;
	GmgResult result = _gmg_vulkan_memory_stager_stage(logical_device, size, 16, &staging_buffer, &src_buffer_offset, src_buffer_data_out);
	if (result.type < 0) return result;

	_GmgVulkanStagedTexture* staged = DasStk_push(&staging_buffer->staged_textures, NULL);
	if (staged == NULL) return _gmg_stacktrace_add(GmgResultType_error_out_of_memory_host);

	staged->dst_image = dst_image;
	staged->src_buffer_offset = src_buffer_offset;
	staged->area = *area;

	return GmgResult_success;
}

// ==========================================================
//
//
// Internal: Vulkan
//
//
// ==========================================================

typedef_DasStk(VkAttachmentReference);

static GmgResult _gmg_vulkan_convert_from_result(VkResult result) {
	GmgResultType type;
	switch (result) {
		case VK_SUCCESS: return GmgResult_success;
		case VK_ERROR_OUT_OF_HOST_MEMORY: type = GmgResultType_error_out_of_memory_host; break;
		case VK_ERROR_OUT_OF_DEVICE_MEMORY: type = GmgResultType_error_out_of_memory_device; break;
		default:
			das_abort("unhandled VkResult: %d", result);
	}

	return _gmg_stacktrace_add_(type, 1);
}

static uint32_t _gmg_vulkan_convert_to_physical_device_features_field_offsets_plus_one[] = {
    [GmgPhysicalDeviceFeature_robust_buffer_access] = offsetof(VkPhysicalDeviceFeatures, robustBufferAccess) + 1,
    [GmgPhysicalDeviceFeature_full_draw_index_uint32] = offsetof(VkPhysicalDeviceFeatures, fullDrawIndexUint32) + 1,
    [GmgPhysicalDeviceFeature_image_cube_array] = offsetof(VkPhysicalDeviceFeatures, imageCubeArray) + 1,
    [GmgPhysicalDeviceFeature_independent_blend] = offsetof(VkPhysicalDeviceFeatures, independentBlend) + 1,
    [GmgPhysicalDeviceFeature_geometry_shader] = offsetof(VkPhysicalDeviceFeatures, geometryShader) + 1,
    [GmgPhysicalDeviceFeature_tessellation_shader] = offsetof(VkPhysicalDeviceFeatures, tessellationShader) + 1,
    [GmgPhysicalDeviceFeature_sample_rate_shading] = offsetof(VkPhysicalDeviceFeatures, sampleRateShading) + 1,
    [GmgPhysicalDeviceFeature_dual_src_blend] = offsetof(VkPhysicalDeviceFeatures, dualSrcBlend) + 1,
    [GmgPhysicalDeviceFeature_logic_op] = offsetof(VkPhysicalDeviceFeatures, logicOp) + 1,
    [GmgPhysicalDeviceFeature_multi_draw_indirect] = offsetof(VkPhysicalDeviceFeatures, multiDrawIndirect) + 1,
    [GmgPhysicalDeviceFeature_draw_indirect_first_instance] = offsetof(VkPhysicalDeviceFeatures, drawIndirectFirstInstance) + 1,
    [GmgPhysicalDeviceFeature_depth_clamp] = offsetof(VkPhysicalDeviceFeatures, depthClamp) + 1,
    [GmgPhysicalDeviceFeature_depth_bias_clamp] = offsetof(VkPhysicalDeviceFeatures, depthBiasClamp) + 1,
    [GmgPhysicalDeviceFeature_fill_mode_non_solid] = offsetof(VkPhysicalDeviceFeatures, fillModeNonSolid) + 1,
    [GmgPhysicalDeviceFeature_depth_bounds] = offsetof(VkPhysicalDeviceFeatures, depthBounds) + 1,
    [GmgPhysicalDeviceFeature_wide_lines] = offsetof(VkPhysicalDeviceFeatures, wideLines) + 1,
    [GmgPhysicalDeviceFeature_large_points] = offsetof(VkPhysicalDeviceFeatures, largePoints) + 1,
    [GmgPhysicalDeviceFeature_alpha_to_one] = offsetof(VkPhysicalDeviceFeatures, alphaToOne) + 1,
    [GmgPhysicalDeviceFeature_multi_viewport] = offsetof(VkPhysicalDeviceFeatures, multiViewport) + 1,
    [GmgPhysicalDeviceFeature_sampler_anisotropy] = offsetof(VkPhysicalDeviceFeatures, samplerAnisotropy) + 1,
    [GmgPhysicalDeviceFeature_texture_compression_etc2] = offsetof(VkPhysicalDeviceFeatures, textureCompressionETC2) + 1,
    [GmgPhysicalDeviceFeature_texture_compression_astc_ldr] = offsetof(VkPhysicalDeviceFeatures, textureCompressionASTC_LDR) + 1,
    [GmgPhysicalDeviceFeature_texture_compression_bc] = offsetof(VkPhysicalDeviceFeatures, textureCompressionBC) + 1,
    [GmgPhysicalDeviceFeature_occlusion_query_precise] = offsetof(VkPhysicalDeviceFeatures, occlusionQueryPrecise) + 1,
    [GmgPhysicalDeviceFeature_pipeline_statistics_query] = offsetof(VkPhysicalDeviceFeatures, pipelineStatisticsQuery) + 1,
    [GmgPhysicalDeviceFeature_vertex_pipeline_stores_and_atomics] = offsetof(VkPhysicalDeviceFeatures, vertexPipelineStoresAndAtomics) + 1,
    [GmgPhysicalDeviceFeature_fragment_stores_and_atomics] = offsetof(VkPhysicalDeviceFeatures, fragmentStoresAndAtomics) + 1,
    [GmgPhysicalDeviceFeature_shader_tessellation_and_geometry_point_size] = offsetof(VkPhysicalDeviceFeatures, shaderTessellationAndGeometryPointSize) + 1,
    [GmgPhysicalDeviceFeature_shader_image_gather_extended] = offsetof(VkPhysicalDeviceFeatures, shaderImageGatherExtended) + 1,
    [GmgPhysicalDeviceFeature_shader_storage_image_extended_formats] = offsetof(VkPhysicalDeviceFeatures, shaderStorageImageExtendedFormats) + 1,
    [GmgPhysicalDeviceFeature_shader_storage_image_multisample] = offsetof(VkPhysicalDeviceFeatures, shaderStorageImageMultisample) + 1,
    [GmgPhysicalDeviceFeature_shader_storage_image_read_withoutFormat] = offsetof(VkPhysicalDeviceFeatures, shaderStorageImageReadWithoutFormat) + 1,
    [GmgPhysicalDeviceFeature_shader_storage_image_write_without_format] = offsetof(VkPhysicalDeviceFeatures, shaderStorageImageWriteWithoutFormat) + 1,
    [GmgPhysicalDeviceFeature_shader_uniform_buffer_array_dynamic_indexing] = offsetof(VkPhysicalDeviceFeatures, shaderUniformBufferArrayDynamicIndexing) + 1,
    [GmgPhysicalDeviceFeature_shader_sampled_image_array_dynamic_indexing] = offsetof(VkPhysicalDeviceFeatures, shaderSampledImageArrayDynamicIndexing) + 1,
    [GmgPhysicalDeviceFeature_shader_storage_buffer_array_dynamic_indexing] = offsetof(VkPhysicalDeviceFeatures, shaderStorageBufferArrayDynamicIndexing) + 1,
    [GmgPhysicalDeviceFeature_shader_storage_image_array_dynamic_indexing] = offsetof(VkPhysicalDeviceFeatures, shaderStorageImageArrayDynamicIndexing) + 1,
    [GmgPhysicalDeviceFeature_shader_clip_distance] = offsetof(VkPhysicalDeviceFeatures, shaderClipDistance) + 1,
    [GmgPhysicalDeviceFeature_shader_cull_distance] = offsetof(VkPhysicalDeviceFeatures, shaderCullDistance) + 1,
    [GmgPhysicalDeviceFeature_shader_float64] = offsetof(VkPhysicalDeviceFeatures, shaderFloat64) + 1,
    [GmgPhysicalDeviceFeature_shader_int64] = offsetof(VkPhysicalDeviceFeatures, shaderInt64) + 1,
    [GmgPhysicalDeviceFeature_shader_int16] = offsetof(VkPhysicalDeviceFeatures, shaderInt16) + 1,
    [GmgPhysicalDeviceFeature_shader_resource_residency] = offsetof(VkPhysicalDeviceFeatures, shaderResourceResidency) + 1,
    [GmgPhysicalDeviceFeature_shader_resource_min_lod] = offsetof(VkPhysicalDeviceFeatures, shaderResourceMinLod) + 1,
    [GmgPhysicalDeviceFeature_sparse_binding] = offsetof(VkPhysicalDeviceFeatures, sparseBinding) + 1,
    [GmgPhysicalDeviceFeature_sparse_residency_buffer] = offsetof(VkPhysicalDeviceFeatures, sparseResidencyBuffer) + 1,
    [GmgPhysicalDeviceFeature_sparse_residency_image2_d] = offsetof(VkPhysicalDeviceFeatures, sparseResidencyImage2D) + 1,
    [GmgPhysicalDeviceFeature_sparse_residency_image3_d] = offsetof(VkPhysicalDeviceFeatures, sparseResidencyImage3D) + 1,
    [GmgPhysicalDeviceFeature_sparse_residency2_samples] = offsetof(VkPhysicalDeviceFeatures, sparseResidency2Samples) + 1,
    [GmgPhysicalDeviceFeature_sparse_residency4_samples] = offsetof(VkPhysicalDeviceFeatures, sparseResidency4Samples) + 1,
    [GmgPhysicalDeviceFeature_sparse_residency8_samples] = offsetof(VkPhysicalDeviceFeatures, sparseResidency8Samples) + 1,
    [GmgPhysicalDeviceFeature_sparse_residency16_samples] = offsetof(VkPhysicalDeviceFeatures, sparseResidency16Samples) + 1,
    [GmgPhysicalDeviceFeature_sparse_residency_aliased] = offsetof(VkPhysicalDeviceFeatures, sparseResidencyAliased) + 1,
    [GmgPhysicalDeviceFeature_variable_multisample_rate] = offsetof(VkPhysicalDeviceFeatures, variableMultisampleRate) + 1,
    [GmgPhysicalDeviceFeature_inherited_queries] = offsetof(VkPhysicalDeviceFeatures, inheritedQueries) + 1,
    [GmgPhysicalDeviceFeature_COUNT] = 0,
};

static GmgResult _gmg_vulkan_convert_to_physical_device_features(GmgPhysicalDeviceFeatureFlags gmg, VkPhysicalDeviceFeatures* vk) {
	das_zero_elmt(vk);

	while (gmg) {
		// get the index of the least significant bit that is set.
		// this will be the next enumeration.
		GmgPhysicalDeviceFeature feature = das_least_set_bit_idx(gmg);

		uint32_t offset = _gmg_vulkan_convert_to_physical_device_features_field_offsets_plus_one[feature];
		if (offset == 0) {
			das_abort("VULKAN: unhandled feature %u\n", feature);
		}
		offset -= 1;

		*(VkBool32*)das_ptr_add(vk, offset) = VK_TRUE;

		// remove the least significant bit that we just checked
		gmg = (gmg) & ((gmg) - 1);
	}

	return GmgResult_success;
}

static GmgPhysicalDeviceType _gmg_vulkan_convert_from_physical_device_type[] = {
    [VK_PHYSICAL_DEVICE_TYPE_OTHER] = GmgPhysicalDeviceType_other,
    [VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU] = GmgPhysicalDeviceType_gpu_integrated,
    [VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU] = GmgPhysicalDeviceType_gpu_discrete,
    [VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU] = GmgPhysicalDeviceType_gpu_virtual,
    [VK_PHYSICAL_DEVICE_TYPE_CPU] = GmgPhysicalDeviceType_cpu,
};

static VkImageType _gmg_vulkan_convert_to_image_type[] = {
	[GmgTextureType_1d] = VK_IMAGE_TYPE_1D,
	[GmgTextureType_1d_array] = VK_IMAGE_TYPE_1D,
	[GmgTextureType_2d] = VK_IMAGE_TYPE_2D,
	[GmgTextureType_2d_array] = VK_IMAGE_TYPE_1D,
	[GmgTextureType_3d] = VK_IMAGE_TYPE_3D,
	[GmgTextureType_cube] = VK_IMAGE_TYPE_2D,
	[GmgTextureType_cube_array] = VK_IMAGE_TYPE_2D,
};

static VkImageViewType _gmg_vulkan_convert_to_image_view_type[] = {
	[GmgTextureType_1d] = VK_IMAGE_VIEW_TYPE_1D,
	[GmgTextureType_1d_array] = VK_IMAGE_VIEW_TYPE_1D_ARRAY,
	[GmgTextureType_2d] = VK_IMAGE_VIEW_TYPE_2D,
	[GmgTextureType_2d_array] = VK_IMAGE_VIEW_TYPE_1D_ARRAY,
	[GmgTextureType_3d] = VK_IMAGE_VIEW_TYPE_3D,
	[GmgTextureType_cube] = VK_IMAGE_VIEW_TYPE_CUBE,
	[GmgTextureType_cube_array] = VK_IMAGE_VIEW_TYPE_CUBE_ARRAY,
};

static VkAttachmentLoadOp _gmg_vulkan_convert_to_attachment_load_op[] = {
	[GmgAttachmentLoadOp_preserve] = VK_ATTACHMENT_LOAD_OP_LOAD,
	[GmgAttachmentLoadOp_clear] = VK_ATTACHMENT_LOAD_OP_CLEAR,
	[GmgAttachmentLoadOp_uninitialized] = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
};

static VkAttachmentStoreOp _gmg_vulkan_convert_to_attachment_store_op[] = {
	[GmgAttachmentStoreOp_preserve] = VK_ATTACHMENT_STORE_OP_STORE,
	[GmgAttachmentStoreOp_discard] = VK_ATTACHMENT_STORE_OP_DONT_CARE,
};

static VkDescriptorType _gmg_vulkan_convert_to_descriptor_type[GmgDescriptorType_COUNT] = {
	[GmgDescriptorType_sampler] = VK_DESCRIPTOR_TYPE_SAMPLER,
	[GmgDescriptorType_texture] = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
	[GmgDescriptorType_uniform_buffer] = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
	[GmgDescriptorType_storage_buffer] = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
	[GmgDescriptorType_uniform_buffer_dynamic] = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC,
	[GmgDescriptorType_storage_buffer_dynamic] = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC,
};

static VkFilter _gmg_vulkan_convert_to_filter[] = {
	[GmgFilter_nearest] = VK_FILTER_NEAREST,
	[GmgFilter_linear] = VK_FILTER_LINEAR,
	[GmgFilter_cubic_img] = VK_FILTER_CUBIC_IMG,
};

static VkSamplerMipmapMode _gmg_vulkan_convert_to_sampler_mipmap_mode[] = {
	[GmgSamplerMipmapMode_nearest] = VK_SAMPLER_MIPMAP_MODE_NEAREST,
	[GmgSamplerMipmapMode_linear] = VK_SAMPLER_MIPMAP_MODE_LINEAR,
};

static VkSamplerAddressMode _gmg_vulkan_convert_to_sampler_address_mode[] = {
	[GmgSamplerAddressMode_repeat] = VK_SAMPLER_ADDRESS_MODE_REPEAT,
	[GmgSamplerAddressMode_mirrored_repeat] = VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT,
	[GmgSamplerAddressMode_clamp_to_edge] = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
	[GmgSamplerAddressMode_clamp_to_border] = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER,
	[GmgSamplerAddressMode_mirror_clamp_to_edge] = VK_SAMPLER_ADDRESS_MODE_MIRROR_CLAMP_TO_EDGE,
};

static VkCompareOp _gmg_vulkan_convert_to_compare_op[] = {
	[GmgCompareOp_never] = VK_COMPARE_OP_NEVER,
	[GmgCompareOp_less] = VK_COMPARE_OP_LESS,
	[GmgCompareOp_equal] = VK_COMPARE_OP_EQUAL,
	[GmgCompareOp_less_or_equal] = VK_COMPARE_OP_LESS_OR_EQUAL,
	[GmgCompareOp_greater] = VK_COMPARE_OP_GREATER,
	[GmgCompareOp_not_equal] = VK_COMPARE_OP_NOT_EQUAL,
	[GmgCompareOp_greater_or_equal] = VK_COMPARE_OP_GREATER_OR_EQUAL,
	[GmgCompareOp_always] = VK_COMPARE_OP_ALWAYS,
};

static VkBorderColor _gmg_vulkan_convert_to_border_color[] = {
	[GmgBorderColor_float_transparent_black] = VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK,
	[GmgBorderColor_int_transparent_black] = VK_BORDER_COLOR_INT_TRANSPARENT_BLACK,
	[GmgBorderColor_float_opaque_black] = VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK,
	[GmgBorderColor_int_opaque_black] = VK_BORDER_COLOR_INT_OPAQUE_BLACK,
	[GmgBorderColor_float_opaque_white] = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE,
	[GmgBorderColor_int_opaque_white] = VK_BORDER_COLOR_INT_OPAQUE_WHITE,
};

static GmgResult _gmg_vulkan_convert_to_shader_stage_flags(GmgShaderStageFlags gmg, VkShaderStageFlags* vk_out) {
	VkShaderStageFlags vk = 0;

	if (gmg & GmgShaderStageFlags_vertex) vk |= VK_SHADER_STAGE_VERTEX_BIT;
	if (gmg & GmgShaderStageFlags_tessellation_control) vk |= VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
	if (gmg & GmgShaderStageFlags_tessellation_evaluation) vk |= VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
	if (gmg & GmgShaderStageFlags_geometry) vk |= VK_SHADER_STAGE_GEOMETRY_BIT;
	if (gmg & GmgShaderStageFlags_fragment) vk |= VK_SHADER_STAGE_FRAGMENT_BIT;
	if (gmg & GmgShaderStageFlags_compute) vk |= VK_SHADER_STAGE_COMPUTE_BIT;
	if ((gmg & GmgShaderStageFlags_all_graphics) == GmgShaderStageFlags_all_graphics) vk |= VK_SHADER_STAGE_ALL_GRAPHICS;

	*vk_out = vk;
	return GmgResult_success;
}

static GmgResult _gmg_vulkan_convert_to_cull_mode(GmgCullModeFlags gmg, VkCullModeFlags* vk_out) {
	VkCullModeFlags vk = 0;

	if (gmg & GmgCullModeFlags_front) vk |= VK_CULL_MODE_FRONT_BIT;
	if (gmg & GmgCullModeFlags_back) vk |= VK_CULL_MODE_BACK_BIT;

	*vk_out = vk;
	return GmgResult_success;
}

static GmgResult _gmg_vulkan_convert_to_viewport(GmgViewport* gmg, VkViewport* vk_out) {
	*vk_out = (VkViewport) {
		.x = gmg->x,
		.y = gmg->y,
		.width = gmg->width,
		.height = gmg->height,
		.minDepth = gmg->min_depth,
		.maxDepth = gmg->max_depth,
	};
	return GmgResult_success;
}

static GmgResult _gmg_vulkan_convert_to_rect2d(GmgRect2u* gmg, VkRect2D* vk_out) {
	*vk_out = (VkRect2D) {
		.offset = { .x = gmg->x, .y = gmg->y, },
		.extent = { .width = gmg->width, .height = gmg->height, },
	};
	return GmgResult_success;
}

static VkPrimitiveTopology _gmg_vulkan_convert_to_topology[] = {
	[GmgPrimitiveTopology_point_list] = VK_PRIMITIVE_TOPOLOGY_POINT_LIST,
	[GmgPrimitiveTopology_line_list] = VK_PRIMITIVE_TOPOLOGY_LINE_LIST,
	[GmgPrimitiveTopology_line_strip] = VK_PRIMITIVE_TOPOLOGY_LINE_STRIP,
	[GmgPrimitiveTopology_triangle_list] = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
	[GmgPrimitiveTopology_triangle_strip] = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP,
	[GmgPrimitiveTopology_triangle_fan] = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN,
	[GmgPrimitiveTopology_line_list_with_adjacency] = VK_PRIMITIVE_TOPOLOGY_LINE_LIST_WITH_ADJACENCY,
	[GmgPrimitiveTopology_line_strip_with_adjacency] = VK_PRIMITIVE_TOPOLOGY_LINE_STRIP_WITH_ADJACENCY,
	[GmgPrimitiveTopology_triangle_list_with_adjacency] = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST_WITH_ADJACENCY,
	[GmgPrimitiveTopology_triangle_strip_with_adjacency] = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP_WITH_ADJACENCY,
	[GmgPrimitiveTopology_patch_list] = VK_PRIMITIVE_TOPOLOGY_PATCH_LIST,
};

static VkStencilOp _gmg_vulkan_convert_to_stencil_op[] = {
	[GmgStencilOp_keep] = VK_STENCIL_OP_KEEP,
	[GmgStencilOp_zero] = VK_STENCIL_OP_ZERO,
	[GmgStencilOp_replace] = VK_STENCIL_OP_REPLACE,
	[GmgStencilOp_increment_and_clamp] = VK_STENCIL_OP_INCREMENT_AND_CLAMP,
	[GmgStencilOp_decrement_and_clamp] = VK_STENCIL_OP_DECREMENT_AND_CLAMP,
	[GmgStencilOp_invert] = VK_STENCIL_OP_INVERT,
	[GmgStencilOp_increment_and_wrap] = VK_STENCIL_OP_INCREMENT_AND_WRAP,
	[GmgStencilOp_decrement_and_wrap] = VK_STENCIL_OP_DECREMENT_AND_WRAP,
};

static VkLogicOp _gmg_vulkan_convert_to_logic_op[] = {
	[GmgLogicOp_clear] = VK_LOGIC_OP_CLEAR,
	[GmgLogicOp_and] = VK_LOGIC_OP_AND,
	[GmgLogicOp_and_reverse] = VK_LOGIC_OP_AND_REVERSE,
	[GmgLogicOp_copy] = VK_LOGIC_OP_COPY,
	[GmgLogicOp_and_inverted] = VK_LOGIC_OP_AND_INVERTED,
	[GmgLogicOp_no_op] = VK_LOGIC_OP_NO_OP,
	[GmgLogicOp_xor] = VK_LOGIC_OP_XOR,
	[GmgLogicOp_or] = VK_LOGIC_OP_OR,
	[GmgLogicOp_nor] = VK_LOGIC_OP_NOR,
	[GmgLogicOp_equivalent] = VK_LOGIC_OP_EQUIVALENT,
	[GmgLogicOp_invert] = VK_LOGIC_OP_INVERT,
	[GmgLogicOp_or_reverse] = VK_LOGIC_OP_OR_REVERSE,
	[GmgLogicOp_copy_inverted] = VK_LOGIC_OP_COPY_INVERTED,
	[GmgLogicOp_or_inverted] = VK_LOGIC_OP_OR_INVERTED,
	[GmgLogicOp_nand] = VK_LOGIC_OP_NAND,
	[GmgLogicOp_set] = VK_LOGIC_OP_SET,
};

static VkBlendFactor _gmg_vulkan_convert_to_blend_factor[] = {
	[GmgBlendFactor_zero] = VK_BLEND_FACTOR_ZERO,
	[GmgBlendFactor_one] = VK_BLEND_FACTOR_ONE,
	[GmgBlendFactor_src_color] = VK_BLEND_FACTOR_SRC_COLOR,
	[GmgBlendFactor_one_minus_src_color] = VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR,
	[GmgBlendFactor_dst_color] = VK_BLEND_FACTOR_DST_COLOR,
	[GmgBlendFactor_one_minus_dst_color] = VK_BLEND_FACTOR_ONE_MINUS_DST_COLOR,
	[GmgBlendFactor_src_alpha] = VK_BLEND_FACTOR_SRC_ALPHA,
	[GmgBlendFactor_one_minus_src_alpha] = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
	[GmgBlendFactor_dst_alpha] = VK_BLEND_FACTOR_DST_ALPHA,
	[GmgBlendFactor_one_minus_dst_alpha] = VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA,
	[GmgBlendFactor_constant_color] = VK_BLEND_FACTOR_CONSTANT_COLOR,
	[GmgBlendFactor_one_minus_constant_color] = VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_COLOR,
	[GmgBlendFactor_constant_alpha] = VK_BLEND_FACTOR_CONSTANT_ALPHA,
	[GmgBlendFactor_one_minus_constant_alpha] = VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_ALPHA,
	[GmgBlendFactor_src_alpha_saturate] = VK_BLEND_FACTOR_SRC_ALPHA_SATURATE,
	[GmgBlendFactor_src1_color] = VK_BLEND_FACTOR_SRC1_COLOR,
	[GmgBlendFactor_one_minus_src1_color] = VK_BLEND_FACTOR_ONE_MINUS_SRC1_COLOR,
	[GmgBlendFactor_src1_alpha] = VK_BLEND_FACTOR_SRC1_ALPHA,
	[GmgBlendFactor_one_minus_src1_alpha] = VK_BLEND_FACTOR_ONE_MINUS_SRC1_ALPHA,
};

static VkBlendOp _gmg_vulkan_convert_to_blend_op[] = {
	[GmgBlendOp_add] = VK_BLEND_OP_ADD,
	[GmgBlendOp_subtract] = VK_BLEND_OP_SUBTRACT,
	[GmgBlendOp_reverse_subtract] = VK_BLEND_OP_REVERSE_SUBTRACT,
	[GmgBlendOp_min] = VK_BLEND_OP_MIN,
	[GmgBlendOp_max] = VK_BLEND_OP_MAX,
};

static VkFrontFace _gmg_vulkan_convert_to_front_face[] = {
	[GmgFrontFace_counter_clockwise] = VK_FRONT_FACE_COUNTER_CLOCKWISE,
	[GmgFrontFace_clockwise] = VK_FRONT_FACE_CLOCKWISE,
};

static VkPolygonMode _gmg_vulkan_convert_to_polygon_mode[] = {
	[GmgPolygonMode_fill] = VK_POLYGON_MODE_FILL,
	[GmgPolygonMode_line] = VK_POLYGON_MODE_LINE,
	[GmgPolygonMode_point] = VK_POLYGON_MODE_POINT,
};

static GmgResult _gmg_vulkan_convert_to_stencil_op_state(GmgStencilOpState* gmg, VkStencilOpState* vk_out) {
	*vk_out = (VkStencilOpState) {
		.failOp = _gmg_vulkan_convert_to_stencil_op[gmg->fail_op],
		.passOp = _gmg_vulkan_convert_to_stencil_op[gmg->pass_op],
		.depthFailOp = _gmg_vulkan_convert_to_stencil_op[gmg->depth_fail_op],
		.compareOp = _gmg_vulkan_convert_to_compare_op[gmg->depth_fail_op],
		.compareMask = gmg->compare_mask,
		.writeMask = gmg->write_mask,
		.reference = gmg->reference,
	};

	return GmgResult_success;
}

static GmgResult _gmg_vulkan_convert_to_pipeline_color_blend_attachement(GmgRenderStateBlendAttachment* gmg, VkPipelineColorBlendAttachmentState* vk_out) {
	VkColorComponentFlags vk_write_mask = 0;
	if (gmg->color_write_mask & GmgColorComponentFlags_r) vk_write_mask |= VK_COLOR_COMPONENT_R_BIT;
	if (gmg->color_write_mask & GmgColorComponentFlags_g) vk_write_mask |= VK_COLOR_COMPONENT_G_BIT;
	if (gmg->color_write_mask & GmgColorComponentFlags_b) vk_write_mask |= VK_COLOR_COMPONENT_B_BIT;
	if (gmg->color_write_mask & GmgColorComponentFlags_a) vk_write_mask |= VK_COLOR_COMPONENT_A_BIT;

	*vk_out = (VkPipelineColorBlendAttachmentState) {
		.blendEnable = gmg->blend_enable,
		.srcColorBlendFactor = _gmg_vulkan_convert_to_blend_factor[gmg->src_color_blend_factor],
		.dstColorBlendFactor = _gmg_vulkan_convert_to_blend_factor[gmg->dst_color_blend_factor],
		.colorBlendOp = _gmg_vulkan_convert_to_blend_op[gmg->color_blend_op],
		.srcAlphaBlendFactor = _gmg_vulkan_convert_to_blend_factor[gmg->src_alpha_blend_factor],
		.dstAlphaBlendFactor = _gmg_vulkan_convert_to_blend_factor[gmg->dst_alpha_blend_factor],
		.alphaBlendOp = _gmg_vulkan_convert_to_blend_op[gmg->alpha_blend_op],
		.colorWriteMask = vk_write_mask,
	};
	return GmgResult_success;
}

static VkFormat _gmg_vulkan_convert_to_format[GmgTextureFormat_COUNT] = {
	[GmgTextureFormat_r8_unorm] = VK_FORMAT_R8_UNORM,
	[GmgTextureFormat_r8g8_unorm] = VK_FORMAT_R8G8_UNORM,
	[GmgTextureFormat_r8g8b8_unorm] = VK_FORMAT_R8G8B8_UNORM,
	[GmgTextureFormat_r8g8b8a8_unorm] = VK_FORMAT_R8G8B8A8_UNORM,
	[GmgTextureFormat_b8g8r8_unorm] = VK_FORMAT_B8G8R8_UNORM,
	[GmgTextureFormat_b8g8r8a8_unorm] = VK_FORMAT_B8G8R8A8_UNORM,
	[GmgTextureFormat_d16] = VK_FORMAT_D16_UNORM,
	[GmgTextureFormat_d32] = VK_FORMAT_D32_SFLOAT,
	[GmgTextureFormat_s8] = VK_FORMAT_S8_UINT,
	[GmgTextureFormat_d16_s8] = VK_FORMAT_D16_UNORM_S8_UINT,
	[GmgTextureFormat_d24_s8] = VK_FORMAT_D24_UNORM_S8_UINT,
	[GmgTextureFormat_d32_s8] = VK_FORMAT_D32_SFLOAT_S8_UINT,
};

static GmgResult _gmg_vulkan_convert_from_format(VkFormat vk, GmgTextureFormat* gmg_out) {
	GmgTextureFormat gmg;
	switch (vk) {
		case VK_FORMAT_R8_UNORM: gmg = GmgTextureFormat_r8_unorm; break;
		case VK_FORMAT_R8G8_UNORM: gmg = GmgTextureFormat_r8g8_unorm; break;
		case VK_FORMAT_R8G8B8_UNORM: gmg = GmgTextureFormat_r8g8b8_unorm; break;
		case VK_FORMAT_R8G8B8A8_UNORM: gmg = GmgTextureFormat_r8g8b8a8_unorm; break;
		case VK_FORMAT_B8G8R8_UNORM: gmg = GmgTextureFormat_b8g8r8_unorm; break;
		case VK_FORMAT_B8G8R8A8_UNORM: gmg = GmgTextureFormat_b8g8r8a8_unorm; break;
		case VK_FORMAT_D16_UNORM: gmg = GmgTextureFormat_d16; break;
		case VK_FORMAT_D32_SFLOAT: gmg = GmgTextureFormat_d32; break;
		case VK_FORMAT_S8_UINT: gmg = GmgTextureFormat_s8; break;
		case VK_FORMAT_D16_UNORM_S8_UINT: gmg = GmgTextureFormat_d16_s8; break;
		case VK_FORMAT_D24_UNORM_S8_UINT: gmg = GmgTextureFormat_d24_s8; break;
		case VK_FORMAT_D32_SFLOAT_S8_UINT: gmg = GmgTextureFormat_d32_s8; break;
		default: das_abort("unhandled vulkan format %u", vk); break;
	}

	*gmg_out = gmg;
	return GmgResult_success;
}

static GmgResult _gmg_vulkan_convert_to_render_pass_args(GmgLogicalDevice* logical_device,
	GmgAttachmentLayout* attachments, uint32_t attachments_count,
	GmgBool is_render_pass_layout, VkAttachmentDescription** vk_attachments_out, VkSubpassDescription* vk_subpass_out
) {
	DasAlctor temp_alctor = gmg_logical_device_temp_alctor(logical_device);
	VkAttachmentDescription* vk_attachments = das_alloc_array(VkAttachmentDescription, temp_alctor, attachments_count);
	if (vk_attachments == NULL) return _gmg_stacktrace_add(GmgResultType_error_out_of_memory_host);
	if (is_render_pass_layout) {
		das_zero_elmts(vk_attachments, attachments_count);
	}

	DasStk(VkAttachmentReference) vk_color_refs = {0};
	VkAttachmentReference* vk_depth_ref = NULL;

	for (uint32_t i = 0; i < attachments_count; i += 1) {
		GmgAttachmentLayout* gmg = &attachments[i];
		GmgBool is_depth = gmg->format >= GmgTextureFormat_d16;

		VkAttachmentDescription* vk = &vk_attachments[i];
		vk->format = _gmg_vulkan_convert_to_format[gmg->format];
		vk->samples = gmg->samples_count;

		VkImageLayout final_layout;
		if (is_depth) final_layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
		else final_layout = gmg->present ? VK_IMAGE_LAYOUT_PRESENT_SRC_KHR : VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		vk->initialLayout = final_layout;
		vk->finalLayout = final_layout;

		VkAttachmentReference* ref;
		if (is_depth) {
			ref = das_alloc_elmt(VkAttachmentReference, temp_alctor);
			if (vk_depth_ref) return _gmg_stacktrace_add(GmgResultType_error_multiple_depth_stencil_attachments);
			vk_depth_ref = ref;
		} else {
			ref = DasStk_push(&vk_color_refs, NULL);
		}
		if (ref == NULL) return _gmg_stacktrace_add(GmgResultType_error_out_of_memory_host);
		ref->attachment = i;
		ref->layout = is_depth ? VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL : VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	}

	VkSubpassDescription vk_subpass = {0};
	vk_subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	vk_subpass.colorAttachmentCount = DasStk_count(&vk_color_refs);
	vk_subpass.pColorAttachments = DasStk_data(&vk_color_refs);
	vk_subpass.pDepthStencilAttachment = vk_depth_ref;

	*vk_attachments_out = vk_attachments;
	*vk_subpass_out = vk_subpass;

	return GmgResult_success;
}

static VkFormat _gmg_vulkan_convert_to_vertex_input_format[GmgVertexElmtType_COUNT][GmgVertexVectorType_COUNT] = {
	[GmgVertexElmtType_u8] = {
		[GmgVertexVectorType_1] = VK_FORMAT_R8_UINT,
		[GmgVertexVectorType_2] = VK_FORMAT_R8G8_UINT,
		[GmgVertexVectorType_3] = VK_FORMAT_R8G8B8_UINT,
		[GmgVertexVectorType_4] = VK_FORMAT_R8G8B8A8_UINT,
	},
	[GmgVertexElmtType_s8] = {
		[GmgVertexVectorType_1] = VK_FORMAT_R8_SINT,
		[GmgVertexVectorType_2] = VK_FORMAT_R8G8_SINT,
		[GmgVertexVectorType_3] = VK_FORMAT_R8G8B8_SINT,
		[GmgVertexVectorType_4] = VK_FORMAT_R8G8B8A8_SINT,
	},
	[GmgVertexElmtType_u8_f32] = {
		[GmgVertexVectorType_1] = VK_FORMAT_R8_USCALED,
		[GmgVertexVectorType_2] = VK_FORMAT_R8G8_USCALED,
		[GmgVertexVectorType_3] = VK_FORMAT_R8G8B8_USCALED,
		[GmgVertexVectorType_4] = VK_FORMAT_R8G8B8A8_USCALED,
	},
	[GmgVertexElmtType_s8_f32] = {
		[GmgVertexVectorType_1] = VK_FORMAT_R8_SSCALED,
		[GmgVertexVectorType_2] = VK_FORMAT_R8G8_SSCALED,
		[GmgVertexVectorType_3] = VK_FORMAT_R8G8B8_SSCALED,
		[GmgVertexVectorType_4] = VK_FORMAT_R8G8B8A8_SSCALED,
	},
	[GmgVertexElmtType_u8_f32_normalize] = {
		[GmgVertexVectorType_1] = VK_FORMAT_R8_UNORM,
		[GmgVertexVectorType_2] = VK_FORMAT_R8G8_UNORM,
		[GmgVertexVectorType_3] = VK_FORMAT_R8G8B8_UNORM,
		[GmgVertexVectorType_4] = VK_FORMAT_R8G8B8A8_UNORM,
	},
	[GmgVertexElmtType_s8_f32_normalize] = {
		[GmgVertexVectorType_1] = VK_FORMAT_R8_SNORM,
		[GmgVertexVectorType_2] = VK_FORMAT_R8G8_SNORM,
		[GmgVertexVectorType_3] = VK_FORMAT_R8G8B8_SNORM,
		[GmgVertexVectorType_4] = VK_FORMAT_R8G8B8A8_SNORM,
	},
	[GmgVertexElmtType_u16] = {
		[GmgVertexVectorType_1] = VK_FORMAT_R16_UINT,
		[GmgVertexVectorType_2] = VK_FORMAT_R16G16_UINT,
		[GmgVertexVectorType_3] = VK_FORMAT_R16G16B16_UINT,
		[GmgVertexVectorType_4] = VK_FORMAT_R16G16B16A16_UINT,
	},
	[GmgVertexElmtType_s16] = {
		[GmgVertexVectorType_1] = VK_FORMAT_R16_SINT,
		[GmgVertexVectorType_2] = VK_FORMAT_R16G16_SINT,
		[GmgVertexVectorType_3] = VK_FORMAT_R16G16B16_SINT,
		[GmgVertexVectorType_4] = VK_FORMAT_R16G16B16A16_SINT,
	},
	[GmgVertexElmtType_u16_f32] = {
		[GmgVertexVectorType_1] = VK_FORMAT_R16_USCALED,
		[GmgVertexVectorType_2] = VK_FORMAT_R16G16_USCALED,
		[GmgVertexVectorType_3] = VK_FORMAT_R16G16B16_USCALED,
		[GmgVertexVectorType_4] = VK_FORMAT_R16G16B16A16_USCALED,
	},
	[GmgVertexElmtType_s16_f32] = {
		[GmgVertexVectorType_1] = VK_FORMAT_R16_SSCALED,
		[GmgVertexVectorType_2] = VK_FORMAT_R16G16_SSCALED,
		[GmgVertexVectorType_3] = VK_FORMAT_R16G16B16_SSCALED,
		[GmgVertexVectorType_4] = VK_FORMAT_R16G16B16A16_SSCALED,
	},
	[GmgVertexElmtType_u16_f32_normalize] = {
		[GmgVertexVectorType_1] = VK_FORMAT_R16_UNORM,
		[GmgVertexVectorType_2] = VK_FORMAT_R16G16_UNORM,
		[GmgVertexVectorType_3] = VK_FORMAT_R16G16B16_UNORM,
		[GmgVertexVectorType_4] = VK_FORMAT_R16G16B16A16_UNORM,
	},
	[GmgVertexElmtType_s16_f32_normalize] = {
		[GmgVertexVectorType_1] = VK_FORMAT_R16_SNORM,
		[GmgVertexVectorType_2] = VK_FORMAT_R16G16_SNORM,
		[GmgVertexVectorType_3] = VK_FORMAT_R16G16B16_SNORM,
		[GmgVertexVectorType_4] = VK_FORMAT_R16G16B16A16_SNORM,
	},
	[GmgVertexElmtType_u32] = {
		[GmgVertexVectorType_1] = VK_FORMAT_R32_UINT,
		[GmgVertexVectorType_2] = VK_FORMAT_R32G32_UINT,
		[GmgVertexVectorType_3] = VK_FORMAT_R32G32B32_UINT,
		[GmgVertexVectorType_4] = VK_FORMAT_R32G32B32A32_UINT,
	},
	[GmgVertexElmtType_s32] = {
		[GmgVertexVectorType_1] = VK_FORMAT_R32_SINT,
		[GmgVertexVectorType_2] = VK_FORMAT_R32G32_SINT,
		[GmgVertexVectorType_3] = VK_FORMAT_R32G32B32_SINT,
		[GmgVertexVectorType_4] = VK_FORMAT_R32G32B32A32_SINT,
	},
	[GmgVertexElmtType_u64] = {
		[GmgVertexVectorType_1] = VK_FORMAT_R64_UINT,
		[GmgVertexVectorType_2] = VK_FORMAT_R64G64_UINT,
		[GmgVertexVectorType_3] = VK_FORMAT_R64G64B64_UINT,
		[GmgVertexVectorType_4] = VK_FORMAT_R64G64B64A64_UINT,
	},
	[GmgVertexElmtType_s64] = {
		[GmgVertexVectorType_1] = VK_FORMAT_R64_SINT,
		[GmgVertexVectorType_2] = VK_FORMAT_R64G64_SINT,
		[GmgVertexVectorType_3] = VK_FORMAT_R64G64B64_SINT,
		[GmgVertexVectorType_4] = VK_FORMAT_R64G64B64A64_SINT,
	},
	[GmgVertexElmtType_f16] = {
		[GmgVertexVectorType_1] = VK_FORMAT_R16_SFLOAT,
		[GmgVertexVectorType_2] = VK_FORMAT_R16G16_SFLOAT,
		[GmgVertexVectorType_3] = VK_FORMAT_R16G16B16_SFLOAT,
		[GmgVertexVectorType_4] = VK_FORMAT_R16G16B16A16_SFLOAT,
	},
	[GmgVertexElmtType_f32] = {
		[GmgVertexVectorType_1] = VK_FORMAT_R32_SFLOAT,
		[GmgVertexVectorType_2] = VK_FORMAT_R32G32_SFLOAT,
		[GmgVertexVectorType_3] = VK_FORMAT_R32G32B32_SFLOAT,
		[GmgVertexVectorType_4] = VK_FORMAT_R32G32B32A32_SFLOAT,
	},
	[GmgVertexElmtType_f64] = {
		[GmgVertexVectorType_1] = VK_FORMAT_R64_SFLOAT,
		[GmgVertexVectorType_2] = VK_FORMAT_R64G64_SFLOAT,
		[GmgVertexVectorType_3] = VK_FORMAT_R64G64B64_SFLOAT,
		[GmgVertexVectorType_4] = VK_FORMAT_R64G64B64A64_SFLOAT,
	},
};

VkIndexType _gmg_vulkan_convert_to_index_type[] = {
	[GmgIndexType_u8] = VK_INDEX_TYPE_UINT8_EXT,
	[GmgIndexType_u16] = VK_INDEX_TYPE_UINT16,
	[GmgIndexType_u32] = VK_INDEX_TYPE_UINT32,
};

typedef_DasStk(VkWriteDescriptorSet);
static GmgResult _gmg_vulkan_convert_to_descriptor_write(GmgLogicalDevice* logical_device, VkDescriptorSet descriptor_set, _GmgDescriptor* d, DasStk(VkWriteDescriptorSet)* vk_writes) {
	VkWriteDescriptorSet* vk_write_elmt = DasStk_push(vk_writes, NULL);
	if (!vk_write_elmt) return _gmg_stacktrace_add(GmgResultType_error_out_of_memory_host);

	*vk_write_elmt = (VkWriteDescriptorSet) {
		.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
		.pNext = NULL,
		.dstSet =  descriptor_set,
		.dstBinding = d->binding_idx,
		.dstArrayElement = d->elmt_idx,
		.descriptorCount = 1,
		.descriptorType = _gmg_vulkan_convert_to_descriptor_type[d->type],
	};

	DasAlctor temp_alctor = DasStk_alctor(vk_writes);
	GmgResult result;
	switch (d->type) {
		case GmgDescriptorType_sampler: {
			GmgSampler* sampler;
			result = gmg_sampler_get(logical_device, ((GmgSamplerId) { .object_id = d->object_id }), &sampler);
			if (result.type < 0) return result;

			VkDescriptorImageInfo* info = das_alloc_elmt(VkDescriptorImageInfo, temp_alctor);
			info->sampler = sampler->vulkan.handle;
			vk_write_elmt->pImageInfo = info;
			break;
		};
		case GmgDescriptorType_texture: {
			VkDescriptorImageInfo* info = das_alloc_elmt(VkDescriptorImageInfo, temp_alctor);
			if (d->object_id.raw) {
				GmgTexture* texture;
				result = gmg_texture_get(logical_device, ((GmgTextureId) { .object_id = d->object_id }), &texture);
				if (result.type < 0) return result;

				info->imageView = texture->vulkan.image_view;
			} else {
				info->imageView = VK_NULL_HANDLE;
			}
			info->imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			vk_write_elmt->pImageInfo = info;
			break;
		};
		case GmgDescriptorType_uniform_buffer:
		case GmgDescriptorType_uniform_buffer_dynamic:
		case GmgDescriptorType_storage_buffer:
		case GmgDescriptorType_storage_buffer_dynamic: {
			GmgBuffer* buffer;
			result = gmg_buffer_get(logical_device, ((GmgBufferId) { .object_id = d->object_id }), &buffer);
			if (result.type < 0) return result;

			VkDescriptorBufferInfo* info = das_alloc_elmt(VkDescriptorBufferInfo, temp_alctor);
			info->buffer = buffer->vulkan.handle;
			info->offset = d->buffer_start_idx * (uint64_t)buffer->elmt_size;
			info->range = buffer->elmt_size;

			vk_write_elmt->pBufferInfo = info;
			break;
		};
	}

	return GmgResult_success;
}

static int _gmg_vulkan_command_buffer_build_transfer_job(void* args_) {
	_GmgVulkanCommandBufferBuildTransfer* args = args_;

	GmgLogicalDevice* logical_device = args->logical_device;
	VkCommandBuffer vk_command_buffer = args->command_buffer;

	VkResult vk_result = vkResetCommandBuffer(vk_command_buffer, 0);
	if (vk_result < 0) goto VK_END;

	VkCommandBufferBeginInfo vk_command_buffer_begin_info = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
		.pNext = NULL,
		.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
		.pInheritanceInfo = NULL,
	};

	vk_result = vkBeginCommandBuffer(vk_command_buffer, &vk_command_buffer_begin_info);
	if (vk_result < 0) goto VK_END;

	_GmgVulkanMemoryStager* stager = &logical_device->vulkan.memory_stager;

	for (uint32_t i = 0; i < stager->filled_buffers_count + 1; i += 1) {
		_GmgVulkanStagingBuffer* buffer = DasStk_get(&stager->buffers, i);
		if (i == stager->filled_buffers_count && buffer->position == 0) {
			break;
		}

		if (!DasStk_push(args->used_staging_buffers, &buffer->core)) {
			vk_result = VK_ERROR_OUT_OF_HOST_MEMORY;
			goto VK_END;
		}

		VkBuffer src_buffer = buffer->core.buffer;
		buffer->core.buffer = VK_NULL_HANDLE;

		DasStk_foreach(&buffer->staged_buffers, j) {
			_GmgVulkanStagedBuffer* stage = DasStk_get(&buffer->staged_buffers, j);

			VkBufferCopy copy_region = {
				.srcOffset = stage->src_buffer_offset,
				.dstOffset = stage->dst_buffer_offset,
				.size = stage->size,
			};

			vkCmdCopyBuffer(vk_command_buffer, src_buffer, stage->dst_buffer, 1, &copy_region);
		}

		DasStk_clear(&buffer->staged_buffers);

		DasStk_foreach(&buffer->staged_textures, j) {
			_GmgVulkanStagedTexture* stage = DasStk_get(&buffer->staged_textures, j);
			GmgTextureArea* area = &stage->area;

			VkBufferImageCopy copy_region = {
				.bufferOffset = stage->src_buffer_offset,
				.bufferRowLength = 0,
				.bufferImageHeight = 0,
				.imageSubresource = {
					.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
					.mipLevel = area->mip_level,
					.baseArrayLayer = area->array_layer,
					.layerCount = area->array_layers_count,
				},
				.imageOffset = {
					.x = area->offset_x,
					.y = area->offset_y,
					.z = area->offset_z,
				},
				.imageExtent = {
					.width = area->width,
					.height = area->height,
					.depth = area->depth,
				},
			};

			VkImageMemoryBarrier memory_barrier = {
				.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
				.pNext = NULL,
				.srcAccessMask = 0,
				.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
				.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
				.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				.srcQueueFamilyIndex = logical_device->vulkan.queue_type_to_queue_family_idx[_GmgVulkanQueueType_graphics],
				.dstQueueFamilyIndex = logical_device->vulkan.queue_type_to_queue_family_idx[_GmgVulkanQueueType_graphics],
				.image = stage->dst_image,
				.subresourceRange = {
					.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
					.baseMipLevel = area->mip_level,
					.levelCount = 1,
					.baseArrayLayer = area->array_layer,
					.layerCount = area->array_layers_count,
				},
			};

			vkCmdPipelineBarrier(vk_command_buffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, NULL, 0, NULL, 1, &memory_barrier);

			vkCmdCopyBufferToImage(vk_command_buffer, src_buffer, stage->dst_image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copy_region);

			memory_barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
			memory_barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			memory_barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			memory_barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
			vkCmdPipelineBarrier(vk_command_buffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT, 0, 0, NULL, 0, NULL, 1, &memory_barrier);

		}

		DasStk_clear(&buffer->staged_textures);

		buffer->position = 0;
	}

	stager->filled_buffers_count = 0;

	vk_result = vkEndCommandBuffer(vk_command_buffer);
	if (vk_result < 0) goto VK_END;

VK_END:
	_gmg_logical_device_add_job_result(logical_device, _gmg_vulkan_convert_from_result(vk_result));
	return 0;
}

static GmgResult _gmg_vulkan_descriptor_set_allocator_alloc(GmgLogicalDevice* logical_device, GmgDescriptorSetAllocator* allocator, GmgDescriptorSetLayoutId layout_id, VkDescriptorSet* out) {
	uint32_t layout_idx;
	GmgResult result = _gmg_descriptor_set_allocator_get_layout_idx(logical_device, allocator, layout_id, &layout_idx);
	if (result.type < 0) return result;

	_GmgVkDescriptorSetAllocatorLayoutAux* layout_aux = &allocator->vulkan.layout_auxs[layout_idx];

	if (DasStk_count(&layout_aux->free_sets)) {
		*out = *DasStk_get_last(&layout_aux->free_sets);
		DasStk_pop(&layout_aux->free_sets);
		return GmgResult_success;
	}

	if (layout_aux->free_pool_idx == DasStk_count(&allocator->vulkan.pools)) {
		_GmgVkDescriptorSetAllocatorPool* pool = DasStk_push(&allocator->vulkan.pools, NULL);
		if (pool == NULL) return _gmg_stacktrace_add(GmgResultType_error_out_of_memory_host);

		pool->layout_free_counts = das_alloc_array(uint32_t, GMG_TODO_INTERNAL_ALLOCATOR, allocator->layouts_count);
		if (pool->layout_free_counts == NULL) return _gmg_stacktrace_add(GmgResultType_error_out_of_memory_host);

		for (uint32_t i = 0; i < allocator->layouts_count; i += 1) {
			pool->layout_free_counts[i] = allocator->vulkan.layout_auxs[i].cap_per_pool;
		}

		VkDescriptorPoolCreateInfo vk_pool_create_info = {
			.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
			.pNext = NULL,
			.flags = 0,
			.maxSets = allocator->vulkan.pool_max_sets,
			.poolSizeCount = allocator->vulkan.pool_sizes_count,
			.pPoolSizes = allocator->vulkan.pool_sizes,
		};

		VkResult vk_result = vkCreateDescriptorPool(logical_device->vulkan.handle, &vk_pool_create_info, GMG_TODO_VULKAN_ALLOCATOR, &pool->handle);
		if (vk_result) return _gmg_vulkan_convert_from_result(vk_result);
	}

	_GmgVkDescriptorSetAllocatorPool* pool = DasStk_get(&allocator->vulkan.pools, layout_aux->free_pool_idx);
	pool->layout_free_counts[layout_idx] -= 1;

	if (pool->layout_free_counts[layout_idx] == 0) {
		layout_aux->free_pool_idx += 1;
	}

	GmgDescriptorSetLayout* layout;
	result = gmg_descriptor_set_layout_get(logical_device, layout_id, &layout);
	if (result.type < 0) return result;

	VkDescriptorSetAllocateInfo vk_alloc_info = {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
		.pNext = NULL,
		.descriptorPool = pool->handle,
		.descriptorSetCount = 1,
		.pSetLayouts = &layout->vulkan.handle,
	};

	VkResult vk_result = vkAllocateDescriptorSets(logical_device->vulkan.handle, &vk_alloc_info, out);
	if (vk_result) _gmg_vulkan_convert_from_result(vk_result);

	return GmgResult_success;
}

static GmgResult _gmg_vulkan_descriptor_set_allocator_dealloc(GmgLogicalDevice* logical_device, _GmgVulkanDescriptorSetToBeDeallocated* entry) {
	GmgDescriptorSetAllocator* allocator;
	GmgResult result = gmg_descriptor_set_allocator_get(logical_device, entry->allocator_id, &allocator);
	if (result.type < 0) return result;

	uint32_t layout_idx;
	result = _gmg_descriptor_set_allocator_get_layout_idx(logical_device, allocator, entry->layout_id, &layout_idx);
	if (result.type < 0) return result;

	_GmgVkDescriptorSetAllocatorLayoutAux* layout_aux = &allocator->vulkan.layout_auxs[layout_idx];
	if (!DasStk_push(&layout_aux->free_sets, &entry->handle)) return _gmg_stacktrace_add(GmgResultType_error_out_of_memory_host);

	return GmgResult_success;
}

static GmgResult _gmg_vulkan_descriptor_set_mark_for_deallocation(GmgLogicalDevice* logical_device, VkFence latest_used_fence, _GmgVulkanDescriptorSetToBeDeallocated* entry) {
	DasDeque_foreach(&logical_device->vulkan.graphics_submits, i) {
		_GmgVulkanSubmit* submit = DasDeque_get(&logical_device->vulkan.graphics_submits, i);
		GmgBool found = gmg_false;
		if (submit->fence == latest_used_fence) {
			if (!DasStk_push(&submit->descriptor_sets_to_be_deallocated, &entry)) {
				return _gmg_stacktrace_add(GmgResultType_error_out_of_memory_host);
			}

			found = gmg_true;
			break;
		}

		if (!found) {
			//
			// the submission the fence belongs to has already been freed
			//
			GmgResult result = _gmg_vulkan_descriptor_set_allocator_dealloc(logical_device, entry);
			if (result.type < 0) return result;
		}
	}
	return GmgResult_success;
}

static GmgResult _gmg_vulkan_descriptor_set_maybe_update(_GmgVulkanCommandBufferBuildGraphics* build, GmgDescriptorSetLayoutId layout_id, GmgDescriptorSetAllocatorId allocator_id, _GmgVkDescriptorSet* set, DasStk(VkWriteDescriptorSet)* vk_writes) {
	GmgLogicalDevice* logical_device = build->logical_device;

	GmgDescriptorSetAllocator* allocator;
	GmgResult result = gmg_descriptor_set_allocator_get(logical_device, allocator_id, &allocator);
	if (result.type < 0) return result;
	if (!set->has_updated && allocator->vulkan.reset_counter == set->reset_counter) {
		return GmgResult_success;
	}

	set->has_updated = gmg_false;
	set->reset_counter = allocator->vulkan.reset_counter;

	if (set->handle) {
		_GmgVulkanDescriptorSetToBeDeallocatedEntry* to_be_dealloc = DasStk_push(&build->descriptor_sets_to_be_deallocated, NULL);
		if (!to_be_dealloc) return _gmg_stacktrace_add(GmgResultType_error_out_of_memory_host);
		*to_be_dealloc = (_GmgVulkanDescriptorSetToBeDeallocatedEntry) {
			.latest_used_fence = set->latest_used_fence,
			.inner = {
				.handle = set->handle,
				.layout_id = layout_id,
				.allocator_id = allocator_id,
			}
		};
	}

	result = _gmg_vulkan_descriptor_set_allocator_alloc(build->logical_device, allocator, layout_id, &set->handle);
	if (result.type < 0) return result;

	set->latest_used_fence = build->latest_fence;

	for (uint32_t i = 0; i < set->descriptors_count; i += 1) {
		_GmgDescriptor* d = &set->descriptors[i];
		result = _gmg_vulkan_convert_to_descriptor_write(logical_device, set->handle, d, vk_writes);
		if (result.type < 0) return result;
	}

	return GmgResult_success;
}

static int _gmg_vulkan_command_buffer_build_graphics_job(void* args_) {
	_GmgVulkanCommandBufferBuildGraphics* args = args_;

	GmgLogicalDevice* logical_device = args->logical_device;
	GmgRenderPass* render_pass = args->render_pass;
	VkCommandBuffer vk_command_buffer = args->command_buffer;
	VkFence latest_fence = args->latest_fence;
	DasAlctor temp_alctor = DasLinearAlctor_as_das(&args->temp_alctor);
	das_alloc_reset(temp_alctor);

	GmgResult result;

	if (!DasStk_init_with_alctor(&args->descriptor_sets_to_be_deallocated, DasStk_min_cap, temp_alctor)) {
		result = _gmg_stacktrace_add(GmgResultType_error_out_of_memory_host);
		goto END;
	}

	DasStk(VkWriteDescriptorSet) vk_writes;
	if (!DasStk_init_with_alctor(&vk_writes, DasStk_min_cap, temp_alctor)) {
		result = _gmg_stacktrace_add(GmgResultType_error_out_of_memory_host);
		goto END;
	}

	result = _gmg_vulkan_descriptor_set_maybe_update(args, render_pass->pass_descriptor_set_layout_id, render_pass->pass_descriptor_set_allocator_id, &render_pass->vulkan.pass_descriptor_set, &vk_writes);
	if (result.type < 0) goto END;


	if (render_pass->draw_cmd_descriptor_set_allocator_id.raw) {
		//
		// look in the current draw commands recording for any descriptors that have not been reused/allocated.
		// for each of those, allocate a descriptor set and then create all the update structures
		// to write the descriptors to that set.
		//
		GmgDescriptorSetAllocator* allocator;
		result = gmg_descriptor_set_allocator_get(logical_device, render_pass->draw_cmd_descriptor_set_allocator_id, &allocator);
		if (result.type < 0) goto END;

		uint32_t count = 0;
		GmgVkRenderPassDescriptorSets* sets = &render_pass->vulkan.draw_cmd_sets[0];
		DasStk_foreach(&sets->entries, i) {
			GmgVkRenderPassDescriptorSet* set_elmt = DasStk_get(&sets->entries, i);
			if (set_elmt->handle != VK_NULL_HANDLE) continue;

			uint64_t key = *DasStk_get(&sets->keys, i);
			GmgDescriptorSetLayoutId layout_id = (GmgDescriptorSetLayoutId) { .raw = key & 0xffffffff };

			result = _gmg_vulkan_descriptor_set_allocator_alloc(logical_device, allocator, layout_id, &set_elmt->handle);
			if (result.type < 0) goto END;

			for (uint32_t j = set_elmt->descriptors_start_idx; j < set_elmt->descriptors_start_idx + set_elmt->descriptors_count; j += 1) {
				_GmgDescriptor* d = DasStk_get(&sets->descriptors, j);
				result = _gmg_vulkan_convert_to_descriptor_write(logical_device, set_elmt->handle, d, &vk_writes);
				if (result.type < 0) goto END;
			}

			count += 1;
		}
		sets->submission_fence = args->latest_fence;
	}

	VkResult vk_result = vkResetCommandBuffer(vk_command_buffer, 0);
	if (vk_result < 0) { result = _gmg_vulkan_convert_from_result(vk_result); goto END; }

	VkCommandBufferBeginInfo vk_command_buffer_begin_info = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
		.pNext = NULL,
		.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
		.pInheritanceInfo = NULL,
	};

	vk_result = vkBeginCommandBuffer(vk_command_buffer, &vk_command_buffer_begin_info);
	if (vk_result < 0) { result = _gmg_vulkan_convert_from_result(vk_result); goto END; }

	GmgMaterialSpecId bound_material_spec_id = {0};
	GmgBufferId bound_index_buffer_id = {0};
	GmgBufferId bound_vertex_buffer_ids[gmg_vulkan_max_bound_vertex_buffers_count] = {0};
	VkDeviceSize unused_but_must_exist_vertex_buffer_offsets[gmg_vulkan_max_bound_vertex_buffers_count] = {0};
	VkBuffer vk_vertex_buffer_buf[gmg_vulkan_max_bound_vertex_buffers_count] = {0};

	VkViewport vk_viewport;
	result = _gmg_vulkan_convert_to_viewport(&render_pass->viewport, &vk_viewport);
	if (result.type < 0) goto END;

	VkRect2D vk_scissor;
	result = _gmg_vulkan_convert_to_rect2d(&render_pass->scissor, &vk_scissor);
	if (result.type < 0) goto END;

	vkCmdSetViewport(vk_command_buffer, 0, 1, &vk_viewport);
	vkCmdSetScissor(vk_command_buffer, 0, 1, &vk_scissor);

	GmgFrameBuffer* frame_buffer;
	result = gmg_frame_buffer_get(logical_device, render_pass->frame_buffer_id, &frame_buffer);
	if (result.type < 0) goto END;

	uint32_t vk_clear_values_count = 0;
	VkClearValue* vk_clear_values = NULL;
	if (render_pass->attachment_clear_values) {
		vk_clear_values_count = frame_buffer->attachments_count;
		vk_clear_values = das_alloc_array(VkClearValue, temp_alctor, frame_buffer->attachments_count);
		if (vk_clear_values == NULL) {
			result = _gmg_stacktrace_add(GmgResultType_error_out_of_memory_host);
			goto END;
		}
		for (uint32_t i = 0; i < frame_buffer->attachments_count; i += 1) {
			GmgClearValue* gmg = &render_pass->attachment_clear_values[i];
			VkClearValue* vk = &vk_clear_values[i];

			GmgTexture* texture;
			result = gmg_texture_get(logical_device, frame_buffer->attachments[i], &texture);
			if (result.type < 0) goto END;

			if (texture->format >= GmgTextureFormat_d16) {
				vk->depthStencil.depth = gmg->depth;
				vk->depthStencil.stencil = gmg->stencil;
			} else {
				das_copy_array(vk->color.float32, gmg->color.float32);
			}
		}
	}

	VkRenderPassBeginInfo render_pass_being_info = {
		.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
		.pNext = NULL,
		.renderPass = render_pass->vulkan.handle,
		.framebuffer = frame_buffer->vulkan.handle,
		.renderArea = { .offset.x = 0, .offset.y = 0, .extent.width = frame_buffer->width, .extent.height = frame_buffer->height },
		.clearValueCount = vk_clear_values_count,
		.pClearValues = vk_clear_values,
	};

	vkCmdBeginRenderPass(vk_command_buffer, &render_pass_being_info, VK_SUBPASS_CONTENTS_INLINE);

	VkDescriptorSet bound_descriptor_sets[2] = {0};

	result = _gmg_render_pass_sort(render_pass);
	if (result.type < 0) goto END;

	//
	// vulkan expects all descriptor sets to be valid before being bound and used
	// by a vkCmdDraw* function. part of making a descriptor set valid is that
	// it must be initialized with a value...
	// so finish up by updating all the materials before we record the command buffer.
	//
	DasStk_foreach(&render_pass->vulkan.material_ids, i) {
		GmgMaterial* material;
		result = gmg_material_get(logical_device, *DasStk_get(&render_pass->vulkan.material_ids, i), &material);
		if (result.type < 0) goto END;

		if (material->descriptor_set_layout_id.raw) {
			result = _gmg_vulkan_descriptor_set_maybe_update(args, material->descriptor_set_layout_id, material->descriptor_set_allocator_id, &material->vulkan.descriptor_set, &vk_writes);
			if (result.type < 0) goto END;
		}
	}

	if (DasStk_count(&vk_writes)) {
		vkUpdateDescriptorSets(logical_device->vulkan.handle, DasStk_count(&vk_writes), DasStk_data(&vk_writes), 0, NULL);
	}

	DasStk_foreach(&render_pass->draw_cmd_sort_keys, i) {
		uint32_t cmd_idx = *DasStk_get(&render_pass->draw_cmd_sort_keys, i) & GmgRenderPass_draw_cmd_sort_key_cmd_idx_MASK;
		_GmgDrawCmd* cmd = DasStk_get(&render_pass->draw_cmds, cmd_idx);

		GmgMaterial* material;
		result = gmg_material_get(logical_device, cmd->material_id, &material);
		if (result.type < 0) goto END;

		GmgMaterialSpec* material_spec;
		result = gmg_material_spec_get(logical_device, material->spec_id, &material_spec);
		if (result.type < 0) goto END;

		GmgShader* shader;
		result = gmg_shader_get(logical_device, material_spec->shader_id, &shader);
		if (result.type < 0) goto END;

		if (bound_material_spec_id.raw != material->spec_id.raw) {
			bound_material_spec_id.raw = material->spec_id.raw;

			vkCmdBindPipeline(vk_command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, material_spec->vulkan.pipeline);

			if (render_pass->pass_descriptor_set_layout_id.raw) {
				uint32_t dynamic_descriptors_count = render_pass->pass_dynamic_descriptors_count;
				uint32_t* dynamic_descriptor_offsets = dynamic_descriptors_count
					?  render_pass->pass_dynamic_descriptor_offsets
					: NULL;

				vkCmdBindDescriptorSets(
					vk_command_buffer,
					VK_PIPELINE_BIND_POINT_GRAPHICS,
					shader->vulkan.pipeline_layout,
					0,
					1,
					&render_pass->vulkan.pass_descriptor_set.handle,
					dynamic_descriptors_count,
					dynamic_descriptor_offsets
				);
			}
			bound_descriptor_sets[0] = VK_NULL_HANDLE;
			bound_descriptor_sets[1] = VK_NULL_HANDLE;
		}

		if (material->vulkan.descriptor_set.handle && bound_descriptor_sets[0] != material->vulkan.descriptor_set.handle) {
			bound_descriptor_sets[0] = material->vulkan.descriptor_set.handle;

			vkCmdBindDescriptorSets(
				vk_command_buffer,
				VK_PIPELINE_BIND_POINT_GRAPHICS,
				shader->vulkan.pipeline_layout,
				1,
				1,
				&material->vulkan.descriptor_set.handle,
				material->dynamic_descriptors_count,
				material->dynamic_descriptor_offsets
			);
		}

		if (cmd->vulkan.draw_cmd_descriptor_set_idx_plus_1) {
			VkDescriptorSet cmd_descriptor_set = DasStk_get(&render_pass->vulkan.draw_cmd_sets[0].entries, cmd->vulkan.draw_cmd_descriptor_set_idx_plus_1 - 1)->handle;

			if (cmd_descriptor_set != bound_descriptor_sets[1]) {
				bound_descriptor_sets[1] = cmd_descriptor_set;

				uint32_t dynamic_descriptors_count = cmd->dynamic_descriptor_offsets_count;
				uint32_t* dynamic_descriptor_offsets = dynamic_descriptors_count
					?  DasStk_get(&render_pass->vulkan.draw_cmd_dynamic_descriptor_offsets, cmd->dynamic_descriptor_offsets_start_idx)
					: NULL;

				vkCmdBindDescriptorSets(
					vk_command_buffer,
					VK_PIPELINE_BIND_POINT_GRAPHICS,
					shader->vulkan.pipeline_layout,
					2,
					1,
					&cmd_descriptor_set,
					dynamic_descriptors_count,
					dynamic_descriptor_offsets
				);
			}

			if (cmd->dynamic_descriptor_offsets_count) {
				bound_descriptor_sets[1] = VK_NULL_HANDLE;
			}
		}

		if (cmd->push_constants_size) {
			GmgMaterialSpec* material_spec;
			result = gmg_material_spec_get(logical_device, material->spec_id, &material_spec);
			if (result.type < 0) goto END;

			GmgShader* shader;
			result = gmg_shader_get(logical_device, material_spec->shader_id, &shader);
			if (result.type < 0) goto END;

			VkShaderStageFlags shader_stage_flags;
			result = _gmg_vulkan_convert_to_shader_stage_flags(shader->push_constants_shader_stage_flags, &shader_stage_flags);
			if (result.type < 0) goto END;

			vkCmdPushConstants(vk_command_buffer,
				shader->vulkan.pipeline_layout,
				shader_stage_flags,
				cmd->push_constants_offset,
				cmd->push_constants_size,
				DasStk_get(&render_pass->push_constant_data,cmd->push_constants_data_start_idx));
		}


		GmgVertexArray* vertex_array;
		result = gmg_vertex_array_get(logical_device, cmd->vertex_array_id, &vertex_array);
		if (result.type < 0) goto END;

		{
			if (bound_index_buffer_id.raw != vertex_array->index_buffer_id.raw) {
				bound_index_buffer_id.raw = vertex_array->index_buffer_id.raw;

				GmgBuffer* index_buffer;
				result = gmg_buffer_get(logical_device, vertex_array->index_buffer_id, &index_buffer);
				if (result.type < 0) goto END;

				vkCmdBindIndexBuffer(vk_command_buffer, index_buffer->vulkan.handle, 0,
					_gmg_vulkan_convert_to_index_type[index_buffer->index.type]);
			}

			{
				uint32_t vertex_buffer_change_start_idx = UINT32_MAX;
				uint32_t vertex_buffer_change_end_idx;
				for (uint32_t k = 0; k < vertex_array->vertex_buffers_count; k += 1) {
					GmgBufferId vertex_buffer_id = vertex_array->vertex_buffer_ids[k];
					if (vertex_buffer_id.raw != bound_vertex_buffer_ids[k].raw) {
						bound_vertex_buffer_ids[k].raw = vertex_buffer_id.raw;
						if (vertex_buffer_change_start_idx == UINT32_MAX) {
							vertex_buffer_change_start_idx = k;
						}

						vertex_buffer_change_end_idx = k + 1;
					}
				}

				if (vertex_buffer_change_start_idx != UINT32_MAX) {
					uint32_t count = vertex_buffer_change_end_idx - vertex_buffer_change_start_idx;
					for (uint32_t k = 0, idx = vertex_buffer_change_start_idx; k < count; k += 1, idx += 1) {
						GmgBuffer* vertex_buffer;
						result = gmg_buffer_get(logical_device, vertex_array->vertex_buffer_ids[idx], &vertex_buffer);
						if (result.type < 0) goto END;

						vk_vertex_buffer_buf[k] = vertex_buffer->vulkan.handle;
					}

					vkCmdBindVertexBuffers(vk_command_buffer,
						vertex_buffer_change_start_idx,
						count,
						vk_vertex_buffer_buf,
						unused_but_must_exist_vertex_buffer_offsets);
				}
			}
		}


		if (vertex_array->index_buffer_id.raw) {
			vkCmdDrawIndexed(vk_command_buffer,
				cmd->indexed.indices_count,
				cmd->instances_count,
				cmd->indexed.indices_start_idx,
				cmd->indexed.vertices_start_idx,
				cmd->instances_start_idx);
		} else {
			vkCmdDraw(vk_command_buffer,
				cmd->vertexed.vertices_count,
				cmd->instances_count,
				cmd->vertexed.vertices_start_idx,
				cmd->instances_start_idx);
		}
	}

	vkCmdEndRenderPass(vk_command_buffer);

	vk_result = vkEndCommandBuffer(vk_command_buffer);
	if (vk_result < 0) { result = _gmg_vulkan_convert_from_result(vk_result); goto END; }

END:
	_gmg_logical_device_add_job_result(logical_device, result);
	return 0;
}

// ==========================================================
//
//
// Internal: Vulkan Implementation
//
//
// ==========================================================

static GmgResult _gmg_vulkan_backend_init(GmgSetup* setup) {
	GmgResult result;
	VkResult vk_result;

	const char* layers[5];
	const char* extensions[5];
	uint32_t layers_count = 0;
	uint32_t extensions_count = 0;

	if (setup->debug_backend) {
		layers[layers_count] = "VK_LAYER_LUNARG_api_dump";
		layers_count += 1;
		layers[layers_count] = "VK_LAYER_KHRONOS_validation";
		layers_count += 1;
	}

	extensions[extensions_count] = "VK_KHR_surface";
	extensions_count += 1;

	switch (setup->display_manager_type) {
#ifdef _WIN32
		case GmgDisplayManagerType_windows:
			extensions[extensions_count] = VK_KHR_WIN32_SURFACE_EXTENSION_NAME;
			extensions_count += 1;
			break;
#endif // _WIN32
#ifdef __APPLE__
		case GmgDisplayManagerType_macos:
			extensions[extensions_count] = VK_MVK_MACOS_SURFACE_EXTENSION_NAME;
			extensions_count += 1;
			break;
		case GmgDisplayManagerType_ios:
			extensions[extensions_count] = VK_MVK_IOS_SURFACE_EXTENSION_NAME;
			extensions_count += 1;
			break;
#endif //  __APPLE__
#ifdef __unix__
#ifdef GMG_ENABLE_XLIB
		case GmgDisplayManagerType_xlib:
			extensions[extensions_count] = VK_KHR_XLIB_SURFACE_EXTENSION_NAME;
			extensions_count += 1;
			break;
#endif // GMG_ENABLE_XLIB
#ifdef GMG_ENABLE_XCB
		case GmgDisplayManagerType_xcb:
			extensions[extensions_count] = VK_KHR_XCB_SURFACE_EXTENSION_NAME;
			extensions_count += 1;
			break;
#endif // GMG_ENABLE_XCB
#ifdef GMG_ENABLE_WAYLAND
		case GmgDisplayManagerType_wayland:
			extensions[extensions_count] = VK_KHR_WAYLAND_SURFACE_EXTENSION_NAME;
			extensions_count += 1;
			break;
#endif // GMG_ENABLE_WAYLAND
#endif // __unix__
#ifdef __ANDROID__
		case GmgDisplayManagerType_android:
			extensions[extensions_count] =  VK_KHR_ANDROID_SURFACE_EXTENSION_NAME;
			extensions_count += 1;
			break;
#endif // __ANDROID__
		default: das_abort("unhandled display_manager_type %u", setup->display_manager_type);
	}

	//
	// create the instance.
	//
	VkInstance vk_instance = VK_NULL_HANDLE;
	{
		VkApplicationInfo app_info = {
			.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
			.pNext = NULL,
			.pApplicationName = setup->application_name,
			.applicationVersion = setup->application_version,
			.pEngineName = setup->engine_name,
			.engineVersion = setup->engine_version,
			.apiVersion = VK_MAKE_VERSION(1, 0, 0),
		};

		VkInstanceCreateInfo vk_create_info = {
			.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
			.pNext = NULL,
			.flags = 0,
			.pApplicationInfo = &app_info,
			.enabledLayerCount = layers_count,
			.ppEnabledLayerNames = (const char* const*)layers,
			.enabledExtensionCount = extensions_count,
			.ppEnabledExtensionNames = (const char* const*)extensions,
		};

		vk_result = vkCreateInstance(&vk_create_info, GMG_TODO_VULKAN_ALLOCATOR, &vk_instance);
		if (vk_result) return _gmg_vulkan_convert_from_result(vk_result);
	}
	_gmg.vulkan.instance = vk_instance;

	//
	// create the physical devices.
	//
	{
		VkPhysicalDevice* physical_devices = NULL;
		uint32_t physical_devices_count = 0;

		vk_result = vkEnumeratePhysicalDevices(vk_instance, &physical_devices_count, NULL);
		if (vk_result) return _gmg_vulkan_convert_from_result(vk_result);

		physical_devices = das_alloc_array(VkPhysicalDevice, GMG_TODO_INTERNAL_ALLOCATOR, physical_devices_count);
		if (physical_devices == NULL) return _gmg_stacktrace_add(GmgResultType_error_out_of_memory_host);
		vk_result = vkEnumeratePhysicalDevices(vk_instance, &physical_devices_count, physical_devices);
		if (vk_result) return _gmg_vulkan_convert_from_result(vk_result);

		_gmg.physical_devices = das_alloc_array(GmgPhysicalDevice, GMG_TODO_INTERNAL_ALLOCATOR, physical_devices_count);
		if (_gmg.physical_devices == NULL) return _gmg_stacktrace_add(GmgResultType_error_out_of_memory_host);
		das_zero_elmts(_gmg.physical_devices, physical_devices_count);
		for (uint32_t i = 0; i < physical_devices_count; i += 1) {
			GmgPhysicalDevice* d = &_gmg.physical_devices[i];
			d->vulkan.handle = physical_devices[i];
		}

		das_dealloc_array(VkPhysicalDevice, GMG_TODO_INTERNAL_ALLOCATOR, physical_devices, physical_devices_count);
	}

	return GmgResult_success;
}

static GmgResult _gmg_vulkan_backend_physical_device_features(GmgPhysicalDevice* physical_device, GmgPhysicalDeviceFeatureFlags* flags_out) {
	GmgResult result;
	VkResult vk_result;
	if (physical_device->vulkan.features == NULL) {
		physical_device->vulkan.features = das_alloc_elmt(VkPhysicalDeviceFeatures, GMG_TODO_INTERNAL_ALLOCATOR);
		if (physical_device->vulkan.features == NULL)
			return _gmg_stacktrace_add(GmgResultType_error_out_of_memory_host);
		vkGetPhysicalDeviceFeatures(physical_device->vulkan.handle, physical_device->vulkan.features);
	}

	GmgPhysicalDeviceFeatureFlags flags = 0;
	VkPhysicalDeviceFeatures* vk_features = physical_device->vulkan.features;
	for (GmgPhysicalDeviceFeature feature = 0; feature < GmgPhysicalDeviceFeature_COUNT; feature += 1) {
		uint32_t offset = _gmg_vulkan_convert_to_physical_device_features_field_offsets_plus_one[feature];
		if (offset == 0) {
			das_abort("VULKAN: unhandled feature %u\n", feature);
		}
		offset -= 1;

		if (*(VkBool32*)das_ptr_add(vk_features, offset)) {
			GmgPhysicalDeviceFeatureFlags_insert(flags, feature);
		}
	}

	*flags_out = flags;

	return GmgResult_success;
}

static GmgResult _gmg_vulkan_backend_physical_device_properties(GmgPhysicalDevice* physical_device, GmgPhysicalDeviceProperties* properties_out) {
	GmgResult result;
	VkResult vk_result;
	if (physical_device->vulkan.properties == NULL) {
		physical_device->vulkan.properties = das_alloc_elmt(VkPhysicalDeviceProperties, GMG_TODO_INTERNAL_ALLOCATOR);
		if (physical_device->vulkan.properties == NULL)
			return _gmg_stacktrace_add(GmgResultType_error_out_of_memory_host);
		vkGetPhysicalDeviceProperties(physical_device->vulkan.handle, physical_device->vulkan.properties);
	}

	VkPhysicalDeviceProperties* vk_props = physical_device->vulkan.properties;
	*properties_out = (GmgPhysicalDeviceProperties){
		.vulkan.api_version = vk_props->apiVersion,
		.driver_version = vk_props->driverVersion,
		.vendor_id = vk_props->vendorID,
		.device_id = vk_props->deviceID,
		.device_type = _gmg_vulkan_convert_from_physical_device_type[vk_props->deviceType],
		.device_name = vk_props->deviceName,
	};
	return GmgResult_success;

}
static GmgResult _gmg_vulkan_backend_physical_device_surface_texture_format(GmgPhysicalDevice* physical_device, GmgSurface surface, GmgTextureFormat* out) {
	GmgResult result;
	VkResult vk_result;

	if (physical_device->vulkan.surface_formats == NULL) {
		vk_result = vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device->vulkan.handle, surface.vulkan, &physical_device->vulkan.surface_formats_count, NULL);
		if (vk_result) return _gmg_vulkan_convert_from_result(vk_result);

		physical_device->vulkan.surface_formats = das_alloc_array(VkSurfaceFormatKHR, GMG_TODO_INTERNAL_ALLOCATOR, physical_device->vulkan.surface_formats_count);
		if (physical_device->vulkan.surface_formats == NULL) return _gmg_stacktrace_add(GmgResultType_error_out_of_memory_host);

		vk_result = vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device->vulkan.handle, surface.vulkan, &physical_device->vulkan.surface_formats_count, physical_device->vulkan.surface_formats);
		if (vk_result) return _gmg_vulkan_convert_from_result(vk_result);
	}
	VkSurfaceFormatKHR* surface_formats = physical_device->vulkan.surface_formats;
	uint32_t surface_formats_count = physical_device->vulkan.surface_formats_count;

	VkFormat image_format = VK_FORMAT_UNDEFINED;
	if (surface_formats_count == 1 && surface_formats[0].format == VK_FORMAT_UNDEFINED) {
		image_format = VK_FORMAT_B8G8R8A8_UNORM;
	} else {
		for (int i = 0; i < surface_formats_count; i += 1) {
			if (surface_formats[i].format == VK_FORMAT_B8G8R8A8_UNORM) {
				image_format = surface_formats[i].format;
				break;
			}
		}

		if (image_format == VK_FORMAT_UNDEFINED) {
			image_format = surface_formats[0].format;
		}
	}

	result = _gmg_vulkan_convert_from_format(image_format, out);
	if (result.type < 0) return result;

	return GmgResult_success;
}


static GmgResult _gmg_vulkan_backend_logical_device_init(GmgPhysicalDevice* physical_device, GmgLogicalDevice* logical_device, GmgLogicalDeviceCreateArgs* create_args) {
	GmgResult result;
	VkResult vk_result;
	if (physical_device->vulkan.queue_family_properties == NULL) {
		vkGetPhysicalDeviceQueueFamilyProperties(physical_device->vulkan.handle, &physical_device->vulkan.queue_families_count, NULL);
		physical_device->vulkan.queue_family_properties = das_alloc_array(
			VkQueueFamilyProperties, GMG_TODO_INTERNAL_ALLOCATOR, physical_device->vulkan.queue_families_count);
		if (physical_device->vulkan.queue_family_properties == NULL)
			return _gmg_stacktrace_add(GmgResultType_error_out_of_memory_host);

		vkGetPhysicalDeviceQueueFamilyProperties(physical_device->vulkan.handle, &physical_device->vulkan.queue_families_count, physical_device->vulkan.queue_family_properties);
	}


	VkQueueFamilyProperties* queue_family_properties = physical_device->vulkan.queue_family_properties;
	uint32_t queue_family_count = physical_device->vulkan.queue_families_count;

	uint32_t queue_type_to_score[_GmgVulkanQueueType_COUNT] = {0};

	VkQueueFlags vk_all_queue_flags = VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT | VK_QUEUE_TRANSFER_BIT;
	for (uint32_t family_idx = 0; family_idx < queue_family_count; family_idx += 1) {
		VkQueueFamilyProperties* props = &physical_device->vulkan.queue_family_properties[family_idx];
		VkQueueFlags vk_queue_flags = props->queueFlags & vk_all_queue_flags;
		VkQueueFlags flags = vk_queue_flags;
		uint32_t count = 0;
		while (flags) {
			if (flags & 1) count += 1;
			flags >>= 1;
		}

		if (count == 0)
			continue;

		for (_GmgVulkanQueueType queue_type = 0; queue_type < _GmgVulkanQueueType_primary_end; queue_type += 1) {
			uint32_t async_family_idx = UINT32_MAX;
			if (vk_queue_flags & _GmgVulkanQueueType_to_flags(queue_type)) {
				if (queue_type_to_score[queue_type] < count) {
					if (queue_type_to_score[queue_type]) {
						//
						// this queue_type already has been assigned another family index but
						// this new one shares with other queue types more.
						// so see if we can use the old family index for the asynchronous queue types.
						//
						uint32_t old_family_idx = logical_device->vulkan.queue_type_to_queue_family_idx[queue_type];
						async_family_idx = old_family_idx;
					}

					queue_type_to_score[queue_type] = count;
					logical_device->vulkan.queue_type_to_queue_family_idx[queue_type] = family_idx;
				} else {
					//
					// this queue_type shares with other queue types more in another family index.
					// so see if we can use this for the asynchronous queue types.
					//
					async_family_idx = family_idx;
				}


				if (async_family_idx != UINT32_MAX) {
					//
					// only assign if this queue_type to the asynchronous version if it shares more than it's existing one.
					if (queue_type == _GmgVulkanQueueType_compute && queue_type_to_score[_GmgVulkanQueueType_async_compute] < count) {
						queue_type_to_score[_GmgVulkanQueueType_async_compute] = count;
						logical_device->vulkan.queue_type_to_queue_family_idx[_GmgVulkanQueueType_async_compute] = async_family_idx;
					} else if (queue_type == _GmgVulkanQueueType_transfer && queue_type_to_score[_GmgVulkanQueueType_async_transfer] < count) {
						queue_type_to_score[_GmgVulkanQueueType_async_transfer] = count;
						logical_device->vulkan.queue_type_to_queue_family_idx[_GmgVulkanQueueType_async_transfer] = async_family_idx;
					}
				}
			}
		}
	}

	//
	// ensure that we have a queue to assign to each queue type as we at least need one for each type
	for (_GmgVulkanQueueType queue_type = 0; queue_type < _GmgVulkanQueueType_primary_end; queue_type += 1) {
		if (queue_type_to_score[queue_type] == 0) {
			return _gmg_stacktrace_add(GmgResultType_error_cannot_find_graphics_queue + queue_type);
		}
	}

	VkDeviceQueueCreateInfo vk_queue_create_infos[_GmgVulkanQueueType_COUNT];
	uint32_t vk_queue_create_infos_count = 0;
	for (uint32_t family_idx = 0; family_idx < queue_family_count; family_idx += 1) {
		for (_GmgVulkanQueueType queue_type = 0; queue_type < _GmgVulkanQueueType_primary_end; queue_type += 1) {
			if (
				queue_type_to_score[queue_type] &&
				logical_device->vulkan.queue_type_to_queue_family_idx[queue_type] == family_idx
			) {
				static float queue_priority = 0.f;
				vk_queue_create_infos[vk_queue_create_infos_count] = (VkDeviceQueueCreateInfo) {
					.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
					.pNext = NULL,
					.flags = 0,
					.queueFamilyIndex = family_idx,
					.queueCount = 1,
					.pQueuePriorities = &queue_priority,
				};
				vk_queue_create_infos_count += 1;
				break;
			}
		}
	}

	{
		VkPhysicalDeviceFeatures features;
		result = _gmg_vulkan_convert_to_physical_device_features(create_args->feature_flags, &features);
		if (result.type < 0) return result;

		static const char* extensions[] = {
			"VK_KHR_swapchain",
		};

		VkDeviceCreateInfo vk_create_info = {
			.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
			.pNext = NULL,
			.flags = 0,
			.queueCreateInfoCount = vk_queue_create_infos_count,
			.pQueueCreateInfos = vk_queue_create_infos,
			.enabledLayerCount = 0,
			.ppEnabledLayerNames = NULL,
			.enabledExtensionCount = sizeof(extensions) / sizeof(*extensions),
			.ppEnabledExtensionNames = (const char* const*)extensions,
			.pEnabledFeatures = &features,
		};

		vk_result = vkCreateDevice(physical_device->vulkan.handle, &vk_create_info, GMG_TODO_VULKAN_ALLOCATOR, &logical_device->vulkan.handle);
		if (vk_result) return _gmg_vulkan_convert_from_result(vk_result);
	}

	//
	// do this loop one more time and fetch the queues that were created by vulkan when creating a logical device.
	// store these for each queue type that uses the queue.
	//
	for (uint32_t family_idx = 0; family_idx < queue_family_count; family_idx += 1) {
		for (_GmgVulkanQueueType queue_type = 0; queue_type < _GmgVulkanQueueType_primary_end; queue_type += 1) {
			if (
				queue_type_to_score[queue_type] &&
				logical_device->vulkan.queue_type_to_queue_family_idx[queue_type] == family_idx
			) {
				vkGetDeviceQueue(logical_device->vulkan.handle, family_idx, 0, &logical_device->vulkan.queue_type_to_queue[queue_type]);
			}
		}
	}

	result = _gmg_vulkan_device_memory_allocator_init(logical_device, create_args->vulkan.allocator_min_block_size_device_local_memory, create_args->vulkan.allocator_min_block_size_host_visible_memory);
	if (result.type < 0) return result;

	result = _gmg_vulkan_memory_stager_init(logical_device, create_args->vulkan.memory_stager_buffer_size);
	if (result.type < 0) return result;

	VkCommandPoolCreateInfo vk_command_pool_create_info = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
		.pNext = NULL,
		.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
		.queueFamilyIndex = logical_device->vulkan.queue_type_to_queue_family_idx[_GmgVulkanQueueType_graphics],
	};

	vk_result = vkCreateCommandPool(
		logical_device->vulkan.handle, &vk_command_pool_create_info, GMG_TODO_VULKAN_ALLOCATOR,
		&logical_device->vulkan.graphics_command_pool);
	if (vk_result) return _gmg_vulkan_convert_from_result(vk_result);

	VkSemaphoreCreateInfo vk_semaphore_create_info = {0};
	vk_semaphore_create_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	vk_result = vkCreateSemaphore(
		logical_device->vulkan.handle, &vk_semaphore_create_info, GMG_TODO_VULKAN_ALLOCATOR, &logical_device->vulkan.semaphore_present);
	if (vk_result) return _gmg_vulkan_convert_from_result(vk_result);

	vk_result = vkCreateSemaphore(
		logical_device->vulkan.handle, &vk_semaphore_create_info, GMG_TODO_VULKAN_ALLOCATOR, &logical_device->vulkan.semaphore_render);
	if (vk_result) return _gmg_vulkan_convert_from_result(vk_result);

	VkDescriptorSetLayoutCreateInfo vk_set_layout_create_info = {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
		.pNext = NULL,
		.flags = 0,
		.bindingCount = 0,
		.pBindings = NULL,
	};

	vk_result = vkCreateDescriptorSetLayout(logical_device->vulkan.handle, &vk_set_layout_create_info, GMG_TODO_VULKAN_ALLOCATOR, &logical_device->vulkan.dummy_descriptor_set_layout);
	if (vk_result) return _gmg_vulkan_convert_from_result(vk_result);

	return GmgResult_success;
}
static GmgResult _gmg_vulkan_backend_logical_device_deinit(GmgLogicalDevice* logical_device) {
	GmgResult result;
	VkResult vk_result;
das_abort("unimplemented");
	return GmgResult_success;
}

static GmgResult _gmg_vulkan_backend_logical_device_submit(GmgLogicalDevice* logical_device, GmgRenderPassId* ordered_render_pass_ids, uint32_t render_passes_count) {
	GmgResult result;
	VkResult vk_result;
	DasAlctor temp_alctor = gmg_logical_device_temp_alctor(logical_device);

	uint32_t command_buffers_count = render_passes_count;

	for (uint32_t i = 0; i < render_passes_count; i += 1) {
		GmgRenderPass* render_pass;
		result = gmg_render_pass_get(logical_device, ordered_render_pass_ids[i], &render_pass);
		if (result.type < 0) return result;

		switch (_gmg.backend_type) {
#ifdef GMG_ENABLE_VULKAN
			case GmgBackendType_vulkan: {
				_GmgVkDescriptorSet* set = &render_pass->vulkan.pass_descriptor_set;
				for (uint32_t i = 0; i < set->descriptors_count; i += 1) {
					if (set->descriptors[i].object_id.raw == 0)
						return _gmg_stacktrace_add(GmgResultType_error_render_pass_has_unset_descriptors);
				}
				break;
			};
#endif
		}
	}


	//
	// add any stage buffers that need their memory flushed to the internal flush list.
	//
	_GmgVulkanMemoryStager* stager = &logical_device->vulkan.memory_stager;
	GmgBool has_transfer_job = gmg_false;
	if (DasStk_count(&stager->buffers)) {
		for (uint32_t i = 0; i < stager->filled_buffers_count + 1; i += 1) {
			_GmgVulkanStagingBuffer* buffer = DasStk_get(&stager->buffers, i);
			if (i == stager->filled_buffers_count && buffer->position == 0) {
				break;
			}
			if (buffer->needs_flush) {
				result = _gmg_vulkan_device_memory_allocator_flush(logical_device, buffer->core.device_memory_alloc_id, buffer->core.memory_mapped_data, 0, buffer->position);
				if (result.type < 0) return result;
			}
		}
		has_transfer_job = gmg_true;
		command_buffers_count += 1;
	}

	//
	// flush all the memory updates that have occured since the last submit
	//
	uint32_t flushes_count = DasStk_count(&logical_device->vulkan.flush_memory_ranges);
	if (flushes_count) {
		VkMappedMemoryRange* vk_ranges = das_alloc_array(VkMappedMemoryRange, temp_alctor, flushes_count);
		if (vk_ranges == NULL) return _gmg_stacktrace_add(GmgResultType_error_out_of_memory_host);

		DasStk_foreach(&logical_device->vulkan.flush_memory_ranges, i) {
			_GmgVulkanFlushMemoryRange* range = DasStk_get(&logical_device->vulkan.flush_memory_ranges, i);
			vk_ranges[i] = (VkMappedMemoryRange) {
				.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE,
				.pNext = NULL,
				.memory = range->memory,
				.offset = range->offset,
				.size = range->size,
			};
		}

		vk_result = vkFlushMappedMemoryRanges(
			logical_device->vulkan.handle, DasStk_count(&logical_device->vulkan.flush_memory_ranges), vk_ranges);
		if (vk_result) return _gmg_vulkan_convert_from_result(vk_result);
	}

	//
	// recycle any vulkan resources (command buffers, descriptor sets, and fences) from previous
	// submissions that have been completed by the GPU
	//
	while (DasDeque_count(&logical_device->vulkan.graphics_submits)) {
		_GmgVulkanSubmit* submit = DasDeque_get_first(&logical_device->vulkan.graphics_submits);
		vk_result = vkGetFenceStatus(logical_device->vulkan.handle, submit->fence);
		if (vk_result == VK_SUCCESS) {
			if (!DasStk_push_many(&logical_device->vulkan.graphics_command_buffers_available, DasStk_data(&submit->command_buffers), DasStk_count(&submit->command_buffers))) {
				return _gmg_stacktrace_add(GmgResultType_error_out_of_memory_host);
			}

			DasStk_foreach(&submit->descriptor_sets_to_be_deallocated, j) {
				_GmgVulkanDescriptorSetToBeDeallocated* entry = DasStk_get(&submit->descriptor_sets_to_be_deallocated, j);
				result = _gmg_vulkan_descriptor_set_allocator_dealloc(logical_device, entry);
				if (result.type < 0) return result;
			}

			DasStk_foreach(&submit->descriptor_pools_deallocate, j) {
				vkDestroyDescriptorPool(logical_device->vulkan.handle, *DasStk_get(&submit->descriptor_pools_deallocate, j), GMG_TODO_VULKAN_ALLOCATOR);
			}

			if (!DasStk_push_many(&logical_device->vulkan.staging_buffers_available, DasStk_data(&submit->staging_buffers), DasStk_count(&submit->staging_buffers))) {
				return _gmg_stacktrace_add(GmgResultType_error_out_of_memory_host);
			}

			DasStk_clear(&submit->command_buffers);
			DasStk_clear(&submit->descriptor_sets_to_be_deallocated);
			DasStk_clear(&submit->descriptor_pools_deallocate);
			DasStk_clear(&submit->staging_buffers);

			if (!DasStk_push(&logical_device->vulkan.graphics_submits_available, submit))
				return _gmg_stacktrace_add(GmgResultType_error_out_of_memory_host);

			DasDeque_pop_front(&logical_device->vulkan.graphics_submits);
		} else if (vk_result != VK_NOT_READY) {
			return _gmg_vulkan_convert_from_result(vk_result);
		} else {
			break;
		}
	}

	//
	// get the submission structure to keep track of the data
	// that can be recycled when the submission has been completed by the GPU.
	//
	_GmgVulkanSubmit* submit;
	if (DasStk_count(&logical_device->vulkan.graphics_submits_available)) {
		//
		// reuse an old structure.
		//
		submit = DasStk_get_last(&logical_device->vulkan.graphics_submits_available);
		if (DasDeque_push_back(&logical_device->vulkan.graphics_submits, submit) == UINTPTR_MAX)
			return _gmg_stacktrace_add(GmgResultType_error_out_of_memory_host);
		DasStk_pop(&logical_device->vulkan.graphics_submits_available);
		submit = DasDeque_get_last(&logical_device->vulkan.graphics_submits);

		vk_result = vkResetFences(logical_device->vulkan.handle, 1, &submit->fence);
		if (vk_result) return _gmg_vulkan_convert_from_result(vk_result);
	} else {
		//
		// we have run out of structures to use so make a new one.
		//
		if (DasDeque_push_back(&logical_device->vulkan.graphics_submits, NULL) == UINTPTR_MAX)
			return _gmg_stacktrace_add(GmgResultType_error_out_of_memory_host);
		submit = DasDeque_get_last(&logical_device->vulkan.graphics_submits);
		das_zero_elmt(submit);

		VkFenceCreateInfo fence_create_info = {
			.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
			.pNext = NULL,
			.flags = 0,
		};

		VkResult vk_result = vkCreateFence(logical_device->vulkan.handle, &fence_create_info, GMG_TODO_VULKAN_ALLOCATOR, &submit->fence);
		if (vk_result) return _gmg_vulkan_convert_from_result(vk_result);
	}

	//
	// if we need more command buffers for our submit, allocate more
	//
	uint32_t available_command_buffers_count = DasStk_count(&logical_device->vulkan.graphics_command_buffers_available);
	if (available_command_buffers_count < command_buffers_count) {
		uint32_t create_count = command_buffers_count - available_command_buffers_count;
		VkCommandBuffer* insert_ptr = DasStk_push_many(&logical_device->vulkan.graphics_command_buffers_available, NULL, create_count);
		if (insert_ptr == NULL) return _gmg_stacktrace_add(GmgResultType_error_out_of_memory_host);

		VkCommandBufferAllocateInfo vk_command_buffer_alloc_info = {
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
			.pNext = NULL,
			.commandPool = logical_device->vulkan.graphics_command_pool,
			.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
			.commandBufferCount = create_count,
		};

		vk_result = vkAllocateCommandBuffers(logical_device->vulkan.handle, &vk_command_buffer_alloc_info, insert_ptr);
		if (vk_result) return _gmg_vulkan_convert_from_result(vk_result);
	}

	uint32_t command_buffer_idx = DasStk_count(&logical_device->vulkan.graphics_command_buffers_available) - command_buffers_count;

	//
	// if we have any stage data that needs coping to the GPU,
	// then build a transfer command buffer by dispatching a job.
	//
	GmgCallbackArgs callback_args;
	_GmgVulkanCommandBufferBuildTransfer transfer_build;
	if (has_transfer_job) {
		transfer_build = (_GmgVulkanCommandBufferBuildTransfer) {
			.logical_device = logical_device,
			.command_buffer = *DasStk_get(&logical_device->vulkan.graphics_command_buffers_available, command_buffer_idx),
			.used_staging_buffers = &submit->staging_buffers,
		};

		command_buffer_idx += 1;

		callback_args.execute_job.start_fn = _gmg_vulkan_command_buffer_build_transfer_job;
		callback_args.execute_job.args = &transfer_build;
		_gmg.callback_impl_fn(logical_device, GmgCallbackFn_execute_job, &callback_args, _gmg.callback_userdata);
	}

	uint32_t graphics_build_states_count = DasStk_count(&logical_device->vulkan.graphics_build_states);
	if (graphics_build_states_count < render_passes_count) {
		if (!DasStk_resize(&logical_device->vulkan.graphics_build_states, render_passes_count, das_true))
			return _gmg_stacktrace_add(GmgResultType_error_out_of_memory_host);

		for (uint32_t i = graphics_build_states_count; i < render_passes_count; i += 1) {
			_GmgVulkanCommandBufferBuildGraphics* build = DasStk_get(&logical_device->vulkan.graphics_build_states, i);
			DasError das_error = DasLinearAlctor_init(&build->temp_alctor, gmg_graphics_build_allocator_reserve_size, gmg_graphics_build_allocator_grow_size);
			if (das_error) {
				return GmgResult_from_DasError(das_error);
			}

			VkSemaphoreCreateInfo vk_semaphore_create_info = {0};
			vk_semaphore_create_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
		}
	}

	//
	// build a command buffer for each render pass by dispatching a job for each of them
	//
	for (uint32_t i = 0; i < render_passes_count; i += 1) {
		_GmgVulkanCommandBufferBuildGraphics* build = DasStk_get(&logical_device->vulkan.graphics_build_states, i);

		build->logical_device = logical_device;
		build->command_buffer = *DasStk_get(&logical_device->vulkan.graphics_command_buffers_available, command_buffer_idx);
		build->latest_fence = submit->fence;
		result = gmg_render_pass_get(logical_device, ordered_render_pass_ids[i], &build->render_pass);
		if (result.type < 0) return result;

		command_buffer_idx += 1;

		callback_args.execute_job.start_fn = _gmg_vulkan_command_buffer_build_graphics_job;
		callback_args.execute_job.args = build;
		_gmg.callback_impl_fn(logical_device, GmgCallbackFn_execute_job, &callback_args, _gmg.callback_userdata);
	}

	//
	// wait for the command buffers to be built
	//
	_gmg.callback_impl_fn(logical_device, GmgCallbackFn_wait_for_jobs, NULL, _gmg.callback_userdata);

	//
	// make sure all of the jobs completed successfully
	//
	DasStk_foreach(&logical_device->job_results, i) {
		GmgResult result = *DasStk_get(&logical_device->job_results, i);
		if (result.type < 0) return result;
	}

	DasStk_clear(&logical_device->job_results);

	//
	// each of the jobs that build render passes can mark a descriptor set for deallocation.
	// use the fence in the entry to find the list for the submission the descriptor set was last used in.
	// merge it into the submission list so when the submission is completed. we can deallocate all the descriptor sets.
	//
	for (uint32_t i = 0; i < render_passes_count; i += 1) {
		_GmgVulkanCommandBufferBuildGraphics* build = DasStk_get(&logical_device->vulkan.graphics_build_states, i);
		DasStk_foreach(&build->descriptor_sets_to_be_deallocated, j) {
			_GmgVulkanDescriptorSetToBeDeallocatedEntry* entry = DasStk_get(&build->descriptor_sets_to_be_deallocated, j);
			DasDeque_foreach(&logical_device->vulkan.graphics_submits, k) {
				_GmgVulkanSubmit* submit = DasDeque_get(&logical_device->vulkan.graphics_submits, k);
				GmgBool found = gmg_false;
				if (submit->fence == entry->latest_used_fence) {
					if (!DasStk_push(&submit->descriptor_sets_to_be_deallocated, &entry->inner)) {
						return _gmg_stacktrace_add(GmgResultType_error_out_of_memory_host);
					}

					found = gmg_true;
					break;
				}

				if (!found) {
					//
					// the submission the fence belongs to has already been freed
					//
					result = _gmg_vulkan_descriptor_set_allocator_dealloc(logical_device, &entry->inner);
					if (result.type < 0) return result;
				}
			}
		}
	}

	//
	//
	// submit the work to vulkan
	//
	//

	VkPipelineStageFlags wait_flags = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	VkCommandBuffer* queued_command_buffers = DasStk_get_back(&logical_device->vulkan.graphics_command_buffers_available, command_buffers_count - 1);
	VkSubmitInfo vk_submit_info = {
		.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
		.pNext = NULL,
		.waitSemaphoreCount = 1,
		.pWaitSemaphores = &logical_device->vulkan.semaphore_present,
		.pWaitDstStageMask = &wait_flags,
		.commandBufferCount = command_buffers_count,
		.pCommandBuffers = queued_command_buffers,
		.signalSemaphoreCount = 1,
		.pSignalSemaphores = &logical_device->vulkan.semaphore_render,
	};

	vk_result = vkQueueSubmit(logical_device->vulkan.queue_type_to_queue[_GmgVulkanQueueType_graphics], 1, &vk_submit_info, submit->fence);
	if (vk_result) return _gmg_vulkan_convert_from_result(vk_result);

	if (!DasStk_push_many(&submit->command_buffers, queued_command_buffers, command_buffers_count)) {
		return _gmg_stacktrace_add(GmgResultType_error_out_of_memory_host);
	}

	DasStk_pop_many(&logical_device->vulkan.graphics_command_buffers_available, command_buffers_count);

	return GmgResult_success;
}

static GmgResult _gmg_vulkan_backend_sampler_init(GmgSampler* sampler, GmgLogicalDevice* logical_device, GmgSamplerCreateArgs* create_args) {
	GmgResult result;
	VkResult vk_result;
	VkSamplerCreateInfo vk_create_info = {
		.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
		.pNext = NULL,
		.flags = 0,
		.magFilter = _gmg_vulkan_convert_to_filter[create_args->mag_filter],
		.minFilter = _gmg_vulkan_convert_to_filter[create_args->min_filter],
		.mipmapMode = _gmg_vulkan_convert_to_sampler_mipmap_mode[create_args->mipmap_mode],
		.addressModeU = _gmg_vulkan_convert_to_sampler_address_mode[create_args->address_mode_u],
		.addressModeV = _gmg_vulkan_convert_to_sampler_address_mode[create_args->address_mode_v],
		.addressModeW = _gmg_vulkan_convert_to_sampler_address_mode[create_args->address_mode_w],
		.mipLodBias = create_args->mip_lod_bias,
		.anisotropyEnable = (create_args->flags & GmgSamplerCreateFlags_anisotropy_enable) != 0,
		.maxAnisotropy = create_args->max_anisotropy,
		.compareEnable = (create_args->flags & GmgSamplerCreateFlags_compare_enable) != 0,
		.compareOp = _gmg_vulkan_convert_to_compare_op[create_args->compare_op],
		.minLod = create_args->min_lod,
		.maxLod = create_args->max_lod,
		.borderColor = _gmg_vulkan_convert_to_border_color[create_args->border_color],
		.unnormalizedCoordinates = (create_args->flags & GmgSamplerCreateFlags_unnormalized_coordinates) != 0,
	};

	vk_result = vkCreateSampler(logical_device->vulkan.handle, &vk_create_info, GMG_TODO_VULKAN_ALLOCATOR, &sampler->vulkan.handle);
	if (vk_result) return _gmg_vulkan_convert_from_result(vk_result);

	return GmgResult_success;
}
static GmgResult _gmg_vulkan_backend_sampler_deinit(GmgSampler* sampler, GmgLogicalDevice* logical_device) {
	GmgResult result;
	VkResult vk_result;
	vkDestroySampler(logical_device->vulkan.handle, sampler->vulkan.handle, GMG_TODO_VULKAN_ALLOCATOR);
	return GmgResult_success;
}


static GmgResult _gmg_vulkan_backend_texture_reinit(GmgTexture* texture, GmgLogicalDevice* logical_device) {
	GmgResult result;
	VkResult vk_result;
	if (texture->vulkan.image != VK_NULL_HANDLE) {
		result = _gmg_vulkan_device_memory_dealloc(logical_device, texture->vulkan.device_memory_alloc_id);
		if (result.type < 0) return result;

		vkDestroyImageView(logical_device->vulkan.handle, texture->vulkan.image_view, GMG_TODO_VULKAN_ALLOCATOR);
		vkDestroyImage(logical_device->vulkan.handle, texture->vulkan.image, GMG_TODO_VULKAN_ALLOCATOR);
	}
	GmgTextureFlags flags = texture->flags;

	VkImageCreateFlags vk_create_flags = 0;
	if (texture->type == GmgTextureType_cube || texture->type == GmgTextureType_cube_array) {
		vk_create_flags |= VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
	}

	VkImageUsageFlags vk_usage_flags = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
	if (flags & GmgTextureFlags_sampled) vk_usage_flags |= VK_IMAGE_USAGE_SAMPLED_BIT;
	if (flags & GmgTextureFlags_color_attachment) vk_usage_flags |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	if (flags & GmgTextureFlags_depth_stencil_attachment) vk_usage_flags |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
	if (flags & GmgTextureFlags_input_attachment) vk_usage_flags |= VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT;

	VkQueueFlags queue_flags = VK_QUEUE_TRANSFER_BIT;
	if (flags & GmgTextureFlags_used_for_graphics) queue_flags |= VK_QUEUE_GRAPHICS_BIT;
	if (flags & GmgTextureFlags_used_for_compute) queue_flags |= VK_QUEUE_COMPUTE_BIT;

	uint32_t vk_queue_family_indices[5];
	uint16_t vk_queue_family_indices_count;
	result = _GmgLogicalDevice_vulkan_queue_family_indices_get(
		logical_device, queue_flags, vk_queue_family_indices, &vk_queue_family_indices_count);
	if (result.type < 0) return result;

	VkSharingMode sharing_mode;
	if (vk_queue_family_indices_count > 1) {
		sharing_mode = VK_SHARING_MODE_CONCURRENT;
	} else {
		sharing_mode = VK_SHARING_MODE_EXCLUSIVE;
		vk_queue_family_indices_count = 0;
	}

	VkImageCreateInfo vk_image_create_info = {
		.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
		.pNext = NULL,
		.flags = vk_create_flags,
		.imageType = _gmg_vulkan_convert_to_image_type[texture->type],
		.format = _gmg_vulkan_convert_to_format[texture->format],
		.extent = (VkExtent3D) { texture->width, texture->height, texture->depth },
		.mipLevels = texture->mip_levels,
		.arrayLayers = texture->array_layers_count,
		.samples = texture->samples,
		.tiling = VK_IMAGE_TILING_OPTIMAL,
		.usage = vk_usage_flags,
		.sharingMode = sharing_mode,
		.queueFamilyIndexCount = vk_queue_family_indices_count,
		.pQueueFamilyIndices = vk_queue_family_indices,
		.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
	};

	vk_result = vkCreateImage(logical_device->vulkan.handle, &vk_image_create_info, GMG_TODO_VULKAN_ALLOCATOR, &texture->vulkan.image);
	if (vk_result) return _gmg_vulkan_convert_from_result(vk_result);

	VkImageAspectFlags aspect_mask = 0;
	switch (texture->format) {
		case GmgTextureFormat_s8:
			aspect_mask = VK_IMAGE_ASPECT_STENCIL_BIT;
			break;
		case GmgTextureFormat_d16_s8:
		case GmgTextureFormat_d24_s8:
		case GmgTextureFormat_d32_s8:
			aspect_mask |= VK_IMAGE_ASPECT_STENCIL_BIT;
		case GmgTextureFormat_d16:
		case GmgTextureFormat_d32:
			aspect_mask |= VK_IMAGE_ASPECT_DEPTH_BIT;
			break;
		default:
			aspect_mask = VK_IMAGE_ASPECT_COLOR_BIT;
			break;
	}

	VkImageViewCreateInfo vk_image_view_create_info = {
		.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
		.pNext = NULL,
		.flags = 0,
		.image = texture->vulkan.image,
		.viewType = _gmg_vulkan_convert_to_image_view_type[texture->type],
		.format = _gmg_vulkan_convert_to_format[texture->format],
		.components = {
			.r = VK_COMPONENT_SWIZZLE_R,
			.g = VK_COMPONENT_SWIZZLE_G,
			.b = VK_COMPONENT_SWIZZLE_B,
			.a = VK_COMPONENT_SWIZZLE_A,
		},
		.subresourceRange = {
			.aspectMask = aspect_mask,
			.baseMipLevel = 0,
			.levelCount = texture->mip_levels,
			.baseArrayLayer = 0,
			.layerCount = texture->array_layers_count,
		},
	};

	VkMemoryRequirements mem_req;
	vkGetImageMemoryRequirements(logical_device->vulkan.handle, texture->vulkan.image, &mem_req);

	_GmgVulkanAllocResult alloc_result;
	_GmgVulkanAllocArgs args = {
		.size = mem_req.size,
		.align = mem_req.alignment,
		.memory_type_bits = mem_req.memoryTypeBits,
		.memory_location = _GmgVulkanMemoryLocation_gpu,
		.type = _GmgVulkanAllocType_buffer,
	};
	result = _gmg_vulkan_device_memory_alloc(logical_device, &args, &alloc_result, NULL);
	if (result.type < 0) return result;

	texture->vulkan.device_memory_alloc_id = alloc_result.id;

	vk_result = vkBindImageMemory(logical_device->vulkan.handle, texture->vulkan.image, alloc_result.device_memory, alloc_result.device_memory_offset);
	if (vk_result) return _gmg_vulkan_convert_from_result(vk_result);

	vk_result = vkCreateImageView(logical_device->vulkan.handle, &vk_image_view_create_info, GMG_TODO_VULKAN_ALLOCATOR, &texture->vulkan.image_view);
	if (vk_result) return _gmg_vulkan_convert_from_result(vk_result);

	return GmgResult_success;

}
static GmgResult _gmg_vulkan_backend_texture_deinit(GmgTexture* texture, GmgLogicalDevice* logical_device) {
	GmgResult result;
	VkResult vk_result;
	vkDestroyImageView(logical_device->vulkan.handle, texture->vulkan.image_view, GMG_TODO_VULKAN_ALLOCATOR);
	vkDestroyImage(logical_device->vulkan.handle, texture->vulkan.image, GMG_TODO_VULKAN_ALLOCATOR);
	return GmgResult_success;
}
static GmgResult _gmg_vulkan_backend_texture_get_write_buffer(GmgTexture* texture, GmgLogicalDevice* logical_device, GmgTextureArea* area, void** data_out) {
	GmgResult result;
	VkResult vk_result;
	return _gmg_vulkan_memory_stager_stage_texture(logical_device, texture->format, texture->vulkan.image, area, data_out);
}

static GmgResult _gmg_vulkan_backend_descriptor_set_layout_init(GmgDescriptorSetLayout* layout, GmgLogicalDevice* logical_device, GmgDescriptorSetLayoutCreateArgs* create_args) {
	GmgResult result;
	VkResult vk_result;

	DasAlctor temp_alctor = gmg_logical_device_temp_alctor(logical_device);
	VkDescriptorSetLayoutBinding* vk_bindings = das_alloc_array(VkDescriptorSetLayoutBinding, temp_alctor, create_args->descriptor_bindings_count);
	if (!vk_bindings) return _gmg_stacktrace_add(GmgResultType_error_out_of_memory_host);

	for (uint32_t i = 0; i < create_args->descriptor_bindings_count; i += 1) {
		GmgDescriptorBinding* binding = &create_args->descriptor_bindings[i];
		VkDescriptorSetLayoutBinding* vk_binding = &vk_bindings[i];
		vk_binding->binding = binding->binding_idx;
		vk_binding->descriptorType = _gmg_vulkan_convert_to_descriptor_type[binding->type];
		vk_binding->descriptorCount = binding->count;
		vk_binding->pImmutableSamplers = NULL;
		result = _gmg_vulkan_convert_to_shader_stage_flags(binding->shader_stage_flags, &vk_binding->stageFlags);
		if (result.type < 0) return result;

		layout->vulkan.descriptor_sizes[binding->type] += binding->count;
	}

	VkDescriptorSetLayoutCreateInfo vk_set_layout_create_info = {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
		.pNext = NULL,
		.flags = 0,
		.bindingCount = create_args->descriptor_bindings_count,
		.pBindings = vk_bindings,
	};

	vk_result = vkCreateDescriptorSetLayout(logical_device->vulkan.handle, &vk_set_layout_create_info, GMG_TODO_VULKAN_ALLOCATOR, &layout->vulkan.handle);
	if (vk_result) return _gmg_vulkan_convert_from_result(vk_result);

	return GmgResult_success;
}

static GmgResult _gmg_vulkan_backend_descriptor_set_layout_deinit(GmgDescriptorSetLayout* layout, GmgLogicalDevice* logical_device) {
	GmgResult result;
	VkResult vk_result;
	vkDestroyDescriptorSetLayout(logical_device->vulkan.handle, layout->vulkan.handle, GMG_TODO_VULKAN_ALLOCATOR);
	return GmgResult_success;
}

static GmgResult _gmg_vulkan_backend_descriptor_set_allocator_reset(GmgDescriptorSetAllocator* allocator, GmgLogicalDevice* logical_device) {
	_GmgVulkanSubmit* submit = DasDeque_get_last(&logical_device->vulkan.graphics_submits);
	for (uint32_t i = 0; i < allocator->layouts_count; i += 1) {
		if (!DasStk_push(&submit->descriptor_pools_deallocate, &DasStk_get(&allocator->vulkan.pools, i)->handle)) {
			return _gmg_stacktrace_add(GmgResultType_error_out_of_memory_host);
		}
	}
	DasStk_clear(&allocator->vulkan.pools);
	allocator->vulkan.reset_counter += 1;

	return GmgResult_success;
}

static GmgResult _gmg_vulkan_backend_descriptor_set_allocator_init(GmgDescriptorSetAllocator* allocator, GmgLogicalDevice* logical_device, GmgDescriptorSetAllocatorCreateArgs* create_args) {
	_GmgVkDescriptorSetAllocatorLayoutAux* layout_auxs = das_alloc_array(_GmgVkDescriptorSetAllocatorLayoutAux, GMG_TODO_INTERNAL_ALLOCATOR, create_args->layouts_count);
	if (layout_auxs == NULL) return _gmg_stacktrace_add(GmgResultType_error_out_of_memory_host);
	das_zero_elmts(layout_auxs, create_args->layouts_count);

	GmgDescriptorSetLayoutId* layout_ids = das_alloc_array(GmgDescriptorSetLayoutId, GMG_TODO_INTERNAL_ALLOCATOR, create_args->layouts_count);
	if (layout_ids == NULL) return _gmg_stacktrace_add(GmgResultType_error_out_of_memory_host);

	uint32_t pool_max_sets = 0;

	uint32_t type_sizes[GmgDescriptorType_COUNT] = {0};
	uint32_t vk_pool_sizes_count = 0;

	for (uint32_t i = 0; i < create_args->layouts_count; i += 1) {
		GmgDescriptorSetLayoutAllocatorCreateArgs* layout_create_args = &create_args->layouts[i];
		layout_auxs[i].cap_per_pool = layout_create_args->cap_per_pool;
		layout_ids[i] = layout_create_args->id;
		pool_max_sets += layout_create_args->cap_per_pool;

		GmgDescriptorSetLayout* layout;
		GmgResult result = gmg_descriptor_set_layout_get(logical_device, layout_create_args->id, &layout);
		if (result.type < 0) return result;

		for (uint32_t type = 0; type < GmgDescriptorType_COUNT; type += 1) {
			uint32_t size = layout->vulkan.descriptor_sizes[type];
			if (size) {
				if (type_sizes[type] == 0) {
					vk_pool_sizes_count += 1;
				}
				type_sizes[type] += size * layout_create_args->cap_per_pool;
			}
		}
	}

	VkDescriptorPoolSize* vk_pool_sizes = das_alloc_array(VkDescriptorPoolSize, GMG_TODO_INTERNAL_ALLOCATOR, vk_pool_sizes_count);
	for (uint32_t i = 0, j = 0; i < GmgDescriptorType_COUNT; i += 1) {
		uint32_t count = type_sizes[i];
		if (count) {
			vk_pool_sizes[j].type = _gmg_vulkan_convert_to_descriptor_type[i];
			vk_pool_sizes[j].descriptorCount = count;
			j += 1;
		}
	}

	allocator->layout_ids = layout_ids;
	allocator->layouts_count = create_args->layouts_count;
	allocator->vulkan.layout_auxs = layout_auxs;
	allocator->vulkan.pool_sizes = vk_pool_sizes;
	allocator->vulkan.pool_sizes_count = vk_pool_sizes_count;
	allocator->vulkan.pool_max_sets = pool_max_sets;

	return GmgResult_success;
}

static GmgResult _gmg_vulkan_backend_descriptor_set_allocator_deinit(GmgDescriptorSetAllocator* allocator, GmgLogicalDevice* logical_device) {
	return _gmg_vulkan_backend_descriptor_set_allocator_reset(allocator, logical_device);
}

static GmgResult _gmg_vulkan_backend_shader_module_init(GmgShaderModule* shader_module, GmgLogicalDevice* logical_device, GmgShaderModuleCreateArgs* create_args) {
	GmgResult result;
	VkResult vk_result;
	if (create_args->format != GmgShaderFormat_spir_v) {
		return _gmg_stacktrace_add(GmgResultType_error_incompatible_shader_format);
	}

	VkShaderModuleCreateInfo vk_create_info = {
		.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
		.pNext = NULL,
		.flags = 0,
		.codeSize = create_args->code_size,
		.pCode = (uint32_t*)create_args->code,
	};

	vk_result = vkCreateShaderModule(logical_device->vulkan.handle, &vk_create_info, GMG_TODO_VULKAN_ALLOCATOR, &shader_module->vulkan.handle);
	if (vk_result) return _gmg_vulkan_convert_from_result(vk_result);
	return GmgResult_success;
}

static GmgResult _gmg_vulkan_backend_shader_module_deinit(GmgShaderModule* shader_module, GmgLogicalDevice* logical_device) {
	GmgResult result;
	VkResult vk_result;
	vkDestroyShaderModule(logical_device->vulkan.handle, shader_module->vulkan.handle, GMG_TODO_VULKAN_ALLOCATOR);
	return GmgResult_success;
}

static GmgResult _gmg_vulkan_backend_shader_init(GmgShader* shader, GmgLogicalDevice* logical_device, GmgShaderCreateArgs* create_args) {
	GmgResult result;
	VkResult vk_result;
	uint8_t stages_count = GmgShaderType_stages_counts[create_args->type];
	for (uint32_t i = 0; i < stages_count; i += 1) {
		GmgShaderFormat format = create_args->stages.array[i].format;
		if (format != GmgShaderFormat_none && format != GmgShaderFormat_spir_v) {
			return _gmg_stacktrace_add(GmgResultType_error_incompatible_shader_format);
		}
	}

	VkPushConstantRange vk_push_constant_range = {
		.offset = 0,
		.size = create_args->push_constants_size,
	};

	result = _gmg_vulkan_convert_to_shader_stage_flags(create_args->push_constants_shader_stage_flags, &vk_push_constant_range.stageFlags);
	if (result.type < 0) return result;

	VkDescriptorSetLayout vk_layouts[3];
	uint32_t vk_layouts_count = 0;
	GmgDescriptorSetLayout* layout;

	if (create_args->pass_descriptor_set_layout_id.raw) {
		result = gmg_descriptor_set_layout_get(logical_device, create_args->pass_descriptor_set_layout_id, &layout);
		if (result.type < 0) return result;

		vk_layouts[vk_layouts_count] = layout->vulkan.handle;
	} else {
		vk_layouts[vk_layouts_count] = logical_device->vulkan.dummy_descriptor_set_layout;
	}
	vk_layouts_count += 1;

	if (create_args->material_descriptor_set_layout_id.raw) {
		result = gmg_descriptor_set_layout_get(logical_device, create_args->material_descriptor_set_layout_id, &layout);
		if (result.type < 0) return result;

		vk_layouts[vk_layouts_count] = layout->vulkan.handle;
	} else {
		vk_layouts[vk_layouts_count] = logical_device->vulkan.dummy_descriptor_set_layout;
	}
	vk_layouts_count += 1;

	if (create_args->draw_cmd_descriptor_set_layout_id.raw) {
		result = gmg_descriptor_set_layout_get(logical_device, create_args->draw_cmd_descriptor_set_layout_id, &layout);
		if (result.type < 0) return result;

		vk_layouts[vk_layouts_count] = layout->vulkan.handle;
	} else {
		vk_layouts[vk_layouts_count] = logical_device->vulkan.dummy_descriptor_set_layout;
	}
	vk_layouts_count += 1;
	while (vk_layouts_count && vk_layouts[vk_layouts_count - 1] == logical_device->vulkan.dummy_descriptor_set_layout) {
		vk_layouts_count -= 1;
	}

	VkPipelineLayoutCreateInfo vk_create_info = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
		.pNext = NULL,
		.flags = 0,
		.setLayoutCount = vk_layouts_count,
		.pSetLayouts = vk_layouts,
		.pushConstantRangeCount = create_args->push_constants_size ? 1 : 0,
		.pPushConstantRanges = create_args->push_constants_size ? &vk_push_constant_range : NULL,
	};

	vk_result = vkCreatePipelineLayout(logical_device->vulkan.handle, &vk_create_info, GMG_TODO_VULKAN_ALLOCATOR, &shader->vulkan.pipeline_layout);
	if (vk_result) return _gmg_vulkan_convert_from_result(vk_result);

	return GmgResult_success;
}
static GmgResult _gmg_vulkan_backend_shader_deinit(GmgShader* shader, GmgLogicalDevice* logical_device) {
	GmgResult result;
	VkResult vk_result;
	vkDestroyPipelineLayout(logical_device->vulkan.handle, shader->vulkan.pipeline_layout, GMG_TODO_VULKAN_ALLOCATOR);
	return GmgResult_success;
}

static GmgResult _gmg_vulkan_backend_material_spec_cache_init(GmgMaterialSpecCache* material_spec_cache, GmgLogicalDevice* logical_device, GmgMaterialSpecCacheCreateArgs* create_args) {
	GmgResult result;
	VkResult vk_result;
	VkPipelineCacheCreateInfo vk_create_info = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO,
		.pNext = NULL,
		.flags = 0,
		.initialDataSize = create_args->size,
		.pInitialData = create_args->data,
	};

	vk_result = vkCreatePipelineCache(logical_device->vulkan.handle, &vk_create_info, GMG_TODO_VULKAN_ALLOCATOR, &material_spec_cache->vulkan.handle);
	if (vk_result) return _gmg_vulkan_convert_from_result(vk_result);

	return GmgResult_success;
}
static GmgResult _gmg_vulkan_backend_material_spec_cache_deinit(GmgMaterialSpecCache* material_spec_cache, GmgLogicalDevice* logical_device) {
	GmgResult result;
	VkResult vk_result;
	vkDestroyPipelineCache(logical_device->vulkan.handle, material_spec_cache->vulkan.handle, GMG_TODO_VULKAN_ALLOCATOR);

	return GmgResult_success;
}
static GmgResult _gmg_vulkan_backend_material_spec_cache_get_data(GmgMaterialSpecCache* material_spec_cache, GmgLogicalDevice* logical_device, void** data_out, uintptr_t* size_out) {
	GmgResult result;
	VkResult vk_result;
	vk_result = vkGetPipelineCacheData(logical_device->vulkan.handle, material_spec_cache->vulkan.handle, size_out, data_out);
	if (vk_result) return _gmg_vulkan_convert_from_result(vk_result);
	return GmgResult_success;
}


static GmgResult _gmg_vulkan_backend_material_spec_init(GmgMaterialSpec* material_spec, GmgLogicalDevice* logical_device, GmgMaterialSpecCreateArgs* create_args) {
	GmgResult result;
	VkResult vk_result;
	GmgRenderState* render_state = create_args->render_state;
	DasAlctor temp_alctor = gmg_logical_device_temp_alctor(logical_device);

	GmgRenderPass* render_pass;
	result = gmg_render_pass_get(logical_device, create_args->render_pass_id, &render_pass);
	if (result.type < 0) return result;

	GmgShader* shader;
	result = gmg_shader_get(logical_device, create_args->shader_id, &shader);
	if (result.type < 0) return result;

	GmgVertexLayout* vertex_layout;
	result = gmg_vertex_layout_get(create_args->vertex_layout_id, &vertex_layout);
	if (result.type < 0) return result;

	VkPipelineCache vk_pipeline_cache = VK_NULL_HANDLE;
	if (create_args->cache_id.raw) {
		GmgMaterialSpecCache* material_spec_cache;
		result = gmg_material_spec_cache_get(logical_device, create_args->cache_id, &material_spec_cache);
		if (result.type < 0) return result;

		vk_pipeline_cache = material_spec_cache->vulkan.handle;
	}

	VkPipelineShaderStageCreateInfo vk_shader_stages[GmgShader_stages_count] = {0};
	uint32_t vk_shader_stages_count;
	{
		static VkShaderStageFlags vk_shader_stages_stages_graphics[] = {
			VK_SHADER_STAGE_VERTEX_BIT,
			VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT,
			VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT,
			VK_SHADER_STAGE_GEOMETRY_BIT,
			VK_SHADER_STAGE_FRAGMENT_BIT,
		};

		static VkShaderStageFlags vk_shader_stages_stages_graphics_mesh[] = {
			VK_SHADER_STAGE_TASK_BIT_NV,
			VK_SHADER_STAGE_MESH_BIT_NV,
			VK_SHADER_STAGE_FRAGMENT_BIT,
		};

		uint32_t vk_max_shader_stages_count = GmgShader_stages_count_graphics;
		VkShaderStageFlags* vk_shader_stages_stages = vk_shader_stages_stages_graphics;
		if (shader->type == GmgShaderType_graphics_mesh) {
			vk_max_shader_stages_count = GmgShader_stages_count_graphics_mesh;
			vk_shader_stages_stages = vk_shader_stages_stages_graphics_mesh;
		}

		vk_shader_stages_count = vk_max_shader_stages_count;
		for (uint32_t dst_idx = 0, src_idx = 0; src_idx < vk_max_shader_stages_count; dst_idx += 1, src_idx += 1) {
			GmgShaderStage* gmg = &shader->stages[src_idx];

			//
			// skip over the empty shader stages on the gmg end.
			// but keep the destination vulkan index the same.
			while (gmg->format == GmgShaderFormat_none) {
				src_idx += 1;
				vk_shader_stages_count -= 1;
				if (src_idx == vk_max_shader_stages_count) {
					goto VK_PIPELINE_INIT_SHADER_END;
				}
				gmg = &shader->stages[src_idx];
			}

			GmgShaderModule* module;
			result = gmg_shader_module_get(logical_device, gmg->spir_v.module_id, &module);
			if (result.type < 0) return result;

			vk_shader_stages[dst_idx] = (VkPipelineShaderStageCreateInfo) {
				.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
				.pNext = NULL,
				.flags = 0,
				.stage = vk_shader_stages_stages[src_idx],
				.module = module->vulkan.handle,
				.pName = gmg->spir_v.entry_point_name,
				.pSpecializationInfo = NULL,
			};
		}
VK_PIPELINE_INIT_SHADER_END: {}
	}

	VkPipelineVertexInputStateCreateInfo vk_vertex_input;
	{
		VkVertexInputBindingDescription* vk_binding_descs = das_alloc_array(VkVertexInputBindingDescription, temp_alctor, vertex_layout->bindings_count);
		if (vk_binding_descs == NULL) return _gmg_stacktrace_add(GmgResultType_error_out_of_memory_host);

		uint32_t vk_attribs_count = 0;
		for (uint32_t i = 0; i < vertex_layout->bindings_count; i += 1) {
			GmgVertexBindingInfo* binding = &vertex_layout->bindings[i];
			vk_attribs_count += binding->attribs_count;
		}

		VkVertexInputAttributeDescription* vk_attribs = das_alloc_array(VkVertexInputAttributeDescription, temp_alctor, vk_attribs_count);
		if (vk_attribs == NULL) return _gmg_stacktrace_add(GmgResultType_error_out_of_memory_host);

		uint32_t vk_attrib_idx = 0;
		for (uint32_t i = 0; i < vertex_layout->bindings_count; i += 1) {
			GmgVertexBindingInfo* binding = &vertex_layout->bindings[i];
			VkVertexInputBindingDescription* vk_desc = &vk_binding_descs[i];

			vk_desc->binding = i;
			vk_desc->stride = binding->size;
			vk_desc->inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

			uint32_t offset = 0;
			for (uint32_t j = 0; j < binding->attribs_count; j += 1) {
				GmgVertexAttribInfo* attrib = &binding->attribs[j];
				VkVertexInputAttributeDescription* vk_attrib = &vk_attribs[vk_attrib_idx];
				if (vk_attrib == NULL) return _gmg_stacktrace_add(GmgResultType_error_out_of_memory_host);

				vk_attrib->location = attrib->location_idx;
				vk_attrib->binding = i;
				vk_attrib->format = _gmg_vulkan_convert_to_vertex_input_format[attrib->elmt_type][attrib->vector_type];
				vk_attrib->offset = offset;

				offset = (uint32_t)(uintptr_t)das_ptr_round_up_align((void*)(uintptr_t)offset, GmgVertexElmtType_aligns[attrib->elmt_type]);
				offset += (uint32_t)GmgVertexElmtType_sizes[attrib->elmt_type] * (uint32_t)GmgVertexVectorType_elmts_count(attrib->vector_type);
				vk_attrib_idx += 1;
			}
		}

		vk_vertex_input = (VkPipelineVertexInputStateCreateInfo) {
			.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
			.pNext = NULL,
			.flags = 0,
			.vertexBindingDescriptionCount = vertex_layout->bindings_count,
			.vertexAttributeDescriptionCount = vk_attribs_count,
			.pVertexBindingDescriptions = vk_binding_descs,
			.pVertexAttributeDescriptions = vk_attribs,
		};
	}

	VkPipelineInputAssemblyStateCreateInfo vk_input_assembly = (VkPipelineInputAssemblyStateCreateInfo) {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
		.pNext = NULL,
		.flags = 0,
		.topology = _gmg_vulkan_convert_to_topology[render_state->topology],
		.primitiveRestartEnable = render_state->enable_primitive_restart,
	};

	VkPipelineTessellationStateCreateInfo vk_tessellation = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO,
		.pNext = NULL,
		.flags = 0,
		.patchControlPoints = render_state->tessellation_patch_control_points,
	};

	VkPipelineViewportStateCreateInfo vk_viewport = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
		.pNext = NULL,
		.flags = 0,
		.viewportCount = render_state->viewports_count,
		.pViewports = NULL,
		.scissorCount = render_state->viewports_count, // vulkan spec says that viewportCount & scissorCount have to be the same number.
		.pScissors = NULL,
	};

	VkPipelineRasterizationStateCreateInfo vk_rasterization;
	{
		GmgRenderStateRasterization* gmg_info = &render_state->rasterization;

		vk_rasterization = (VkPipelineRasterizationStateCreateInfo) {
			.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
			.pNext = NULL,
			.flags = 0,
			.depthClampEnable = (gmg_info->flags & GmgRasterizationFlags_enable_depth_clamp) != 0,
			.rasterizerDiscardEnable = (gmg_info->flags & GmgRasterizationFlags_enable_discard) != 0,
			.polygonMode = _gmg_vulkan_convert_to_polygon_mode[gmg_info->polygon_mode],
			.frontFace = _gmg_vulkan_convert_to_front_face[gmg_info->front_face],
			.depthBiasEnable = (gmg_info->flags & GmgRasterizationFlags_enable_depth_bias) != 0,
			.depthBiasConstantFactor = gmg_info->depth_bias_constant_factor,
			.depthBiasClamp =  gmg_info->depth_bias_clamp,
			.depthBiasSlopeFactor = gmg_info->depth_bias_slope_factor,
			.lineWidth = gmg_info->line_width,
		};

		result = _gmg_vulkan_convert_to_cull_mode(gmg_info->cull_mode_flags, &vk_rasterization.cullMode);
		if (result.type < 0) return result;
	}

	VkPipelineMultisampleStateCreateInfo vk_multisample;
	{
		GmgRenderStateMultisample* gmg_info = &render_state->multisample;

		vk_multisample = (VkPipelineMultisampleStateCreateInfo) {
			.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
			.pNext = NULL,
			.flags = 0,
			.rasterizationSamples = gmg_info->rasterization_samples_count,
			.sampleShadingEnable = (gmg_info->flags & GmgMultisampleFlags_enable_sample_shading) != 0,
			.minSampleShading = gmg_info->min_sample_shading,
			.pSampleMask = gmg_info->sample_mask,
			.alphaToCoverageEnable = (gmg_info->flags & GmgMultisampleFlags_enable_alpha_to_coverage) != 0,
			.alphaToOneEnable = (gmg_info->flags & GmgMultisampleFlags_enable_alpha_to_one) != 0,
		};
	};

	VkPipelineDepthStencilStateCreateInfo vk_depth_stencil;
	{
		GmgRenderStateDepthStencil* gmg_info = &render_state->depth_stencil;

		vk_depth_stencil = (VkPipelineDepthStencilStateCreateInfo) {
			.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
			.pNext = NULL,
			.flags = 0,
			.depthTestEnable = (gmg_info->flags & GmgDepthStencilFlags_enable_depth_test) != 0,
			.depthWriteEnable = (gmg_info->flags & GmgDepthStencilFlags_enable_depth_write) != 0,
			.depthCompareOp = _gmg_vulkan_convert_to_compare_op[gmg_info->depth_compare_op],
			.depthBoundsTestEnable = (gmg_info->flags & GmgDepthStencilFlags_enable_depth_bounds_test) != 0,
			.stencilTestEnable = (gmg_info->flags & GmgDepthStencilFlags_enable_stencil_test) != 0,
			.minDepthBounds = 0.f,
			.maxDepthBounds = 0.f,
		};
		result = _gmg_vulkan_convert_to_stencil_op_state(&gmg_info->front, &vk_depth_stencil.front);
		if (result.type < 0) return result;

		result = _gmg_vulkan_convert_to_stencil_op_state(&gmg_info->back, &vk_depth_stencil.back);
		if (result.type < 0) return result;
	};

	VkPipelineColorBlendStateCreateInfo vk_color_blend;
	{
		GmgRenderStateBlend* gmg_info = &render_state->blend;

		VkPipelineColorBlendAttachmentState* vk_attachments = das_alloc_array(VkPipelineColorBlendAttachmentState, temp_alctor, gmg_info->attachments_count);

		for (uint32_t i = 0; i < gmg_info->attachments_count; i += 1) {
			result = _gmg_vulkan_convert_to_pipeline_color_blend_attachement(&gmg_info->attachments[i], &vk_attachments[i]);
			if (result.type < 0) return result;
		}

		vk_color_blend = (VkPipelineColorBlendStateCreateInfo) {
			.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
			.pNext = NULL,
			.flags = 0,
			.logicOpEnable = gmg_info->enable_logic_op,
			.logicOp = _gmg_vulkan_convert_to_logic_op[gmg_info->logic_op],
			.attachmentCount = gmg_info->attachments_count,
			.pAttachments = vk_attachments,
		};

		das_copy_array(vk_color_blend.blendConstants, gmg_info->blend_constants);
	}

	static VkDynamicState vk_dynamic_states[] = {
		VK_DYNAMIC_STATE_VIEWPORT,
		VK_DYNAMIC_STATE_SCISSOR,
	};
	VkPipelineDynamicStateCreateInfo vk_dynamic = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
		.pNext = NULL,
		.flags = 0,
		.dynamicStateCount = sizeof(vk_dynamic_states) / sizeof(*vk_dynamic_states),
		.pDynamicStates = vk_dynamic_states,
	};

	VkGraphicsPipelineCreateInfo vk_create_info = {
		.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
		.pNext = NULL,
		.flags = 0,
		.stageCount = vk_shader_stages_count,
		.pStages = vk_shader_stages,
		.pVertexInputState = &vk_vertex_input,
		.pInputAssemblyState = &vk_input_assembly,
		.pTessellationState = &vk_tessellation,
		.pViewportState = &vk_viewport,
		.pRasterizationState = &vk_rasterization,
		.pMultisampleState = &vk_multisample,
		.pDepthStencilState = &vk_depth_stencil,
		.pColorBlendState = &vk_color_blend,
		.pDynamicState = &vk_dynamic,
		.layout = shader->vulkan.pipeline_layout,
		.renderPass = render_pass->vulkan.handle,
		.subpass = 0,
		.basePipelineHandle = VK_NULL_HANDLE,
		.basePipelineIndex = 0,
	};

	vk_result = vkCreateGraphicsPipelines(logical_device->vulkan.handle, vk_pipeline_cache, 1, &vk_create_info, GMG_TODO_VULKAN_ALLOCATOR, &material_spec->vulkan.pipeline);
	if (vk_result) return _gmg_vulkan_convert_from_result(vk_result);

	return GmgResult_success;
}

static GmgResult _gmg_vulkan_backend_material_spec_deinit(GmgMaterialSpec* material_spec, GmgLogicalDevice* logical_device) {
	GmgResult result;
	VkResult vk_result;
	vkDestroyPipeline(logical_device->vulkan.handle, material_spec->vulkan.pipeline, GMG_TODO_VULKAN_ALLOCATOR);
	return GmgResult_success;
}

static GmgResult _gmg_vulkan_backend_buffer_reinit(GmgBuffer* buffer, GmgLogicalDevice* logical_device, uint64_t old_size, uint64_t new_size) {
	GmgResult result;
	VkResult vk_result;
	if (buffer->vulkan.handle != VK_NULL_HANDLE) {
		GmgResult result = _gmg_vulkan_device_memory_dealloc(logical_device, buffer->vulkan.device_memory_alloc_id);
		if (result.type < 0) return result;

		vkDestroyBuffer(logical_device->vulkan.handle, buffer->vulkan.handle, GMG_TODO_VULKAN_ALLOCATOR);
	}

	VkBufferCreateFlags vk_create_flags = 0;

	VkBufferUsageFlags vk_usage_flags = 0;
	vk_usage_flags |= VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
	vk_usage_flags |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;

	switch (buffer->type) {
		case GmgBufferType_vertex: vk_usage_flags |= VK_BUFFER_USAGE_VERTEX_BUFFER_BIT; break;
		case GmgBufferType_index: vk_usage_flags |= VK_BUFFER_USAGE_INDEX_BUFFER_BIT; break;
		case GmgBufferType_uniform: vk_usage_flags |= VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT; break;
		case GmgBufferType_storage: vk_usage_flags |= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT; break;
	}

	GmgBufferFlags flags = buffer->flags;
	if (flags & GmgBufferFlags_indirect_draw) vk_usage_flags |= VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT;

	VkQueueFlags queue_flags = VK_QUEUE_TRANSFER_BIT;
	if (flags & GmgBufferFlags_used_for_graphics) queue_flags |= VK_QUEUE_GRAPHICS_BIT;
	if (flags & GmgBufferFlags_used_for_compute) queue_flags |= VK_QUEUE_COMPUTE_BIT;

	uint32_t vk_queue_family_indices[5];
	uint16_t vk_queue_family_indices_count;
	result = _GmgLogicalDevice_vulkan_queue_family_indices_get(
		logical_device, queue_flags, vk_queue_family_indices, &vk_queue_family_indices_count);
	if (result.type < 0) return result;

	VkSharingMode sharing_mode;
	if (vk_queue_family_indices_count > 1) {
		sharing_mode = VK_SHARING_MODE_CONCURRENT;
	} else {
		sharing_mode = VK_SHARING_MODE_EXCLUSIVE;
		vk_queue_family_indices_count = 0;
	}

	VkBufferCreateInfo vk_create_info = {
		.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
		.pNext = NULL,
		.flags = vk_create_flags,
		.size = new_size,
		.usage = vk_usage_flags,
		.sharingMode = sharing_mode,
		.queueFamilyIndexCount = vk_queue_family_indices_count,
		.pQueueFamilyIndices = vk_queue_family_indices,
	};

	vk_result = vkCreateBuffer(logical_device->vulkan.handle, &vk_create_info, GMG_TODO_VULKAN_ALLOCATOR, &buffer->vulkan.handle);
	if (vk_result) return _gmg_vulkan_convert_from_result(vk_result);

	VkMemoryRequirements mem_req;
	vkGetBufferMemoryRequirements(logical_device->vulkan.handle, buffer->vulkan.handle, &mem_req);

	_GmgVulkanAllocResult alloc_result;
	_GmgVulkanAllocArgs args = {
		.size = mem_req.size,
		.align = mem_req.alignment,
		.memory_type_bits = mem_req.memoryTypeBits,
		.memory_location = _GmgVulkanMemoryLocation_gpu,
		.type = _GmgVulkanAllocType_buffer,
	};
	result = _gmg_vulkan_device_memory_alloc(logical_device, &args, &alloc_result, NULL);
	if (result.type < 0) return result;

	buffer->vulkan.device_memory_alloc_id = alloc_result.id;

	vk_result = vkBindBufferMemory(logical_device->vulkan.handle, buffer->vulkan.handle, alloc_result.device_memory, alloc_result.device_memory_offset);
	if (vk_result) return _gmg_vulkan_convert_from_result(vk_result);

	return GmgResult_success;

	return GmgResult_success;
}
static GmgResult _gmg_vulkan_backend_buffer_deinit(GmgBuffer* buffer, GmgLogicalDevice* logical_device) {
	GmgResult result;
	VkResult vk_result;
	vkDestroyBuffer(logical_device->vulkan.handle, buffer->vulkan.handle, GMG_TODO_VULKAN_ALLOCATOR);
	return GmgResult_success;
}
static GmgResult _gmg_vulkan_backend_buffer_get_write_buffer(GmgBuffer* buffer, GmgLogicalDevice* logical_device, uint64_t offset, uintptr_t size, void** data_out) {
	GmgResult result;
	VkResult vk_result;

	result = _gmg_vulkan_memory_stager_stage_buffer(logical_device, buffer->vulkan.handle, offset, size, data_out);
	if (result.type < 0) return result;

	return GmgResult_success;
}


static GmgResult _gmg_vulkan_backend_render_pass_layout_init(GmgRenderPassLayout* render_pass_layout, GmgLogicalDevice* logical_device, GmgRenderPassLayoutCreateArgs* create_args) {
	GmgResult result;
	VkResult vk_result;
	VkAttachmentDescription* vk_attachments;
	VkSubpassDescription vk_subpass;
	result = _gmg_vulkan_convert_to_render_pass_args(logical_device,
		create_args->attachments, create_args->attachments_count,
		gmg_true, &vk_attachments, &vk_subpass);
	if (result.type < 0) return result;

	VkRenderPassCreateInfo vk_create_info = {
		.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
		.pNext = NULL,
		.flags = 0,
		.pAttachments = vk_attachments,
		.attachmentCount = create_args->attachments_count,
		.pSubpasses = &vk_subpass,
		.subpassCount = 1,
		.pDependencies = NULL,
		.dependencyCount = 0,
	};

	vk_result = vkCreateRenderPass(logical_device->vulkan.handle, &vk_create_info, GMG_TODO_VULKAN_ALLOCATOR, &render_pass_layout->vulkan.handle);
	if (vk_result) return _gmg_vulkan_convert_from_result(vk_result);

	return GmgResult_success;
}
static GmgResult _gmg_vulkan_backend_render_pass_layout_deinit(GmgRenderPassLayout* render_pass_layout, GmgLogicalDevice* logical_device) {
	GmgResult result;
	VkResult vk_result;
	vkDestroyRenderPass(logical_device->vulkan.handle, render_pass_layout->vulkan.handle, GMG_TODO_VULKAN_ALLOCATOR);
	return GmgResult_success;
}

static GmgResult _gmg_vulkan_backend_render_pass_init(GmgRenderPass* render_pass, GmgLogicalDevice* logical_device, GmgRenderPassCreateArgs* create_args) {
	GmgResult result;
	VkResult vk_result;

	GmgRenderPassLayout* rpl;
	result = gmg_render_pass_layout_get(logical_device, create_args->layout_id, &rpl);
	if (result.type < 0) return result;

	VkAttachmentDescription* vk_attachments;
	VkSubpassDescription vk_subpass;
	result = _gmg_vulkan_convert_to_render_pass_args(logical_device,
		rpl->attachments, create_args->attachments_count,
		gmg_true, &vk_attachments, &vk_subpass);
	if (result.type < 0) return result;

	for (uint32_t i = 0; i < create_args->attachments_count; i += 1) {
		GmgAttachmentInfo* gmg = &create_args->attachments[i];
		GmgAttachmentLayout* gmg_rpl = &rpl->attachments[i];
		VkAttachmentDescription* vk = &vk_attachments[i];
		vk->loadOp = _gmg_vulkan_convert_to_attachment_load_op[gmg->load_op];
		vk->storeOp = _gmg_vulkan_convert_to_attachment_store_op[gmg->store_op];
		vk->stencilLoadOp = _gmg_vulkan_convert_to_attachment_load_op[gmg->stencil_load_op];
		vk->stencilStoreOp = _gmg_vulkan_convert_to_attachment_store_op[gmg->stencil_store_op];
	}

	VkSubpassDependency vk_dependencies[] = {
		{
			.srcSubpass      = VK_SUBPASS_EXTERNAL,
			.dstSubpass      = 0,
			.srcStageMask    = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
			.dstStageMask    = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
			.srcAccessMask   = VK_ACCESS_MEMORY_READ_BIT,
			.dstAccessMask   = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
			.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT
		},
		{
			.srcSubpass      = 0,
			.dstSubpass      = VK_SUBPASS_EXTERNAL,
			.srcStageMask    = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
			.dstStageMask    = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
			.srcAccessMask   = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
			.dstAccessMask   = VK_ACCESS_MEMORY_READ_BIT,
			.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT
		}
	};

	VkRenderPassCreateInfo vk_create_info = {
		.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
		.pNext = NULL,
		.flags = 0,
		.pAttachments = vk_attachments,
		.attachmentCount = create_args->attachments_count,
		.pSubpasses = &vk_subpass,
		.subpassCount = 1,
		.pDependencies = vk_dependencies,
		.dependencyCount = 2,
	};

	vk_result = vkCreateRenderPass(logical_device->vulkan.handle, &vk_create_info, GMG_TODO_VULKAN_ALLOCATOR, &render_pass->vulkan.handle);
	if (vk_result) return _gmg_vulkan_convert_from_result(vk_result);

	return GmgResult_success;
}
static GmgResult _gmg_vulkan_backend_render_pass_deinit(GmgRenderPass* render_pass, GmgLogicalDevice* logical_device) {
	GmgResult result;
	VkResult vk_result;
	vkDestroyRenderPass(logical_device->vulkan.handle, render_pass->vulkan.handle, GMG_TODO_VULKAN_ALLOCATOR);
	return GmgResult_success;
}

static GmgResult _gmg_vulkan_backend_render_pass_clear_draw_cmds(GmgRenderPass* render_pass, GmgLogicalDevice* logical_device) {
	GmgDescriptorSetAllocator* allocator = NULL;
	if (render_pass->draw_cmd_descriptor_set_allocator_id.raw) {
		GmgResult result = gmg_descriptor_set_allocator_get(logical_device, render_pass->draw_cmd_descriptor_set_allocator_id, &allocator);
		if (result.type < 0) return result;
	}

	if (allocator && allocator->vulkan.reset_counter != render_pass->draw_cmd_descriptor_set_allocator_last_reset_counter) {
		//
		// the allocator for the draw command has been cleared...
		// so clear out all the previous recording data so it cannot be reused.
		// this is okay to do, since all of the pools from the allocator get thrown away.
		//
		GmgVkRenderPassDescriptorSets* prev = &render_pass->vulkan.draw_cmd_sets[1];
		render_pass->draw_cmd_descriptor_set_allocator_last_reset_counter = allocator->vulkan.reset_counter;
		DasStk_clear(&prev->keys);
		DasStk_clear(&prev->entries);
		DasStk_clear(&prev->descriptors);
	} else {
		//
		// mark all of the descriptor sets that come from the submission before last
		// that were not reused in the last submission.
		//
		GmgVkRenderPassDescriptorSets* prev = &render_pass->vulkan.draw_cmd_sets[1];
		DasStk_foreach(&prev->entries, i) {
			GmgVkRenderPassDescriptorSet* set = DasStk_get(&prev->entries, i);
			if (set->handle == VK_NULL_HANDLE) continue;

			uint64_t key = *DasStk_get(&prev->keys, i);
			GmgDescriptorSetLayoutId layout_id = (GmgDescriptorSetLayoutId) { .raw = key & 0xffffffff };

			_GmgVulkanDescriptorSetToBeDeallocated entry = {
				.handle = set->handle,
				.layout_id = layout_id,
				.allocator_id = render_pass->draw_cmd_descriptor_set_allocator_id,
			};

			GmgResult result = _gmg_vulkan_descriptor_set_mark_for_deallocation(logical_device, prev->submission_fence, &entry);
			if (result.type < 0) return result;
		}

		//
		// swap draw command sets around so the previous recording will be in index 1.
		//
		GmgVkRenderPassDescriptorSets tmp = render_pass->vulkan.draw_cmd_sets[0];
		render_pass->vulkan.draw_cmd_sets[0] = render_pass->vulkan.draw_cmd_sets[1];
		render_pass->vulkan.draw_cmd_sets[1] = tmp;
	}

	//
	// clear the draw command sets so we can start from a fresh.
	//
	GmgVkRenderPassDescriptorSets* current = &render_pass->vulkan.draw_cmd_sets[0];
	DasStk_clear(&current->keys);
	DasStk_clear(&current->entries);
	DasStk_clear(&current->descriptors);

	DasStk_clear(&render_pass->vulkan.draw_cmd_dynamic_descriptor_offsets);
	DasStk_clear(&render_pass->vulkan.material_ids);

	return GmgResult_success;
}

static GmgResult _gmg_vulkan_backend_render_pass_queue_descriptors(GmgDrawCmdBuilder* builder, GmgRenderPass* render_pass, _GmgDrawCmd* cmd) {
	uint32_t descriptors_count = DasStk_count(&builder->descriptors);
	uint32_t hash = _gmg_fnv_hash_32_initial;
	hash = _gmg_fnv_hash_32((uint8_t*)&descriptors_count, sizeof(descriptors_count), hash);
	hash = _gmg_fnv_hash_32((uint8_t*)DasStk_data(&builder->descriptors), DasStk_count(&builder->descriptors) * DasStk_elmt_size(&builder->descriptors), hash);

	uint32_t found_descriptor_set_idx = UINT32_MAX;
	uint32_t copy_descriptor_set_idx = UINT32_MAX;
	uint64_t key = (uint64_t)hash << 32 | (uint64_t)builder->descriptor_set_layout_id.raw;

	//
	// first loop over the draw command set data for the current recording
	// and see if any previous draw command have matching descriptors.
	// if so we will just reuse those sets of descriptors.
	//
	// if we cannot find a match we then search through the previous recording.
	// if we find a match, when copy the entry over to the current recording and nullify the original entry.
	//
	GmgVkRenderPassDescriptorSets* sets;
	for (uint32_t i = 0; i < 2; i += 1) {
		sets = &render_pass->vulkan.draw_cmd_sets[i];
		DasStk_foreach(&sets->keys, j) {
			if (key == *DasStk_get(&sets->keys, j)) {
				GmgVkRenderPassDescriptorSet* set = DasStk_get(&sets->entries, j);
				_GmgDescriptor* set_descriptors = DasStk_get(&sets->descriptors, set->descriptors_start_idx);
				if (set->descriptors_count != DasStk_count(&builder->descriptors)) continue;
				if (!das_cmp_elmts(DasStk_data(&builder->descriptors), set_descriptors, DasStk_count(&builder->descriptors))) continue;

				found_descriptor_set_idx = j;
				break;
			}
		}

		if (found_descriptor_set_idx != UINT32_MAX) {
			if (i == 1) {
				//
				// the matching descriptors where found in the previous recording.
				// so reuse the descriptor set for these descriptors by copying the handle over.
				//
				copy_descriptor_set_idx = found_descriptor_set_idx;
				found_descriptor_set_idx = UINT32_MAX;
			}
			break;
		}
	}

	//
	// if we where unable to reuse a descriptor set from this recording, then push on the data.
	// or if a copy was found in the previous recording we will copy over the descriptor set handle.
	//
	if (found_descriptor_set_idx == UINT32_MAX) {
		sets = &render_pass->vulkan.draw_cmd_sets[0];
		found_descriptor_set_idx = DasStk_count(&sets->entries);

		GmgVkRenderPassDescriptorSet* set = DasStk_push(&sets->entries, NULL);
		if (!set) return _gmg_stacktrace_add(GmgResultType_error_out_of_memory_host);

		if (copy_descriptor_set_idx != UINT32_MAX) {
			GmgVkRenderPassDescriptorSet* src_set = DasStk_get(&render_pass->vulkan.draw_cmd_sets[1].entries, copy_descriptor_set_idx);
			set->handle = src_set->handle;
			src_set->handle = VK_NULL_HANDLE;
		}

		set->handle = VK_NULL_HANDLE;
		set->descriptors_start_idx = DasStk_count(&sets->descriptors);
		set->descriptors_count = DasStk_count(&builder->descriptors);

		if (!DasStk_push(&sets->keys, &key)) return _gmg_stacktrace_add(GmgResultType_error_out_of_memory_host);
		if (!DasStk_push_many(&sets->descriptors, DasStk_data(&builder->descriptors), DasStk_count(&builder->descriptors))) return _gmg_stacktrace_add(GmgResultType_error_out_of_memory_host);
	}

	//
	// store the draw command's descriptor set index back in the command.
	// and then copy over the dynamic offsets if there are any.
	//
	cmd->vulkan.draw_cmd_descriptor_set_idx_plus_1 = found_descriptor_set_idx + 1;
	cmd->dynamic_descriptor_offsets_start_idx = DasStk_count(&render_pass->vulkan.draw_cmd_dynamic_descriptor_offsets);
	cmd->dynamic_descriptor_offsets_count = DasStk_count(&builder->dynamic_descriptor_offsets);

	if (DasStk_count(&builder->dynamic_descriptor_offsets)) {
		if (!DasStk_push_many(&render_pass->vulkan.draw_cmd_dynamic_descriptor_offsets, DasStk_data(&builder->dynamic_descriptor_offsets), DasStk_count(&builder->dynamic_descriptor_offsets))) return _gmg_stacktrace_add(GmgResultType_error_out_of_memory_host);
	}

	return GmgResult_success;
}

static GmgResult _gmg_vulkan_backend_frame_buffer_init(GmgFrameBuffer* frame_buffer, GmgLogicalDevice* logical_device, GmgFrameBufferCreateArgs* create_args) {
	GmgResult result;
	VkResult vk_result;
	GmgRenderPassLayout* render_pass_layout;
	result = gmg_render_pass_layout_get(logical_device, create_args->render_pass_layout_id, &render_pass_layout);
	if (result.type < 0) return result;
	DasAlctor temp_alctor = gmg_logical_device_temp_alctor(logical_device);

	VkImageView* vk_attachments = das_alloc_array(VkImageView, temp_alctor, create_args->attachments_count);
	for (uint32_t i = 0; i < create_args->attachments_count; i += 1) {
		GmgTexture* texture;
		result = gmg_texture_get(logical_device, create_args->attachments[i], &texture);
		if (result.type < 0) return result;

		vk_attachments[i] = texture->vulkan.image_view;
	}

	VkFramebufferCreateInfo vk_create_info = {
		.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
		.pNext = NULL,
		.flags = 0,
		.renderPass = render_pass_layout->vulkan.handle,
		.attachmentCount = create_args->attachments_count,
		.pAttachments = vk_attachments,
		.width = create_args->width,
		.height = create_args->height,
		.layers = create_args->layers,
	};

	vk_result = vkCreateFramebuffer(logical_device->vulkan.handle, &vk_create_info, GMG_TODO_VULKAN_ALLOCATOR, &frame_buffer->vulkan.handle);
	if (vk_result) return _gmg_vulkan_convert_from_result(vk_result);

	return GmgResult_success;
}
static GmgResult _gmg_vulkan_backend_frame_buffer_deinit(GmgFrameBuffer* frame_buffer, GmgLogicalDevice* logical_device) {
	GmgResult result;
	VkResult vk_result;
	vkDestroyFramebuffer(logical_device->vulkan.handle, frame_buffer->vulkan.handle, GMG_TODO_VULKAN_ALLOCATOR);
	return GmgResult_success;
}


static GmgResult _gmg_vulkan_backend_swapchain_reinit(GmgSwapchain* swapchain, GmgLogicalDevice* logical_device) {
	GmgResult result;
	VkResult vk_result;
	GmgPhysicalDevice* physical_device;
	result = gmg_physical_device_get(logical_device->physical_device_idx, &physical_device);
	if (result.type < 0) return result;

	if (physical_device->vulkan.surface_capabilities == NULL) {
		physical_device->vulkan.surface_capabilities = das_alloc_elmt(VkSurfaceCapabilitiesKHR, GMG_TODO_INTERNAL_ALLOCATOR);
		if (physical_device->vulkan.surface_capabilities == NULL) return _gmg_stacktrace_add(GmgResultType_error_out_of_memory_host);

		vk_result = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physical_device->vulkan.handle, swapchain->surface.vulkan, physical_device->vulkan.surface_capabilities);
		if (vk_result) return _gmg_vulkan_convert_from_result(vk_result);
	}
	VkSurfaceCapabilitiesKHR* surface_capabilities = physical_device->vulkan.surface_capabilities;

	if (physical_device->vulkan.surface_present_modes == NULL) {
		vk_result = vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device->vulkan.handle, swapchain->surface.vulkan, &physical_device->vulkan.surface_present_modes_count, NULL);
		if (vk_result) return _gmg_vulkan_convert_from_result(vk_result);

		physical_device->vulkan.surface_present_modes = das_alloc_array(VkPresentModeKHR, GMG_TODO_INTERNAL_ALLOCATOR, physical_device->vulkan.surface_present_modes_count);
		if (physical_device->vulkan.surface_present_modes == NULL) return _gmg_stacktrace_add(GmgResultType_error_out_of_memory_host);

		vk_result = vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device->vulkan.handle, swapchain->surface.vulkan, &physical_device->vulkan.surface_present_modes_count, physical_device->vulkan.surface_present_modes);
		if (vk_result) return _gmg_vulkan_convert_from_result(vk_result);
	}
	VkPresentModeKHR* surface_present_modes = physical_device->vulkan.surface_present_modes;
	uint32_t surface_present_modes_count = physical_device->vulkan.surface_present_modes_count;


	uint32_t min_images_count = das_max_u(swapchain->min_textures_count, surface_capabilities->minImageCount);
	if (surface_capabilities->maxImageCount != 0) {
		min_images_count = das_min_u(min_images_count, surface_capabilities->maxImageCount);
	}

	VkExtent2D image_extent = { .width = swapchain->width, .height = swapchain->height };
	if (surface_capabilities->currentExtent.width != UINT32_MAX) {
		image_extent.width = das_min_u(das_max_u(image_extent.width, surface_capabilities->minImageExtent.width), surface_capabilities->maxImageExtent.width);
		image_extent.height = das_min_u(das_max_u(image_extent.height, surface_capabilities->minImageExtent.height), surface_capabilities->maxImageExtent.height);
	}

	uint32_t array_layers_count = das_min_u(das_max_u(swapchain->array_layers_count, 1), surface_capabilities->maxImageArrayLayers);
	swapchain->width = image_extent.width;
	swapchain->height = image_extent.height;
	swapchain->array_layers_count = array_layers_count;

	VkPresentModeKHR present_mode = VK_PRESENT_MODE_FIFO_KHR; // always present so default to fifo
	if (!swapchain->vsync || !swapchain->fifo) {
		VkPresentModeKHR desired_present_mode;
		if (swapchain->vsync) {
			desired_present_mode = VK_PRESENT_MODE_MAILBOX_KHR;
		} else {
			desired_present_mode = swapchain->fifo ? VK_PRESENT_MODE_FIFO_RELAXED_KHR : VK_PRESENT_MODE_IMMEDIATE_KHR;
		}
		for (uint32_t i = 0; i < surface_present_modes_count; i += 1) {
			if (surface_present_modes[i] == desired_present_mode) {
				present_mode = desired_present_mode;
				break;
			}
		}
	}

	VkSurfaceKHR vk_surface = swapchain->surface.vulkan;
	uint32_t present_queue_family_idx;
	//
	// get the present queue
	{
		uint32_t queue_families_count;
		for (uint32_t i = 0; i < physical_device->vulkan.queue_families_count; i += 1) {
			VkBool32 is_supported;
			vk_result = vkGetPhysicalDeviceSurfaceSupportKHR(physical_device->vulkan.handle, i, vk_surface, &is_supported);
			if (vk_result) return _gmg_vulkan_convert_from_result(vk_result);

			if (is_supported) {
				vkGetDeviceQueue(logical_device->vulkan.handle, i, 0, &swapchain->vulkan.present_queue);
				present_queue_family_idx = i;
			}
		}

		if (swapchain->vulkan.present_queue == VK_NULL_HANDLE) {
			return _gmg_stacktrace_add(GmgResultType_error_cannot_find_present_queue);
		}
	}

	VkCompositeAlphaFlagBitsKHR alpha_fmt = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	VkCompositeAlphaFlagBitsKHR alpha_list[] = {
		VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
		VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR,
		VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR,
		VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR
	};

	for (int i = 0; i < sizeof(alpha_list) / sizeof(VkCompositeAlphaFlagBitsKHR); i += 1) {
		if (surface_capabilities->supportedCompositeAlpha & alpha_list[i]) {
			alpha_fmt = alpha_list[i];
			break;
		}
	}

	VkSwapchainCreateInfoKHR vk_create_info = {
		.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
		.pNext = NULL,
		.flags = 0,
		.surface = vk_surface,
		.minImageCount = min_images_count,
		.imageFormat = _gmg_vulkan_convert_to_format[swapchain->format],
		.imageColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR,
		.imageExtent = image_extent,
		.imageArrayLayers = array_layers_count,
		.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
		.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
		.queueFamilyIndexCount = 0,
		.pQueueFamilyIndices = NULL,
		.preTransform = surface_capabilities->currentTransform,
		.compositeAlpha = alpha_fmt,
		.presentMode = present_mode,
		.clipped = VK_TRUE,
		.oldSwapchain = swapchain->vulkan.handle,
	};

	vk_result = vkCreateSwapchainKHR(logical_device->vulkan.handle, &vk_create_info, GMG_TODO_VULKAN_ALLOCATOR, &swapchain->vulkan.handle);
	if (vk_result) return _gmg_vulkan_convert_from_result(vk_result);


	uint32_t images_count;
	vk_result = vkGetSwapchainImagesKHR(logical_device->vulkan.handle, swapchain->vulkan.handle, &images_count, NULL);
	if (vk_result) return _gmg_vulkan_convert_from_result(vk_result);

	DasAlctor temp_alctor = gmg_logical_device_temp_alctor(logical_device);
	VkImage* images = das_alloc_array(VkImage, temp_alctor, images_count);
	if (images == NULL) return _gmg_stacktrace_add(GmgResultType_error_out_of_memory_host);

	vk_result = vkGetSwapchainImagesKHR(logical_device->vulkan.handle, swapchain->vulkan.handle, &images_count, images);
	if (vk_result) return _gmg_vulkan_convert_from_result(vk_result);


	swapchain->texture_ids = das_realloc_array(GmgTextureId, GMG_TODO_INTERNAL_ALLOCATOR, swapchain->texture_ids, swapchain->textures_count, images_count);
	if (swapchain->texture_ids == NULL) return _gmg_stacktrace_add(GmgResultType_error_out_of_memory_host);
	swapchain->textures_count = images_count;

	for (uint32_t i = 0; i < images_count; i += 1) {
		VkImageAspectFlags aspect_mask = 0;
		switch (swapchain->format) {
			case GmgTextureFormat_s8:
				aspect_mask |= VK_IMAGE_ASPECT_STENCIL_BIT;
				break;
			case GmgTextureFormat_d16_s8:
			case GmgTextureFormat_d24_s8:
			case GmgTextureFormat_d32_s8:
				aspect_mask |= VK_IMAGE_ASPECT_STENCIL_BIT;
			case GmgTextureFormat_d16:
			case GmgTextureFormat_d32:
				aspect_mask |= VK_IMAGE_ASPECT_DEPTH_BIT;
				break;
			default:
				aspect_mask = VK_IMAGE_ASPECT_COLOR_BIT;
				break;
		}
		VkImageView image_view;
		VkImageViewCreateInfo vk_image_view_create_info = {
			.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
			.pNext = NULL,
			.flags = 0,
			.image = images[i],
			.viewType = VK_IMAGE_VIEW_TYPE_2D,
			.format = _gmg_vulkan_convert_to_format[swapchain->format],
			.components = {
				.r = VK_COMPONENT_SWIZZLE_R,
				.g = VK_COMPONENT_SWIZZLE_G,
				.b = VK_COMPONENT_SWIZZLE_B,
				.a = VK_COMPONENT_SWIZZLE_A,
			},
			.subresourceRange = {
				.aspectMask = aspect_mask,
				.baseMipLevel = 0,
				.levelCount = 1,
				.baseArrayLayer = 0,
				.layerCount = array_layers_count,
			},
		};

		vk_result = vkCreateImageView(logical_device->vulkan.handle, &vk_image_view_create_info, GMG_TODO_VULKAN_ALLOCATOR, &image_view);
		if (vk_result) return _gmg_vulkan_convert_from_result(vk_result);


		_GmgLogicalDeviceObjectId object_id;
		_GmgLogicalDeviceObject* object = DasPool_alloc(_GmgLogicalDeviceObjectId, &logical_device->object_pool, &object_id);
		if (object == NULL) return _gmg_stacktrace_add(GmgResultType_error_logical_device_object_pool_full);
		swapchain->texture_ids[i].raw = object_id.raw;

		GmgTexture* texture = &object->texture;
		texture->vulkan.image = images[i];
		texture->vulkan.image_view = image_view;
		texture->format = swapchain->format;
		texture->flags |= GmgTextureFlags_swapchain;
		texture->width = image_extent.width;
		texture->height = image_extent.height;
		texture->depth = 1;
		texture->mip_levels = 1;
		texture->array_layers_count = array_layers_count;
		texture->samples = 1;
	}

	return GmgResult_success;
}
static GmgResult _gmg_vulkan_backend_swapchain_deinit(GmgSwapchain* swapchain, GmgLogicalDevice* logical_device) {
	GmgResult result;
	VkResult vk_result;
	vkDestroySwapchainKHR(logical_device->vulkan.handle, swapchain->vulkan.handle, GMG_TODO_VULKAN_ALLOCATOR);
	return GmgResult_success;
}
static GmgResult _gmg_vulkan_backend_swapchain_get_next_texture_idx(GmgSwapchain* swapchain, GmgLogicalDevice* logical_device, uint32_t* next_texture_idx_out) {
	GmgResult result;
	VkResult vk_result;
	VkSemaphore semaphore = logical_device->vulkan.semaphore_present;

	uint32_t image_idx = 0;
	vk_result = vkAcquireNextImageKHR(logical_device->vulkan.handle, swapchain->vulkan.handle, UINT64_MAX, semaphore, VK_NULL_HANDLE, &image_idx);
	das_assert(vk_result != VK_TIMEOUT, "we unexpectedly got VK_TIMEOUT while aquiring the next swapchain image");
	das_assert(vk_result != VK_NOT_READY, "we unexpectedly got VK_NOT_READY while aquiring the next swapchain image");
	das_assert(vk_result != VK_SUBOPTIMAL_KHR, "we unexpectedly got VK_SUBOPTIMAL_KHR while aquiring the next swapchain image");
	if (vk_result) return _gmg_vulkan_convert_from_result(vk_result);

	swapchain->vulkan.image_idx = image_idx;
	*next_texture_idx_out = image_idx;

	return GmgResult_success;
}
static GmgResult _gmg_vulkan_backend_swapchain_present(GmgLogicalDevice* logical_device, GmgSwapchainId* swapchain_ids, uint32_t swapchains_count) {
	GmgResult result;
	VkResult vk_result;
	for (uint32_t i = 0; i < swapchains_count; i += 1) {
		GmgSwapchain* swapchain;
		result = gmg_swapchain_get(logical_device, swapchain_ids[i], &swapchain);
		if (result.type < 0) return result;

		//
		// TODO: find the present queue using platform function like: vkGetPhysicalDeviceXlibPresentationSupportKHR
		// so we can then call the vkQueuePresentKHR function a single time for all the swapchains
		//
		VkPresentInfoKHR  present_info = {
			.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
			.pNext = NULL,
			.waitSemaphoreCount = 1,
			.pWaitSemaphores = &logical_device->vulkan.semaphore_render,
			.swapchainCount = 1,
			.pSwapchains = &swapchain->vulkan.handle,
			.pImageIndices = &swapchain->vulkan.image_idx,
			.pResults = NULL,
		};
		vk_result = vkQueuePresentKHR(swapchain->vulkan.present_queue, &present_info);
		if (vk_result) return _gmg_vulkan_convert_from_result(vk_result);
	}

	return GmgResult_success;
}

static _GmgBackendImplFn _gmg_vulkan_backend_impl_fns[_GmgBackendImplFnType_COUNT] = {
	[_GmgBackendImplFnType_none] = _gmg_backend_none,
#define X(name) [_GmgBackendImplFnType_##name] = (_GmgBackendImplFn)_gmg_vulkan_backend_##name,
_GMG_BACKEND_IMPL_FN_LIST
#undef X

};

#endif // GMG_ENABLE_VULKAN

// ==========================================================
//
//
// General
//
//
// ==========================================================

static  const char* _GmgResultType_strings[GmgResultType_COUNT] = {
	[GmgResultType_error_ - GmgResultType_error_END] = "",
	[GmgResultType_error_unexpected_null_id - GmgResultType_error_END] = "unexpected_null_id",
	[GmgResultType_error_object_use_after_free - GmgResultType_error_END] = "object_use_after_free",
	[GmgResultType_error_logical_device_object_pool_full - GmgResultType_error_END] = "logical_device_object_pool_full",
	[GmgResultType_error_out_of_memory_host - GmgResultType_error_END] = "out_of_memory_host",
	[GmgResultType_error_out_of_memory_device - GmgResultType_error_END] = "out_of_memory_device",
	[GmgResultType_error_multiple_depth_stencil_attachments - GmgResultType_error_END] = "multiple_depth_stencil_attachments",
	[GmgResultType_error_cannot_find_transfer_queue - GmgResultType_error_END] = "cannot_find_transfer_queue",
	[GmgResultType_error_cannot_find_compute_queue - GmgResultType_error_END] = "cannot_find_compute_queue",
	[GmgResultType_error_cannot_find_graphics_queue - GmgResultType_error_END] = "cannot_find_graphics_queue",
	[GmgResultType_error_cannot_find_present_queue - GmgResultType_error_END] = "cannot_find_present_queue",
	[GmgResultType_error_incompatible_shader_format - GmgResultType_error_END] = "incompatible_shader_format",
	[GmgResultType_error_vertex_layout_pool_full - GmgResultType_error_END] = "vertex_layout_pool_full",
	[GmgResultType_error_layout_size_mismatch - GmgResultType_error_END] = "layout_size_mismatch",
	[GmgResultType_error_incompatible_backend_type - GmgResultType_error_END] = "incompatible_backend_type",
	[GmgResultType_error_missing_vertex_shader - GmgResultType_error_END] = "missing_vertex_shader",
	[GmgResultType_error_missing_fragment_shader - GmgResultType_error_END] = "missing_fragment_shader",
	[GmgResultType_error_missing_mesh_shader - GmgResultType_error_END] = "missing_mesh_shader",
	[GmgResultType_error_missing_compute_shader - GmgResultType_error_END] = "missing_compute_shader",
	[GmgResultType_error_binding_idx_out_of_bounds - GmgResultType_error_END] = "binding_idx_out_of_bounds",
	[GmgResultType_error_descriptor_type_mismatch - GmgResultType_error_END] = "descriptor_type_mismatch",
	[GmgResultType_error_binding_is_not_dynamic - GmgResultType_error_END] = "binding_is_not_dynamic",
	[GmgResultType_error_binding_elmt_idx_out_of_bounds - GmgResultType_error_END] = "binding_elmt_idx_out_of_bounds",
	[GmgResultType_error_expected_vertex_buffer - GmgResultType_error_END] = "expected_vertex_buffer",
	[GmgResultType_error_incompatible_binding_idx - GmgResultType_error_END] = "incompatible_binding_idx",
	[GmgResultType_error_expected_index_buffer - GmgResultType_error_END] = "expected_index_buffer",
	[GmgResultType_error_incompatible_render_pass_layout - GmgResultType_error_END] = "incompatible_render_pass_layout",
	[GmgResultType_error_incompatible_vertex_layout - GmgResultType_error_END] = "incompatible_vertex_layout",
	[GmgResultType_error_draw_cmd_already_in_process - GmgResultType_error_END] = "draw_cmd_already_in_process",
	[GmgResultType_error_uninitialized - GmgResultType_error_END] = "uninitialized",
	[GmgResultType_error_push_constants_out_of_bounds - GmgResultType_error_END] = "push_constants_out_of_bounds",
	[GmgResultType_error_vertex_array_has_index_buffer - GmgResultType_error_END] = "vertex_array_has_index_buffer",
	[GmgResultType_error_vertex_array_does_not_have_index_buffer - GmgResultType_error_END] = "vertex_array_does_not_have_index_buffer",
	[GmgResultType_error_buffer_start_idx_out_of_bounds - GmgResultType_error_END] = "buffer_start_idx_out_of_bounds",
	[GmgResultType_error_map_out_of_bounds - GmgResultType_error_END] = "map_out_of_bounds",
	[GmgResultType_error_attachment_index_out_of_bounds - GmgResultType_error_END] = "attachment_index_out_of_bounds",
	[GmgResultType_error_width_must_not_be_zero - GmgResultType_error_END] = "width_must_not_be_zero",
	[GmgResultType_error_height_must_not_be_zero - GmgResultType_error_END] = "height_must_not_be_zero",
	[GmgResultType_error_depth_must_not_be_zero - GmgResultType_error_END] = "depth_must_not_be_zero",
	[GmgResultType_error_mip_levels_must_not_be_zero - GmgResultType_error_END] = "mip_levels_must_not_be_zero",
	[GmgResultType_error_array_layers_must_not_be_zero - GmgResultType_error_END] = "array_layers_must_not_be_zero",
	[GmgResultType_error_descriptor_set_layout_not_in_allocator - GmgResultType_error_END] = "descriptor_set_layout_not_in_allocator",
	[GmgResultType_error_shader_does_not_have_a_draw_cmd_descriptor_set - GmgResultType_error_END] = "shader_does_not_have_a_draw_cmd_descriptor_set",
	[GmgResultType_error_draw_cmd_descriptor_is_set_twice - GmgResultType_error_END] = "draw_cmd_descriptor_is_set_twice",
	[GmgResultType_error_draw_cmd_has_unset_descriptors - GmgResultType_error_END] = "draw_cmd_has_unset_descriptors",
	[GmgResultType_error_pass_descriptor_set_layout_mismatch - GmgResultType_error_END] = "pass_descriptor_set_layout_mismatch",
	[GmgResultType_error_render_pass_attachments_mismatch_with_layout - GmgResultType_error_END] = "render_pass_attachments_mismatch_with_layout",
	[GmgResultType_error_cannot_submit_zero_render_passes - GmgResultType_error_END] = "cannot_submit_zero_render_passes",

	[GmgResultType_success - GmgResultType_error_END] = "success",
};

const char* GmgResultType_string(GmgResultType type) {
	return _GmgResultType_strings[type - GmgResultType_error_END];
}

GmgResult GmgResult_from_DasError(DasError error) {
	// TODO handle this error better by translating.
	// but this cannot be done until the DasError is platform agnostic
	return _gmg_stacktrace_add(GmgResultType_error_out_of_memory_host);
}

das_noreturn void gmg_abort_error(GmgResult result, const char* file, const char* function, int line) {
	fprintf(stderr,
		"\nABORTING: gmg error \"%s\"\n"
		"user location: %s at %s:%d\n",
		GmgResultType_string(result.type),
		function, file, line);
	if (result.stacktrace_id.raw) {
		GmgStacktrace trace;
		gmg_stacktrace_get(result, &trace);
		fprintf(stderr,
			"stacktrace:\n%.*s\n",
			(int)DasStk_count(&trace.string),
			DasStk_data(&trace.string));
	}
	abort();
}

uint8_t GmgTextureFormat_bytes_per_pixel[GmgTextureFormat_COUNT] = {
	[GmgTextureFormat_r8_unorm] = 1,
	[GmgTextureFormat_r8g8_unorm] = 2,
	[GmgTextureFormat_r8g8b8_unorm] = 3,
	[GmgTextureFormat_r8g8b8a8_unorm] = 4,
	[GmgTextureFormat_b8g8r8_unorm] = 3,
	[GmgTextureFormat_b8g8r8a8_unorm] = 4,
};

uint8_t GmgIndexType_sizes[GmgIndexType_COUNT] = {
	[GmgIndexType_u8] = 1,
	[GmgIndexType_u16] = 2,
	[GmgIndexType_u32] = 4,
};

// ==========================================================
//
//
// Vertex Layout
//
//
// ==========================================================

uint8_t GmgVertexElmtType_sizes[GmgVertexElmtType_COUNT] = {
	[GmgVertexElmtType_u8] = 1,
	[GmgVertexElmtType_s8] = 1,
	[GmgVertexElmtType_u8_f32] = 1,
	[GmgVertexElmtType_s8_f32] = 1,
	[GmgVertexElmtType_u8_f32_normalize] = 1,
	[GmgVertexElmtType_s8_f32_normalize] = 1,
	[GmgVertexElmtType_u16] = 2,
	[GmgVertexElmtType_s16] = 2,
	[GmgVertexElmtType_u16_f32] = 2,
	[GmgVertexElmtType_s16_f32] = 2,
	[GmgVertexElmtType_u16_f32_normalize] = 2,
	[GmgVertexElmtType_s16_f32_normalize] = 2,
	[GmgVertexElmtType_u32] = 4,
	[GmgVertexElmtType_s32] = 4,
	[GmgVertexElmtType_u64] = 8,
	[GmgVertexElmtType_s64] = 8,
	[GmgVertexElmtType_f16] = 2,
	[GmgVertexElmtType_f32] = 4,
	[GmgVertexElmtType_f64] = 8,
};

uint8_t GmgVertexElmtType_aligns[GmgVertexElmtType_COUNT] = {
	[GmgVertexElmtType_u8] = 1,
	[GmgVertexElmtType_s8] = 1,
	[GmgVertexElmtType_u8_f32] = 1,
	[GmgVertexElmtType_s8_f32] = 1,
	[GmgVertexElmtType_u8_f32_normalize] = 1,
	[GmgVertexElmtType_s8_f32_normalize] = 1,
	[GmgVertexElmtType_u16] = 2,
	[GmgVertexElmtType_s16] = 2,
	[GmgVertexElmtType_u16_f32] = 2,
	[GmgVertexElmtType_s16_f32] = 2,
	[GmgVertexElmtType_u16_f32_normalize] = 2,
	[GmgVertexElmtType_s16_f32_normalize] = 2,
	[GmgVertexElmtType_u32] = 4,
	[GmgVertexElmtType_s32] = 4,
	[GmgVertexElmtType_u64] = 8,
	[GmgVertexElmtType_s64] = 8,
	[GmgVertexElmtType_f16] = 2,
	[GmgVertexElmtType_f32] = 4,
	[GmgVertexElmtType_f64] = 8,
};

GmgResult gmg_vertex_layout_register(GmgVertexLayout* vl, GmgVertexLayoutId* id_out) {
	GmgVertexLayout* dst_vl = DasPool_alloc(GmgVertexLayoutId, &_gmg.vertex_layout_pool, id_out);
	if (dst_vl == NULL) return _gmg_stacktrace_add(GmgResultType_error_vertex_layout_pool_full);

	GmgVertexBindingInfo* bindings = das_alloc_array(GmgVertexBindingInfo, GMG_TODO_INTERNAL_ALLOCATOR, vl->bindings_count);
	if (bindings == NULL) return _gmg_stacktrace_add(GmgResultType_error_out_of_memory_host);
	for (uint32_t i = 0; i < vl->bindings_count; i += 1) {
		GmgVertexBindingInfo* src = &vl->bindings[i];

		GmgVertexAttribInfo* attribs = das_alloc_array(GmgVertexAttribInfo, GMG_TODO_INTERNAL_ALLOCATOR, src->attribs_count);
		if (attribs == NULL) return _gmg_stacktrace_add(GmgResultType_error_out_of_memory_host);
		das_copy_elmts(attribs, src->attribs, src->attribs_count);

		uint16_t size = 0;
		for (uint32_t i = 0; i < src->attribs_count; i += 1) {
			GmgVertexAttribInfo* info = &src->attribs[i];
			size = (uintptr_t)das_ptr_round_up_align((void*)(uintptr_t)size, GmgVertexElmtType_aligns[info->elmt_type]);
			size += GmgVertexElmtType_sizes[info->elmt_type] * GmgVertexVectorType_elmts_count(info->vector_type);
		}

		if (size != src->size) {
			return _gmg_stacktrace_add(GmgResultType_error_layout_size_mismatch);
		}

		bindings[i].attribs = attribs;
		bindings[i].attribs_count = src->attribs_count;
		bindings[i].size = src->size;
	}

	dst_vl->bindings = bindings;
	dst_vl->bindings_count = vl->bindings_count;

	return GmgResult_success;
}

GmgResult gmg_vertex_layout_deregister(GmgVertexLayoutId id) {
	DasPool_dealloc(GmgVertexLayoutId, &_gmg.vertex_layout_pool, id);
	return GmgResult_success;
}

GmgResult gmg_vertex_layout_get(GmgVertexLayoutId id, GmgVertexLayout** out) {
	if (id.raw == 0) return _gmg_stacktrace_add(GmgResultType_error_unexpected_null_id);
	if (!DasPool_is_id_valid(GmgVertexLayoutId, &_gmg.vertex_layout_pool, id))
		return _gmg_stacktrace_add(GmgResultType_error_object_use_after_free);

	*out = DasPool_id_to_ptr(GmgVertexLayoutId, &_gmg.vertex_layout_pool, id);
	return GmgResult_success;
}

// ==========================================================
//
//
// Physical Device
//
//
// ==========================================================

GmgResult gmg_physical_devices_get(GmgPhysicalDevice** physical_devices_out, uint32_t* physical_devices_count_out) {
	*physical_devices_out = _gmg.physical_devices;
	*physical_devices_count_out = _gmg.physical_devices_count;
	return GmgResult_success;
}

GmgResult gmg_physical_device_get(uint32_t physical_device_idx, GmgPhysicalDevice** out) {
	*out = &_gmg.physical_devices[physical_device_idx];
	return GmgResult_success;
}

GmgResult gmg_physical_device_features(uint32_t physical_device_idx, GmgPhysicalDeviceFeatureFlags* flags_out) {
	GmgPhysicalDevice* physical_device;
	GmgResult result = gmg_physical_device_get(physical_device_idx, &physical_device);
	if (result.type < 0) return result;

	return _gmg_backend_fn_call(physical_device_features, physical_device, flags_out);
}

GmgResult gmg_physical_device_properties(uint32_t physical_device_idx, GmgPhysicalDeviceProperties* properties_out) {
	GmgPhysicalDevice* physical_device;
	GmgResult result = gmg_physical_device_get(physical_device_idx, &physical_device);
	if (result.type < 0) return result;

	return _gmg_backend_fn_call(physical_device_properties, physical_device, properties_out);
}

GmgResult gmg_physical_device_surface_texture_format(uint32_t physical_device_idx, GmgSurface surface, GmgTextureFormat* format_out) {
	if (surface.type != _gmg.backend_type) {
		return _gmg_stacktrace_add(GmgResultType_error_incompatible_backend_type);
	}
	GmgPhysicalDevice* physical_device;
	GmgResult result = gmg_physical_device_get(physical_device_idx, &physical_device);
	if (result.type < 0) return result;

	return _gmg_backend_fn_call(physical_device_surface_texture_format, physical_device, surface, format_out);
}


// ==========================================================
//
//
// Logical Device
//
//
// ==========================================================

GmgResult _GmgLogicalDevice_vulkan_queue_family_indices_get(GmgLogicalDevice* logical_device, VkQueueFlags queue_flags, uint32_t queue_family_indices_out[5], uint16_t* queue_family_indices_count_out) {
	VkQueueFlags flags = queue_flags;

	VkQueueFlags queue_types[3] = { VK_QUEUE_GRAPHICS_BIT, VK_QUEUE_COMPUTE_BIT, VK_QUEUE_TRANSFER_BIT };
	_GmgVulkanQueueType queue_types_data[3][3] = {
		{ _GmgVulkanQueueType_graphics, UINT8_MAX, UINT8_MAX },
		{ _GmgVulkanQueueType_compute, _GmgVulkanQueueType_async_compute, UINT8_MAX },
		{ _GmgVulkanQueueType_transfer, _GmgVulkanQueueType_async_transfer, UINT8_MAX },
	};

	uint32_t queue_family_indices_count = 0;

	for (uint32_t i = 0; i < 3; i += 1) {
		VkQueueFlags queue_type = queue_types[i];
		if (queue_flags & queue_type) {
			for (uint32_t j = 0; j < 3; j += 1) {
				_GmgVulkanQueueType type = queue_types_data[i][j];
				if (type == UINT8_MAX) break;

				GmgBool found = gmg_false;
				uint32_t queue_family_idx = logical_device->vulkan.queue_type_to_queue_family_idx[type];
				for (uint32_t k = 0; k < queue_family_indices_count; k += 1) {
					if (queue_family_indices_out[k] == queue_family_idx) {
						found = gmg_true;
						break;
					}
				}

				if (!found) {
					queue_family_indices_out[queue_family_indices_count] = queue_family_idx;
					queue_family_indices_count += 1;
				}
			}
		}

	}

	*queue_family_indices_count_out = queue_family_indices_count;

	return GmgResult_success;
}

GmgResult gmg_logical_device_init(uint32_t physical_device_idx, GmgLogicalDeviceCreateArgs* create_args, GmgLogicalDevice** out) {
	GmgPhysicalDevice* physical_device;
	GmgResult result = gmg_physical_device_get(physical_device_idx, &physical_device);
	if (result.type < 0) return result;

	GmgLogicalDevice* logical_device = das_alloc_elmt(GmgLogicalDevice, GMG_TODO_INTERNAL_ALLOCATOR);
	if (logical_device == NULL) return _gmg_stacktrace_add(GmgResultType_error_out_of_memory_host);
	logical_device->physical_device_idx = physical_device_idx;
	das_zero_elmt(logical_device);

	*out = logical_device;

	uint32_t reserved_cap = create_args->object_pool_reserve_cap
		? create_args->object_pool_reserve_cap
		: gmg_default_object_pool_reserve_cap;

	uint32_t commit_grow_count = create_args->object_pool_grow_count
		? create_args->object_pool_grow_count
		: gmg_default_object_pool_grow_count;

	DasError das_error = DasPool_init(_GmgLogicalDeviceObjectId, &logical_device->object_pool, reserved_cap, commit_grow_count);
	if (das_error) {
		return GmgResult_from_DasError(das_error);
	}

	das_error = DasLinearAlctor_init(&logical_device->temp_alctor, gmg_frame_allocator_reserve_size, gmg_frame_allocator_grow_size);
	if (das_error) {
		return GmgResult_from_DasError(das_error);
	}

	return _gmg_backend_fn_call(logical_device_init, physical_device, logical_device, create_args);
}

GmgResult gmg_logical_device_deinit(GmgLogicalDevice* logical_device) {
	GmgResult result = _gmg_backend_fn_call(logical_device_deinit, logical_device);
	if (result.type < 0) return result;

	DasError das_error = DasPool_deinit(_GmgLogicalDeviceObjectId, &logical_device->object_pool);
	if (das_error) {
		return GmgResult_from_DasError(das_error);
	}

	return GmgResult_success;
}

GmgResult gmg_logical_device_submit(GmgLogicalDevice* logical_device, GmgRenderPassId* ordered_render_pass_ids, uint32_t render_passes_count) {
	if (render_passes_count == 0) {
		return _gmg_stacktrace_add(GmgResultType_error_cannot_submit_zero_render_passes);
	}
	GmgResult result = _gmg_backend_fn_call(logical_device_submit, logical_device, ordered_render_pass_ids, render_passes_count);
	if (result.type < 0) return result;

	DasAlctor temp_alctor = gmg_logical_device_temp_alctor(logical_device);
	das_alloc_reset(temp_alctor);

	return GmgResult_success;
}

DasAlctor gmg_logical_device_temp_alctor(GmgLogicalDevice* logical_device) {
	return DasLinearAlctor_as_das(&logical_device->temp_alctor);
}

// ==========================================================
//
//
// Sampler
//
//
// ==========================================================

GmgResult gmg_sampler_init(GmgLogicalDevice* logical_device, GmgSamplerCreateArgs* create_args, GmgSamplerId* id_out) {
	return _gmg_logical_device_object_init(logical_device, create_args, (_GmgLogicalDeviceObjectId*)id_out, _GmgBackendImplFnType_sampler_init, NULL);
}

GmgResult gmg_sampler_deinit(GmgLogicalDevice* logical_device, GmgSamplerId id) {
	return _gmg_logical_device_object_deinit(logical_device, id.object_id, _GmgBackendImplFnType_sampler_deinit);
}

GmgResult gmg_sampler_get(GmgLogicalDevice* logical_device, GmgSamplerId id, GmgSampler** out) {
	return _gmg_logical_device_object_get(logical_device, id.object_id, (_GmgLogicalDeviceObject**)out);
}

// ==========================================================
//
//
// Texture
//
//
// ==========================================================

GmgResult gmg_texture_init(GmgLogicalDevice* logical_device, GmgTextureCreateArgs* create_args, GmgTextureId* id_out) {
	das_assert(!(create_args->flags & GmgTextureFlags_swapchain), "swapchain textures cannot be made by the user");

	_GmgLogicalDeviceObjectId object_id;
	_GmgLogicalDeviceObject* object = DasPool_alloc(_GmgLogicalDeviceObjectId, &logical_device->object_pool, &object_id);
	if (object == NULL) return _gmg_stacktrace_add(GmgResultType_error_logical_device_object_pool_full);
	id_out->object_id = object_id;

	GmgTexture* texture = &object->texture;
	texture->type = create_args->type;
	texture->format = create_args->format;
	texture->flags = create_args->flags;
	texture->samples = create_args->samples;

	return GmgResult_success;
}

GmgResult gmg_texture_deinit(GmgLogicalDevice* logical_device, GmgTextureId id) {
	return _gmg_logical_device_object_deinit(logical_device, id.object_id, _GmgBackendImplFnType_texture_deinit);
}

GmgResult gmg_texture_get(GmgLogicalDevice* logical_device, GmgTextureId id, GmgTexture** out) {
	return _gmg_logical_device_object_get(logical_device, id.object_id, (_GmgLogicalDeviceObject**)out);
}

GmgResult gmg_texture_resize(GmgLogicalDevice* logical_device, GmgTextureId id, uint32_t width, uint32_t height, uint32_t depth, uint32_t mip_levels, uint32_t array_layers_count) {
	if (width == 0) {
		return _gmg_stacktrace_add(GmgResultType_error_width_must_not_be_zero);
	}
	if (height == 0) {
		return _gmg_stacktrace_add(GmgResultType_error_height_must_not_be_zero);
	}
	if (depth == 0) {
		return _gmg_stacktrace_add(GmgResultType_error_depth_must_not_be_zero);
	}
	if (mip_levels == 0) {
		return _gmg_stacktrace_add(GmgResultType_error_mip_levels_must_not_be_zero);
	}
	if (array_layers_count == 0) {
		return _gmg_stacktrace_add(GmgResultType_error_array_layers_must_not_be_zero);
	}

	GmgTexture* texture;
	GmgResult result = gmg_texture_get(logical_device, id, &texture);
	if (result.type < 0) return result;

	if (
		texture->width == width &&
		texture->height == height &&
		texture->depth == depth &&
		texture->mip_levels == mip_levels &&
		texture->array_layers_count == array_layers_count
	) return GmgResult_success_(GmgResultType_nothing_changed);

	texture->width = width;
	texture->height = height;
	texture->depth = depth;
	texture->mip_levels = mip_levels;
	texture->array_layers_count = array_layers_count;

	return _gmg_backend_fn_call(texture_reinit, texture, logical_device);
}

GmgResult gmg_texture_get_write_buffer(GmgLogicalDevice* logical_device, GmgTextureId id, GmgTextureArea* area, void** data_out) {
	if (area->width == 0) {
		return _gmg_stacktrace_add(GmgResultType_error_width_must_not_be_zero);
	}
	if (area->height == 0) {
		return _gmg_stacktrace_add(GmgResultType_error_height_must_not_be_zero);
	}
	if (area->depth == 0) {
		return _gmg_stacktrace_add(GmgResultType_error_depth_must_not_be_zero);
	}
	if (area->array_layers_count == 0) {
		return _gmg_stacktrace_add(GmgResultType_error_array_layers_must_not_be_zero);
	}

	GmgTexture* texture;
	GmgResult result = gmg_texture_get(logical_device, id, &texture);
	if (result.type < 0) return result;

	return _gmg_backend_fn_call(texture_get_write_buffer, texture, logical_device, area, data_out);
}

// ==========================================================
//
//
// Descriptors
//
//
// ==========================================================

GmgResult gmg_descriptor_set_layout_init(GmgLogicalDevice* logical_device, GmgDescriptorSetLayoutCreateArgs* create_args, GmgDescriptorSetLayoutId* id_out) {
	GmgDescriptorSetLayout* layout;
	GmgResult result = _gmg_logical_device_object_init(logical_device, create_args, (_GmgLogicalDeviceObjectId*)id_out, _GmgBackendImplFnType_descriptor_set_layout_init, (_GmgLogicalDeviceObject**)&layout);

	GmgDescriptorBinding* descriptor_bindings = das_alloc_array(GmgDescriptorBinding, GMG_TODO_INTERNAL_ALLOCATOR, create_args->descriptor_bindings_count);
	if (descriptor_bindings == NULL) return _gmg_stacktrace_add(GmgResultType_error_out_of_memory_host);
	das_copy_elmts(descriptor_bindings, create_args->descriptor_bindings, create_args->descriptor_bindings_count);
	uint16_t dynamic_descriptors_count = 0;
	uint32_t start_idx = 0;
	for (uint32_t i = 0; i < create_args->descriptor_bindings_count; i += 1) {
		GmgDescriptorBinding* binding = &descriptor_bindings[i];
		if (binding->count == 0) return _gmg_stacktrace_add(GmgResultType_error_binding_count_cannot_be_zero);
		if (binding->type == GmgDescriptorType_uniform_buffer_dynamic || binding->type == GmgDescriptorType_storage_buffer_dynamic) {
			binding->_dynamic_idx = dynamic_descriptors_count;
			dynamic_descriptors_count += 1;
		}
		binding->_start_idx = start_idx;
		start_idx += binding->count;
	}

	layout->descriptor_bindings = descriptor_bindings;
	layout->descriptor_bindings_count = create_args->descriptor_bindings_count;
	layout->descriptors_count = start_idx;
	layout->dynamic_descriptors_count = dynamic_descriptors_count;
	return GmgResult_success;
}

GmgResult gmg_descriptor_set_layout_deinit(GmgLogicalDevice* logical_device, GmgDescriptorSetLayoutId id) {
	return _gmg_logical_device_object_deinit(logical_device, id.object_id, _GmgBackendImplFnType_descriptor_set_layout_deinit);
}

GmgResult gmg_descriptor_set_layout_get(GmgLogicalDevice* logical_device, GmgDescriptorSetLayoutId id, GmgDescriptorSetLayout** out) {
	return _gmg_logical_device_object_get(logical_device, id.object_id, (_GmgLogicalDeviceObject**)out);
}


GmgResult gmg_descriptor_set_allocator_init(GmgLogicalDevice* logical_device, GmgDescriptorSetAllocatorCreateArgs* create_args, GmgDescriptorSetAllocatorId* id_out) {
	return _gmg_logical_device_object_init(logical_device, create_args, (_GmgLogicalDeviceObjectId*)id_out, _GmgBackendImplFnType_descriptor_set_allocator_init, NULL);
}

GmgResult gmg_descriptor_set_allocator_deinit(GmgLogicalDevice* logical_device, GmgDescriptorSetAllocatorId id) {
	return _gmg_logical_device_object_deinit(logical_device, id.object_id, _GmgBackendImplFnType_descriptor_set_allocator_deinit);
}

GmgResult gmg_descriptor_set_allocator_get(GmgLogicalDevice* logical_device, GmgDescriptorSetAllocatorId id, GmgDescriptorSetAllocator** out) {
	return _gmg_logical_device_object_get(logical_device, id.object_id, (_GmgLogicalDeviceObject**)out);
}

GmgResult gmg_descriptor_set_allocator_reset(GmgLogicalDevice* logical_device, GmgDescriptorSetAllocatorId id) {
	GmgDescriptorSetAllocator* allocator;
	GmgResult result = gmg_descriptor_set_allocator_get(logical_device, id, &allocator);
	if (result.type < 0) return result;

	return _gmg_backend_fn_call(descriptor_set_allocator_reset, allocator, logical_device);
}

// ==========================================================
//
//
// Shader
//
//
// ==========================================================

uint8_t GmgShaderType_stages_counts[GmgShaderType_COUNT] = {
	[GmgShaderType_graphics] = GmgShader_stages_count_graphics,
	[GmgShaderType_graphics_mesh] = GmgShader_stages_count_graphics_mesh,
	[GmgShaderType_compute] = GmgShader_stages_count_compute,
};

GmgResult gmg_shader_module_init(GmgLogicalDevice* logical_device, GmgShaderModuleCreateArgs* create_args, GmgShaderModuleId* id_out) {
	return _gmg_logical_device_object_init(logical_device, create_args, (_GmgLogicalDeviceObjectId*)id_out, _GmgBackendImplFnType_shader_module_init, NULL);
}

GmgResult gmg_shader_module_deinit(GmgLogicalDevice* logical_device, GmgShaderModuleId id) {
	return _gmg_logical_device_object_deinit(logical_device, id.object_id, _GmgBackendImplFnType_shader_module_deinit);
}

GmgResult gmg_shader_module_get(GmgLogicalDevice* logical_device, GmgShaderModuleId id, GmgShaderModule** out) {
	return _gmg_logical_device_object_get(logical_device, id.object_id, (_GmgLogicalDeviceObject**)out);
}

GmgResult gmg_shader_init(GmgLogicalDevice* logical_device, GmgShaderCreateArgs* create_args, GmgShaderId* id_out) {
	switch (create_args->type) {
		case GmgShaderType_graphics:
			if (create_args->stages.graphics.vertex.format == GmgShaderFormat_none) {
				return _gmg_stacktrace_add(GmgResultType_error_missing_vertex_shader);
			}
			if (create_args->stages.graphics.fragment.format == GmgShaderFormat_none) {
				return _gmg_stacktrace_add(GmgResultType_error_missing_fragment_shader);
			}
			break;
		case GmgShaderType_graphics_mesh:
			if (create_args->stages.graphics_mesh.mesh.format == GmgShaderFormat_none) {
				return _gmg_stacktrace_add(GmgResultType_error_missing_mesh_shader);
			}
			if (create_args->stages.graphics_mesh.fragment.format == GmgShaderFormat_none) {
				return _gmg_stacktrace_add(GmgResultType_error_missing_fragment_shader);
			}
			break;
		case GmgShaderType_compute:
			if (create_args->stages.compute.format == GmgShaderFormat_none) {
				return _gmg_stacktrace_add(GmgResultType_error_missing_compute_shader);
			}
			break;
	}

	GmgShader* shader;
	GmgResult result = _gmg_logical_device_object_init(logical_device, create_args, (_GmgLogicalDeviceObjectId*)id_out, _GmgBackendImplFnType_shader_init, (_GmgLogicalDeviceObject**)&shader);
	if (result.type < 0) return result;

	shader->type = create_args->type;
	das_copy_array(shader->stages, create_args->stages.array);
	shader->pass_descriptor_set_layout_id = create_args->pass_descriptor_set_layout_id;
	shader->material_descriptor_set_layout_id = create_args->material_descriptor_set_layout_id;
	shader->draw_cmd_descriptor_set_layout_id = create_args->draw_cmd_descriptor_set_layout_id;
	shader->push_constants_size = create_args->push_constants_size;
	shader->push_constants_shader_stage_flags = create_args->push_constants_shader_stage_flags;

	return GmgResult_success;
}

GmgResult gmg_shader_deinit(GmgLogicalDevice* logical_device, GmgShaderId id) {
	return _gmg_logical_device_object_deinit(logical_device, id.object_id, _GmgBackendImplFnType_shader_deinit);
}

GmgResult gmg_shader_get(GmgLogicalDevice* logical_device, GmgShaderId id, GmgShader** out) {
	return _gmg_logical_device_object_get(logical_device, id.object_id, (_GmgLogicalDeviceObject**)out);
}

// ==========================================================
//
//
// Material Spec Cache
//
//
// ==========================================================

GmgResult gmg_material_spec_cache_init(GmgLogicalDevice* logical_device, GmgMaterialSpecCacheCreateArgs* create_args, GmgMaterialSpecCacheId* id_out) {
	return _gmg_logical_device_object_init(logical_device, create_args, (_GmgLogicalDeviceObjectId*)id_out, _GmgBackendImplFnType_material_spec_cache_init, NULL);
}

GmgResult gmg_material_spec_cache_deinit(GmgLogicalDevice* logical_device, GmgMaterialSpecCacheId id) {
	return _gmg_logical_device_object_deinit(logical_device, id.object_id, _GmgBackendImplFnType_material_spec_cache_deinit);
}

GmgResult gmg_material_spec_cache_get(GmgLogicalDevice* logical_device, GmgMaterialSpecCacheId id, GmgMaterialSpecCache** out) {
	return _gmg_logical_device_object_get(logical_device, id.object_id, (_GmgLogicalDeviceObject**)out);
}

GmgResult gmg_material_spec_cache_get_data(GmgLogicalDevice* logical_device, GmgMaterialSpecCacheId id, void** data_out, uintptr_t* size_out) {
	GmgMaterialSpecCache* material_spec_cache;
	GmgResult result = gmg_material_spec_cache_get(logical_device, id, &material_spec_cache);
	if (result.type < 0) return result;

	return _gmg_backend_fn_call(material_spec_cache_get_data, material_spec_cache, logical_device);
}

// ==========================================================
//
//
// Material Spec
//
//
// ==========================================================

GmgResult gmg_material_spec_init(GmgLogicalDevice* logical_device, GmgMaterialSpecCreateArgs* create_args, GmgMaterialSpecId* id_out) {
	GmgMaterialSpec* material_spec;
	GmgResult result =  _gmg_logical_device_object_init(logical_device, create_args, (_GmgLogicalDeviceObjectId*)id_out, _GmgBackendImplFnType_material_spec_init, (_GmgLogicalDeviceObject**)&material_spec);
	if (result.type < 0) return result;

	material_spec->shader_id = create_args->shader_id;
	material_spec->render_pass_id = create_args->render_pass_id;
	material_spec->vertex_layout_id = create_args->vertex_layout_id;

	return GmgResult_success;
}

GmgResult gmg_material_spec_deinit(GmgLogicalDevice* logical_device, GmgMaterialSpecId id) {
	return _gmg_logical_device_object_deinit(logical_device, id.object_id, _GmgBackendImplFnType_material_spec_deinit);
}

GmgResult gmg_material_spec_get(GmgLogicalDevice* logical_device, GmgMaterialSpecId id, GmgMaterialSpec** out) {
	return _gmg_logical_device_object_get(logical_device, id.object_id, (_GmgLogicalDeviceObject**)out);
}

// ==========================================================
//
//
// Material
//
//
// ==========================================================

GmgResult gmg_material_init(GmgLogicalDevice* logical_device, GmgMaterialCreateArgs* create_args, GmgMaterialId* id_out) {
	GmgMaterial* material;
	GmgResult result = _gmg_logical_device_object_init(logical_device, create_args, (_GmgLogicalDeviceObjectId*)id_out, _GmgBackendImplFnType_none, (_GmgLogicalDeviceObject**)&material);
	if (result.type < 0) return result;

	GmgMaterialSpec* material_spec;
	result = gmg_material_spec_get(logical_device, create_args->spec_id, &material_spec);
	if (result.type < 0) return result;

	GmgShader* shader;
	result = gmg_shader_get(logical_device, material_spec->shader_id, &shader);
	if (result.type < 0) return result;

	if (shader->material_descriptor_set_layout_id.raw) {
		GmgDescriptorSetLayout* layout;
		result = gmg_descriptor_set_layout_get(logical_device, shader->material_descriptor_set_layout_id, &layout);
		if (result.type < 0) return result;

		uint32_t* dynamic_descriptor_offsets = NULL;
		if (layout->dynamic_descriptors_count) {
			dynamic_descriptor_offsets = das_alloc_array(uint32_t, GMG_TODO_INTERNAL_ALLOCATOR, layout->dynamic_descriptors_count);
			if (dynamic_descriptor_offsets == NULL) return _gmg_stacktrace_add(GmgResultType_error_out_of_memory_host);
			das_zero_elmts(dynamic_descriptor_offsets, layout->dynamic_descriptors_count);
		}

		switch (_gmg.backend_type) {
#ifdef GMG_ENABLE_VULKAN
			case GmgBackendType_vulkan: {
				_GmgVkDescriptorSet* set = &material->vulkan.descriptor_set;
				set->descriptors_count = layout->descriptors_count;
				set->descriptors = das_alloc_array(_GmgDescriptor, GMG_TODO_INTERNAL_ALLOCATOR, layout->descriptors_count);
				if (set->descriptors == NULL) return _gmg_stacktrace_add(GmgResultType_error_out_of_memory_host);
				das_zero_elmts(set->descriptors, layout->descriptors_count);
				set->has_updated = gmg_true;
				break;
			};
#endif
		}

		if (result.type < 0) return result;
		material->dynamic_descriptor_offsets = dynamic_descriptor_offsets;
		material->dynamic_descriptors_count = layout->dynamic_descriptors_count;
	}

	material->spec_id = create_args->spec_id;
	material->descriptor_set_layout_id = shader->material_descriptor_set_layout_id;
	material->descriptor_set_allocator_id = create_args->descriptor_set_allocator_id;

	return GmgResult_success;
}

GmgResult gmg_material_deinit(GmgLogicalDevice* logical_device, GmgMaterialId id) {
	switch (_gmg.backend_type) {
#ifdef GMG_ENABLE_VULKAN
		case GmgBackendType_vulkan: {
			GmgMaterial* material;
			GmgResult result = gmg_material_get(logical_device, id, &material);
			if (result.type < 0) return result;

			_GmgVulkanDescriptorSetToBeDeallocated entry = {
				.handle = material->vulkan.descriptor_set.handle,
				.layout_id = material->descriptor_set_layout_id,
				.allocator_id = material->descriptor_set_allocator_id,
			};
			result = _gmg_vulkan_descriptor_set_mark_for_deallocation(logical_device, material->vulkan.descriptor_set.latest_used_fence, &entry);
			if (result.type < 0) return result;
			break;
		};
#endif
	}

	return _gmg_logical_device_object_deinit(logical_device, id.object_id, _GmgBackendImplFnType_none);
}

GmgResult gmg_material_get(GmgLogicalDevice* logical_device, GmgMaterialId id, GmgMaterial** out) {
	return _gmg_logical_device_object_get(logical_device, id.object_id, (_GmgLogicalDeviceObject**)out);
}

GmgResult gmg_material_set_descriptor(GmgLogicalDevice* logical_device, GmgMaterialId id, uint16_t binding_idx, uint16_t elmt_idx, _GmgLogicalDeviceObjectId object_id, GmgDescriptorType type, uint64_t buffer_start_idx) {
	GmgMaterial* material;
	GmgResult result = gmg_material_get(logical_device, id, &material);
	if (result.type < 0) return result;

	GmgDescriptorSetLayout* layout;
	result = gmg_descriptor_set_layout_get(logical_device, material->descriptor_set_layout_id, &layout);
	if (result.type < 0) return result;

	_GmgDescriptor d = {
		.binding_idx = binding_idx,
		.elmt_idx = elmt_idx,
		.type = type,
		.object_id = object_id,
		.buffer_start_idx = buffer_start_idx,
	};

	uint32_t set_binding_idx = 0;
	result = _gmg_check_set_descriptor(logical_device, layout->descriptor_bindings, layout->descriptor_bindings_count, &d, &set_binding_idx);
	if (result.type < 0) return result;

	switch (_gmg.backend_type) {
#ifdef GMG_ENABLE_VULKAN
		case GmgBackendType_vulkan: {
			_GmgVkDescriptorSet* set = &material->vulkan.descriptor_set;
			set->has_updated = gmg_true;
			set->descriptors[set_binding_idx] = d;
			break;
		};
#endif
	}

	return GmgResult_success;
}

GmgResult gmg_material_set_sampler(GmgLogicalDevice* logical_device, GmgMaterialId id, uint16_t binding_idx, uint16_t elmt_idx, GmgSamplerId sampler_id) {
	return gmg_material_set_descriptor(logical_device, id, binding_idx, elmt_idx, sampler_id.object_id, GmgDescriptorType_sampler, 0);
}

GmgResult gmg_material_set_texture(GmgLogicalDevice* logical_device, GmgMaterialId id, uint16_t binding_idx, uint16_t elmt_idx, GmgTextureId texture_id) {
	return gmg_material_set_descriptor(logical_device, id, binding_idx, elmt_idx, texture_id.object_id, GmgDescriptorType_texture, 0);
}

GmgResult gmg_material_set_uniform_buffer(GmgLogicalDevice* logical_device, GmgMaterialId id, uint16_t binding_idx, uint16_t elmt_idx, GmgBufferId buffer_id, uint64_t buffer_start_idx) {
	return gmg_material_set_descriptor(logical_device, id, binding_idx, elmt_idx, buffer_id.object_id, GmgDescriptorType_uniform_buffer, buffer_start_idx);
}

GmgResult gmg_material_set_storage_buffer(GmgLogicalDevice* logical_device, GmgMaterialId id, uint16_t binding_idx, uint16_t elmt_idx, GmgBufferId buffer_id, uint64_t buffer_start_idx) {
	return gmg_material_set_descriptor(logical_device, id, binding_idx, elmt_idx, buffer_id.object_id, GmgDescriptorType_storage_buffer, buffer_start_idx);
}

GmgResult gmg_material_set_dynamic_descriptor_offset(GmgLogicalDevice* logical_device, GmgMaterialId id, uint16_t binding_idx, uint16_t elmt_idx, uint32_t dynamic_offset) {
	GmgMaterial* material;
	GmgResult result = gmg_material_get(logical_device, id, &material);
	if (result.type < 0) return result;

	GmgDescriptorSetLayout* layout;
	result = gmg_descriptor_set_layout_get(logical_device, material->descriptor_set_layout_id, &layout);
	if (result.type < 0) return result;

	_GmgDescriptor* descriptors;
	switch (_gmg.backend_type) {
#ifdef GMG_ENABLE_VULKAN
		case GmgBackendType_vulkan: {
			descriptors = material->vulkan.descriptor_set.descriptors;
			break;
		};
#endif
	}

	uint16_t dynamic_idx;
	result = _gmg_check_set_dynamic_descriptor_offset(logical_device, descriptors, layout->descriptor_bindings, layout->descriptor_bindings_count, binding_idx, elmt_idx, &dynamic_offset, &dynamic_idx);
	if (result.type < 0) return result;

	material->dynamic_descriptor_offsets[dynamic_idx] = dynamic_offset;
	return GmgResult_success;
}

// ==========================================================
//
//
// Buffer
//
//
// ==========================================================

GmgResult gmg_buffer_init(GmgLogicalDevice* logical_device, GmgBufferCreateArgs* create_args, GmgBufferId* id_out) {
	GmgBuffer* buffer;
	GmgResult result = _gmg_logical_device_object_init(logical_device, create_args, (_GmgLogicalDeviceObjectId*)id_out, _GmgBackendImplFnType_none, (_GmgLogicalDeviceObject**)&buffer);
	if (result.type < 0) return result;

	buffer->flags = create_args->flags;
	buffer->type = create_args->type;

	uint16_t elmt_size;
	switch (create_args->type) {
		case GmgBufferType_vertex: {
			GmgVertexLayout* vl;
			GmgResult result = gmg_vertex_layout_get(create_args->vertex.layout_id, &vl);
			if (result.type < 0) return result;

			if (vl->bindings_count <= create_args->vertex.binding_idx) {
				return _gmg_stacktrace_add(GmgResultType_error_binding_idx_out_of_bounds);
			}

			buffer->vertex.layout_id = create_args->vertex.layout_id;
			buffer->vertex.binding_idx = create_args->vertex.binding_idx;
			elmt_size = vl->bindings[create_args->vertex.binding_idx].size;
			break;
		};
		case GmgBufferType_index:
			buffer->index.type = create_args->index.type;
			elmt_size = GmgIndexType_sizes[create_args->index.type];
			break;
		case GmgBufferType_uniform:
		case GmgBufferType_storage: {
			elmt_size = create_args->uniform.elmt_size;
			break;
		};
	}
	buffer->elmt_size = elmt_size;
	return GmgResult_success;
}

GmgResult gmg_buffer_deinit(GmgLogicalDevice* logical_device, GmgBufferId id) {
	return _gmg_logical_device_object_deinit(logical_device, id.object_id, _GmgBackendImplFnType_buffer_deinit);
}

GmgResult gmg_buffer_get(GmgLogicalDevice* logical_device, GmgBufferId id, GmgBuffer** out) {
	return _gmg_logical_device_object_get(logical_device, id.object_id, (_GmgLogicalDeviceObject**)out);
}

GmgResult gmg_buffer_resize(GmgLogicalDevice* logical_device, GmgBufferId id, uint64_t elmts_count) {
	GmgBuffer* buffer;
	GmgResult result = gmg_buffer_get(logical_device, id, &buffer);
	if (result.type < 0) return result;

	if (buffer->elmts_count == elmts_count) {
		return GmgResult_success_(GmgResultType_nothing_changed);
	}

	buffer->elmts_count = elmts_count;

	uint64_t old_size = buffer->elmts_count * (uint64_t)buffer->elmt_size;
	uint64_t new_size = elmts_count * (uint64_t)buffer->elmt_size;
	return _gmg_backend_fn_call(buffer_reinit, buffer, logical_device, old_size, new_size);
}

GmgResult gmg_buffer_get_write_buffer(GmgLogicalDevice* logical_device, GmgBufferId id, uint64_t start_idx, uintptr_t elmts_count, void** data_out) {
	GmgBuffer* buffer;
	GmgResult result = gmg_buffer_get(logical_device, id, &buffer);
	if (result.type < 0) return result;

	if (start_idx + elmts_count > buffer->elmts_count) {
		return _gmg_stacktrace_add(GmgResultType_error_map_out_of_bounds);
	}

	uint64_t offset = start_idx * (uint64_t)buffer->elmt_size;
	uint64_t size = buffer->elmts_count * (uint64_t)buffer->elmt_size;
	return _gmg_backend_fn_call(buffer_get_write_buffer, buffer, logical_device, offset, size, data_out);
}

// ==========================================================
//
//
// Vertex Array
//
//
// ==========================================================

GmgResult gmg_vertex_array_init(GmgLogicalDevice* logical_device, GmgVertexArrayCreateArgs* create_args, GmgVertexArrayId* id_out) {
	GmgVertexArray* array;
	GmgResult result = _gmg_logical_device_object_init(logical_device, create_args, (_GmgLogicalDeviceObjectId*)id_out, _GmgBackendImplFnType_none, (_GmgLogicalDeviceObject**)&array);
	if (result.type < 0) return result;

	GmgBufferId* vertex_buffer_ids = das_alloc_array(GmgBufferId, GMG_TODO_INTERNAL_ALLOCATOR, create_args->vertex_buffers_count);
	if (vertex_buffer_ids == NULL) return _gmg_stacktrace_add(GmgResultType_error_out_of_memory_host);
	das_copy_elmts(vertex_buffer_ids, create_args->vertex_buffer_ids, create_args->vertex_buffers_count);

	for (uint32_t i = 0; i < create_args->vertex_buffers_count; i += 1) {
		GmgBuffer* buffer;
		result = gmg_buffer_get(logical_device, vertex_buffer_ids[i], &buffer);
		if (result.type < 0) return result;

		if (buffer->type != GmgBufferType_vertex) {
			return _gmg_stacktrace_add(GmgResultType_error_expected_vertex_buffer);
		}
		if (buffer->vertex.binding_idx != i) {
			return _gmg_stacktrace_add(GmgResultType_error_incompatible_binding_idx);
		}
	}

	if (create_args->index_buffer_id.raw) {
		GmgBuffer* buffer;
		result = gmg_buffer_get(logical_device, create_args->index_buffer_id, &buffer);
		if (result.type < 0) return result;

		if (buffer->type != GmgBufferType_index) {
			return _gmg_stacktrace_add(GmgResultType_error_expected_index_buffer);
		}
	}

	array->layout_id = create_args->layout_id;
	array->vertex_buffer_ids = vertex_buffer_ids;
	array->vertex_buffers_count = create_args->vertex_buffers_count;
	array->index_buffer_id = create_args->index_buffer_id;

	return GmgResult_success;
}

GmgResult gmg_vertex_array_deinit(GmgLogicalDevice* logical_device, GmgVertexArrayId id) {
	return _gmg_logical_device_object_deinit(logical_device, id.object_id, _GmgBackendImplFnType_none);
}

GmgResult gmg_vertex_array_get(GmgLogicalDevice* logical_device, GmgVertexArrayId id, GmgVertexArray** out) {
	return _gmg_logical_device_object_get(logical_device, id.object_id, (_GmgLogicalDeviceObject**)out);
}

// ==========================================================
//
//
// Render Pass Layout
//
//
// ==========================================================

GmgResult gmg_render_pass_layout_init(GmgLogicalDevice* logical_device, GmgRenderPassLayoutCreateArgs* create_args, GmgRenderPassLayoutId* id_out) {
	GmgRenderPassLayout* rpl;
	GmgResult result = _gmg_logical_device_object_init(logical_device, create_args, (_GmgLogicalDeviceObjectId*)id_out, _GmgBackendImplFnType_render_pass_layout_init, (_GmgLogicalDeviceObject**)&rpl);

	GmgAttachmentLayout* attachments = das_alloc_array(GmgAttachmentLayout, GMG_TODO_INTERNAL_ALLOCATOR, create_args->attachments_count);
	if (attachments == NULL) return _gmg_stacktrace_add(GmgResultType_error_out_of_memory_host);
	das_copy_elmts(attachments, create_args->attachments, create_args->attachments_count);

	rpl->attachments = attachments;
	rpl->attachments_count = create_args->attachments_count;

	return GmgResult_success;
}

GmgResult gmg_render_pass_layout_deinit(GmgLogicalDevice* logical_device, GmgRenderPassLayoutId id) {
	return _gmg_logical_device_object_deinit(logical_device, id.object_id, _GmgBackendImplFnType_render_pass_layout_deinit);
}

GmgResult gmg_render_pass_layout_get(GmgLogicalDevice* logical_device, GmgRenderPassLayoutId id, GmgRenderPassLayout** out) {
	return _gmg_logical_device_object_get(logical_device, id.object_id, (_GmgLogicalDeviceObject**)out);
}

// ==========================================================
//
//
// Render Pass
//
//
// ==========================================================

GmgResult gmg_render_pass_init(GmgLogicalDevice* logical_device, GmgRenderPassCreateArgs* create_args, GmgRenderPassId* id_out) {
	GmgRenderPassLayout* rpl;
	GmgResult result = gmg_render_pass_layout_get(logical_device, create_args->layout_id, &rpl);
	if (result.type < 0) return result;

	if (rpl->attachments_count != create_args->attachments_count) {
		return _gmg_stacktrace_add(GmgResultType_error_render_pass_attachments_mismatch_with_layout);
	}

	GmgRenderPass* rp;
	result = _gmg_logical_device_object_init(logical_device, create_args, (_GmgLogicalDeviceObjectId*)id_out, _GmgBackendImplFnType_render_pass_init, (_GmgLogicalDeviceObject**)&rp);
	if (result.type < 0) return result;

	GmgClearValue* attachment_clear_values = NULL;
	if (create_args->attachment_clear_values) {
		attachment_clear_values = das_alloc_array(GmgClearValue, GMG_TODO_INTERNAL_ALLOCATOR, create_args->attachments_count);
		das_copy_elmts(attachment_clear_values, create_args->attachment_clear_values, create_args->attachments_count);
	}

	uint32_t* pass_dynamic_descriptor_offsets = NULL;
	uint32_t pass_dynamic_descriptor_offsets_count = 0;
	if (create_args->pass_descriptor_set_layout_id.raw) {
		GmgDescriptorSetLayout* layout;
		result = gmg_descriptor_set_layout_get(logical_device, create_args->pass_descriptor_set_layout_id, &layout);
		if (result.type < 0) return result;

		switch (_gmg.backend_type) {
#ifdef GMG_ENABLE_VULKAN
			case GmgBackendType_vulkan: {
				_GmgVkDescriptorSet* set = &rp->vulkan.pass_descriptor_set;
				set->descriptors_count = layout->descriptors_count;
				set->descriptors = das_alloc_array(_GmgDescriptor, GMG_TODO_INTERNAL_ALLOCATOR, layout->descriptors_count);
				if (set->descriptors == NULL) return _gmg_stacktrace_add(GmgResultType_error_out_of_memory_host);
				das_zero_elmts(set->descriptors, layout->descriptors_count);
				set->has_updated = gmg_true;
				break;
			};
#endif
		}

		if (layout->dynamic_descriptors_count) {
			pass_dynamic_descriptor_offsets_count = layout->dynamic_descriptors_count;
			pass_dynamic_descriptor_offsets = das_alloc_array(uint32_t, GMG_TODO_INTERNAL_ALLOCATOR, layout->dynamic_descriptors_count);
			if (pass_dynamic_descriptor_offsets == NULL) return _gmg_stacktrace_add(GmgResultType_error_out_of_memory_host);
			das_zero_elmts(pass_dynamic_descriptor_offsets, layout->dynamic_descriptors_count);
		}
	}

	rp->layout_id = create_args->layout_id;

	rp->scissor.width = INT32_MAX;
	rp->scissor.height = INT32_MAX;

	rp->attachments_count = create_args->attachments_count;
	rp->attachment_clear_values = attachment_clear_values;
	rp->pass_dynamic_descriptors_count = pass_dynamic_descriptor_offsets_count;
	rp->pass_dynamic_descriptor_offsets = pass_dynamic_descriptor_offsets;
	rp->pass_descriptor_set_layout_id = create_args->pass_descriptor_set_layout_id;
	rp->pass_descriptor_set_allocator_id = create_args->pass_descriptor_set_allocator_id;
	rp->draw_cmd_descriptor_set_allocator_id = create_args->draw_cmd_descriptor_set_allocator_id;

	return GmgResult_success;
}

GmgResult gmg_render_pass_deinit(GmgLogicalDevice* logical_device, GmgRenderPassId id) {
	switch (_gmg.backend_type) {
#ifdef GMG_ENABLE_VULKAN
		case GmgBackendType_vulkan: {
			GmgRenderPass* render_pass;
			GmgResult result = gmg_render_pass_get(logical_device, id, &render_pass);
			if (result.type < 0) return result;

			_GmgVulkanDescriptorSetToBeDeallocated entry = {
				.handle = render_pass->vulkan.pass_descriptor_set.handle,
				.layout_id = render_pass->pass_descriptor_set_layout_id,
				.allocator_id = render_pass->pass_descriptor_set_allocator_id,
			};
			result = _gmg_vulkan_descriptor_set_mark_for_deallocation(logical_device, render_pass->vulkan.pass_descriptor_set.latest_used_fence, &entry);
			if (result.type < 0) return result;
			break;
		};
#endif
	}

	return _gmg_logical_device_object_deinit(logical_device, id.object_id, _GmgBackendImplFnType_render_pass_deinit);
}

GmgResult gmg_render_pass_get(GmgLogicalDevice* logical_device, GmgRenderPassId id, GmgRenderPass** out) {
	return _gmg_logical_device_object_get(logical_device, id.object_id, (_GmgLogicalDeviceObject**)out);
}

GmgResult gmg_render_pass_set_frame_buffer(GmgLogicalDevice* logical_device, GmgRenderPassId id, GmgFrameBufferId frame_buffer_id) {
	GmgRenderPass* render_pass;
	GmgResult result = gmg_render_pass_get(logical_device, id, &render_pass);
	if (result.type < 0) return result;

	GmgFrameBuffer* frame_buffer;
	result = gmg_frame_buffer_get(logical_device, frame_buffer_id, &frame_buffer);
	if (result.type < 0) return result;

	if (frame_buffer->render_pass_layout_id.raw != render_pass->layout_id.raw) {
		return _gmg_stacktrace_add(GmgResultType_error_incompatible_render_pass_layout);
	}

	render_pass->frame_buffer_id = frame_buffer_id;
	return GmgResult_success;
}

GmgResult gmg_render_pass_set_draw_order(GmgLogicalDevice* logical_device, GmgRenderPassId id, GmgDrawOrder draw_order) {
	GmgRenderPass* render_pass;
	GmgResult result = gmg_render_pass_get(logical_device, id, &render_pass);
	if (result.type < 0) return result;

	render_pass->draw_order = draw_order;
	return GmgResult_success;
}

GmgResult gmg_render_pass_set_viewport(GmgLogicalDevice* logical_device, GmgRenderPassId id, GmgViewport* viewport) {
	GmgRenderPass* render_pass;
	GmgResult result = gmg_render_pass_get(logical_device, id, &render_pass);
	if (result.type < 0) return result;

	render_pass->viewport = *viewport;
	return GmgResult_success;
}

GmgResult gmg_render_pass_set_scissor(GmgLogicalDevice* logical_device, GmgRenderPassId id, GmgRect2u* scissor) {
	GmgRenderPass* render_pass;
	GmgResult result = gmg_render_pass_get(logical_device, id, &render_pass);
	if (result.type < 0) return result;

	render_pass->scissor = *scissor;
	return GmgResult_success;
}

GmgResult gmg_render_pass_set_attachment_clear_value(GmgLogicalDevice* logical_device, GmgRenderPassId id, uint16_t attachment_idx, GmgClearValue* clear_value) {
	GmgRenderPass* render_pass;
	GmgResult result = gmg_render_pass_get(logical_device, id, &render_pass);
	if (result.type < 0) return result;

	if (attachment_idx >= render_pass->attachments_count)
		return _gmg_stacktrace_add(GmgResultType_error_attachment_index_out_of_bounds);

	render_pass->attachment_clear_values[attachment_idx] = *clear_value;
	return GmgResult_success;
}

// ===========================================
//
// Pass Descriptors
//
// ===========================================

GmgResult gmg_render_pass_set_descriptor(GmgLogicalDevice* logical_device, GmgRenderPassId id, uint16_t binding_idx, uint16_t elmt_idx, _GmgLogicalDeviceObjectId object_id, GmgDescriptorType type, uint64_t buffer_start_idx) {
	GmgRenderPass* render_pass;
	GmgResult result = gmg_render_pass_get(logical_device, id, &render_pass);
	if (result.type < 0) return result;

	GmgDescriptorSetLayout* layout;
	result = gmg_descriptor_set_layout_get(logical_device, render_pass->pass_descriptor_set_layout_id, &layout);
	if (result.type < 0) return result;

	_GmgDescriptor d = {
		.binding_idx = binding_idx,
		.elmt_idx = elmt_idx,
		.type = type,
		.object_id = object_id,
		.buffer_start_idx = buffer_start_idx,
	};

	uint32_t set_binding_idx = 0;
	result = _gmg_check_set_descriptor(logical_device, layout->descriptor_bindings, layout->descriptor_bindings_count, &d, &set_binding_idx);
	if (result.type < 0) return result;

	switch (_gmg.backend_type) {
#ifdef GMG_ENABLE_VULKAN
		case GmgBackendType_vulkan: {
			_GmgVkDescriptorSet* set = &render_pass->vulkan.pass_descriptor_set;
			set->has_updated = gmg_true;
			set->descriptors[set_binding_idx] = d;
			break;
		};
#endif
	}

	return GmgResult_success;
}

GmgResult gmg_render_pass_set_sampler(GmgLogicalDevice* logical_device, GmgRenderPassId id, uint16_t binding_idx, uint16_t elmt_idx, GmgSamplerId sampler_id) {
	return gmg_render_pass_set_descriptor(logical_device, id, binding_idx, elmt_idx, sampler_id.object_id, GmgDescriptorType_sampler, 0);
}

GmgResult gmg_render_pass_set_texture(GmgLogicalDevice* logical_device, GmgRenderPassId id, uint16_t binding_idx, uint16_t elmt_idx, GmgTextureId texture_id) {
	return gmg_render_pass_set_descriptor(logical_device, id, binding_idx, elmt_idx, texture_id.object_id, GmgDescriptorType_texture, 0);
}

GmgResult gmg_render_pass_set_uniform_buffer(GmgLogicalDevice* logical_device, GmgRenderPassId id, uint16_t binding_idx, uint16_t elmt_idx, GmgBufferId buffer_id, uint64_t buffer_start_idx) {
	return gmg_render_pass_set_descriptor(logical_device, id, binding_idx, elmt_idx, buffer_id.object_id, GmgDescriptorType_uniform_buffer, buffer_start_idx);
}

GmgResult gmg_render_pass_set_storage_buffer(GmgLogicalDevice* logical_device, GmgRenderPassId id, uint16_t binding_idx, uint16_t elmt_idx, GmgBufferId buffer_id, uint64_t buffer_start_idx) {
	return gmg_render_pass_set_descriptor(logical_device, id, binding_idx, elmt_idx, buffer_id.object_id, GmgDescriptorType_storage_buffer, buffer_start_idx);
}

GmgResult gmg_render_pass_set_dynamic_descriptor_offset(GmgLogicalDevice* logical_device, GmgRenderPassId id, uint16_t binding_idx, uint16_t elmt_idx, uint32_t dynamic_offset) {
	GmgRenderPass* render_pass;
	GmgResult result = gmg_render_pass_get(logical_device, id, &render_pass);
	if (result.type < 0) return result;

	GmgDescriptorSetLayout* layout;
	result = gmg_descriptor_set_layout_get(logical_device, render_pass->pass_descriptor_set_layout_id, &layout);
	if (result.type < 0) return result;

	_GmgDescriptor* descriptors;
	switch (_gmg.backend_type) {
#ifdef GMG_ENABLE_VULKAN
		case GmgBackendType_vulkan: {
			descriptors = render_pass->vulkan.pass_descriptor_set.descriptors;
			break;
		};
#endif
	}

	uint16_t dynamic_idx;
	result = _gmg_check_set_dynamic_descriptor_offset(logical_device, descriptors, layout->descriptor_bindings, layout->descriptor_bindings_count, binding_idx, elmt_idx, &dynamic_offset, &dynamic_idx);
	if (result.type < 0) return result;

	render_pass->pass_dynamic_descriptor_offsets[dynamic_idx] = dynamic_offset;
	return GmgResult_success;
}

// ===========================================
//
// Draw Command
//
// ===========================================

GmgResult gmg_render_pass_clear_draw_cmds(GmgLogicalDevice* logical_device, GmgRenderPassId id, uint32_t keep_descriptor_pools_count) {
	GmgRenderPass* render_pass;
	GmgResult result = gmg_render_pass_get(logical_device, id, &render_pass);
	if (result.type < 0) return result;

	DasStk_clear(&render_pass->draw_cmd_sort_keys);
	DasStk_clear(&render_pass->draw_cmds);
	DasStk_clear(&render_pass->push_constant_data);

	return _gmg_backend_fn_call(render_pass_clear_draw_cmds, render_pass, logical_device);
}

GmgResult gmg_render_pass_draw_cmd_start(GmgLogicalDevice* logical_device, GmgRenderPassId id, GmgMaterialId material_id, GmgVertexArrayId vertex_array_id, GmgDrawCmdBuilder* builder_in_out) {
	GmgRenderPass* render_pass;
	GmgResult result = gmg_render_pass_get(logical_device, id, &render_pass);
	if (result.type < 0) return result;

	GmgMaterial* material;
	result = gmg_material_get(logical_device, material_id, &material);
	if (result.type < 0) return result;

	if (material->descriptor_set_layout_id.raw) {
		switch (_gmg.backend_type) {
#ifdef GMG_ENABLE_VULKAN
			case GmgBackendType_vulkan: {
				if (!DasStk_push(&render_pass->vulkan.material_ids, &material_id))
					return _gmg_stacktrace_add(GmgResultType_error_out_of_memory_host);

				_GmgVkDescriptorSet* set = &render_pass->vulkan.pass_descriptor_set;
				for (uint32_t i = 0; i < set->descriptors_count; i += 1) {
					if (set->descriptors[i].object_id.raw == 0)
						return _gmg_stacktrace_add(GmgResultType_error_material_has_unset_descriptors);
				}
				break;
			};
#endif
		}
	}

	GmgMaterialSpec* material_spec;
	result = gmg_material_spec_get(logical_device, material->spec_id, &material_spec);
	if (result.type < 0) return result;

	GmgVertexArray* vertex_array;
	result = gmg_vertex_array_get(logical_device, vertex_array_id, &vertex_array);
	if (result.type < 0) return result;

	if (vertex_array->layout_id.raw != material_spec->vertex_layout_id.raw) {
		return _gmg_stacktrace_add(GmgResultType_error_incompatible_vertex_layout);
	}

	_GmgDrawCmd* cmd = &builder_in_out->draw_cmd;
	das_zero_elmt(cmd);

	if (builder_in_out->render_pass != NULL) {
		return _gmg_stacktrace_add(GmgResultType_error_draw_cmd_already_in_process);
	}

	GmgShader* shader;
	result = gmg_shader_get(logical_device, material_spec->shader_id, &shader);
	if (result.type < 0) return result;

	if (shader->pass_descriptor_set_layout_id.raw != render_pass->pass_descriptor_set_layout_id.raw) {
		return _gmg_stacktrace_add(GmgResultType_error_pass_descriptor_set_layout_mismatch);
	}

	if (shader->draw_cmd_descriptor_set_layout_id.raw) {
		GmgDescriptorSetLayout* layout;
		result = gmg_descriptor_set_layout_get(logical_device, shader->draw_cmd_descriptor_set_layout_id, &layout);
		if (result.type < 0) return result;

		GmgDescriptorSetAllocator* allocator;
		result = gmg_descriptor_set_allocator_get(logical_device, render_pass->draw_cmd_descriptor_set_allocator_id, &allocator);
		if (result.type < 0) return result;

		//
		// ensure that the shader of the material has a draw command descriptor set layout
		// that can be allocated with the descriptor set allocator that is in the render pass.
		uint32_t layout_idx;
		result = _gmg_descriptor_set_allocator_get_layout_idx(logical_device, allocator, shader->draw_cmd_descriptor_set_layout_id, &layout_idx);
		if (result.type < 0) return result;

		builder_in_out->descriptor_bindings = layout->descriptor_bindings;
		builder_in_out->descriptor_bindings_count = layout->descriptor_bindings_count;
		DasStk_clear(&builder_in_out->descriptors);
		DasStk_resize(&builder_in_out->descriptors, layout->descriptors_count, das_true);
		builder_in_out->descriptors_been_set_count = 0;
		DasStk_clear(&builder_in_out->dynamic_descriptor_offsets);
		DasStk_resize(&builder_in_out->dynamic_descriptor_offsets, layout->dynamic_descriptors_count, das_true);
	}

	builder_in_out->descriptor_set_layout_id = shader->draw_cmd_descriptor_set_layout_id;
	builder_in_out->logical_device = logical_device;
	builder_in_out->render_pass = render_pass;

	cmd->material_id = material_id;
	cmd->vertex_array_id = vertex_array_id;
	cmd->instances_count = 1;

	return GmgResult_success;
}

GmgResult gmg_draw_cmd_set_push_constants(GmgDrawCmdBuilder* builder, void* data, uint32_t offset, uint32_t size) {
	if (builder->render_pass == NULL) {
		return _gmg_stacktrace_add(GmgResultType_error_uninitialized);
	}

	if (builder->draw_cmd.push_constants_size) {
		return _gmg_stacktrace_add(GmgResultType_error_push_constants_has_already_been_set);
	}

	GmgLogicalDevice* logical_device = builder->logical_device;

	GmgMaterial* material;
	GmgResult result = gmg_material_get(logical_device, builder->draw_cmd.material_id, &material);
	if (result.type < 0) return result;

	GmgMaterialSpec* material_spec;
	result = gmg_material_spec_get(logical_device, material->spec_id, &material_spec);
	if (result.type < 0) return result;

	GmgShader* shader;
	result = gmg_shader_get(logical_device, material_spec->shader_id, &shader);
	if (result.type < 0) return result;
	if (offset + size > shader->push_constants_size) {
		return _gmg_stacktrace_add(GmgResultType_error_push_constants_out_of_bounds);
	}

	uint32_t push_constants_data_start_idx = DasStk_count(&builder->render_pass->push_constant_data);
	if (!DasStk_push_many(&builder->render_pass->push_constant_data, data, size))
		return _gmg_stacktrace_add(GmgResultType_error_out_of_memory_host);

	builder->draw_cmd.push_constants_data_start_idx = push_constants_data_start_idx;
	builder->draw_cmd.push_constants_offset = offset;
	builder->draw_cmd.push_constants_size = size;

	return GmgResult_success;
}

GmgResult gmg_draw_cmd_set_depth(GmgDrawCmdBuilder* builder, uint32_t depth) {
	if (builder->render_pass == NULL) {
		return _gmg_stacktrace_add(GmgResultType_error_uninitialized);
	}
	builder->draw_cmd.depth = depth;
	return GmgResult_success;
}

GmgResult gmg_draw_cmd_set_instances(GmgDrawCmdBuilder* builder, uint32_t instances_start_idx, uint32_t instances_count) {
	if (builder->render_pass == NULL) {
		return _gmg_stacktrace_add(GmgResultType_error_uninitialized);
	}
	builder->draw_cmd.instances_start_idx = instances_start_idx;
	builder->draw_cmd.instances_count = instances_count;
	return GmgResult_success;
}

GmgResult gmg_draw_cmd_set_descriptor(GmgDrawCmdBuilder* builder, uint16_t binding_idx, uint16_t elmt_idx, _GmgLogicalDeviceObjectId object_id, GmgDescriptorType type, uint64_t buffer_start_idx) {
	if (!builder->descriptor_set_layout_id.raw) return _gmg_stacktrace_add(GmgResultType_error_shader_does_not_have_a_draw_cmd_descriptor_set);
	if (builder->descriptor_bindings_count <= binding_idx) {
		return _gmg_stacktrace_add(GmgResultType_error_binding_idx_out_of_bounds);
	}

	_GmgDescriptor d = {
		.binding_idx = binding_idx,
		.elmt_idx = elmt_idx,
		.type = type,
		.object_id = object_id,
		.buffer_start_idx = buffer_start_idx,
	};

	uint32_t set_binding_idx;
	GmgResult result = _gmg_check_set_descriptor(builder->logical_device, builder->descriptor_bindings, builder->descriptor_bindings_count, &d, &set_binding_idx);
	if (result.type < 0) return result;

	_GmgDescriptor* dst = DasStk_get(&builder->descriptors, set_binding_idx);
	if (dst->object_id.raw) return _gmg_stacktrace_add(GmgResultType_error_draw_cmd_descriptor_is_set_twice);

	*dst = d;
	builder->descriptors_been_set_count += 1;

	return GmgResult_success;
}

GmgResult gmg_draw_cmd_set_sampler(GmgDrawCmdBuilder* builder, uint16_t binding_idx, uint16_t elmt_idx, GmgSamplerId sampler_id) {
	return gmg_draw_cmd_set_descriptor(builder, binding_idx, elmt_idx, sampler_id.object_id, GmgDescriptorType_sampler, 0);
}

GmgResult gmg_draw_cmd_set_texture(GmgDrawCmdBuilder* builder, uint16_t binding_idx, uint16_t elmt_idx, GmgTextureId texture_id) {
	return gmg_draw_cmd_set_descriptor(builder, binding_idx, elmt_idx, texture_id.object_id, GmgDescriptorType_texture, 0);
}

GmgResult gmg_draw_cmd_set_uniform_buffer(GmgDrawCmdBuilder* builder, uint16_t binding_idx, uint16_t elmt_idx, GmgBufferId buffer_id, uint64_t buffer_start_idx) {
	return gmg_draw_cmd_set_descriptor(builder, binding_idx, elmt_idx, buffer_id.object_id, GmgDescriptorType_uniform_buffer, buffer_start_idx);
}

GmgResult gmg_draw_cmd_set_storage_buffer(GmgDrawCmdBuilder* builder, uint16_t binding_idx, uint16_t elmt_idx, GmgBufferId buffer_id, uint64_t buffer_start_idx) {
	return gmg_draw_cmd_set_descriptor(builder, binding_idx, elmt_idx, buffer_id.object_id, GmgDescriptorType_storage_buffer, buffer_start_idx);
}

GmgResult gmg_draw_cmd_set_dynamic_descriptor_offset(GmgDrawCmdBuilder* builder, uint16_t binding_idx, uint16_t elmt_idx, uint32_t dynamic_offset) {
	uint16_t dynamic_idx;
	GmgResult result = _gmg_check_set_dynamic_descriptor_offset(builder->logical_device, DasStk_data(&builder->descriptors), builder->descriptor_bindings, builder->descriptor_bindings_count, binding_idx, elmt_idx, &dynamic_offset, &dynamic_idx);
	if (result.type < 0) return result;

	*DasStk_get(&builder->dynamic_descriptor_offsets, dynamic_idx) = dynamic_offset;
	return GmgResult_success;
}

static GmgResult _gmg_draw_cmd_queue(GmgDrawCmdBuilder* builder, GmgRenderPass* render_pass, _GmgDrawCmd* cmd) {
	GmgDescriptorSetLayoutId descriptor_set_layout_id = builder->descriptor_set_layout_id;
	if (descriptor_set_layout_id.raw) {
		if (builder->descriptors_been_set_count != DasStk_count(&builder->descriptors)) {
			return _gmg_stacktrace_add(GmgResultType_error_draw_cmd_has_unset_descriptors);
		}

		GmgResult result = _gmg_backend_fn_call(render_pass_queue_descriptors, builder, render_pass, cmd);
		if (result.type < 0) return result;
	}

	uint64_t cmd_idx = DasStk_count(&render_pass->draw_cmds);
	uint64_t sort_key = cmd_idx;
	switch (render_pass->draw_order) {
		case GmgDrawOrder_material_spec_cmd_idx: {
			GmgMaterial* material;
			GmgResult result = gmg_material_get(builder->logical_device, cmd->material_id, &material);
			if (result.type < 0) return result;

			sort_key |= (uint64_t)material->spec_id.raw << 32;
			break;
		};
		case GmgDrawOrder_depth_cmd_idx:
			sort_key |= (uint64_t)cmd->depth << 32;
			break;
		case GmgDrawOrder_cmd_idx:
			break;
	}

	if (!DasStk_push(&render_pass->draw_cmd_sort_keys, &sort_key)) {
		return _gmg_stacktrace_add(GmgResultType_error_out_of_memory_host);
	}

	if (!DasStk_push(&render_pass->draw_cmds, cmd)) {
		return _gmg_stacktrace_add(GmgResultType_error_out_of_memory_host);
	}
	return GmgResult_success;
}

GmgResult gmg_draw_cmd_queue_vertexed(GmgDrawCmdBuilder* builder, uint32_t vertices_start_idx, uint32_t vertices_count) {
	GmgRenderPass* render_pass = builder->render_pass;
	_GmgDrawCmd* cmd = &builder->draw_cmd;
	if (render_pass == NULL) {
		return _gmg_stacktrace_add(GmgResultType_error_uninitialized);
	}
	GmgLogicalDevice* logical_device = builder->logical_device;

	GmgVertexArray* vertex_array;
	GmgResult result = gmg_vertex_array_get(logical_device, builder->draw_cmd.vertex_array_id, &vertex_array);
	if (result.type < 0) return result;

	if (vertex_array->index_buffer_id.raw != 0) {
		return _gmg_stacktrace_add(GmgResultType_error_vertex_array_has_index_buffer);
	}

	cmd->vertexed.vertices_start_idx = vertices_start_idx;
	cmd->vertexed.vertices_count = vertices_count;

	builder->render_pass = NULL;
	return _gmg_draw_cmd_queue(builder, render_pass, cmd);
}

GmgResult gmg_draw_cmd_queue_indexed(GmgDrawCmdBuilder* builder, uint32_t vertices_start_idx, uint32_t indices_start_idx, uint32_t indices_count) {
	GmgRenderPass* render_pass = builder->render_pass;
	_GmgDrawCmd* cmd = &builder->draw_cmd;
	if (render_pass == NULL) {
		return _gmg_stacktrace_add(GmgResultType_error_uninitialized);
	}
	GmgLogicalDevice* logical_device = builder->logical_device;

	GmgVertexArray* vertex_array;
	GmgResult result = gmg_vertex_array_get(logical_device, builder->draw_cmd.vertex_array_id, &vertex_array);
	if (result.type < 0) return result;

	if (vertex_array->index_buffer_id.raw == 0) {
		return _gmg_stacktrace_add(GmgResultType_error_vertex_array_does_not_have_index_buffer);
	}

	cmd->indexed.vertices_start_idx = vertices_start_idx;
	cmd->indexed.indices_start_idx = indices_start_idx;
	cmd->indexed.indices_count = indices_count;

	builder->render_pass = NULL;
	return _gmg_draw_cmd_queue(builder, render_pass, cmd);
}

// ==========================================================
//
//
// Frame Buffer
//
//
// ==========================================================

GmgResult gmg_frame_buffer_init(GmgLogicalDevice* logical_device, GmgFrameBufferCreateArgs* create_args, GmgFrameBufferId* id_out) {
	GmgFrameBuffer* frame_buffer;

	GmgResult result = _gmg_logical_device_object_init(logical_device, create_args, (_GmgLogicalDeviceObjectId*)id_out, _GmgBackendImplFnType_frame_buffer_init, (_GmgLogicalDeviceObject**)&frame_buffer);
	if (result.type < 0) return result;

	GmgTextureId* attachments = das_alloc_array(GmgTextureId, GMG_TODO_INTERNAL_ALLOCATOR, create_args->attachments_count);
	if (attachments == NULL) return _gmg_stacktrace_add(GmgResultType_error_out_of_memory_host);
	das_copy_elmts(attachments, create_args->attachments, create_args->attachments_count);

	frame_buffer->render_pass_layout_id = create_args->render_pass_layout_id;
	frame_buffer->width = create_args->width;
	frame_buffer->height = create_args->height;
	frame_buffer->layers = create_args->layers;
	frame_buffer->attachments = attachments;
	frame_buffer->attachments_count = create_args->attachments_count;
	return GmgResult_success;
}

GmgResult gmg_frame_buffer_deinit(GmgLogicalDevice* logical_device, GmgFrameBufferId id) {
	return _gmg_logical_device_object_deinit(logical_device, id.object_id, _GmgBackendImplFnType_frame_buffer_deinit);
}

GmgResult gmg_frame_buffer_get(GmgLogicalDevice* logical_device, GmgFrameBufferId id, GmgFrameBuffer** out) {
	return _gmg_logical_device_object_get(logical_device, id.object_id, (_GmgLogicalDeviceObject**)out);
}

// ==========================================================
//
//
// Swapchain
//
//
// ==========================================================

static GmgResult _gmg_swapchain_deinit_objects(GmgSwapchain* swapchain, GmgLogicalDevice* logical_device) {
	GmgResult result;
	for (uint32_t i = 0; i < swapchain->textures_count; i += 1) {
		result = gmg_texture_deinit(logical_device, swapchain->texture_ids[i]);
		if (result.type < 0) return result;
	}

	//
	// we do not deallocate these arrays here as they are realloced in the internal
	// initialize function that is called when resizing.
	//

	return GmgResult_success;
}

GmgResult gmg_swapchain_init(GmgLogicalDevice* logical_device, GmgSwapchainCreateArgs* create_args, GmgSwapchainId* id_out) {
	GmgSwapchain* swapchain;
	GmgResult result = _gmg_logical_device_object_init(logical_device, create_args, (_GmgLogicalDeviceObjectId*)id_out, _GmgBackendImplFnType_none, (_GmgLogicalDeviceObject**)&swapchain);
	if (result.type < 0) return result;

	swapchain->surface = create_args->surface;
	swapchain->array_layers_count = create_args->array_layers_count;
	swapchain->format = create_args->format;
	swapchain->min_textures_count = create_args->min_textures_count;

	return GmgResult_success;
}

GmgResult gmg_swapchain_deinit(GmgLogicalDevice* logical_device, GmgSwapchainId id) {
	GmgSwapchain* swapchain;
	GmgResult result = gmg_swapchain_get(logical_device, id, &swapchain);
	if (result.type < 0) return result;

	result = _gmg_swapchain_deinit_objects(swapchain, logical_device);
	if (result.type < 0) return result;

	result = _gmg_backend_fn_call(swapchain_deinit, swapchain, logical_device);
	if (result.type < 0) return result;

	DasPool_dealloc(_GmgLogicalDeviceObjectId, &logical_device->object_pool, id.object_id);

	return GmgResult_success;
}

GmgResult gmg_swapchain_get(GmgLogicalDevice* logical_device, GmgSwapchainId id, GmgSwapchain** out) {
	return _gmg_logical_device_object_get(logical_device, id.object_id, (_GmgLogicalDeviceObject**)out);
}

GmgResult gmg_swapchain_resize(GmgLogicalDevice* logical_device, GmgSwapchainId id, uint32_t width, uint32_t height, GmgBool vsync, GmgBool fifo) {
	GmgSwapchain* swapchain;
	GmgResult result = gmg_swapchain_get(logical_device, id, &swapchain);
	if (result.type < 0) return result;

	result = _gmg_swapchain_deinit_objects(swapchain, logical_device);
	if (result.type < 0) return result;

	if (
		swapchain->width == width &&
		swapchain->height == height &&
		swapchain->vsync == vsync &&
		swapchain->fifo == fifo
	) return GmgResult_success_(GmgResultType_nothing_changed);

	swapchain->width = width;
	swapchain->height = height;
	swapchain->vsync = vsync;
	swapchain->fifo = fifo;

	return _gmg_backend_fn_call(swapchain_reinit, swapchain, logical_device);
}

GmgResult gmg_swapchain_get_next_texture_idx(GmgLogicalDevice* logical_device, GmgSwapchainId id, uint32_t* next_texture_idx_out) {
	GmgSwapchain* swapchain;
	GmgResult result = gmg_swapchain_get(logical_device, id, &swapchain);
	if (result.type < 0) return result;

	result = _gmg_backend_fn_call(swapchain_get_next_texture_idx, swapchain, logical_device, next_texture_idx_out);
	if (result.type < 0) return result;

	return GmgResult_success;
}

GmgResult gmg_swapchain_present(GmgLogicalDevice* logical_device, GmgSwapchainId* swapchain_ids, uint32_t swapchains_count) {
	GmgResult result = _gmg_backend_fn_call(swapchain_present, logical_device, swapchain_ids, swapchains_count);
	if (result.type < 0) return result;

	return GmgResult_success;
}

// ==========================================================
//
//
// Vulkan Utilities - TODO see what extensions and layers we can enable through flags that get enabled on initialization
//
//
// ==========================================================

GmgResult _gmg_vulkan_instance_layers_get(VkLayerProperties** properties_out, uint32_t* properties_count_out) {
	VkLayerProperties* properties;
	uint32_t properties_count;

	VkResult vk_result = vkEnumerateInstanceLayerProperties(&properties_count, NULL);
	if (vk_result) return _gmg_vulkan_convert_from_result(vk_result);

	properties = das_alloc_array(VkLayerProperties, GMG_TODO_INTERNAL_ALLOCATOR, properties_count);
	if (properties == NULL) return _gmg_stacktrace_add(GmgResultType_error_out_of_memory_host);

	vk_result = vkEnumerateInstanceLayerProperties(&properties_count, properties);
	if (vk_result) return _gmg_vulkan_convert_from_result(vk_result);

	*properties_out = properties;
	*properties_count_out = properties_count;
	return GmgResult_success;
}

GmgResult _gmg_vulkan_instance_extensions_get(VkExtensionProperties** properties_out, uint32_t* properties_count_out) {
	VkExtensionProperties* properties;
	uint32_t properties_count;

	VkResult vk_result = vkEnumerateInstanceExtensionProperties(NULL, &properties_count, NULL);
	if (vk_result) return _gmg_vulkan_convert_from_result(vk_result);

	properties = das_alloc_array(VkExtensionProperties, GMG_TODO_INTERNAL_ALLOCATOR, properties_count);
	if (properties == NULL) return _gmg_stacktrace_add(GmgResultType_error_out_of_memory_host);

	vk_result = vkEnumerateInstanceExtensionProperties(NULL, &properties_count, properties);
	if (vk_result) return _gmg_vulkan_convert_from_result(vk_result);

	*properties_out = properties;
	*properties_count_out = properties_count;
	return GmgResult_success;
}

GmgResult _gmg_vulkan_logical_device_layers_get(uint32_t physical_device_idx, VkLayerProperties** properties_out, uint32_t* properties_count_out) {
	GmgPhysicalDevice* physical_device;
	GmgResult result = gmg_physical_device_get(physical_device_idx, &physical_device);
	if (result.type < 0) return result;

	VkLayerProperties* properties;
	uint32_t properties_count;

	VkResult vk_result = vkEnumerateDeviceLayerProperties(physical_device->vulkan.handle, &properties_count, NULL);
	if (vk_result) return _gmg_vulkan_convert_from_result(vk_result);

	properties = das_alloc_array(VkLayerProperties, GMG_TODO_INTERNAL_ALLOCATOR, properties_count);
	if (properties == NULL) return _gmg_stacktrace_add(GmgResultType_error_out_of_memory_host);

	vk_result = vkEnumerateDeviceLayerProperties(physical_device->vulkan.handle, &properties_count, properties);
	if (vk_result) return _gmg_vulkan_convert_from_result(vk_result);

	*properties_out = properties;
	*properties_count_out = properties_count;
	return GmgResult_success;
}

GmgResult _gmg_vulkan_logical_device_extensions_get(uint32_t physical_device_idx, VkExtensionProperties** properties_out, uint32_t* properties_count_out) {
	GmgPhysicalDevice* physical_device;
	GmgResult result = gmg_physical_device_get(physical_device_idx, &physical_device);
	if (result.type < 0) return result;

	VkExtensionProperties* properties;
	uint32_t properties_count;

	VkResult vk_result = vkEnumerateDeviceExtensionProperties(physical_device->vulkan.handle, NULL, &properties_count, NULL);
	if (vk_result) return _gmg_vulkan_convert_from_result(vk_result);

	properties = das_alloc_array(VkExtensionProperties, GMG_TODO_INTERNAL_ALLOCATOR, properties_count);
	if (properties == NULL) return _gmg_stacktrace_add(GmgResultType_error_out_of_memory_host);

	vk_result = vkEnumerateDeviceExtensionProperties(physical_device->vulkan.handle, NULL, &properties_count, properties);
	if (vk_result) return _gmg_vulkan_convert_from_result(vk_result);

	*properties_out = properties;
	*properties_count_out = properties_count;
	return GmgResult_success;
}

void _gmg_vulkan_print_layers(VkLayerProperties* properties, uint32_t properties_count) {
	printf("##### Layers (count: %u) #####\n", properties_count);

	for (uint32_t i = 0; i < properties_count; i += 1) {
		VkLayerProperties* props = &properties[i];
		printf("\t### Layer: %u ###\n", i);
		printf("\tname = %s\n", props->layerName);
		printf("\tspec_version = %u.%u.%u\n", VK_VERSION_MAJOR(props->specVersion), VK_VERSION_MINOR(props->specVersion), VK_VERSION_PATCH(props->specVersion));
		printf("\timplementation_version = %u.%u.%u\n", VK_VERSION_MAJOR(props->implementationVersion), VK_VERSION_MINOR(props->implementationVersion), VK_VERSION_PATCH(props->implementationVersion));
		printf("\tdescription = %s\n", props->description);
		printf("\n");
	}
}

void _gmg_vulkan_print_extensions(VkExtensionProperties* properties, uint32_t properties_count) {
	printf("##### Extensions (count: %u) #####\n", properties_count);

	for (uint32_t i = 0; i < properties_count; i += 1) {
		VkExtensionProperties* props = &properties[i];
		printf("\t### Extension: %u ###\n", i);
		printf("\tname = %s\n", props->extensionName);
		printf("\tspec_version = %u.%u.%u\n", VK_VERSION_MAJOR(props->specVersion), VK_VERSION_MINOR(props->specVersion), VK_VERSION_PATCH(props->specVersion));
		printf("\n");
	}
}

// ==========================================================
//
//
// Gmg
//
//
// ==========================================================

GmgResult gmg_init(GmgSetup* setup) {
	das_assert(setup->callback_impl_fn, "GmgSetup.callback_impl_fn must be a pointer to a function to handle various events");

	das_zero_elmt(&_gmg);
	_gmg.backend_type = setup->backend_type;
	_gmg.callback_impl_fn = setup->callback_impl_fn;
	_gmg.callback_userdata = setup->callback_userdata;

	DasError das_error = DasPool_init(GmgVertexLayoutId, &_gmg.vertex_layout_pool, 32 * 1024, 1024);
	if (das_error) return GmgResult_from_DasError(das_error);

	das_error = DasPool_init(GmgStacktraceId, &_gmg.stacktrace_pool, 32 * 1024, 1024);
	if (das_error) return GmgResult_from_DasError(das_error);

	switch (setup->backend_type) {
#ifdef GMG_ENABLE_VULKAN
		case GmgBackendType_vulkan: {
			_gmg.backend_impl_fns = _gmg_vulkan_backend_impl_fns;

			break;
		};
#endif
	}

	GmgResult result = _gmg_backend_fn_call(init, setup);
	if (result.type < 0) return result;

	return GmgResult_success;
}

GmgResult gmg_vulkan_instance(VkInstance* out) {
	if (_gmg.backend_type != GmgBackendType_vulkan)
		return _gmg_stacktrace_add(GmgResultType_error_incompatible_backend_type);

	if (_gmg.vulkan.instance == VK_NULL_HANDLE)
		return _gmg_stacktrace_add(GmgResultType_error_uninitialized);

	*out = _gmg.vulkan.instance;
	return GmgResult_success;
}

GmgResult gmg_stacktrace_get(GmgResult result, GmgStacktrace* out) {
	das_assert(result.type < 0, "cannot get an error from a result that is not an error");
	GmgStacktrace* error;

	if (result.stacktrace_id.raw == 0) return _gmg_stacktrace_add(GmgResultType_error_unexpected_null_id);
	if (!DasPool_is_id_valid(GmgStacktraceId, &_gmg.stacktrace_pool, result.stacktrace_id))
		return _gmg_stacktrace_add(GmgResultType_error_object_use_after_free);

	*out = *(GmgStacktrace*)DasPool_id_to_ptr(GmgStacktraceId, &_gmg.stacktrace_pool, result.stacktrace_id);
	return GmgResult_success;
}

