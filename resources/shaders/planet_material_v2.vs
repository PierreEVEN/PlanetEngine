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
    vec3 scaled_pos = pos;

	vec2 local_dir = normalize(mat3(lod_local_transform) * scaled_pos).xz;
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

	vec2 vertex_pos = (lod_local_transform * vec4(scaled_pos, 1)).xz;
	vec3 planet_pos = grid_to_sphere(vertex_pos, radius);
    mat3 rot = mat3(model);
    vec3 norm_f64 = normalize(rot * (planet_pos + vec3(radius, 0, 0)));


    //dvec3 norm_f64 = normalize(rot * (planet_pos + vec3(radius, 0, 0)));
    //float h0 = get_height_at_locationd(norm_f64);

    float h0 = get_height_at_location_int(norm_f64);

    coordinates = vec2(mod(seamless_uv_from_sphere_normal(norm_f64) * 1000, 1));    
    altitude = float(h0);
    out_norm = vec3(norm_f64);
    vec4 world_pos = model * vec4(planet_pos, 1.0);
    world_pos.xyz += out_norm * altitude_with_water(altitude);
	out_position = world_pos.xyz;
	gl_Position = pv_matrix * world_pos;
    // out_norm = vec3(final_norm);
}