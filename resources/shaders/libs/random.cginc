#ifndef RANDOM_H_
#define RANDOM_H_

#include "maths.cginc"

float rand(float pos) {
    const float sizel = 367023.89362;
    return fract(sin(pos) * sizel);
}

float rand(vec2 pos) {
    const float u = 53.2978389725796;
    const float v = 3.78309903253987;
    const float sizel = 367023.89362;
    return fract(sin(dot(pos.xy, vec2(u, v))) * sizel);
}

float noise(float pos) {
    const float i = floor(pos);
    const float f = fract(pos);
    
    const float u = cubic_hermine_curve(f);

    return mix(rand(i), rand(i + 1.0), u);    
}

float noise(vec2 pos) {
    const vec2 i = floor(pos);
    const vec2 f = fract(pos);

    const float p = rand(i);
    const float q = rand(i + vec2(1.0, 0.0));
    const float r = rand(i + vec2(0.0, 1.0));
    const float s = rand(i + vec2(1.0, 1.0));

    const vec2 u = cubic_hermine_curve(f);

    return mix(p, q, u.x) +
            (r - p)* u.y * (1.0 - u.x) +
            (s - q) * u.x * u.y;
}

#endif // RANDOM_H_