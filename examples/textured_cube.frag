#version 450

layout (location = 0) in vec2 f_tex_coords;

layout (location = 0) out vec4 out_frag_color;

layout (set = 1, binding = 0) uniform sampler u_sampler;
layout (set = 1, binding = 1) uniform texture2D u_texture;

void main() {
	out_frag_color = texture(sampler2D(u_texture, u_sampler), f_tex_coords);
}

