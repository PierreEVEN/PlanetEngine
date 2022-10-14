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

layout(location = 1) uniform mat4 model;
layout(location = 2) uniform mat4 lod_local_transform;
layout(location = 3) uniform float radius;
layout(location = 4) uniform float cell_width;
layout(location = 5) uniform float grid_cell_count;

layout(location = 7) uniform sampler2D height_map;
layout(location = 6) uniform sampler2D normal_map;

void main()
{
	vec2 vertex_pos = (lod_local_transform * vec4(pos, 1)).xz;
	vec3 planet_pos = grid_to_sphere(vertex_pos, radius);
    mat3 rot = mat3(model);
    vec3 norm_f64 = normalize(rot * (planet_pos + vec3(radius, 0, 0)));

    vec2 coords = (pos.xz + (grid_cell_count) / 2) / (grid_cell_count + 2);

    vec2 packed_normal = texture(normal_map, coords).xy;

	out_norm = normalize(vec3(packed_normal, 1 - length(packed_normal)));
    altitude = texture(height_map, coords).r;

    debug_scalar = out_norm;

    coordinates = vec2(mod(seamless_uv_from_sphere_normal(norm_f64) * 1000, 1));
    //out_norm = vec3(norm_f64);
    vec4 world_pos = model * vec4(planet_pos, 1.0);
    world_pos.xyz += norm_f64 * altitude;
	out_position = world_pos.xyz;
	gl_Position = pv_matrix * world_pos;
    // out_norm = vec3(final_norm);
}