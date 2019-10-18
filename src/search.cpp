#pragma once

#include "game.cpp"

Int material_values[GamePieceType::count] = {100, 325, 335, 500, 975, 0};

Int eval_material(GameState *state, GameSideEnum side)
{
    Int value = 0;
    for (GamePieceTypeEnum piece_type = 0; piece_type < GamePieceType::king; piece_type++)
    {
        BitBoard occupancy = get_occupancy(state, side, piece_type);
        while (occupancy)
        {
            occupancy = occupancy & (occupancy - 1);
            value += material_values[piece_type];
        }
    }
    return value;
}

Int square_values[GamePieceType::count][8][8] = {
    // NOTE: Pawn
    {
        {0, 0, 0, 0, 0, 0, 0, 0},
        {-4, -4, 1, 5, 5, 1, -4, -4},
        {-6, -4, 5, 10, 10, 5, -4, -6},
        {-6, -4, 2, 8, 8, 2, -4, -6},
        {-6, -4, 1, 2, 2, 1, -4, -6},
        {-6, -4, 1, 1, 1, 1, -4, -6},
        {0, 0, 0, 0, 0, 0, 0, 0},
    },
    // NOTE: Knight
    {
        {-16, -12, -8, -8, -8, -8, -12, -1},
        {-8, 0, 1, 2, 2, 1, 0, -8},
        {-8, 0, 4, 6, 6, 4, 0, -8},
        {-8, 0, 6, 8, 8, 6, 0, -8},
        {-8, 0, 6, 8, 8, 6, 0, -8},
        {-8, 0, 4, 6, 6, 4, 0, -8},
        {-8, 0, 0, 0, 0, 0, 0, -8},
        {-8, -8, -8, -8, -8, -8, -8, -8},
    },
    // NOTE: Bishop
    {
        {-4, -4, -12, -4, -4, -12, -4, -4},
        {-4, 2, 1, 1, 1, 1, 2, -4},
        {-4, 1, 2, 4, 4, 2, 1, -4},
        {-4, 0, 4, 6, 6, 4, 0, -4},
        {-4, 0, 4, 6, 6, 4, 0, -4},
        {-4, 0, 2, 4, 4, 2, 0, -4},
        {-4, 0, 0, 0, 0, 0, 0, -4},
        {-4, -4, -4, -4, -4, -4, -4, -4},
    },
    // NOTE: Rook
    {
        {0, 0, 0, 2, 2, 0, 0, 0},
        {-5, 0, 0, 0, 0, 0, 0, -5},
        {-5, 0, 0, 0, 0, 0, 0, -5},
        {-5, 0, 0, 0, 0, 0, 0, -5},
        {-5, 0, 0, 0, 0, 0, 0, -5},
        {-5, 0, 0, 0, 0, 0, 0, -5},
        {-5, 0, 0, 0, 0, 0, 0, -5},
        {5, 5, 5, 5, 5, 5, 5, 5},
    },
    // NOTE: Queen
    {
        {-5, -5, -5, -5, -5, -5, -5, -5},
        {0, 0, 1, 1, 1, 1, 0, 0},
        {0, 0, 1, 2, 2, 1, 0, 0},
        {0, 0, 2, 3, 3, 2, 0, 0},
        {0, 0, 2, 3, 3, 2, 0, 0},
        {0, 0, 1, 2, 2, 1, 0, 0},
        {0, 0, 1, 1, 1, 1, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 0},
    },
};

Int king_square_values_middle[8][8] = {
    {40, 50, 30, 10, 10, 30, 50, 40},
    {30, 40, 20, 0, 0, 20, 40, 30},
    {10, 20, 0, -20, -20, 0, 20, 10},
    {0, 10, -10, -30, -30, -10, 10, 0},
    {-10, 0, -20, -40, -40, -20, 0, -10},
    {-20, -10, -30, -50, -50, -30, -10, -20},
    {-30, -20, -40, -60, -60, -40, -20, -30},
    {-40, -30, -50, -70, -70, -50, -30, -40},
};

Int king_square_values_end[8][8] = {
    {-72, -48, -36, -24, -24, -36, -48, -72},
    {-48, -24, -12, 0, 0, -12, -24, -48},
    {-36, -12, 0, 12, 12, 0, -12, -36},
    {-24, 0, 12, 24, 24, 12, 0, -24},
    {-24, 0, 12, 24, 24, 12, 0, -24},
    {-36, -12, 0, 12, 12, 0, -12, -36},
    {-48, -24, -12, 0, 0, -12, -24, -48},
    {-72, -48, -36, -24, -24, -36, -48, -72},
};

