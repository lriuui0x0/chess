#pragma once
#include "../lib/util.hpp"

typedef UInt64 BitBoard;

#define NO_SQUARE (-1)
BitBoard bit_square(Int square)
{
    ASSERT(square != NO_SQUARE);
    BitBoard result = 1llu << square;
    return result;
}

struct SimplePieceTable
{
    BitBoard move[64];
};

struct SlidingPieceTableSquare
{
    BitBoard blocker_mask;
    UInt64 blocker_bit_count;
    UInt64 magic;
    BitBoard *move;
};

struct SlidingPieceTable
{
    SlidingPieceTableSquare board[64];
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
    rook,
    knight,
    bishop,
    queen,
    king,
    pawn,

    count,
};
}
typedef Int GamePieceTypeEnum;

namespace GameSide
{
enum
{
    white,
    black,

    count,
};
}
typedef Int GameSideEnum;

GameSideEnum oppose(GameSideEnum side)
{
    return side ^ 1;
}

typedef UInt8 GamePiece;
#define NO_GAME_PIECE ((GamePiece)-1)

GamePiece get_piece(GameSideEnum side, GamePieceTypeEnum piece_type)
{
    ASSERT(side >= 0 && side < GameSide::count);
    ASSERT(piece_type >= 0 && GamePieceType::count);
    GamePiece piece = piece_type | (side << 3);
    return piece;
}

GamePieceTypeEnum get_piece_type(GamePiece piece)
{
    ASSERT(piece != NO_GAME_PIECE);
    GamePieceTypeEnum piece_type = piece & (0x7);
    ASSERT(piece_type >= 0 && piece_type < GamePieceType::count);
    return piece_type;
}

GameSideEnum get_side(GamePiece piece)
{
    ASSERT(piece != NO_GAME_PIECE);
    GameSideEnum side = piece >> 3;
    ASSERT(side >= 0 && side < GameSide::count);
    return side;
}

Str get_game_piece_name(GamePiece piece)
{
    GameSideEnum side = get_side(piece);
    GamePieceTypeEnum piece_type = get_piece_type(piece);
    switch (piece_type)
    {
    case GamePieceType::rook:
    {
        return side == GameSide::white ? str("white rook") : str("black rook");
    }
    break;

    case GamePieceType::knight:
    {
        return side == GameSide::white ? str("white knight") : str("black knight");
    }
    break;

    case GamePieceType::bishop:
    {
        return side == GameSide::white ? str("white bishop") : str("black bishop");
    }
    break;

    case GamePieceType::queen:
    {
        return side == GameSide::white ? str("white queen") : str("black queen");
    }
    break;

    case GamePieceType::king:
    {
        return side == GameSide::white ? str("white king") : str("black king");
    }
    break;

    case GamePieceType::pawn:
    {
        return side == GameSide::white ? str("white pawn") : str("black pawn");
    }
    break;

    default:
    {
        ASSERT(false);
        return str("");
    }
    }
}

namespace GameCastlingMask
{
enum
{
    queen_side = 0x1,
    king_side = 0x2,
    both_side = 0x3,
};
}

struct GamePosition
{
    GamePiece board[64];
    BitBoard occupancy_side[GameSide::count];
    BitBoard occupancy_piece_type[GamePieceType::count];
};

struct GameHistoryState
{
    Int castling[GameSide::count];
    Int en_passant;
    GamePiece captured_piece;
};
#define MAX_HISTORY_STATE (1000)

struct GameState
{
    BitBoardTable *bit_board_table;
    GamePosition position;
    GameSideEnum player_side;
    GameSideEnum current_side;

    BufferI<GameHistoryState, MAX_HISTORY_STATE> history_states;
    Int history_index;
};

GameHistoryState *get_history_state(GameState *state)
{
    ASSERT(state->history_index >= 0 && state->history_index < MAX_HISTORY_STATE);
    GameHistoryState *history_state = &state->history_states[state->history_index];
    return history_state;
}

