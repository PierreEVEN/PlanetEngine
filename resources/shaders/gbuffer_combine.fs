#version 430
precision highp float;

out vec4 oFragmentColor;

layout(location = 0) in vec2 uv;
uniform sampler2D position;
uniform sampler2D color;
uniform sampler2D normal;
uniform sampler2D depth;

vec3 light_dir = normalize(vec3(1, 1, 1));

void main()
{
	float depth = texture(depth, uv).r;

	if (depth <= 0) {
		oFragmentColor = vec4(.55078125, .765625, 0.828125, 1);
		return;
	}

	vec3 color = texture(color, uv).rgb;
	vec3 position = texture(position, uv).rgb;
	vec3 normal = normalize(texture(normal, uv).rgb);
	oFragmentColor = vec4(color * dot(normal, light_dir), 1);
}