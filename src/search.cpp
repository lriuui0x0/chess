#pragma once

#include "game.cpp"

struct ValuePair
{
    Int middle;
    Int end;
};

ValuePair operator-(ValuePair x, ValuePair y)
{
    ValuePair result;
    result.middle = x.middle - y.middle;
    result.middle = x.end - y.end;
    return result;
}

Int material_values[GamePieceType::count] = {100, 325, 335, 500, 975, 0};

struct MaterialValue
{
    Int pawn;
    Int non_pawn;
    Int all;
};

MaterialValue eval_material(GameState *state, GameSideEnum side)
{
    MaterialValue value = {};
    for (GamePieceTypeEnum piece_type = GamePieceType::pawn; piece_type < GamePieceType::king; piece_type++)
    {
        BitBoard occupancy = get_occupancy(state, side, piece_type);
        while (occupancy)
        {
            occupancy = occupancy & (occupancy - 1);

            if (piece_type == GamePieceType::pawn)
            {
                value.pawn += material_values[piece_type];
            }
            else
            {
                value.non_pawn += material_values[piece_type];
            }
        }
    }
    value.all = value.pawn + value.non_pawn;
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

ValuePair eval_square(GameState *state, GameSideEnum side)
{
    ValuePair value = {0, 0};
    for (GamePieceTypeEnum piece_type = 0; piece_type < GamePieceType::count; piece_type++)
    {
        BitBoard occupancy = get_occupancy(state, side, piece_type);
        while (occupancy)
        {
            BitBoard square_bit = occupancy & -occupancy;
            occupancy -= square_bit;
            Square square = first_set(square_bit);
            Square row_rel = get_row_rel(square, side);
            Square column_rel = get_column_rel(square, side);

            if (piece_type == GamePieceType::king)
            {
                value.middle += king_square_values_middle[row_rel][column_rel];
                value.end += king_square_values_end[row_rel][column_rel];
            }
            else
            {
                value.middle += square_values[piece_type][row_rel][column_rel];
                value.end += square_values[piece_type][row_rel][column_rel];
            }
        }
    }
    return value;
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
            BitBoard supported_mask = left(pawn_occupancy, side) | right(pawn_occupancy, side) | down_left(pawn_occupancy, side) | down_right(pawn_occupancy, side);
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

ValuePair eval_mobility(GameState *state, GameSideEnum side)
{
    ValuePair value = {};
    GameSideEnum oppose_side = oppose(side);
    BitBoard oppose_pawn_occupancy = get_occupancy(state, oppose_side, GamePieceType::pawn);
    BitBoard oppose_pawn_control = up_left(oppose_pawn_occupancy, oppose_side) | up_right(oppose_pawn_occupancy, oppose_side);
    for (GamePieceTypeEnum piece_type = GamePieceType::knight; piece_type <= GamePieceType::queen; piece_type++)
    {
        BitBoard occupancy = get_occupancy(state, side, piece_type);
        while (occupancy)
        {
            BitBoard square_bit = occupancy & -occupancy;
            occupancy -= square_bit;
            Square square = first_set(square_bit);
            BitBoard move = 0;
            switch (piece_type)
            {
            case GamePieceType::knight:
            {
                move = check_knight_move(state, square, side);
            }
            break;

            case GamePieceType::bishop:
            {
                move = check_bishop_move(state, square, side);
            }
            break;

            case GamePieceType::rook:
            {
                move = check_rook_move(state, square, side);
            }
            break;

            case GamePieceType::queen:
            {
                move = check_queen_move(state, square, side);
            }
            break;
            }
            BitBoard mobility_mask = move & ~oppose_pawn_control;
            Int mobility = bit_count(mobility_mask);

            switch (piece_type)
            {
            case GamePieceType::knight:
            {
                value.middle += 4 * (mobility - 4);
                value.end += 4 * (mobility - 4);
            }
            break;

            case GamePieceType::bishop:
            {
                value.middle += 3 * (mobility - 7);
                value.end += 3 * (mobility - 7);
            }
            break;

            case GamePieceType::rook:
            {
                value.middle += 2 * (mobility - 7);
                value.end += 4 * (mobility - 7);
            }
            break;

            case GamePieceType::queen:
            {
                value.middle += 1 * (mobility - 14);
                value.end += 2 * (mobility - 14);
            }
            break;
            }
        }
    }
    return value;
}

ValuePair eval_theme(GameState *state, GameSideEnum side)
{
    ValuePair value = {};
    // NOTE: Bishop support castled king
    BitBoard bishop_occupancy = get_occupancy(state, side, GamePieceType::bishop);
    BitBoard king_occupancy = get_occupancy(state, side, GamePieceType::king);
    BitBoard bishop_support_mask[2] = {bit_square(get_abs_square(Sq::c1, side)), bit_square(get_abs_square(Sq::b1, side))};
    BitBoard king_support_mask[2] = {bit_square(get_abs_square(Sq::f1, side)), bit_square(get_abs_square(Sq::g1, side))};
    for (Int i = 0; i < 2; i++)
    {
        if ((bishop_support_mask[i] & bishop_occupancy) && (king_support_mask[i] & king_occupancy))
        {
            value.middle += 20;
            value.end += 20;
        }
    }

    // NOTE: Rook / Queen near enemy king line
    GameSideEnum oppose_side = oppose(side);
    Square oppose_king_square = first_set(get_occupancy(state, oppose_side, GamePieceType::king));
    Square oppose_king_row_rel = get_row_rel(oppose_king_square, side);
    BitBoard oppose_pawn_occupancy = get_occupancy(state, oppose_side, GamePieceType::pawn);
    for (GamePieceTypeEnum piece_type = GamePieceType::rook; piece_type <= GamePieceType::queen; piece_type++)
    {
        BitBoard occupancy = get_occupancy(state, side, piece_type);
        while (occupancy)
        {
            BitBoard square_bit = occupancy & -occupancy;
            occupancy -= square_bit;
            Square square = first_set(square_bit);
            Square row = get_row(square);
            Square row_rel = get_row_rel(square, side);
            if (row_rel == 6 && (oppose_king_row_rel == 7 || (oppose_pawn_occupancy & get_row_mask(row))))
            {
                switch (piece_type)
                {
                case GamePieceType::rook:
                {
                    value.middle += 20;
                    value.end += 30;
                }
                break;

                case GamePieceType::queen:
                {
                    value.middle += 5;
                    value.end += 10;
                }
                break;
                }
            }
        }
    }

    // NOTE: Rook open columns
    BitBoard rook_occupancy = get_occupancy(state, side, GamePieceType::rook);
    BitBoard pawn_occupancy = get_occupancy(state, side, GamePieceType::pawn);
    while (rook_occupancy)
    {
        BitBoard square_bit = rook_occupancy & -rook_occupancy;
        rook_occupancy -= square_bit;
        Square square = first_set(square_bit);
        Square column = get_column(square);
        BitBoard column_mask = get_column_mask(column);

        if (!(pawn_occupancy & column_mask))
        {
            if (!(oppose_pawn_occupancy & column_mask))
            {
                value.middle += 10;
                value.end += 10;
            }
            else
            {
                value.middle += 5;
                value.end += 5;
            }
        }
    }

    // NOTE: Queen moves too early
    BitBoard queen_occupancy = get_occupancy(state, side, GamePieceType::queen);
    while (queen_occupancy)
    {
        BitBoard square_bit = queen_occupancy & -queen_occupancy;
        queen_occupancy -= square_bit;
        Square square = first_set(square_bit);
        Square row_rel = get_row_rel(square, side);
        if (row_rel > 1)
        {
            BitBoard knight_occupancy = get_occupancy(state, side, GamePieceType::knight);
            BitBoard knight_initial_mask = bit_square(get_abs_square(Sq::b1, side)) | bit_square(get_abs_square(Sq::g1, side));
            Int non_moved_knight_count = bit_count(knight_occupancy & knight_initial_mask);

            BitBoard bishop_occupancy = get_occupancy(state, side, GamePieceType::bishop);
            BitBoard bishop_initial_mask = bit_square(get_abs_square(Sq::c1, side)) | bit_square(get_abs_square(Sq::f1, side));
            Int non_moved_bishop_count = bit_count(bishop_occupancy & bishop_initial_mask);

            value.middle -= (non_moved_knight_count + non_moved_bishop_count) * 2;
            value.middle -= (non_moved_knight_count + non_moved_bishop_count) * 2;
        }
    }

    return value;
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

Int attack_values[100] = {0, 0, 1, 2, 3, 5, 7, 9, 12, 15,
                          18, 22, 26, 30, 35, 39, 44, 50, 56, 62,
                          68, 75, 82, 85, 89, 97, 105, 113, 122, 131,
                          140, 150, 169, 180, 191, 202, 213, 225, 237, 248,
                          260, 272, 283, 295, 307, 319, 330, 342, 354, 366,
                          377, 389, 401, 412, 424, 436, 448, 459, 471, 483,
                          494, 500, 500, 500, 500, 500, 500, 500, 500, 500,
                          500, 500, 500, 500, 500, 500, 500, 500, 500, 500,
                          500, 500, 500, 500, 500, 500, 500, 500, 500, 500,
                          500, 500, 500, 500, 500, 500, 500, 500, 500, 500};

Int eval_attack(GameState *state, GameSideEnum side)
{
    Int count = 0;
    Int weight = 0;
    GameSideEnum oppose_side = oppose(side);
    BitBoard king_bit = get_occupancy(state, oppose_side, GamePieceType::king);
    Square king_square = first_set(king_bit);
    BitBoard king_move = state->bit_board_table->king_table.move[king_square];
    BitBoard near_king_mask = (king_move | up(king_move, oppose_side)) & ~king_bit;
    for (GamePieceTypeEnum piece_type = GamePieceType::knight; piece_type <= GamePieceType::queen; piece_type++)
    {
        BitBoard occupancy = get_occupancy(state, side, piece_type);
        while (occupancy)
        {
            BitBoard square_bit = occupancy & -occupancy;
            occupancy -= square_bit;
            Square square = first_set(square_bit);
            BitBoard move = 0;
            switch (piece_type)
            {
            case GamePieceType::knight:
            {
                move = check_knight_move(state, square, side);
            }
            break;

            case GamePieceType::bishop:
            {
                move = check_bishop_move(state, square, side);
            }
            break;

            case GamePieceType::rook:
            {
                move = check_rook_move(state, square, side);
            }
            break;

            case GamePieceType::queen:
            {
                move = check_queen_move(state, square, side);
            }
            break;
            }
            BitBoard attack_mask = move & near_king_mask;
            Int attack = bit_count(attack_mask);

            count++;
            switch (piece_type)
            {
            case GamePieceType::knight:
            {
                weight += 2 * attack;
            }
            break;

            case GamePieceType::bishop:
            {
                weight += 2 * attack;
            }
            break;

            case GamePieceType::rook:
            {
                weight += 3 * attack;
            }
            break;

            case GamePieceType::queen:
            {
                weight += 4 * attack;
            }
            break;
            }
        }
    }

    BitBoard queen_occupancy = get_occupancy(state, side, GamePieceType::queen);
    Int queen_count = bit_count(queen_occupancy);
    if (count < 2 || queen_count == 0)
    {
        weight = 0;
    }

    Int value = attack_values[weight];
    return value;
}

ValuePair eval_safety(GameState *state, GameSideEnum side)
{
    ValuePair value = {};
    GameSideEnum oppose_side = oppose(side);
    Square king_square = first_set(get_occupancy(state, side, GamePieceType::king));
    for (GamePieceTypeEnum piece_type = GamePieceType::knight; piece_type <= GamePieceType::queen; piece_type++)
    {
        BitBoard occupancy = get_occupancy(state, oppose_side, piece_type);
        while (occupancy)
        {
            BitBoard square_bit = occupancy & -occupancy;
            occupancy -= square_bit;
            Square square = first_set(square_bit);

            Int distance = ABS(get_row(square) - get_row(king_square)) + ABS(get_column(square) - get_column(king_square));
            Int safety = distance - 7;

            switch (piece_type)
            {
            case GamePieceType::knight:
            {
                value.middle += 3 * safety;
                value.end += 3 * safety;
            }
            break;

            case GamePieceType::bishop:
            {
                value.middle += 2 * safety;
                value.end += 1 * safety;
            }
            break;

            case GamePieceType::rook:
            {
                value.middle += 2 * safety;
                value.end += 1 * safety;
            }
            break;

            case GamePieceType::queen:
            {
                value.middle += 2 * safety;
                value.end += 4 * safety;
            }
            break;
            }
        }
    }

    // NOTE: Pawn shield
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
                value.middle += 10 * bit_count(shield);
            }
            else if (shield2)
            {
                value.middle += 5 * bit_count(shield2);
            }
        }
    }
    return value;
}

