#version 450

layout (location = 0) in vec3 v_pos;
layout (location = 1) in vec2 v_tex_coords;

layout (set = 0, binding = 0) uniform UBO {
	mat4 mvp;
} ubo;

layout (location = 0) out vec2 f_tex_coords;

out gl_PerVertex
{
	vec4 gl_Position;
};

void main()
{
	f_tex_coords = v_tex_coords;
	gl_Position = ubo.mvp * vec4(v_pos, 1.0);
}
