#include "deps/common.h"

#define APP_NAME "Textured Cube Example"
#define APP_VERTEX_SHADER_SRC "bin/textured_cube.vert.spv"
#define APP_FRAGMENT_SHADER_SRC "bin/textured_cube.frag.spv"
#define APP_VERTICES_COUNT 24
#define APP_INDICES_COUNT 36
#define APP_DEPTH_STENCIL_FORMAT GmgTextureFormat_d32
#define APP_TEXTURE_PATH "texture.jpg"

typedef struct AppColor AppColor;
struct AppColor {
	uint8_t r;
	uint8_t g;
	uint8_t b;
	uint8_t a;
};

#define AppColor_init(r, g, b, a) ((AppColor) { r, g, b, a })

typedef struct AppVertex AppVertex;
struct AppVertex {
	AppVec3 pos;
	AppVec2 tex_coords;
};

static GmgVertexAttribInfo app_vertex_layout_attrib_infos[] = {
	{
		.location_idx = 0,
		.elmt_type = GmgVertexElmtType_f32,
		.vector_type = GmgVertexVectorType_3,
	},
	{
		.location_idx = 1,
		.elmt_type = GmgVertexElmtType_f32,
		.vector_type = GmgVertexVectorType_2,
	},
};

typedef struct AppUBO AppUBO;
struct AppUBO {
	float mvp[4][4];
};

#include "deps/common.c"

void app_create_render_pass() {
	{
		typedef struct GmgAttachmentLayout GmgAttachmentLayout;
		GmgAttachmentLayout attachment_layouts[] = {
			{
				.format = app.surface_format,
				.samples_count = 1,
				.present = gmg_true,
			},
			{
				.format = GmgTextureFormat_d32,
				.samples_count = 1,
				.present = gmg_false,
			},
		};

		GmgRenderPassLayoutCreateArgs rpl_create_args = {
			.attachments = attachment_layouts,
			.attachments_count = sizeof(attachment_layouts) / sizeof(*attachment_layouts),
		};

		gmg_assert_result(gmg_render_pass_layout_init(app.logical_device, &rpl_create_args, &app.render_pass_layout_id));
	}

	{
		GmgAttachmentInfo render_pass_attachments[] = {
			{
				.load_op = GmgAttachmentLoadOp_clear,
				.store_op = GmgAttachmentStoreOp_preserve,
				.stencil_load_op = GmgAttachmentLoadOp_uninitialized,
				.stencil_store_op = GmgAttachmentStoreOp_discard,
			},
			{
				.load_op = GmgAttachmentLoadOp_clear,
				.store_op = GmgAttachmentStoreOp_preserve,
				.stencil_load_op = GmgAttachmentLoadOp_clear,
				.stencil_store_op = GmgAttachmentStoreOp_discard,
			},
		};

		GmgClearValue clear_values[] = {
			{ .color.float32 = { 0.f, 0.f, 0.f, 1.f } },
			{ .depth = 1.0, .stencil = 0 },
		};

		GmgRenderPassCreateArgs render_pass_create_args = {
			.layout_id = app.render_pass_layout_id,
			.attachments = render_pass_attachments,
			.attachments_count = sizeof(render_pass_attachments) / sizeof(*render_pass_attachments),
			.viewport = {
				.x = 0.f,
				.y = 0.f,
				.width = WINDOW_WIDTH,
				.height = WINDOW_HEIGHT,
				.min_depth = 0.f,
				.max_depth = 1.f,
			},
			.attachment_clear_values = clear_values,
		};

		gmg_assert_result(gmg_render_pass_init(app.logical_device, &render_pass_create_args, &app.render_pass_id));
	}
}

void app_render_state(GmgRenderState* render_state) {
	render_state->enable_primitive_restart = gmg_false;
	render_state->topology = GmgPrimitiveTopology_triangle_list;
	render_state->viewports_count = 1;

	render_state->rasterization.polygon_mode = GmgPolygonMode_fill;
	render_state->rasterization.front_face = GmgFrontFace_counter_clockwise;
	render_state->rasterization.line_width = 1.f;

	render_state->multisample.rasterization_samples_count = GmgSampleCount_1;

	static GmgRenderStateBlendAttachment blend_color_attachment = {0};
	blend_color_attachment.color_write_mask = 0xf;

	render_state->blend.attachments = &blend_color_attachment;
	render_state->blend.attachments_count = 1;

	render_state->depth_stencil.flags = GmgDepthStencilFlags_enable_depth_test | GmgDepthStencilFlags_enable_depth_write;
	render_state->depth_stencil.depth_compare_op = GmgCompareOp_less_or_equal;
	render_state->depth_stencil.back.fail_op = GmgStencilOp_keep;
	render_state->depth_stencil.back.pass_op = GmgStencilOp_keep;
	render_state->depth_stencil.back.compare_op = GmgCompareOp_always;
	render_state->depth_stencil.front.fail_op = GmgStencilOp_keep;
	render_state->depth_stencil.front.pass_op = GmgStencilOp_keep;
	render_state->depth_stencil.front.compare_op = GmgCompareOp_always;
}

