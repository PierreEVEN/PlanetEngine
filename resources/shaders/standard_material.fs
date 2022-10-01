#version 430
precision highp float;


layout (location = 0) out vec3 gColor;
layout (location = 1) out vec3 gPosition;
layout (location = 2) out vec3 gNormal;

layout(location = 0) in vec3 normal;
layout(location = 1) in vec3 pos;

void main()
{
	float slope = pow(dot(normal, vec3(0,0,1)), 8);
	

	gColor = mod(pos, 1.001);
	gPosition = pos;
	gNormal = normal;
}