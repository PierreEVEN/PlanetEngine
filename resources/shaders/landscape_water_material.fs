#version 430 core
#extension GL_ARB_explicit_uniform_location : enable
precision highp float;

#include "libs/world_data.cginc"

layout(location = 0) in vec3 g_LocalNormal;
layout(location = 1) in vec3 position;
layout(location = 2) in float altitude;
layout(location = 3) in vec2 g_Coordinates;
layout(location = 4) in float planet_radius;
layout(location = 5) in vec3 g_DebugScalar;
layout(location = 6) in vec3 g_Normal;
layout(location = 7) in vec3 g_Tangent;
layout(location = 8) in vec3 g_BiTangent;
layout(location = 9) in vec3 g_Normal_PS;

out vec4 oFragmentColor;

void main()
{
    if (altitude > 0)
        discard;
    oFragmentColor.x = max(-altitude, 0);
}