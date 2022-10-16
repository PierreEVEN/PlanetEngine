#version 430

out vec4 oFragmentColor;
layout(location = 0) in vec2 uv;
layout(location = 1) uniform sampler2D Color;
layout(location = 2) uniform vec2 input_resolution;

void main()
{
    vec3 color2 = vec3(0);
    vec3 color = vec3(0);

    color2 += textureLod(Color, uv + vec2(0, 2) / input_resolution, 0).rgb;
    color2 += textureLod(Color, uv + vec2(-2, 0) / input_resolution, 0).rgb;
    color2 += textureLod(Color, uv + vec2(2, 0) / input_resolution, 0).rgb;
    color2 += textureLod(Color, uv + vec2(0, -2) / input_resolution, 0).rgb;

    color += textureLod(Color, uv + vec2(1, 1) / input_resolution, 0).rgb;
    color += textureLod(Color, uv + vec2(-1, 1) / input_resolution, 0).rgb;
    color += textureLod(Color, uv + vec2(-1, -1) / input_resolution, 0).rgb;
    color += textureLod(Color, uv + vec2(1, -1) / input_resolution, 0).rgb;

    oFragmentColor.rgb = color * 0.5 + color2 * 0.125;
}