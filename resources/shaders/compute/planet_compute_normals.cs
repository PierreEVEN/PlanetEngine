#version 430

#include "../libs/planet_chunk_data.cginc"

layout (local_size_x = 16, local_size_y = 16) in;

layout (rg16f, binding = 0) uniform image2D img_output;

void main() {
  ivec2 coords = ivec2(gl_GlobalInvocationID);

  imageStore(img_output, coords, normalize(vec4(1)));
}