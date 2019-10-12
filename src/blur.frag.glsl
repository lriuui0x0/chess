#version 450

layout(set = 0, binding = 0) uniform sampler2D texture_sampler;

int blur_radius = 15;
float weights[15] = {0.132368, 0.125279, 0.106209, 0.080656, 0.054865, 0.033431, 0.018246, 0.00892, 0.003906, 0.001532, 0.000538, 0.000169, 0.000048, 0.000012, 0.000003};
// float weights[5] = {0.227027, 0.1945946, 0.1216216, 0.054054, 0.016216};

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
                uv1 = frag_texture_coord + vec2(i, 0) * uv_scale;
                uv2 = frag_texture_coord - vec2(i, 0) * uv_scale;
            }
            else
            {
                uv1 = frag_texture_coord + vec2(0, i) * uv_scale;
                uv2 = frag_texture_coord - vec2(0, i) * uv_scale;
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
