#pragma once

#include "../lib/util.hpp"
#include "math.cpp"
#include "asset.cpp"
#include "game.cpp"
#include "collision.cpp"

struct Camera
{
    Vec3 pos;
    Quaternion rot;
};

Camera get_scene_camera()
{
    Camera camera;
    camera.pos = {350, -1750, -330};
    camera.rot = get_rotation_quaternion(get_basis_x(), -degree_to_radian(70));
    return camera;
}

struct Animation
{
    Real t;
    Vec3 pos_from;
    Vec3 pos_to;
    Quaternion rot_from;
    Quaternion rot_to;
};

enum struct AnimationType
{
    none,
    move,
    jump,
    capture,
    flash,
    illegal_flash,
};

struct Entity
{
    Vec3 pos;
    Quaternion rot;
    Vec3 scale;
    Vec4 color_overlay;
    Mesh *mesh;
    Image *light_map;
    AnimationType animation_type;
    Animation animation;
};

struct Board : Entity
{
    CollisionBox collision_box[64];
};

#define SQUARE_SIZE (100.0f)

Void fill_board_initial_state(Board *board, Mesh *board_mesh, Image *light_map)
{
    board->pos = {0, 0, 0};
    board->rot = get_rotation_quaternion(get_basis_y(), 0);
    board->scale = {-1, -1, 1};
    board->mesh = board_mesh;
    board->light_map = light_map;
    board->color_overlay = Vec4{1, 1, 1, 1};
    board->animation_type = AnimationType::none;

    for (Int square_i = 0; square_i < 64; square_i++)
    {
        Int row = get_row(square_i);
        Int column = get_column(square_i);

        CollisionBox *collision_box = &board->collision_box[square_i];
        collision_box->center = {column * SQUARE_SIZE, 0, row * SQUARE_SIZE};
        collision_box->radius = {SQUARE_SIZE / 2, 0, SQUARE_SIZE / 2};
    }
}

Vec3 get_square_pos(Int square)
{
    Int row = get_row(square);
    Int column = get_column(square);
    return {column * SQUARE_SIZE, 0, row * SQUARE_SIZE};
}

#define ENTITY_PIECE_COUNT (32)

struct Piece : Entity
{
    Int square;
};

Void fill_piece_initial_state(GamePiece game_piece, Piece *piece, Int square, AssetStore *asset_store)
{
    ASSERT(game_piece != NO_GAME_PIECE);
    GameSideEnum side = get_side(game_piece);
    GamePieceTypeEnum piece_type = get_piece_type(game_piece);

    piece->pos = get_square_pos(square);
    piece->rot = side == GameSide::white ? get_rotation_quaternion(get_basis_y(), 0) : get_rotation_quaternion(get_basis_y(), PI);
    piece->scale = Vec3{1, -1, 1};
    piece->square = square;
    piece->mesh = &asset_store->piece_meshes[side][piece_type];
    piece->light_map = &asset_store->piece_light_maps[piece_type];
    piece->animation_type = AnimationType::none;
    piece->color_overlay = Vec4{1, 1, 1, 1};
}

Void update_piece_mesh(Piece *piece, GamePiece game_piece, AssetStore *asset_store)
{
    GameSideEnum side = get_side(game_piece);
    GamePieceTypeEnum piece_type = get_piece_type(game_piece);
    piece->mesh = &asset_store->piece_meshes[side][piece_type];
    piece->light_map = &asset_store->piece_light_maps[piece_type];
}

struct GhostPiece : Entity
{
    Piece *shadowed_piece;
};

#define GHOST_PIECE_ALPHA (0.7)
Void fill_ghost_piece_initila_state(GhostPiece *ghost_piece)
{
    ghost_piece->animation_type = AnimationType::none;
    ghost_piece->color_overlay = Vec4{1, 1, 1, GHOST_PIECE_ALPHA};
}

Void update_ghost_piece(GhostPiece *ghost_piece, Piece *piece, Int square)
{
    ghost_piece->pos = get_square_pos(square);
    ghost_piece->rot = piece->rot;
    ghost_piece->scale = piece->scale;
    ghost_piece->mesh = piece->mesh;
    ghost_piece->light_map = piece->light_map;
}

Void start_move_animation(Piece *piece, Int square)
{
    piece->animation_type = AnimationType::move;
    piece->animation.t = 0;
    piece->animation.pos_from = piece->pos;
    piece->animation.pos_to = get_square_pos(square);
}

Void start_jump_animation(Piece *piece, Int square)
{
    piece->animation_type = AnimationType::jump;
    piece->animation.t = 0;
    piece->animation.pos_from = piece->pos;
    piece->animation.pos_to = get_square_pos(square);
}

Void start_capture_animation(Piece *piece, RandomGenerator *generator)
{
    piece->animation_type = AnimationType::capture;
    piece->animation.t = 0;

    Real distance = 1500;
    Real angle = degree_to_radian(get_random_number(generator, 0, 360));
    Real height = -500;

    Vec3 pos_change_xz = Vec3{distance * cosf(angle), 0, distance * sinf(angle)};
    Vec3 pos_to = piece->pos + Vec3{pos_change_xz.x, height, pos_change_xz.z};
    piece->animation.pos_from = piece->pos;
    piece->animation.pos_to = pos_to;

    Quaternion rot_to = get_neighbour(get_rotation_quaternion(-get_basis_y(), (angle + PI) - PI / 2) * get_rotation_quaternion(get_basis_x(), PI / 2), piece->rot);
    piece->animation.rot_from = piece->rot;
    piece->animation.rot_to = rot_to;
}