GameHistoryState *push_history_state(GameState *state)
{
    GameHistoryState *old_history_state = get_history_state(state);
    state->history_index = (state->history_index + 1) % MAX_HISTORY_STATE;
    state->history_states.count = MIN(MAX_HISTORY_STATE, state->history_states.count + 1);
    GameHistoryState *history_state = &state->history_states[state->history_index];

    history_state->castling[0] = old_history_state->castling[0];
    history_state->castling[1] = old_history_state->castling[1];
    history_state->en_passant = NO_SQUARE;
    history_state->captured_piece = NO_GAME_PIECE;
    return history_state;
}

GameHistoryState *pop_history_state(GameState *state)
{
    if (state->history_states.count > 1)
    {
        GameHistoryState *history_state = get_history_state(state);
        state->history_index = (state->history_index - 1 + MAX_HISTORY_STATE) % MAX_HISTORY_STATE;
        state->history_states.count--;
        return history_state;
    }
    else
    {
        return null;
    }
}

Bool is_empty(GamePiece piece)
{
    Bool result = piece == NO_GAME_PIECE;
    return result;
}

Bool is_friend(GameState *state, GamePiece piece)
{
    GameSideEnum side = get_side(piece);
    Bool result = piece != NO_GAME_PIECE && state->current_side == side;
    return result;
}

Bool is_foe(GameState *state, GamePiece piece)
{
    GameSideEnum side = get_side(piece);
    Bool result = piece != NO_GAME_PIECE && state->current_side != side;
    return result;
}

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

Void add_game_piece(GameState *state, Int square, GamePiece piece)
{
    GameSideEnum side = get_side(piece);
    GamePieceTypeEnum piece_type = get_piece_type(piece);
    state->position.board[square] = piece;
    state->position.occupancy_side[side] |= bit_square(square);
    state->position.occupancy_piece_type[piece_type] |= bit_square(square);
}

GamePiece remove_game_peice(GameState *state, Int square)
{
    GamePiece piece = state->position.board[square];
    GameSideEnum side = get_side(piece);
    GamePieceTypeEnum piece_type = get_piece_type(piece);
    state->position.board[square] = NO_GAME_PIECE;
    state->position.occupancy_side[side] &= ~bit_square(square);
    state->position.occupancy_piece_type[piece_type] &= ~bit_square(square);
    return piece;
}

GameState get_initial_game_state(BitBoardTable *bit_board_table)
{
    GameState state = {};
    state.bit_board_table = bit_board_table;
    memset(state.position.board, NO_GAME_PIECE, sizeof(state.position.board));
    state.player_side = GameSide::white;
    state.current_side = GameSide::white;

    state.history_states.count = 1;
    GameHistoryState *history_state = get_history_state(&state);
    history_state->castling[GameSide::white] = history_state->castling[GameSide::black] = GameCastlingMask::both_side;
    history_state->en_passant = NO_SQUARE;
    history_state->captured_piece = NO_GAME_PIECE;

    add_game_piece(&state, get_square(0, 0), get_piece(GameSide::white, GamePieceType::rook));
    add_game_piece(&state, get_square(0, 1), get_piece(GameSide::white, GamePieceType::knight));
    add_game_piece(&state, get_square(0, 2), get_piece(GameSide::white, GamePieceType::bishop));
    add_game_piece(&state, get_square(0, 3), get_piece(GameSide::white, GamePieceType::queen));
    add_game_piece(&state, get_square(0, 4), get_piece(GameSide::white, GamePieceType::king));
    add_game_piece(&state, get_square(0, 5), get_piece(GameSide::white, GamePieceType::bishop));
    add_game_piece(&state, get_square(0, 6), get_piece(GameSide::white, GamePieceType::knight));
    add_game_piece(&state, get_square(0, 7), get_piece(GameSide::white, GamePieceType::rook));
    for (Int pawn_i = 0; pawn_i < 8; pawn_i++)
    {
        add_game_piece(&state, get_square(1, pawn_i), get_piece(GameSide::white, GamePieceType::pawn));
    }

    add_game_piece(&state, get_square(7, 0), get_piece(GameSide::black, GamePieceType::rook));
    add_game_piece(&state, get_square(7, 1), get_piece(GameSide::black, GamePieceType::knight));
    add_game_piece(&state, get_square(7, 2), get_piece(GameSide::black, GamePieceType::bishop));
    add_game_piece(&state, get_square(7, 3), get_piece(GameSide::black, GamePieceType::queen));
    add_game_piece(&state, get_square(7, 4), get_piece(GameSide::black, GamePieceType::king));
    add_game_piece(&state, get_square(7, 5), get_piece(GameSide::black, GamePieceType::bishop));
    add_game_piece(&state, get_square(7, 6), get_piece(GameSide::black, GamePieceType::knight));
    add_game_piece(&state, get_square(7, 7), get_piece(GameSide::black, GamePieceType::rook));
    for (Int pawn_i = 0; pawn_i < 8; pawn_i++)
    {
        add_game_piece(&state, get_square(6, pawn_i), get_piece(GameSide::black, GamePieceType::pawn));
    }

    return state;
}

