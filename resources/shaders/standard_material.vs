#version 430

layout(location = 0) in vec3 pos;
layout(location = 1) in vec3 norm;
layout(location = 2) in vec2 tc;
layout(location = 3) in vec3 colors;

layout(location = 0) out vec3 out_norm;
layout(location = 1) out float time;
layout(location = 4) out float altitude;


layout(location = 1) uniform mat4 model;
layout(location = 2) uniform float inner_width;
layout(location = 3) uniform float outer_width;
layout(location = 4) uniform float cell_width;

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
	return pNoise(pos, 1) * 100 + pNoise(pos, 4) * 50 + pNoise(pos, 10) * 20 +  pNoise(pos * 0.001, 7) * 400 - 10;
 }

float altitude_with_water(float altitude) {
	return altitude < 1 ? 1 : altitude;
}


float square_distance(vec3 a, vec3 b) {
	return 
		max(abs(a.x - b.x),
		max(abs(a.y - b.y),
		abs(a.z - b.z)
	));
}

void main()
{
    time = world_time;

	mat4 model_no_rotation = model;
	model_no_rotation[3][0] = 0;
	model_no_rotation[3][1] = 0;
	model_no_rotation[3][2] = 0;
	
	vec3 final_pos = (model * vec4(pos, 1)).xyz;
	vec3 final_pos_unrotated = (model_no_rotation * vec4(pos, 1)).xyz;
	vec3 center = (model * vec4(0, 0, 0, 1)).xyz;

	float center_distance_normalized = (square_distance(center, final_pos) - inner_width) / (outer_width - inner_width);

	vec3 normalized_pos_direction = normalize(final_pos_unrotated);
	vec2 normalized_direction = vec2(abs(
		normalized_pos_direction.x) < abs(normalized_pos_direction.y) ? 
			normalized_pos_direction.y < 0 ? -1 : 1 :
			0,
		abs(normalized_pos_direction.y) < abs(normalized_pos_direction.x) ? 
			normalized_pos_direction.x < 0 ? -1 : 1 :
			0);

	// Compute normals
	vec3 h0 = vec3(0, 0, get_height_at_location(final_pos.xy));
	vec3 h1 = vec3(1, 0, get_height_at_location(final_pos.xy + vec2(1, 0)));
	vec3 h2 = vec3(0, 1, get_height_at_location(final_pos.xy + vec2(0, 1)));
    out_norm = normalize(cross(h1 - h0, h2 - h0));

	altitude = h0.z;


	float h_left = altitude_with_water(get_height_at_location(final_pos.xy + normalized_direction * cell_width * 1));
	float h_right = altitude_with_water(get_height_at_location(final_pos.xy - normalized_direction * cell_width * 1));
	float h_mean = (h_left + h_right) / 2;

	final_pos.z = mix(altitude_with_water(altitude), h_mean, pos.z );

	// Underwater normal and depth
	float depth_scale = clamp(-altitude / 10, 0, 1);
	if (altitude < 1) {
		out_norm = mix(vec3(0,0,1), out_norm, 1 - depth_scale);
	}

	// Morph to sphere
	float planet_radius = 8000;
	final_pos = vec3(sin(final_pos.x / planet_radius), sin(final_pos.y / planet_radius), cos(final_pos.x / planet_radius) * cos(final_pos.y / planet_radius)) * planet_radius;
	final_pos += normalize(final_pos.xyz) * altitude;
	final_pos -= vec3(0, 0, planet_radius);

	gl_Position = pv_matrix * vec4(final_pos, 1.0);
}