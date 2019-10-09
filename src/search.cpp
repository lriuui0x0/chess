#pragma once

#include "game.cpp"

Void generate_all_moves(GameState *state, Buffer<GameMove> *moves)
{
    BitBoard all_pieces = state->occupancy_side[state->current_side];
    while (all_pieces)
    {
        Square square_from = first_set(all_pieces);
        all_pieces -= bit_square(square_from);

        BitBoard all_moves = check_game_move(state, square_from);
        while (all_moves)
        {
            Square square_to = first_set(all_moves);
            all_moves -= bit_square(square_to);

            GameMove move = get_game_move(state, square_from, square_to);
            if (is_move_legal(state, move))
            {
                moves->data[moves->count++] = move;
            }
        }
    }
}

struct ValuedMove
{
    GameMove move;
    Int value;
};

#define VALUE_INF (1000000000)

Int evaluate_material(GameState *state, GameSideEnum side, Bool middle_game, Bool include_pawns)
{
    Int middle_game_values[GamePieceType::count - 1] = {128, 782, 830, 1289, 2529};
    Int end_game_values[GamePieceType::count - 1] = {213, 865, 918, 1378, 2687};
    Int *piece_values = middle_game ? middle_game_values : end_game_values;

    Int value = 0;
    GamePieceTypeEnum start_piece = include_pawns ? GamePieceType::pawn : GamePieceType::knight;
    for (GamePieceTypeEnum piece_type = start_piece; piece_type < GamePieceType::king; piece_type++)
    {
        BitBoard occupancy = get_occupancy(state, side, piece_type);
        while (occupancy)
        {
            occupancy = occupancy & (occupancy - 1);
            value += piece_values[piece_type];
        }
    }
    return value;
}

Int evalute_piece_square(GameState *state, GameSideEnum side, Bool middle_game)
{
    Int middle_game_values[GamePieceType::count - 1][8][4] = {
        {{-169, -96, -80, -79}, {-79, -39, -24, -9}, {-64, -20, 4, 19}, {-28, 5, 41, 47}, {-29, 13, 42, 52}, {-11, 28, 63, 55}, {-67, -21, 6, 37}, {-200, -80, -53, -32}},
        {{-44, -4, -11, -28}, {-18, 7, 14, 3}, {-8, 24, -3, 15}, {1, 8, 26, 37}, {-7, 30, 23, 28}, {-17, 4, -1, 8}, {-21, -19, 10, -6}, {-48, -3, -12, -25}},
        {{-31, -20, -14, -5}, {-21, -13, -8, 6}, {-25, -11, -1, 3}, {-13, -5, -4, -6}, {-27, -15, -4, 3}, {-22, -2, 6, 12}, {-2, 12, 16, 18}, {-17, -19, -1, 9}},
        {{3, -5, -5, 4}, {-3, 5, 8, 12}, {-3, 6, 13, 7}, {4, 5, 9, 8}, {0, 14, 12, 5}, {-4, 10, 6, 8}, {-5, 6, 10, 8}, {-2, -2, 1, -2}},
        {{272, 325, 273, 190}, {277, 305, 241, 183}, {198, 253, 168, 120}, {169, 191, 136, 108}, {145, 176, 112, 69}, {122, 159, 85, 36}, {87, 120, 64, 25}, {64, 87, 49, 0}},
    };
    Int end_game_values[GamePieceType::count - 1][8][4] = {
        {{-105, -74, -46, -18}, {-70, -56, -15, 6}, {-38, -33, -5, 27}, {-36, 0, 13, 34}, {-41, -20, 4, 35}, {-51, -38, -17, 19}, {-64, -45, -37, 16}, {-98, -89, -53, -16}},
        {{-63, -30, -35, -8}, {-38, -13, -14, 0}, {-18, 0, -7, 13}, {-26, -3, 1, 16}, {-24, -6, -10, 17}, {-26, 2, 1, 16}, {-34, -18, -7, 9}, {-51, -40, -39, -20}},
        {{-9, -13, -10, -9}, {-12, -9, -1, -2}, {6, -8, -2, -6}, {-6, 1, -9, 7}, {-5, 8, 7, -6}, {6, 1, -7, 10}, {4, 5, 20, -5}, {18, 0, 19, 13}},
        {{-69, -57, -47, -26}, {-55, -31, -22, -4}, {-39, -18, -9, 3}, {-23, -3, 13, 24}, {-29, -6, 9, 21}, {-38, -18, -12, 1}, {-50, -27, -24, -8}, {-75, -52, -43, -36}},
        {{0, 41, 80, 93}, {57, 98, 138, 131}, {86, 138, 165, 173}, {103, 152, 168, 169}, {98, 166, 197, 194}, {87, 164, 174, 189}, {40, 99, 128, 141}, {5, 60, 75, 75}},
    };
    Int(*piece_square_values)[GamePieceType::count - 1][8][4] = middle_game ? &middle_game_values : &end_game_values;

    Int pawn_middle_game_values[8][8] = {
        {0, 0, 0, 0, 0, 0, 0, 0},
        {3, 3, 10, 19, 16, 19, 7, -5},
        {-9, -15, 11, 15, 32, 22, 5, -22},
        {-8, -23, 6, 20, 40, 17, 4, -12},
        {13, 0, -13, 1, 11, -2, -13, 5},
        {-5, -12, -7, 22, -8, -5, -15, -18},
        {-7, 7, -3, -13, 5, -16, 10, -8},
        {0, 0, 0, 0, 0, 0, 0, 0},
    };
    Int pawn_end_game_values[8][8] = {
        {0, 0, 0, 0, 0, 0, 0, 0},
        {-10, -6, 10, 0, 14, 7, -5, -19},
        {-10, -10, -10, 4, 4, 3, -6, -4},
        {6, -2, -8, -4, -13, -12, -10, -9},
        {9, 4, 3, -12, -12, -6, 13, 8},
        {28, 20, 21, 28, 30, 7, 6, 13},
        {0, -11, 12, 21, 25, 19, 4, 7},
        {0, 0, 0, 0, 0, 0, 0, 0},
    };
    Int(*pawn_square_values)[8][8] = middle_game ? &pawn_middle_game_values : &pawn_end_game_values;

    Int value = 0;
    for (GamePieceTypeEnum piece_type = 0; piece_type < GamePieceType::count; piece_type++)
    {
        BitBoard occupancy = get_occupancy(state, side, piece_type);
        while (occupancy)
        {
            BitBoard bit_square = occupancy & -occupancy;
            occupancy -= bit_square;
            Square square = first_set(bit_square);
            Int row = get_row(square);
            Int column = get_column(square);
            if (side == GameSide::black)
            {
                row = 7 - row;
                column = 7 - column;
            }

            if (piece_type == GamePieceType::pawn)
            {
                value += (*pawn_square_values)[row][column];
            }
            else
            {
                value += (*piece_square_values)[piece_type - 1][row][MIN(column, 7 - column)];
            }
        }
    }
    return value;
}

