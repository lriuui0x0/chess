#pragma once

#include "../lib/util.hpp"
#include "math.cpp"
#include "asset.cpp"
#include "game.cpp"

struct Ray
{
    Vec3 pos;
    Vec3 dir;
};

struct CollisionBox
{
    Vec3 center;
    Vec3 radius;
};

Real check_collision(Ray *ray, CollisionBox *collision_box)
{
    Real t_min = 0;
    Real t_max = 100000000;
    for (Int i = 0; i < 3; i++)
    {
        Real reciprocal_d = 1 / ray->dir[i];
        Real t1 = (collision_box->center[i] - collision_box->radius[i] - ray->pos[i]) * reciprocal_d;
        Real t2 = (collision_box->center[i] + collision_box->radius[i] - ray->pos[i]) * reciprocal_d;
        if (t1 > t2)
        {
            swap(&t1, &t2);
        }

        t_min = MAX(t_min, t1);
        t_max = MIN(t_max, t2);
        if (t_min > t_max)
        {
            return -1;
        }
    }

    return t_min;
}

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
    Mesh *mesh;
};

struct Animation
{
    Real t;
    Vec3 pos_from;
    Vec3 pos_to;
    Quaternion rotation;
};

enum struct AnimationType
{
    stand,
    move,
    jump,
};

struct Piece : Entity
{
    GamePiece *game_piece;
    CollisionBox collision_box;
    AnimationType animation_type;
    Animation animation;
};

struct Board : Entity
{
    CollisionBox collision_box[BOARD_SQUARE_COUNT];
};

Vec3 get_square_pos(Int row, Int column)
{
    return {(Real)(column * 100.0), 0, (Real)(row * 100.0)};
}

Void start_move_animation(Piece *piece, Int row_change, Int column_change)
{
    piece->animation_type = AnimationType::move;
    piece->animation.t = 0;
    piece->animation.pos_from = piece->pos;
    piece->animation.pos_to = piece->pos + get_square_pos(row_change, column_change);
}

Void start_jump_animation(Piece *piece, Int row_change, Int column_change)
{
    piece->animation_type = AnimationType::jump;
    piece->animation.t = 0;
    piece->animation.pos_from = piece->pos;
    piece->animation.pos_to = piece->pos + get_square_pos(row_change, column_change);
    piece->animation.rotation = piece->rotation;
}

Void update_animation(Piece *piece, Real dt)
{
    if (piece->animation_type == AnimationType::move)
    {
        piece->animation.t += dt;
        piece->pos = lerp(piece->animation.pos_from, piece->animation.pos_to, piece->animation.t);
    }
    else if (piece->animation_type == AnimationType::jump)
    {
        piece->animation.t += dt;
        Real height = -400;
        Real rotate_angle = degree_to_radian(10);
        Real first_half = 0.5;
        if (piece->animation.t <= 0.5)
        {
            Real t = piece->animation.t * 2;
            Vec3 pos_to = lerp(piece->animation.pos_from, piece->animation.pos_to, first_half) + Vec3{0, height, 0};
            Quaternion rotation_to = get_rotation_quaternion(get_basis_x(), rotate_angle) * piece->animation.rotation;
            piece->pos = lerp(piece->animation.pos_from, pos_to, t);
            piece->rotation = slerp(piece->animation.rotation, rotation_to, t);
        }
        else
        {
            Real t = (piece->animation.t - 0.5) * 2;
            Vec3 pos_from = lerp(piece->animation.pos_from, piece->animation.pos_to, first_half) + Vec3{0, height, 0};
            Quaternion rotation_from = get_rotation_quaternion(get_basis_x(), rotate_angle) * piece->animation.rotation;
            piece->pos = lerp(pos_from, piece->animation.pos_to, t);
            piece->rotation = slerp(rotation_from, piece->animation.rotation, t);
        }
    }

    if (piece->animation.t >= 1)
    {
        piece->animation_type = AnimationType::stand;
    }
}

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
            if (game_piece->index == 0)
            {
                piece->pos = get_square_pos(0, 0);
            }
            else if (game_piece->index == 1)
            {
                piece->pos = get_square_pos(0, 7);
            }
        }
        break;

        case GamePieceType::knight:
        {
            piece->mesh = white_knight_mesh;
            if (game_piece->index == 0)
            {
                piece->pos = get_square_pos(0, 1);
            }
            else if (game_piece->index == 1)
            {
                piece->pos = get_square_pos(0, 6);
            }
        }
        break;

        case GamePieceType::bishop:
        {
            piece->mesh = white_bishop_mesh;
            if (game_piece->index == 0)
            {
                piece->pos = get_square_pos(0, 2);
            }
            else if (game_piece->index == 1)
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
            piece->pos = get_square_pos(1, game_piece->index);
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
            if (game_piece->index == 0)
            {
                piece->pos = get_square_pos(7, 0);
            }
            else if (game_piece->index == 1)
            {
                piece->pos = get_square_pos(7, 7);
            }
        }
        break;

        case GamePieceType::knight:
        {
            piece->mesh = black_knight_mesh;
            if (game_piece->index == 0)
            {
                piece->pos = get_square_pos(7, 1);
            }
            else if (game_piece->index == 1)
            {
                piece->pos = get_square_pos(7, 6);
            }
        }
        break;

        case GamePieceType::bishop:
        {
            piece->mesh = black_bishop_mesh;
            if (game_piece->index == 0)
            {
                piece->pos = get_square_pos(7, 2);
            }
            else if (game_piece->index == 1)
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
            piece->pos = get_square_pos(6, game_piece->index);
        }
        break;
        }
    }

    // NOTE: Set up piece collision boxes
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
}
