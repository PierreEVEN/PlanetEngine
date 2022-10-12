#version 430
precision highp float;

#include "libs/deferred_output.cginc"


#include "libs/world_data.cginc"

layout(location = 0) in vec3 normal;
layout(location = 1) in vec3 debug_scalar;
layout(location = 2) in vec3 position;
layout(location = 3) in float altitude;
layout(location = 4) in vec2 coordinates;

uniform sampler2D grass;
uniform sampler2D sand;
uniform sampler2D rock;
uniform int fragment_normal_maps;
uniform vec3 ground_color;

void main()
{
	vec3 normal_vector = normal;


	vec2 uv = position.xy / 2 + camera_pos.zx;

	vec3 rock_color = texture(grass, coordinates * 1000).rgb;
	vec3 sand_color = texture(sand, coordinates * 1000).rgb;
	vec3 grass_color = texture(rock, coordinates * 1000).rgb;
	vec3 snow_color = vec3(0.8, 0.75, 0.8);
	vec3 rock_color_bis = texture(rock, coordinates * 1398.48945).rgb;

	float slope = pow(dot(normal_vector, vec3(0,0,1)), 20) + .2;
	slope = 0;

	gColor = mix((rock_color_bis + rock_color) / 2, grass_color, clamp(slope, 0, 1));

	gColor = mix(sand_color, gColor, clamp((altitude - 5 ) / 10 , 0, 1));

	gColor = mix(gColor, snow_color, clamp((altitude - 4000) / 800, 0, 1));

	float depth_scale = clamp(-altitude / 300, 0, 1);

	if (altitude < 0.001) {
		gColor = mix(vec3(97, 130, 223) / 256, vec3(97, 130, 223) / 350 , depth_scale);
	}

	gNormal = normal;
}