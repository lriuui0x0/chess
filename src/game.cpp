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
    Int index;
    GameSide side;
    GamePieceType type;
    Int type_index;
    Str name;
    Int row;
    Int column;
    Int moves;
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
    GamePiece *selected_piece;
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
    state.selected_piece = null;

    GamePiece *piece = &state.pieces[0];
    *piece = GamePiece{0, GameSide::white, GamePieceType::rook, 0, str("white rook 0"), 0, 0, 0};
    state.board[piece->row][piece->column] = piece;
    piece++;

    *piece = GamePiece{1, GameSide::white, GamePieceType::knight, 0, str("white knight 0"), 0, 1, 0};
    state.board[piece->row][piece->column] = piece;
    piece++;

    *piece = GamePiece{2, GameSide::white, GamePieceType::bishop, 0, str("white bishop 0"), 0, 2, 0};
    state.board[piece->row][piece->column] = piece;
    piece++;

    *piece = GamePiece{3, GameSide::white, GamePieceType::queen, 0, str("white queen"), 0, 3, 0};
    state.board[piece->row][piece->column] = piece;
    piece++;

    *piece = GamePiece{4, GameSide::white, GamePieceType::king, 0, str("white king"), 0, 4, 0};
    state.board[piece->row][piece->column] = piece;
    piece++;

    *piece = GamePiece{5, GameSide::white, GamePieceType::bishop, 1, str("white bishop 1"), 0, 5, 0};
    state.board[piece->row][piece->column] = piece;
    piece++;

    *piece = GamePiece{6, GameSide::white, GamePieceType::knight, 1, str("white knight 1"), 0, 6, 0};
    state.board[piece->row][piece->column] = piece;
    piece++;

    *piece = GamePiece{7, GameSide::white, GamePieceType::rook, 1, str("white rook 1"), 0, 7, 0};
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

        *piece = GamePiece{8 + pawn_i, GameSide::white, GamePieceType::pawn, pawn_i, concat_str(str("white pawn "), number_suffix), 1, pawn_i, 0};
        state.board[piece->row][piece->column] = piece;
        piece++;
    }

    *piece = GamePiece{16, GameSide::black, GamePieceType::rook, 0, str("black rook 0"), 7, 0, 0};
    state.board[piece->row][piece->column] = piece;
    piece++;

    *piece = GamePiece{17, GameSide::black, GamePieceType::knight, 0, str("black knight 0"), 7, 1, 0};
    state.board[piece->row][piece->column] = piece;
    piece++;

    *piece = GamePiece{18, GameSide::black, GamePieceType::bishop, 0, str("black bishop 0"), 7, 2, 0};
    state.board[piece->row][piece->column] = piece;
    piece++;

    *piece = GamePiece{19, GameSide::black, GamePieceType::queen, 0, str("black queen"), 7, 3, 0};
    state.board[piece->row][piece->column] = piece;
    piece++;

    *piece = GamePiece{20, GameSide::black, GamePieceType::king, 0, str("black king"), 7, 4, 0};
    state.board[piece->row][piece->column] = piece;
    piece++;

    *piece = GamePiece{21, GameSide::black, GamePieceType::bishop, 1, str("black bishop 1"), 7, 5, 0};
    state.board[piece->row][piece->column] = piece;
    piece++;

    *piece = GamePiece{22, GameSide::black, GamePieceType::knight, 1, str("black knight 1"), 7, 6, 0};
    state.board[piece->row][piece->column] = piece;
    piece++;

    *piece = GamePiece{23, GameSide::black, GamePieceType::rook, 1, str("black rook 1"), 7, 7, 0};
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

        *piece = GamePiece{24 + pawn_i, GameSide::black, GamePieceType::pawn, pawn_i, concat_str(str("black pawn "), number_suffix), 6, pawn_i, 0};
        state.board[piece->row][piece->column] = piece;
        piece++;
    }

    return state;
}

Bool is_friend(GameState *state, GamePiece *piece)
{
    return piece && piece->side == state->player_side;
}

Bool is_foe(GameState *state, GamePiece *piece)
{
    return piece && piece->side != state->player_side;
}

Bool is_occupied(GameState *state, Int row, Int column)
{
    GamePiece *piece = state->board[row][column];
    return piece != null;
}

Bool is_friend_occupied(GameState *state, Int row, Int column)
{
    GamePiece *piece = state->board[row][column];
    return piece != null && piece->side == state->player_side;
}

Bool is_foe_occupied(GameState *state, Int row, Int column)
{
    GamePiece *piece = state->board[row][column];
    return piece != null && piece->side != state->player_side;
}

