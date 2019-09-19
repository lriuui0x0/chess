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

struct PawnStateType
{
};

struct PawnState
{
    Bool moved;
    GamePieceType promoted_type;
};

struct GamePiece
{
    GameSide side;
    GamePieceType type;
    Int index;
    Str name;
    Int row;
    Int column;
    PawnState pawn_state;
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
    GamePiece *piece = &state.pieces[0];

    *piece = GamePiece{GameSide::white, GamePieceType::rook, 0, str("white rook 0"), 0, 0};
    state.board[piece->row][piece->column] = piece;
    piece++;

    *piece = GamePiece{GameSide::white, GamePieceType::knight, 0, str("white knight 0"), 0, 1};
    state.board[piece->row][piece->column] = piece;
    piece++;

    *piece = GamePiece{GameSide::white, GamePieceType::bishop, 0, str("white bishop 0"), 0, 2};
    state.board[piece->row][piece->column] = piece;
    piece++;

    *piece = GamePiece{GameSide::white, GamePieceType::queen, 0, str("white queen"), 0, 3};
    state.board[piece->row][piece->column] = piece;
    piece++;

    *piece = GamePiece{GameSide::white, GamePieceType::king, 0, str("white king"), 0, 4};
    state.board[piece->row][piece->column] = piece;
    piece++;

    *piece = GamePiece{GameSide::white, GamePieceType::bishop, 1, str("white bishop 1"), 0, 5};
    state.board[piece->row][piece->column] = piece;
    piece++;

    *piece = GamePiece{GameSide::white, GamePieceType::knight, 1, str("white knight 1"), 0, 6};
    state.board[piece->row][piece->column] = piece;
    piece++;

    *piece = GamePiece{GameSide::white, GamePieceType::rook, 1, str("white rook 1"), 0, 7};
    state.board[piece->row][piece->column] = piece;
    piece++;

    for (Int pawn_i = 0; pawn_i < 8; pawn_i++)
    {
        UInt8 number_suffix_data[2];
        number_suffix_data[0] = '0' + pawn_i;
        number_suffix_data[1] = '\0';
        Str number_suffix;
        number_suffix.count = 1;
        number_suffix.data = number_suffix_data;

        *piece = GamePiece{GameSide::white, GamePieceType::pawn, pawn_i, concat_str(str("white pawn "), number_suffix), 1, pawn_i};
        state.board[piece->row][piece->column] = piece;
        piece++;
    }

    *piece = GamePiece{GameSide::black, GamePieceType::rook, 0, str("black rook 0"), 7, 0};
    state.board[piece->row][piece->column] = piece;
    piece++;

    *piece = GamePiece{GameSide::black, GamePieceType::knight, 0, str("black knight 0"), 7, 1};
    state.board[piece->row][piece->column] = piece;
    piece++;

    *piece = GamePiece{GameSide::black, GamePieceType::bishop, 0, str("black bishop 0"), 7, 2};
    state.board[piece->row][piece->column] = piece;
    piece++;

    *piece = GamePiece{GameSide::black, GamePieceType::queen, 0, str("black queen"), 7, 3};
    state.board[piece->row][piece->column] = piece;
    piece++;

    *piece = GamePiece{GameSide::black, GamePieceType::king, 0, str("black king"), 7, 4};
    state.board[piece->row][piece->column] = piece;
    piece++;

    *piece = GamePiece{GameSide::black, GamePieceType::bishop, 1, str("black bishop 1"), 7, 5};
    state.board[piece->row][piece->column] = piece;
    piece++;

    *piece = GamePiece{GameSide::black, GamePieceType::knight, 1, str("black knight 1"), 7, 6};
    state.board[piece->row][piece->column] = piece;
    piece++;

    *piece = GamePiece{GameSide::black, GamePieceType::rook, 1, str("black rook 1"), 7, 7};
    state.board[piece->row][piece->column] = piece;
    piece++;

    for (Int pawn_i = 0; pawn_i < 8; pawn_i++)
    {
        UInt8 number_suffix_data[2];
        number_suffix_data[0] = '0' + pawn_i;
        number_suffix_data[1] = '\0';
        Str number_suffix;
        number_suffix.count = 1;
        number_suffix.data = number_suffix_data;

        *piece = GamePiece{GameSide::black, GamePieceType::pawn, pawn_i, concat_str(str("black pawn "), number_suffix), 6, pawn_i};
        state.board[piece->row][piece->column] = piece;
        piece++;
    }

    return state;
}

Bool is_move_orthogonal(Int row_from, Int column_from, Int row_to, Int column_to)
{
    return row_to == row_from || column_to == column_from;
}

Bool is_move_diagonal(Int row_from, Int column_from, Int row_to, Int column_to)
{
    return ABS(row_to - row_from) == ABS(column_to - column_from);
}

Bool check_game_move(GamePiece *piece, Int row_to, Int column_to)
{
    switch (piece->type)
    {
    case GamePieceType::rook:
    {
        if (is_move_orthogonal(piece->row, piece->column, row_to, column_to))
        {
            return true;
        }
    }
    break;

    case GamePieceType::knight:
    {
        Int row_change = ABS(row_to - piece->row);
        Int column_change = ABS(column_to - piece->column);
        return row_change == 1 && column_change == 2 || row_change == 2 && column_change == 1;
    }
    break;

    case GamePieceType::bishop:
    {
        if (is_move_diagonal(piece->row, piece->column, row_to, column_to))
        {
            return true;
        }
    }
    break;

    case GamePieceType::queen:
    {
        if (is_move_orthogonal(piece->row, piece->column, row_to, column_to) ||
            is_move_diagonal(piece->row, piece->column, row_to, column_to))
        {
            return true;
        }
    }
    break;

    case GamePieceType::king:
    {
        Int row_change = ABS(row_to - piece->row);
        Int column_change = ABS(column_to - piece->column);
        return MAX(row_change, column_change) == 1;
    }
    break;

    case GamePieceType::pawn:
    {
        Int direction = piece->side == GameSide::white ? 1 : -1;
        Int row_change = row_to - piece->row;
        Int column_change = column_to - piece->column;
        return row_change == direction && column_change == 0;
    }
    break;
    }

    return false;
}
