#version 430 core
#extension GL_ARB_explicit_uniform_location : enable
precision highp float;

#include "libs/deferred_output.cginc"


#include "libs/world_data.cginc"

layout(location = 0) in vec3 normal;
layout(location = 1) in vec3 position;
layout(location = 2) in float altitude;
layout(location = 3) in vec2 coordinates;
layout(location = 4) in float planet_radius;
layout(location = 5) in vec4 g_DebugScalar;
layout(location = 6) in vec3 g_LocalNormal;
layout(location = 7) in vec3 g_Tangent;
layout(location = 8) in vec3 g_BiTangent;

layout(location = 9) uniform sampler2D grass_color;
layout(location = 10) uniform sampler2D sand_color;
layout(location = 11) uniform sampler2D rock_color;

layout(location = 12) uniform sampler2D rock_normal;
layout(location = 13) uniform sampler2D grass_normal;
layout(location = 14) uniform sampler2D sand_normal;

layout(location = 15) uniform sampler2D rock_mrao;
layout(location = 16) uniform sampler2D grass_mrao;
layout(location = 17) uniform sampler2D sand_mrao;

layout(location = 18) uniform sampler2D water_normal;
layout(location = 19) uniform sampler2D water_displacement;

struct LandData {
	vec3 color;
	vec3 normal;
	vec3 mrao;
};

LandData mix_ld(LandData a, LandData b, float value) {
	value = clamp(value, 0, 1);
	LandData res;
	res.color = mix(a.color, b.color, value);
	res.normal = mix(a.normal, b.normal, value);
	res.mrao = mix(a.mrao, b.mrao, value);
	return res;
}


LandData make_ld_tex(sampler2D color, sampler2D normal, sampler2D mrao, vec2 tc) {
	LandData res;
	res.color = texture(color, tc).rgb;
	res.normal = texture(normal, tc).rgb;
	res.mrao = vec3(0, 1, 1);
	return res;
}

LandData make_ld_col(vec3 color) {
	LandData res;
	res.color = color;
	res.normal = vec3(0, 0, 1);
	res.mrao = vec3(0, 1, 1);
	return res;
}

vec3 make_mrao(float metalness, float roughness, float ao) {
	return vec3(metalness, roughness, ao);
}


vec3 water_color() {
	vec3 pixel_to_cam = normalize(position);
	float fresnel = clamp(pow(1 - abs(dot(normal, pixel_to_cam)), 16), 0, 1);
	return mix(vec3(30, 30, 150),vec3(10, 20, 50),  fresnel);
}


void main()
{
	mat3 TBN = mat3(g_Tangent, g_BiTangent, normal);
	float slope = 1 - pow(dot(g_LocalNormal, vec3(0,0,1)) - 0.0001, 64);
	float camera_distance = length(position);

	// Create materials
	float textures_scale = 1000;
	LandData rock = make_ld_tex(rock_color, rock_normal, rock_mrao, coordinates * textures_scale);
	rock.mrao = make_mrao(0.01, 0.5, 0);
	LandData grass = make_ld_tex(grass_color, grass_normal, grass_mrao, coordinates * textures_scale);
	grass.mrao = make_mrao(0.03, 0.7, 0);
	LandData sand = make_ld_tex(sand_color, sand_normal, sand_mrao, coordinates * textures_scale);
	sand.mrao = make_mrao(0.2, 0.5, 0);
	LandData water = make_ld_col(water_color() / 256);
	water.mrao = make_mrao(0.5, 0.05, 0);
	LandData water_deep = make_ld_col(water_color() / 750);
	water_deep.mrao = make_mrao(1, 0.05, 0);

	LandData ground = mix_ld(grass, rock, slope); // Grass rock
	ground = mix_ld(ground, sand, (-altitude + 10) / 2); // Add beach
	LandData ocean = mix_ld(water, water_deep, pow(-altitude / 200000, 0.5)); // Ocean

	ocean.normal = texture(water_normal, coordinates * 10 + vec2(world_time * -0.01)).rgb;
	ocean.normal *= texture(water_normal, coordinates * 10 + vec2(-world_time * 0.02, world_time * 0.024)).rgb;



	LandData ground_ocean = mix_ld(ground, ocean, -altitude * 10); // Mix all
	LandData result = ground_ocean;

	// Disable normals by distance
	vec3 output_normal = mix((result.normal), vec3(0, 0, 1), vec3(pow(clamp(camera_distance / 200000, 0, 1), 0.25)));

	gNormal = TBN * output_normal;
	gColor = vec3(g_BiTangent);
	gColor = result.color;
	//gNormal = TBN * vec3(0,0,1);
	gColor = g_DebugScalar.xyz;

	//gColor = vec3(dot(g_Tangent, vec3(1,0,0)));

	gMrao = vec4(result.mrao, 1);
}