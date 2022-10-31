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

layout(location = 21) uniform mat3 scene_rotation;
layout(location = 22) uniform mat3 inv_scene_rotation;

// Outputs
layout(location = 0) out vec3 g_Normal;
layout(location = 1) out vec3 g_WorldPosition;
layout(location = 2) out float g_Altitude;
layout(location = 3) out vec2 g_TextureCoordinates;
layout(location = 4) out float g_PlanetRadius;
layout(location = 5) out vec4 g_DebugScalar;
layout(location = 6) out vec3 g_LocalNormal;
layout(location = 7) out vec3 g_Tangent;
layout(location = 8) out vec3 g_BiTangent;

vec3 unpack_normal(vec2 packed_normal) {
    return vec3(packed_normal, 1 - packed_normal.x - packed_normal.y);
}

vec3 unpack_tangent(float packed_tangent_z) {
    return vec3(sqrt(1 - packed_tangent_z * packed_tangent_z), 0, packed_tangent_z);
}

vec3 unpack_bi_tangent(float packed_bi_tangent_z) {
    return vec3(0, sqrt(1 - packed_bi_tangent_z * packed_bi_tangent_z), packed_bi_tangent_z);
}

void waves(vec2 uvs, out vec3 offset, out vec3 normal, out vec3 color) {
    return;
    float pos = world_time + uvs.y * 1000;
    offset.z = sin(pos) * 2;
}


vec2 uv_from_sphere_pos(vec3 sphere_norm, vec3 world_norm, out vec3 tang, out vec3 bitang) {

    vec3 abs_norm = abs(sphere_norm);
    const float multiplier = 1 / PI * 2;
    world_norm /= multiplier;

    if (abs_norm.z > abs_norm.x && abs_norm.z > abs_norm.y && sphere_norm.z > 0)
    {
        float x = fma(atan(sphere_norm.x, sphere_norm.z), multiplier, 0.5);
        float y = fma(atan(sphere_norm.y, sphere_norm.z), multiplier, 0.5);

        float xa = cos(abs(world_norm.x));
        float za = cos(world_norm.y) * sin(-world_norm.x);
        float ya_square = 1 - xa * xa - za * za;
        float ya = ya_square > 0 ? sqrt(ya_square) * -sign(world_norm.x) * sign(world_norm.y) : 0;

        float yb = cos(abs(world_norm.y));
        float zb = cos(world_norm.x) * sin(-world_norm.y);
        float xb_square = 1 - xa * xa - za * za;
        float xb = xb_square > 0 ? sqrt(xb_square) * -sign(world_norm.y) * sign(world_norm.x) : 0;

        tang = vec3(xa, ya, za);
        bitang = vec3(xb, yb, zb);
        return vec2(x, y);
    }


    if (abs_norm.z > abs_norm.x && abs_norm.z > abs_norm.y && sphere_norm.z < 0)
    {
        float x = fma(atan(sphere_norm.x, sphere_norm.z), multiplier, 0.5);
        float y = fma(atan(sphere_norm.y, sphere_norm.z), multiplier, 0.5);

        float xa = cos(abs(world_norm.x));
        float za = -cos(world_norm.y) * sin(-world_norm.x);
        float ya_square = 1 - xa * xa - za * za;
        float ya = ya_square > 0 ? sqrt(ya_square) * -sign(world_norm.x) * sign(world_norm.y) : 0;

        float yb = cos(abs(world_norm.y));
        float zb = cos(world_norm.x) * sin(-world_norm.y);
        float xb_square = 1 - xa * xa - za * za;
        float xb = xb_square > 0 ? sqrt(xb_square) * -sign(world_norm.y) * sign(world_norm.x) : 0;

        tang = vec3(xa, -ya, za);
        bitang = vec3(xb, -yb, zb);
        return vec2(x, y);
    }

    tang = vec3(0);
    bitang = vec3(0);
    return vec2(0, 0);
}





