#pragma once

enum class Shading {
    Diffuse = 0,
    Phong = 1,
    Blinn_Phong = 2,
    PBR = 3,
};

class GameSettings {
public:
    static GameSettings& get();

    // Misc
    bool v_sync     = true;
    bool wireframe  = false;
    bool fullscreen = false;
    int  max_fps    = 0;

    // Bloom
    float bloom_intensity = 0.5f;

    // Post Process
    float gamma    = 2.2f;
    float exposure = 1.f;

    // Lighting
    Shading shading            = Shading::PBR;
    bool    enable_atmosphere  = true;
    int     atmosphere_quality = 8;

    // Screen Space Reflection
    bool  screen_space_reflections = true;
    float ssr_quality              = 0.3f;
};