enum struct GameMoveType
{
    move,
    castling,
    capture,
    illegal,
};

struct GameMove
{
    GameMoveType type;
    union {
        GamePiece *captured_piece;
        struct
        {
            GamePiece *castling_rook;
            Int rook_row_to;
            Int rook_column_to;
        };
    };
};

GameMove check_orthogonal_move(GameState *state, GamePiece *piece, Int row_to, Int column_to)
{
    ASSERT(row_to != piece->row || column_to != piece->column);

    GameMove result;
    if (column_to == piece->column)
    {
        if (row_to > piece->row)
        {
            for (Int row = piece->row + 1; row < row_to; row++)
            {
                if (is_occupied(state, row, piece->column))
                {
                    result.type = GameMoveType::illegal;
                    return result;
                }
            }
        }
        else
        {
            for (Int row = piece->row - 1; row > row_to; row--)
            {
                if (is_occupied(state, row, piece->column))
                {
                    result.type = GameMoveType::illegal;
                    return result;
                }
            }
        }

        if (is_foe_occupied(state, row_to, column_to))
        {
            result.type = GameMoveType::capture;
            result.captured_piece = state->board[row_to][column_to];
            return result;
        }
        else
        {
            result.type = GameMoveType::move;
            return result;
        }
    }
    else if (row_to == piece->row)
    {
        if (column_to > piece->column)
        {
            for (Int column = piece->column + 1; column < column_to; column++)
            {
                if (is_occupied(state, piece->row, column))
                {
                    result.type = GameMoveType::illegal;
                    return result;
                }
            }
        }
        else
        {
            for (Int column = piece->column - 1; column > column_to; column--)
            {
                if (is_occupied(state, piece->row, column))
                {
                    result.type = GameMoveType::illegal;
                    return result;
                }
            }
        }

        if (is_foe_occupied(state, row_to, column_to))
        {
            result.type = GameMoveType::capture;
            result.captured_piece = state->board[row_to][column_to];
            return result;
        }
        else
        {
            result.type = GameMoveType::move;
            return result;
        }
    }
    else
    {
        result.type = GameMoveType::illegal;
        return result;
    }
}

GameMove check_diagonal_move(GameState *state, GamePiece *piece, Int row_to, Int column_to)
{
    ASSERT(row_to != piece->row || column_to != piece->column);

    GameMove result;
    if (row_to - piece->row == column_to - piece->column)
    {
        if (row_to > piece->row)
        {
            for (Int row = piece->row + 1, column = piece->column + 1; row < row_to; row++, column++)
            {
                if (is_occupied(state, row, column))
                {
                    result.type = GameMoveType::illegal;
                    return result;
                }
            }
        }
        else
        {
            for (Int row = piece->row - 1, column = piece->column - 1; row > row_to; row--, column--)
            {
                if (is_occupied(state, row, column))
                {
                    result.type = GameMoveType::illegal;
                    return result;
                }
            }
        }

        if (is_foe_occupied(state, row_to, column_to))
        {
            result.type = GameMoveType::capture;
            result.captured_piece = state->board[row_to][column_to];
            return result;
        }
        else
        {
            result.type = GameMoveType::move;
            return result;
        }
    }
    else if (row_to - piece->row == -(column_to - piece->column))
    {
        if (row_to > piece->row)
        {
            for (Int row = piece->row + 1, column = piece->column - 1; row < row_to; row++, column--)
            {
                if (is_occupied(state, row, column))
                {
                    result.type = GameMoveType::illegal;
                    return result;
                }
            }
        }
        else
        {
            for (Int row = piece->row - 1, column = piece->column + 1; row > row_to; row--, column++)
            {
                if (is_occupied(state, row, column))
                {
                    result.type = GameMoveType::illegal;
                    return result;
                }
            }
        }

        if (is_foe_occupied(state, row_to, column_to))
        {
            result.type = GameMoveType::capture;
            result.captured_piece = state->board[row_to][column_to];
            return result;
        }
        else
        {
            result.type = GameMoveType::move;
            return result;
        }
    }
    else
    {
        result.type = GameMoveType::illegal;
        return result;
    }
}

