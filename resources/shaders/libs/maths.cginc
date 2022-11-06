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


mat3 Rx(float theta) {    
    mat3 _rx;
    _rx[0] = vec3(1,0,0);
    _rx[1] = vec3(0,cos(theta),-sin(theta));
    _rx[2] = vec3(0,sin(theta),cos(theta));
    return _rx;
}

mat3 Ry(float theta) {    
    mat3 _ry;
    _ry[0] = vec3(cos(theta),0,sin(theta));
    _ry[1] = vec3(0, 1, 0);
    _ry[2] = vec3(-sin(theta),0,cos(theta));
    return _ry;
}

mat3 Rz(float theta) {    
    mat3 _rz;
    _rz[0] = vec3(cos(theta),-sin(theta), 0);
    _rz[1] = vec3(-sin(theta),cos(theta), 0);
    _rz[2] = vec3(0, 0, 1);
    return _rz;
}

vec3 unpack_normal(vec2 packed_normal) {
    return vec3(packed_normal, sqrt(1 - packed_normal.x * packed_normal.x - packed_normal.y * packed_normal.y));
}

vec3 unpack_tangent_z(float packed_tangent_z) {
    return vec3(sqrt(1 - packed_tangent_z * packed_tangent_z), 0, packed_tangent_z);
}

vec3 unpack_bi_tangent_z(float packed_bi_tangent_z) {
    return vec3(0, sqrt(1 - packed_bi_tangent_z * packed_bi_tangent_z), packed_bi_tangent_z);
}

float cubic_hermine_curve(float f) {
    return f * f * (3 - 2 * f);
}
vec2 cubic_hermine_curve(vec2 f) {
    return f * f * (3 - 2 * f);
}
vec3 cubic_hermine_curve(vec3 f) {
    return f * f * (3 - 2 * f);
}
vec4 cubic_hermine_curve(vec4 f) {
    return f * f * (3 - 2 * f);
}

float quintic_hermine_curve(float f) {
    return f * f * f * (f * (f * 6 - 15) + 10);
}
vec2 quintic_hermine_curve(vec2 f) {
    return f * f * f * (f * (f * 6 - 15) + 10);
}
vec3 quintic_hermine_curve(vec3 f) {
    return f * f * f * (f * (f * 6 - 15) + 10);
}
vec4 quintic_hermine_curve(vec4 f) {
    return f * f * f * (f * (f * 6 - 15) + 10);
}
#endif // MATH_H_