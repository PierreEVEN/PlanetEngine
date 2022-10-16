#ifndef WORLD_DATA_H_
#define WORLD_DATA_H_

layout (std140, binding = 0) uniform WorldData
{
    mat4 proj_matrix;
    mat4 view_matrix;
    mat4 pv_matrix;
    mat4 proj_matrix_inv;
    mat4 view_matrix_inv;
    mat4 pv_matrix_inv;
    float world_time;
	vec3 camera_pos;
	vec3 camera_forward;
};

#endif // WORLD_DATA_H_