#version 430
precision highp float;

#include "resources/shaders/libs/deferred_output.shi"
#include "resources/shaders/libs/world_data.shi"
#include "resources/shaders/libs/landscape.shi"

layout(location = 0) in vec3 normal;
layout(location = 1) in float time;
layout(location = 2) in vec3 position;
layout(location = 3) in float altitude;
layout(location = 4) in vec2 coordinates;

uniform sampler2D grass;
uniform sampler2D sand;
uniform sampler2D rock;

uniform int fragment_normal_maps;
layout(location = 5) uniform int fragment_normals;

void main()
{
	vec3 normal_vector = normal;	
	if (fragment_normals != 0) {
		float offset = max(0.2, distance(position, camera_pos) / 500.0);
		vec3 h0 = vec3(0, 0, get_height_at_location(coordinates.xy));
		vec3 h1 = vec3(offset, 0, get_height_at_location(coordinates.xy + vec2(offset, 0)));
		vec3 h2 = vec3(0, offset, get_height_at_location(coordinates.xy + vec2(0, offset)));
		normal_vector = normalize(cross(h1 - h0, h2 - h0));

	}

	vec2 uv = position.xy / 2;

	vec3 grass_color = texture(grass, uv).rgb;
	vec3 sand_color = texture(sand, uv).rgb;
	vec3 rock_color = texture(rock, uv).rgb;
	vec3 rock_color_bis = texture(rock, uv * 1.39848945).rgb;

	float slope = pow(dot(normal_vector, vec3(0,0,1)), 20) + .2;
	

	gColor = mix((rock_color_bis + rock_color) / 2, grass_color, clamp(slope, 0, 1));

	gColor = mix(sand_color, gColor, clamp(altitude / 1 - 2, 0, 1));

	float depth_scale = clamp(-altitude / 10, 0, 1);

	if (altitude < 1) {
		normal_vector = vec3(0,0,1);
		gColor = mix(vec3(97, 130, 223) / 256, vec3(97, 130, 223) / 350 , depth_scale);
	}

	gNormal = normal_vector;
	gPosition = position;
}