#version 450

layout(set = 0, binding = 0) uniform Transform {
    mat4 model;
    mat4 view;
    mat4 normal_view;
    mat4 projection;
} transform;

layout(push_constant) uniform Light {
    vec3 pos;
} light;

layout(location = 0) in vec3 pos;
layout(location = 1) in vec3 normal;

layout(location = 0) out vec3 color;

void main() {
    vec3 world_pos = vec3(transform.view * transform.model * vec4(pos, 1)); 
    vec3 light_dir = normalize(light.pos - world_pos);
    vec3 normal_dir = vec3(transform.normal_view * transform.model * vec4(normal, 1));

    vec3 ambient_color = 0.2 * vec3(1);
    float diffuse_coef = max(dot(light_dir, normal_dir), 0);
    vec3 diffuse_color = 0.9 * diffuse_coef * vec3(1);

    color = (ambient_color + diffuse_color) * vec3(1, 0, 0);

    gl_Position = transform.projection * vec4(world_pos.x, world_pos.y, world_pos.z, 1);
}
