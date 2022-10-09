#version 430
precision highp float;

layout (location = 0) out vec3 gColor;
layout (location = 1) out vec3 gNormal;

layout(location = 0) in vec3 normal;
layout(location = 1) in vec3 debug_scalar;
layout(location = 2) in vec3 position;
layout(location = 3) in float altitude;
layout(location = 4) in vec2 coordinates;

uniform sampler2D grass;
uniform sampler2D sand;
uniform sampler2D rock;
uniform int fragment_normal_maps;
uniform vec3 ground_color;
// layout(location = 5) uniform int fragment_normals;

layout (std140, binding = 0) uniform WorldData
{
    mat4 proj_matrix;
    mat4 view_matrix;
    mat4 pv_matrix;
    mat4 proj_matrix_inv;
    mat4 view_matrix_inv;
    mat4 pv_matrix_inv;
	vec3 camera_pos;
	vec3 camera_forward;
    float world_time;
};

void main()
{
	vec3 normal_vector = normal;



	vec2 uv = position.xy / 2 + camera_pos.zx;

	vec3 rock_color = texture(grass, coordinates * 1000).rgb;
	vec3 sand_color = texture(sand, coordinates * 1000).rgb;
	vec3 grass_color = texture(rock, coordinates * 1000).rgb;
	vec3 snow_color = vec3(0.8, 0.75, 0.8);
	vec3 rock_color_bis = texture(rock, coordinates * 1398.48945).rgb;

	float slope = pow(dot(normal_vector, vec3(0,0,1)), 20) + .2;
	slope = 0;

	gColor = mix((rock_color_bis + rock_color) / 2, grass_color, clamp(slope, 0, 1));

	gColor = mix(sand_color, gColor, clamp((altitude - 5 ) / 10 , 0, 1));

	gColor = mix(gColor, snow_color, clamp((altitude - 4000) / 800, 0, 1));

	float depth_scale = clamp(-altitude / 300, 0, 1);

	if (altitude < 1) {
		gColor = mix(vec3(97, 130, 223) / 256, vec3(97, 130, 223) / 350 , depth_scale);
	}

	gNormal = normal;

	//gColor = vec3(coordinates, 0);
}