#ifndef LANDSCAPE_H_
#define LANDSCAPE_H_

#include "noise.cginc"
#include "random.cginc"

float get_height_at_location_v2(vec3 pos) {
    return simplex_noise(pos + vec3(1, 1.005, 0), 17, 0.65, 1.75) * 30000 ;
}

#endif // LANDSCAPE_H_