Int eval_square(GameState *state, GameSideEnum side, Bool middle)
{
    Int value = 0;
    for (GamePieceTypeEnum piece_type = 0; piece_type < GamePieceType::count; piece_type++)
    {
        BitBoard occupancy = get_occupancy(state, side, piece_type);
        while (occupancy)
        {
            BitBoard bit_square = occupancy & -occupancy;
            occupancy -= bit_square;
            Square square = first_set(bit_square);
            Square row_rel = get_row_rel(square, side);
            Square column_rel = get_column_rel(square, side);

            if (piece_type == GamePieceType::king)
            {
                value += middle ? king_square_values_middle[row_rel][column_rel] : king_square_values_end[row_rel][column_rel];
            }
            else
            {
                value += square_values[piece_type][row_rel][column_rel];
            }
        }
    }
}

Int knight_pawn_adjust_values[9] = {-20, -16, -12, -8, -4, 0, 4, 8, 12};
Int rook_pawn_adjust_values[9] = {15, 12, 9, 6, 3, 0, -3, -6, -9};

Int eval_material_adjust(GameState *state, GameSideEnum side)
{
    Int value = 0;
    Int bishop_count = bit_count(get_occupancy(state, side, GamePieceType::bishop));
    if (bishop_count > 1)
    {
        value += 30;
    }

    Int knight_count = bit_count(get_occupancy(state, side, GamePieceType::knight));
    if (knight_count > 1)
    {
        value -= 8;
    }

    Int rook_count = bit_count(get_occupancy(state, side, GamePieceType::rook));
    if (rook_count > 1)
    {
        value -= 16;
    }

    Int pawn_count = bit_count(get_occupancy(state, side, GamePieceType::pawn));
    value += knight_pawn_adjust_values[pawn_count] * knight_count;
    value += rook_pawn_adjust_values[pawn_count] * rook_count;
    return value;
}

Int passed_pawn_values[8][8] = {
    {0, 0, 0, 0, 0, 0, 0, 0},
    {20, 20, 20, 20, 20, 20, 20, 20},
    {20, 20, 20, 20, 20, 20, 20, 20},
    {32, 32, 32, 32, 32, 32, 32, 32},
    {56, 56, 56, 56, 56, 56, 56, 56},
    {92, 92, 92, 92, 92, 92, 92, 92},
    {140, 140, 140, 140, 140, 140, 140, 140},
    {0, 0, 0, 0, 0, 0, 0, 0},
};

Int weak_pawn_values[8][8] = {
    {0, 0, 0, 0, 0, 0, 0, 0},
    {-10, -12, -14, -16, -16, -14, -12, -10},
    {-10, -12, -14, -16, -16, -14, -12, -10},
    {-10, -12, -14, -16, -16, -14, -12, -10},
    {-10, -12, -14, -16, -16, -14, -12, -10},
    {-10, -12, -14, -16, -16, -14, -12, -10},
    {-10, -12, -14, -16, -16, -14, -12, -10},
    {0, 0, 0, 0, 0, 0, 0, 0},
};

Int eval_pawn_structure(GameState *state, GameSideEnum side)
{
    Int value = 0;
    GameSideEnum oppose_side = oppose(side);
    BitBoard pawn_occupancy = get_occupancy(state, side, GamePieceType::pawn);
    BitBoard oppose_pawn_occupancy = get_occupancy(state, oppose_side, GamePieceType::pawn);
    BitBoard both_pawn_occupancy = pawn_occupancy | oppose_pawn_occupancy;

    BitBoard pawn_occupancy_copy = pawn_occupancy;
    while (pawn_occupancy_copy)
    {
        BitBoard pawn_bit = pawn_occupancy_copy & -pawn_occupancy_copy;
        pawn_occupancy_copy -= pawn_bit;
        Square pawn_square = first_set(pawn_bit);
        Square row = get_row(pawn_square);
        Square column = get_column(pawn_square);
        Square row_rel = get_row_rel(pawn_square, side);
        Square column_rel = get_column_rel(pawn_square, side);

        BitBoard column_mask = get_column_mask(column);
        BitBoard adj_column_mask = (column > 0 ? get_column_mask(column - 1) : 0) | (column < 7 ? get_column_mask(column + 1) : 0);
        BitBoard forward_mask = get_forward_mask(row, side);

        BitBoard non_passed_mask = (adj_column_mask & forward_mask & oppose_pawn_occupancy) | (column_mask & forward_mask & both_pawn_occupancy);
        BitBoard opposed_mask = column_mask & forward_mask & oppose_pawn_occupancy;
        BitBoard doubled_mask = column_mask & forward_mask & pawn_occupancy;
        BitBoard non_weak_mask = adj_column_mask & ~forward_mask & pawn_occupancy;

        if (doubled_mask)
        {
            value -= 20 * bit_count(doubled_mask);
        }

        if (!non_passed_mask)
        {
            Int passed_value = passed_pawn_values[row_rel][column_rel];
            BitBoard supported_mask = (left(pawn_bit, side) | right(pawn_bit, side) | down_left(pawn_bit, side) | down_right(pawn_bit, side)) & pawn_occupancy;
            if (supported_mask)
            {
                passed_value = passed_value * 10 / 8;
            }
            value += passed_value;
        }

        if (!non_weak_mask)
        {
            Int weak_value = weak_pawn_values[row_rel][column_rel];
            if (!opposed_mask)
            {
                weak_value -= 4;
            }
            value += weak_value;
        }
    }
    return value;
}

