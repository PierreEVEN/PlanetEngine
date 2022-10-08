 #version 430

layout(location = 0) in vec3 pos;
layout(location = 0) out vec3 out_norm;
layout(location = 1) out float time;
layout(location = 2) out vec3 out_position;
layout(location = 3) out float altitude;
layout(location = 4) out vec2 coordinates;

layout(location = 1) uniform mat4 model;
layout(location = 2) uniform mat4 lod_local_transform;
layout(location = 3) uniform float radius;

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

/*
 FROM https://outerra.blogspot.com/2017/06/fp64-approximations-for-sincos-for.html 
*/

//sin approximation, error < 5e-9
double dsin(double x)
{
    //minimax coefs for sin for 0..pi/2 range
    const double a3 = -1.666665709650470145824129400050267289858e-1LF;
    const double a5 =  8.333017291562218127986291618761571373087e-3LF;
    const double a7 = -1.980661520135080504411629636078917643846e-4LF;
    const double a9 =  2.600054767890361277123254766503271638682e-6LF;

    const double m_2_pi = 0.636619772367581343076LF;
    const double m_pi_2 = 1.57079632679489661923LF;

    double y = abs(x * m_2_pi);
    double q = floor(y);
    int quadrant = int(q);

    double t = (quadrant & 1) != 0 ? 1 - y + q : y - q;
    t *= m_pi_2;

    double t2 = t * t;
    double r = fma(fma(fma(fma(a9, t2, a7), t2, a5), t2, a3), t2*t, t);

    r = x < 0 ? -r : r;

    return (quadrant & 2) != 0 ? -r : r;
}
//cos approximation, error < 5e-9
double dcos(double x)
{
    //sin(x + PI/2) = cos(x)
    return dsin(x + 1.57079632679489661923LF);
}

const float PI = 3.14159265358979323846;
const float HALF_PI = 3.14159265358979323846 / 2.0;

vec3 to_3d_v4(vec2 pos, float rho) {
    dvec2 dpos = pos;
	dvec2 norm_pos = clamp(dpos / rho, -HALF_PI, HALF_PI);

    double cos_y = dcos(norm_pos.y);
    return vec3(
        cos_y * dcos(norm_pos.x) - 1,
        cos_y * dsin(norm_pos.x), 
        dsin (norm_pos.y)
    ) * rho;
}

void main()
{
	vec3 post_transform_pos = (lod_local_transform * vec4(pos, 1)).xyz;

	vec3 planet_pos = to_3d_v4(post_transform_pos.xz, radius );
	out_position = planet_pos;
    //out_position = vec3(0, post_transform_pos.x, post_transform_pos.z);

	gl_Position = pv_matrix * model * vec4(out_position, 1.0);
}