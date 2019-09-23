#pragma once

#include "../lib/util.hpp"
#include "math.cpp"
#include "asset.cpp"
#include "game.cpp"
#include "collision.cpp"

struct Camera
{
    Vec3 pos;
    Quaternion rotation;
};

struct Entity
{
    Vec3 pos;
    Quaternion rotation;
    Vec3 scale;
    Real alpha;
    Mesh *mesh;
};

struct Board : Entity
{
    CollisionBox collision_box[BOARD_SQUARE_COUNT];
};

Void fill_board_initial_state(Board *board, Mesh *board_mesh)
{
    board->pos = {0, 0, 0};
    board->rotation = get_rotation_quaternion(get_basis_y(), 0);
    board->scale = {-1, -1, 1};
    board->mesh = board_mesh;
    board->alpha = 1;
}

Vec3 get_square_pos(Int row, Int column)
{
    return {(Real)(column * 100.0), 0, (Real)(row * 100.0)};
}

struct Animation
{
    Real t;
    Vec3 pos_from;
    Vec3 pos_to;
};

enum struct AnimationType
{
    stand,
    move,
    jump,
    flash,
};

struct Piece : Entity
{
    GamePiece *game_piece;
    CollisionBox collision_box;
    AnimationType animation_type;
    Animation animation;
};

Void fill_piece_initial_state(GamePiece *game_piece, Piece *piece,
                              Mesh *white_rook_mesh,
                              Mesh *white_knight_mesh,
                              Mesh *white_bishop_mesh,
                              Mesh *white_queen_mesh,
                              Mesh *white_king_mesh,
                              Mesh *white_pawn_mesh,
                              Mesh *black_rook_mesh,
                              Mesh *black_knight_mesh,
                              Mesh *black_bishop_mesh,
                              Mesh *black_queen_mesh,
                              Mesh *black_king_mesh,
                              Mesh *black_pawn_mesh)
{
    piece->game_piece = game_piece;
    if (game_piece->side == GameSide::white)
    {
        piece->rotation = get_rotation_quaternion(get_basis_y(), 0);
        piece->scale = Vec3{1, -1, 1};

        switch (game_piece->type)
        {
        case GamePieceType::rook:
        {
            piece->mesh = white_rook_mesh;
            if (game_piece->type_index == 0)
            {
                piece->pos = get_square_pos(0, 0);
            }
            else if (game_piece->type_index == 1)
            {
                piece->pos = get_square_pos(0, 7);
            }
        }
        break;

        case GamePieceType::knight:
        {
            piece->mesh = white_knight_mesh;
            if (game_piece->type_index == 0)
            {
                piece->pos = get_square_pos(0, 1);
            }
            else if (game_piece->type_index == 1)
            {
                piece->pos = get_square_pos(0, 6);
            }
        }
        break;

        case GamePieceType::bishop:
        {
            piece->mesh = white_bishop_mesh;
            if (game_piece->type_index == 0)
            {
                piece->pos = get_square_pos(0, 2);
            }
            else if (game_piece->type_index == 1)
            {
                piece->pos = get_square_pos(0, 5);
            }
        }
        break;

        case GamePieceType::queen:
        {
            piece->mesh = white_queen_mesh;
            piece->pos = get_square_pos(0, 3);
        }
        break;

        case GamePieceType::king:
        {
            piece->mesh = white_king_mesh;
            piece->pos = get_square_pos(0, 4);
        }
        break;

        case GamePieceType::pawn:
        {
            piece->mesh = white_pawn_mesh;
            piece->pos = get_square_pos(1, game_piece->type_index);
        }
        break;
        }
    }
    else if (game_piece->side == GameSide::black)
    {
        piece->rotation = get_rotation_quaternion(get_basis_y(), PI);
        piece->scale = {1, -1, 1};

        switch (game_piece->type)
        {
        case GamePieceType::rook:
        {
            piece->mesh = black_rook_mesh;
            if (game_piece->type_index == 0)
            {
                piece->pos = get_square_pos(7, 0);
            }
            else if (game_piece->type_index == 1)
            {
                piece->pos = get_square_pos(7, 7);
            }
        }
        break;

        case GamePieceType::knight:
        {
            piece->mesh = black_knight_mesh;
            if (game_piece->type_index == 0)
            {
                piece->pos = get_square_pos(7, 1);
            }
            else if (game_piece->type_index == 1)
            {
                piece->pos = get_square_pos(7, 6);
            }
        }
        break;

        case GamePieceType::bishop:
        {
            piece->mesh = black_bishop_mesh;
            if (game_piece->type_index == 0)
            {
                piece->pos = get_square_pos(7, 2);
            }
            else if (game_piece->type_index == 1)
            {
                piece->pos = get_square_pos(7, 5);
            }
        }
        break;

        case GamePieceType::queen:
        {
            piece->mesh = black_queen_mesh;
            piece->pos = get_square_pos(7, 3);
        }
        break;

        case GamePieceType::king:
        {
            piece->mesh = black_king_mesh;
            piece->pos = get_square_pos(7, 4);
        }
        break;

        case GamePieceType::pawn:
        {
            piece->mesh = black_pawn_mesh;
            piece->pos = get_square_pos(6, game_piece->type_index);
        }
        break;
        }
    }

    Vec3 min = {+10000, +10000, +10000};
    Vec3 max = {-10000, -10000, -10000};
    for (Int vertex_i = 0; vertex_i < piece->mesh->vertex_count; vertex_i++)
    {
        Vec3 pos = piece->mesh->vertices_data[vertex_i].pos;
        for (Int i = 0; i < 3; i++)
        {
            min[i] = MIN(min[i], pos[i]);
            max[i] = MAX(max[i], pos[i]);
        }
    }
    min.y = 0;
    piece->collision_box.center = 0.5 * (max + min);
    piece->collision_box.radius = 0.5 * (max - min);

    piece->animation_type = AnimationType::stand;
    piece->alpha = 1;
}

