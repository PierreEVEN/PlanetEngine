#version 430
precision highp float;

#include "libs/world_data.cginc"
#include "libs/lighting.cginc"

layout(location = 1) uniform sampler2D Input_color;
layout(location = 2) uniform sampler2D Input_normal;
layout(location = 3) uniform sampler2D Input_mrao;
layout(location = 5) uniform sampler2D Input_Depth;
layout(location = 6) uniform samplerCube WORLD_Cubemap;
layout(location = 11) uniform samplerCube ENV_cubemap;
layout(location = 12) uniform sampler2D Input_SSR_Color;

out vec4 oFragmentColor;

layout(location = 0) in vec2 uv;
layout(location = 7) uniform float z_near;
layout(location = 8) uniform int enable_atmosphere;
layout(location = 9) uniform int atmosphere_quality;
layout(location = 10) uniform int shading;

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

#include "libs/atmosphere.cginc"

vec3 getSceneWorldPosition(float linear_depth) {
    // Get z depth
    float zDepth = linear_depth;

    // compute clip space depth
    vec4 clipSpacePosition = vec4(uv * 2.0 - 1.0, zDepth, 1.0);

    // Transform local space to view space
    vec4 viewSpacePosition = proj_matrix_inv * clipSpacePosition;

    viewSpacePosition /= viewSpacePosition.w;

    // Transform view space to world space
    vec4 worldSpacePosition = view_matrix_inv * viewSpacePosition;
    return worldSpacePosition.xyz;
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


vec3 add_sun(vec3 base_color, vec3 sun_location, float sun_radius, vec3 pixel_direction, vec3 camera_location, float scene_depth) {
	// Trace sun disc
    RaySphereTraceResult sunInfos = raySphereIntersection(sun_location, sun_radius, pixel_direction, camera_location);
    float distanceThroughSun = max(0.0, sunInfos.atmosphereDistanceOut - sunInfos.atmosphereDistanceIn);

	// Draw sun disc
    if (scene_depth > length(camera_location - sun_location) - sun_radius) {
        base_color += distanceThroughSun / 2000;
        base_color += texture(WORLD_Cubemap, Rx(-PI / 2) * pixel_direction).xyz * 10.2;
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

vec3 surface_reflection(vec3 normal, vec3 camera_dir, vec3 camera_pos, vec3 world_position, vec3 light_direction) {

    vec3 reflect_dir = reflect(camera_dir, normal);

    vec3 reflect_color = vec3(0);
    float reflect_depth = 100000000000000.0;

    // Get SSR Uvs    
    vec4 ssr_uv = texture(Input_SSR_Color, uv);
    if (ssr_uv.b > 0.0) {
		vec3 col = texture(Input_color, ssr_uv.xy).rgb;
		vec3 norm = normalize(texture(Input_normal, ssr_uv.xy).rgb);
		vec3 mrao = texture(Input_mrao, ssr_uv.xy).rgb;
        reflect_color = surface_shading(1, col, norm, mrao, light_direction, camera_dir);
        reflect_depth = z_near / texture(Input_Depth, ssr_uv.xy).x;
    }
    
    reflect_color = add_sun(
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
	float depth = texture(Input_Depth, uv).r;
	float linear_depth = z_near / depth;
	oFragmentColor = vec4(0);

    vec3 cameraDirection = getSceneWorldDirection();

	if (depth > 0) {
		vec3 col = texture(Input_color, uv).rgb;
		vec3 norm = normalize(texture(Input_normal, uv).rgb);
		vec3 mrao = texture(Input_mrao, uv).rgb;
        if (mrao.g < 0.2)
            oFragmentColor = vec4(surface_reflection(norm, cameraDirection, camera_pos, getSceneWorldPosition(depth), light_dir), 1);
        else
            oFragmentColor = vec4(surface_shading(shading, col, norm, mrao, light_dir, cameraDirection), 1);
	}

    oFragmentColor.xyz += add_sun(
        oFragmentColor.xyz,
        light_dir * 10000000000.0,
        100000000.0,
        cameraDirection,
        camera_pos,
        linear_depth
    );

    if (enable_atmosphere != 0) {
        AtmosphereSettings atmosphere;
        atmosphere.center = planetCenter;
        atmosphere.radius = atmosphereRadius;
        oFragmentColor.xyz = add_atmosphere(
            oFragmentColor.xyz,
            atmosphere,
            cameraDirection,
            camera_pos,
            linear_depth
        );
    }
}