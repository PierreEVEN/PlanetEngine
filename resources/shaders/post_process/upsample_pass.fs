#version 430

out vec4 oFragmentColor;
layout(location = 0) in vec2 uv;
layout(location = 1) uniform sampler2D LastSample;
layout(location = 2) uniform sampler2D Color;

layout(location = 0) uniform vec2 input_resolution;
layout(location = 3) uniform float bloom_strength;
layout(location = 5) uniform float step;

void main()
{
    vec3 color = texture(LastSample, uv).rgb * 0.25;

    color += texture(LastSample, uv + vec2(1, 0) / input_resolution).rgb * 0.125;
    color += texture(LastSample, uv + vec2(0, 1) / input_resolution).rgb * 0.125;
    color += texture(LastSample, uv + vec2(-1, 0) / input_resolution).rgb * 0.125;
    color += texture(LastSample, uv + vec2(0, -1) / input_resolution).rgb * 0.125;

    color += texture(LastSample, uv + vec2(1, 1) / input_resolution).rgb * 0.0625;
    color += texture(LastSample, uv + vec2(-1, 1) / input_resolution).rgb * 0.0625;
    color += texture(LastSample, uv + vec2(-1, -1) / input_resolution).rgb * 0.0625;
    color += texture(LastSample, uv + vec2(1, -1) / input_resolution).rgb * 0.0625;

    oFragmentColor.rgb = color / (1 / bloom_strength) * pow(step, 0.2) + texture(Color, uv).rgb;
    //oFragmentColor.rgb = texture(Color, uv).rgb - 0.8;
}