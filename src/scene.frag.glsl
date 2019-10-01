#version 450

layout(set = 2, binding = 0) uniform sampler2D lightmap_sampler;
layout(set = 3, binding = 0) uniform sampler2D shadow_sampler;

layout(location = 0) in flat vec4 frag_color;
layout(location = 1) in vec2 frag_uv;
layout(location = 2) in vec3 shadow_coord;

layout(location = 0) out vec4 color;

vec3 gamma_inverse(vec3 color)
{
    vec3 result;
    for (int i = 0; i < 3; i++)
    {
        if (color[i] <= 0.0031308)
        {
            result[i] = 12.92 * color[i];
        }
        else
        {
            result[i] = 1.055 * pow(color[i], 1 / 2.4) - 0.055;
        }
    }
    return result;
}

vec3 shadow(vec3 color)
{
    vec2 shadow_uv = (shadow_coord.xy + vec2(1)) / 2;
    float closest_depth = texture(shadow_sampler, shadow_uv).r;
    if (shadow_coord.z > closest_depth)
    {
        return vec3(0);
    }
    else
    {
        return color;
    }
}

void main() {
    float light = texture(lightmap_sampler, frag_uv).r;
    color = vec4(gamma_inverse(shadow(light * frag_color.rgb)), frag_color.a);
}
