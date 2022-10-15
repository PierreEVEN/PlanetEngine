#version 430

#include "../libs/compute_base.cginc"

layout (local_size_x = 1, local_size_y = 1, local_size_z = 1) in;

layout (r32f, binding = 0) uniform image2D heightmap_input;
layout (rg16f, binding = 1) uniform image2D img_output;

void main() {
    // Don't compute on borders
      if (gl_GlobalInvocationID.x == 0 || gl_GlobalInvocationID.y == 0 || gl_GlobalInvocationID.x == Chunk_CellCount * 4 + 4 || gl_GlobalInvocationID.y == Chunk_CellCount * 4 + 4) {
      imageStore(img_output, ivec2(gl_GlobalInvocationID.xy), normalize(vec4(1)));
          return;
      }

    ivec2 vertex_pos_2D = pixel_pos_to_vertex_2D_pos(gl_GlobalInvocationID.xy);

      // Don't compute center
    if (
      abs(vertex_pos_2D.x - 1) > Chunk_CellCount * 2 + 2 ||
      abs(vertex_pos_2D.y - 1) > Chunk_CellCount * 2 + 2 ||
      (Chunk_CurrentLOD != 0 && abs(vertex_pos_2D.x) < Chunk_CellCount - 1 && abs(vertex_pos_2D.y) < Chunk_CellCount - 1)
    ) {    
      imageStore(img_output, ivec2(gl_GlobalInvocationID.xy), normalize(vec4(0)));
      return;
    }

    mat3 rot = mat3(Chunk_LocalTransform);

    vec2 f0 = normalize((rot * vec3(1, 0, 0)).xz);

    ivec2 dirx = ivec2(1, 0);
    ivec2 diry = ivec2(0, 1);

    if (f0.x > 0.99) {
      dirx = ivec2(1, 0);
      diry = ivec2(0, 1);
    }
    if (f0.x < -0.99) {
      dirx = ivec2(-1, 0);
      diry = ivec2(0, -1);
    }
    if (f0.y < -0.99) {
      dirx = ivec2(0, 1);
      diry = ivec2(-1, 0);
    }
    if (f0.y > 0.99) {
      dirx = ivec2(0, -1);
      diry = ivec2(1, 0);
    }



    float h0 = imageLoad(heightmap_input, ivec2(gl_GlobalInvocationID.xy + ivec2(0, 0))).r;
    float h1 = imageLoad(heightmap_input, ivec2(gl_GlobalInvocationID.xy + dirx)).r;
    float h2 = imageLoad(heightmap_input, ivec2(gl_GlobalInvocationID.xy + diry)).r;

    vec3 v0 = vec3(0, h0, 0);
    vec3 v1 = vec3(Chunk_CellWidth, h1, 0);
    vec3 v2 = vec3(0, h2, Chunk_CellWidth);

    vec3 norm = normalize(normalize(cross(v2 - v0, v1 - v0)));

    vec2 norm_2d = norm.xz;
    
    if (f0.x > 0.99) {
    }
    if (f0.x < -0.99) {
      norm_2d = vec2(norm_2d.x, norm_2d.y);
    }
    if (f0.y < -0.99) {
      norm_2d = vec2(norm_2d.x, norm_2d.y);
    }
    if (f0.y > 0.99) {
    }

    imageStore(img_output, ivec2(gl_GlobalInvocationID.xy), vec4(norm_2d, 0, 1));
}