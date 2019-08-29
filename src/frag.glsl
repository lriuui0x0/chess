#version 450

layout(location = 0) in flat vec3 shaded_color;

layout(location = 0) out vec4 fragment_color;

void main() {
    fragment_color = vec4(shaded_color, 1.0);
}