Int eval_pieces(GameState *state, GameSideEnum side)
{
    for (GamePieceTypeEnum piece_type = GamePieceType::knight; piece_type < GamePieceType::king; piece_type++)
    {
    }
}

Int eval_blockage(GameState *state, GameSideEnum side)
{
    Int value = 0;

    // NOTE: Central pawn block initial bishop
    BitBoard both_occupancy = get_occupancy(state);
    BitBoard pawn_occupancy = get_occupancy(state, side, GamePieceType::pawn);
    BitBoard bishop_occupancy = get_occupancy(state, side, GamePieceType::bishop);
    BitBoard central_pawn_mask[2] = {bit_square(get_abs_square(Sq::d2, side)), bit_square(get_abs_square(Sq::e2, side))};
    BitBoard initial_bishop_mask[2] = {bit_square(get_abs_square(Sq::c1, side)), bit_square(get_abs_square(Sq::f1, side))};
    for (Int i = 0; i < 2; i++)
    {
        BitBoard blocking_pawn_mask = up(central_pawn_mask[i], side);
        if ((central_pawn_mask[i] & pawn_occupancy) && (initial_bishop_mask[i] & bishop_occupancy) && (blocking_pawn_mask & both_occupancy))
        {
            value -= 24;
        }
    }

    // NOTE: Trapped knight at oppose corner
    BitBoard knight_occupancy = get_occupancy(state, side, GamePieceType::knight);
    BitBoard oppose_pawn_occupancy = get_occupancy(state, oppose(side), GamePieceType::pawn);
    BitBoard trapped_knight_mask[2] = {bit_square(get_abs_square(Sq::a8, side)), bit_square(get_abs_square(Sq::h8, side))};
    BitBoard knight_blocking_pawn_mask[2] = {bit_square(get_abs_square(Sq::a7, side)) | bit_square(get_abs_square(Sq::c7, side)), bit_square(get_abs_square(Sq::h7, side)) | bit_square(get_abs_square(Sq::f7, side))};
    BitBoard trapped_knight_mask2[2] = {bit_square(get_abs_square(Sq::a7, side)), bit_square(get_abs_square(Sq::h7, side))};
    BitBoard knight_blocking_pawn_mask2[2] = {bit_square(get_abs_square(Sq::a6, side)) | bit_square(get_abs_square(Sq::b7, side)), bit_square(get_abs_square(Sq::h6, side)) | bit_square(get_abs_square(Sq::g7, side))};
    for (Int i = 0; i < 2; i++)
    {
        if ((trapped_knight_mask[i] & knight_occupancy) && (knight_blocking_pawn_mask[i] & oppose_pawn_occupancy))
        {
            value -= 150;
        }
        if ((trapped_knight_mask2[i] & knight_occupancy) && ((knight_blocking_pawn_mask2[i] & oppose_pawn_occupancy) == knight_blocking_pawn_mask2[i]))
        {
            value -= 100;
        }
    }

    // NOTE: Trapped bishop at oppose corner
    BitBoard trapped_bishop_mask[4] = {bit_square(get_abs_square(Sq::a7, side)), bit_square(get_abs_square(Sq::h7, side)), bit_square(get_abs_square(Sq::b8, side)), bit_square(get_abs_square(Sq::g8, side))};
    BitBoard bishop_blocking_pawn_mask[4] = {bit_square(get_abs_square(Sq::b6, side)), bit_square(get_abs_square(Sq::g6, side)), bit_square(get_abs_square(Sq::c7, side)), bit_square(get_abs_square(Sq::f7, side))};
    BitBoard trapped_bishop_mask2[2] = {bit_square(get_abs_square(Sq::a6, side)), bit_square(get_abs_square(Sq::b5, side))};
    BitBoard bishop_blocking_pawn_mask2[2] = {bit_square(get_abs_square(Sq::h6, side)), bit_square(get_abs_square(Sq::g5, side))};
    for (Int i = 0; i < 2; i++)
    {
        if ((trapped_bishop_mask[i] & bishop_occupancy) && (bishop_blocking_pawn_mask[i] & oppose_pawn_occupancy))
        {
            value -= 150;
        }
        if ((trapped_bishop_mask2[i] & bishop_occupancy) && (bishop_blocking_pawn_mask2[i] & oppose_pawn_occupancy))
        {
            value -= 50;
        }
    }

    // NOTE: Uncastled king block rook
    BitBoard king_occupancy = get_occupancy(state, side, GamePieceType::king);
    BitBoard rook_occupancy = get_occupancy(state, side, GamePieceType::rook);
    BitBoard blocking_king_mask[2] = {bit_square(get_abs_square(Sq::c1, side)) | bit_square(get_abs_square(Sq::b1, side)), bit_square(get_abs_square(Sq::f1, side)) | bit_square(get_abs_square(Sq::g1, side))};
    BitBoard blocking_rook_mask[2] = {bit_square(get_abs_square(Sq::a1, side)) | bit_square(get_abs_square(Sq::b1, side)), bit_square(get_abs_square(Sq::h1, side)) | bit_square(get_abs_square(Sq::g1, side))};
    for (Int i = 0; i < 2; i++)
    {
        if ((blocking_king_mask[i] & king_occupancy) && (blocking_rook_mask[i] & rook_occupancy))
        {
            value -= 24;
        }
    }

    // NOTE: Knight block queen side pawn
    if ((knight_occupancy & bit_square(get_abs_square(Sq::c3, side))) &&
        (pawn_occupancy & bit_square(get_abs_square(Sq::c2, side))) &&
        (pawn_occupancy & bit_square(get_abs_square(Sq::d4, side))) &&
        !(pawn_occupancy & bit_square(get_abs_square(Sq::e4, side))))
    {
        value -= 5;
    }

    return value;
}

