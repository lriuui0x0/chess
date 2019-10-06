#version 450

layout(set = 0, binding = 0) uniform Scene
{
    mat4 light_view;
    mat4 light_projection;
    mat4 view;
    mat4 normal_view;
    mat4 projection;
} scene;

layout(location = 0) in vec3 pos;
layout(location = 1) in vec3 color;

layout(location = 0) out vec3 frag_color;

void main()
{
    frag_color = color;
    gl_Position = scene.projection * scene.view * vec4(pos, 1);
}
