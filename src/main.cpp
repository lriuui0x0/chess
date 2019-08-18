#include "util.cpp"
#include "math.cpp"
#include "window.cpp"
#include "vulkan.cpp"
#include "model.cpp"

struct Transform
{
    Mat4 model;
    Mat4 view;
    Mat4 projection;
};

int WinMain(HINSTANCE instance, HINSTANCE prev_instance, LPSTR command_line, int show_code)
{
    Window window = create_window(wrap_str("Chess"), 800, 600, 50, 50);
    assert(window);

    VulkanContext vulkan_context;
    assert(create_vulkan_context(window, &vulkan_context));

    VulkanSwapchain vulkan_swapchain;
    assert(create_vulkan_swapchain(&vulkan_context, 800, 600, &vulkan_swapchain));

    VulkanPipeline vulkan_pipeline;
    assert(create_vulkan_pipeline(&vulkan_context, &vulkan_swapchain, &vulkan_pipeline));

    Array<VulkanFrame> vulkan_frames;
    assert(create_vulkan_frame(&vulkan_context, 3, &vulkan_frames));

    Str file;
    assert(read_file("../asset/bishop.asset", &file));
    Model bishop_model;
    assert(deserialise_model(file, &bishop_model));
    Int vertices_data_length = sizeof(Vec3) * bishop_model.vertices_count;
    Int indices_data_length = sizeof(UInt4) * bishop_model.indices_count;

    Real min_x = 1000;
    Real min_y = 1000;
    Real min_z = 1000;
    Real max_x = -1000;
    Real max_y = -1000;
    Real max_z = -1000;
    for (Int vertex_index = 0; vertex_index < bishop_model.vertices_count; vertex_index++)
    {
        Vec3 vertex = bishop_model.vertices_data[vertex_index];
        min_x = min(min_x, vertex.x);
        min_y = min(min_y, vertex.y);
        min_z = min(min_z, vertex.z);
        max_x = max(max_x, vertex.x);
        max_y = max(max_y, vertex.y);
        max_z = max(max_z, vertex.z);
    }

    VulkanBuffer host_vertex_buffer;
    assert(create_vulkan_buffer(&vulkan_context, vertices_data_length,
                                VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                                &host_vertex_buffer));
    memcpy(host_vertex_buffer.data, bishop_model.vertices_data, vertices_data_length);

    VulkanBuffer vertex_buffer;
    assert(create_vulkan_buffer(&vulkan_context, vertices_data_length,
                                VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                                &vertex_buffer));

    VulkanBuffer host_index_buffer;
    assert(create_vulkan_buffer(&vulkan_context, indices_data_length,
                                VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                                &host_index_buffer));
    memcpy(host_index_buffer.data, bishop_model.indices_data, indices_data_length);

    VulkanBuffer index_buffer;
    assert(create_vulkan_buffer(&vulkan_context, indices_data_length,
                                VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                                &index_buffer));

    Transform transform;
    transform.model = get_identity_matrix();
    transform.view = get_view_matrix({0, 50, -200}, {0, 50, 0}, {0, 1, 0});
    transform.projection = get_perspective_matrix(degree_to_radian(60), 800.0 / 600.0, 150, 250);

    for (Int vertex_index = 0; vertex_index < bishop_model.vertices_count; vertex_index++)
    {
        Vec3 vertex3 = bishop_model.vertices_data[vertex_index];
        Vec4 vertex = {vertex3.x, vertex3.y, vertex3.z, 1};

        Vec4 view_vertex = transform.view * vertex;
        Vec4 projection_vertex = transform.projection * view_vertex;
        Vec4 final_vertex = perspective_divide(projection_vertex);
        OutputDebugStringA("");
    }


    VulkanBuffer host_uniform_buffer;
    assert(create_vulkan_buffer(&vulkan_context, sizeof(transform),
                                VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                                &host_uniform_buffer));
    memcpy(host_uniform_buffer.data, &transform, sizeof(transform));

    VulkanBuffer uniform_buffer;
    assert(create_vulkan_buffer(&vulkan_context, sizeof(transform),
                                VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                                &uniform_buffer));

    VkDescriptorBufferInfo descriptor_buffer_info = {};
    descriptor_buffer_info.buffer = uniform_buffer.handle;
    descriptor_buffer_info.offset = 0;
    descriptor_buffer_info.range = VK_WHOLE_SIZE;

    VkWriteDescriptorSet descriptor_set_write = {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
    descriptor_set_write.dstSet = vulkan_pipeline.descriptor_set;
    descriptor_set_write.dstBinding = 0;
    descriptor_set_write.dstArrayElement = 0;
    descriptor_set_write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    descriptor_set_write.descriptorCount = 1;
    descriptor_set_write.pBufferInfo = &descriptor_buffer_info;

    vkUpdateDescriptorSets(vulkan_context.device_handle, 1, &descriptor_set_write, 0, NULL);

    show_window(window);

    Int current_angle = 0;
    Bool is_running = true;
    Int current_frame_index = 0;
    while (is_running)
    {
        WindowMessage message;
        while (get_window_message(window, &message))
        {
            if (message.type == WindowMessageType::close)
            {
                is_running = false;
                break;
            }
        }

        transform.model = get_rotation_matrix_z(degree_to_radian(current_angle));
        memcpy(host_uniform_buffer.data, &transform, sizeof(transform));

        assert(render_vulkan_frame(&vulkan_context, &vulkan_swapchain, &vulkan_pipeline, &vulkan_frames[current_frame_index],
                                   &host_vertex_buffer, &vertex_buffer, &host_index_buffer, &index_buffer, &host_uniform_buffer, &uniform_buffer, bishop_model.indices_count));
        current_frame_index = (current_frame_index + 1) % 3;
        current_angle = (current_angle + 1) % 360;
    }

    return 0;
}
