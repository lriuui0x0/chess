#pragma once

#include "../lib/vulkan.hpp"
#include "asset.cpp"

struct DebugUIDrawState
{
    Font *font;
    Int window_width;
    Int window_height;
    VulkanBuffer *vertex_buffer;

    Int character_count;
    Vec2 pos;
    Vec2 line_pos;
    Int indent;
};

struct DebugUIVertex
{
    Vec2 pos;
    Vec2 texture_coord;
};

DebugUIDrawState create_debug_ui_draw_state(Font *font, Int window_width, Int window_height, VulkanBuffer *vertex_buffer, Vec2 initial_pos)
{
    DebugUIDrawState result;
    result.font = font;
    result.window_width = window_width;
    result.window_height = window_height;
    result.vertex_buffer = vertex_buffer;
    result.pos = initial_pos;
    result.line_pos = initial_pos;
    result.indent = 0;
    result.character_count = 0;
    return result;
}

void debug_ui_draw_char(DebugUIDrawState *draw_state, Int8 character)
{
    Font *font = draw_state->font;
    if (character == ' ')
    {
        draw_state->pos.x += (Real)font->whitespace_advance / draw_state->window_width;
        return;
    }

    ASSERT(character >= font->start_char && character < font->start_char + font->num_char);

    Int8 char_index = character - font->start_char;
    FontCharHeader *char_header = &font->pos[char_index];

    Real width = (Real)char_header->width / draw_state->window_width;
    Real height = (Real)font->height / draw_state->window_height;
    Real left_bearing = (Real)char_header->left_bearing / draw_state->window_width;
    Real advance = (Real)char_header->advance / draw_state->window_width;
    Vec2 char_pos = {draw_state->pos.x + left_bearing, draw_state->pos.y};

    Real texture_coord_x_min = (Real)char_header->offset / font->width;
    Real texture_coord_x_max = (Real)(char_header->offset + char_header->width) / font->width;

    DebugUIVertex *debug_ui_vertex = (DebugUIVertex *)draw_state->vertex_buffer->data + draw_state->character_count++ * 6;
    debug_ui_vertex[0].pos = char_pos;
    debug_ui_vertex[0].texture_coord = {texture_coord_x_min, 0};

    debug_ui_vertex[1].pos = {char_pos.x, char_pos.y + height};
    debug_ui_vertex[1].texture_coord = {texture_coord_x_min, 1};

    debug_ui_vertex[2].pos = {char_pos.x + width, char_pos.y};
    debug_ui_vertex[2].texture_coord = {texture_coord_x_max, 0};

    debug_ui_vertex[3].pos = {char_pos.x + width, char_pos.y + height};
    debug_ui_vertex[3].texture_coord = {texture_coord_x_max, 1};

    debug_ui_vertex[4].pos = {char_pos.x + width, char_pos.y};
    debug_ui_vertex[4].texture_coord = {texture_coord_x_max, 0};

    debug_ui_vertex[5].pos = {char_pos.x, char_pos.y + height};
    debug_ui_vertex[5].texture_coord = {texture_coord_x_min, 1};

    draw_state->pos.x += advance;
}

void debug_ui_draw_indent(DebugUIDrawState *draw_state, Int indent)
{
    draw_state->indent += indent;
    Real indent_advance = 2 * indent * ((Real)draw_state->font->whitespace_advance / draw_state->window_width);
    draw_state->pos.x += indent_advance;
    draw_state->line_pos.x += indent_advance;
}

void debug_ui_draw_newline(DebugUIDrawState *draw_state)
{
    Real line_advance = (Real)draw_state->font->line_advance / draw_state->window_height;
    draw_state->line_pos.y += line_advance;
    draw_state->pos = draw_state->line_pos;
}

void debug_ui_draw_str(DebugUIDrawState *draw_state, Str string)
{
    for (Int i = 0; i < string.count; i++)
    {
        debug_ui_draw_char(draw_state, string[i]);
    }
}

