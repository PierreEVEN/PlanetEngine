#version 430
precision highp float;

#include "../libs/world_data.cginc"
#include "../libs/lighting.cginc"

layout(location = 0) in vec2 uv;
layout(location = 0) out vec4 oFragmentColor;
layout(location = 1) out vec3 oNormal;
layout(location = 2) out vec3 oMrao;
layout(location = 3) out float oDepth;

layout(location = 1) uniform sampler2D Scene_color;
layout(location = 2) uniform sampler2D Scene_normal;
layout(location = 3) uniform sampler2D Scene_mrao;
layout(location = 4) uniform sampler2D Scene_depth;

layout(location = 5) uniform sampler2D Translucency_color;
layout(location = 6) uniform sampler2D Translucency_normal;
layout(location = 7) uniform sampler2D Translucency_mrao;
layout(location = 8) uniform sampler2D Translucency_depth;

layout(location = 9) uniform float z_near;
layout(location = 10) uniform int shading;

vec3 light_dir = normalize(vec3(1, 0, 0));

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

void main()
{

    vec3 scene_albedo = texture(Scene_color, uv).rgb;
    vec3 scene_normal = normalize(texture(Scene_normal, uv).rgb);
    vec3 scene_mrao = texture(Scene_mrao, uv).rgb;
	float scene_depth = texture(Scene_depth, uv).r;

    vec4 trans_albedo = texture(Translucency_color, uv);
    vec3 trans_normal = normalize(texture(Translucency_normal, uv).rgb);
    vec3 trans_mrao = texture(Translucency_mrao, uv).rgb;
	float trans_depth = texture(Translucency_depth, uv).r;

    vec3 ground_color = surface_shading(shading, scene_albedo, scene_normal, scene_mrao, light_dir, getSceneWorldDirection());

    vec3 trans_color = surface_shading(shading, trans_albedo.rgb, trans_normal, trans_mrao, light_dir, getSceneWorldDirection());

    
	oFragmentColor = vec4(ground_color, 1);
    oNormal = scene_normal;
    oMrao = scene_mrao;
    oDepth = scene_depth;

    if (trans_depth > scene_depth) {
        oFragmentColor = mix(oFragmentColor, vec4(trans_color, 1), trans_albedo.a);
        oNormal = trans_normal;
        oMrao = trans_mrao;
        oDepth = trans_depth;
    }
}