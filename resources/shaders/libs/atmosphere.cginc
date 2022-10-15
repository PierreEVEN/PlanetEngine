#ifndef ATMOSPHERE_H_
#define ATMOSPHERE_H_

#include "maths.cginc"

float getAtmosphereDensityAtLocation(vec3 location) {
	float distance_from_planet_center = length(location - planetCenter);

    float groundDistance = distance_from_planet_center - planetRadius;
	float atmosphere_altitude = atmosphereRadius - planetRadius;
    float heightFactor = groundDistance / atmosphere_altitude;
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

#endif // ATMOSPHERE_H_