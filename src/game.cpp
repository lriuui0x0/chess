#pragma once
#include "../lib/util.hpp"

typedef UInt64 BitBoard;

struct SimplePieceTable
{
    BitBoard move[64];
};

struct SlidingPieceTable
{
    BitBoard blocker_mask[64];
    UInt64 blocker_bit_count[64];
    UInt64 magic[64];
    BitBoard move[64][1 << 13];
};

struct BitBoardTable
{
    SlidingPieceTable rook_table;
    SimplePieceTable knight_table;
    SlidingPieceTable bishop_table;
    SimplePieceTable king_table;
};

namespace GamePieceType
{
enum
{
    rook = 1,
    knight,
    bishop,
    queen,
    king,
    pawn,
};
}
typedef Int GamePieceTypeEnum;

namespace GameSide
{
enum
{
    white,
    black,
};
}
typedef Int GameSideEnum;

GameSideEnum oppose(GameSideEnum side)
{
    return side ^ 1;
}

typedef UInt8 GamePiece;
// #define PIECE_COUNT (16)

GamePiece get_piece(GamePieceTypeEnum type, GameSideEnum side)
{
    GamePiece result = type | (side << 3);
    return result;
}

GamePieceTypeEnum get_piece_type(GamePiece piece)
{
    GamePieceTypeEnum result = piece & (0x8);
    return result;
}

GameSideEnum get_side(GamePiece piece)
{
    GameSideEnum result = piece >> 3;
    return result;
}

struct GamePosition
{
    GamePiece board[64];
    BufferI<Int, 10> pieces[PIECE_COUNT];
    BitBoard side_occupancy[2];
};

struct GameState
{
    GamePosition position;
    GameSideEnum player_side;
    GameSideEnum current_side;
};

Int get_row(Int square)
{
    ASSERT(square >= 0 && square < 64);
    return square / 8;
}

Int get_column(Int square)
{
    ASSERT(square >= 0 && square < 64);
    return square % 8;
}

Int get_square(Int row, Int column)
{
    ASSERT(row >= 0 && row < 8);
    ASSERT(column >= 0 && column < 8);
    return row * 8 + column;
}

Void add_piece(GameState *state, Int row, Int column, GameSideEnum side, GamePieceTypeEnum piece_type)
{
    Int square = get_square(row, column);
    GamePiece piece = get_piece(side, piece_type);
    state->position.board[square] = piece;
    state->position.pieces[piece].data[state->position.pieces[piece].count++] = square;
    state->position.side_occupancy[side] |= 1llu << square;
}

GameState get_initial_game_state()
{
    GameState state = {};
    state.player_side = GameSide::white;
    state.current_side = GameSide::white;

    add_piece(&state, 0, 0, GameSide::white, GamePieceType::rook);
    add_piece(&state, 0, 1, GameSide::white, GamePieceType::knight);
    add_piece(&state, 0, 2, GameSide::white, GamePieceType::bishop);
    add_piece(&state, 0, 3, GameSide::white, GamePieceType::queen);
    add_piece(&state, 0, 4, GameSide::white, GamePieceType::king);
    add_piece(&state, 0, 5, GameSide::white, GamePieceType::bishop);
    add_piece(&state, 0, 6, GameSide::white, GamePieceType::knight);
    add_piece(&state, 0, 7, GameSide::white, GamePieceType::rook);
    for (Int pawn_i = 0; pawn_i < 8; pawn_i++)
    {
        add_piece(&state, 1, pawn_i, GameSide::white, GamePieceType::pawn);
    }

    add_piece(&state, 7, 0, GameSide::black, GamePieceType::rook);
    add_piece(&state, 7, 1, GameSide::black, GamePieceType::knight);
    add_piece(&state, 7, 2, GameSide::black, GamePieceType::bishop);
    add_piece(&state, 7, 3, GameSide::black, GamePieceType::queen);
    add_piece(&state, 7, 4, GameSide::black, GamePieceType::king);
    add_piece(&state, 7, 5, GameSide::black, GamePieceType::bishop);
    add_piece(&state, 7, 6, GameSide::black, GamePieceType::knight);
    add_piece(&state, 7, 7, GameSide::black, GamePieceType::rook);
    for (Int pawn_i = 0; pawn_i < 8; pawn_i++)
    {
        add_piece(&state, 6, pawn_i, GameSide::black, GamePieceType::pawn);
    }

    return state;
}

BitBoard get_sliding_piece_move(GameState *state, BitBoard square, GameSideEnum side, SlidingPieceTable *sliding_piece_table)
{
    BitBoard occupancy = state->position.side_occupancy[0] | state->position.side_occupancy[1];
    BitBoard blocker_mask = sliding_piece_table->blocker_mask[square];
    BitBoard blocker = blocker_mask & occupancy;
    UInt64 hash = blocker * sliding_piece_table->magic[square];
    BitBoard move = sliding_piece_table->move[square][hash] & ~state->position.side_occupancy[side];
    return move;
}

