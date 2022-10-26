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

vec3 getSceneWorldPosition(vec2 uvs) {
	float linear_depth = texture(DepthMap, uvs).r;
    if (linear_depth <= 0)
        return vec3(0);
    // compute clip space depth
    vec4 clipSpacePosition = vec4(uvs * 2.0 - 1.0, linear_depth, 1.0);

    // Transform local space to view space
    vec4 viewSpacePosition = proj_matrix_inv * clipSpacePosition;
    viewSpacePosition /= viewSpacePosition.w;

    // Transform view space to world space
    return (view_matrix_inv * viewSpacePosition).xyz;
}

float depth_at(vec2 uvs) {
	return z_near / texture(DepthMap, uvs).r;
}

void main() {

    /* PARAMS */
    float maxDistance = 100000;
    float resolution  = 0.3;

    /* World infos */
    vec3 world_position = getSceneWorldPosition(uv);
    vec3 camera_to_pixel = getSceneWorldDirection(uv);
    vec3 reflected_ray = reflect(camera_to_pixel, texture(NormalMap, uv).xyz);
    float world_depth = depth_at(uv);

    //oFragmentColor = vec4(vec3(length(world_position) / 100000), 1); return; 

    /* Skip pixel if not used with SSR */
    vec3 mrao = texture(GBUFFER_mrao, uv).rgb;    
    if (world_depth <= 0.0 || mrao.g >= 0.2) { 
        oFragmentColor = vec4(uv, 0, 1); return; 
    }

    //maxDistance = world_depth;

    // Compute reflection end point from world space to screen space
    vec4 end_screen_space = pv_matrix * vec4(world_position + (reflected_ray * maxDistance), 1.0);

    vec3 world_start = world_position;
    vec3 world_end = world_position + (reflected_ray * maxDistance);

    /* compute ray start and end point in screen space */
    vec2 ray_start = uv * InputResolution;
    vec2 ray_end = (end_screen_space.xy / end_screen_space.w * 0.5 + 0.5) * InputResolution;

    if (ray_end.x < 0 || ray_end.y < 0 || ray_end.x > InputResolution.x || ray_end.y > InputResolution.y)
    {
        ray_end = ray_start + normalize(ray_end - ray_start) * InputResolution;
    }


    // Compute pixel steps @TODO replace with a better approach
    float delta = max(abs(ray_end.x - ray_start.x), abs(ray_end.y - ray_start.y));
    int sp_steps = int(delta * clamp_01(resolution));

    float to_early = 0;

    vec2 out_uv = uv;
    if (sp_steps > 1000) {out_uv = vec2(0); sp_steps = 0;}
    for (float i = 1; i <= sp_steps; ++i) {
        // Update current tested pixel
        out_uv = mix(ray_start, ray_end, i / float(sp_steps)) / InputResolution;

        out_uv = clamp_01(out_uv);

        // Compare pixel depth vs expected depth
        float expected_depth = length(mix(world_start, world_end, i / float(sp_steps)));
        float pixel_depth = length(getSceneWorldPosition(out_uv));
        
        if (pixel_depth > 0 && pixel_depth < expected_depth) {
            //out_uv = vec2((expected_depth - 10000) / 100000);
            if (i == 1)
                to_early = 1;
            break;
        }
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

    float visibility = 
            (1 - to_early)
            * (out_uv.x < 0 || out_uv.x > 1 ? 0 : 1)
            * (out_uv.y < 0 || out_uv.y > 1 ? 0 : 1);

    visibility = clamp_01(visibility);

    vec4 final_uvs = vec4(out_uv, 0, 0);
    final_uvs.b = visibility;
    oFragmentColor = final_uvs;
}
