#version 430
precision highp float;


layout (location = 0) out vec3 gColor;
layout (location = 1) out vec3 gNormal;

layout(location = 0) in vec3 normal;
layout(location = 1) in float time;
layout(location = 2) in vec2 tc;
layout(location = 3) in vec3 color;
layout(location = 4) in float altitude;

void main()
{
	float slope = pow(dot(normal, vec3(0,0,1)), 8);
	

	gColor = mix(vec3(0.5, 0.5, 0.5), vec3(0.7, 1, 0.6), slope);

	gColor = mix(vec3(0.8,0.7,0.5), gColor, clamp(altitude / 5, 0, 1));

	float depth_scale = clamp(-altitude / 10, 0, 1);

	if (altitude < 1) {
		gColor = mix(vec3(97, 130, 223) / 256, vec3(97, 130, 223) / 350 , depth_scale);
	}

	gNormal = normal;
}