Int eval_shield(GameState *state, GameSideEnum side)
{
    Int value = 0;
    Square king_square = get_king_square(state, side);
    Square row = get_row(king_square);
    Square column = get_column(king_square);
    Int row_rel = get_row_rel(king_square, side);
    if (row_rel == 0)
    {
        BitBoard shield_mask = column < 3 ? 0x700 : column > 4 ? 0xe000 : 0;
        if (shield_mask)
        {
            BitBoard shield_mask2 = shield_mask << 8;
            if (side == GameSide::black)
            {
                shield_mask <<= 40;
                shield_mask2 <<= 24;
            }

            BitBoard pawn_occupancy = get_occupancy(state, side, GamePieceType::pawn);
            BitBoard shield = pawn_occupancy & shield_mask;
            BitBoard shield2 = pawn_occupancy & shield_mask2;
            if (shield)
            {
                value += 10 * bit_count(shield);
            }
            else if (shield2)
            {
                value += 5 * bit_count(shield2);
            }
        }
    }
    return value;
}

Int eval(GameState *state, GameSideEnum side)
{
    GameSideEnum oppose_side = oppose(side);
    Int value_middle = 0;
    Int value_end = 0;

    Int value_material = eval_material(state, side) - eval_material(state, oppose_side);

    Int value_material_adjust = eval_material_adjust(state, side) - eval_material_adjust(state, side);

    Int value_square_middle = eval_square(state, side, true) - eval_square(state, oppose_side, true);
    Int value_square_end = eval_square(state, side, false) - eval_square(state, oppose_side, true);

    Int value_pawn_structure = eval_pawn_structure(state, side) - eval_pawn_structure(state, oppose_side);

    Int value_blockage = eval_blockage(state, side) - eval_blockage(state, oppose_side);

    Int value_shield = eval_shield(state, side) - eval_shield(state, oppose_side);

    Int value_tempo = state->current_side == side ? 10 : -10;
}

struct ValuedMove
{
    GameMove move;
    Int value;
};

#define VALUE_INF (1000000000)

ValuedMove negamax(GameState *state, Int alpha, Int beta, Int depth)
{
    if (depth >= 1)
    {
        ValuedMove valued_move;
        valued_move.value = eval(state, state->current_side);
        return valued_move;
    }

    GameMove moves_data[256];
    Buffer<GameMove> moves;
    moves.count = 0;
    moves.data = moves_data;
    generate_all_moves(state, &moves);

    ValuedMove best_move;
    best_move.value = -VALUE_INF;
    for (Int i = 0; i < moves.count; i++)
    {
        GameMove move = moves[i];

        record_game_move(state, move);
        ValuedMove valued_move = negamax(state, -beta, -alpha, depth + 1);
        valued_move.value = -valued_move.value;
        rollback_game_move(state, move);

        if (valued_move.value > best_move.value)
        {
            best_move.value = valued_move.value;
            best_move.move = move;
        }

        alpha = MAX(alpha, best_move.value);
        if (alpha >= beta)
        {
            break;
        }
    }
    return best_move;
}
