#version 430

layout(location = 0) in vec3 pos;
layout(location = 0) out vec3 out_norm;
layout(location = 1) out float time;
layout(location = 2) out vec3 out_position;
layout(location = 3) out float altitude;
layout(location = 4) out vec2 coordinates;

layout(location = 1) uniform mat4 model;
layout(location = 2) uniform float inner_width;
layout(location = 3) uniform float outer_width;
layout(location = 4) uniform float cell_width;
layout(location = 6) uniform float radius;
layout(location = 8) uniform int morph_to_sphere;
layout(location = 10) uniform mat4 temp_rotation;
layout(location = 12) uniform mat4 temp_location;

layout (std140, binding = 0) uniform WorldData
{
    mat4 proj_matrix;
    mat4 view_matrix;
    mat4 pv_matrix;
    mat4 proj_matrix_inv;
    mat4 view_matrix_inv;
    mat4 pv_matrix_inv;
	vec3 camera_pos;
	vec3 camera_forward;
    float world_time;
};

const float PI = 3.14159265358979323846;
const float HALF_PI = 3.14159265358979323846 / 2.0;

vec3 to_3d_v4(vec2 pos, float rho) {
	vec2 norm_pos = clamp(pos / rho, -HALF_PI, HALF_PI);
    float cos_y = cos(norm_pos.y);
    return vec3(
        cos_y * sin(norm_pos.x), 
        sin(norm_pos.y),
        cos_y * cos(norm_pos.x) 
    ) * rho;
}

void main()
{
	vec3 post_transform_pos = (temp_location * temp_rotation * vec4(pos, 1)).xyz;

	vec3 planet_pos = to_3d_v4(post_transform_pos.xy, radius );
	out_position = planet_pos;

	gl_Position = pv_matrix * model * vec4(out_position, 1.0);
}