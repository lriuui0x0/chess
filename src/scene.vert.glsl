#version 450

struct HemiLight
{
    vec4 dir;
    vec4 color;
    vec4 opp_color;
};

layout(set = 0, binding = 0) uniform Scene
{
    mat4 view;
    mat4 normal_view;
    mat4 projection;
    HemiLight hemi_light;
    vec4 dir_light;
} scene;

layout(set = 1, binding = 0) uniform Entity
{
    mat4 world;
    mat4 normal_world;
    vec4 color_overlay;
} entity;

layout(location = 0) in vec3 pos;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec3 color;
layout(location = 3) in vec2 uv;

layout(location = 0) out flat vec4 frag_color;
layout(location = 1) out vec2 frag_uv;

vec3 phong_shade(vec3 light_dir, vec3 normal_dir)
{
    vec3 ambient_color = 0.1 * vec3(1);
    float diffuse_coef = max(dot(light_dir, normal_dir), 0);
    vec3 diffuse_color = 0.7 * diffuse_coef * vec3(1);
    return ambient_color + diffuse_color;
}

vec3 hemi_shade(HemiLight light, vec3 normal_dir)
{
    float w = 0.5 * (1 + dot(vec3(light.dir), normal_dir));
    vec3 light_color =  w * vec3(light.color) + (1 - w) * vec3(light.opp_color);
    return light_color;
}

void main()
{
    vec3 normal_dir = vec3(entity.normal_world * vec4(normal, 1));
    vec3 hemi_part = hemi_shade(scene.hemi_light, normal_dir);
    vec3 phong_part = phong_shade(vec3(-scene.dir_light), normal_dir);
    frag_color = vec4(0.1 * hemi_part + 0.9 * phong_part, 1) * vec4(color, 1) * entity.color_overlay;
    frag_uv = uv;
    gl_Position = scene.projection * scene.view * entity.world * vec4(pos, 1);
}