Void start_move_animation(Piece *piece, Int row, Int column)
{
    piece->animation_type = AnimationType::move;
    piece->animation.t = 0;
    piece->animation.pos_from = piece->pos;
    piece->animation.pos_to = get_square_pos(row, column);
}

Void start_jump_animation(Piece *piece, Int row, Int column)
{
    piece->animation_type = AnimationType::jump;
    piece->animation.t = 0;
    piece->animation.pos_from = piece->pos;
    piece->animation.pos_to = get_square_pos(row, column);
}

Void start_flash_animation(Piece *piece)
{
    piece->animation_type = AnimationType::flash;
    piece->animation.t = 0;
}

Void stop_flash_animation(Piece *piece)
{
    piece->animation_type = AnimationType::stand;
    piece->alpha = 1;
}

Void update_animation(Piece *piece, Real dt)
{
    if (piece->animation_type == AnimationType::move)
    {
        piece->animation.t += dt;
        Real a = 0.1;
        Real ft = a * square(piece->animation.t) + (1 - a) * piece->animation.t;
        piece->pos = lerp(piece->animation.pos_from, piece->animation.pos_to, ft);

        if (piece->animation.t >= 1)
        {
            piece->animation_type = AnimationType::stand;
        }
    }
    else if (piece->animation_type == AnimationType::jump)
    {
        piece->animation.t += dt;
        Vec3 new_pos = lerp(piece->animation.pos_from, piece->animation.pos_to, piece->animation.t);
        Real height = -200;
        Real a = -4 * height;
        Real b = 4 * height;
        new_pos.y = a * square(piece->animation.t) + b * piece->animation.t;
        piece->pos = new_pos;

        if (piece->animation.t >= 1)
        {
            piece->animation_type = AnimationType::stand;
        }
    }
    else if (piece->animation_type == AnimationType::flash)
    {
        piece->animation.t += dt * 0.2;
        Real alpha_min = 0.5;
        if (piece->animation.t <= 0.5)
        {
            piece->alpha = lerp(1.0f, alpha_min, piece->animation.t * 2);
        }
        else
        {
            piece->alpha = lerp(alpha_min, 1.0f, (piece->animation.t - 0.5) * 2);
        }

        if (piece->animation.t >= 1)
        {
            piece->animation.t -= 1;
        }
    }
}

struct GhostPiece : Entity
{
};

GhostPiece get_ghost_piece(Piece *piece, Int row, Int column)
{
    GhostPiece result;
    result.pos = get_square_pos(row, column);
    result.rotation = piece->rotation;
    result.scale = piece->scale;
    result.mesh = piece->mesh;
    result.alpha = 0.7;
    return result;
}