void generate_cube(AppVec3 pos, float scale, AppVec3 rotation_vector, float angle, uint32_t vertices_start_idx, AppVertex* vertices_out, uint32_t* indices_out) {
	AppVec3 corners[8] = {
		AppVec3_init(-0.5f, -0.5f, -0.5f),
		AppVec3_init( 0.5f, -0.5f, -0.5f),
		AppVec3_init( 0.5f, -0.5f,  0.5f),
		AppVec3_init(-0.5f, -0.5f,  0.5f),
		AppVec3_init(-0.5f,  0.5f, -0.5f),
		AppVec3_init( 0.5f,  0.5f, -0.5f),
		AppVec3_init( 0.5f,  0.5f,  0.5f),
		AppVec3_init(-0.5f,  0.5f,  0.5f)
	};

	static AppVec2 tex_coords[] = {
		AppVec2_init(0.50f, 0.3333f),
		AppVec2_init(0.25f, 0.3333f),
		AppVec2_init(0.25f, 0.f),
		AppVec2_init(0.50f, 0.f),

		AppVec2_init(1.f, 0.6666f),
		AppVec2_init(0.75f, 0.6666f),
		AppVec2_init(0.75f, 0.3333f),
		AppVec2_init(1.f, 0.3333f),

		AppVec2_init(0.75f, 0.6666f),
		AppVec2_init(0.50f, 0.6666f),
		AppVec2_init(0.50f, 0.3333f),
		AppVec2_init(0.75f, 0.3333f),

		AppVec2_init(0.50f, 0.6666f),
		AppVec2_init(0.25f, 0.6666f),
		AppVec2_init(0.25f, 0.3333f),
		AppVec2_init(0.50f, 0.3333f),

		AppVec2_init(0.25f, 0.6666f),
		AppVec2_init(0.f, 0.6666f),
		AppVec2_init(0.f, 0.3333f),
		AppVec2_init(0.25f, 0.3333f),

		AppVec2_init(0.50f, 1.f),
		AppVec2_init(0.25f, 1.f),
		AppVec2_init(0.25f, 0.6666f),
		AppVec2_init(0.50f, 0.6666f),

	};

	static uint32_t indices[] = {
		0, 1, 2, 3,
		4, 5, 1, 0,
		5, 6, 2, 1,
		6, 7, 3, 2,
		7, 4, 0, 3,
		4, 7, 6, 5,
	};

	for (uint32_t i = 0; i < 24; i += 1) {
		vertices_out[i] = (AppVertex) {
			.pos = AppVec3_add(pos, AppVec3_rotate(AppVec3_mul_scalar(corners[indices[i]], scale), rotation_vector, angle)),
			.tex_coords = tex_coords[i],
		};
	}

	for (uint32_t i = 0, o = 0; i < 24; i += 4, o += 6) {
		indices_out[o + 0] = i + 0 + vertices_start_idx;
		indices_out[o + 1] = i + 1 + vertices_start_idx;
		indices_out[o + 2] = i + 2 + vertices_start_idx;
		indices_out[o + 3] = i + 2 + vertices_start_idx;
		indices_out[o + 4] = i + 3 + vertices_start_idx;
		indices_out[o + 5] = i + 0 + vertices_start_idx;
	}
}

