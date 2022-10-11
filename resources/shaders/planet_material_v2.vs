#version 430

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

void main()
{
	vec2 local_dir = normalize(mat3(lod_local_transform) * pos).xz;
	vec2 grid_tangent = vec2(
        abs(local_dir.x) < abs(local_dir.y) ? 
			local_dir.y < 0 ? -1 : 1 :
			0,
		abs(local_dir.y) < abs(local_dir.x) ? 
			local_dir.x < 0 ? -1 : 1 :
			0);

	// float h_left = altitude_with_water(get_height_at_location(final_pos.xy + normalized_direction * cell_width * 1));
	// float h_right = altitude_with_water(get_height_at_location(final_pos.xy - normalized_direction * cell_width * 1));
	// float h_mean = (h_left + h_right) / 2;

	vec2 vertex_pos = (lod_local_transform * vec4(pos, 1)).xz;
	vec3 planet_pos = to_3d_v4(vertex_pos, radius);
    dmat3 rot = dmat3(model);
    dvec3 norm_f64 = normalize(rot * (planet_pos + dvec3(radius, 0, 0)));
    out_norm = vec3(norm_f64);

    coordinates = vec2(mod(seamless_uv_from_sphere_normal(norm_f64) * 1000, 1));
    


    debug_scalar = vec3(coordinates, 0);

    dvec3 t_1 = cross(out_norm, vec3(1, 0, 0));
    dvec3 t_2 = cross(out_norm, vec3(0, 1, 0));

    dvec3 n_1 = norm_f64 + t_1 * 0.00001;

    double h0 = get_height_at_location(norm_f64);
    altitude = float(h0);

    dvec3 p0 = norm_f64 * h0;
    dvec3 p1 = norm_f64 * 0;//get_height_at_location(norm_f64 + t_1 * 0.000001) + t_1;
    dvec3 p2 = norm_f64 * 0;//get_height_at_location(norm_f64 + t_2 * 0.000001) + t_2;

    dvec3 final_norm = cross(p1 - p0, p2 - p0);


    vec4 world_pos = model * vec4(planet_pos, 1.0);
    world_pos.xyz += out_norm * altitude_with_water(altitude);
	out_position = world_pos.xyz;
	gl_Position = pv_matrix * world_pos;
    // out_norm = vec3(final_norm);
}