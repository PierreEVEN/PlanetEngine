#version 430 core

#include "libs/deferred_output.cginc"

in vec3 color; 

void main()
{
	gNormal = vec3(1, 0, 0);
	gColor = color;//vec3(1, 1, 0);
	gMrao = vec3(0, 1, 1);
}
