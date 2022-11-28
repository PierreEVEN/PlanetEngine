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
layout(location = 9) uniform ivec2 Translucency_depth_Res;

layout(location = 10) uniform float z_near;
layout(location = 11) uniform int shading;
layout(location = 12) uniform vec3 sun_direction;

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

vec3 getSceneWorldPosition(vec2 uvs, float depth) {
    if (depth <= 0)
        return vec3(0);
    // compute clip space depth
    vec4 clipSpacePosition = vec4(uvs * 2.0 - 1.0, depth, 1.0);

    // Transform local space to view space
    vec4 viewSpacePosition = proj_matrix_inv * clipSpacePosition;
    viewSpacePosition /= viewSpacePosition.w;

    // Transform view space to world space
    vec4 worldSpacePosition = view_matrix_inv * viewSpacePosition;
    return worldSpacePosition.xyz / worldSpacePosition.w;
}

vec2 screen_space_refraction(vec2 uv_start, sampler2D depth_map, vec3 initial_pos, vec3 initial_dir, vec3 normal, float ior) {
    const float maxDistance = 10000;
    const vec2 Input_normal_Res = Translucency_depth_Res;
    const int max_iterations = 100;
    const float resolution = 0.3;

    vec3 camera_to_pixel = initial_dir;
    vec3 reflected_ray = refract(camera_to_pixel, normal, ior);

    // Compute reflection end point from world space to screen space
    vec3 world_start = initial_pos;
    vec3 world_end = world_start + reflected_ray * maxDistance;

    if (dot(camera_to_pixel, world_end) < 0) { 
        return uv_start; 
    }

    
    vec4 end_screen_space = pv_matrix * vec4(world_end, 1.0);
    vec2 ray_end = (end_screen_space.xy / end_screen_space.w * 0.5 + 0.5);

    float delta = max(abs(ray_end.x - uv_start.x) * Input_normal_Res.x, abs(ray_end.y - uv_start.y) * Input_normal_Res.y);
    int sp_steps = min(max_iterations, int(delta * clamp_01(resolution)));

    vec2 out_uv = uv;
    int finished = 0;
    float pixel_depth = 0;
    
    for (float i = 1; i <= sp_steps; ++i) {
        // Compare pixel depth vs expected depth
        vec3 expected_world_pos = mix(world_start, world_end, i / float(sp_steps));
        vec4 screen_space_position = pv_matrix * vec4(expected_world_pos, 1);
        out_uv = fma(screen_space_position.xy / screen_space_position.w, vec2(0.5), vec2(0.5));

        float expected_depth = length(expected_world_pos);
        pixel_depth = length(getSceneWorldPosition(out_uv, texture(Scene_depth, out_uv).r));

        if (pixel_depth > 0 && pixel_depth < expected_depth) {
            finished = 1;
            break;
        }
    }

    return out_uv;
}

vec2 screen_space_reflection(vec2 uv_start, sampler2D depth_map, vec3 initial_pos, vec3 initial_dir, vec3 normal) {
    const float maxDistance = 10000;
    const vec2 Input_normal_Res = Translucency_depth_Res;
    const int max_iterations = 100;
    const float resolution = 0.3;

    vec3 camera_to_pixel = initial_dir;
    vec3 reflected_ray = reflect(camera_to_pixel, normal);

    // Compute reflection end point from world space to screen space
    vec3 world_start = initial_pos;
    vec3 world_end = world_start + reflected_ray * maxDistance;

    if (dot(camera_to_pixel, world_end) < 0) { 
        return uv_start; 
    }

    
    vec4 end_screen_space = pv_matrix * vec4(world_end, 1.0);
    vec2 ray_end = (end_screen_space.xy / end_screen_space.w * 0.5 + 0.5);

    float delta = max(abs(ray_end.x - uv_start.x) * Input_normal_Res.x, abs(ray_end.y - uv_start.y) * Input_normal_Res.y);
    int sp_steps = min(max_iterations, int(delta * clamp_01(resolution)));

    vec2 out_uv = uv;
    int finished = 0;
    float pixel_depth = 0;
    
    for (float i = 1; i <= sp_steps; ++i) {
        // Compare pixel depth vs expected depth
        vec3 expected_world_pos = mix(world_start, world_end, i / float(sp_steps));
        vec4 screen_space_position = pv_matrix * vec4(expected_world_pos, 1);
        out_uv = fma(screen_space_position.xy / screen_space_position.w, vec2(0.5), vec2(0.5));

        float expected_depth = length(expected_world_pos);
        pixel_depth = length(getSceneWorldPosition(out_uv, texture(Scene_depth, out_uv).r));

        if (pixel_depth > 0 && pixel_depth < expected_depth) {
            finished = 1;
            break;
        }
    }
    
    float visibility = 
            (finished + (pixel_depth <= 0 ? 1 : 0))
            * (out_uv.x <= 0 || out_uv.x >= 1 ? 0 : 1)
            * (out_uv.y <= 0 || out_uv.y >= 1 ? 0 : 1);

    return out_uv;
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

    vec3 ground_color = surface_shading(shading, scene_albedo, scene_normal, scene_mrao, sun_direction, getSceneWorldDirection());

    vec3 trans_color = surface_shading(shading, trans_albedo.rgb, trans_normal, trans_mrao, sun_direction, getSceneWorldDirection());

    
	oFragmentColor = vec4(ground_color, 1);
    oNormal = scene_normal;
    oMrao = scene_mrao;
    oDepth = scene_depth;

    if (trans_depth > scene_depth) {

        vec3 ground_color = ground_color;
        vec2 refracted_uvs = screen_space_refraction(uv, Scene_depth, getSceneWorldPosition(uv, trans_depth), getSceneWorldDirection(), trans_normal, 1.03);

        vec3 refracted_albedo = texture(Scene_color, refracted_uvs).rgb * mix(vec3(1), vec3(0.1, 0.55,1), pow(trans_albedo.a, 0.2));
        vec3 refracted_normal = normalize(texture(Scene_normal, refracted_uvs).rgb);
        vec3 refracted_mrao = texture(Scene_mrao, refracted_uvs).rgb;
        vec3 refracted_color = surface_shading(shading, refracted_albedo, refracted_normal, refracted_mrao, sun_direction, getSceneWorldDirection());

        oFragmentColor = vec4(mix(refracted_color, trans_color, trans_albedo.a), 1);
        oNormal = trans_normal;
        oMrao = trans_mrao;
        oDepth = trans_depth;
    }

    if (oMrao.g < 0.2) {
        vec2 reflected_uvs = screen_space_reflection(uv, Scene_depth, getSceneWorldPosition(uv, oDepth), getSceneWorldDirection(), oNormal);

        vec3 reflected_albedo = texture(Scene_color, reflected_uvs).rgb;
        vec3 reflected_normal = normalize(texture(Scene_normal, reflected_uvs).rgb);
        vec3 reflected_mrao = texture(Scene_mrao, reflected_uvs).rgb;
        
        vec3 reflected_color = surface_shading(shading, reflected_albedo, reflected_normal, reflected_mrao, sun_direction, getSceneWorldDirection());
        
        oFragmentColor = vec4(mix(reflected_color, oFragmentColor.rgb, 0.5), 1);
    }

}