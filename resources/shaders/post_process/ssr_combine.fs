#version 430

#include "../libs/world_data.cginc"

out vec4 oFragmentColor;
layout(location = 0) in vec2 uv;
layout(location = 1) uniform sampler2D Input_lighting_Color;
layout(location = 2) uniform ivec2 Input_lighting_Color_Res;
layout(location = 3) uniform sampler2D Input_SSR_Color;
layout(location = 4) uniform sampler2D GBUFFER_mrao;
layout(location = 5) uniform sampler2D Input_Depth;

void main() {
    vec4 uvs = texture(Input_SSR_Color, uv);
    
    vec3 mrao = texture(GBUFFER_mrao, uv).rgb;
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

    if (ssr_uv.b <= 0)
        oFragmentColor = texture(Input_lighting_Color, uv.xy);
    else
        oFragmentColor = texture(Input_lighting_Color, ssr_uv.xy);
}