Int phase_values[GamePieceType::count] = {0, 1, 1, 2, 4, 0};

Int eval(GameState *state, GameSideEnum side)
{
    Int middle = 0;
    Int end = 0;

    MaterialValue materials[2] = {eval_material(state, GameSide::white), eval_material(state, GameSide::black)};
    Int material = materials[GameSide::white].all - materials[GameSide::black].all;
    middle += material;
    end += material;

    Int material_adjust = eval_material_adjust(state, GameSide::white) - eval_material_adjust(state, GameSide::black);
    middle += material_adjust;
    end += material_adjust;

    ValuePair square = eval_square(state, GameSide::white) - eval_square(state, GameSide::black);
    middle += square.middle;
    end += square.end;

    Int pawn_structure = eval_pawn_structure(state, GameSide::white) - eval_pawn_structure(state, GameSide::black);
    middle += pawn_structure;
    end += pawn_structure;

    ValuePair mobility = eval_mobility(state, GameSide::white) - eval_mobility(state, GameSide::black);
    middle += mobility.middle;
    end += mobility.end;

    ValuePair theme = eval_theme(state, GameSide::white) - eval_theme(state, GameSide::black);
    middle += theme.middle;
    end += theme.end;

    Int blockage = eval_blockage(state, GameSide::white) - eval_blockage(state, GameSide::black);
    middle += blockage;
    end += blockage;

    Int attack = eval_attack(state, GameSide::white) - eval_attack(state, GameSide::black);
    middle += attack;
    end += attack;

    ValuePair safety = eval_safety(state, GameSide::white) - eval_safety(state, GameSide::black);
    middle += safety.middle;
    end += safety.end;

    Int tempo = state->current_side == GameSide::white ? 10 : -10;
    middle += tempo;
    end += tempo;

    Int phase = 0;
    for (GameSideEnum side = 0; side < GameSide::count; side++)
    {
        for (GamePieceTypeEnum piece_type = GamePieceType::knight; piece_type <= GamePieceType::queen; piece_type++)
        {
            BitBoard occupancy = get_occupancy(state, side, piece_type);
            Int count = bit_count(occupancy);
            phase += count * phase_values[piece_type];
        }
    }
    phase = MIN(phase, 24);

    Int value = (middle * phase + end * (24 - phase)) / 24;
    GameSideEnum strong = value > 0 ? GameSide::white : GameSide::black;
    GameSideEnum weak = oppose(strong);
    if (materials[strong].pawn == 0)
    {
        if (materials[strong].non_pawn < 400)
        {
            value = 0;
        }
        else if (materials[weak].pawn == 0 && materials[strong].non_pawn == 2 * material_values[GamePieceType::knight])
        {
            value = 0;
        }
        else if (materials[strong].non_pawn == material_values[GamePieceType::rook] && materials[weak].non_pawn == material_values[GamePieceType::knight])
        {
            value /= 2;
        }
        else if (materials[strong].non_pawn == material_values[GamePieceType::rook] && materials[weak].non_pawn == material_values[GamePieceType::bishop])
        {
            value /= 2;
        }
        else if (materials[strong].non_pawn == material_values[GamePieceType::rook] + material_values[GamePieceType::knight] && materials[weak].non_pawn == material_values[GamePieceType::rook])
        {
            value /= 2;
        }
        else if (materials[strong].non_pawn == material_values[GamePieceType::rook] + material_values[GamePieceType::bishop] && materials[weak].non_pawn == material_values[GamePieceType::rook])
        {
            value /= 2;
        }
    }

    if (side == GameSide::black)
    {
        value = -value;
    }
    return value;
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