void debug_ui_draw_real(DebugUIDrawState *draw_state, Real real)
{
    Bool is_negative = false;
    if (real < 0)
    {
        is_negative = true;
        real = -real;
    }

    Int integer = real;
    Real fraction = real - integer;

    Int8 integer_char_count = 0;
    Int8 integer_chars[16];
    do
    {
        integer_chars[integer_char_count++] = integer % 10 + '0';
        integer /= 10;
    } while (integer);

    Int8 fraction_chars[6];
    for (Int i = 0; i < 6; i++)
    {
        fraction *= 10;
        Int digit = (Int)fraction;
        fraction_chars[i] = digit + '0';
        fraction -= digit;
    }

    if (is_negative)
    {
        debug_ui_draw_char(draw_state, '-');
    }
    for (Int i = integer_char_count - 1; i >= 0; i--)
    {
        debug_ui_draw_char(draw_state, integer_chars[i]);
    }
    debug_ui_draw_char(draw_state, '.');
    for (Int i = 0; i < 6; i++)
    {
        debug_ui_draw_char(draw_state, fraction_chars[i]);
    }
}

void debug_ui_draw_vec3(DebugUIDrawState *draw_state, Vec3 vec3)
{
    debug_ui_draw_real(draw_state, vec3.x);
    debug_ui_draw_str(draw_state, str(", "));
    debug_ui_draw_real(draw_state, vec3.y);
    debug_ui_draw_str(draw_state, str(", "));
    debug_ui_draw_real(draw_state, vec3.z);
}

