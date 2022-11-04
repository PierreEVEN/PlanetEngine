#version 430 core
#extension GL_ARB_enhanced_layouts : enable
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
layout(location = 0) out vec3 g_LocalNormal;
layout(location = 1) out vec3 g_WorldPosition;
layout(location = 2) out float g_Altitude;
layout(location = 3) out vec2 g_TextureCoordinates;
layout(location = 4) out float g_PlanetRadius;
layout(location = 5) out vec3 g_DebugScalar;
layout(location = 6) out vec3 g_Normal;
layout(location = 7) out vec3 g_Tangent;
layout(location = 8) out vec3 g_BiTangent;

vec2 uv_from_sphere_pos(vec3 sphere_norm, vec3 world_norm, out vec3 tang, out vec3 bitang) {

    vec3 abs_norm = abs(sphere_norm);
    const float multiplier = 1 / PI * 2;

    float xa = cos(abs(world_norm.x / multiplier));
    float ya = cos(abs(world_norm.y / multiplier));

    float x = 0;
    float y = 0;
    // TOP
    if (abs_norm.z > abs_norm.x && abs_norm.z > abs_norm.y && sphere_norm.z > 0)
    {

         x = fma(atan(sphere_norm.x, sphere_norm.z), multiplier, 0.5);
         y = fma(atan(sphere_norm.y, sphere_norm.z), multiplier, 0.5);         
    }
    // FRONT
    else if (abs_norm.x > abs_norm.y && abs_norm.x > abs_norm.z && sphere_norm.x > 0)
    {
        x = fma(atan(sphere_norm.x, sphere_norm.z), multiplier, -0.5);
        y = fma(atan(sphere_norm.y, sphere_norm.x), multiplier, 0.5);
        
        xa = cos(abs(world_norm.x / multiplier)) * sign(world_norm.z);
        ya = cos(abs(world_norm.y / multiplier));
    }
    // BOTTOM
    else if (abs_norm.z > abs_norm.x && abs_norm.z > abs_norm.y && sphere_norm.z < 0)
    {
        x = fma(atan(-sphere_norm.x, -sphere_norm.z), multiplier, 0.5);
        y = fma(atan(sphere_norm.y, -sphere_norm.z), multiplier, 0.5);
        
        xa = -cos(abs(world_norm.x / multiplier));
        ya = cos(abs(world_norm.y / multiplier));
    }
    // BACK
    else if (abs_norm.x > abs_norm.y && abs_norm.x > abs_norm.z && sphere_norm.x < 0)
    {
        x = fma(atan(-sphere_norm.x, -sphere_norm.z), multiplier, -0.5);
        y = fma(atan(sphere_norm.y, -sphere_norm.x), multiplier, 0.5);
    }
    // LEFT
    else if (abs_norm.y > abs_norm.x && abs_norm.y > abs_norm.z && sphere_norm.y > 0)
    {
        x = fma(atan(sphere_norm.x, sphere_norm.y), multiplier, 0.5);
        y = fma(atan(-sphere_norm.y, sphere_norm.z), multiplier, 1.5);
    }
    // RIGHT
    else if (abs_norm.y > abs_norm.x && abs_norm.y > abs_norm.z && sphere_norm.y < 0)
    {
        x = fma(atan(-sphere_norm.x, sphere_norm.y), multiplier, -0.5);
        y = fma(atan(-sphere_norm.y, sphere_norm.z), multiplier, -0.5);
    }

    float zy = sqrt(1 - ya * ya) * sign(-world_norm.y);
    float zx = sqrt(1 - xa * xa) * sign(-world_norm.x) * sign(world_norm.z);

    tang = vec3(xa, 0, zx);
    bitang = vec3(0, ya, zy);

    tang = scene_rotation * tang;
    bitang = scene_rotation * bitang;

    x = mod(x * 1000, 1);
    y = mod(y * 1000, 1);

    return vec2(x, y);
}

struct LocalData {
    vec2 pos2D;
    vec3 tangent;
    vec3 bitangent;
    vec3 normal;
    float vertex_altitude;
    float absolute_altitude;
};

LocalData compute_local_space_data() {
    ivec2 coords = ivec2(pos.xz + grid_cell_count * 2 + 2);

    LocalData result;

    result.pos2D = clamp((lod_local_transform * vec4(pos, 1)).xz, -radius * PI / 2, radius * PI / 2);

    vec4 tang_bitang = texelFetch(normal_map, coords, 0);
    result.tangent = unpack_tangent_z(-tang_bitang.y);
    result.bitangent = unpack_bi_tangent_z(tang_bitang.x);
    result.normal = normalize(cross(result.tangent, result.bitangent));

    // Load chunk heightmap
    vec2 altitudes = texelFetch(height_map, coords, 0).rg;
    result.vertex_altitude = altitudes.r;
    result.absolute_altitude = altitudes.g;
    return result;
}

void fix_tbn(inout vec3 tang, inout vec3 bitang, vec3 normal) {
    
    vec3 temp_bitang = cross(tang, normal);
    tang = cross(normal, temp_bitang);

    vec3 temp_tang = cross(bitang, normal);
    bitang = cross(normal, temp_tang);


}


void main()
{
    LocalData local_space_data = compute_local_space_data();                                //OK
	vec3 sphere_pos_local_offset = grid_to_sphere(local_space_data.pos2D, radius);          //OK
    vec3 sphere_pos_local = sphere_pos_local_offset + vec3(radius, 0, 0);                   //OK
    vec3 local_sphere_normal = normalize(sphere_pos_local);                                 //OK
    vec3 sphere_normal_world_space = mat3(model) * local_sphere_normal;                     //OK
    vec3 sphere_normal_planet_space = mat3(planet_world_orientation) * local_sphere_normal; //OK

    /**
    /* Compute world space data
    **/

    vec3 sphere_bitangent = vec3(0);
    vec3 sphere_tangent = vec3(0);
    vec2 text_coords = uv_from_sphere_pos(sphere_normal_planet_space, sphere_normal_world_space, sphere_tangent, sphere_bitangent);// * 1000;
    g_DebugScalar = vec3(text_coords, 0);

    // Compute world space TBN
    //fix_tbn(sphere_tangent, sphere_bitangent, sphere_normal_world_space);
    mat3 sphere_TBN = mat3(sphere_tangent, sphere_bitangent, sphere_normal_world_space);

    


    // sphere_TBN = mat3(1);

    // Compute vertex position (with altitude)
    vec4 world_position = model * vec4(sphere_pos_local_offset, 1) + vec4(sphere_TBN * vec3(0, 0, local_space_data.vertex_altitude), 0);
    vec3 world_normals = sphere_TBN * local_space_data.normal;
    vec3 world_tangent = sphere_TBN * local_space_data.tangent;
    vec3 world_bi_tangent = sphere_TBN * local_space_data.bitangent;

    //fix_tbn(world_tangent, world_bi_tangent, world_normals);

    /**
    /*  VERTEX OUTPUTS
    **/

    // To fragment shader
	g_WorldPosition = world_position.xyz;
    g_LocalNormal = local_space_data.normal;
    g_Altitude = local_space_data.absolute_altitude;
    g_PlanetRadius = radius;
    g_TextureCoordinates = text_coords;
    
    g_Normal = world_normals;
    g_Tangent = world_tangent;
    g_BiTangent = world_bi_tangent;
    
    // Vertex position
	gl_Position = pv_matrix * world_position; 
}