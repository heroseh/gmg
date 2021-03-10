#version 450

layout (location = 0) in vec2 f_tex_coords;

layout (location = 0) out vec4 out_frag_color;

layout (binding = 1) uniform sampler u_sampler;
layout (binding = 2) uniform texture2D u_texture;

void main() {
	out_frag_color = texture(sampler2D(u_texture, u_sampler), f_tex_coords);
}