Int evalute_imbalance(GameState *state, GameSideEnum side)
{
    Int friend_value[GamePieceType::count][GamePieceType::count] = {
        {38},
        {255, -62},
        {104, 4, 0},
        {-2, 47, 105, -208},
        {24, 117, 133, -134, -6},
        {40, 32, 0, -26, -189, 1438},
    };

    Int foe_value[GamePieceType::count][GamePieceType::count] = {
        {0},
        {63, 0},
        {65, 42, 0},
        {39, 24, -24, 0},
        {100, -42, 137, 268, 0},
        {36, 9, 59, 46, 97, 0},
    };

    Int piece_count[GameSide::count][GamePieceType::count];
    for (GameSideEnum side = 0; side < GameSide::count; side++)
    {
        for (GamePieceTypeEnum piece_type = 0; piece_type < GamePieceType::king; piece_type++)
        {
            piece_count[side][piece_type] = bit_count(get_occupancy(state, side, piece_type));
        }
        // NOTE: Use king as bishop pair
        piece_count[side][GamePieceType::king] = piece_count[side][GamePieceType::bishop] > 1 ? 1 : 0;
    }

    Int value = 0;
    for (GamePieceTypeEnum piece_type = 0; piece_type < GamePieceType::count; piece_type++)
    {
        if (piece_count[side][piece_type])
        {
            Int value_factor = 0;
            for (GamePieceTypeEnum piece_type2 = 0; piece_type2 <= piece_type; piece_type2++)
            {
                value_factor += friend_value[piece_type][piece_type2] * piece_count[side][piece_type2];
                value_factor += foe_value[piece_type][piece_type2] * piece_count[oppose(side)][piece_type2];
            }
            value += piece_count[side][piece_type] * value_factor;
        }
    }

    return value;
}

