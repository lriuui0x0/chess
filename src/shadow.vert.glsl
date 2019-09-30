#version 450

layout(set = 0, binding = 0) uniform Scene
{
    mat4 view;
    mat4 projection;
} scene;

layout(set = 1, binding = 0) uniform Entity
{
    mat4 world;
} entity;

layout(location = 0) in vec3 pos;

void main()
{
    gl_Position = scene.projection * scene.view * entity.world * vec4(pos, 1);
}
