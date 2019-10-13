#version 450

layout(set = 0, binding = 0) uniform sampler2D texture_sampler;

int blur_radius = 3;
float offsets[3] = {0.0, 1.3846153846, 3.2307692308};
float weights[3] = {0.2270270270, 0.3162162162, 0.0702702703};

layout(location = 0) in vec2 frag_texture_coord;

layout(location = 0) out vec4 color;

layout(push_constant) uniform BlurData {
    uint mode;
    float overlay;
} blur_data;

void main() {
    if (blur_data.mode == 0 || blur_data.mode == 1)
    {
        vec2 uv_scale = 1.0 / textureSize(texture_sampler, 0);
        vec4 sum_color = weights[0] * texture(texture_sampler, frag_texture_coord);

        for (int i = 1; i < blur_radius; i++)
        {
            vec2 uv1;
            vec2 uv2;
            if (blur_data.mode == 0)
            {
                uv1 = frag_texture_coord + vec2(offsets[i], 0) * uv_scale;
                uv2 = frag_texture_coord - vec2(offsets[i], 0) * uv_scale;
            }
            else
            {
                uv1 = frag_texture_coord + vec2(0, offsets[i]) * uv_scale;
                uv2 = frag_texture_coord - vec2(0, offsets[i]) * uv_scale;
            }
            sum_color += weights[i] * texture(texture_sampler, uv1);
            sum_color += weights[i] * texture(texture_sampler, uv2);
        }

        color = sum_color;
    }
    else
    {
        color = blur_data.overlay * texture(texture_sampler, frag_texture_coord);
    }
}
