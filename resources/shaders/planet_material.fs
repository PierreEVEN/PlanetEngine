#version 430
precision highp float;

layout (location = 0) out vec3 gColor;
layout (location = 1) out vec3 gPosition;
layout (location = 2) out vec3 gNormal;

layout(location = 0) in vec3 normal;
layout(location = 1) in float time;
layout(location = 2) in vec3 position;
layout(location = 3) in float altitude;
layout(location = 4) in vec2 coordinates;

uniform sampler2D grass;
uniform sampler2D sand;
uniform sampler2D rock;

float PI = 3.14159265358979323846;
float rand(vec2 c){
	return fract(sin(dot(c.xy ,vec2(12.9898,78.233))) * 43758.5453);
}
float noise(vec2 p, float freq ){
	float unit = 800/freq;
	vec2 ij = floor(p/unit);
	vec2 xy = mod(p,unit)/unit;
	//xy = 3.*xy*xy-2.*xy*xy*xy;
	xy = .5*(1.-cos(PI*xy));
	float a = rand((ij+vec2(0.,0.)));
	float b = rand((ij+vec2(1.,0.)));
	float c = rand((ij+vec2(0.,1.)));
	float d = rand((ij+vec2(1.,1.)));
	float x1 = mix(a, b, xy.x);
	float x2 = mix(c, d, xy.x);
	return mix(x1, x2, xy.y);
}
float pNoise(vec2 p, int res){
	float persistance = .5;
	float n = 0.;
	float normK = 0.;
	float f = 4.;
	float amp = 1.;
	int iCount = 0;
	for (int i = 0; i<50; i++){
		n+=amp*noise(p, f);
		f*=2.;
		normK+=amp;
		amp*=persistance;
		if (iCount == res) break;
		iCount++;
	}
	float nf = n/normK;
	return nf*nf*nf*nf;
}


float get_height_at_location(vec2 pos) {
	vec2 poss = pos;
	return pNoise(poss, 4) * 100 + pNoise(poss * 3, 4) * 15 + pNoise(poss * 20, 5) * 4 +  pNoise(poss * 0.1 + vec2(330, 100), 2) * 400 - 80;
 }


void main()
{
	vec3 normal_vector = normal;	
	vec3 h0 = vec3(0, 0, get_height_at_location(coordinates.xy));
	vec3 h1 = vec3(1, 0, get_height_at_location(coordinates.xy + vec2(1, 0)));
	vec3 h2 = vec3(0, 1, get_height_at_location(coordinates.xy + vec2(0, 1)));
    normal_vector = normalize(cross(h1 - h0, h2 - h0));

	vec2 uv = position.xy / 5 ;

	vec3 grass_color = texture(grass, uv).rgb;
	vec3 sand_color = texture(sand, uv).rgb;
	vec3 rock_color = texture(rock, uv).rgb;

	float slope = pow(dot(normal_vector, vec3(0,0,1)), 8);
	

	gColor = mix(rock_color, grass_color, slope);

	gColor = mix(sand_color, gColor, clamp(altitude / 1 - 2, 0, 1));

	float depth_scale = clamp(-altitude / 10, 0, 1);

	if (altitude < 1) {
		normal_vector = vec3(0,0,1);
		gColor = mix(vec3(97, 130, 223) / 256, vec3(97, 130, 223) / 350 , depth_scale);
	}

	gNormal = normal_vector;
	gPosition = position;
}