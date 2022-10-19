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

// Outputs
layout(location = 0) out vec3 g_Normal;
layout(location = 1) out vec3 g_WorldPosition;
layout(location = 2) out float g_Altitude;
layout(location = 3) out vec2 g_TextureCoordinates;
layout(location = 4) out float g_PlanetRadius;
layout(location = 5) out vec3 g_DebugScalar;

vec2 pack_normal(vec3 normal) {
    return normal.xy;
}

vec3 unpack_normal(vec2 packed_normal) {
    return vec3(packed_normal, 1 - length(packed_normal));
}

void main()
{
    /* [1] transform 2D pos to 3D pos */

    // Vertex position in 2D space
    vec2 grid_2d_pos_base = (lod_local_transform * vec4(pos, 1)).xz;
    // Clamp position to [-PI/2, PI/2] (don't loop around planet)
    vec2 grid_2d_pos = clamp(grid_2d_pos_base, -radius * PI / 2, radius * PI / 2);
    vec2 grid_2d_pos_tx = clamp(grid_2d_pos_base, -radius * PI / 2, radius * PI / 2) + vec2(radius * PI / 2 * 1 * 0, 0); // 2D pos 90° along x axis
    vec2 grid_2d_pos_ty = clamp(grid_2d_pos_base, -radius * PI / 2, radius * PI / 2) + vec2(0, radius * PI / 2 * 0); // 2D pos 90° along y axis
    
	vec3 planet_view_pos = grid_to_sphere(grid_2d_pos, radius); //@TODO not actually a readable value    

    // Used to compute tangent and bitangent
    vec3 planet_pos = planet_view_pos + vec3(radius, 0, 0);
    vec3 planet_pos_tx = grid_to_sphere_centered(grid_2d_pos_tx + vec2(radius * PI / 2, 0), radius);
    vec3 planet_pos_ty = grid_to_sphere_centered(grid_2d_pos_ty + vec2(0, radius * PI / 2), radius);
    
    // Compute normals, tangent and bi-tangent
    vec3 sphere_normal = normalize(planet_pos);
    vec3 sphere_bitangent = normalize(planet_pos_tx);
    vec3 sphere_tangent = normalize(planet_pos_ty);

    // Compute world space TBN
    mat3 sphere_TBN = mat3(model) * mat3(sphere_tangent, sphere_bitangent, sphere_normal);

    g_DebugScalar = sphere_TBN * vec3(0,0,1);

    /**
    /*  LOAD CHUNK DATA
    **/

    // Transform vertex coordinates to pixel position (used to fetch precomputed chunk data)
    vec2 coords = (pos.xz + grid_cell_count * 2 + 3) / (grid_cell_count * 4 + 6);

    // Load chunk normals and heightmap
    vec3 tex_normal = unpack_normal(texture(normal_map, coords).xy); // Normals are encoded into RG16f pixels
    float height = texture(height_map, coords).r;

    /**
    /* Compute world space data
    **/

    // Compute texture coordinates (still some artefacts to fix)
    vec2 text_coords = vec2(mod(seamless_uv_from_sphere_normal(dvec3(normalize(planet_pos))) * 1000, 1));

    // Compute vertex position (with altitude)
    vec4 world_position = model * vec4(planet_view_pos, 1) + vec4(sphere_TBN * vec3(0, 0, height < 0 ? 0 : height), 0);
    vec3 world_normals = sphere_TBN * tex_normal;

    /**
    /*  OUTPUTS
    **/

    // To fragment shader
	g_WorldPosition = world_position.xyz;
    g_Normal = world_normals;
    g_Altitude = height;
    g_PlanetRadius = radius;
    g_TextureCoordinates = text_coords;
    
    // Vertex position
	gl_Position = pv_matrix * world_position; 
}