#version 460

#include "libs/world_data.cginc"
#include "libs/maths.cginc"

layout(location = 0) in vec3 pos;

layout(location = 0) out vec3 out_norm;
layout(location = 1) out vec3 out_pos;
layout(location = 2) out vec4 out_col;

layout(location = 1) uniform sampler2D Scene_color;
layout(location = 2) uniform sampler2D Scene_normal;
layout(location = 3) uniform sampler2D Scene_mrao;
layout(location = 4) uniform sampler2D Scene_depth;

vec3 getSceneWorldDirection(vec2 clipSpacePosition) {

    // Transform local space to view space
    vec4 viewSpacePosition = proj_matrix_inv * vec4(clipSpacePosition, 1, 1);

    viewSpacePosition /= viewSpacePosition.w;

    // Transform view space to world space
    vec4 worldSpacePosition = view_matrix_inv * viewSpacePosition;
    return normalize(worldSpacePosition.xyz);
}


float wave_f(float w, float k) {
	return 2 * pow((sin(w) + 1) / 2.0, k);
}


struct Wave {
	vec3 P;
	vec3 B;
	vec3 T;
	vec3 N;
};

Wave single_wave(vec2 pos, float A, float w, vec2 d, float Q, float phi_t) {
	Wave wave;

	float QA = Q * A;

	wave.P = vec3(
		QA * d.x * cos(w * dot(d, pos) + phi_t),
		QA * d.y * cos(w * dot(d, pos) + phi_t),
		A * sin(w * dot(d, pos) + phi_t)
	);

	float WA = w * A;
	float C = cos(w * dot(d, wave.P.xy + pos) + phi_t);
	float S = sin(w * dot(d, wave.P.xy + pos) + phi_t);

	wave.B = vec3(
		Q * d.x * d.x * WA * S,
		Q * d.x * d.y * WA * S,
		d.x * WA * C
	);

	wave.T = vec3(
		Q * d.x * d.y * WA * S,
		Q * d.y * d.y * WA * S,
		d.y * WA * C
	);

	wave.N = vec3(
		d.x * WA * C,
		d.y * WA * C,
		Q * WA * S
	);

	return wave;
}

Wave add(Wave w0, Wave w1) {
	Wave result;
	result.P = w0.P + w1.P;
	result.B = w0.B + w1.B;
	result.T = w0.T + w1.T;
	result.N = w0.N + w1.N;
	return result;
}

Wave compute(Wave w, vec2 pos) {
	Wave result;

	result.P = vec3(
		pos.x + w.P.x,
		pos.y + w.P.y,
		w.P.z
	);

	result.B = vec3(
		1 - w.B.x,
		-w.B.y,
		w.B.z
	);

	result.T = vec3(
		-w.T.x,
		1 - w.T.y,
		w.T.z
	);

	result.N = vec3(
		-w.N.x,
		-w.N.y,
		1 - w.N.z
	);

	return result;
}

vec3 gerstner_waves(vec2 base_position, float time, out vec3 local_normal, float Amplitude) {

	vec2 d = normalize(vec2(1, 1));
	float phi = 0.2;

	float gravity = 9.81;
	float L = 200000;

	// wi
	float scale_w = sqrt(gravity * 2 * PI / L);

	float q = 0.9; // Stepness

	Wave w0 = single_wave(base_position, Amplitude, scale_w, d, q, phi * time);
	int octaves = 10;
	for (int i = 0; i < octaves; ++i) {
		Amplitude *= 0.8;
		phi *= 1.2;
		scale_w *= 1.2;
		q *= 1;
		float dir_value = d.x * 891.37 - d.y * 79.549865489;
		d = vec2(cos(dir_value), sin(dir_value));
		w0 = add(w0, single_wave(base_position, Amplitude, scale_w, d, q, phi * time));
	}

	Wave final_wave = compute(w0, base_position);

	local_normal = normalize(final_wave.N);

	return final_wave.P;
}

void uv_from_sphere_pos(vec3 world_norm, out vec3 tang, out vec3 bitang) {

    const float multiplier = 1 / PI * 2;

    float xa = cos(abs(world_norm.x / multiplier));
    float ya = cos(abs(world_norm.y / multiplier));

    float zy = sqrt(1 - ya * ya) * sign(-world_norm.y);
    float zx = sqrt(1 - xa * xa) * sign(-world_norm.x) * sign(world_norm.z);

    tang = vec3(xa, 0, zx);
    bitang = vec3(0, ya, zy);
}

void main()
{
	int res = 1024;
	vec3 screen_pos = Rz(-PI / 2) * pos;
	screen_pos = vec3(screen_pos.x, -screen_pos.y, 0) / (res - 1) * 2 + vec3(-1, 1, 0);

	vec3 world_direction = getSceneWorldDirection(screen_pos.xy);


	vec3 planet_pos = vec3(0);
	float planet_radius = 6000000;

    RaySphereTraceResult sunInfos = raySphereIntersection(planet_pos, planet_radius, world_direction, camera_pos);

    float outMax = min(sunInfos.atmosphereDistanceOut, 100000000000.0);
	float distance_to_atmosphere = sunInfos.atmosphereDistanceIn;
    float distanceThroughAtmosphere = outMax - sunInfos.atmosphereDistanceIn;

	
	if (distanceThroughAtmosphere < 0) {
		gl_Position = vec4(-100000000.0);
		return;
	}
	
	float distance = sunInfos.atmosphereDistanceIn;
	vec3 world_pos = distance * world_direction;
	out_norm = normalize(world_pos + camera_pos);

	vec2 coords_2d = (world_pos + camera_pos).xy ;
	
	vec3 wave_norm = vec3(0,0,1);



	vec3 v1 = gerstner_waves(coords_2d, world_time, wave_norm, 4 * clamp_01(1 + (1000 - length(world_pos)) * 0.00002));

	world_pos = v1 + vec3(-camera_pos.xy,world_pos.z);


	gl_Position = pv_matrix * vec4(world_pos, 1);


	vec2 final_screen_pos = (gl_Position.xy / gl_Position.w + 1) / 2;
	
	float z_near = 0.2;
	float background_depth = texture(Scene_depth, final_screen_pos).r;

	float water_depth = background_depth - distance;



	out_col = vec4(0.3, 0.5, 1, 0.8);

	out_norm = wave_norm;


	vec3 tang = vec3(0);
	vec3 bitang = vec3(0);
	vec3 world_norm = normalize(world_pos + camera_pos - planet_pos);
	uv_from_sphere_pos(world_norm, tang, bitang);

	mat3 TBN = mat3(tang, bitang, world_norm);

	out_norm = TBN * out_norm;

	return;
}