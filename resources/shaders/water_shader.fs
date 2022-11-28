#version 430
precision highp float;

#include "libs/deferred_output_translucent.cginc"

#include "libs/maths.cginc"

layout(location = 0) in vec3 normal;
layout(location = 2) in vec3 scene_normal;
layout(location = 1) in vec3 pos;
layout(location = 3) in float depth;
layout(location = 4) in vec2 uvs;
layout(location = 5) in vec3 world_direction;
layout(location = 6) in float wave_level;

#include "libs/random.cginc"

void main()
{
	float opacity = clamp_01(depth / 1000);
    float fresnel = (0.04 + (1.0 - 0.04) * (pow(1.0 - max(0.0, dot(-normal, world_direction)), 5.0)));
	gColor = vec4(0.0293, 0.0698, 0.1717, mix(0.2, 0.99, opacity));

	float foam_noise = simplex_noise(uvs * 0.3, 3, 0.65, 1.75) + 0.6;	
	float foam_level = 1 - clamp_01(length(pos) / 5000);
	float m = clamp_01(clamp_01(1 - depth / 10) + clamp_01(wave_level / 4 - 1));
	
	float foam_mask =  clamp_01(m * foam_level * foam_noise);
	gColor = mix(gColor, vec4(vec3(1), 1), foam_mask);
	gNormal = mix(normal, vec3(1, 0, 0), m);
	gMrao = mix(vec3(0.2, 0.2, 0), vec3(0.2, 0.2, 0), foam_mask * foam_noise);
}