Int evalute_pawn(GameState *state, GameSideEnum side, Bool middle_game)
{
    GameSideEnum oppose_side = oppose(side);
    BitBoard pawn_occupancy = get_occupancy(state, side, GamePieceType::pawn);
    BitBoard oppose_pawn_occupancy = get_occupancy(state, oppose_side, GamePieceType::pawn);

    Int value = 0;
    BitBoard pawn_occupancy_copy = pawn_occupancy;
    while (pawn_occupancy_copy)
    {
        BitBoard pawn_bit = pawn_occupancy_copy & -pawn_occupancy_copy;
        pawn_occupancy_copy -= pawn_bit;
        Square pawn_square = first_set(pawn_bit);
        Int pawn_row = get_row(pawn_square);
        Int pawn_column = get_column(pawn_square);

        BitBoard adj_columns = (pawn_column > 0 ? column_mask(pawn_column - 1) : 0) | (pawn_column < 7 ? column_mask(pawn_column + 1) : 0);
        Bool isolated = !(adj_columns & pawn_occupancy);
        Bool backward = !(forward_mask(pawn_row, oppose_side) & adj_columns & pawn_occupancy) &&
                        ((up(pawn_bit, side) | up(up_left(pawn_bit, side), side) | up(up_right(pawn_bit, side), side)) & oppose_pawn_occupancy);
        BitBoard support = (down_left(pawn_bit, side) | down_right(pawn_bit, side)) & pawn_occupancy;
        Bool doubled = !support && down(pawn_bit, pawn_occupancy);
        Bool phalanx = (left(pawn_bit, side) | right(pawn_bit, side)) & pawn_occupancy;
        Bool connected = support || phalanx;
        Bool unopposed = !(forward_mask(pawn_row, side) & column_mask(pawn_column) & oppose_pawn_occupancy);

        Int isolated_value = middle_game ? 5 : 15;
        Int backward_value = middle_game ? 9 : 24;
        Int doubled_value = middle_game ? 11 : 56;
        Int unopposed_value = middle_game ? 13 : 27;
        Int connected_values[8] = {0, 7, 8, 12, 29, 48, 86, 0};
        if (isolated)
        {
            value -= isolated * isolated_value + unopposed * unopposed_value;
        }
        if (backward)
        {
            value -= backward * backward_value + unopposed * unopposed_value;
        }
        if (doubled)
        {
            value -= doubled * doubled_value;
        }
        if (connected)
        {
            pawn_row = MIN(pawn_row, 7 - pawn_row);
            Int connected_value = connected_values[pawn_row] * (2 + phalanx + unopposed) + 21 * bit_count(support);
            value += middle_game ? connected_value : connected_value * (pawn_row - 2) / 4;
        }
    }
    return value;
}

Int evaluate_phase(GameState *state)
{
    Int middle_game_limit = 15258;
    Int end_game_limit = 3915;
    Int material_value = evaluate_material(state, GameSide::white, true, false) + evaluate_material(state, GameSide::black, true, false);
    material_value = MAX(end_game_limit, MIN(material_value, middle_game_limit));
    Int value = (material_value - end_game_limit) * 128 / (middle_game_limit - end_game_limit);
    return value;
}

Int evaluate(GameState *state)
{
    GameSideEnum side = state->current_side;
    GameSideEnum oppose_side = oppose(side);
    Int imbalance_value = evalute_imbalance(state, side);
    Int value_middle_game = 0;
    value_middle_game += evaluate_material(state, side, true, true) - evaluate_material(state, oppose_side, true, true);
    value_middle_game += evalute_piece_square(state, side, true) - evalute_piece_square(state, oppose_side, true);
    value_middle_game += imbalance_value;
    value_middle_game += evalute_pawn(state, side, true);

    Int value_end_game = 0;
    value_end_game += evaluate_material(state, side, false, true) - evaluate_material(state, oppose_side, false, true);
    value_end_game += evalute_piece_square(state, side, false) - evalute_piece_square(state, oppose_side, false);
    value_end_game += imbalance_value;
    value_end_game += evalute_pawn(state, side, false);

    Int phase = evaluate_phase(state);
    Int value = (value_middle_game * phase + value_end_game * (128 - phase)) / 128;
    return value;
}

ValuedMove negamax(GameState *state, Int alpha, Int beta, Int depth)
{
    if (depth >= 4)
    {
        ValuedMove valued_move;
        valued_move.value = evaluate(state);
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
            best_move = valued_move;
        }

        alpha = MAX(alpha, best_move.value);
        if (alpha >= beta)
        {
            break;
        }
    }
    return best_move;
}
