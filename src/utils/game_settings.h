#pragma once

class GameSettings {
public:
    static GameSettings& get();


    float bloom_intensity = 0.5f;
    float gamma           = 2.2f;
    float exposure        = 1.f;
    bool  v_sync          = true;
    bool  wireframe       = false;
    int   max_fps         = 0;
    bool  fullscreen      = false;
};
