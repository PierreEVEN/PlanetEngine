#version 430

layout(location = 0) in vec3 pos;
layout(location = 1) in vec3 norm;
layout(location = 2) in vec2 tc;
layout(location = 3) in vec3 colors;

layout(location = 0) out vec3 out_norm;
layout(location = 1) out float time;
layout(location = 2) out vec2 out_tc;
layout(location = 3) out vec3 out_color;
layout(location = 4) out float altitude;


layout(location = 1) uniform mat4 model;

layout (std140, binding = 0) uniform WorldData
{
    mat4 proj_matrix;
    mat4 view_matrix;
    mat4 pv_matrix;
    float world_time;
};

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
	return pNoise(pos, 1) * 100 + pNoise(pos, 4) * 50 + pNoise(pos, 10) * 20;
 }


void main()
{
    time = world_time;
    out_tc = tc;
	out_color = abs(norm);
	
	vec4 final_pos = model * vec4(pos, 1);

	vec3 h0 = vec3(0, 0, get_height_at_location(final_pos.xy));
	vec3 h1 = vec3(1, 0, get_height_at_location(final_pos.xy + vec2(1, 0)));
	vec3 h2 = vec3(0, 1, get_height_at_location(final_pos.xy + vec2(0, 1)));

    out_norm = normalize(cross(h1 - h0, h2 - h0));

	final_pos.z += h0.z;
	altitude = final_pos.z;
	if (final_pos.z < 1) {
		final_pos.z = 1;
	}

	gl_Position = pv_matrix * final_pos;
}