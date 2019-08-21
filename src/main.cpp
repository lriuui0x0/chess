#include "util.cpp"
#include "math.cpp"
#include "window.cpp"
#include "vulkan.cpp"
#include "model.cpp"

struct Transform
{
    Mat4 model;
    Mat4 view;
    Mat4 normal_view;
    Mat4 projection;
};

int WinMain(HINSTANCE instance, HINSTANCE prev_instance, LPSTR command_line, int show_code)
{
    Str file;
    assert(read_file("../asset/bishop.asset", &file));
    Model bishop_model;
    assert(deserialise_model(file, &bishop_model));
    Int vertices_data_length = sizeof(Vertex) * bishop_model.vertices_count;
    Int indices_data_length = sizeof(UInt4) * bishop_model.indices_count;

    Real min_x = 1000;
    Real min_y = 1000;
    Real min_z = 1000;
    Real max_x = -1000;
    Real max_y = -1000;
    Real max_z = -1000;
    for (Int vertex_index = 0; vertex_index < bishop_model.vertices_count; vertex_index++)
    {
        Vec3 vertex = bishop_model.vertices_data[vertex_index].pos;
        min_x = min(min_x, vertex.x);
        min_y = min(min_y, vertex.y);
        min_z = min(min_z, vertex.z);
        max_x = max(max_x, vertex.x);
        max_y = max(max_y, vertex.y);
        max_z = max(max_z, vertex.z);
    }

    Vec3 light_pos = {0, 0, -100};
    Vec3 camera_pos = {0, 100, -300};
    Vec3 camera_dir = {0, 0, 1};

    Transform transform;
    transform.model = get_identity_matrix();
    transform.view = get_view_matrix(camera_pos, camera_dir, {0, -1, 0});
    transform.normal_view = get_normal_view_matrix(camera_pos, camera_dir, {0, -1, 0});
    transform.projection = get_perspective_matrix(degree_to_radian(60), 800.0 / 600.0, 50, 500);

    for (Int vertex_index = 0; vertex_index < bishop_model.vertices_count; vertex_index++)
    {
        Vec3 vertex = bishop_model.vertices_data[vertex_index].pos;
        Vec3 normal = bishop_model.vertices_data[vertex_index].normal;

        Vec3 world_pos = vec3(transform.view * (transform.model * vec4(vertex, 1)));
        Vec3 light_dir = normalize(light_pos - world_pos);
        Vec3 normal_dir = normalize(vec3(transform.normal_view * (transform.model * vec4(normal, 1))));

        Real diffuse_coef = dot(light_dir, normal_dir);

        OutputDebugStringA("");
    }

    Window window = create_window(wrap_str("Chess"), 800, 600, 50, 50);
    assert(window);

    VulkanContext vulkan_context;
    assert(create_vulkan_context(window, &vulkan_context));

    VulkanSwapchain vulkan_swapchain;
    assert(create_vulkan_swapchain(&vulkan_context, 800, 600, &vulkan_swapchain));

    VulkanPipeline vulkan_pipeline;
    assert(create_vulkan_pipeline(&vulkan_context, &vulkan_swapchain, &vulkan_pipeline));

    VulkanFrame vulkan_frame;
    assert(create_vulkan_frame(&vulkan_context, &vulkan_swapchain, &vulkan_pipeline, vertices_data_length, indices_data_length, sizeof(transform), &vulkan_frame));

    VulkanBuffer host_vertex_buffer;
    assert(create_vulkan_buffer(&vulkan_context, vertices_data_length,
                                VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                                &host_vertex_buffer));
    memcpy(host_vertex_buffer.data, bishop_model.vertices_data, vertices_data_length);

    VulkanBuffer host_index_buffer;
    assert(create_vulkan_buffer(&vulkan_context, indices_data_length,
                                VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                                &host_index_buffer));
    memcpy(host_index_buffer.data, bishop_model.indices_data, indices_data_length);

    VulkanBuffer host_uniform_buffer;
    assert(create_vulkan_buffer(&vulkan_context, sizeof(transform),
                                VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                                &host_uniform_buffer));
    memcpy(host_uniform_buffer.data, &transform, sizeof(transform));

    show_window(window);

    Int current_angle = 0;

    Bool camera_moving_x_pos = false;
    Bool camera_moving_x_neg = false;
    Bool camera_moving_y_pos = false;
    Bool camera_moving_y_neg = false;
    Bool camera_moving_z_pos = false;
    Bool camera_moving_z_neg = false;

    Bool camera_rotating_x_pos = false;
    Bool camera_rotating_x_neg = false;
    Bool camera_rotating_y_pos = false;
    Bool camera_rotating_y_neg = false;
    Bool camera_rotating_z_pos = false;
    Bool camera_rotating_z_neg = false;

    Bool is_running = true;
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
            else if (message.type == WindowMessageType::key_down)
            {
                WindowMessageKeyCode key_code = message.key_down_data.key_code;

                if (key_code == WindowMessageKeyCode::key_d)
                {
                    camera_moving_x_pos = true;
                }
                else if (key_code == WindowMessageKeyCode::key_a)
                {
                    camera_moving_x_neg = true;
                }
                else if (key_code == WindowMessageKeyCode::key_x)
                {
                    camera_moving_y_pos = true;
                }
                else if (key_code == WindowMessageKeyCode::key_2)
                {
                    camera_moving_y_neg = true;
                }
                else if (key_code == WindowMessageKeyCode::key_w)
                {
                    camera_moving_z_pos = true;
                }
                else if (key_code == WindowMessageKeyCode::key_s)
                {
                    camera_moving_z_neg = true;
                }

                if (key_code == WindowMessageKeyCode::key_u)
                {
                    camera_rotating_x_pos = true;
                }
                else if (key_code == WindowMessageKeyCode::key_j)
                {
                    camera_rotating_x_neg = true;
                }
                else if (key_code == WindowMessageKeyCode::key_k)
                {
                    camera_rotating_y_pos = true;
                }
                else if (key_code == WindowMessageKeyCode::key_h)
                {
                    camera_rotating_y_neg = true;
                }
                else if (key_code == WindowMessageKeyCode::key_l)
                {
                    camera_rotating_z_pos = true;
                }
                else if (key_code == WindowMessageKeyCode::key_g)
                {
                    camera_rotating_z_neg = true;
                }
            }
            else if (message.type == WindowMessageType::key_up)
            {
                WindowMessageKeyCode key_code = message.key_down_data.key_code;

                if (key_code == WindowMessageKeyCode::key_d)
                {
                    camera_moving_x_pos = false;
                }
                else if (key_code == WindowMessageKeyCode::key_a)
                {
                    camera_moving_x_neg = false;
                }
                else if (key_code == WindowMessageKeyCode::key_x)
                {
                    camera_moving_y_pos = false;
                }
                else if (key_code == WindowMessageKeyCode::key_2)
                {
                    camera_moving_y_neg = false;
                }
                else if (key_code == WindowMessageKeyCode::key_w)
                {
                    camera_moving_z_pos = false;
                }
                else if (key_code == WindowMessageKeyCode::key_s)
                {
                    camera_moving_z_neg = false;
                }

                if (key_code == WindowMessageKeyCode::key_u)
                {
                    camera_rotating_x_pos = false;
                }
                else if (key_code == WindowMessageKeyCode::key_j)
                {
                    camera_rotating_x_neg = false;
                }
                else if (key_code == WindowMessageKeyCode::key_k)
                {
                    camera_rotating_y_pos = false;
                }
                else if (key_code == WindowMessageKeyCode::key_h)
                {
                    camera_rotating_y_neg = false;
                }
                else if (key_code == WindowMessageKeyCode::key_l)
                {
                    camera_rotating_z_pos = false;
                }
                else if (key_code == WindowMessageKeyCode::key_g)
                {
                    camera_rotating_z_neg = false;
                }
            }
        }

        Real speed = 3;
        if (camera_moving_x_pos)
        {
            camera_pos.x += speed;
        }
        else if (camera_moving_x_neg)
        {
            camera_pos.x -= speed;
        }
        else if (camera_moving_y_pos)
        {
            camera_pos.y += speed;
        }
        else if (camera_moving_y_neg)
        {
            camera_pos.y -= speed;
        }
        else if (camera_moving_z_pos)
        {
            camera_pos.z += speed;
        }
        else if (camera_moving_z_neg)
        {
            camera_pos.z -= speed;
        }

        Real rotating_speed = PI / 180;
        if (camera_rotating_x_pos)
        {
            camera_dir = vec3(get_rotation_matrix_x(rotating_speed) * vec4(camera_dir));
        }
        else if (camera_rotating_x_neg)
        {
            camera_dir = vec3(get_rotation_matrix_x(-rotating_speed) * vec4(camera_dir));
        }
        else if (camera_rotating_y_pos)
        {
            camera_dir = vec3(get_rotation_matrix_y(rotating_speed) * vec4(camera_dir));
        }
        else if (camera_rotating_y_neg)
        {
            camera_dir = vec3(get_rotation_matrix_y(-rotating_speed) * vec4(camera_dir));
        }
        else if (camera_rotating_z_pos)
        {
            camera_dir = vec3(get_rotation_matrix_z(rotating_speed) * vec4(camera_dir));
        }
        else if (camera_rotating_z_neg)
        {
            camera_dir = vec3(get_rotation_matrix_z(-rotating_speed) * vec4(camera_dir));
        }

        transform.model = get_rotation_matrix_y(degree_to_radian(current_angle));
        transform.view = get_view_matrix(camera_pos, camera_dir, {0, 1, 0});
        transform.normal_view = get_normal_view_matrix(camera_pos, camera_dir, {0, 1, 0});
        memcpy(host_uniform_buffer.data, &transform, sizeof(transform));

        assert(render_vulkan_frame(&vulkan_context, &vulkan_swapchain, &vulkan_pipeline, &vulkan_frame,
                                   &host_vertex_buffer, &host_index_buffer, &host_uniform_buffer,
                                   bishop_model.indices_count, light_pos));
        current_angle = (current_angle + 1) % 360;
    }

    return 0;
}
