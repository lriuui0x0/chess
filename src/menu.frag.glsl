#version 450

layout(set = 0, binding = 0) uniform sampler2D texture_sampler;

layout(location = 0) in vec2 frag_texture_coord;
layout(location = 1) in vec4 frag_color;

layout(location = 0) out vec4 color;

void main() {
    color = texture(texture_sampler, frag_texture_coord);
}
