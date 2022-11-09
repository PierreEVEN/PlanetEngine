#version 430 core
#extension GL_ARB_enhanced_layouts : enable
#extension GL_ARB_explicit_uniform_location : enable

layout (triangles) in;
layout (line_strip, max_vertices = 6) out;

#include "libs/world_data.cginc"
layout(location = 0) in vec3 g_LocalNormal[];
layout(location = 1) in vec3 g_WorldPosition[];
layout(location = 2) in float g_Altitude[];
layout(location = 3) in vec2 g_TextureCoordinates[];
layout(location = 4) in float g_PlanetRadius[];
layout(location = 5) in vec3 g_DebugScalar[];
layout(location = 6) in vec3 g_Normal[];
layout(location = 7) in vec3 g_Tangent[];
layout(location = 8) in vec3 g_BiTangent[];

out vec3 color; 

layout(location = 2) uniform mat4 mesh_transform_cs;

in VS_OUT {vec4 gl_Position;} gs_in[];

void GenerateLine(int index)
{
    float MAGNITUDE = length((mat3(mesh_transform_cs) * vec3(1, 0, 0)).xyz) * 0.5;
    color = vec3(0,0,1);
    gl_Position = gs_in[index].gl_Position;
    EmitVertex();
    gl_Position = gs_in[index].gl_Position + vec4(0.01,0,0,0);//pv_matrix * vec4(g_Normal[index] * MAGNITUDE, 1);
    EmitVertex();
    EndPrimitive();
    return;
    color = vec3(1,0,0);
    gl_Position = gs_in[index].gl_Position;
    EmitVertex();
    gl_Position = gs_in[index].gl_Position + pv_matrix * vec4(g_Tangent[index] * MAGNITUDE, 1);
    EmitVertex();
    color = vec3(0,1,0);
    gl_Position = gs_in[index].gl_Position;
    EmitVertex();
    gl_Position = gs_in[index].gl_Position + pv_matrix * vec4(g_BiTangent[index] * MAGNITUDE, 1);
    EmitVertex();
    EndPrimitive();
}

void main()
{
    GenerateLine(0);
}