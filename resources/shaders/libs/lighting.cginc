#ifndef LIGHTING_H_
#define LIGHTING_H_

#include "maths.cginc"

struct PhongParams {
    float ambiant_strength;
    float specular_strength;
    float specular_shininess;
};

vec3 phong_lighting(vec3 color, vec3 normal, vec3 light_direction, vec3 pixel_direction, PhongParams params) {
    // Ambient lighting
    vec3 ambient = vec3(0.001);

    // Diffuse lighting
    float diffuse = max(dot(normal, light_direction), 0);

    // Specular lighting
    vec3 refl_light = reflect(-light_direction, normal);
    float specular = pow(max(dot(pixel_direction, refl_light), 0), params.specular_shininess) * params.specular_strength;

    // Phong lighting
    return (ambient + diffuse + specular) * color;
}


vec3 blinn_phong_lighting(vec3 color, vec3 normal, vec3 light_direction, vec3 pixel_direction, PhongParams params) {
    // Ambient lighting
    vec3 ambient = vec3(0.001);

    // Diffuse lighting
    float diffuse = max(dot(normal, light_direction), 0);

    // Specular lighting
    vec3 half_way_dir = normalize(light_direction + pixel_direction);
    float specular = pow(max(dot(normal, half_way_dir), 0), params.specular_shininess) * params.specular_strength;

    // Phong lighting
    return (ambient + diffuse + specular) * color;
}






vec3 fresnel_schlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(clamp(1.0 - max(cosTheta, 0), 0.0, 1.0), 5.0);
}

float DistributionGGX(vec3 N, vec3 H, float roughness)
{
    float a      = roughness * roughness;
    float a2     = a * a;
    float NdotH  = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;

    float num   = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom       = PI * denom * denom;

    return num / denom;
}

float GeometrySchlickGGX(float NdotV, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r * r) / 8.0;

    float num   = NdotV;
    float denom = NdotV * (1.0 - k) + k;

    return num / denom;
}
float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2  = GeometrySchlickGGX(NdotV, roughness);
    float ggx1  = GeometrySchlickGGX(NdotL, roughness);

    return ggx1 * ggx2;
}

vec3 pbr_lighting(vec3 pixel_color, vec3 normal, vec3 light_direction, vec3 pixel_direction, vec3 mrao, vec3 ambiant) {

    vec3 N = normalize(normal);
    vec3 V = pixel_direction;
    //return V;
    float metallic  = mrao.r;
    float roughness = clamp(mrao.g, 0.02, 1.0);
    float ao        = mrao.b;
    vec3 light_color = vec3(10);
    
    vec3 F0 = vec3(0.04);
    F0        = mix(F0, pixel_color, metallic);

    // calculate per-light radiance
    vec3 L           = light_direction;
    vec3 H           = normalize(V + L);
    //float  distance    = length(light_pos - frag_pos);
    //float  attenuation = 1.0 / (distance * distance);
    vec3 radiance    = light_color * 1;//attenuation;

    // cook-torrance brdf
    float  NDF = DistributionGGX(N, H, roughness);
    float  G   = GeometrySmith(N, V, L, roughness);
    vec3 F   = fresnel_schlick(dot(H, V), F0);

    //return F;

    vec3 kS = F;
    vec3 kD = vec3(1) - kS;
    kD *= 1.0 - metallic;

    vec3 numerator   = NDF * G * F;
    float  denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001;
    vec3 specular    = numerator / denominator;

    // add to outgoing radiance Lo
    float NdotL = max(dot(N, L), 0.0);
    vec3 Lo = (kD * pixel_color / PI + specular) * radiance * NdotL;

    vec3 ambient = ambiant;// * pixel_color * ao;
    vec3 color   = ambient + Lo;

    return color;
}


vec3 surface_shading(int in_shading, vec3 albedo, vec3 normal, vec3 mrao, vec3 light_dir, vec3 world_direction) {
    switch (in_shading) {
    case 0:
        return albedo;
    case 1:
        PhongParams phong_params;
        phong_params.ambiant_strength = 0.001;
        phong_params.specular_strength = (mrao.r) * 32;
        phong_params.specular_shininess = (1 - mrao.g) * 16 + 1;
        return phong_lighting(albedo, normal, light_dir, -world_direction, phong_params);
    case 2:
        PhongParams blinn_phong_params;
        blinn_phong_params.ambiant_strength = 0.001;
        blinn_phong_params.specular_strength = (mrao.r) * 32;
        blinn_phong_params.specular_shininess = (1 - mrao.g) * 16 + 1;
        return blinn_phong_lighting(albedo, normal, light_dir, -world_direction, blinn_phong_params);
    case 3:
        vec3 ambient = vec3(mix(0.0, 0.01, clamp(dot(normal, light_dir) + 0.3, 0, 1)));
        return pbr_lighting(albedo, normal, light_dir, -world_direction, mrao, ambient);
    }
}


#endif // LIGHTING_H_