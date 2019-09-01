#version 450

layout(location = 0) in vec2 pos;

layout(location = 0) out vec4 shaded_color;

void main()
{
    gl_Position = vec4(pos, 0, 1);
    shaded_color = vec4(1, 0, 0, 1);
}
