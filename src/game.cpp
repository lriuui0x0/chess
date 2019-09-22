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

struct PawnState
{
    Bool moved;
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
    GameSide player_side;
    GameSide current_side;
    Int selected_piece_index;
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
    state.player_side = GameSide::white;
    state.current_side = GameSide::white;
    state.selected_piece_index = -1;

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
        piece->pawn_state.moved = false;
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
        piece->pawn_state.moved = false;
        state.board[piece->row][piece->column] = piece;
        piece++;
    }

    return state;
}

Bool is_occupied(GameState *state, Int row, Int column)
{
    return state->board[row][column] != null;
}

Bool check_orthogonal_move(GameState *state, GamePiece *piece, Int row_to, Int column_to)
{
    ASSERT(row_to != piece->row || column_to != piece->column);

    if (column_to == piece->column)
    {
        if (row_to > piece->row)
        {
            for (Int row = piece->row + 1; row <= row_to; row++)
            {
                if (is_occupied(state, row, piece->column))
                {
                    return false;
                }
            }
        }
        else
        {
            for (Int row = piece->row - 1; row >= row_to; row--)
            {
                if (is_occupied(state, row, piece->column))
                {
                    return false;
                }
            }
        }

        return true;
    }
    else if (row_to == piece->row)
    {
        if (column_to > piece->column)
        {
            for (Int column = piece->column + 1; column <= column_to; column++)
            {
                if (is_occupied(state, piece->row, column))
                {
                    return false;
                }
            }
        }
        else
        {
            for (Int column = piece->column - 1; column >= column_to; column--)
            {
                if (is_occupied(state, piece->row, column))
                {
                    return false;
                }
            }
        }

        return true;
    }

    return false;
}

Bool check_diagonal_move(GameState *state, GamePiece *piece, Int row_to, Int column_to)
{
    ASSERT(row_to != piece->row || column_to != piece->column);

    if (row_to - piece->row == column_to - piece->column)
    {
        if (row_to > piece->row)
        {
            for (Int row = piece->row + 1, column = piece->column + 1; row <= row_to; row++, column++)
            {
                if (is_occupied(state, row, column))
                {
                    return false;
                }
            }
        }
        else
        {
            for (Int row = piece->row - 1, column = piece->column - 1; row >= row_to; row--, column--)
            {
                if (is_occupied(state, row, column))
                {
                    return false;
                }
            }
        }

        return true;
    }
    else if (row_to - piece->row == -(column_to - piece->column))
    {
        if (row_to > piece->row)
        {
            for (Int row = piece->row + 1, column = piece->column - 1; row <= row_to; row++, column--)
            {
                if (is_occupied(state, row, column))
                {
                    return false;
                }
            }
        }
        else
        {
            for (Int row = piece->row - 1, column = piece->column + 1; row >= row_to; row--, column++)
            {
                if (is_occupied(state, row, column))
                {
                    return false;
                }
            }
        }

        return true;
    }

    return false;
}

Bool check_game_move(GameState *state, GamePiece *piece, Int row_to, Int column_to)
{
    if (row_to == piece->row && column_to == piece->column)
    {
        return false;
    }

    switch (piece->type)
    {
    case GamePieceType::rook:
    {
        if (check_orthogonal_move(state, piece, row_to, column_to))
        {
            return true;
        }
    }
    break;

    case GamePieceType::knight:
    {
        Int row_change = ABS(row_to - piece->row);
        Int column_change = ABS(column_to - piece->column);
        if ((row_change == 1 && column_change == 2 || row_change == 2 && column_change == 1) &&
            !is_occupied(state, row_to, column_to))
        {
            return true;
        }
    }
    break;

    case GamePieceType::bishop:
    {
        if (check_diagonal_move(state, piece, row_to, column_to))
        {
            return true;
        }
    }
    break;

    case GamePieceType::queen:
    {
        if (check_orthogonal_move(state, piece, row_to, column_to))
        {
            return true;
        }
        if (check_diagonal_move(state, piece, row_to, column_to))
        {
            return true;
        }
    }
    break;

    case GamePieceType::king:
    {
        Int row_change = ABS(row_to - piece->row);
        Int column_change = ABS(column_to - piece->column);
        if (MAX(row_change, column_change) == 1 &&
            !is_occupied(state, row_to, column_to))
        {
            return true;
        }
    }
    break;

    case GamePieceType::pawn:
    {
        Int direction = piece->side == GameSide::white ? 1 : -1;
        Int row_change = row_to - piece->row;
        Int column_change = column_to - piece->column;

        if (column_change == 0)
        {
            if (!piece->pawn_state.moved && row_change == direction * 2 &&
                !is_occupied(state, piece->row + direction * 1, piece->column) &&
                !is_occupied(state, piece->row + direction * 2, piece->column))
            {
                return true;
            }
            else if (row_change == direction * 1 &&
                     !is_occupied(state, piece->row + direction * 1, piece->column))
            {
                return true;
            }
        }
    }
    break;
    }

    return false;
}

Void update_game_piece_pos(GameState *state, GamePiece *piece, Int row, Int column)
{
    state->board[piece->row][piece->column] = null;
    state->board[row][column] = piece;
    piece->row = row;
    piece->column = column;
    if (piece->type == GamePieceType::pawn)
    {
        piece->pawn_state.moved = true;
    }
}
