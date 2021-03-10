#include "deps/common.h"

#define APP_NAME "Triangle Example"
#define APP_VERTEX_SHADER_SRC "bin/triangle.vert.spv"
#define APP_FRAGMENT_SHADER_SRC "bin/triangle.frag.spv"
#define APP_VERTICES_COUNT 3
#define APP_INDICES_COUNT 3
#define APP_DEPTH_STENCIL_FORMAT GmgTextureFormat_none

typedef struct AppVertex AppVertex;
struct AppVertex {
	struct {
		float x;
		float y;
	} pos;
	union {
		struct {
			uint8_t color_r;
			uint8_t color_g;
			uint8_t color_b;
			uint8_t color_a;
		};
		uint8_t color_array[4];
	};
};

static GmgVertexAttribInfo app_vertex_layout_attrib_infos[] = {
	{
		.location_idx = 0,
		.elmt_type = GmgVertexElmtType_f32,
		.vector_type = GmgVertexVectorType_2,
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
		GmgAttachmentLayout attachment_layout = {
			.format = app.surface_format,
			.samples_count = 1,
			.present = gmg_true,
		};

		GmgRenderPassLayoutCreateArgs rpl_create_args = {
			.attachments = &attachment_layout,
			.attachments_count = 1,
		};

		gmg_assert_result(gmg_render_pass_layout_init(app.logical_device, &rpl_create_args, &app.render_pass_layout_id));
	}

	{
		GmgAttachmentInfo render_pass_color_attachment = {
			.load_op = GmgAttachmentLoadOp_clear,
			.store_op = GmgAttachmentStoreOp_preserve,
			.stencil_load_op = GmgAttachmentLoadOp_uninitialized,
			.stencil_store_op = GmgAttachmentStoreOp_discard,
		};

		GmgClearValue clear_value = { .color.float32 = { 0.f, 0.f, 0.f, 1.f } };

		GmgRenderPassCreateArgs render_pass_create_args = {
			.layout_id = app.render_pass_layout_id,
			.attachments = &render_pass_color_attachment,
			.attachments_count = 1,
			.viewport = {
				.x = 0.f,
				.y = 0.f,
				.width = WINDOW_WIDTH,
				.height = WINDOW_HEIGHT,
				.min_depth = 0.f,
				.max_depth = 1.f,
			},
			.attachment_clear_values = &clear_value,
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
}

int main(int argc, char** argv) {
	app_common_init();

	{
		AppVertex* vertices_ptr;
		gmg_assert_result(gmg_buffer_get_write_buffer(app.logical_device, app.vertex_buffer_id, 0, APP_VERTICES_COUNT, (void**)&vertices_ptr));
		vertices_ptr[0] = (AppVertex) {
			.pos.x = 0.2,
			.pos.y = 0.2,
			.color_array = { 0xff, 0, 0, 0xff },
		};
		vertices_ptr[1] = (AppVertex) {
			.pos.x = 0.5,
			.pos.y = 0.8,
			.color_array = { 0, 0xff, 0, 0xff },
		};
		vertices_ptr[2] = (AppVertex) {
			.pos.x = 0.8,
			.pos.y = 0.2,
			.color_array = { 0, 0, 0xff, 0xff },
		};
	}

	{
		uint32_t* indices;
		gmg_assert_result(gmg_buffer_get_write_buffer(app.logical_device, app.index_buffer_id, 0, APP_INDICES_COUNT, (void**)&indices));
		indices[0] = 0;
		indices[1] = 1;
		indices[2] = 2;
	}

	{
		AppUBO* ubo;
		gmg_assert_result(gmg_buffer_get_write_buffer(app.logical_device, app.uniform_buffer_id, 0, 1, (void**)&ubo));
		ortho_projection(ubo->mvp, 0.f, 1.f, 1.f, 0.f, -1.f, 1.f);
	}

	while (1) {
		SDL_Event event;
		while (SDL_PollEvent(&event)) {
			if (event.type == SDL_QUIT) {
				return 0;
			}
		}

		//
		// game logic update
		//

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