BitBoard get_both_occupancy(GameState *state)
{
    BitBoard occupancy = state->position.occupancy_side[0] | state->position.occupancy_side[1];
    return occupancy;
}

BitBoard check_sliding_piece_move(GameState *state, Int square, GameSideEnum side, SlidingPieceTable *sliding_piece_table)
{
    SlidingPieceTableSquare *table_square = &sliding_piece_table->board[square];
    BitBoard occupancy = get_both_occupancy(state);
    BitBoard blocker_mask = table_square->blocker_mask;
    BitBoard blocker = blocker_mask & occupancy;
    UInt64 hash = (blocker * table_square->magic) >> (64 - table_square->blocker_bit_count);
    BitBoard move = table_square->move[hash] & ~state->position.occupancy_side[side];
    return move;
}

BitBoard check_simple_piece_move(GameState *state, Int square, GameSideEnum side, SimplePieceTable *simple_piece_table)
{
    BitBoard move = simple_piece_table->move[square] & ~state->position.occupancy_side[side];
    return move;
}

BitBoard check_pawn_move(GameState *state, Int square, GameSideEnum side)
{
    BitBoard square_bit = bit_square(square);
    BitBoard occupancy = get_both_occupancy(state);
    if (side == GameSide::white)
    {
        BitBoard move = 0;
        move |= (square_bit << 8) & ~occupancy;

        BitBoard start_squares = ~(((UInt64)-1) << 8) << 8;
        if ((square_bit & start_squares) && move)
        {
            move |= (square_bit << 16) & ~occupancy;
        }
        return move;
    }
    else if (side == GameSide::black)
    {
        BitBoard move = 0;
        move |= (square_bit >> 8) & ~occupancy;

        BitBoard start_squares = ~(((UInt64)-1) >> 8) >> 8;
        if ((square_bit & start_squares) && move)
        {
            move |= (square_bit >> 16) & ~occupancy;
        }
        return move;
    }
    else
    {
        ASSERT(false);
        return 0;
    }
}

BitBoard check_pawn_capture(GameState *state, Int square, GameSideEnum side)
{
    GameHistoryState *history_state = get_history_state(state);
    BitBoard square_bit = bit_square(square);
    BitBoard oppose_occupancy = state->position.occupancy_side[oppose(side)];
    if (history_state->en_passant != NO_SQUARE)
    {
        oppose_occupancy |= bit_square(history_state->en_passant);
    }

    BitBoard left_side_mask = 0xfefefefefefefefe;
    BitBoard right_side_mask = 0x7f7f7f7f7f7f7f7f;
    if (side == GameSide::white)
    {
        BitBoard move = 0;
        move |= (square_bit << 7) & oppose_occupancy & left_side_mask;
        move |= (square_bit << 9) & oppose_occupancy & right_side_mask;
        return move;
    }
    else if (side == GameSide::black)
    {
        BitBoard move = 0;
        move |= (square_bit >> 7) & oppose_occupancy & right_side_mask;
        move |= (square_bit >> 9) & oppose_occupancy & left_side_mask;
        return move;
    }
    else
    {
        ASSERT(false);
        return 0;
    }
}

