#version 430
precision highp float;


layout (location = 0) out vec3 gPosition;
layout (location = 1) out vec3 gNormal;

layout(location = 0) in vec2 uv;

void main()
{
	gPosition = vec3(uv, 0).rgb;
	gNormal = vec3(uv, 0);
}