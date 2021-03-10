#define WINDOW_WIDTH 1280
#define WINDOW_HEIGHT 720

static SDL_Window* window;

typedef struct App App;
struct App {
	GmgLogicalDevice* logical_device;
	GmgVertexLayoutId vertex_layout_id;
	GmgBufferId vertex_buffer_id;
	GmgBufferId index_buffer_id;
	GmgVertexArrayId vertex_array_id;
	GmgBufferId uniform_buffer_id;
	GmgSurface surface;
	GmgTextureFormat surface_format;
	GmgRenderPassLayoutId render_pass_layout_id;
	GmgRenderPassId render_pass_id;
	GmgSwapchainId swapchain_id;
	GmgFrameBufferId* swapchain_frame_buffer_ids;
	GmgShaderModuleId vertex_shader_module_id;
	GmgShaderModuleId fragment_shader_module_id;
	GmgShaderId shader_id;
	GmgMaterialSpecId material_spec_id;
	GmgMaterialId material_id;
	GmgTextureId depth_stencil_texture_id;
};

static App app;

void ortho_projection(float out[4][4], float left, float right, float bottom, float top, float near, float far) {
	float diff_x = right - left;
	float diff_y = top - bottom;
	float diff_z = far - near;

	float tx = -((right + left) / diff_x);
	float ty = -((top + bottom) / diff_y);
	float tz = -((far + near) / diff_z);

	out[0][0] = 2.0 / diff_x;
	out[0][1] = 0.0;
	out[0][2] = 0.0;
	out[0][3] = 0.0;

	out[1][0] = 0.0;
	out[1][1] = 2.0 / diff_y;
	out[1][2] = 0.0;
	out[1][3] = 0.0;

	out[2][0] = 0.0;
	out[2][1] = 0.0;
	out[2][2] = -2.0 / diff_z;
	out[2][3] = 0.0;

	out[3][0] = tx;
	out[3][1] = ty;
	out[3][2] = tz;
	out[3][3] = 1.0;
}

void perspective_projection(float out[4][4], float fovy, float aspect_ratio, float z_near, float z_far) {
	das_assert(aspect_ratio != 0.0, "aspect_ratio cannot be 0.0");
	das_assert(z_far != z_near, "z_near and z_far cannot be equal");

	float tan_half_fovy = tanf(fovy / 2.0);
	float a = 1.0 / tan_half_fovy;

	memset(out, 0, sizeof(float) * 16);
	out[0][0] = a / aspect_ratio;
	out[1][1] = a;
	out[2][2] = -((z_far + z_near) / (z_far - z_near));
	out[2][3] = -1.0;
	out[3][2] = -((2.0 * z_far * z_near) / (z_far - z_near));
}

int app_file_read_all(char* path, DasStk(uint8_t)* bytes_out) {
    FILE* file = fopen(path, "rb");
    if (file == NULL) { return errno; }

	// seek the cursor to the end
    if (fseek(file, 0, SEEK_END) == -1) { goto ERR; }

	// get the location of the position we have seeked the cursor to.
	// this will tell us the file size
    long int file_size = ftell(file);
    if (file_size == -1) { goto ERR; }

	// seek the cursor back to the beginning
    if (fseek(file, 0, SEEK_SET) == -1) { goto ERR; }

	//
	// ensure the buffer has enough capacity
    if (DasStk_cap(bytes_out) < file_size) {
        DasStk_resize_cap(bytes_out, file_size);
		/* todo: maybe make das optionally return NULL instead of using the OOMH
        if (!DasStk_resize_cap(bytes_out, file_size)) {
			return ENOMEM;
		}
		*/
    }

	// read the file in
    size_t read_size = fread((char*)DasStk_data(bytes_out), 1, file_size, file);
    if (read_size != file_size) { goto ERR; }

    if (fclose(file) != 0) { return errno; }

    DasStk_set_count(bytes_out, read_size);
    return 0;

ERR: {}
	int err = errno;
    fclose(file);
    return err;
}

