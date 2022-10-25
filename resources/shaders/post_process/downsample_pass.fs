#version 430

out vec4 oFragmentColor;
layout(location = 0) in vec2 uv;
layout(location = 1) uniform sampler2D InputTexture;
layout(location = 2) uniform vec2 InputResolution;

void main()
{
    vec3 color2 = vec3(0);
    vec3 color1 = vec3(0);

    color2 += texture(InputTexture, uv + vec2(0, 2) / InputResolution).rgb;
    color2 += texture(InputTexture, uv + vec2(-2, 0) / InputResolution).rgb;
    color2 += texture(InputTexture, uv + vec2(2, 0) / InputResolution).rgb;
    color2 += texture(InputTexture, uv + vec2(0, -2) / InputResolution).rgb;
    color2/= 4;

    color1 = texture(InputTexture, uv + vec2(1, 1) / InputResolution).rgb;
    color1 = texture(InputTexture, uv + vec2(-1, 1) / InputResolution).rgb;
    color1 = texture(InputTexture, uv + vec2(-1, -1) / InputResolution).rgb;
    color1 = texture(InputTexture, uv + vec2(1, -1) / InputResolution).rgb;
    color1/= 4;

    oFragmentColor.rgb = max(vec3(0), color1 * 0.5 + color2 * 0.5);
}