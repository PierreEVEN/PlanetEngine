#version 430

out vec4 oFragmentColor;
layout(location = 0) in vec2 uv;
layout(location = 2) uniform sampler2D Input_Color;

layout(location = 3) uniform float gamma;
layout(location = 4) uniform float exposure;

#include "../libs/random.cginc"

void main()
{
    vec3 color = max(vec3(0), texture(Input_Color, uv).rgb);
    oFragmentColor.a = 1;

    // Tone mapping
    color = vec3(1.0) - exp(-color * exposure);

    // Gamma correction
    color = pow(color, vec3(1.0 / gamma));

    float s = 45;

    vec2 y = vec2(noise(uv * s).x, 0);


    color = vec3(y, 0);

    oFragmentColor.rgb = color;
}