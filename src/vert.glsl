#version 450

layout(set = 0, binding = 0) uniform Transform {
    mat4 view;
    mat4 normal_view;
    mat4 projection;
} transform;

layout(set = 1, binding = 0) uniform EntityTransform {
    mat4 world;
} entity_transform;

layout(location = 0) in vec3 pos;
layout(location = 1) in vec3 normal;

layout(location = 0) out vec3 world_pos;

vec3 shade(vec3 light_dir, vec3 normal_dir)
{
    vec3 ambient_color = 0.2 * vec3(1);
    float diffuse_coef = max(dot(light_dir, normal_dir), 0);
    vec3 diffuse_color = 0.9 * diffuse_coef * vec3(1);

    return (ambient_color + diffuse_color) * vec3(0.330882340669632, 0.330882340669632, 0.330882340669632);
}

void main() {
    world_pos = vec3(transform.view * entity_transform.world * vec4(pos, 1)); 
    gl_Position = transform.projection * vec4(world_pos.x, world_pos.y, world_pos.z, 1);
}
