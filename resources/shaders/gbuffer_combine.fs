#version 430
precision highp float;

#include "libs/deferred_input.cginc"
#include "libs/world_data.cginc"

out vec4 oFragmentColor;

layout(location = 0) in vec2 uv;


int NumScatterPoints = 5;
int NumOpticalDepthPoints = 5;
vec3 planetCenter = vec3(0,0, -6000000);
float atmosphereRadius = 6100000;
float planetRadius = 6000000;
float atmosphereDensityFalloff = 0.5;
float scatter_strength = 1;
vec3 scatterCoefficients = pow(400 / vec3(700, 550, 460), vec3(4)) * scatter_strength;
const float epsilon = 1;

vec3 light_dir = normalize(vec3(0.5, 0, 1));

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
	float depth = texture(depth, uv).r;
	float linear_depth = 1 / depth;
	if (depth <= 0) {
		oFragmentColor = vec4(0);
	}
	else {
		vec3 col = texture(color, uv).rgb;
		vec3 norm = normalize(texture(normal, uv).rgb);
		oFragmentColor = vec4(col * max(0, dot(norm, light_dir)), 1);
	}

    vec3 cameraDirection = normalize(getSceneWorldDirection());

	// Trace atmosphere sphere
    RaySphereTraceResult hitInfo = raySphereIntersection(planetCenter, atmosphereRadius, cameraDirection, camera_pos);

	// Trace sun disc
    vec3 sunPosition = light_dir * 10000000000.0;
    RaySphereTraceResult sunInfos = raySphereIntersection(sunPosition, 500000000.0, cameraDirection, camera_pos);
    float distanceThroughSun = max(0.0, sunInfos.atmosphereDistanceOut - sunInfos.atmosphereDistanceIn);


    float outMax = min(hitInfo.atmosphereDistanceOut, linear_depth);
	float distance_to_atmosphere = hitInfo.atmosphereDistanceIn;
    float distanceThroughAtmosphere = outMax - hitInfo.atmosphereDistanceIn;


    if (distanceThroughAtmosphere > 0.0) {
        vec3 pointInAtmosphere = camera_pos + cameraDirection * (distance_to_atmosphere - epsilon);
        vec3 light = computeLight(pointInAtmosphere, cameraDirection, distanceThroughAtmosphere - epsilon, oFragmentColor.xyz);
        oFragmentColor = vec4(light, 1);
	}
	
	// Draw sun disc
    if (depth <= 0) oFragmentColor += vec4(1.0, .5, .2, 1.0) * distanceThroughSun / 200000000;
}