BitBoard check_castling_move(GameState *state, GameSideEnum side)
{
    GameHistoryState *history_state = get_history_state(state);
    BitBoard occupancy = get_both_occupancy(state);
    BitBoard move = 0;
    if (history_state->castling[side] & GameCastlingMask::queen_side)
    {
        BitBoard blocker_mask = side == GameSide::white ? 0xellu : 0xellu << 56;
        BitBoard occupancy = get_both_occupancy(state);
        if (!(blocker_mask & occupancy))
        {
            BitBoard move_to = side == GameSide::white ? 0x4llu : 0x4llu << 56;
            move |= move_to;
        }
    }

    if (history_state->castling[side] & GameCastlingMask::king_side)
    {
        BitBoard blocker_mask = side == GameSide::white ? 0x60llu : 0x60llu << 56;
        if (!(blocker_mask & occupancy))
        {
            BitBoard move_to = side == GameSide::white ? 0x40llu : 0x40llu << 56;
            move |= move_to;
        }
    }
    return move;
}

BitBoard check_game_move(GameState *state, Int square, BitBoardTable *bit_board_table)
{
    ASSERT(square >= 0 && square < 64);
    GamePiece piece = state->position.board[square];
    ASSERT(!is_empty(piece));

    GameSideEnum side = get_side(piece);
    GamePieceTypeEnum piece_type = get_piece_type(piece);
    switch (piece_type)
    {
    case GamePieceType::rook:
    {
        BitBoard move = check_sliding_piece_move(state, square, side, &bit_board_table->rook_table);
        return move;
    }
    break;

    case GamePieceType::knight:
    {
        BitBoard move = check_simple_piece_move(state, square, side, &bit_board_table->knight_table);
        return move;
    }
    break;

    case GamePieceType::bishop:
    {
        BitBoard move = check_sliding_piece_move(state, square, side, &bit_board_table->bishop_table);
        return move;
    }
    break;

    case GamePieceType::queen:
    {
        BitBoard rook_move = check_sliding_piece_move(state, square, side, &bit_board_table->rook_table);
        BitBoard bishop_move = check_sliding_piece_move(state, square, side, &bit_board_table->bishop_table);
        BitBoard move = rook_move | bishop_move;
        return move;
    }
    break;

    case GamePieceType::king:
    {
        BitBoard move = check_simple_piece_move(state, square, side, &bit_board_table->king_table);
        BitBoard castling_move = check_castling_move(state, side);
        move |= castling_move;
        return move;
    }
    break;

    case GamePieceType::pawn:
    {
        BitBoard move = check_pawn_move(state, square, side);
        move |= check_pawn_capture(state, square, side);
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

BitBoard check_attack_by(GameState *state, Int square, GameSideEnum side)
{
    BitBoard attacks = 0;
    BitBoard rook_attack = check_sliding_piece_move(state, square, side, &state->bit_board_table->rook_table);
    attacks |= rook_attack & state->position.occupancy_piece_type[GamePieceType::rook];
    BitBoard knight_attack = check_simple_piece_move(state, square, side, &state->bit_board_table->knight_table);
    attacks |= knight_attack & state->position.occupancy_piece_type[GamePieceType::knight];
    BitBoard bishop_attack = check_sliding_piece_move(state, square, side, &state->bit_board_table->bishop_table);
    attacks |= bishop_attack & state->position.occupancy_piece_type[GamePieceType::bishop];
    BitBoard queen_attack = rook_attack | bishop_attack;
    attacks |= queen_attack & state->position.occupancy_piece_type[GamePieceType::queen];
    BitBoard king_attack = check_simple_piece_move(state, square, side, &state->bit_board_table->king_table);
    attacks |= king_attack & state->position.occupancy_piece_type[GamePieceType::king];
    BitBoard pawn_attack = check_pawn_capture(state, square, side);
    attacks |= pawn_attack & state->position.occupancy_piece_type[GamePieceType::pawn];
    return attacks;
}

typedef UInt16 GameMove;

namespace GameMoveType
{
enum
{
    general,
    castling,
    en_passant,
    promotion,
};
};
typedef Int GameMoveTypeEnum;

BufferI<GamePieceTypeEnum, 4> promotion_list = {4, {GamePieceType::queen, GamePieceType::rook, GamePieceType::bishop, GamePieceType::knight}};

Int get_square_from(GameMove move)
{
    Int result = move & 0x3f;
    return result;
}

Int get_square_to(GameMove move)
{
    Int result = (move >> 6) & 0x3f;
    return result;
}

GameMoveTypeEnum get_move_type(GameMove move)
{
    GameMoveTypeEnum result = (move >> 12) & 0x3;
    return result;
}

Int get_promotion_piece_index(GameMove move)
{
    Int result = (move >> 14) & 0x3;
    return result;
}

GameMove add_promotion_piece_type(GameMove move, Int promotion_index)
{
    ASSERT(get_move_type(move) == GameMoveType::promotion);
    ASSERT(promotion_index >= 0 && promotion_index < promotion_list.count);
    GameMove result = move | (promotion_index << 14);
    return result;
}

GameMove get_game_move(GameState *state, Int square_from, Int square_to)
{
    GameMove move = square_from | (square_to << 6);

    GamePiece piece = state->position.board[square_from];
    GamePieceTypeEnum piece_type = get_piece_type(piece);
    if (piece_type == GamePieceType::king)
    {
        if (ABS(square_to - square_from) == 2)
        {
            move |= GameMoveType::castling << 12;
        }
    }
    else if (piece_type == GamePieceType::pawn)
    {
        Int column_from = get_column(square_from);
        Int column_to = get_column(square_to);
        if (column_from != column_to && is_empty(state->position.board[square_to]))
        {
            move |= GameMoveType::en_passant << 12;
        }
        else if (bit_square(square_to) & (0xffllu | 0xffllu << 56))
        {
            move |= GameMoveType::promotion << 12;
        }
    }
    return move;
}

Int get_capture_square(GameState *state, GameMove move)
{
    Int square_from = get_square_from(move);
    Int square_to = get_square_to(move);
    GameMoveTypeEnum move_type = get_move_type(move);
    if (!is_empty(state->position.board[square_to]))
    {
        return square_to;
    }
    else if (move_type == GameMoveType::en_passant)
    {
        Int row_from = get_row(square_from);
        Int column_to = get_column(square_to);
        Int en_passant_square = get_square(row_from, column_to);
        return en_passant_square;
    }
    else
    {
        return NO_SQUARE;
    }
}

GameMove get_castling_rook_move(GameMove move)
{
    ASSERT(get_move_type(move) == GameMoveType::castling);
    Int square_from = get_square_from(move);
    Int square_to = get_square_to(move);
    if (square_to < square_from)
    {
        Int rook_square_from = square_from - 4;
        Int rook_square_to = square_from - 1;
        GameMove rook_move = rook_square_from | (rook_square_to << 6);
        return rook_move;
    }
    else if (square_to > square_from)
    {
        Int rook_square_from = square_from + 3;
        Int rook_square_to = square_from + 1;
        GameMove rook_move = rook_square_from | (rook_square_to << 6);
        return rook_move;
    }
    else
    {
        ASSERT(false);
        return 0;
    }
}

Void record_game_move(GameState *state, GameMove move)
{
    GameHistoryState *history_state = push_history_state(state);
    Int square_from = get_square_from(move);
    Int square_to = get_square_to(move);
    Int capture_square = get_capture_square(state, move);

    if (capture_square != NO_SQUARE)
    {
        history_state->captured_piece = remove_game_peice(state, capture_square);
        ASSERT(!is_empty(history_state->captured_piece));
    }
    GamePiece piece = remove_game_peice(state, square_from);
    GameSideEnum side = get_side(piece);
    GamePieceTypeEnum piece_type = get_piece_type(piece);
    add_game_piece(state, square_to, piece);

    GameMoveTypeEnum move_type = get_move_type(move);
    if (move_type == GameMoveType::castling)
    {
        GameMove rook_move = get_castling_rook_move(move);
        Int rook_square_from = get_square_from(rook_move);
        Int rook_square_to = get_square_to(rook_move);
        GamePiece rook_piece = remove_game_peice(state, rook_square_from);
        ASSERT(get_piece_type(rook_piece) == GamePieceType::rook);
        add_game_piece(state, rook_square_to, rook_piece);
    }

    if (move_type == GameMoveType::promotion)
    {
        ASSERT(piece_type == GamePieceType::pawn);
        GamePieceTypeEnum promotion_piece_type = promotion_list[get_promotion_piece_index(move)];
        GamePiece promotion_piece = get_piece(side, promotion_piece_type);
        add_game_piece(state, square_to, promotion_piece);
    }

    Int king_square = side == GameSide::white ? 4 : 4 + 56;
    Int rook_square_queen_side = side == GameSide::white ? 0 : 0 + 56;
    Int rook_square_king_side = side == GameSide::white ? 7 : 7 + 56;
    if (square_from == king_square || square_to == king_square)
    {
        history_state->castling[side] &= ~GameCastlingMask::both_side;
    }
    if (square_from == rook_square_queen_side || square_to == rook_square_queen_side)
    {
        history_state->castling[side] &= ~GameCastlingMask::queen_side;
    }
    if (square_from == rook_square_king_side || square_to == rook_square_king_side)
    {
        history_state->castling[side] &= ~GameCastlingMask::king_side;
    }

    if (piece_type == GamePieceType::pawn && ABS(square_to - square_from) == 16)
    {
        history_state->en_passant = (square_from + square_to) / 2;
    }

    state->current_side = oppose(state->current_side);
}

Bool rollback_game_move(GameState *state, GameMove move)
{
    GameHistoryState *history_state = pop_history_state(state);
    if (!history_state)
    {
        return false;
    }

    Int square_from = get_square_from(move);
    Int square_to = get_square_to(move);
    GameMoveTypeEnum move_type = get_move_type(move);

    GamePiece piece = state->position.board[square_to];
    GameSideEnum side = get_side(piece);
    if (move_type == GameMoveType::promotion)
    {
        remove_game_peice(state, square_to);
        GamePiece pawn_piece = get_piece(side, GamePieceType::pawn);
        add_game_piece(state, square_to, pawn_piece);
    }

    if (move_type == GameMoveType::castling)
    {
        GameMove rook_move = get_castling_rook_move(move);
        Int rook_square_from = get_square_from(rook_move);
        Int rook_square_to = get_square_to(rook_move);
        GamePiece rook_piece = remove_game_peice(state, rook_square_to);
        ASSERT(get_piece_type(rook_piece) == GamePieceType::rook);
        add_game_piece(state, rook_square_from, rook_piece);
    }

    piece = remove_game_peice(state, square_to);
    add_game_piece(state, square_from, piece);
    if (history_state->captured_piece != NO_GAME_PIECE)
    {
        if (move_type == GameMoveType::en_passant)
        {
            Int row_from = get_row(square_from);
            Int column_to = get_column(square_to);
            Int en_passant_square = get_square(row_from, column_to);
            add_game_piece(state, en_passant_square, history_state->captured_piece);
        }
        else
        {
            add_game_piece(state, square_to, history_state->captured_piece);
        }
    }

    state->current_side = oppose(state->current_side);
    return true;
}

Bool is_move_legal(GameState *state, GameMove move)
{
    Int square_from = get_square_from(move);
    Int square_to = get_square_to(move);
    GameMoveTypeEnum move_type = get_move_type(move);

    GamePiece piece = state->position.board[square_from];
    GameSideEnum side = get_side(piece);
    if (move_type == GameMoveType::castling)
    {
        Int step = square_to > square_from ? 1 : -1;
        for (Int square = square_from; square <= square_to; square += step)
        {
            BitBoard attack_by = check_attack_by(state, square, side);
            if (attack_by)
            {
                return false;
            }
        }
        return true;
    }
    else
    {
        record_game_move(state, move);
        BitBoard attack_by = check_attack_by(state, square_to, side);
        rollback_game_move(state, move);
        return !attack_by;
    }
}
