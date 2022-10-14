#ifndef COMPUTE_BASE_H_
#define COMPUTE_BASE_H_

#include "../libs/planet_chunk_data.cginc"

ivec2 pixel_pos_to_vertex_2D_pos(uvec2 pixe_pos) {    
  return ivec2(vec2(pixe_pos.xy) - (vec2(Chunk_CellCount) * 2 + 1));
}



#endif // COMPUTE_BASE_H_