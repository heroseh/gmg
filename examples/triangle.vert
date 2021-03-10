#version 450

layout (location = 0) in vec2 v_pos;
layout (location = 1) in vec4 v_color;

layout (binding = 0) uniform UBO {
	mat4 mvp;
} ubo;

layout (location = 0) out vec4 f_color;

out gl_PerVertex
{
	vec4 gl_Position;
};

void main()
{
	f_color = v_color;
	gl_Position = ubo.mvp * vec4(v_pos, 0.0, 1.0);
}
