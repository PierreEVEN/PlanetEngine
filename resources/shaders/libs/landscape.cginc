#ifndef LANDSCAPE_H_
#define LANDSCAPE_H_

#include "noise.cginc"


float get_height_at_location(vec3 pos) {
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

#endif // LANDSCAPE_H_