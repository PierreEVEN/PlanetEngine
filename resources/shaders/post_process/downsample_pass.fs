#version 430

out vec4 oFragmentColor;
layout(location = 0) in vec2 uv;
layout(location = 1) uniform sampler2D Color;
layout(location = 2) uniform vec2 input_resolution;

void main()
{
    vec3 color2 = vec3(0);
    vec3 color1 = vec3(0);

    color2 += texture(Color, uv + vec2(0, 2) / input_resolution).rgb;
    color2 += texture(Color, uv + vec2(-2, 0) / input_resolution).rgb;
    color2 += texture(Color, uv + vec2(2, 0) / input_resolution).rgb;
    color2 += texture(Color, uv + vec2(0, -2) / input_resolution).rgb;
    color2/= 4;

    color1 = texture(Color, uv + vec2(1, 1) / input_resolution).rgb;
    color1 = texture(Color, uv + vec2(-1, 1) / input_resolution).rgb;
    color1 = texture(Color, uv + vec2(-1, -1) / input_resolution).rgb;
    color1 = texture(Color, uv + vec2(1, -1) / input_resolution).rgb;
    color1/= 4;

    oFragmentColor.rgb = color1 * 0.5 + color2 * 0.5;
}