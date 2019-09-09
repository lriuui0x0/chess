#version 450

layout(set = 0, binding = 0) uniform Scene {
    mat4 view;
    mat4 projection;
} scene;

layout(set = 1, binding = 0) uniform Entity {
    mat4 world;
    mat4 normal_world;
    vec4 color;
} entity;

layout(location = 0) in vec3 pos;
layout(location = 1) in vec3 color;

layout(location = 0) out vec4 frag_color;

void main()
{
    frag_color = vec4(color, 1.0);
    gl_Position = scene.projection * scene.view * entity.world * vec4(pos, 1);
}
