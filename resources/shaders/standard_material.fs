#version 430
precision highp float;

#include "resources/shaders/libs/deferred_output.shi"

layout(location = 0) in vec3 normal;
layout(location = 1) in vec3 pos;

void main()
{
	float slope = pow(dot(normal, vec3(0,0,1)), 8);
	

	gColor = mod(pos, 1.001);
	gNormal = normal;
}