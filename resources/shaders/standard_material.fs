#version 430
precision highp float;


layout (location = 0) out vec3 gColor;
layout (location = 1) out vec3 gNormal;

layout(location = 0) in vec3 normal;
layout(location = 1) in float time;
layout(location = 2) in vec2 tc;

void main()
{
	gColor = normal;//vec3(1, abs(sin(time)), 1);
	gNormal = vec3(tc, 1);
}