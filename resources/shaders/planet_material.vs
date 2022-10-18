#version 430  core
#extension GL_ARB_explicit_uniform_location : enable

#include "libs/world_data.cginc"
#include "libs/maths.cginc"

// Inputs
layout(location = 0) in vec3 pos;

// Outputs
layout(location = 0) out vec3 out_norm;
layout(location = 1) out vec3 debug_scalar;
layout(location = 2) out vec3 out_position;
layout(location = 3) out float altitude;
layout(location = 4) out vec2 coordinates;
layout(location = 5) out float planet_radius;

layout(location = 1) uniform mat4 model;
layout(location = 2) uniform mat4 lod_local_transform;
layout(location = 3) uniform float radius;
layout(location = 4) uniform float cell_width;
layout(location = 5) uniform float grid_cell_count;

layout(location = 7) uniform sampler2D height_map;
layout(location = 6) uniform sampler2D normal_map;

layout(location = 12) uniform vec4 debug_vector;
layout(location = 13) uniform mat3 planet_rotation;

vec3 grid_to_absolute_sphere(vec2 pos_2d, float planet_radius) {
    return (grid_to_sphere(pos_2d, planet_radius) + vec3(planet_radius, 0, 0));
}

void main()
{
    /* [1] transform 2D pos to 3D pos */

    // Vertex position in 2D space
    vec2 grid_2d_pos_base = (lod_local_transform * vec4(pos, 1)).xz;
    // Clamp position to [-PI/2, PI/2] (don't loop around planet)
    vec2 grid_2d_pos = clamp(grid_2d_pos_base, -radius * PI / 2, radius * PI / 2);

    // Used to compute tangent and bitangent
    vec2 grid_2d_pos_tx = clamp(grid_2d_pos_base, -radius * PI / 2, radius * PI / 2) + vec2(radius * PI / 2 * 1 * 0, 0); // 2D pos 90° along x axis
    vec2 grid_2d_pos_ty = clamp(grid_2d_pos_base, -radius * PI / 2, radius * PI / 2) + vec2(0, radius * PI / 2 * 0); // 2D pos 90° along y axis
    
    vec3 planet_pos_tx = grid_to_sphere_centered(grid_2d_pos_tx, radius);
    vec3 planet_pos_ty = grid_to_sphere_centered(grid_2d_pos_ty + debug_vector.xy * 10000, radius);
    debug_scalar = vec3(grid_2d_pos_ty + debug_vector.xy * 10000, 0);//planet_rotation * vec3(normalize(planet_pos_ty));
    
    // Map 2D pos on 3D sphere
	vec3 planet_pos_r = grid_to_sphere(grid_2d_pos, radius); //@TODO not actually a readable value
    vec3 planet_pos = planet_pos_r + vec3(radius, 0, 0);
    
    vec3 planet_normal = normalize(mat3(model) * planet_pos);

    vec2 coords = (pos.xz + grid_cell_count * 2 + 3) / (grid_cell_count * 4 + 6);

    vec2 packed_normal = texture(normal_map, coords).xy;
	out_norm = normalize(vec3(packed_normal.x, packed_normal.y, 1 - length(packed_normal)));
    debug_scalar = vec3(out_norm.xy * 100, 0);

    mat3 TBN = mat3(vec3(0, 1, 0), vec3(1, 0, 0), planet_normal);
    out_norm = TBN * out_norm;

    vec3 planet_normal_unrotated = normalize(planet_pos);
    coordinates = vec2(mod(seamless_uv_from_sphere_normal(dvec3(planet_normal_unrotated)) * 1000, 1));
    vec4 world_pos = vec4(0,0,0,1);
    world_pos = model * vec4(planet_pos_r,1);
    altitude = texture(height_map, coords).r;
    world_pos.xyz += planet_normal * (altitude < 0 ? 0 : altitude);

    // Waves
    /*
    if (altitude < 0) {
        float intensity = clamp(-altitude * 10, 0, 1);
        float progressx = -world_pos.z * 0.1 + -world_time * 1;
        float progressy = -world_pos.y * .05 + -world_time * 0.3;
        world_pos.x += sin(progressx) * 4 * intensity;
        world_pos.z -= (cos(1 - progressx) + 1) * 8 * intensity;
        world_pos.z -= (cos(2 - progressx) + 2) * 3* intensity;
        world_pos.z -= (cos(3 - progressx) + 3) * 2* intensity;
        world_pos.z -= (cos(4 - progressx) + 4) * 1 * intensity;
        world_pos.x += sin(progressy) * 8 * intensity;
    }
    */

	out_position = world_pos.xyz;

	gl_Position = pv_matrix * world_pos;
    planet_radius = radius;
}