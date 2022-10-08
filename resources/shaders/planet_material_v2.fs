#version 430
precision highp float;

layout (location = 0) out vec3 gColor;
layout (location = 1) out vec3 gPosition;
layout (location = 2) out vec3 gNormal;

layout(location = 0) in vec3 normal;
layout(location = 1) in float time;
layout(location = 2) in vec3 position;
layout(location = 3) in float altitude;
layout(location = 4) in vec2 coordinates;

uniform sampler2D grass;
uniform sampler2D sand;
uniform sampler2D rock;
uniform int fragment_normal_maps;
uniform vec3 ground_color;
layout(location = 5) uniform int fragment_normals;

void main()
{
	vec3 normal_vector = normal;

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

	gColor = ground_color;

	gNormal = normal;
	gPosition = position;
}