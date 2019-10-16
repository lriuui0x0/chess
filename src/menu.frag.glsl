#version 450

layout(set = 0, binding = 0) uniform sampler2D texture_sampler[2];

layout(location = 0) in vec2 frag_texture_coord;
layout(location = 1) in vec4 frag_color;
layout(location = 2) in flat float frag_font_type;

layout(location = 0) out vec4 color;

void main() {
    if (frag_font_type < 0)
    {
        color = frag_color;
    }
    else
    {
        color = texture(texture_sampler[int(round(frag_font_type))], frag_texture_coord);
        color = vec4(color.rgb, color.a * frag_color.a);
    }
}
