#ifndef MATH_H_
#define MATH_H_

float PI = 3.14159265358979323846;
const float HALF_PI = 3.14159265358979323846 / 2.0;
const dvec2 sqrt_2_2 = dvec2(double(sqrt(2.0 / 2.0)));

mat3 rotation_from_mat4(mat4 transformation) {
    mat3 rot;
    rot[0] = normalize(transformation[0].xyz);
    rot[1] = normalize(transformation[1].xyz);
    rot[2] = normalize(transformation[2].xyz);
    return rot;
}


dvec2 seamless_uv_from_sphere_normal(dvec3 sphere_norm) {
    dvec3 abs_norm = abs(sphere_norm);

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

// return cos(x) * rho
float scaled_cos(float x, float rho) {
    if (abs(x) > HALF_PI / 5)
        return cos(x) * rho;

    float x_2 = x * x;
    float x_4 = x_2 * x_2;
    float x_6 = x_4 * x_4;
    float x_8 = x_6 * x_6;
    float x_10 = x_8 * x_8;

    return rho - x_2 / 2 * rho + x_4 / 24 * rho - x_6 / 720 * rho + x_8 / 40320 * rho - x_10 / 3628800 * rho;
}

// return rho - cos(x)
float rho_minus_cos(float x, float rho) {

    if (abs(x) > HALF_PI / 5)
        return rho - cos(x) * rho;

    float x_2 = x * x;
    float x_4 = x_2 * x_2;
    float x_6 = x_4 * x_4;
    float x_8 = x_6 * x_6;
    float x_10 = x_8 * x_8;

    return x_2 / 2 * rho + x_4 / 24 * rho - x_6 / 720 * rho + x_8 / 40320 * rho - x_10 / 3628800 * rho;
}

// return cos(x) - 1
float cos_minus_one(float x) {

    if (abs(x) > HALF_PI / 5)
        return cos(x) - 1;

    float x_2 = x * x;
    float x_4 = x_2 * x_2;
    float x_6 = x_4 * x_4;
    float x_8 = x_6 * x_6;
    float x_10 = x_8 * x_8;

    return -x_2 / 2 + x_4 / 24 - x_6 / 720 + x_8 / 40320 - x_10 / 3628800;
}

// return sin(x)
float sin_2(float x, float rho) {
    if (abs(x) > HALF_PI / 5)
        return sin(x) * rho;

    float x_3 = x * x * x;
    float x_5 = x_3 * x_3;
    float x_7 = x_5 * x_5;
    float x_9 = x_7 * x_7;

    return x * rho - x_3 / 6 * rho + x_5 / 120 * rho - x_7 / 5040 * rho + x_9 / 362880 * rho;
}

// Transform 2D position into 3D sphere position. the sphere center is vec3(-rho, 0, 0)
vec3 grid_to_sphere(vec2 pos, float rho) {
	vec2 norm_pos = pos / rho;
    float cos_y = cos_minus_one(norm_pos.y);
    float sin_x = sin_2(norm_pos.x, rho);
    return vec3(
        cos_y * rho - scaled_cos(norm_pos.y, 1) * rho_minus_cos(norm_pos.x, rho),
        cos_y * sin_x + sin_x,
        sin_2(norm_pos.y, rho)
    );
}

// Transform 2D position into 3D sphere position. the sphere center is vec3(0, 0, 0)
vec3 grid_to_sphere_centered(vec2 pos, float rho) {
	vec2 norm_pos = pos / rho;
    return vec3(
        cos(norm_pos.y) * cos(norm_pos.x) * rho,
        cos(norm_pos.y) * sin(norm_pos.x) * rho,
        sin(norm_pos.y) * rho
    );
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

float clamp_01(float a) { return clamp(a, 0, 1); }
vec2 clamp_01(vec2 a) { return clamp(a, vec2(0), vec2(1)); }
vec3 clamp_01(vec3 a) { return clamp(a, vec3(0), vec3(1)); }
vec4 clamp_01(vec4 a) { return clamp(a, vec4(0), vec4(1)); }

#endif // MATH_H_