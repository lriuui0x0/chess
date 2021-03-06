#version 450

layout(location = 0) in vec2 pos;
layout(location = 1) in vec2 texture_coord;

layout(location = 0) out vec2 frag_texture_coord;

void main()
{
    gl_Position = vec4(pos, 0, 1);
    frag_texture_coord = texture_coord;
}
