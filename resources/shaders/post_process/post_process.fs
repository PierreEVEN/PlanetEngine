#version 430

out vec4 oFragmentColor;
layout(location = 0) in vec2 uv;
layout(location = 2) uniform sampler2D Input_Color;

layout(location = 3) uniform float gamma;
layout(location = 4) uniform float exposure;

#include "../libs/random.cginc"


vec3 aces_tonemap(vec3 color){	
	mat3 m1 = mat3(
        0.59719, 0.07600, 0.02840,
        0.35458, 0.90834, 0.13383,
        0.04823, 0.01566, 0.83777
	);
	mat3 m2 = mat3(
        1.60475, -0.10208, -0.00327,
        -0.53108,  1.10813, -0.07276,
        -0.07367, -0.00605,  1.07602
	);
	vec3 v = m1 * color;    
	vec3 a = v * (v + 0.0245786) - 0.000090537;
	vec3 b = v * (0.983729 * v + 0.4329510) + 0.238081;
	return pow(clamp(m2 * (a / b), 0.0, 1.0), vec3(1.0 / 2.2));	
}

void main()
{
    vec3 color = max(vec3(0), texture(Input_Color, uv).rgb);
    oFragmentColor.a = 1;

    // Tone mapping
    //color = vec3(1.0) - exp(-color * exposure);

    // Gamma correction
    //color = pow(color, vec3(1.0 / gamma));

    color = aces_tonemap(color);


    oFragmentColor.rgb = color;
}