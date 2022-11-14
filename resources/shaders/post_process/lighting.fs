#version 430
precision highp float;

#include "../libs/world_data.cginc"
#include "../libs/lighting.cginc"

layout(location = 1) uniform sampler2D Scene_color;
layout(location = 2) uniform sampler2D Scene_normal;
layout(location = 3) uniform sampler2D Scene_mrao;
layout(location = 4) uniform sampler2D Scene_depth;

layout(location = 5) uniform sampler2D Translucency_color;
layout(location = 6) uniform sampler2D Translucency_normal;
layout(location = 7) uniform sampler2D Translucency_mrao;
layout(location = 8) uniform sampler2D Translucency_depth;

layout(location = 9) uniform float z_near;
layout(location = 10) uniform int enable_atmosphere;
layout(location = 11) uniform int atmosphere_quality;
layout(location = 12) uniform int shading;

layout(location = 13) uniform samplerCube WORLD_Cubemap;
layout(location = 14) uniform sampler2D Input_SSR_Color;

layout(location = 0) in vec2 uv;

out vec4 oFragmentColor;

int NumScatterPoints = 5;
int NumOpticalDepthPoints = 5;
vec3 planetCenter = vec3(0,0, 0);
float atmosphereRadius = 6200000;
float planetRadius = 6000000;
float atmosphereDensityFalloff = 6;
float scatter_strength = 2;
vec3 scatterCoefficients = pow(400 / vec3(700, 550, 460), vec3(4)) * scatter_strength;
const float epsilon = 1;
vec3 light_dir = normalize(vec3(1, 0, 0));

#include "../libs/atmosphere.cginc"

float get_linear_depth(vec2 uvs) {    
	float scene_depth = texture(Scene_depth, uvs).r;
	return z_near / scene_depth;
}

vec3 getSceneWorldDirection() {
    // compute clip space direction
    vec4 clipSpacePosition = vec4(uv * 2.0 - 1.0, 1.0, 1.0);

    // Transform local space to view space
    vec4 viewSpacePosition = proj_matrix_inv * clipSpacePosition;

    viewSpacePosition /= viewSpacePosition.w;

    // Transform view space to world space
    vec4 worldSpacePosition = view_matrix_inv * viewSpacePosition;
    return normalize(worldSpacePosition.xyz);
}

struct AtmosphereSettings {
    vec3 center;
    float radius;
};


vec3 add_space(vec3 base_color, vec3 sun_location, float sun_radius, vec3 pixel_direction, vec3 camera_location, float scene_depth) {
	// Trace sun disc
    RaySphereTraceResult sunInfos = raySphereIntersection(sun_location, sun_radius, pixel_direction, camera_location);
    float distanceThroughSun = max(0.0, sunInfos.atmosphereDistanceOut - sunInfos.atmosphereDistanceIn);

	// Draw sun disc
    if (scene_depth > length(camera_location - sun_location) - sun_radius) {
        base_color += distanceThroughSun / 2000;
        base_color += texture(WORLD_Cubemap, pixel_direction).xyz * .2;
    }
    return base_color;
}

vec3 add_atmosphere(vec3 base_color, AtmosphereSettings atmosphere, vec3 pixel_direction, vec3 view_pos, float scene_depth) {

    RaySphereTraceResult hitInfo = raySphereIntersection(atmosphere.center, atmosphere.radius, pixel_direction, view_pos);

    float outMax = min(hitInfo.atmosphereDistanceOut, scene_depth);
	float distance_to_atmosphere = hitInfo.atmosphereDistanceIn;
    float distanceThroughAtmosphere = outMax - hitInfo.atmosphereDistanceIn;

    if (distanceThroughAtmosphere > 0.0) {
        vec3 pointInAtmosphere = view_pos + pixel_direction * (distance_to_atmosphere - epsilon);
        return computeLight(pointInAtmosphere, pixel_direction, distanceThroughAtmosphere - epsilon, base_color);
	}

    return base_color;
}



vec3 ground_color_shading(int in_shading, vec3 albedo, vec3 normal, vec3 mrao, vec3 light_dir, vec3 camera_pos, vec3 world_direction, float linear_depth) {

    vec3 ground_albedo = surface_shading(in_shading, albedo, normal, mrao, light_dir, world_direction);
        return ground_albedo;
}


vec3 surface_reflection(int shading_mode, vec3 normal, vec3 camera_dir, vec3 camera_pos, vec3 light_direction, float linear_depth) {

    // Compute reflected ray dir
    vec3 reflect_dir = reflect(camera_dir, normal);
    vec3 world_position = camera_dir * linear_depth;
    vec3 reflect_color = vec3(0);
    float reflect_depth = 100000000000000.0;

    // If found screen space hit, compute screen space reflections result
    vec4 ssr_uv = texture(Input_SSR_Color, uv);
    if (ssr_uv.b > 0.0) {
		vec3 col = texture(Scene_color, ssr_uv.xy).rgb;
		vec3 norm = normalize(texture(Scene_normal, ssr_uv.xy).rgb);
		vec3 mrao = texture(Scene_mrao, ssr_uv.xy).rgb;
        reflect_color = ground_color_shading(shading_mode, col, norm, mrao, light_direction, camera_pos, camera_dir, linear_depth);
        reflect_depth = get_linear_depth(ssr_uv.xy).r;
    }
    
    // Add sun and atmosphere to reflections
    reflect_color += add_space(
        reflect_color,
        light_dir * 10000000000.0,
        100000000.0,
        reflect_dir,
        world_position,
        reflect_depth
    );

    AtmosphereSettings atmosphere;
    atmosphere.center = planetCenter;
    atmosphere.radius = atmosphereRadius;
    vec3 reflect_atmosphere = add_atmosphere(
        reflect_color, 
        atmosphere,
        reflect_dir, 
        world_position + camera_pos,
        reflect_depth);
    
    return reflect_atmosphere;
}


void main()
{
    NumScatterPoints = atmosphere_quality;
    NumOpticalDepthPoints = atmosphere_quality;

    vec3 world_direction = getSceneWorldDirection();

    vec3 translucent_color = texture(Translucency_color, uv).rgb;
    float translucent_depth = z_near / texture(Translucency_depth, uv).r;

    oFragmentColor = texture(Translucency_color, uv);

    vec3 space = add_space(
        translucent_color,
        light_dir * 10000000000.0,
        100000000.0,
        world_direction,
        camera_pos,
        translucent_depth
    );

    oFragmentColor = vec4(space, 1);

    if (enable_atmosphere != 0) {
        AtmosphereSettings atmosphere;
        atmosphere.center = planetCenter;
        atmosphere.radius = atmosphereRadius;
        oFragmentColor.xyz = add_atmosphere(
            space,
            atmosphere,
            world_direction,
            camera_pos,
            translucent_depth
        );
    }
}