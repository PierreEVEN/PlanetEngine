#ifndef LANDSCAPE_H_
#define LANDSCAPE_H_

#include "noise.cginc"



float get_height_at_location__b(vec3 pos) {
        float glob_height = float(cnoise(pos * 2)) * 1000.0 + cnoise(pos * 10) * 100.0;

        if (glob_height > 0) {

            // hill mask
            float hill = (float(cnoise(pos * 500)) * 0.5) + 0.5;
            float large_hill = (float(cnoise(pos * 20)) * 0.5) + 0.5;

            // highlands
            float highlands = (float(cnoise(pos * 50.0)));
            highlands = pow(float(abs(highlands)), 0.8);

            highlands *=  clamp(glob_height / 900, 0 , 1);

            // Compute transition between ocean and ground
            float transition_scale = (float(cnoise(pos * 1000)) * 0.5 + 0.5) * pow(float(1 - large_hill), 2);
            float transition = float(clamp(glob_height * transition_scale, 0, 1));
            float transition_beach = float(clamp(glob_height * 20, 0, 1));


            float local_noise = float(cnoise(pos * 20000)) * 0.5 + 0.5;
            float mini_noise = float(cnoise(pos * 100000)) * 0.5 + 0.5;

            return float(
                mix(glob_height, 
                glob_height +
                    large_hill * 2000 +
                    hill * 500 +
                    highlands * 10000
                , 
                transition)  + 
                mix (0,
                    local_noise * 30 +
                    mini_noise * 5,
                transition_beach)
            );
        }

        return float(glob_height * 1000);
}

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


float get_height_at_location(vec3 pos) {
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

        float land = 50 + highlands * 10000 + hill * 500;// + mini_noise * 5;

        return mix(continents, land, clamp_01(transition));
    }


    return continents * 1000;
}

#endif // LANDSCAPE_H_