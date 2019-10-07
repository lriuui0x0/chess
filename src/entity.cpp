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

struct Piece : Entity
{
    Square square;
};

#define ENTITY_PIECE_COUNT (32)
struct PieceManager
{
    AssetStore *asset_store;
    Piece pieces[ENTITY_PIECE_COUNT];
    Piece *piece_mapping[64];
};

Void set_piece_mesh(PieceManager *piece_manager, Piece *piece, GamePiece game_piece)
{
    ASSERT(piece && !is_empty(game_piece));
    GameSideEnum side = get_side(game_piece);
    GamePieceTypeEnum piece_type = get_piece_type(game_piece);
    piece->mesh = &piece_manager->asset_store->piece_meshes[side][piece_type];
    piece->light_map = &piece_manager->asset_store->piece_light_maps[piece_type];
}

Void fill_piece_initial_state(PieceManager* piece_manager, Piece *piece, GamePiece game_piece, Square square)
{
    ASSERT(piece && !is_empty(game_piece));
    GameSideEnum side = get_side(game_piece);
    piece->square = square;
    piece->pos = get_square_pos(square);
    piece->rot = side == GameSide::white ? get_rotation_quaternion(get_basis_y(), 0) : get_rotation_quaternion(get_basis_y(), PI);
    piece->scale = Vec3{1, -1, 1};
    piece->animation_type = AnimationType::none;
    piece->color_overlay = Vec4{1, 1, 1, 1};
    set_piece_mesh(piece_manager, piece, game_piece);
}

Void fill_piece_manager_initial_state(PieceManager *piece_manager, GameState *game_state)
{
    Int piece_i = 0;
    for (Int square = 0; square < 64; square++)
    {
        GamePiece game_piece = game_state->board[square];
        if (!is_empty(game_piece))
        {
            Piece *piece = &piece_manager->pieces[piece_i++];
            fill_piece_initial_state(piece_manager, piece, game_piece, square);
            piece_manager->piece_mapping[square] = piece;
        }
        else
        {
            piece_manager->piece_mapping[square] = null;
        }
    }

    for (; piece_i < ENTITY_PIECE_COUNT; piece_i++)
    {
        Piece *piece = &piece_manager->pieces[piece_i];
        piece->square = NO_SQUARE;
    }
}

Void add_piece(PieceManager *piece_manager, Square square, Piece *piece)
{
    ASSERT(square >= 0 && square < 64);
    ASSERT(piece);
    ASSERT(!piece_manager->piece_mapping[square]);
    piece_manager->piece_mapping[square] = piece;
    piece->square = square;
    piece->pos = get_square_pos(square);
}

Piece *remove_piece(PieceManager *piece_manager, Square square)
{
    ASSERT(square >= 0 && square < 64);
    Piece *piece = piece_manager->piece_mapping[square];
    ASSERT(piece);
    piece->square = NO_SQUARE;
    piece_manager->piece_mapping[square] = null;
    return piece;
}

Void record_move(PieceManager *piece_manager, GameMove move)
{
    Square capture_square = get_capture_square(move);
    if (capture_square != NO_SQUARE)
    {
        remove_piece(piece_manager, capture_square);
    }

    Square square_from = get_from(move);
    Square square_to = get_to(move);
    Piece *piece = remove_piece(piece_manager, square_from);
    add_piece(piece_manager, square_to, piece);

    GameMoveTypeEnum move_type = get_move_type(move);
    if (move_type == GameMoveType::castling)
    {
        GameMove rook_move = get_castling_rook_move(move);
        Square rook_square_from = get_from(rook_move);
        Square rook_square_to = get_to(rook_move);
        Piece *rook_piece = remove_piece(piece_manager, rook_square_from);
        add_piece(piece_manager, rook_square_to, rook_piece);
    }

    if (move_type == GameMoveType::promotion)
    {
        GameSideEnum side = get_side(move);
        Int promotion_index = get_promotion_index(move);
        GamePiece promoted_piece = get_game_piece(side, promotion_list[promotion_index]);
        Piece *piece = piece_manager->piece_mapping[square_to];
        set_piece_mesh(piece_manager, piece, promoted_piece);
    }
}

Void rollback_move(PieceManager *piece_manager, GameMove move)
{
    Square square_from = get_from(move);
    Square square_to = get_to(move);
    GameMoveTypeEnum move_type = get_move_type(move);
    if (move_type == GameMoveType::promotion)
    {
        GameSideEnum side = get_side(move);
        GamePiece pawn_piece = get_game_piece(side, GamePieceType::pawn);
        Piece *piece = piece_manager->piece_mapping[square_to];
        set_piece_mesh(piece_manager, piece, pawn_piece);
    }

    if (move_type == GameMoveType::castling)
    {
        GameMove rook_move = get_castling_rook_move(move);
        Square rook_square_from = get_from(rook_move);
        Square rook_square_to = get_to(rook_move);
        Piece *rook_piece = remove_piece(piece_manager, rook_square_to);
        add_piece(piece_manager, rook_square_from, rook_piece);
    }

    Piece *piece = remove_piece(piece_manager, square_to);
    add_piece(piece_manager, square_from, piece);

    Square capture_square = get_capture_square(move);
    if (capture_square != NO_SQUARE)
    {
        Piece *captured_piece = null;
        for (Int piece_i = 0; piece_i < ENTITY_PIECE_COUNT; piece_i++)
        {
            captured_piece = &piece_manager->pieces[piece_i];
            if (captured_piece->square == NO_SQUARE)
            {
                break;
            }
        }
        ASSERT(captured_piece);

        GamePiece game_piece = get_captured_piece(move);
        fill_piece_initial_state(piece_manager, captured_piece, game_piece, capture_square);
        add_piece(piece_manager, capture_square, captured_piece);
    }
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

Void start_capture_animation(Piece *piece)
{
    piece->animation_type = AnimationType::capture;
    piece->animation.t = 0;
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
        Real animation_time = 0.1;
        Real dt = elapsed_time / animation_time;
        entity->animation.t += dt;
        Real alpha = lerp(1.0f, 0.0f, entity->animation.t);
        entity->color_overlay = Vec4{1, 1, 1, alpha};

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
