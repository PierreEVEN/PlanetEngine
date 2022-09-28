#version 430
precision highp float;

out vec4 oFragmentColor;

layout(location = 0) in vec2 uv;
uniform sampler2D color;
uniform sampler2D normal;

vec3 light_dir = normalize(vec3(1, 1, 1));

void main()
{
	vec3 color = texture(color, uv).rgb;
	vec3 normal = normalize(texture(normal, uv).rgb);
	oFragmentColor = vec4(color * pow(dot(normal, light_dir), 1), 1);
}