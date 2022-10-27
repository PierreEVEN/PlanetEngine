#version 430

out vec4 oFragmentColor;
layout(location = 0) in vec2 uv;
layout(location = 1) uniform sampler2D InputTexture;
layout(location = 2) uniform sampler2D Color;

layout(location = 0) uniform ivec2 InputResolution;
layout(location = 3) uniform float bloom_strength;
layout(location = 5) uniform float step;

void main()
{
    vec3 color = texture(InputTexture, uv).rgb * 0.25;
    vec2 InputResolution = vec2(InputResolution);

    color += texture(InputTexture, uv + vec2(1, 0) / InputResolution).rgb * 0.125;
    color += texture(InputTexture, uv + vec2(0, 1) / InputResolution).rgb * 0.125;
    color += texture(InputTexture, uv + vec2(-1, 0) / InputResolution).rgb * 0.125;
    color += texture(InputTexture, uv + vec2(0, -1) / InputResolution).rgb * 0.125;

    color += texture(InputTexture, uv + vec2(1, 1) / InputResolution).rgb * 0.0625;
    color += texture(InputTexture, uv + vec2(-1, 1) / InputResolution).rgb * 0.0625;
    color += texture(InputTexture, uv + vec2(-1, -1) / InputResolution).rgb * 0.0625;
    color += texture(InputTexture, uv + vec2(1, -1) / InputResolution).rgb * 0.0625;

    oFragmentColor.rgb = color / (1 / bloom_strength) * pow(step, 0.2) + texture(Color, uv).rgb;
}