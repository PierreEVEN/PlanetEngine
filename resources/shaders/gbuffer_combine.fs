#version 430
precision highp float;

out vec4 oFragmentColor;

layout(location = 0) in vec2 uv;
uniform sampler2D position;
uniform sampler2D color;
uniform sampler2D normal;
uniform sampler2D depth;

vec3 light_dir = normalize(vec3(0, 0, 1));

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
	float depth = texture(depth, uv).r;

	if (depth <= 0) {
		oFragmentColor = vec4(0);//vec4(sin(world_time), .765625, 0.828125, 1);
		return;
	}

	vec3 col = texture(color, uv).rgb;
	vec3 pos = texture(position, uv).rgb;
	vec3 norm = normalize(texture(normal, uv).rgb);


	oFragmentColor = vec4(col * max(0, dot(norm, light_dir)), 1) + 0.04;

	// vec2 device_space = (uv * 2 - 1);
	// vec4 rev_pos = pv_matrix_inv * vec4(device_space.x, -device_space.y, -1, 1);
	// oFragmentColor = vec4(rev_pos.xyz, 1);
}