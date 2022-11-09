#version 430

#include "../libs/world_data.cginc"
#include "../libs/maths.cginc"

out vec4 oFragmentColor;
layout(location = 0) in vec2 uv;
layout(location = 1) uniform sampler2D Input_lighting_Color;
layout(location = 2) uniform ivec2 Input_lighting_Color_Res;
layout(location = 3) uniform sampler2D Input_SSR_Color;
layout(location = 4) uniform sampler2D Input_normal;
layout(location = 5) uniform sampler2D Input_mrao;
layout(location = 6) uniform sampler2D Input_Depth;
layout(location = 7) uniform samplerCube ENV_cubemap;

vec3 getSceneWorldDirection() {
    // compute clip space direction
    vec4 clipSpacePosition = vec4(uv * 2.0 - 1.0, 1.0, 1.0);

    // Transform local space to view space
    vec4 viewSpacePosition = proj_matrix_inv * clipSpacePosition;

    viewSpacePosition /= viewSpacePosition.w;

    // Transform view space to world space
    vec4 worldSpacePosition = view_matrix_inv * viewSpacePosition;
    return normalize(worldSpacePosition.xyz);
}

void main() {
    vec4 uvs = texture(Input_SSR_Color, uv);
    
    vec3 mrao = texture(Input_mrao, uv).rgb;
    float depth = texture(Input_Depth, uv).r;
    if (uvs.b == 10 || mrao.g >= 0.2 || depth <= 0) {
        oFragmentColor = texture(Input_lighting_Color, uv);
        return;
    }

    oFragmentColor = mix(texture(Input_lighting_Color, uvs.xy), texture(Input_lighting_Color, uv), mrao.g);
    
    int   size       = 2;
    float separation = 2;

    vec2 texSize  = vec2(Input_lighting_Color_Res);
    vec2 texCoord = gl_FragCoord.xy / texSize;
    vec4 ssr_uv = texture(Input_SSR_Color, texCoord);

    // Removes holes in the UV map.
    if (ssr_uv.b <= 0.0) {
        ssr_uv = vec4(0.0);
        float count = 0.0;

        for (int i = -size; i <= size; ++i) {
            for (int j = -size; j <= size; ++j) {
                vec4 new_uv = texture(Input_SSR_Color, ((vec2(i, j) * separation) + gl_FragCoord.xy) / texSize);
                if (new_uv.b > 0) {
                    ssr_uv += new_uv;
                    count += 1.0;
                }
            }
        }

        ssr_uv.xy /= count;
    }

    if (ssr_uv.b <= 0) {
        vec3 normal = texture(Input_normal, uv).xyz;
        oFragmentColor = texture(ENV_cubemap, Rx(-PI / 2) * reflect(normalize(getSceneWorldDirection()), normal));
        return;
    }
    else
        oFragmentColor = texture(Input_lighting_Color, ssr_uv.xy);
}
