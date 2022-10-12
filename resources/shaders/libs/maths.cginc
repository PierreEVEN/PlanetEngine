#ifndef MATH_H_
#define MATH_H_

float PI = 3.14159265358979323846;
const float HALF_PI = 3.14159265358979323846 / 2.0;
const dvec2 sqrt_2_2 = dvec2(double(sqrt(2.0 / 2.0)));

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

/*
 FROM https://outerra.blogspot.com/2017/06/fp64-approximations-for-sincos-for.html 
*/

//sin approximation, error < 5e-9
double dsin(double x)
{
    //minimax coefs for sin for 0..pi/2 range
    const double a3 = -1.666665709650470145824129400050267289858e-1LF;
    const double a5 =  8.333017291562218127986291618761571373087e-3LF;
    const double a7 = -1.980661520135080504411629636078917643846e-4LF;
    const double a9 =  2.600054767890361277123254766503271638682e-6LF;

    const double m_2_pi = 0.636619772367581343076LF;
    const double m_pi_2 = 1.57079632679489661923LF;

    double y = abs(x * m_2_pi);
    double q = floor(y);
    int quadrant = int(q);

    double t = (quadrant & 1) != 0 ? 1 - y + q : y - q;
    t *= m_pi_2;

    double t2 = t * t;
    double r = fma(fma(fma(fma(a9, t2, a7), t2, a5), t2, a3), t2*t, t);

    r = x < 0 ? -r : r;

    return (quadrant & 2) != 0 ? -r : r;
}
//cos approximation, error < 5e-9
double dcos(double x)
{
    //sin(x + PI/2) = cos(x)
    return dsin(x + 1.57079632679489661923LF);
}


float cos_2(float x_f, float rho_f) {

    if (abs(x_f) > HALF_PI / 5)
        return rho_f - cos(float(x_f)) * rho_f;

    float rho = float(rho_f);
    float x = float(x_f);
    float x_2 = x * x;
    float x_4 = x_2 * x_2;
    float x_6 = x_4 * x_4;
    float x_8 = x_6 * x_6;
    float x_10 = x_8 * x_8;

    return float(
        x_2 / 2 * rho + x_4 / 24 * rho - x_6 / 720 * rho + x_8 / 40320 * rho - x_10 / 3628800 * rho
    );
}

float cos_3(float x_f) {

    if (abs(x_f) > HALF_PI / 5)
        return cos(float(x_f)) - 1;

    float x = float(x_f);
    float x_2 = x * x;
    float x_4 = x_2 * x_2;
    float x_6 = x_4 * x_4;
    float x_8 = x_6 * x_6;
    float x_10 = x_8 * x_8;

    return float(
        -x_2 / 2 + x_4 / 24 - x_6 / 720 + x_8 / 40320 - x_10 / 3628800
    );
}

float sin_2(float x_f, float rho_f) {

    if (abs(x_f) > HALF_PI / 5)
        return sin(float(x_f)) * rho_f;


    float rho = float(rho_f);
    float x = float(x_f);
    float x_3 = x * x * x;
    float x_5 = x_3 * x_3;
    float x_7 = x_5 * x_5;
    float x_9 = x_7 * x_7;
    float x_11 = x_9 * x_9;

    return float(
        x * rho - x_3 / 6 * rho + x_5 / 120 * rho - x_7 / 5040 * rho + x_9 / 362880 * rho// - x_11 / 39916800 * rho
    );
}

vec3 to_3d_v4(vec2 pos, float rho) {
    vec2 dpos = pos;
	vec2 norm_pos = clamp(dpos / rho, -HALF_PI, HALF_PI);
    float cos_y = cos(norm_pos.y);
    return vec3(
        (cos_3(norm_pos.y)) * rho - cos_y * cos_2(norm_pos.x, rho),
        cos_y * sin(norm_pos.x) * rho, 
        sin (norm_pos.y) * rho
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

#endif // MATH_H_