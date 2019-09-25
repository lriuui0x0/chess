#version 450

layout(location = 0) in flat vec4 frag_color;

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

void main() {
    color = vec4(gamma_inverse(frag_color.rgb), frag_color.a);
}
