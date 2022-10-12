#ifndef PLANET_CHUNK_DATA_H_
#define PLANET_CHUNK_DATA_H_

layout(std430, binding = 3, std140) buffer PLANET_CHUNK
{
  mat4 Chunk_LocalTransform;
  mat4 Chunk_PlanetTransform;
  float Chunk_PlanetRadius;
  float Chunk_CellWidth;
  int Chunk_CellCount;
};

#endif // PLANET_CHUNK_DATA_H_