#version 460

#include "libs/world_data.cginc"
#include "libs/maths.cginc"
#include "libs/random.cginc"

layout(location = 0) in vec3 pos;

layout(location = 0) out vec3 out_norm;
layout(location = 1) out vec3 out_pos;
layout(location = 2) out vec3 scene_normal;
layout(location = 3) out float depth;
layout(location = 4) out vec2 uvs;
layout(location = 5) out vec3 world_direction;
layout(location = 6) out float wave_level;

layout(location = 1) uniform sampler2D Scene_depth;
layout(location = 2) uniform sampler2D Scene_color;
layout(location = 3) uniform float z_near;
layout(location = 4) uniform mat4 model;


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

Wave single_wave(vec2 pos, vec2 dir, float A, float w,  float Q, float time_offset) {
	Wave wave;
	float QA = Q * A;

	wave.P = vec3(
		QA * dir.x * cos(w * dot(dir, pos) + time_offset),
		QA * dir.y * cos(w * dot(dir, pos) + time_offset),
		A * sin(w * dot(dir, pos) + time_offset)
	);

	float WA = w * A;
	float C = cos(w * dot(dir, wave.P.xy + pos) + time_offset);
	float S = sin(w * dot(dir, wave.P.xy + pos) + time_offset);

	wave.B = vec3(
		Q * dir.x * dir.x * WA * S,
		Q * dir.x * dir.y * WA * S,
		dir.x * WA * C
	);

	wave.T = vec3(
		Q * dir.x * dir.y * WA * S,
		Q * dir.y * dir.y * WA * S,
		dir.y * WA * C
	);

	wave.N = vec3(
		dir.x * WA * C,
		dir.y * WA * C,
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

vec3 gerstner_waves(vec2 base_position, float time, out vec3 local_normal, float multiplier) {

	float phi = 1;
	float gravity = 9.81;
	float L = 10000 ;

	// wi
	float scale_w = sqrt(gravity * 2 * PI / L);

	float q = 0.7; // Stepness
	float Amplitude = 2 * multiplier * (simplex_noise(base_position * 0.008) * 0.7 + 1);
	Wave w0;
	w0.P = vec3(0);
	w0.T = vec3(0);
	w0.B = vec3(0);
	w0.N = vec3(0);
	int octaves = 15;
	for (int i = 0; i < octaves; ++i) {
		Amplitude = mix(Amplitude, 0.0, 0.2);
		float iter = i * 20.2;
		vec2 dir = vec2(sin(iter), cos(iter));
		phi *= 1.06;
		scale_w *= 1.15;
		w0 = add(w0, single_wave(base_position, dir, Amplitude, scale_w, q, phi * time));
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

vec2 wave_dx(vec2 pos, vec2 dir, float time, float amplitude, float frequency) {
	float x = dot(pos, dir) * frequency + time;
	float height = exp(sin(x) - 1);
	float dx = height * cos(x);
	return vec2(height, -dx);
}

float wave_height(vec2 pos, int details) {

	float time = world_time * 2;
	float amplitude = 1;
	float frequency = 1;



	for (int i = 0; i < details; ++i) {
		float iter = i * 12.2;
		vec2 dir = vec2(sin(iter), cos(iter));
		vec2 wave = wave_dx(pos, dir, amplitude, frequency, time);
	}
	return 0;
}

vec3 getSceneWorldPosition(float linear_depth, vec2 uv) {
    // Get z depth
    float zDepth = linear_depth;

    // compute clip space depth
    vec4 clipSpacePosition = vec4(uv * 2.0 - 1.0, zDepth, 1.0);

    // Transform local space to view space
    vec4 viewSpacePosition = proj_matrix_inv * clipSpacePosition;

    viewSpacePosition /= viewSpacePosition.w;

    // Transform view space to world space
    vec4 worldSpacePosition = view_matrix_inv * viewSpacePosition;
    return worldSpacePosition.xyz;
}

void main()
{
	int res = 1024;
	vec3 screen_pos = Rz(-PI / 2) * pos;
	screen_pos = (vec3(screen_pos.x, -screen_pos.y, 0) / (res - 1) * 2 + vec3(-1, 1, 0)) * 1.2;

	world_direction = getSceneWorldDirection(screen_pos.xy);
	
	vec3 planet_pos = vec3(0);
	float planet_radius = 600000;

    RaySphereTraceResult sunInfos = raySphereIntersection(planet_pos, planet_radius, world_direction, camera_pos);

    float outMax = min(sunInfos.atmosphereDistanceOut, 100000000000.0);
	float distance_to_atmosphere = sunInfos.atmosphereDistanceIn;
    float distanceThroughAtmosphere = outMax - sunInfos.atmosphereDistanceIn;

	
	if (distanceThroughAtmosphere < 0) {
		gl_Position = vec4(-100000000.0);
		return;
	}
	
	float world_distance = sunInfos.atmosphereDistanceIn;
	vec3 world_pos = world_distance * world_direction;
	out_pos = world_pos;
	out_norm = normalize(world_pos + camera_pos);

	vec2 coords_2d = (world_pos + camera_pos).xy;
	
	vec3 wave_norm = vec3(0,0,1);
	vec3 v1 = gerstner_waves(coords_2d, world_time, wave_norm, 0.8 * clamp_01(1 + (1000 - length(world_pos)) * 0.0001));

	wave_level = v1.z;

	world_pos = v1 + vec3(-camera_pos.xy, world_pos.z);


	gl_Position = pv_matrix * vec4(world_pos, 1);


	vec2 final_screen_pos = (gl_Position.xy / gl_Position.w + 1) / 2;
	
	float background_depth = texture(Scene_depth, final_screen_pos).r;

	if (background_depth <= 0)
		background_depth = 0.0000001;

	float background_distance = length(getSceneWorldPosition(background_depth, final_screen_pos));
	
	float water_depth = background_distance - world_distance;

	depth = water_depth;
	uvs = coords_2d;

	out_norm = wave_norm;

	vec3 tang = vec3(0);
	vec3 bitang = vec3(0);
	vec3 world_norm = normalize(world_pos + camera_pos - planet_pos);
	uv_from_sphere_pos(world_norm, tang, bitang);
	scene_normal = world_norm;
	mat3 TBN = mat3(tang, bitang, world_norm);

	out_norm = TBN * out_norm;

	return;
}