#version 430
#include "resources/shaders/deferred.shi"
precision highp float;

out vec4 oFragmentColor;

layout(location = 0) in vec2 uv;
uniform sampler2D color;
uniform sampler2D normal;
uniform sampler2D depth;


int NumScatterPoints = 10;
int NumOpticalDepthPoints = 10;
vec3 planetCenter = vec3(0,0, -6000000);
float atmosphereRadius = 6100000;
float planetRadius = 6000000;
float atmosphereDensityFalloff = 0.5;
float scatter_strength = 1;
vec3 scatterCoefficients = pow(400 / vec3(700, 550, 460), vec3(4)) * scatter_strength;
const float epsilon = 1;

vec3 light_dir = normalize(vec3(0.5, 0, 1));

layout (std140, binding = 0) uniform WorldData
{
    mat4 proj_matrix;
    mat4 view_matrix;
    mat4 pv_matrix;
    mat4 proj_matrix_inv;
    mat4 view_matrix_inv;
    mat4 pv_matrix_inv;
	vec3 camera_pos;
	vec3 camera_forward;
    float world_time;
};






struct RaySphereTraceResult {
    float atmosphereDistanceIn;
    float atmosphereDistanceOut;
};




float atmosphereDistanceIn;
float atmosphereDistanceOut;



float getAtmosphereDensityAtLocation(vec3 location) {
	float distance_from_planet_center = length(location - planetCenter);

    float groundDistance = distance_from_planet_center - planetRadius;
	float atmosphere_altitude = atmosphereRadius - planetRadius;
    float heightFactor = clamp(groundDistance / atmosphere_altitude, 0, 1);
    return exp(-heightFactor * atmosphereDensityFalloff) * (1 - heightFactor ) / atmosphere_altitude;
}

float opticalDepth(vec3 rayOrigin, vec3 rayDir, float rayLength) {
    vec3 densitySamplePoint = rayOrigin;
    float stepSize = rayLength / (float(NumOpticalDepthPoints) - 1);
    float opticalDepthValue = 0;

    for (int i = 0; i < NumOpticalDepthPoints; ++i) {
        float localDensity = getAtmosphereDensityAtLocation(densitySamplePoint);
        opticalDepthValue += localDensity * stepSize;
        densitySamplePoint += rayDir * stepSize;
    }
    return opticalDepthValue;
}


RaySphereTraceResult raySphereIntersection(vec3 spherePosition, float sphereRadius, vec3 lineDirection, vec3 lineOrigin) {
    RaySphereTraceResult res;
    float AMdist = dot(lineDirection, spherePosition - lineOrigin);


    float AMdistAbs = abs(AMdist);

    vec3 M = lineOrigin + lineDirection * AMdist;
    float BMdist = length(spherePosition - M);
    float MCdist = sqrt(sphereRadius * sphereRadius - BMdist * BMdist);

    float ABdist = length(lineOrigin - spherePosition);

    if (ABdist <= sphereRadius) {
        res.atmosphereDistanceIn = 0.0;
        res.atmosphereDistanceOut = MCdist + AMdist;
    }
    else {
        res.atmosphereDistanceIn = abs(AMdist - MCdist);
        res.atmosphereDistanceOut = abs(AMdist + MCdist);
    }

    return res;
}

vec3 computeLight(vec3 cameraPosition, vec3 rayDir, float raylength, vec3 originalColor) {
    vec3 inScatterPoint = cameraPosition;
    float step_size = raylength / (float(NumScatterPoints - 1));
    vec3 inScatteredLight = vec3(0);
    float viewRayOpticalDepth = 0.0;

    for (int i = 0; i < NumScatterPoints; ++i) {
        RaySphereTraceResult rsResult = raySphereIntersection(planetCenter, atmosphereRadius, light_dir, inScatterPoint);
        float sunRayLength = max(0.0, rsResult.atmosphereDistanceOut - rsResult.atmosphereDistanceIn);
        float sunRayOpticalDepth = opticalDepth(inScatterPoint, light_dir, sunRayLength);
        float viewRayOpticalDepth = opticalDepth(inScatterPoint, -rayDir, step_size * float(i));
        vec3 transmittance = exp(-(sunRayOpticalDepth + viewRayOpticalDepth) * scatterCoefficients);
        float localDensity = getAtmosphereDensityAtLocation(inScatterPoint);

        inScatteredLight += localDensity * transmittance * step_size * scatterCoefficients;
        inScatterPoint += rayDir * step_size;
    }

    float colorTransmittance = exp(-viewRayOpticalDepth);

    return inScatteredLight + originalColor * colorTransmittance;
}


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