GameMove check_game_move(GameState *state, GamePiece *piece, Int row_to, Int column_to)
{
    GameMove result;
    if (row_to == piece->row && column_to == piece->column)
    {
        result.type = GameMoveType::illegal;
        return result;
    }

    if (is_friend_occupied(state, row_to, column_to))
    {
        result.type = GameMoveType::illegal;
        return result;
    }

    switch (piece->type)
    {
    case GamePieceType::rook:
    {
        result = check_orthogonal_move(state, piece, row_to, column_to);
        return result;
    }
    break;

    case GamePieceType::knight:
    {
        Int row_change_abs = ABS(row_to - piece->row);
        Int column_change_abs = ABS(column_to - piece->column);
        if (row_change_abs == 1 && column_change_abs == 2 || row_change_abs == 2 && column_change_abs == 1)
        {
            if (is_foe_occupied(state, row_to, column_to))
            {
                result.type = GameMoveType::capture;
                result.captured_piece = state->board[row_to][column_to];
                return result;
            }
            else
            {
                result.type = GameMoveType::move;
                return result;
            }
        }
        else
        {
            result.type = GameMoveType::illegal;
            return result;
        }
    }
    break;

    case GamePieceType::bishop:
    {
        result = check_diagonal_move(state, piece, row_to, column_to);
        return result;
    }
    break;

    case GamePieceType::queen:
    {
        result = check_orthogonal_move(state, piece, row_to, column_to);
        if (result.type == GameMoveType::illegal)
        {
            result = check_diagonal_move(state, piece, row_to, column_to);
        }
        return result;
    }
    break;

    case GamePieceType::king:
    {
        Int row_change = row_to - piece->row;
        Int row_change_abs = ABS(row_change);
        Int column_change = column_to - piece->column;
        Int column_change_abs = ABS(column_change);
        if (MAX(row_change_abs, column_change_abs) == 1)
        {
            if (is_foe_occupied(state, row_to, column_to))
            {
                result.type = GameMoveType::capture;
                result.captured_piece = state->board[row_to][column_to];
                return result;
            }
            else
            {
                result.type = GameMoveType::move;
                return result;
            }
        }
        else if (row_change_abs == 0 && column_change_abs == 2 && piece->moves == 0)
        {
            GamePiece *rook_piece;
            if (column_change > 0)
            {
                for (Int column = piece->column + 1; column < BOARD_COLUMN_COUNT - 1; column++)
                {
                    if (is_occupied(state, piece->row, column))
                    {
                        result.type = GameMoveType::illegal;
                        return result;
                    }
                }
                rook_piece = state->board[piece->row][BOARD_COLUMN_COUNT - 1];
            }
            else
            {
                for (Int column = piece->column - 1; column > 0; column--)
                {
                    if (is_occupied(state, piece->row, column))
                    {
                        result.type = GameMoveType::illegal;
                        return result;
                    }
                }
                rook_piece = state->board[piece->row][0];
            }

            if (is_friend(state, rook_piece) &&
                rook_piece->type == GamePieceType::rook &&
                rook_piece->moves == 0)
            {
                result.type = GameMoveType::castling;
                result.castling_rook = rook_piece;
                result.rook_row_to = piece->row;
                if (column_change > 0)
                {
                    result.rook_column_to = column_to - 1;
                }
                else
                {
                    result.rook_column_to = column_to + 1;
                }
                return result;
            }
            else
            {
                result.type = GameMoveType::illegal;
                return result;
            }
        }
        else
        {
            result.type = GameMoveType::illegal;
            return result;
        }
    }
    break;

    case GamePieceType::pawn:
    {
        Int direction = piece->side == GameSide::white ? 1 : -1;
        Int row_change = row_to - piece->row;
        Int column_change = column_to - piece->column;
        Int column_change_abs = ABS(column_change);
        if (column_change == 0)
        {
            if (piece->moves == 0 && row_change == direction * 2 &&
                !is_occupied(state, piece->row + direction * 1, piece->column) &&
                !is_occupied(state, piece->row + direction * 2, piece->column))
            {
                result.type = GameMoveType::move;
                return result;
            }
            else if (row_change == direction * 1 &&
                     !is_occupied(state, piece->row + direction * 1, piece->column))
            {
                result.type = GameMoveType::move;
                return result;
            }
            else
            {
                result.type = GameMoveType::illegal;
                return result;
            }
        }
        else if (column_change_abs == 1 && row_change == direction * 1)
        {
            if (is_foe_occupied(state, row_to, column_to))
            {
                result.type = GameMoveType::capture;
                result.captured_piece = state->board[row_to][column_to];
                return result;
            }
            else
            {
                result.type = GameMoveType::illegal;
                return result;
            }
        }
        else
        {
            result.type = GameMoveType::illegal;
            return result;
        }
    }
    break;

    default:
    {
        ASSERT(false);
    }
    break;
    }
}

Void update_game_piece_pos(GameState *state, GamePiece *piece, Int row, Int column)
{
    state->board[piece->row][piece->column] = null;
    state->board[row][column] = piece;
    piece->row = row;
    piece->column = column;
    piece->moves++;
}
