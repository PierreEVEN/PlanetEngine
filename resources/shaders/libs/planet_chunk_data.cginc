#ifndef PLANET_CHUNK_DATA_H_
#define PLANET_CHUNK_DATA_H_

layout(std430, binding = 3) buffer PLANET_CHUNK
{
  mat4 Chunk_LocalTransform;
  mat4 Chunk_PlanetModel;
  mat4 Chunk_WorldOrientation;
  float Chunk_PlanetRadius;
  float Chunk_CellWidth;
  int Chunk_CellCount;
  int Chunk_CurrentLOD;
};

#endif // PLANET_CHUNK_DATA_H_