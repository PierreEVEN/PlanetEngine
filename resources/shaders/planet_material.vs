#version 430  core
#extension GL_ARB_explicit_uniform_location : enable

#include "libs/world_data.cginc"
#include "libs/maths.cginc"

// Inputs
layout(location = 0) in vec3 pos;

// Uniforms
layout(location = 1) uniform mat4 model; // Planet mesh rotation
layout(location = 2) uniform mat4 lod_local_transform; // Chunk grid local transform (90° rotations + 2D offset)
layout(location = 3) uniform float radius; // planet radius
layout(location = 4) uniform float grid_cell_count;
layout(location = 5) uniform sampler2D height_map;
layout(location = 6) uniform sampler2D normal_map;
layout(location = 7) uniform vec4 debug_vector;
layout(location = 8) uniform mat4 planet_world_orientation;

// Outputs
layout(location = 0) out vec3 g_Normal;
layout(location = 1) out vec3 g_WorldPosition;
layout(location = 2) out float g_Altitude;
layout(location = 3) out vec2 g_TextureCoordinates;
layout(location = 4) out float g_PlanetRadius;
layout(location = 5) out vec3 g_DebugScalar;
layout(location = 6) out vec3 g_LocalNormal;
layout(location = 7) out vec3 g_Tangent;
layout(location = 8) out vec3 g_BiTangent;

vec2 pack_normal(vec3 normal) {
    return normal.xy;
}

vec3 unpack_normal(vec2 packed_normal) {
    return vec3(packed_normal, 1 - packed_normal.x - packed_normal.y);
}

void waves(vec2 uvs, out vec3 offset, out vec3 normal, out vec3 color) {
    return;
    float pos = world_time + uvs.y * 1000;
    offset.z = sin(pos) * 2;
}


void main()
{
    /* [1] transform 2D pos to 3D pos */

    // Vertex position in 2D space
    vec2 grid_2d_pos_base = (lod_local_transform * vec4(pos, 1)).xz;
    // Clamp position to [-PI/2, PI/2] (don't loop around planet)
    vec2 grid_2d_pos = clamp(grid_2d_pos_base, -radius * PI / 2, radius * PI / 2);
    vec2 grid_2d_pos_tx = clamp(grid_2d_pos_base, -radius * PI / 2, radius * PI / 2) + vec2(radius * PI / 2, 0); // 2D pos 90° along x axis
    vec2 grid_2d_pos_ty = clamp(grid_2d_pos_base, -radius * PI / 2, radius * PI / 2) + vec2(0, radius * PI / 2 * 1); // 2D pos 90° along y axis
    
	vec3 planet_view_pos = grid_to_sphere(grid_2d_pos, radius);

    // Used to compute tangent and bitangent
    vec3 planet_pos = planet_view_pos + vec3(radius, 0, 0);
    vec3 planet_pos_tx = grid_to_sphere_centered(grid_2d_pos_tx, radius);
    vec3 planet_pos_ty = vec3(grid_to_sphere_centered(grid_2d_pos_ty, radius));
    
    // Compute normals, tangent and bi-tangent
    vec3 sphere_normal = normalize(planet_pos);
    vec3 sphere_bitangent = normalize(planet_pos_tx);
    vec3 sphere_tangent = normalize(planet_pos_ty);

    // Compute world space TBN
    mat3 sphere_TBN = mat3(model) * mat3(sphere_tangent, sphere_bitangent, sphere_normal);

    /**
    /*  LOAD CHUNK DATA
    **/

    // Transform vertex coordinates to pixel position (used to fetch precomputed chunk data)
    ivec2 coords = ivec2(pos.xz + grid_cell_count * 2 + 2);

    // Load chunk normals and heightmap
    vec3 tex_normal = normalize(unpack_normal(texelFetch(normal_map, coords, 0).xy)); // Normals are encoded into RG16f pixels
    vec2 altitudes = texelFetch(height_map, coords, 0).rg;
    float vertex_height = altitudes.r;
    float real_height = altitudes.g;

    /**
    /* Compute world space data
    **/

    // Compute texture coordinates (still some artefacts to fix)
    vec2 text_coords = vec2(mod(seamless_uv_from_sphere_normal(dvec3((mat3(planet_world_orientation) * normalize(planet_pos))) * 1000), 1));

    vec3 wave_offset = vec3(0);
    vec3 wave_color = vec3(1);
    vec3 wave_normal = tex_normal;
    if (real_height <= 0)
        waves(text_coords, wave_offset, wave_normal, wave_color);

    // Compute vertex position (with altitude)
    vec4 world_position = model * vec4(planet_view_pos, 1) + vec4(sphere_TBN * vec3(0, 0, vertex_height), 0) + vec4(sphere_TBN * wave_offset, 0);
    vec3 world_normals = sphere_TBN * wave_normal;

    /**
    /*  OUTPUTS
    **/

    // To fragment shader
	g_WorldPosition = world_position.xyz;
    g_LocalNormal = wave_normal;
    g_Normal = world_normals;
    g_Altitude = real_height;
    g_PlanetRadius = radius;
    g_TextureCoordinates = text_coords;
    g_Tangent = sphere_tangent;
    g_BiTangent = sphere_bitangent;
    
    // Vertex position
	gl_Position = pv_matrix * world_position; 
}