#ifndef MATH_H_
#define MATH_H_

float PI = 3.14159265358979323846;
const float HALF_PI = 3.14159265358979323846 / 2.0;

dvec2 seamless_uv_from_sphere_normal(dvec3 sphere_norm) {
    dvec3 abs_norm = abs(sphere_norm);
    
    const dvec2 sqrt_2_2 = dvec2(sqrt(double(2)) / double(2));

    if (abs_norm.x > abs_norm.y && abs_norm.x > abs_norm.z)
        return (
                sphere_norm.yz * (sphere_norm.x < 0 ? dvec2(1) : dvec2(-1, 1)) /
                sqrt_2_2 + 1
            ) / 2;
        
    if (abs_norm.y > abs_norm.x && abs_norm.y > abs_norm.z)
        return (
                sphere_norm.xz * (sphere_norm.y < 0 ? dvec2(1) : dvec2(-1, 1)) /
                sqrt_2_2 + 1
            ) / 2;

    if (abs_norm.z > abs_norm.x && abs_norm.z > abs_norm.y)
    {
        if (sphere_norm.z > 0)
            if (abs_norm.x > abs_norm.y)
                if (sphere_norm.x > 0)
                    return (dvec2(-sphere_norm.y, sphere_norm.x) / sqrt_2_2 + dvec2(1, 1)) / 2;
                else
                    return (dvec2(sphere_norm.y, -sphere_norm.x) / sqrt_2_2 + dvec2(1, 1)) / 2;
            else
                if (sphere_norm.y > 0)
                    return (dvec2(-sphere_norm.x, sphere_norm.y) / sqrt_2_2 + dvec2(1, 1)) / 2;
                else 
                    return (dvec2(sphere_norm.x, -sphere_norm.y) / sqrt_2_2 + dvec2(1, 1)) / 2;
        else
            if (abs_norm.x > abs_norm.y)
                if (sphere_norm.x > 0)
                    return (dvec2(-sphere_norm.y, -sphere_norm.x) / sqrt_2_2 + dvec2(1, 1)) / 2;
                else
                    return (dvec2(sphere_norm.y, sphere_norm.x) / sqrt_2_2 + dvec2(1, 1)) / 2;
            else
                if (sphere_norm.y > 0)
                    return (dvec2(-sphere_norm.x, -sphere_norm.y) / sqrt_2_2 + dvec2(1, 1)) / 2;
                else 
                    return (dvec2(sphere_norm.x, sphere_norm.y) / sqrt_2_2 + dvec2(1, 1)) / 2;
    }
    return dvec2(0, 0);
}

dvec2 uv_from_sphere_normal(dvec3 sphere_norm) {
    dvec3 abs_norm = abs(sphere_norm);
    
    const dvec2 sqrt_2_2 = dvec2(sqrt(double(2)) / double(2));

    if (abs_norm.x > abs_norm.y && abs_norm.x > abs_norm.z)
        return (
                sphere_norm.yz * (sphere_norm.x < 0 ? dvec2(1) : dvec2(-1, 1)) /
                sqrt_2_2 + 1
            ) / 2;
        
    if (abs_norm.y > abs_norm.x && abs_norm.y > abs_norm.z)
        return (
                sphere_norm.xz * (sphere_norm.y < 0 ? dvec2(1) : dvec2(-1, 1)) /
                sqrt_2_2 + 1
            ) / 2;

    if (abs_norm.z > abs_norm.x && abs_norm.z > abs_norm.y)
    {
        return (dvec2(sphere_norm.y, sphere_norm.x) / sqrt_2_2 + dvec2(1, 1)) / 2;
    }
    return dvec2(0, 0);
}


struct RaySphereTraceResult {
    float atmosphereDistanceIn;
    float atmosphereDistanceOut;
};

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

#endif // MATH_H_