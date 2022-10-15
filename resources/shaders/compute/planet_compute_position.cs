#version 430

#include "../libs/landscape.cginc"
#include "../libs/compute_base.cginc"

layout (local_size_x = 1, local_size_y = 1) in;

layout (r32f, binding = 0) uniform image2D img_output;

void main() {

  ivec2 vertex_pos_2D = pixel_pos_to_vertex_2D_pos(gl_GlobalInvocationID.xy);

  if (Chunk_CurrentLOD != 0 && abs(vertex_pos_2D.x) < Chunk_CellCount - 1 && abs(vertex_pos_2D.y) < Chunk_CellCount - 1) {
    imageStore(img_output, ivec2(gl_GlobalInvocationID), vec4(0, 0, 0, 1));
    return;
  }
  
	vec2 vertex_2d_pos = (Chunk_LocalTransform * vec4(vertex_pos_2D.x, 0, vertex_pos_2D.y, 1)).xz;
	vec3 vertex_3d_pos = grid_to_sphere(vertex_2d_pos, Chunk_PlanetRadius);
  vec3 world_normal = normalize(mat3(Chunk_PlanetTransform) * (vertex_3d_pos + vec3(Chunk_PlanetRadius, 0, 0)));

  float h0 = 0;
  h0 = float(get_height_at_location(world_normal));
  imageStore(img_output, ivec2(gl_GlobalInvocationID), vec4(h0, 0, 0, 1));
}