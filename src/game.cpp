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

enum struct GameMoveType
{
    move,
    pawn_charge,
    pawn_capture,
    en_passant,
    castling_king,
    castling_queen,
};

struct GameMove
{
    GameMoveType type;
    GamePiece *piece;
    Int row_from;
    Int column_from;
    Int row_to;
    Int column_to;
};

struct GameState
{
    GamePiece pieces[PIECE_COUNT];
    GamePiece *board[BOARD_ROW_COUNT][BOARD_COLUMN_COUNT];
    GameSide player_side;
    GameSide current_side;
    GamePiece *selected_piece;
    Array<GameMove> history;
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
    state.history = create_array<GameMove>();

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

struct GameMoveCheck
{
    Bool is_illegal;
    Bool is_pawn_promote;
    GameMoveType move_type;
    GamePiece *captured_piece;
    GamePiece *castling_piece;
};

GameMoveCheck illegal_move()
{
    GameMoveCheck result = {};
    result.is_illegal = true;
    return result;
}

GameMoveCheck general_move(GameState *state, Int row_to, Int column_to)
{
    GameMoveCheck result = {};
    result.move_type = GameMoveType::move;
    if (is_foe_occupied(state, row_to, column_to))
    {
        result.captured_piece = state->board[row_to][column_to];
    }
    return result;
}

GameMoveCheck check_orthogonal_move(GameState *state, GamePiece *piece, Int row_to, Int column_to)
{
    ASSERT(row_to != piece->row || column_to != piece->column);

    if (column_to == piece->column)
    {
        if (row_to > piece->row)
        {
            for (Int row = piece->row + 1; row < row_to; row++)
            {
                if (is_occupied(state, row, piece->column))
                {
                    return illegal_move();
                }
            }
        }
        else
        {
            for (Int row = piece->row - 1; row > row_to; row--)
            {
                if (is_occupied(state, row, piece->column))
                {
                    return illegal_move();
                }
            }
        }
        return general_move(state, row_to, column_to);
    }
    else if (row_to == piece->row)
    {
        if (column_to > piece->column)
        {
            for (Int column = piece->column + 1; column < column_to; column++)
            {
                if (is_occupied(state, piece->row, column))
                {
                    return illegal_move();
                }
            }
        }
        else
        {
            for (Int column = piece->column - 1; column > column_to; column--)
            {
                if (is_occupied(state, piece->row, column))
                {
                    return illegal_move();
                }
            }
        }
        return general_move(state, row_to, column_to);
    }
    else
    {
        return illegal_move();
    }
}

GameMoveCheck check_diagonal_move(GameState *state, GamePiece *piece, Int row_to, Int column_to)
{
    ASSERT(row_to != piece->row || column_to != piece->column);

    if (row_to - piece->row == column_to - piece->column)
    {
        if (row_to > piece->row)
        {
            for (Int row = piece->row + 1, column = piece->column + 1; row < row_to; row++, column++)
            {
                if (is_occupied(state, row, column))
                {
                    return illegal_move();
                }
            }
        }
        else
        {
            for (Int row = piece->row - 1, column = piece->column - 1; row > row_to; row--, column--)
            {
                if (is_occupied(state, row, column))
                {
                    return illegal_move();
                }
            }
        }
        return general_move(state, row_to, column_to);
    }
    else if (row_to - piece->row == -(column_to - piece->column))
    {
        if (row_to > piece->row)
        {
            for (Int row = piece->row + 1, column = piece->column - 1; row < row_to; row++, column--)
            {
                if (is_occupied(state, row, column))
                {
                    return illegal_move();
                }
            }
        }
        else
        {
            for (Int row = piece->row - 1, column = piece->column + 1; row > row_to; row--, column++)
            {
                if (is_occupied(state, row, column))
                {
                    return illegal_move();
                }
            }
        }
        return general_move(state, row_to, column_to);
    }
    else
    {
        return illegal_move();
    }
}

GameMoveCheck check_game_move(GameState *state, GamePiece *piece, Int row_to, Int column_to)
{
    if (row_to == piece->row && column_to == piece->column)
    {
        return illegal_move();
    }

    if (is_friend_occupied(state, row_to, column_to))
    {
        return illegal_move();
    }

    switch (piece->type)
    {
    case GamePieceType::rook:
    {
        return check_orthogonal_move(state, piece, row_to, column_to);
    }
    break;

    case GamePieceType::knight:
    {
        Int row_change_abs = ABS(row_to - piece->row);
        Int column_change_abs = ABS(column_to - piece->column);
        if (row_change_abs == 1 && column_change_abs == 2 || row_change_abs == 2 && column_change_abs == 1)
        {
            return general_move(state, row_to, column_to);
        }
        else
        {
            return illegal_move();
        }
    }
    break;

    case GamePieceType::bishop:
    {
        return check_diagonal_move(state, piece, row_to, column_to);
    }
    break;

    case GamePieceType::queen:
    {
        GameMoveCheck orthogonal_move_check = check_orthogonal_move(state, piece, row_to, column_to);
        if (orthogonal_move_check.is_illegal)
        {
            return check_diagonal_move(state, piece, row_to, column_to);
        }
        else
        {
            return orthogonal_move_check;
        }
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
            return general_move(state, row_to, column_to);
        }
        else if (row_change_abs == 0 && column_change_abs == 2 && piece->moves == 0)
        {
            GamePiece *castling_piece;
            if (column_change > 0)
            {
                for (Int column = piece->column + 1; column < BOARD_COLUMN_COUNT - 1; column++)
                {
                    if (is_occupied(state, piece->row, column))
                    {
                        return illegal_move();
                    }
                }
                castling_piece = state->board[piece->row][BOARD_COLUMN_COUNT - 1];
            }
            else
            {
                for (Int column = piece->column - 1; column > 0; column--)
                {
                    if (is_occupied(state, piece->row, column))
                    {
                        return illegal_move();
                    }
                }
                castling_piece = state->board[piece->row][0];
            }

            if (is_friend(state, castling_piece) &&
                castling_piece->type == GamePieceType::rook &&
                castling_piece->moves == 0)
            {
                GameMoveCheck castling_move = {};
                if (column_change > 0)
                {
                    castling_move.move_type = GameMoveType::castling_king;
                }
                else
                {
                    castling_move.move_type = GameMoveType::castling_queen;
                }
                castling_move.castling_piece = castling_piece;
                return castling_move;
            }
            else
            {
                return illegal_move();
            }
        }
        else
        {
            return illegal_move();
        }
    }
    break;

    case GamePieceType::pawn:
    {
        Int direction;
        Int final_row;
        if (piece->side == GameSide::white)
        {
            direction = 1;
            final_row = BOARD_ROW_COUNT - 1;
        }
        else
        {
            direction = -1;
            final_row = 0;
        }
        Int row_change = row_to - piece->row;
        Int column_change = column_to - piece->column;
        Int column_change_abs = ABS(column_change);
        if (column_change == 0)
        {
            GameMoveCheck pawn_orthogonal_move = {};
            if (piece->moves == 0 && row_change == direction * 2 &&
                !is_occupied(state, piece->row + direction * 1, piece->column) &&
                !is_occupied(state, piece->row + direction * 2, piece->column))
            {
                pawn_orthogonal_move.move_type = GameMoveType::pawn_charge;
                return pawn_orthogonal_move;
            }
            else if (row_change == direction * 1 &&
                     !is_occupied(state, piece->row + direction * 1, piece->column))
            {
                pawn_orthogonal_move.move_type = GameMoveType::move;
                if (row_to == final_row)
                {
                    pawn_orthogonal_move.is_pawn_promote = true;
                }
                return pawn_orthogonal_move;
            }
            else
            {
                return illegal_move();
            }
        }
        else if (column_change_abs == 1 && row_change == direction * 1)
        {
            GameMoveCheck pawn_diagonal_move = {};
            if (is_foe_occupied(state, row_to, column_to))
            {
                pawn_diagonal_move.move_type = GameMoveType::pawn_capture;
                pawn_diagonal_move.captured_piece = state->board[row_to][column_to];
                if (row_to == final_row)
                {
                    pawn_diagonal_move.is_pawn_promote = true;
                }
                return pawn_diagonal_move;
            }
            else if (!is_occupied(state, row_to, column_to) && is_foe_occupied(state, piece->row, column_to))
            {
                if (state->history.count > 0)
                {
                    GameMove *last_move = &state->history[state->history.count - 1];
                    if (last_move->type == GameMoveType::pawn_charge &&
                        last_move->piece->row == piece->row && last_move->piece->column == column_to)
                    {
                        pawn_diagonal_move.move_type = GameMoveType::en_passant;
                        pawn_diagonal_move.captured_piece = last_move->piece;
                        return pawn_diagonal_move;
                    }
                    else
                    {
                        return illegal_move();
                    }
                }
                else
                {
                    return illegal_move();
                }
            }
            else
            {
                return illegal_move();
            }
        }
        else
        {
            return illegal_move();
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

Void move_game_piece(GameState *state, Int row_from, Int column_from, Int row_to, Int column_to)
{
    GamePiece *piece = state->board[row_from][column_from];
    state->board[row_from][column_from] = null;
    state->board[row_to][column_to] = piece;
    piece->row = row_to;
    piece->column = column_to;
    piece->moves++;
}

Void remove_game_piece(GameState *state, Int row, Int column)
{
    state->board[row][column] = null;
}

Void record_game_move(GameState *state, GameMoveType move_type, Int row_from, Int column_from, Int row_to, Int column_to)
{
    GameMove *move = state->history.push();
    move->type = move_type;
    move->piece = state->board[row_from][column_from];
    move->row_from = row_from;
    move->column_from = column_from;
    move->row_to = row_to;
    move->column_to = column_to;

    move_game_piece(state, row_from, column_from, row_to, column_to);
    if (move_type == GameMoveType::castling_king)
    {
        move_game_piece(state, row_from, BOARD_COLUMN_COUNT - 1, row_to, column_to - 1);
    }
    else if (move_type == GameMoveType::castling_queen)
    {
        move_game_piece(state, row_from, 0, row_to, column_to + 1);
    }
    else if (move_type == GameMoveType::en_passant)
    {
        remove_game_piece(state, row_from, column_to);
    }
}
