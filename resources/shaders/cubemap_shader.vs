#version 460

#include "libs/world_data.cginc"

layout(location = 0) in vec3 pos;
layout(location = 1) in vec3 norm;
layout(location = 2) in vec2 tc;
layout(location = 3) in vec3 colors;

layout(location = 0) out vec3 out_norm;
layout(location = 1) out vec3 out_pos;

layout(location = 1) uniform mat4 model;

void main()
{
	gl_Position = model * vec4(pos, 1.0);

	out_pos = gl_Position.xyz;
	out_norm = norm;
    gl_Position = pv_matrix * gl_Position;
}