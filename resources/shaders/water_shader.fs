#version 430
precision highp float;

#include "libs/deferred_output_translucent.cginc"

layout(location = 0) in vec3 normal;
layout(location = 1) in vec3 pos;
layout(location = 2) in vec4 col;



void main()
{
	gColor = col;
	gNormal = normal;
	gMrao = vec3(0, 0, 0);
}