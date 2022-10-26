#version 430

#include "../libs/world_data.cginc"

out vec4 oFragmentColor;
layout(location = 0) in vec2 uv;
layout(location = 1) uniform sampler2D InputTexture;
layout(location = 2) uniform vec2 InputResolution;
layout(location = 3) uniform sampler2D SSR_Coordinates;
layout(location = 4) uniform sampler2D GBUFFER_mrao;

void main() {
    vec4 uvs = texture(SSR_Coordinates, uv);
    
    vec3 mrao = texture(GBUFFER_mrao, uv).rgb;
    if (uvs.b == 10 || mrao.g >= 0.2) {
        oFragmentColor = texture(InputTexture, uv);
        return;
    }

    oFragmentColor = mix(texture(InputTexture, uvs.xy), texture(InputTexture, uv), mrao.g);
    
    int   size       = 6;
    float separation = 2.0;

    vec2 texSize  = InputResolution;
    vec2 texCoord = gl_FragCoord.xy / texSize;
    vec4 uv = texture(SSR_Coordinates, texCoord);

    // Removes holes in the UV map.
    if (uv.b <= 0.0) {
        uv = vec4(0.0);
        float count = 0.0;

        for (int i = -size; i <= size; ++i) {
            for (int j = -size; j <= size; ++j) {
                uv  += texture(SSR_Coordinates, ((vec2(i, j) * separation) + gl_FragCoord.xy) / texSize);
                count += 1.0;
            }
        }

        uv.xyz /= count;
    }

    vec4  color = texture(InputTexture, uv.xy);
    oFragmentColor = color;
}
