#version 430
precision highp float;

out vec4 oFragmentColor;

layout(location = 0) in vec2 uv;
layout(location = 0) uniform sampler2D color;

void main()
{
	oFragmentColor = vec4(texture(color, uv).rgb, 1);
}