#include "deps/common.h"

#define APP_NAME "Cube Example"
#define APP_VERTEX_SHADER_SRC "bin/cube.vert.spv"
#define APP_FRAGMENT_SHADER_SRC "bin/cube.frag.spv"
#define APP_VERTICES_COUNT 8
#define APP_INDICES_COUNT 36
#define APP_DEPTH_STENCIL_FORMAT GmgTextureFormat_d32

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
	AppColor color;
};

static GmgVertexAttribInfo app_vertex_layout_attrib_infos[] = {
	{
		.location_idx = 0,
		.elmt_type = GmgVertexElmtType_f32,
		.vector_type = GmgVertexVectorType_3,
	},
	{
		.location_idx = 1,
		.elmt_type = GmgVertexElmtType_u8_f32_normalize,
		.vector_type = GmgVertexVectorType_4,
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

	static AppColor colors[] = {
		AppColor_init(0xae, 0x55, 0x34, 0xff),
		AppColor_init(0xae, 0x55, 0x34, 0xff),
		AppColor_init(0xae, 0x55, 0x34, 0xff),
		AppColor_init(0xae, 0x55, 0x34, 0xff),
		AppColor_init(0x45, 0x33, 0xae, 0xff),
		AppColor_init(0x45, 0x33, 0xae, 0xff),
		AppColor_init(0x45, 0x33, 0xae, 0xff),
		AppColor_init(0x45, 0x33, 0xae, 0xff),
	};

	static uint32_t indices[] = {
		0, 1, 2, 2, 3, 0,
		4, 5, 1, 1, 0, 4,
		5, 6, 2, 2, 1, 5,
		6, 7, 3, 3, 2, 6,
		7, 4, 0, 0, 3, 7,
		4, 7, 6, 6, 5, 4,
	};

	for (uint32_t i = 0; i < 8; i += 1) {
		vertices_out[i] = (AppVertex) {
			.pos = AppVec3_add(pos, AppVec3_rotate(AppVec3_mul_scalar(corners[i], scale), rotation_vector, angle)),
			.color = colors[i],
		};
	}

	for (uint32_t i = 0; i < 36; i += 1) {
		indices_out[i] = indices[i] + vertices_start_idx;
	}
}

int main(int argc, char** argv) {
	app_common_init();

	{
		AppUBO* ubo;
		gmg_assert_result(gmg_buffer_get_write_buffer(app.logical_device, app.uniform_buffer_id, 0, 1, (void**)&ubo));
		perspective_projection(ubo->mvp, 0.785398f, (float)WINDOW_WIDTH / (float)WINDOW_HEIGHT, 0.1f, 50.f);
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

