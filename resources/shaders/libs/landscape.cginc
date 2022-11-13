#ifndef LANDSCAPE_H_
#define LANDSCAPE_H_

#include "noise.cginc"
#include "random.cginc"

float normalized_layer(vec3 pos, float scale) {
    return fma(cnoise(pos * scale), 0.5, 0.5);
}

float layer(vec3 pos, float scale) {
    return cnoise(pos * scale);
}

float scaled_layer(vec3 pos, float scale, float max_height) {
    return normalized_layer(pos, scale) * max_height;
}


float make_continents(vec3 pos) {
    return layer(pos, 2) + fma(layer(pos, 10), 0.1, -0.05) + fma(layer(pos, 200), 0.05, -0.05);
}


float get_height_at_location_v1(vec3 pos) {
    float continents = make_continents(pos);

    if (continents > 0) {
        float small_hill = layer(pos, 500);
        float large_hill = normalized_layer(pos, 100);
        float mini_hill = layer(pos, 3000);
        float hill = (small_hill * 0.3 + large_hill * 0.4 + mini_hill * 0.3);

        // Make montains more present far from coasts
        float mountain_level = clamp_01(fma(pow(continents, 3), 10, layer(pos, 32) * 0.01));

        // highlands
        float highlands = mix(0, abs(layer(pos, 16.0)), mountain_level);

        float mini_noise = normalized_layer(pos, 100000);

        // Compute transition between ocean and ground
        float transition = clamp_01(0.5 - cos(clamp_01((continents) * 300 + layer(pos, 3000) * 0.3) * PI) * 0.5 - 0.1);

        float land = 50 + highlands * 10000 + hill * 500 + mini_noise * 5;

        return mix(continents, land, clamp_01(transition));
    }


    return continents * 1000;
}


float get_height_at_location_v2(vec3 pos) {


    return simplex_noise(pos, 17, 0.65, 1.75) * 10000;



    float continents = make_continents(pos);

    if (continents > 0) {
        float small_hill = layer(pos, 500);
        float large_hill = normalized_layer(pos, 100);
        float mini_hill = layer(pos, 3000);
        float hill = (small_hill * 0.3 + large_hill * 0.4 + mini_hill * 0.3);

        // Make montains more present far from coasts
        float mountain_level = clamp_01(fma(pow(continents, 3), 10, layer(pos, 32) * 0.01));

        // highlands
        float highlands = mix(0, abs(layer(pos, 16.0)), mountain_level);

        float mini_noise = normalized_layer(pos, 100000);

        // Compute transition between ocean and ground
        float transition = clamp_01(0.5 - cos(clamp_01((continents) * 300 + layer(pos, 3000) * 0.3) * PI) * 0.5 - 0.1);

        float land = 50 + highlands * 10000 + hill * 500 + mini_noise * 5;

        return mix(continents, land, clamp_01(transition));
    }


    return continents * 1000;
}

#endif // LANDSCAPE_H_