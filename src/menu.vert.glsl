#version 450

layout(location = 0) in vec2 pos;
layout(location = 1) in vec2 texture_coord;
layout(location = 2) in vec4 color;
layout(location = 3) in float font_type;

layout(location = 0) out vec2 frag_texture_coord;
layout(location = 1) out vec4 frag_color;
layout(location = 2) out flat float frag_font_type;

void main()
{
    gl_Position = vec4(pos, 0, 1);
    frag_texture_coord = texture_coord;
    frag_color = color;
    frag_font_type = font_type;
}