Void start_flash_animation(Piece *piece)
{
    piece->animation_type = AnimationType::flash;
    piece->animation.t = 0;
}

Void stop_flash_animation(Piece *piece)
{
    piece->animation_type = AnimationType::none;
    piece->color_overlay = Vec4{1, 1, 1, 1};
}

Void start_illegal_flash_animation(GhostPiece *ghost_piece)
{
    ghost_piece->animation_type = AnimationType::illegal_flash;
    ghost_piece->animation.t = 0;
}

Void stop_illegal_flash_animation(GhostPiece *ghost_piece)
{
    ghost_piece->animation_type = AnimationType::none;
    ghost_piece->color_overlay = Vec4{1, 1, 1, GHOST_PIECE_ALPHA};
}

Real quadratic_modify(Real t, Real a)
{
    return a * square(t) + (1 - a) * t;
}

Void update_animation(Entity *entity, Real elapsed_time)
{
    if (entity->animation_type == AnimationType::move)
    {
        Real animation_time = 0.2;
        Real dt = elapsed_time / animation_time;
        entity->animation.t = MIN(entity->animation.t + dt, 1);
        Real ft = quadratic_modify(entity->animation.t, 0.1);
        entity->pos = lerp(entity->animation.pos_from, entity->animation.pos_to, ft);

        if (entity->animation.t >= 1)
        {
            entity->animation_type = AnimationType::none;
        }
    }
    else if (entity->animation_type == AnimationType::jump)
    {
        Real animation_time = 0.2;
        Real dt = elapsed_time / animation_time;
        entity->animation.t = MIN(entity->animation.t + dt, 1);
        Vec3 new_pos = lerp(entity->animation.pos_from, entity->animation.pos_to, entity->animation.t);
        Real height = -200;
        Real a = -4 * height;
        Real b = 4 * height;
        new_pos.y = a * square(entity->animation.t) + b * entity->animation.t;
        entity->pos = new_pos;

        if (entity->animation.t >= 1)
        {
            entity->animation_type = AnimationType::none;
        }
    }
    else if (entity->animation_type == AnimationType::capture)
    {
        Real animation_time = 0.5;
        Real dt = elapsed_time / animation_time;
        entity->animation.t = MIN(entity->animation.t + dt, 1);
        entity->pos = lerp(entity->animation.pos_from, entity->animation.pos_to, entity->animation.t);
        Real rot_t = quadratic_modify(entity->animation.t, -1.5);
        entity->rot = slerp(entity->animation.rot_from, entity->animation.rot_to, rot_t);

        if (entity->animation.t >= 1)
        {
            entity->animation_type = AnimationType::none;
        }
    }
    else if (entity->animation_type == AnimationType::flash)
    {
        Real animation_time = 1.2;
        Real dt = elapsed_time / animation_time;
        entity->animation.t += dt;
        if (entity->animation.t >= 1)
        {
            entity->animation.t -= 1;
        }

        Real alpha_min = 0.5;
        Real alpha_max = 1;
        Real alpha;
        if (entity->animation.t < 0.5)
        {
            alpha = lerp(alpha_max, alpha_min, entity->animation.t * 2);
        }
        else
        {
            alpha = lerp(alpha_min, alpha_max, (entity->animation.t - 0.5) * 2);
        }
        entity->color_overlay = Vec4{1, 1, 1, alpha};
    }
    else if (entity->animation_type == AnimationType::illegal_flash)
    {
        Real animation_time = 0.4;
        Real dt = elapsed_time / animation_time;
        entity->animation.t = MIN(entity->animation.t + dt, 1);

        Real alpha_start = GHOST_PIECE_ALPHA;
        Real alpha_end = 1.0;
        Real alpha;
        Real green_blue_start = 1.0;
        Real green_blue_end = 0.2;
        Real green_blue;
        if (entity->animation.t < 0.25)
        {
            alpha = lerp(alpha_start, alpha_end, entity->animation.t * 4);
            green_blue = lerp(green_blue_start, green_blue_end, entity->animation.t);
        }
        else if (entity->animation.t >= 0.25 && entity->animation.t < 0.5)
        {
            alpha = lerp(alpha_end, alpha_start, (entity->animation.t - 0.25) * 4);
            green_blue = lerp(green_blue_end, green_blue_start, (entity->animation.t - 0.25) * 4);
        }
        else if (entity->animation.t >= 0.5 && entity->animation.t < 0.75)
        {
            alpha = lerp(alpha_start, alpha_end, (entity->animation.t - 0.5) * 4);
            green_blue = lerp(green_blue_start, green_blue_end, (entity->animation.t - 0.5) * 4);
        }
        else
        {
            alpha = lerp(alpha_end, alpha_start, (entity->animation.t - 0.75) * 4);
            green_blue = lerp(green_blue_end, green_blue_start, (entity->animation.t - 0.75) * 4);
        }
        entity->color_overlay = Vec4{1, green_blue, green_blue, alpha};

        if (entity->animation.t >= 1)
        {
            entity->animation_type = AnimationType::none;
        }
    }
}
