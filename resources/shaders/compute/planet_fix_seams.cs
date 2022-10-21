#version 430

#include "../libs/landscape.cginc"
#include "../libs/compute_base.cginc"

layout (local_size_x = 1, local_size_y = 1) in;

layout (r32f, binding = 0) uniform image2D img_input;

void main() {
  // Don't compute on borders
    if (gl_GlobalInvocationID.x == 0 || gl_GlobalInvocationID.y == 0 || gl_GlobalInvocationID.x == Chunk_CellCount * 4 + 4 || gl_GlobalInvocationID.y == Chunk_CellCount * 4 + 4) {
        return;
    }

    ivec2 vertex_pos_2D = pixel_pos_to_vertex_2D_pos(gl_GlobalInvocationID.xy);

    // Don't compute center
    if (
        abs(vertex_pos_2D.x - 1) > Chunk_CellCount * 2 + 2 ||
        abs(vertex_pos_2D.y - 1) > Chunk_CellCount * 2 + 2 ||
        (Chunk_CurrentLOD != 0 && abs(vertex_pos_2D.x) < Chunk_CellCount - 1 && abs(vertex_pos_2D.y) < Chunk_CellCount - 1)
    ) {    
        return;
    }

    vec2 model_pos_2d = normalize((mat3(Chunk_LocalTransform) * vec3(vertex_pos_2D.x, 0, vertex_pos_2D.y)).xz);
      
	vec2 forward = vec2(0, 0);    
    if (abs(model_pos_2d.x) > abs(model_pos_2d.y))
        forward = normalize((mat3(Chunk_LocalTransform) * vec3(0, 0, 1)).xz);
    else
        forward = normalize((mat3(Chunk_LocalTransform) * vec3(1, 0, 0)).xz);

    // Compute fix weight
    float weight = 0;    
    if (abs(vertex_pos_2D.y - 0.1) > abs(vertex_pos_2D.x)) {    
        weight = gl_GlobalInvocationID.x % 2 == 0 ? 1 : 0;
    }
    else {        
        weight = gl_GlobalInvocationID.y % 2 == 0 ? 1 : 0;
    }
    weight *= clamp((max(float(abs(vertex_pos_2D.x) - Chunk_CellCount - 1), float(abs(vertex_pos_2D.y) - Chunk_CellCount - 1)) - 1) / (Chunk_CellCount - 2), 0, 1);

    float hl = imageLoad(img_input, ivec2(gl_GlobalInvocationID.xy) + ivec2(forward)).x;
    float hr = imageLoad(img_input, ivec2(gl_GlobalInvocationID.xy) - ivec2(forward)).x;
    float hc = imageLoad(img_input, ivec2(gl_GlobalInvocationID.xy)).x;

    float h0 = mix(hc, (hl + hr) / 2, weight);

    imageStore(img_input, ivec2(gl_GlobalInvocationID.xy), vec4(h0, 0, 0, 1));
}