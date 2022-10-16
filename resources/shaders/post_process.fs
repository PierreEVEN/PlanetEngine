#version 430

out vec4 oFragmentColor;
layout(location = 0) in vec2 uv;
layout(location = 1) uniform sampler2D SceneColor;

layout(location = 2) uniform float gamma;
layout(location = 3) uniform float exposure;

void main()
{
    vec3 color = textureLod(SceneColor, uv, 0).rgb;

    oFragmentColor.a = 1;

    
    

    // Tone mapping
    vec3 color_mapped = vec3(1.0) - exp(-color * exposure);

    // Gamma correction
    color = pow(color_mapped, vec3(1.0 / gamma));


    oFragmentColor.rgb = color;
}