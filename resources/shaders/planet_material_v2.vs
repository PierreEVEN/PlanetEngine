#version 430  core
#extension GL_ARB_explicit_uniform_location : enable

#include "libs/world_data.cginc"

#include "libs/world_data.cginc"
#include "libs/landscape.cginc"
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

	out_norm = normalize(vec3(texture(normal_map, vec2(0.5)).xy, 1));
    altitude = texture(height_map, (pos.xz + grid_cell_count / 2 + 1) / (grid_cell_count + 2)).r;
	altitude = get_height_at_location_int(norm_f64);

    coordinates = vec2(mod(seamless_uv_from_sphere_normal(norm_f64) * 1000, 1));
    out_norm = vec3(norm_f64);
    vec4 world_pos = model * vec4(planet_pos, 1.0);
    world_pos.xyz += out_norm * altitude_with_water(altitude);
	out_position = world_pos.xyz;
	gl_Position = pv_matrix * world_pos;
    // out_norm = vec3(final_norm);
}