BitBoard get_simple_piece_move(GameState *state, BitBoard square, GameSideEnum side, SimplePieceTable *simple_piece_table)
{
    BitBoard move = simple_piece_table->move[square] & ~state->position.side_occupancy[side];
    return move;
}

BitBoard get_pawn_move(GameState *state, BitBoard square, GameSideEnum side)
{
    BitBoard occupancy = state->position.side_occupancy[0] | state->position.side_occupancy[1];
    if (side == GameSide::white)
    {
        BitBoard move = 0;
        move |= (square << 8) & ~occupancy;

        BitBoard start_squares = ~(((UInt64)-1) << 8) << 8;
        if ((square & start_squares) && move)
        {
            move |= (square << 16) & ~occupancy;
        }
        return move;
    }
    else if (side == GameSide::black)
    {
        BitBoard move = 0;
        move |= (square >> 8) & ~occupancy;

        BitBoard start_squares = ~(((UInt64)-1) >> 8) >> 8;
        if ((square & start_squares) && move)
        {
            move |= (square >> 16) & ~occupancy;
        }
        return move;
    }
    else
    {
        ASSERT(false);
        return 0;
    }
}

BitBoard get_pawn_capture(GameState *state, BitBoard square, GameSideEnum side)
{
    BitBoard left_side_mask = 0xfefefefefefefefe;
    BitBoard right_side_mask = 0x7f7f7f7f7f7f7f7f;
    if (side == GameSide::white)
    {
        BitBoard move = 0;
        move |= (square << 7) & state->position.side_occupancy[oppose(side)] & left_side_mask;
        move |= (square << 9) & state->position.side_occupancy[oppose(side)] & right_side_mask;
        return move;
    }
    else if (side == GameSide::black)
    {
        BitBoard move = 0;
        move |= (square >> 7) & state->position.side_occupancy[oppose(side)] & right_side_mask;
        move |= (square >> 9) & state->position.side_occupancy[oppose(side)] & left_side_mask;
        return move;
    }
    else
    {
        ASSERT(false);
        return 0;
    }
}

BitBoard check_game_move(GameState *state, BitBoard square, BitBoardTable *bit_board_table)
{
    BitBoard occupancy = state->position.side_occupancy[0] | state->position.side_occupancy[1];
    ASSERT(bit_count(square) == 1 && (occupancy & square));

    GamePiece piece = state->position.board[square];
    GamePieceTypeEnum piece_type = get_piece_type(piece);
    GameSideEnum side = get_side(piece);

    switch (piece_type)
    {
    case GamePieceType::rook:
    {
        BitBoard move = get_sliding_piece_move(state, square, side, &bit_board_table->rook_table);
        return move;
    }
    break;

    case GamePieceType::knight:
    {
        BitBoard move = get_simple_piece_move(state, square, side, &bit_board_table->knight_table);
        return move;
    }
    break;

    case GamePieceType::bishop:
    {
        BitBoard move = get_sliding_piece_move(state, square, side, &bit_board_table->bishop_table);
        return move;
    }
    break;

    case GamePieceType::queen:
    {
        BitBoard rook_move = get_sliding_piece_move(state, square, side, &bit_board_table->rook_table);
        BitBoard bishop_move = get_sliding_piece_move(state, square, side, &bit_board_table->bishop_table);
        BitBoard move = rook_move | bishop_move;
        return move;
    }
    break;

    case GamePieceType::king:
    {
        BitBoard move = get_simple_piece_move(state, square, side, &bit_board_table->king_table);
        return move;
    }
    break;

    case GamePieceType::pawn:
    {
        BitBoard move = get_pawn_move(state, square, side);
        move |= get_pawn_capture(state, square, side);
        return move;
    }
    break;

    default:
    {
        ASSERT(false);
        return 0;
    }
    break;
    }
}

// Void move_game_piece(GameState *state, Int row_from, Int column_from, Int row_to, Int column_to)
// {
//     GamePiece *piece = state->position[row_from][column_from];
//     state->position[row_from][column_from] = null;
//     state->position[row_to][column_to] = piece;
//     piece->row = row_to;
//     piece->column = column_to;
//     piece->moves++;
// }

// Void remove_game_piece(GameState *state, Int row, Int column)
// {
//     state->position[row][column] = null;
// }

// Void record_game_move(GameState *state, GameMoveType move_type, Int row_from, Int column_from, Int row_to, Int column_to)
// {
//     GameMove *move = state->history.push();
//     move->type = move_type;
//     move->piece = state->board[row_from][column_from];
//     move->row_from = row_from;
//     move->column_from = column_from;
//     move->row_to = row_to;
//     move->column_to = column_to;

//     move_game_piece(state, row_from, column_from, row_to, column_to);
//     if (move_type == GameMoveType::castling_king)
//     {
//         move_game_piece(state, row_from, 8 - 1, row_to, column_to - 1);
//     }
//     else if (move_type == GameMoveType::castling_queen)
//     {
//         move_game_piece(state, row_from, 0, row_to, column_to + 1);
//     }
//     else if (move_type == GameMoveType::en_passant)
//     {
//         remove_game_piece(state, row_from, column_to);
//     }
// }