Bool create_debug_ui_pipeline(VulkanDevice *device, VulkanPipeline *pipeline)
{
    AttachmentInfo color_attachment;
    color_attachment.format = device->swapchain.format;
    color_attachment.multisample_count = VK_SAMPLE_COUNT_1_BIT;
    color_attachment.load_op = VK_ATTACHMENT_LOAD_OP_LOAD;
    color_attachment.store_op = VK_ATTACHMENT_STORE_OP_STORE;
    color_attachment.initial_layout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    color_attachment.working_layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    color_attachment.final_layout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    Buffer<AttachmentInfo> color_attachments;
    color_attachments.count = 1;
    color_attachments.data = &color_attachment;

    if (!create_render_pass(device, &color_attachments, null, null, &pipeline->render_pass))
    {
        return false;
    }

    Str vertex_shader_code;
    if (!read_file("debug_ui.vert.spv", &vertex_shader_code))
    {
        return false;
    }

    Str fragment_shader_code;
    if (!read_file("debug_ui.frag.spv", &fragment_shader_code))
    {
        return false;
    }

    ShaderInfo shader_info[2];
    shader_info[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
    shader_info[0].code = vertex_shader_code;

    shader_info[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    shader_info[1].code = fragment_shader_code;

    Buffer<ShaderInfo> shaders;
    shaders.count = 2;
    shaders.data = shader_info;

    VertexAttributeInfo vertex_attribute_info[2];
    vertex_attribute_info[0].count = sizeof(Vec2);
    vertex_attribute_info[0].offset = offsetof(DebugUIVertex, pos);

    vertex_attribute_info[1].count = sizeof(Vec2);
    vertex_attribute_info[1].offset = offsetof(DebugUIVertex, texture_coord);

    Buffer<VertexAttributeInfo> vertex_attributes;
    vertex_attributes.count = 2;
    vertex_attributes.data = vertex_attribute_info;

    DescriptorBindingInfo descriptor_binding;
    descriptor_binding.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    descriptor_binding.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;

    DescriptorSetInfo descriptor_set_info[1];
    descriptor_set_info[0].bindings.count = 1;
    descriptor_set_info[0].bindings.data = &descriptor_binding;

    Buffer<DescriptorSetInfo> descriptor_sets;
    descriptor_sets.count = 1;
    descriptor_sets.data = descriptor_set_info;

    if (!create_pipeline(device, pipeline->render_pass, 0, &shaders, sizeof(DebugUIVertex), &vertex_attributes, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, &descriptor_sets, VK_SAMPLE_COUNT_1_BIT, false, true, pipeline))
    {
        return false;
    }

    return true;
}

struct DebugUIFrame
{
    VulkanBuffer vertex_buffer;
    Array<VkFramebuffer> frame_buffers;
    VkImage font_texture;
    VkSampler font_texture_sampler;
    VkDescriptorSet font_texture_descriptor_set;
};

Bool create_debug_ui_frame(VulkanDevice *device, VulkanPipeline *pipeline, Font *debug_font,
                           DebugUIFrame *frame, VulkanBuffer *host_vertex_buffer)
{
    VkResult result_code;

    frame->frame_buffers = create_array<VkFramebuffer>(device->swapchain.images.count);
    for (Int image_i = 0; image_i < device->swapchain.images.count; image_i++)
    {
        VkImageView image_view;
        if (!create_image_view(device, device->swapchain.images[image_i], device->swapchain.format, VK_IMAGE_ASPECT_COLOR_BIT, &image_view))
        {
            return false;
        }

        VkImageView attachments[1] = {image_view};
        VkFramebufferCreateInfo frame_buffer_create_info = {};
        frame_buffer_create_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        frame_buffer_create_info.renderPass = pipeline->render_pass;
        frame_buffer_create_info.attachmentCount = 1;
        frame_buffer_create_info.pAttachments = attachments;
        frame_buffer_create_info.width = device->swapchain.width;
        frame_buffer_create_info.height = device->swapchain.height;
        frame_buffer_create_info.layers = 1;

        VkFramebuffer *frame_buffer = frame->frame_buffers.push();
        result_code = vkCreateFramebuffer(device->handle, &frame_buffer_create_info, null, frame_buffer);
        if (result_code != VK_SUCCESS)
        {
            return false;
        }
    }

    Int max_letter_count = 1024;
    Int vertex_buffer_length = sizeof(DebugUIVertex) * max_letter_count;

    if (!create_buffer(device, vertex_buffer_length,
                       VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                       &frame->vertex_buffer))
    {
        return false;
    }

    if (!create_buffer(device, vertex_buffer_length,
                       VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                       host_vertex_buffer))
    {
        return false;
    }

    if (!create_image(device, debug_font->width, debug_font->height,
                      device->swapchain.format, VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &frame->font_texture))
    {
        return false;
    }

    Int image_buffer_length = sizeof(debug_font->data[0]) * debug_font->width * debug_font->height;
    VulkanBuffer host_image_buffer;
    if (!create_buffer(device, image_buffer_length,
                       VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &host_image_buffer))
    {
        return false;
    }

    memcpy(host_image_buffer.data, debug_font->data, image_buffer_length);

    if (!upload_texture(device, &host_image_buffer, frame->font_texture, debug_font->width, debug_font->height))
    {
        return false;
    }

    VkImageView font_texture_view;
    if (!create_image_view(device, frame->font_texture, device->swapchain.format, VK_IMAGE_ASPECT_COLOR_BIT, &font_texture_view))
    {
        return false;
    }

    VkSamplerCreateInfo sampler_create_info = {};
    sampler_create_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    sampler_create_info.magFilter = VK_FILTER_LINEAR;
    sampler_create_info.minFilter = VK_FILTER_LINEAR;
    sampler_create_info.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    sampler_create_info.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    sampler_create_info.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    sampler_create_info.anisotropyEnable = VK_FALSE;
    sampler_create_info.maxAnisotropy = 16;
    sampler_create_info.compareEnable = VK_FALSE;
    sampler_create_info.compareOp = VK_COMPARE_OP_ALWAYS;
    sampler_create_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
    sampler_create_info.mipLodBias = 0.0f;
    sampler_create_info.minLod = 0.0f;
    sampler_create_info.maxLod = 0.0f;
    sampler_create_info.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    sampler_create_info.unnormalizedCoordinates = VK_FALSE;

    result_code = vkCreateSampler(device->handle, &sampler_create_info, null, &frame->font_texture_sampler);
    if (result_code != VK_SUCCESS)
    {
        return false;
    }

    VkDescriptorSetAllocateInfo debug_font_texture_descriptor_set_alloc_info = {};
    debug_font_texture_descriptor_set_alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    debug_font_texture_descriptor_set_alloc_info.descriptorPool = device->descriptor_pool;
    debug_font_texture_descriptor_set_alloc_info.descriptorSetCount = 1;
    debug_font_texture_descriptor_set_alloc_info.pSetLayouts = &pipeline->descriptor_set_layouts[0];

    result_code = vkAllocateDescriptorSets(device->handle, &debug_font_texture_descriptor_set_alloc_info, &frame->font_texture_descriptor_set);
    if (result_code != VK_SUCCESS)
    {
        return false;
    }

    VkDescriptorImageInfo descriptor_image_info;
    descriptor_image_info.sampler = frame->font_texture_sampler;
    descriptor_image_info.imageView = font_texture_view;
    descriptor_image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    VkWriteDescriptorSet descriptor_set_write = {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
    descriptor_set_write.dstSet = frame->font_texture_descriptor_set;
    descriptor_set_write.dstBinding = 0;
    descriptor_set_write.dstArrayElement = 0;
    descriptor_set_write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    descriptor_set_write.descriptorCount = 1;
    descriptor_set_write.pImageInfo = &descriptor_image_info;

    vkUpdateDescriptorSets(device->handle, 1, &descriptor_set_write, 0, null);

    return true;
}
