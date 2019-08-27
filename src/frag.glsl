#version 450

layout(push_constant) uniform Light {
    vec3 dir;
} light;

layout(location = 0) in vec3 world_pos;

vec3 shade(vec3 light_dir, vec3 normal_dir)
{
    vec3 ambient_color = 0.2 * vec3(1);
    float diffuse_coef = max(dot(light_dir, normal_dir), 0);
    vec3 diffuse_color = 0.9 * diffuse_coef * vec3(1);

    return (ambient_color + diffuse_color) * vec3(0.330882340669632, 0.330882340669632, 0.330882340669632);
}

layout(location = 0) out vec4 fragment_color;

void main() {
    vec3 tangent_x = dFdx(world_pos);
    vec3 tangent_y = dFdy(world_pos);
    vec3 normal_dir = normalize(cross(tangent_x, tangent_y));

    fragment_color = vec4(shade(light.dir, normal_dir), 1.0);
}
