#ifndef LANDSCAPE_H_
#define LANDSCAPE_H_

#include "noise.cginc"


float get_height_at_location(dvec3 pos) {
        double glob_height = cnoise(pos * 2) * 1000.0 + cnoise(pos * 10) * 100.0;

        if (glob_height > 0) {

            // hill mask
            double hill = (cnoise(pos * 500) * 0.5) + 0.5;
            double large_hill = (cnoise(pos * 20) * 0.5) + 0.5;

            // highlands
            double highlands = ((cnoise(pos * 50.0)));
            highlands = pow(float(abs(highlands)), 0.8);

            highlands *=  clamp(glob_height / 900, 0 , 1);

            // Compute transition between ocean and ground
            double transition_scale = (cnoise(pos * 1000) * 0.5 + 0.5) * pow(float(1 - large_hill), 2);
            float transition = float(clamp(glob_height * transition_scale, 0, 1));



            return float(
                mix(glob_height, 
                glob_height +
                    large_hill * 2000 +
                    hill * 500 +
                    highlands * 10000
                
                , 
                transition)
            );
        }

        return float(glob_height);

        return float(
            glob_height +
            cnoise(pos * 10) * 1000.0 +
            cnoise(pos * 1000) * 100.0 +
            cnoise(pos * 10000) * 10.0 +
            cnoise(pos * 100000) * 10.0
        );
}

float altitude_with_water(float altitude) {
	return altitude < 0 ? 0 : altitude;
}

#endif // LANDSCAPE_H_