GmgResult app_gmg_callback_impl_fn(GmgLogicalDevice* logical_device, GmgCallbackFn fn, GmgCallbackArgs* args, void* userdata) {
	switch (fn) {
		case GmgCallbackFn_execute_job:
			//
			// this should really queue the job up on a thread pool
			// that will execute these jobs in parallel.
			//
			// but you can also run this single threaded like we are here
			if (args->execute_job.start_fn(args->execute_job.args) != 0) {
				return GmgResult_error_;
			}
			break;
		case GmgCallbackFn_wait_for_jobs:
			//
			// this should then wait for all jobs in the thread pool to complete
			//
			break;
	}

	return GmgResult_success;
}

void app_create_render_pass();
void app_render_state(GmgRenderState* render_state);

void app_common_init() {
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS)) {
		printf("unable to initialize SDL: %s\n", SDL_GetError());
		exit(1);
	}

	window = SDL_CreateWindow(APP_NAME, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_VULKAN | SDL_WINDOW_SHOWN);
	if (window == NULL) {
		printf("unable to create window: %s\n", SDL_GetError());
		exit(1);
	}

	SDL_SysWMinfo sdl_wm_info;
	SDL_VERSION(&sdl_wm_info.version)
	if (!SDL_GetWindowWMInfo(window, &sdl_wm_info)) {
		printf("unable to get the window manager info: %s\n", SDL_GetError());
		exit(1);
	}

	GmgDisplayManagerType gmg_display_manager_type;
	switch (sdl_wm_info.subsystem) {
		case SDL_SYSWM_WINDOWS:
			gmg_display_manager_type = GmgDisplayManagerType_windows;
			break;
		case SDL_SYSWM_X11:
			gmg_display_manager_type = GmgDisplayManagerType_xlib;
			break;
		case SDL_SYSWM_COCOA:
			gmg_display_manager_type = GmgDisplayManagerType_macos;
			break;
		case SDL_SYSWM_UIKIT:
			gmg_display_manager_type = GmgDisplayManagerType_ios;
			break;
		case SDL_SYSWM_WAYLAND:
			gmg_display_manager_type = GmgDisplayManagerType_wayland;
			break;
		case SDL_SYSWM_ANDROID:
			gmg_display_manager_type = GmgDisplayManagerType_android;
			break;
		default: das_abort("unhandle sdl2 wm type %u\n", sdl_wm_info.subsystem);
			break;
	}

	GmgSetup gmg_setup = {
		.backend_type = GmgBackendType_vulkan,
		.application_name = APP_NAME,
		.application_version = 1,
		.engine_name = "ReelEngine",
		.engine_version = 1,
		.callback_impl_fn = app_gmg_callback_impl_fn,
		.display_manager_type = gmg_display_manager_type,
		.debug_backend = gmg_true,
#ifdef GMG_ENABLE_VULKAN
		.vulkan.api_version = VK_MAKE_VERSION(1, 0, 0),
#endif
	};

	gmg_assert_result(gmg_init(&gmg_setup));

	uint32_t physical_device_idx = 0;
	{

		GmgPhysicalDeviceFeatureFlags feature_flags;
		gmg_assert_result(gmg_physical_device_features(physical_device_idx, &feature_flags));

		GmgLogicalDeviceCreateArgs logical_device_create_args = {0};
		logical_device_create_args.feature_flags = feature_flags;
		logical_device_create_args.object_pool_reserve_cap = 0;
		logical_device_create_args.object_pool_grow_count = 0;

		gmg_assert_result(gmg_logical_device_init(physical_device_idx, &logical_device_create_args, &app.logical_device));
	}

	{
		static GmgVertexBindingInfo binding = {
			.attribs = app_vertex_layout_attrib_infos,
			.attribs_count = sizeof(app_vertex_layout_attrib_infos) / sizeof(*app_vertex_layout_attrib_infos),
			.size = sizeof(AppVertex),
		};

		static GmgVertexLayout vl = {
			.bindings_count = 1,
			.bindings = &binding,
		};

		gmg_assert_result(gmg_vertex_layout_register(&vl, &app.vertex_layout_id));
	}

	{
		GmgBufferCreateArgs vertex_buffer_create_args = {
			.flags = GmgBufferFlags_used_for_graphics,
			.type = GmgBufferType_vertex,
			.vertex.layout_id = app.vertex_layout_id,
			.vertex.binding_idx = 0,
			.elmts_count = APP_VERTICES_COUNT,
		};
		gmg_assert_result(gmg_buffer_init(app.logical_device, &vertex_buffer_create_args, &app.vertex_buffer_id));
	}

	{
		GmgBufferCreateArgs index_buffer_create_args = {
			.flags = GmgBufferFlags_used_for_graphics,
			.type = GmgBufferType_index,
			.index.type = GmgIndexType_u32,
			.elmts_count = APP_INDICES_COUNT,
		};
		gmg_assert_result(gmg_buffer_init(app.logical_device, &index_buffer_create_args, &app.index_buffer_id));
	}

	{
		GmgVertexArrayCreateArgs vertex_array_create_args = {
			.layout_id = app.vertex_layout_id,
			.vertex_buffer_ids = &app.vertex_buffer_id,
			.vertex_buffers_count = 1,
			.index_buffer_id = app.index_buffer_id,
		};
		gmg_assert_result(gmg_vertex_array_init(app.logical_device, &vertex_array_create_args, &app.vertex_array_id));
	}

	{
		GmgBufferCreateArgs uniform_buffer_create_args = {
			.flags = GmgBufferFlags_used_for_graphics,
			.type = GmgBufferType_uniform,
			.uniform.elmt_size = sizeof(AppUBO),
			.elmts_count = 1,
		};
		gmg_assert_result(gmg_buffer_init(app.logical_device, &uniform_buffer_create_args, &app.uniform_buffer_id));
	}

	{
		app.surface.type = GmgBackendType_vulkan;

		VkInstance vk_instance;
		gmg_assert_result(gmg_vulkan_instance(&vk_instance));
		if (!SDL_Vulkan_CreateSurface(window, vk_instance, &app.surface.vulkan)) {
			printf("unable get the vulkan surface: %s\n", SDL_GetError());
			exit(1);
		}

		gmg_assert_result(gmg_physical_device_surface_texture_format(physical_device_idx, app.surface, &app.surface_format));
	}

	app_create_render_pass();

	{
		int drawable_width, drawable_height;
		SDL_Vulkan_GetDrawableSize(window, &drawable_width, &drawable_height);

		GmgSwapchainCreateArgs swapchain_create_args = {
			.surface = app.surface,
			.width = drawable_width,
			.height = drawable_height,
			.array_layers_count = 1,
			.min_textures_count = 2,
			.format = app.surface_format,
		};

		gmg_assert_result(gmg_swapchain_init(app.logical_device, &swapchain_create_args, &app.swapchain_id));

		GmgSwapchain* swapchain;
		gmg_assert_result(gmg_swapchain_get(app.logical_device, app.swapchain_id, &swapchain));

		if (APP_DEPTH_STENCIL_FORMAT != GmgTextureFormat_none) {
			GmgTextureCreateArgs depth_stencil_texture_create_args = {
				.type = GmgTextureType_2d,
				.format = APP_DEPTH_STENCIL_FORMAT,
				.flags = GmgTextureFlags_depth_stencil_attachment,
				.samples = GmgSampleCount_1,
			};

			gmg_assert_result(gmg_texture_init(app.logical_device, &depth_stencil_texture_create_args, &app.depth_stencil_texture_id));
			gmg_assert_result(gmg_texture_resize(app.logical_device, app.depth_stencil_texture_id, drawable_width, drawable_height, 1, 1, 1));
		}

		app.swapchain_frame_buffer_ids = das_alloc_array(GmgFrameBufferId, DasAlctor_default, swapchain->textures_count);
		das_assert(app.swapchain_frame_buffer_ids, "out of memory");

		uint32_t attachments_count = APP_DEPTH_STENCIL_FORMAT == GmgTextureFormat_none ? 1 : 2;
		static GmgTextureId texture_ids[2];
		texture_ids[1] = app.depth_stencil_texture_id;
		for (uint32_t i = 0; i < swapchain->textures_count; i += 1) {
			texture_ids[0] = swapchain->texture_ids[i];
			GmgFrameBufferCreateArgs frame_buffer_create_args = {
				.attachments = texture_ids,
				.attachments_count = attachments_count,
				.layers = 1,
				.render_pass_layout_id = app.render_pass_layout_id,
				.width = swapchain->width,
				.height = swapchain->height,
			};

			gmg_assert_result(gmg_frame_buffer_init(app.logical_device, &frame_buffer_create_args, &app.swapchain_frame_buffer_ids[i]));
		}
	}

	DasStk(uint8_t) vertex_shader_spir_v = {0};
	DasStk(uint8_t) fragment_shader_spir_v = {0};
	{
		if (app_file_read_all(APP_VERTEX_SHADER_SRC, &vertex_shader_spir_v)) {
			printf("unable to read file at %s: %s\n", APP_VERTEX_SHADER_SRC, strerror(errno));
			exit(1);
		}
		if (app_file_read_all(APP_FRAGMENT_SHADER_SRC, &fragment_shader_spir_v)) {
			printf("unable to read file at %s: %s\n", APP_FRAGMENT_SHADER_SRC, strerror(errno));
			exit(1);
		}
	}

	{
		GmgShaderModuleCreateArgs vertex_shader_module_create_args = {
			.format = GmgShaderFormat_spir_v,
			.code = DasStk_data(&vertex_shader_spir_v),
			.code_size = DasStk_count(&vertex_shader_spir_v),
		};

		gmg_assert_result(gmg_shader_module_init(app.logical_device, &vertex_shader_module_create_args, &app.vertex_shader_module_id));

		GmgShaderModuleCreateArgs fragment_shader_module_create_args = {
			.format = GmgShaderFormat_spir_v,
			.code = DasStk_data(&fragment_shader_spir_v),
			.code_size = DasStk_count(&fragment_shader_spir_v),
		};

		gmg_assert_result(gmg_shader_module_init(app.logical_device, &fragment_shader_module_create_args, &app.fragment_shader_module_id));
	}

	{
		static GmgDescriptorBinding descriptor_bindings[] = {
			{
				.type = GmgDescriptorType_uniform_buffer,
				.shader_stage_flags = GmgShaderStageFlags_vertex,
				.binding_idx = 0,
				.count = 1,
				.size = sizeof(AppUBO),
			},
			{
				.type = GmgDescriptorType_sampler,
				.shader_stage_flags = GmgShaderStageFlags_fragment,
				.binding_idx = 1,
				.count = 1,
			},
			{
				.type = GmgDescriptorType_texture,
				.shader_stage_flags = GmgShaderStageFlags_fragment,
				.binding_idx = 2,
				.count = 1,
			}
		};

		GmgShaderCreateArgs shader_create_args = {0};
		shader_create_args.descriptor_bindings = descriptor_bindings;
		shader_create_args.descriptor_bindings_count = sizeof(descriptor_bindings) / sizeof(*descriptor_bindings);
		shader_create_args.type = GmgShaderType_graphics;
		shader_create_args.max_materials_count = 8;

		shader_create_args.stages.graphics.vertex = (GmgShaderStage) {
			.format = GmgShaderFormat_spir_v,
			.spir_v = {
				.module_id = app.vertex_shader_module_id,
				.entry_point_name = "main",
			},
		};

		shader_create_args.stages.graphics.fragment = (GmgShaderStage) {
			.format = GmgShaderFormat_spir_v,
			.spir_v = {
				.module_id = app.fragment_shader_module_id,
				.entry_point_name = "main",
			},
		};

		gmg_assert_result(gmg_shader_init(app.logical_device, &shader_create_args, &app.shader_id));
	}

	{

		GmgRenderState render_state = {0};
		app_render_state(&render_state);

		GmgMaterialSpecCreateArgs material_spec_create_args =  {
			.shader_id = app.shader_id,
			.render_pass_id = app.render_pass_id,
			.render_state = &render_state,
			.vertex_layout_id = app.vertex_layout_id,
			.cache_id = GmgMaterialSpecCacheId_null,
		};

		gmg_assert_result(gmg_material_spec_init(app.logical_device, &material_spec_create_args, &app.material_spec_id));
	}

	{
		GmgMaterialCreateArgs material_create_args = {
			.spec_id = app.material_spec_id,
		};

		gmg_assert_result(gmg_material_init(app.logical_device, &material_create_args, &app.material_id));
		gmg_assert_result(gmg_material_set_uniform_buffer(app.logical_device, app.material_id, 0, 0, app.uniform_buffer_id, 0));
	}
}