int main(int argc, char** argv) {
	app_common_init();

	{
		AppUBO* ubo;
		gmg_assert_result(gmg_buffer_get_write_buffer(app.logical_device, app.uniform_buffer_id, 0, 1, (void**)&ubo));
		perspective_projection(ubo->mvp, 0.785398f, (float)WINDOW_WIDTH / (float)WINDOW_HEIGHT, 0.1f, 50.f);
	}

	{
		GmgTextureId texture_id;
		GmgTextureCreateArgs texture_create_args = {
			.type = GmgTextureType_2d,
			.format = GmgTextureFormat_r8g8b8a8_unorm,
			.flags = GmgTextureFlags_sampled,
			.samples = GmgSampleCount_1,
		};

		gmg_assert_result(gmg_texture_init(app.logical_device, &texture_create_args, &texture_id));

		DasStk(uint8_t) texture_file_data = {0};
		if (app_file_read_all(APP_TEXTURE_PATH, &texture_file_data)) {
			printf("unable to read file at %s: %s\n", APP_TEXTURE_PATH, strerror(errno));
			exit(1);
		}

		int width, height, channels_count;
		uint8_t* pixels = stbi_load_from_memory(DasStk_data(&texture_file_data), DasStk_count(&texture_file_data), &width, &height, &channels_count, 4);
		das_assert(pixels, "error decoding image: %s", stbi_failure_reason());

		{
			gmg_assert_result(gmg_texture_resize(app.logical_device, texture_id, width, height, 1, 1, 1));

			GmgTextureArea texture_area = {0};
			texture_area.width = width;
			texture_area.height = height;
			texture_area.depth = 1;
			texture_area.array_layers_count = 1;

			void* texture_write_buffer;
			gmg_assert_result(gmg_texture_get_write_buffer(app.logical_device, texture_id, &texture_area, (void**)&texture_write_buffer));

			memcpy(texture_write_buffer, pixels, width * height * channels_count);

			gmg_assert_result(gmg_material_set_texture(app.logical_device, app.material_id, 2, 0, texture_id));
		}

		stbi_image_free(pixels);
	}

	{
		GmgSamplerId sampler_id;
		GmgSamplerCreateArgs sampler_create_args = {0};
		sampler_create_args.address_mode_u = GmgSamplerAddressMode_clamp_to_edge;
		sampler_create_args.address_mode_v = GmgSamplerAddressMode_clamp_to_edge;

		gmg_assert_result(gmg_sampler_init(app.logical_device, &sampler_create_args, &sampler_id));

		gmg_assert_result(gmg_material_set_sampler(app.logical_device, app.material_id, 1, 0, sampler_id));
	}

	float inc = 0;
	while (1) {
		inc += 0.001;

		SDL_Event event;
		while (SDL_PollEvent(&event)) {
			if (event.type == SDL_QUIT) {
				return 0;
			}
		}

		//
		// game logic update
		//

		{
			AppVertex* vertices_ptr;
			gmg_assert_result(gmg_buffer_get_write_buffer(app.logical_device, app.vertex_buffer_id, 0, APP_VERTICES_COUNT, (void**)&vertices_ptr));
			uint32_t* indices;
			gmg_assert_result(gmg_buffer_get_write_buffer(app.logical_device, app.index_buffer_id, 0, APP_INDICES_COUNT, (void**)&indices));

			AppVec3 pos = AppVec3_init(0.f, 0.f, -4.0f);
			AppVec3 rotation_vector = AppVec3_init(0.f, inc, -0.5f);
			generate_cube(pos, 1.f, rotation_vector, inc, 0, vertices_ptr, indices);
		}

		uint32_t swapchain_texture_idx;
		gmg_assert_result(gmg_swapchain_get_next_texture_idx(app.logical_device, app.swapchain_id, &swapchain_texture_idx));
		GmgFrameBufferId present_frame_buffer_id = app.swapchain_frame_buffer_ids[swapchain_texture_idx];

		gmg_assert_result(gmg_render_pass_set_frame_buffer(app.logical_device, app.render_pass_id, present_frame_buffer_id));

		gmg_render_pass_clear_draw_cmds(app.logical_device, app.render_pass_id);

		GmgDrawCmdBuilder draw_cmd_builder = {0};

		gmg_assert_result(gmg_render_pass_draw_cmd_start(app.logical_device, app.render_pass_id, app.material_id, app.vertex_array_id, &draw_cmd_builder));
		gmg_assert_result(gmg_render_pass_draw_cmd_queue_indexed(&draw_cmd_builder, 0, 0, APP_INDICES_COUNT));

		gmg_assert_result(gmg_logical_device_submit(app.logical_device, &app.render_pass_id, 1));

		gmg_assert_result(gmg_swapchain_present(app.logical_device, &app.swapchain_id, 1));
	}

	return 0;
};

