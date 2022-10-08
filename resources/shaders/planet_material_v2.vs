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


//	Classic Perlin 3D Noise 
//	by Stefan Gustavson
//
dvec4 permute(dvec4 x){return mod(((x*34.0)+1.0)*x, 289.0);}
dvec4 taylorInvSqrt(dvec4 r){return 1.79284291400159 - 0.85373472095314 * r;}
dvec3 fade(dvec3 t) {return t*t*t*(t*(t*6.0-15.0)+10.0);}

double cnoise(dvec3 P){
  dvec3 Pi0 = floor(P); // Integer part for indexing
  dvec3 Pi1 = Pi0 + dvec3(1.0); // Integer part + 1
  Pi0 = mod(Pi0, 289.0);
  Pi1 = mod(Pi1, 289.0);
  dvec3 Pf0 = fract(P); // Fractional part for interpolation
  dvec3 Pf1 = Pf0 - dvec3(1.0); // Fractional part - 1.0
  dvec4 ix = dvec4(Pi0.x, Pi1.x, Pi0.x, Pi1.x);
  dvec4 iy = dvec4(Pi0.yy, Pi1.yy);
  dvec4 iz0 = Pi0.zzzz;
  dvec4 iz1 = Pi1.zzzz;

  dvec4 ixy = permute(permute(ix) + iy);
  dvec4 ixy0 = permute(ixy + iz0);
  dvec4 ixy1 = permute(ixy + iz1);

  dvec4 gx0 = ixy0 / 7.0;
  dvec4 gy0 = fract(floor(gx0) / 7.0) - 0.5;
  gx0 = fract(gx0);
  dvec4 gz0 = dvec4(0.5) - abs(gx0) - abs(gy0);
  dvec4 sz0 = step(gz0, dvec4(0.0));
  gx0 -= sz0 * (step(0.0, gx0) - 0.5);
  gy0 -= sz0 * (step(0.0, gy0) - 0.5);

  dvec4 gx1 = ixy1 / 7.0;
  dvec4 gy1 = fract(floor(gx1) / 7.0) - 0.5;
  gx1 = fract(gx1);
  dvec4 gz1 = dvec4(0.5) - abs(gx1) - abs(gy1);
  dvec4 sz1 = step(gz1, dvec4(0.0));
  gx1 -= sz1 * (step(0.0, gx1) - 0.5);
  gy1 -= sz1 * (step(0.0, gy1) - 0.5);

  dvec3 g000 = vec3(gx0.x,gy0.x,gz0.x);
  dvec3 g100 = vec3(gx0.y,gy0.y,gz0.y);
  dvec3 g010 = vec3(gx0.z,gy0.z,gz0.z);
  dvec3 g110 = vec3(gx0.w,gy0.w,gz0.w);
  dvec3 g001 = vec3(gx1.x,gy1.x,gz1.x);
  dvec3 g101 = vec3(gx1.y,gy1.y,gz1.y);
  dvec3 g011 = vec3(gx1.z,gy1.z,gz1.z);
  dvec3 g111 = vec3(gx1.w,gy1.w,gz1.w);

  dvec4 norm0 = taylorInvSqrt(dvec4(dot(g000, g000), dot(g010, g010), dot(g100, g100), dot(g110, g110)));
  g000 *= norm0.x;
  g010 *= norm0.y;
  g100 *= norm0.z;
  g110 *= norm0.w;
  dvec4 norm1 = taylorInvSqrt(dvec4(dot(g001, g001), dot(g011, g011), dot(g101, g101), dot(g111, g111)));
  g001 *= norm1.x;
  g011 *= norm1.y;
  g101 *= norm1.z;
  g111 *= norm1.w;

  double n000 = dot(g000, Pf0);
  double n100 = dot(g100, dvec3(Pf1.x, Pf0.yz));
  double n010 = dot(g010, dvec3(Pf0.x, Pf1.y, Pf0.z));
  double n110 = dot(g110, dvec3(Pf1.xy, Pf0.z));
  double n001 = dot(g001, dvec3(Pf0.xy, Pf1.z));
  double n101 = dot(g101, dvec3(Pf1.x, Pf0.y, Pf1.z));
  double n011 = dot(g011, dvec3(Pf0.x, Pf1.yz));
  double n111 = dot(g111, Pf1);

  dvec3 fade_xyz = fade(Pf0);
  dvec4 n_z = mix(dvec4(n000, n100, n010, n110), dvec4(n001, n101, n011, n111), fade_xyz.z);
  dvec2 n_yz = mix(n_z.xy, n_z.zw, fade_xyz.y);
  double n_xyz = mix(n_yz.x, n_yz.y, fade_xyz.x); 
  return 2.2 * n_xyz;
}

void main()
{
	vec3 vertex_pos = (lod_local_transform * vec4(pos, 1)).xyz;
	vec3 planet_pos = to_3d_v4(vertex_pos.xz, radius );
    dmat3 rot = dmat3(model);
    dvec3 norm_f64 = normalize(rot * (planet_pos + dvec3(radius, 0, 0)));
    out_norm = vec3(norm_f64);

    altitude = float(cnoise(out_norm * 2) * 10000.0 + cnoise(out_norm * 10) * 1000.0 + cnoise(out_norm * 1000) * 100.0 + cnoise(out_norm * 10000) * 10.0 + cnoise(out_norm * 100000) * 10.0);

    vec4 world_pos = model * vec4(planet_pos, 1.0);
    world_pos.xyz += out_norm * max(1, altitude);
	out_position = world_pos.xyz;
	gl_Position = pv_matrix * world_pos;
}