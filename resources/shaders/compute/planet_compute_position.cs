#version 430

#include "../libs/planet_chunk_data.cginc"
#include "../libs/landscape.cginc"

layout (local_size_x = 16, local_size_y = 16) in;

layout (r32f, binding = 0) uniform image2D img_output;

vec3 grid_to_sphere_old(vec2 pos, float rho) {
    vec2 dpos = pos;
	vec2 norm_pos = clamp(pos / rho, -HALF_PI, HALF_PI);

    float cos_y = cos(norm_pos.y);
    return vec3(
        cos_y * cos(norm_pos.x) - 1,
        cos_y * sin(norm_pos.x), 
        sin (norm_pos.y)
    ) * rho;
}

void main() {
  vec2 world_coordinates = vec2(gl_GlobalInvocationID.xy) - vec2(Chunk_CellCount) * 2 - 1.5;
  
	vec2 vertex_2d_pos = (Chunk_LocalTransform * vec4(world_coordinates.x, 0, world_coordinates.y, 1)).xz;
	vec3 vertex_3d_pos = grid_to_sphere(vertex_2d_pos, Chunk_PlanetRadius);
  vec3 world_normal = normalize(mat3(Chunk_PlanetTransform) * (vertex_3d_pos + vec3(Chunk_PlanetRadius, 0, 0)));

  float h0 = float(get_height_at_location(world_normal));

  imageStore(img_output, ivec2(gl_GlobalInvocationID), vec4(h0, 0, 0, 1));
}