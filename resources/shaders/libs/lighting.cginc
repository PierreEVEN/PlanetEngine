#ifndef LIGHTING_H_
#define LIGHTING_H_


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


#endif // LIGHTING_H_