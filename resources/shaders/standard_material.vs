#version 430

layout(location = 0) in vec3 pos;
layout(location = 1) in vec3 norm;
layout(location = 2) in vec2 tc;
layout(location = 3) in vec3 colors;

layout(location = 0) out vec3 out_norm;
layout(location = 1) out vec3 out_pos;

layout(location = 1) uniform mat4 model;

layout (std140, binding = 0) uniform WorldData
{
    mat4 proj_matrix;
    mat4 view_matrix;
    mat4 pv_matrix;
    float world_time;
};

void main()
{
	out_norm = norm;
	out_pos = (model * vec4(pos, 1.0)).xyz;
	gl_Position = pv_matrix * vec4(out_pos, 1.0);
}