#version 430

#include "../libs/compute_base.cginc"

layout (local_size_x = 1, local_size_y = 1, local_size_z = 1) in;

layout (rg16f, binding = 0) uniform image2D img_output;
layout (r32f, binding = 1) uniform image2D heightmap_input;

void main() {
  ivec2 vertex_pos_2D = pixel_pos_to_vertex_2D_pos(gl_GlobalInvocationID.xy);

  if (
    abs(vertex_pos_2D.x - 1) > Chunk_CellCount * 2 + 2 ||
    abs(vertex_pos_2D.y - 1) > Chunk_CellCount * 2 + 2 ||
    (Chunk_CurrentLOD != 0 && abs(vertex_pos_2D.x) < Chunk_CellCount - 1 && abs(vertex_pos_2D.y) < Chunk_CellCount - 1)
  ) {    
    imageStore(img_output, ivec2(gl_GlobalInvocationID.xy), normalize(vec4(0)));
    return;
  }

  mat3 rot = mat3(Chunk_LocalTransform);

  float h0 = imageLoad(heightmap_input, ivec2(gl_GlobalInvocationID.xy + uvec2(0, 0))).r;
  float h1 = imageLoad(heightmap_input, ivec2(gl_GlobalInvocationID.xy + uvec2(1, 0))).r;
  float h2 = imageLoad(heightmap_input, ivec2(gl_GlobalInvocationID.xy + uvec2(0, 1))).r;

  vec3 v0 = vec3(0, h0, 0);
  vec3 v1 = vec3(Chunk_CellWidth, h1, 0);
  vec3 v2 = vec3(0, h2, Chunk_CellWidth);

  vec3 norm = normalize(rot * normalize(cross(v1 - v0, v2 - v0)));



  imageStore(img_output, ivec2(gl_GlobalInvocationID.xy), vec4(norm.xz, 0, 1));
}