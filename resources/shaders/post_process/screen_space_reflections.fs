#version 430

#include "../libs/world_data.cginc"
#include "../libs/maths.cginc"

out vec4 oFragmentColor;
layout(location = 0) in vec2 uv;
layout(location = 2) uniform vec2 InputResolution;

layout(location = 4) uniform sampler2D DepthMap;
layout(location = 5) uniform sampler2D NormalMap;
layout(location = 6) uniform float z_near;
layout(location = 7) uniform sampler2D GBUFFER_mrao;

uniform vec2 enabled;


float depth_at(vec2 uvs) {
	return z_near / texture(DepthMap, uvs).r;
}

vec3 getSceneWorldPosition(vec2 uvs) {
	float linear_depth = texture(DepthMap, uvs).r;
    if (linear_depth <= 0)
        return vec3(0);
    // Get z depth
    float zDepth = linear_depth;

    // compute clip space depth
    vec4 clipSpacePosition = vec4(uvs * 2.0 - 1.0, zDepth, 1.0);

    // Transform local space to view space
    vec4 viewSpacePosition = proj_matrix_inv * clipSpacePosition;

    viewSpacePosition /= viewSpacePosition.w;

    // Transform view space to world space
    return (view_matrix_inv * viewSpacePosition).xyz;
}

vec3 getSceneWorldDirection(vec2 uvs) {
    // compute clip space direction
    vec4 clipSpacePosition = vec4(uvs * 2.0 - 1.0, 1.0, 1.0);

    // Transform local space to view space
    vec4 viewSpacePosition = proj_matrix_inv * clipSpacePosition;

    viewSpacePosition /= viewSpacePosition.w;

    // Transform view space to world space
    vec4 worldSpacePosition = view_matrix_inv * viewSpacePosition;
    return normalize(worldSpacePosition.xyz);
}

void main() {

    /* PARAMS */
    float maxDistance = 100000;
    float resolution  = 0.3;
    int   steps       = 5;
    float thickness   = 0.1;

    /* World infos */
    vec3 world_position = getSceneWorldPosition(uv).xyz;
    vec3 camera_to_pixel = getSceneWorldDirection(uv);
    vec3 reflected_ray = reflect(camera_to_pixel, texture(NormalMap, uv).xyz);
    float world_depth = depth_at(uv);

    /* Skip pixel if not used with SSR */
    vec3 mrao = texture(GBUFFER_mrao, uv).rgb;    
    if (world_depth <= 0.0 || mrao.g >= 0.2) { 
        oFragmentColor = vec4(uv, 0, 1); return; 
    }

    // Compute reflection end point from world space to screen space
    vec4 end_screen_space = pv_matrix * vec4(world_position + (reflected_ray * maxDistance), 1.0);

    float depth_start = length(world_position);
    float depth_end = length(world_position + (reflected_ray * maxDistance));

    /* compute ray start and end point in screen space */
    vec2 ray_start = uv * InputResolution;
    vec2 ray_end = (end_screen_space.xy / end_screen_space.w * 0.5 + 0.5) * InputResolution;

    // Compute pixel steps @TODO replace with a better approach
    float deltaX = ray_end.x - ray_start.x; // nb pixels x
    float deltaY = ray_end.y - ray_start.y; // nb pixels y
    bool useX = abs(deltaX) >= abs(deltaY);
    int sp_steps = int((useX ? abs(deltaX) : abs(deltaY)) * clamp_01(resolution));
    vec2 increment = vec2(deltaX, deltaY) / max(sp_steps, 1);

    vec2 ray_pos_sp = ray_start.xy;
    vec2 out_uv = uv;
    int iter = 1000;
    for (float i = 0; i < sp_steps; ++i) {
        if (iter-- <= 0) { oFragmentColor = vec4(0, 0, 10, 1); return; }
        ray_pos_sp += increment;
        out_uv = ray_pos_sp / InputResolution;

        float expected_depth = mix(depth_start, depth_end, i / float(sp_steps));
        float pixel_depth = depth_at(out_uv);

        if (pixel_depth <= expected_depth)
            break;
    }

    /*
    search1 = search0 + ((search1 - search0) / 2.0);

    steps *= hit0;
    iter = 5;
    for (float i = 0; i < steps; ++i) {
        if (iter-- <= 0) { oFragmentColor = vec4(0, 0, 10, 1); return; }

        ray_pos_sp       = mix(ray_start, ray_end, search1);
        out_uv      = ray_pos_sp / InputResolution;
        float depth        = depth_at(out_uv);

        if (depth > 0 && depth < thickness) {
            hit1 = 1;
            search1 = search0 + ((search1 - search0) / 2);
        } else {
            float temp = search1;
            search1 = search1 + ((search1 - search0) / 2);
            search0 = temp;
        }
    }
    */

    /*
    float visibility = hit1
            * 1
            * ( 1 - max(dot(-camera_to_pixel, reflected_ray), 0))
            * ( 1 - clamp_01( depth / thickness))
            * (out_uv.x < 0 || out_uv.x > 1 ? 0 : 1)
            * (out_uv.y < 0 || out_uv.y > 1 ? 0 : 1);

    visibility = clamp_01(visibility);
    */

    vec4 final_uvs = vec4(out_uv, 0, 0);
    final_uvs.ba = vec2(0); // vec2(visibility)
    oFragmentColor = final_uvs;
}
