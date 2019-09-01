#include "util.cpp"
#include "math.cpp"
#include "window.cpp"
#include "vulkan.cpp"
#include "asset.cpp"

Bool read_mesh(RawStr filename, OUT Mesh *mesh)
{
    Str file_contents;
    if (!read_file(filename, &file_contents))
    {
        return false;
    }

    if (!deserialise_mesh(file_contents, mesh))
    {
        return false;
    }

    return true;
}

Bool read_font(RawStr filename, OUT Font *font)
{
    Str file_contents;
    if (!read_file(filename, &file_contents))
    {
        return false;
    }

    if (!deserialise_font(file_contents, font))
    {
        return false;
    }

    return true;
}

Void add_entity(Array<Entity> *entities, Str name, Vec3 pos, Mat4 rotation, Mesh *mesh)
{
    Entity *entity = entities->push();
    entity->name = name;
    entity->pos = pos;
    entity->rotation = rotation;
    entity->mesh = mesh;
}

int WinMain(HINSTANCE instance, HINSTANCE prev_instance, LPSTR command_line, int show_code)
{
    Int window_width = 800;
    Int window_height = 600;

    Mesh board_mesh;
    assert(read_mesh("../asset/board.asset", &board_mesh));

    Mesh black_king_mesh;
    assert(read_mesh("../asset/king_black.asset", &black_king_mesh));
    Mesh black_queen_mesh;
    assert(read_mesh("../asset/queen_black.asset", &black_queen_mesh));
    Mesh black_bishop_mesh;
    assert(read_mesh("../asset/bishop_black.asset", &black_bishop_mesh));
    Mesh black_knight_mesh;
    assert(read_mesh("../asset/knight_black.asset", &black_knight_mesh));
    Mesh black_rook_mesh;
    assert(read_mesh("../asset/rook_black.asset", &black_rook_mesh));
    Mesh black_pawn_mesh;
    assert(read_mesh("../asset/pawn_black.asset", &black_pawn_mesh));

    Mesh white_king_mesh;
    assert(read_mesh("../asset/king_white.asset", &white_king_mesh));
    Mesh white_queen_mesh;
    assert(read_mesh("../asset/queen_white.asset", &white_queen_mesh));
    Mesh white_bishop_mesh;
    assert(read_mesh("../asset/bishop_white.asset", &white_bishop_mesh));
    Mesh white_knight_mesh;
    assert(read_mesh("../asset/knight_white.asset", &white_knight_mesh));
    Mesh white_rook_mesh;
    assert(read_mesh("../asset/rook_white.asset", &white_rook_mesh));
    Mesh white_pawn_mesh;
    assert(read_mesh("../asset/pawn_white.asset", &white_pawn_mesh));

    Font debug_font;
    assert(read_font("../asset/debug_font.asset", &debug_font));

    Mat4 black_inital_model = get_identity_matrix();
    Mat4 white_inital_model = get_rotation_matrix_y(PI);

    Array<Entity> entities = create_array<Entity>();
    add_entity(&entities, wrap_str("board"), {0, 0, 0}, black_inital_model, &board_mesh);

    add_entity(&entities, wrap_str("black_rook1"), {0, 0, 0}, black_inital_model, &black_rook_mesh);
    add_entity(&entities, wrap_str("black_knight1"), {-100, 0, 0}, black_inital_model, &black_knight_mesh);
    add_entity(&entities, wrap_str("black_bishop1"), {-200, 0, 0}, black_inital_model, &black_bishop_mesh);
    add_entity(&entities, wrap_str("black_king"), {-300, 0, 0}, black_inital_model, &black_king_mesh);
    add_entity(&entities, wrap_str("black_queen"), {-400, 0, 0}, black_inital_model, &black_queen_mesh);
    add_entity(&entities, wrap_str("black_bishop2"), {-500, 0, 0}, black_inital_model, &black_bishop_mesh);
    add_entity(&entities, wrap_str("black_knight2"), {-600, 0, 0}, black_inital_model, &black_knight_mesh);
    add_entity(&entities, wrap_str("black_rook2"), {-700, 0, 0}, black_inital_model, &black_rook_mesh);
    for (Int i = 0; i < 8; i++)
    {
        UInt8 number_suffix_data[2];
        number_suffix_data[0] = '0' + i;
        number_suffix_data[1] = '\0';
        Str number_suffix;
        number_suffix.length = 1;
        number_suffix.data = number_suffix_data;
        add_entity(&entities, concat_str(wrap_str("black_pawn"), number_suffix), {-i * 100.0f, 0, 100}, get_identity_matrix(), &black_pawn_mesh);
    }

    add_entity(&entities, wrap_str("white_rook1"), {0, 0, 700}, white_inital_model, &white_rook_mesh);
    add_entity(&entities, wrap_str("white_knight1"), {-100, 0, 700}, white_inital_model, &white_knight_mesh);
    add_entity(&entities, wrap_str("white_bishop1"), {-200, 0, 700}, white_inital_model, &white_bishop_mesh);
    add_entity(&entities, wrap_str("white_king"), {-300, 0, 700}, white_inital_model, &white_king_mesh);
    add_entity(&entities, wrap_str("white_queen"), {-400, 0, 700}, white_inital_model, &white_queen_mesh);
    add_entity(&entities, wrap_str("white_bishop2"), {-500, 0, 700}, white_inital_model, &white_bishop_mesh);
    add_entity(&entities, wrap_str("white_knight2"), {-600, 0, 700}, white_inital_model, &white_knight_mesh);
    add_entity(&entities, wrap_str("white_rook2"), {-700, 0, 700}, white_inital_model, &white_rook_mesh);
    for (Int i = 0; i < 8; i++)
    {
        UInt8 number_suffix_data[2];
        number_suffix_data[0] = '0' + i;
        number_suffix_data[1] = '\0';
        Str number_suffix;
        number_suffix.length = 1;
        number_suffix.data = number_suffix_data;
        add_entity(&entities, concat_str(wrap_str("white_pawn"), number_suffix), {-i * 100.0f, 0, 600}, white_inital_model, &white_pawn_mesh);
    }

    Entity camera;
    camera.pos = {-350, 1600, -450};
    camera.rotation = get_rotation_matrix_x(degree_to_radian(65)) * get_rotation_matrix_z(PI);

    Scene scene;
    scene.view = get_view_matrix(camera.pos, vec3(camera.rotation.z), -vec3(camera.rotation.y));
    scene.normal_view = get_normal_view_matrix(camera.pos, vec3(camera.rotation.z), -vec3(camera.rotation.y));
    scene.projection = get_perspective_matrix(degree_to_radian(30), (Real)window_width / (Real)window_height, 10, 10000);
    scene.light_dir[0] = {1, -1, 1};
    scene.light_dir[1] = {1, -1, -1};
    scene.light_dir[2] = {-1, -1, -1};
    scene.light_dir[3] = {-1, -1, 1};

    Window window = create_window(wrap_str("Chess"), window_width, window_height, 50, 50);
    assert(window);

    VulkanContext vulkan_context;
    assert(create_vulkan_context(window, &vulkan_context));

    VulkanSwapchain vulkan_swapchain;
    assert(create_vulkan_swapchain(&vulkan_context, 800, 600, &vulkan_swapchain));

    VulkanScenePipeline vulkan_scene_pipeline;
    assert(create_vulkan_scene_pipeline(&vulkan_context, &vulkan_swapchain, &vulkan_scene_pipeline));

    VulkanDebugUIPipeline vulkan_debug_ui_pipeline;
    assert(create_vulkan_debug_ui_pipeline(&vulkan_context, &vulkan_swapchain, &vulkan_debug_ui_pipeline));

    VulkanSceneFrame vulkan_scene_frame;
    VulkanBuffer scene_vertex_buffer;
    VulkanBuffer scene_index_buffer;
    VulkanBuffer scene_uniform_buffer;
    assert(create_vulkan_scene_frame(&vulkan_context, &vulkan_swapchain, &vulkan_scene_pipeline, &entities, &vulkan_scene_frame, &scene_vertex_buffer, &scene_index_buffer, &scene_uniform_buffer));

    VulkanDebugUIFrame vulkan_debug_ui_frame;
    VulkanBuffer debug_ui_vertex_buffer;
    assert(create_vulkan_debug_ui_frame(&vulkan_context, &vulkan_swapchain, &vulkan_debug_ui_pipeline, &debug_font, &vulkan_debug_ui_frame, &debug_ui_vertex_buffer));

    Int current_vertex_data_offset = 0;
    Int current_index_data_offset = 0;
    Int current_uniform_data_offset = 0;
    memcpy(scene_uniform_buffer.data, &scene, sizeof(scene));
    current_uniform_data_offset += sizeof(scene);
    for (Int i = 0; i < entities.length; i++)
    {
        Mesh *mesh = entities[i].mesh;
        Int current_vertex_data_length = mesh->vertex_count * sizeof(mesh->vertices_data[0]);
        Int current_index_data_length = mesh->index_count * sizeof(mesh->indices_data[0]);
        memcpy(scene_vertex_buffer.data + current_vertex_data_offset, entities[i].mesh->vertices_data, current_vertex_data_length);
        memcpy(scene_index_buffer.data + current_index_data_offset, entities[i].mesh->indices_data, current_index_data_length);

        Piece *piece = (Piece *)(scene_uniform_buffer.data + current_uniform_data_offset);
        piece->world = get_translate_matrix(entities[i].pos.x, entities[i].pos.y, entities[i].pos.z) * entities[i].rotation;
        piece->normal_world = entities[i].rotation;

        current_vertex_data_offset += current_vertex_data_length;
        current_index_data_offset += current_index_data_length;
        current_uniform_data_offset += sizeof(Piece);
    }

    assert(upload_vulkan_buffer(&vulkan_context, &scene_vertex_buffer, &vulkan_scene_frame.vertex_buffer));
    assert(upload_vulkan_buffer(&vulkan_context, &scene_index_buffer, &vulkan_scene_frame.index_buffer));
    assert(upload_vulkan_buffer(&vulkan_context, &scene_uniform_buffer, &vulkan_scene_frame.uniform_buffer));

    Vec2 pos = {-0.8, -0.8};
    Str text_message = wrap_str("Hello!");
    Real height = (Real)debug_font.height / window_height;
    for (Int i = 0; i < text_message.length; i++)
    {
        Int8 char_index = text_message[i] - debug_font.start_char;
        FontCharPos char_pos = debug_font.pos[char_index];
        Real width = (Real)char_pos.width / window_width;

        Real texture_coord_x_min = (Real)char_pos.offset / debug_font.width;
        Real texture_coord_x_max = (Real)(char_pos.offset + char_pos.width) / debug_font.width;

        DebugUIVertex *debug_ui_vertex = (DebugUIVertex *)debug_ui_vertex_buffer.data + i * 6;
        debug_ui_vertex[0].pos = pos;
        debug_ui_vertex[0].texture_coord = {texture_coord_x_min, 0};

        debug_ui_vertex[1].pos = {pos.x, pos.y + height};
        debug_ui_vertex[1].texture_coord = {texture_coord_x_min, 1};

        debug_ui_vertex[2].pos = {pos.x + width, pos.y};
        debug_ui_vertex[2].texture_coord = {texture_coord_x_max, 0};

        debug_ui_vertex[3].pos = {pos.x + width, pos.y + height};
        debug_ui_vertex[3].texture_coord = {texture_coord_x_max, 1};

        debug_ui_vertex[4].pos = {pos.x + width, pos.y};
        debug_ui_vertex[4].texture_coord = {texture_coord_x_max, 0};

        debug_ui_vertex[5].pos = {pos.x, pos.y + height};
        debug_ui_vertex[4].texture_coord = {texture_coord_x_min, 1};

        pos.x += width;
    }

    assert(upload_vulkan_buffer(&vulkan_context, &debug_ui_vertex_buffer, &vulkan_debug_ui_frame.vertex_buffer));

    show_window(window);

    Bool moving_x_pos = false;
    Bool moving_x_neg = false;
    Bool moving_y_pos = false;
    Bool moving_y_neg = false;
    Bool moving_z_pos = false;
    Bool moving_z_neg = false;

    Bool is_mouse_left_dragging = false;
    Bool is_mouse_right_dragging = false;
    Int last_mouse_x;
    Int last_mouse_y;

    Bool is_running = true;
    while (is_running)
    {
        Int mouse_increment_x = 0;
        Int mouse_increment_y = 0;

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
                    moving_x_pos = true;
                }
                else if (key_code == WindowMessageKeyCode::key_a)
                {
                    moving_x_neg = true;
                }
                else if (key_code == WindowMessageKeyCode::key_q)
                {
                    moving_y_pos = true;
                }
                else if (key_code == WindowMessageKeyCode::key_e)
                {
                    moving_y_neg = true;
                }
                else if (key_code == WindowMessageKeyCode::key_w)
                {
                    moving_z_pos = true;
                }
                else if (key_code == WindowMessageKeyCode::key_s)
                {
                    moving_z_neg = true;
                }
            }
            else if (message.type == WindowMessageType::key_up)
            {
                WindowMessageKeyCode key_code = message.key_down_data.key_code;

                if (key_code == WindowMessageKeyCode::key_d)
                {
                    moving_x_pos = false;
                }
                else if (key_code == WindowMessageKeyCode::key_a)
                {
                    moving_x_neg = false;
                }
                else if (key_code == WindowMessageKeyCode::key_q)
                {
                    moving_y_pos = false;
                }
                else if (key_code == WindowMessageKeyCode::key_e)
                {
                    moving_y_neg = false;
                }
                else if (key_code == WindowMessageKeyCode::key_w)
                {
                    moving_z_pos = false;
                }
                else if (key_code == WindowMessageKeyCode::key_s)
                {
                    moving_z_neg = false;
                }
            }
            else if (message.type == WindowMessageType::mouse_down)
            {
                if (message.mouse_down_data.button_type == WindowMessageMouseButtonType::left)
                {
                    if (!is_mouse_right_dragging)
                    {
                        is_mouse_left_dragging = true;
                        last_mouse_x = message.mouse_down_data.x;
                        last_mouse_y = message.mouse_down_data.y;
                    }
                }
                else if (message.mouse_down_data.button_type == WindowMessageMouseButtonType::right)
                {
                    if (!is_mouse_left_dragging)
                    {
                        is_mouse_right_dragging = true;
                        last_mouse_x = message.mouse_down_data.x;
                        last_mouse_y = message.mouse_down_data.y;
                    }
                }
            }
            else if (message.type == WindowMessageType::mouse_up)
            {
                if (message.mouse_down_data.button_type == WindowMessageMouseButtonType::left)
                {
                    is_mouse_left_dragging = false;
                }
                if (message.mouse_down_data.button_type == WindowMessageMouseButtonType::right)
                {
                    is_mouse_right_dragging = false;
                }
            }
            else if (message.type == WindowMessageType::mouse_move)
            {
                mouse_increment_x = message.mouse_move_data.x - last_mouse_x;
                mouse_increment_y = message.mouse_move_data.y - last_mouse_y;
                last_mouse_x = message.mouse_move_data.x;
                last_mouse_y = message.mouse_move_data.y;
            }
        }

        Real speed = 3;
        if (moving_x_pos)
        {
            camera.pos = camera.pos + speed * vec3(camera.rotation.x);
        }
        else if (moving_x_neg)
        {
            camera.pos = camera.pos - speed * vec3(camera.rotation.x);
        }
        else if (moving_y_pos)
        {
            camera.pos = camera.pos + speed * vec3(camera.rotation.y);
        }
        else if (moving_y_neg)
        {
            camera.pos = camera.pos - speed * vec3(camera.rotation.y);
        }
        else if (moving_z_pos)
        {
            camera.pos = camera.pos + speed * vec3(camera.rotation.z);
        }
        else if (moving_z_neg)
        {
            camera.pos = camera.pos - speed * vec3(camera.rotation.z);
        }

        Real rotating_speed = PI / 1800;

        Mat4 local_transform = get_identity_matrix();
        if (is_mouse_left_dragging)
        {
            assert(!is_mouse_right_dragging);
            if (mouse_increment_x != 0)
            {
                local_transform = get_rotation_matrix_y(rotating_speed * mouse_increment_x) * local_transform;
            }
            if (mouse_increment_y != 0)
            {
                local_transform = get_rotation_matrix_x(rotating_speed * -mouse_increment_y) * local_transform;
            }
        }

        if (is_mouse_right_dragging)
        {
            assert(!is_mouse_left_dragging);
        }

        camera.rotation = camera.rotation * local_transform;

        scene.view = get_view_matrix(camera.pos, vec3(camera.rotation.z), -vec3(camera.rotation.y));
        scene.normal_view = get_normal_view_matrix(camera.pos, vec3(camera.rotation.z), -vec3(camera.rotation.y));
        memcpy(scene_uniform_buffer.data, &scene, sizeof(scene));

        assert(render_vulkan_frame(&vulkan_context, &vulkan_swapchain,
                                   &vulkan_scene_pipeline, &vulkan_scene_frame, &entities, &scene_uniform_buffer,
                                   &vulkan_debug_ui_pipeline, &vulkan_debug_ui_frame, text_message.length));
    }

    return 0;
}
