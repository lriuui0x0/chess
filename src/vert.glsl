#version 450

layout(set = 0, binding = 0) uniform Scene {
    mat4 view;
    mat4 normal_view;
    mat4 projection;
    vec3 light_dir[4];
} scene;

layout(set = 1, binding = 0) uniform Entity {
    mat4 world;
} entity;

layout(location = 0) in vec3 pos;
layout(location = 1) in vec3 normal;

layout(location = 0) out flat vec3 color;

vec3 shade(vec3 light_dir, vec3 normal_dir)
{
    vec3 ambient_color = 0.1 * vec3(1);
    float diffuse_coef = max(dot(light_dir, normal_dir), 0);
    vec3 diffuse_color = 0.7 * diffuse_coef * vec3(1);
    return (ambient_color + diffuse_color) * vec3(0.330882340669632, 0.330882340669632, 0.330882340669632);
}

void main() {
    vec3 world_pos = vec3(scene.view * entity.world * vec4(pos, 1)); 
    vec3 normal_dir = vec3(scene.normal_view * vec4(normal, 1));
    color = vec3(0);
    color += 0.3 * shade(-vec3(scene.light_dir[0]), normal_dir);
    color += 0.3 * shade(-vec3(scene.light_dir[1]), normal_dir);
    color += 0.2 * shade(-vec3(scene.light_dir[2]), normal_dir);
    color += 0.2 * shade(-vec3(scene.light_dir[3]), normal_dir);
    gl_Position = scene.projection * vec4(world_pos.x, world_pos.y, world_pos.z, 1);
}
