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
layout(location = 5) out vec4 g_DebugScalar;
layout(location = 6) out vec3 g_LocalNormal;
layout(location = 7) out vec3 g_Tangent;
layout(location = 8) out vec3 g_BiTangent;

vec3 unpack_normal(vec2 packed_normal) {
    return vec3(packed_normal, 1 - packed_normal.x - packed_normal.y);
}

vec3 unpack_tangent(float packed_tangent_z) {
    return vec3(1 - abs(packed_tangent_z), 0, packed_tangent_z);
}

vec3 unpack_bi_tangent(float packed_bi_tangent_z) {
    return vec3(0, 1 - abs(packed_bi_tangent_z), packed_bi_tangent_z);
}

void waves(vec2 uvs, out vec3 offset, out vec3 normal, out vec3 color) {
    return;
    float pos = world_time + uvs.y * 1000;
    offset.z = sin(pos) * 2;
}

mat3 Rx(float theta) {    
    mat3 _rx;
    _rx[0] = vec3(1,0,0);
    _rx[1] = vec3(0,cos(theta),-sin(theta));
    _rx[2] = vec3(0,sin(theta),cos(theta));
    return _rx;
}

mat3 Ry(float theta) {    
    mat3 _ry;
    _ry[0] = vec3(cos(theta),0,sin(theta));
    _ry[1] = vec3(0, 1, 0);
    _ry[2] = vec3(-sin(theta),0,cos(theta));
    return _ry;
}

mat3 Rz(float theta) {    
    mat3 _rz;
    _rz[0] = vec3(cos(theta),-sin(theta), 0);
    _rz[1] = vec3(-sin(theta),cos(theta), 0);
    _rz[2] = vec3(0, 0, 1);
    return _rz;
}


vec2 uv_from_sphere_pos(vec3 sphere_norm, vec3 world_norm, out vec3 tang, out vec3 bitang) {

    vec3 abs_norm = abs(sphere_norm);
    const float multiplier = 1 / PI * 2;
        
    if (abs_norm.z > abs_norm.x && abs_norm.z > abs_norm.y && sphere_norm.z > 0 ||true)
    {
        sphere_norm = world_norm;
        float x = fma(atan(sphere_norm.x, sphere_norm.z), multiplier, 0.5);
        float y = fma(atan(sphere_norm.y, sphere_norm.z), multiplier, 0.5);

        vec3 test = normalize(vec3(sphere_norm.x, 0, sphere_norm.z));

        float xa = asin(test.z) * multiplier;
        float za = -sphere_norm.x;
        float ya = (1 - xa - abs(za)) * -sign(sphere_norm.x) * sign(sphere_norm.y);

        tang = (vec3(xa, 0, za));
        tang = vec3(length(tang), 0, 0);
        return vec2(x, y);
    }
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
    vec2 grid_2d_pos_tx = clamp(grid_2d_pos_base, -radius * PI / 2, radius * PI / 2) + vec2(radius * PI / 2, 0); // 2D pos 90° along x axis
    vec2 grid_2d_pos_ty = clamp(grid_2d_pos_base, -radius * PI / 2, radius * PI / 2) + vec2(0, radius * PI / 2 * 1); // 2D pos 90° along y axis

    // Used to compute tangent and bitangent
    vec3 planet_pos = planet_view_pos + vec3(radius, 0, 0);
    vec3 planet_pos_tx = grid_to_sphere_centered(grid_2d_pos_tx, radius);
    vec3 planet_pos_ty = vec3(grid_to_sphere_centered(grid_2d_pos_ty, radius));
    
    float theta = PI / 2 * 1;//debug_vector.x;

    mat3 wo;
    wo[0] = normalize(mat3(model)[0]);
    wo[1] = normalize(mat3(model)[1]);
    wo[2] = normalize(mat3(model)[2]);

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
    vec4 tang_bitang = texelFetch(normal_map, coords, 0);
    vec3 tex_tangent = unpack_tangent(tang_bitang.x);
    vec3 tex_bi_tangent = unpack_bi_tangent(tang_bitang.y);
    vec3 tex_normal = normalize(cross(tex_tangent, tex_bi_tangent));
    vec2 altitudes = texelFetch(height_map, coords, 0).rg;
    float vertex_height = altitudes.r;
    float real_height = altitudes.g;

    /**
    /* Compute world space data
    **/

    // Compute texture coordinates (still some artefacts to fix)
    vec2 text_coords = vec2(mod(seamless_uv_from_sphere_normal(dvec3((mat3(planet_world_orientation) * normalize(planet_pos))) * 1000), 1));

    text_coords = vec2(uv_from_sphere_normal(dvec3((mat3(planet_world_orientation) * normalize(planet_pos)))));

    vec3 tang = vec3(0);
    vec3 bitang = vec3(0);

    text_coords = uv_from_sphere_pos(rotation_from_mat4(planet_world_orientation) * normalize(planet_pos), rotation_from_mat4(model) * normalize(planet_pos), tang, bitang);
    g_DebugScalar = vec4(tang, 1);


    vec3 wave_offset = vec3(0);
    vec3 wave_color = vec3(1);

    // Compute vertex position (with altitude)
    vec4 world_position = model * vec4(planet_view_pos, 1) + vec4(sphere_TBN * vec3(0, 0, vertex_height), 0) + vec4(sphere_TBN * wave_offset, 0);
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
    g_Normal = world_normals;
    g_Tangent = world_tangent;
    g_BiTangent = world_bi_tangent;
    
    // Vertex position
	gl_Position = pv_matrix * world_position; 
}