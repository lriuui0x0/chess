#version 450

layout(set = 0, binding = 0) uniform Transform {
    mat4 model;
    mat4 view;
    mat4 projection;
} transform;

layout(location = 0) in vec3 pos;

void main() {
    gl_Position = transform.projection * transform.view * transform.model * vec4(pos, 1.0);
}