void main()
{
    /* [1] transform 2D pos to 3D pos */

    // Vertex position in 2D space
    vec2 grid_2d_pos_base = (lod_local_transform * vec4(pos, 1)).xz;
    // Clamp position to [-PI/2, PI/2] (don't loop around planet)
    vec2 grid_2d_pos = clamp(grid_2d_pos_base, -radius * PI / 2, radius * PI / 2);    
	vec3 planet_view_pos = grid_to_sphere(grid_2d_pos, radius);

    // Used to compute tangent and bitangent
    vec3 planet_pos = planet_view_pos + vec3(radius, 0, 0);
    
    // Compute normals, tangent and bi-tangent
    vec3 sphere_normal = mat3(model) * normalize(planet_pos);
    vec3 sphere_normal2 = mat3(planet_world_orientation) * normalize(planet_pos);
    
    /**
    /*  LOAD CHUNK DATA
    **/

    // Transform vertex coordinates to pixel position (used to fetch precomputed chunk data)
    ivec2 coords = ivec2(pos.xz + grid_cell_count * 2 + 2);

    // Load chunk normals
    vec4 tang_bitang = texelFetch(normal_map, coords, 0);
    vec3 tex_tangent = unpack_tangent(-tang_bitang.y); // OK ON TOUCHE PAS
    vec3 tex_bi_tangent = unpack_bi_tangent(tang_bitang.x); // OK AUSSI
    vec3 tex_normal = normalize(cross(tex_tangent, tex_bi_tangent)); // ET CA AUSSI

    // Load chunk heightmap
    vec2 altitudes = texelFetch(height_map, coords, 0).rg;
    float vertex_height = altitudes.r;
    float real_height = altitudes.g;

    /**
    /* Compute world space data
    **/

    // Compute texture coordinates (still some artefacts to fix)
    vec2 text_coords = vec2(mod(seamless_uv_from_sphere_normal(dvec3((mat3(planet_world_orientation) * normalize(planet_pos))) * 1000), 1));

    text_coords = vec2(uv_from_sphere_normal(dvec3((mat3(planet_world_orientation) * normalize(planet_pos)))));


    vec3 sphere_bitangent = vec3(0);
    vec3 sphere_tangent = vec3(0);

    vec3 test_tang = normalize(cross(vec3(0, 1, 0), sphere_normal2));
    vec3 test_bitang = normalize(cross(test_tang, sphere_normal2));
    test_tang = normalize(cross(test_bitang, sphere_normal2));


    text_coords = uv_from_sphere_pos(rotation_from_mat4(planet_world_orientation) * normalize(planet_pos), rotation_from_mat4(model) * normalize(planet_pos), sphere_tangent, sphere_bitangent) * 10;
    sphere_tangent = mat3(scene_rotation) * test_tang;//sphere_tangent;
    sphere_bitangent = mat3(scene_rotation) * test_bitang;// sphere_bitangent;
    g_DebugScalar = vec4(sphere_tangent, 1);

    // Compute world space TBN
    mat3 sphere_TBN = mat3(sphere_tangent, sphere_bitangent, sphere_normal);

    vec3 wave_color = vec3(1);

    // Compute vertex position (with altitude)
    vec4 world_position = model * vec4(planet_view_pos, 1) + vec4(sphere_TBN * vec3(0, 0, vertex_height), 0);
    vec3 world_normals = sphere_TBN * tex_normal;
    vec3 world_tangent = sphere_TBN * tex_tangent;
    vec3 world_bi_tangent = sphere_TBN * tex_bi_tangent;

    /**
    /*  OUTPUTS
    **/

    // To fragment shader
	g_WorldPosition = world_position.xyz;
    g_LocalNormal = tex_normal;
    g_Altitude = real_height;
    g_PlanetRadius = radius;
    g_TextureCoordinates = text_coords;
    
    g_Normal = mat3(scene_rotation) * tex_normal;
    g_Tangent = mat3(scene_rotation) * tex_tangent;
    g_BiTangent = mat3(scene_rotation) * tex_bi_tangent;
    
    // Vertex position
	gl_Position = pv_matrix * world_position; 
}