#version 430 core
#extension GL_ARB_enhanced_layouts : enable
#extension GL_ARB_explicit_uniform_location : enable

layout (triangles) in;
layout (line_strip, max_vertices = 6) out;

#include "libs/world_data.cginc"
in VS_OUT {
layout(location = 0) in vec3 g_Normal;
layout(location = 1) in vec3 g_WorldPosition;
layout(location = 2) in float g_Altitude;
layout(location = 3) in vec2 g_TextureCoordinates;
layout(location = 4) in float g_PlanetRadius;
layout(location = 5) in vec4 g_DebugScalar;
layout(location = 6) in vec3 g_LocalNormal;
layout(location = 7) in vec3 g_Tangent;
layout(location = 8) in vec3 g_BiTangent;
} gs_in[];

out vec3 color; 

layout(location = 2) uniform mat4 lod_local_transform;

void GenerateLine(int index)
{
    float MAGNITUDE = length((mat3(lod_local_transform) * vec3(1, 0, 0)).xyz) * 0.5;
    color = vec3(0,0,1);
    gl_Position = gl_in[index].gl_Position;
    EmitVertex();
    gl_Position = gl_in[index].gl_Position + pv_matrix * vec4(gs_in[index].g_Normal * MAGNITUDE, 1);
    EmitVertex();
    color = vec3(1,0,0);
    gl_Position = gl_in[index].gl_Position;
    EmitVertex();
    gl_Position = gl_in[index].gl_Position + pv_matrix * vec4(gs_in[index].g_Tangent * MAGNITUDE, 1);
    EmitVertex();
    color = vec3(0,1,0);
    gl_Position = gl_in[index].gl_Position;
    EmitVertex();
    gl_Position = gl_in[index].gl_Position + pv_matrix * vec4(gs_in[index].g_BiTangent * MAGNITUDE, 1);
    EmitVertex();
    EndPrimitive();
}

void main()
{
    GenerateLine(0);
}