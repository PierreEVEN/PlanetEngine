#version 430
precision highp float;

#include "libs/world_data.cginc"
#include "libs/lighting.cginc"

layout(location = 1) uniform sampler2D Input_color;
layout(location = 2) uniform sampler2D Input_normal;
layout(location = 3) uniform sampler2D Input_mrao;
layout(location = 5) uniform sampler2D Input_Depth;
layout(location = 6) uniform samplerCube WORLD_Cubemap;

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

void main()
{
    NumScatterPoints = atmosphere_quality;
    NumOpticalDepthPoints = atmosphere_quality;
	float depth = texture(Input_Depth, uv).r;
	float linear_depth = z_near / depth;
	oFragmentColor = vec4(0);

	if (depth > 0) {
		vec3 col = texture(Input_color, uv).rgb;
		vec3 norm = normalize(texture(Input_normal, uv).rgb);
		vec3 mrao = texture(Input_mrao, uv).rgb;

        switch (shading) {
        case 0:
            oFragmentColor = vec4(col, 1);
            break;
        case 1:
            PhongParams phong_params;
            phong_params.ambiant_strength = 0.001;
            phong_params.specular_strength = (mrao.r) * 32;
            phong_params.specular_shininess = (1 - mrao.g) * 16 + 1;
            oFragmentColor = vec4(phong_lighting(col, norm, light_dir, -getSceneWorldDirection(), phong_params), 1);
            break;
        case 2:
            PhongParams blinn_phong_params;
            blinn_phong_params.ambiant_strength = 0.001;
            blinn_phong_params.specular_strength = (mrao.r) * 32;
            blinn_phong_params.specular_shininess = (1 - mrao.g) * 16 + 1;
            oFragmentColor = vec4(blinn_phong_lighting(col, norm, light_dir, -getSceneWorldDirection(), blinn_phong_params), 1);
            break;
        case 3:
            vec3 ambient = vec3(mix(0.0, 0.01, clamp(dot(norm, light_dir) + 0.3, 0, 1)));
            oFragmentColor = vec4(pbr_lighting(col, norm, light_dir, -getSceneWorldDirection(), mrao, ambient), 1);
            break;
        }
	}

    vec3 cameraDirection = normalize(getSceneWorldDirection());

	// Trace atmosphere sphere
    RaySphereTraceResult hitInfo = raySphereIntersection(planetCenter, atmosphereRadius, cameraDirection, camera_pos);

	// Trace sun disc
    vec3 sunPosition = light_dir * 10000000000.0;
    RaySphereTraceResult sunInfos = raySphereIntersection(sunPosition, 100000000.0, cameraDirection, camera_pos);
    float distanceThroughSun = max(0.0, sunInfos.atmosphereDistanceOut - sunInfos.atmosphereDistanceIn);


    float outMax = min(hitInfo.atmosphereDistanceOut, linear_depth);
	float distance_to_atmosphere = hitInfo.atmosphereDistanceIn;
    float distanceThroughAtmosphere = outMax - hitInfo.atmosphereDistanceIn;

	// Draw sun disc
    if (depth <= 0) {
        oFragmentColor += vec4(1.0, 1, 1, 1.0) * distanceThroughSun / 2000;
        oFragmentColor += texture(WORLD_Cubemap, Rx(-PI / 2) * cameraDirection) * 10.2;
    }

    if (distanceThroughAtmosphere > 0.0 && enable_atmosphere != 0) {
        vec3 pointInAtmosphere = camera_pos + cameraDirection * (distance_to_atmosphere - epsilon);
        vec3 light = computeLight(pointInAtmosphere, cameraDirection, distanceThroughAtmosphere - epsilon, oFragmentColor.xyz);
        oFragmentColor = vec4(light, 1);
	}
	
}