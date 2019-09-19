#pragma once

#include "../lib/util.hpp"

enum struct GamePieceType
{
    rook,
    knight,
    bishop,
    queen,
    king,
    pawn,
};

enum struct GameSide
{
    white,
    black,
};

struct GamePiece
{
    GameSide side;
    GamePieceType type;
    Int index;
    Str name;
};

#define BOARD_SQUARE_COUNT (64)
#define BOARD_ROW_COUNT (8)
#define BOARD_COLUMN_COUNT (8)
#define PIECE_COUNT (32)

struct GameState
{
    GamePiece pieces[PIECE_COUNT];
    GamePiece *board[BOARD_ROW_COUNT][BOARD_COLUMN_COUNT];
};

Int get_row(Int square_index)
{
    ASSERT(square_index >= 0 && square_index < BOARD_SQUARE_COUNT);
    return square_index / 8;
}

Int get_column(Int square_index)
{
    ASSERT(square_index >= 0 && square_index < BOARD_SQUARE_COUNT);
    return square_index % 8;
}

Int get_square(Int row, Int column)
{
    ASSERT(row >= 0 && row < BOARD_ROW_COUNT);
    ASSERT(column >= 0 && column < BOARD_COLUMN_COUNT);
    return row * 8 + column;
}

GameState get_initial_game_state()
{
    GameState state = {};
    Int piece_i = 0;

    state.pieces[piece_i] = GamePiece{GameSide::white, GamePieceType::rook, 0, str("white rook 0")};
    state.board[0][0] = &state.pieces[piece_i++];

    state.pieces[piece_i] = GamePiece{GameSide::white, GamePieceType::knight, 0, str("white knight 0")};
    state.board[0][1] = &state.pieces[piece_i++];

    state.pieces[piece_i] = GamePiece{GameSide::white, GamePieceType::bishop, 0, str("white bishop 0")};
    state.board[0][2] = &state.pieces[piece_i++];

    state.pieces[piece_i] = GamePiece{GameSide::white, GamePieceType::queen, 0, str("white queen")};
    state.board[0][3] = &state.pieces[piece_i++];

    state.pieces[piece_i] = GamePiece{GameSide::white, GamePieceType::king, 0, str("white king")};
    state.board[0][4] = &state.pieces[piece_i++];

    state.pieces[piece_i] = GamePiece{GameSide::white, GamePieceType::bishop, 1, str("white bishop 1")};
    state.board[0][5] = &state.pieces[piece_i++];

    state.pieces[piece_i] = GamePiece{GameSide::white, GamePieceType::knight, 1, str("white knight 1")};
    state.board[0][6] = &state.pieces[piece_i++];

    state.pieces[piece_i] = GamePiece{GameSide::white, GamePieceType::rook, 1, str("white rook 1")};
    state.board[0][7] = &state.pieces[piece_i++];

    for (Int pawn_i = 0; pawn_i < 8; pawn_i++)
    {
        UInt8 number_suffix_data[2];
        number_suffix_data[0] = '0' + pawn_i;
        number_suffix_data[1] = '\0';
        Str number_suffix;
        number_suffix.count = 1;
        number_suffix.data = number_suffix_data;

        state.pieces[piece_i] = GamePiece{GameSide::white, GamePieceType::pawn, pawn_i, concat_str(str("white pawn "), number_suffix)};
        state.board[1][pawn_i] = &state.pieces[piece_i++];
    }

    state.pieces[piece_i] = GamePiece{GameSide::black, GamePieceType::rook, 0, str("black rook 0")};
    state.board[7][0] = &state.pieces[piece_i++];

    state.pieces[piece_i] = GamePiece{GameSide::black, GamePieceType::knight, 0, str("black knight 0")};
    state.board[7][1] = &state.pieces[piece_i++];

    state.pieces[piece_i] = GamePiece{GameSide::black, GamePieceType::bishop, 0, str("black bishop 0")};
    state.board[7][2] = &state.pieces[piece_i++];

    state.pieces[piece_i] = GamePiece{GameSide::black, GamePieceType::queen, 0, str("black queen")};
    state.board[7][3] = &state.pieces[piece_i++];

    state.pieces[piece_i] = GamePiece{GameSide::black, GamePieceType::king, 0, str("black king")};
    state.board[7][4] = &state.pieces[piece_i++];

    state.pieces[piece_i] = GamePiece{GameSide::black, GamePieceType::bishop, 1, str("black bishop 1")};
    state.board[7][5] = &state.pieces[piece_i++];

    state.pieces[piece_i] = GamePiece{GameSide::black, GamePieceType::knight, 1, str("black knight 1")};
    state.board[7][6] = &state.pieces[piece_i++];

    state.pieces[piece_i] = GamePiece{GameSide::black, GamePieceType::rook, 1, str("black rook 1")};
    state.board[7][7] = &state.pieces[piece_i++];

    for (Int pawn_i = 0; pawn_i < 8; pawn_i++)
    {
        UInt8 number_suffix_data[2];
        number_suffix_data[0] = '0' + pawn_i;
        number_suffix_data[1] = '\0';
        Str number_suffix;
        number_suffix.count = 1;
        number_suffix.data = number_suffix_data;

        state.pieces[piece_i] = GamePiece{GameSide::black, GamePieceType::pawn, pawn_i, concat_str(str("black pawn "), number_suffix)};
        state.board[6][pawn_i] = &state.pieces[piece_i++];
    }

    return state;
}
