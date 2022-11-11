#version 430 core
#extension GL_ARB_explicit_uniform_location : enable
precision highp float;

#include "libs/deferred_output.cginc"


#include "libs/world_data.cginc"
#include "libs/maths.cginc"

layout(location = 0) in vec3 g_LocalNormal;
layout(location = 1) in vec3 position;
layout(location = 2) in float altitude;
layout(location = 3) in vec2 g_Coordinates;
layout(location = 4) in float planet_radius;
layout(location = 5) in vec3 g_DebugScalar;
layout(location = 6) in vec3 g_Normal;
layout(location = 7) in vec3 g_Tangent;
layout(location = 8) in vec3 g_BiTangent;
layout(location = 9) in vec3 g_Normal_PS;

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

vec2 uv_from_sphere_pos(vec3 sphere_norm) {
    vec3 abs_norm = abs(sphere_norm);
    const float multiplier = 1 / PI * 2 * 1000;

    float x = 0;
    float y = 0;
    // TOP
    if (abs_norm.z > abs_norm.x && abs_norm.z > abs_norm.y && sphere_norm.z > 0)
    {

         x = mod(atan(sphere_norm.x, sphere_norm.z) * multiplier, 1) + 0.5;
         y = mod(atan(sphere_norm.y, sphere_norm.z) * multiplier, 1) + 0.5;         
    }
    // FRONT
    else if (abs_norm.x > abs_norm.y && abs_norm.x > abs_norm.z && sphere_norm.x > 0)
    {
        x = mod(atan(sphere_norm.x, sphere_norm.z) * multiplier, 1) - 0.5;
        y = mod(atan(sphere_norm.y, sphere_norm.x) * multiplier, 1) + 0.5;
        
    }
    // BOTTOM
    else if (abs_norm.z > abs_norm.x && abs_norm.z > abs_norm.y && sphere_norm.z < 0)
    {
        x = mod(atan(-sphere_norm.x, -sphere_norm.z) * multiplier, 1) + 0.5;
        y = mod(atan(sphere_norm.y, -sphere_norm.z) * multiplier, 1) + 0.5;
        
    }
    // BACK
    else if (abs_norm.x > abs_norm.y && abs_norm.x > abs_norm.z && sphere_norm.x < 0)
    {
        x = mod(atan(-sphere_norm.x, -sphere_norm.z) * multiplier, 1) - 0.5;
        y = mod(atan(sphere_norm.y, -sphere_norm.x) * multiplier, 1) + 0.5;
    }
    // LEFT
    else if (abs_norm.y > abs_norm.x && abs_norm.y > abs_norm.z && sphere_norm.y > 0)
    {
        x = mod(atan(sphere_norm.x, sphere_norm.y) * multiplier, 1) + 0.5;
        y = mod(atan(-sphere_norm.y, sphere_norm.z) * multiplier, 1) + 1.5;
    }
    // RIGHT
    else if (abs_norm.y > abs_norm.x && abs_norm.y > abs_norm.z && sphere_norm.y < 0)
    {
        x = mod(atan(-sphere_norm.x, sphere_norm.y) * multiplier, 1) - 0.5;
        y = mod(atan(-sphere_norm.y, sphere_norm.z) * multiplier, 1) - 0.5;
    }

    return vec2(x, y);
}

struct LandData {
	vec3 color;
	vec3 normal;
	vec3 mrao;
};

vec3 load_normal(sampler2D tex, vec2 text_coords) {
	return normalize(texture(tex, text_coords).rgb * 2 - 1);
}

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
	res.normal = load_normal(normal, tc);
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
	float fresnel = clamp(pow(1 - abs(dot(g_Normal, pixel_to_cam)), 16), 0, 1);
	return mix(vec3(30, 30, 150),vec3(10, 20, 50),  fresnel);
}


void main()
{
    if (altitude > 0)
        discard;


	mat3 TBN = mat3(g_Tangent, g_BiTangent, g_Normal);
	float slope = 1 - pow(dot(g_LocalNormal, vec3(0,0,1)) - 0.0001, 64);
	float camera_distance = length(position);

	// Compute UVs
	
    vec3 sphere_bitangent = vec3(0);
    vec3 sphere_tangent = vec3(0);
    vec2 coordinates = vec2(0);//uv_from_sphere_pos(g_Normal_PS);
	coordinates = g_Coordinates;


	// Create materials
	float textures_scale = 400;

	LandData water = make_ld_col(water_color() / 256);
	water.mrao = make_mrao(0.5, 0.05, 0);
	LandData water_deep = make_ld_col(water_color() / 750);
	water_deep.mrao = make_mrao(1, 0.05, 0);

	LandData ocean = mix_ld(water, water_deep, pow(-altitude / 200000, 0.5)); // Ocean

	ocean.normal = load_normal(water_normal, coordinates * 5 + vec2(world_time * -0.005)).rgb;
	ocean.normal += load_normal(water_normal, coordinates * 5 + vec2(-world_time * 0.004, world_time * 0.006));

	ocean.normal += load_normal(water_normal, coordinates * 40 + vec2(world_time * -0.03));
	ocean.normal += load_normal(water_normal, coordinates * 40 + vec2(-world_time * 0.03, world_time * 0.024));
	ocean.normal = normalize(ocean.normal * vec3(1, 1, 3));

	gNormal = TBN * ocean.normal;
	gColor = ocean.color;
	gMrao = vec4(ocean.mrao, 1);
	gDebugTarget = vec3(0, 1 - ocean.normal.z , 0);//g_DebugScalar;
}