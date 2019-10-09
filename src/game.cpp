#pragma once
#include "../lib/util.hpp"

typedef UInt64 BitBoard;

// NOTE: Square is encoded in 6 or 7 bits, depending on if the square can be invalid
#define NO_SQUARE (0x7f)
typedef Int8 Square;

BitBoard bit_square(Square square)
{
    ASSERT(square != NO_SQUARE);
    BitBoard result = 1llu << square;
    return result;
}

Square get_row(Square square)
{
    ASSERT(square >= 0 && square < 64);
    return square / 8;
}

Square get_column(Square square)
{
    ASSERT(square >= 0 && square < 64);
    return square % 8;
}

Square get_square(Square row, Square column)
{
    ASSERT(row >= 0 && row < 8);
    ASSERT(column >= 0 && column < 8);
    return row * 8 + column;
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

// NOTE: Evaluation lookup table depends on this order.
namespace GamePieceType
{
enum
{
    pawn,
    knight,
    bishop,
    rook,
    queen,
    king,

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

// NOTE: GamePiece is encoded with 4 bits
// x(1)  xxx(3)
// side  piece type
typedef UInt8 GamePiece;
#define NO_GAME_PIECE ((GamePiece)0xf)

GamePiece get_game_piece(GameSideEnum side, GamePieceTypeEnum piece_type)
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

// NOTE: GameCastling is encoded in 4 bits
// x         x          x         x
// black oo  black ooo  white oo  white ooo
typedef UInt8 GameCastling;

namespace GameCastlingMask
{
enum
{
    queen_side = 0x1,
    king_side = 0x2,
    both_side = 0x3,
};
}
typedef Int GameCastlingMaskEnum;

GameCastling castling_for(GameCastling castling, GameSideEnum side, GameCastlingMaskEnum mask)
{
    GameCastling result = castling & (mask << (side * 2));
    return result;
}

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

// NOTE: GameMove is encoded in 32 bits
// x(1)  xxxx(4)         xxxxxxx(7)  xxxx(4)   xx(2)            xx(2)       xxxxxx(6)  xxxxxx(6)
// side  captured piece  en passant  castling  promotion index  move type   square to  square from
typedef UInt32 GameMove;

BufferI<GamePieceTypeEnum, 4> promotion_list = {4, {GamePieceType::queen, GamePieceType::rook, GamePieceType::bishop, GamePieceType::knight}};

Square get_from(GameMove move)
{
    Square result = move & 0x3f;
    return result;
}

Square get_to(GameMove move)
{
    Square result = (move >> 6) & 0x3f;
    return result;
}

GameMoveTypeEnum get_move_type(GameMove move)
{
    GameMoveTypeEnum result = (move >> 12) & 0x3;
    return result;
}

Int get_promotion_index(GameMove move)
{
    Int result = (move >> 14) & 0x3;
    return result;
}

GameCastling get_castling(GameMove move)
{
    GameCastling result = (move >> 16) & 0xf;
    return result;
}

Square get_en_passant(GameMove move)
{
    Square result = (move >> 20) & 0x7f;
    return result;
}

GamePiece get_captured_piece(GameMove move)
{
    GamePiece result = (move >> 27) & 0xf;
    return result;
}

GameSideEnum get_side(GameMove move)
{
    GameSideEnum result = (move >> 31) & 0x1;
    return result;
}

#define MAX_HISTORY_COUNT (1000)

struct GameState
{
    BitBoardTable *bit_board_table;
    GameSideEnum player_side;

    GamePiece board[64];
    BitBoard occupancy_side[GameSide::count];
    BitBoard occupancy_piece_type[GamePieceType::count];
    GameSideEnum current_side;
    GameCastling castling;
    Square en_passant;

    GameMove history[MAX_HISTORY_COUNT];
    Int history_count;
    Int history_index;
    Int undo_count;
};

BitBoard get_occupancy(GameState *state, GameSideEnum side, GamePieceTypeEnum piece_type)
{
    BitBoard result = state->occupancy_side[side] & state->occupancy_piece_type[piece_type];
    return result;
}

BitBoard get_occupancy(GameState *state)
{
    BitBoard occupancy = state->occupancy_side[0] | state->occupancy_side[1];
    return occupancy;
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

Void add_game_piece(GameState *state, Square square, GamePiece piece)
{
    GameSideEnum side = get_side(piece);
    GamePieceTypeEnum piece_type = get_piece_type(piece);
    state->board[square] = piece;
    state->occupancy_side[side] |= bit_square(square);
    state->occupancy_piece_type[piece_type] |= bit_square(square);
}

GamePiece remove_game_peice(GameState *state, Square square)
{
    GamePiece piece = state->board[square];
    GameSideEnum side = get_side(piece);
    GamePieceTypeEnum piece_type = get_piece_type(piece);
    state->board[square] = NO_GAME_PIECE;
    state->occupancy_side[side] &= ~bit_square(square);
    state->occupancy_piece_type[piece_type] &= ~bit_square(square);
    return piece;
}

GameState get_initial_game_state(BitBoardTable *bit_board_table)
{
    GameState state = {};
    state.bit_board_table = bit_board_table;
    memset(state.board, NO_GAME_PIECE, sizeof(state.board));
    state.player_side = GameSide::white;
    state.current_side = GameSide::white;
    state.castling = 0xf;
    state.en_passant = NO_SQUARE;
    state.history_count = 0;
    state.history_index = 0;
    state.undo_count = 0;

    add_game_piece(&state, get_square(0, 0), get_game_piece(GameSide::white, GamePieceType::rook));
    add_game_piece(&state, get_square(0, 1), get_game_piece(GameSide::white, GamePieceType::knight));
    add_game_piece(&state, get_square(0, 2), get_game_piece(GameSide::white, GamePieceType::bishop));
    add_game_piece(&state, get_square(0, 3), get_game_piece(GameSide::white, GamePieceType::queen));
    add_game_piece(&state, get_square(0, 4), get_game_piece(GameSide::white, GamePieceType::king));
    add_game_piece(&state, get_square(0, 5), get_game_piece(GameSide::white, GamePieceType::bishop));
    add_game_piece(&state, get_square(0, 6), get_game_piece(GameSide::white, GamePieceType::knight));
    add_game_piece(&state, get_square(0, 7), get_game_piece(GameSide::white, GamePieceType::rook));
    for (Int pawn_i = 0; pawn_i < 8; pawn_i++)
    {
        add_game_piece(&state, get_square(1, pawn_i), get_game_piece(GameSide::white, GamePieceType::pawn));
    }

    add_game_piece(&state, get_square(7, 0), get_game_piece(GameSide::black, GamePieceType::rook));
    add_game_piece(&state, get_square(7, 1), get_game_piece(GameSide::black, GamePieceType::knight));
    add_game_piece(&state, get_square(7, 2), get_game_piece(GameSide::black, GamePieceType::bishop));
    add_game_piece(&state, get_square(7, 3), get_game_piece(GameSide::black, GamePieceType::queen));
    add_game_piece(&state, get_square(7, 4), get_game_piece(GameSide::black, GamePieceType::king));
    add_game_piece(&state, get_square(7, 5), get_game_piece(GameSide::black, GamePieceType::bishop));
    add_game_piece(&state, get_square(7, 6), get_game_piece(GameSide::black, GamePieceType::knight));
    add_game_piece(&state, get_square(7, 7), get_game_piece(GameSide::black, GamePieceType::rook));
    for (Int pawn_i = 0; pawn_i < 8; pawn_i++)
    {
        add_game_piece(&state, get_square(6, pawn_i), get_game_piece(GameSide::black, GamePieceType::pawn));
    }

    return state;
}

BitBoard check_sliding_piece_move(GameState *state, Square square, GameSideEnum side, SlidingPieceTable *sliding_piece_table)
{
    SlidingPieceTableSquare *table_square = &sliding_piece_table->board[square];
    BitBoard occupancy = get_occupancy(state);
    BitBoard blocker_mask = table_square->blocker_mask;
    BitBoard blocker = blocker_mask & occupancy;
    UInt64 hash = (blocker * table_square->magic) >> (64 - table_square->blocker_bit_count);
    BitBoard move = table_square->move[hash] & ~state->occupancy_side[side];
    return move;
}

BitBoard check_simple_piece_move(GameState *state, Square square, GameSideEnum side, SimplePieceTable *simple_piece_table)
{
    BitBoard move = simple_piece_table->move[square] & ~state->occupancy_side[side];
    return move;
}

#define LEFT_SIDE_MASK (0xfefefefefefefefellu)
#define RIGHT_SIDE_MASK (0x7f7f7f7f7f7f7f7fllu)

BitBoard left(BitBoard pawn_bit, GameSideEnum side)
{
    ASSERT(bit_count(pawn_bit) == 1);
    BitBoard result = side == GameSide::white ? ((pawn_bit << 1) & RIGHT_SIDE_MASK) : ((pawn_bit >> 1) & LEFT_SIDE_MASK);
    return result;
}

BitBoard right(BitBoard pawn_bit, GameSideEnum side)
{
    BitBoard result = left(pawn_bit, oppose(side));
}

BitBoard up(BitBoard pawn_bit, GameSideEnum side)
{
    ASSERT(bit_count(pawn_bit) == 1);
    BitBoard result = side == GameSide::white ? (pawn_bit << 8) : (pawn_bit >> 8);
    return result;
}

BitBoard down(BitBoard pawn_bit, GameSideEnum side)
{
    BitBoard result = up(pawn_bit, oppose(side));
    return result;
}

BitBoard up_left(BitBoard pawn_bit, GameSideEnum side)
{
    ASSERT(bit_count(pawn_bit) == 1);
    BitBoard result = side == GameSide::white ? ((pawn_bit << 7) & RIGHT_SIDE_MASK) : ((pawn_bit >> 9) & LEFT_SIDE_MASK);
    return result;
}

BitBoard down_right(BitBoard pawn_bit, GameSideEnum side)
{
    BitBoard result = up_left(pawn_bit, oppose(side));
    return result;
}

BitBoard up_right(BitBoard pawn_bit, GameSideEnum side)
{
    ASSERT(bit_count(pawn_bit) == 1);
    BitBoard result = side == GameSide::white ? ((pawn_bit << 9) & LEFT_SIDE_MASK) : ((pawn_bit >> 7) & RIGHT_SIDE_MASK);
    return result;
}

BitBoard down_left(BitBoard pawn_bit, GameSideEnum side)
{
    BitBoard result = up_right(pawn_bit, oppose(side));
    return result;
}

BitBoard row_mask(Square row)
{
    ASSERT(row >= 0 && row < 8);
    BitBoard result = 0xff << row;
    return result;
}

BitBoard column_mask(Square column)
{
    ASSERT(column >= 0 && column < 8);
    BitBoard result = LEFT_SIDE_MASK << column;
    return result;
}

BitBoard forward_mask(Int row, GameSideEnum side)
{
    ASSERT(row >= 0 && row < 8);
    BitBoard result = side == GameSide::white ? (BitBoard(-1)) << (8 * row) : (BitBoard(-1)) >> (8 * (7 - row));
    return result;
}

BitBoard check_pawn_move(GameState *state, Square square, GameSideEnum side)
{
    BitBoard move = 0;
    BitBoard occupancy = get_occupancy(state);
    BitBoard square_bit = bit_square(square);

    square_bit = up(square_bit, side);
    move |= square_bit & ~occupancy;

    BitBoard start_squares = side == GameSide::white ? ~(((UInt64)-1) << 8) << 8 : ~(((UInt64)-1) >> 8) >> 8;
    if ((square_bit & start_squares) && move)
    {
        square_bit = up(square_bit, side);
        move |= square_bit & ~occupancy;
    }
    return move;
}

BitBoard check_pawn_capture(GameState *state, Square square, GameSideEnum side)
{
    BitBoard square_bit = bit_square(square);
    BitBoard oppose_occupancy = state->occupancy_side[oppose(side)];
    if (state->en_passant != NO_SQUARE)
    {
        oppose_occupancy |= bit_square(state->en_passant);
    }

    BitBoard move = 0;
    move |= up_left(square_bit, side) & oppose_occupancy;
    move |= up_right(square_bit, side) & oppose_occupancy;
    return move;
}

BitBoard check_castling_move(GameState *state, GameSideEnum side)
{
    BitBoard occupancy = get_occupancy(state);
    BitBoard move = 0;
    if (castling_for(state->castling, side, GameCastlingMask::queen_side))
    {
        BitBoard blocker_mask = side == GameSide::white ? 0xellu : 0xellu << 56;
        if (!(blocker_mask & occupancy))
        {
            BitBoard move_to = side == GameSide::white ? 0x4llu : 0x4llu << 56;
            move |= move_to;
        }
    }

    if (castling_for(state->castling, side, GameCastlingMask::king_side))
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

BitBoard check_game_move(GameState *state, Square square)
{
    ASSERT(square >= 0 && square < 64);
    GamePiece piece = state->board[square];
    ASSERT(!is_empty(piece));

    GameSideEnum side = get_side(piece);
    GamePieceTypeEnum piece_type = get_piece_type(piece);
    switch (piece_type)
    {
    case GamePieceType::pawn:
    {
        BitBoard move = check_pawn_move(state, square, side);
        move |= check_pawn_capture(state, square, side);
        return move;
    }
    break;

    case GamePieceType::knight:
    {
        BitBoard move = check_simple_piece_move(state, square, side, &state->bit_board_table->knight_table);
        return move;
    }
    break;

    case GamePieceType::bishop:
    {
        BitBoard move = check_sliding_piece_move(state, square, side, &state->bit_board_table->bishop_table);
        return move;
    }
    break;

    case GamePieceType::rook:
    {
        BitBoard move = check_sliding_piece_move(state, square, side, &state->bit_board_table->rook_table);
        return move;
    }
    break;

    case GamePieceType::queen:
    {
        BitBoard bishop_move = check_sliding_piece_move(state, square, side, &state->bit_board_table->bishop_table);
        BitBoard rook_move = check_sliding_piece_move(state, square, side, &state->bit_board_table->rook_table);
        BitBoard move = bishop_move | rook_move;
        return move;
    }
    break;

    case GamePieceType::king:
    {
        BitBoard move = check_simple_piece_move(state, square, side, &state->bit_board_table->king_table);
        BitBoard castling_move = check_castling_move(state, side);
        move |= castling_move;
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

BitBoard check_attack_by(GameState *state, Square square, GameSideEnum side)
{
    BitBoard attacks = 0;
    BitBoard pawn_attack = check_pawn_capture(state, square, side);
    attacks |= pawn_attack & state->occupancy_piece_type[GamePieceType::pawn];
    BitBoard knight_attack = check_simple_piece_move(state, square, side, &state->bit_board_table->knight_table);
    attacks |= knight_attack & state->occupancy_piece_type[GamePieceType::knight];
    BitBoard bishop_attack = check_sliding_piece_move(state, square, side, &state->bit_board_table->bishop_table);
    attacks |= bishop_attack & state->occupancy_piece_type[GamePieceType::bishop];
    BitBoard rook_attack = check_sliding_piece_move(state, square, side, &state->bit_board_table->rook_table);
    attacks |= rook_attack & state->occupancy_piece_type[GamePieceType::rook];
    BitBoard queen_attack = bishop_attack | rook_attack;
    attacks |= queen_attack & state->occupancy_piece_type[GamePieceType::queen];
    BitBoard king_attack = check_simple_piece_move(state, square, side, &state->bit_board_table->king_table);
    attacks |= king_attack & state->occupancy_piece_type[GamePieceType::king];
    return attacks;
}

// NOTE: Get all information for game move except for promotion index information
GameMove get_game_move(GameState *state, Square square_from, Square square_to)
{
    GameMove move = 0;
    move |= (GameMove)square_from;
    move |= (GameMove)square_to << 6;
    move |= (GameMove)state->castling << 16;
    move |= (GameMove)state->en_passant << 20;
    GamePiece captured_piece = state->board[square_to];
    move |= (GameMove)captured_piece << 27;
    move |= (GameMove)state->current_side << 31;

    GamePiece piece = state->board[square_from];
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
        BitBoard final_row_mask = 0xffllu | (0xffllu << 56);
        Square column_from = get_column(square_from);
        Square column_to = get_column(square_to);
        if (column_from != column_to && is_empty(captured_piece))
        {
            move |= GameMoveType::en_passant << 12;
        }
        else if (bit_square(square_to) & final_row_mask)
        {
            move |= GameMoveType::promotion << 12;
        }
    }
    return move;
}

GameMove add_promotion_index(GameMove move, Int promotion_index)
{
    ASSERT(get_move_type(move) == GameMoveType::promotion);
    ASSERT(promotion_index >= 0 && promotion_index < promotion_list.count);
    move |= promotion_index << 14;
    return move;
}

Square get_en_passant_capture_sqaure(Square square_from, Square square_to)
{
    Square row_from = get_row(square_from);
    Square column_to = get_column(square_to);
    Square en_passant_square = get_square(row_from, column_to);
    return en_passant_square;
}

Square get_capture_square(GameMove move)
{
    Square square_from = get_from(move);
    Square square_to = get_to(move);
    GamePiece captured_piece = get_captured_piece(move);
    if (!is_empty(captured_piece))
    {
        GameMoveTypeEnum move_type = get_move_type(move);
        if (move_type == GameMoveType::en_passant)
        {
            Square en_passant_square = get_en_passant_capture_sqaure(square_from, square_to);
            return en_passant_square;
        }
        else
        {
            return square_to;
        }
    }
    else
    {
        return NO_SQUARE;
    }
}

// NOTE: Get a pseudo move that contains only square from and square to
GameMove get_castling_rook_move(GameMove move)
{
    ASSERT(get_move_type(move) == GameMoveType::castling);
    Square square_from = get_from(move);
    Square square_to = get_to(move);
    if (square_to < square_from)
    {
        Square rook_square_from = square_from - 4;
        Square rook_square_to = square_from - 1;
        GameMove rook_move = rook_square_from | (rook_square_to << 6);
        return rook_move;
    }
    else if (square_to > square_from)
    {
        Square rook_square_from = square_from + 3;
        Square rook_square_to = square_from + 1;
        GameMove rook_move = rook_square_from | (rook_square_to << 6);
        return rook_move;
    }
    else
    {
        ASSERT(false);
        return 0;
    }
}

Int wrap_history_index(Int history_index)
{
    return (history_index + MAX_HISTORY_COUNT) % MAX_HISTORY_COUNT;
}

Void push_game_history(GameState *state, GameMove move)
{
    ASSERT(state->history_count >= state->undo_count);
    state->history_index = wrap_history_index(state->history_index - state->undo_count);
    state->history_count -= state->undo_count;
    state->undo_count = 0;

    state->history[state->history_index] = move;
    state->history_index = wrap_history_index(state->history_index + 1);
    state->history_count = MIN(MAX_HISTORY_COUNT, state->history_count + 1);
}

Void record_game_move(GameState *state, GameMove move)
{
    Square capture_square = get_capture_square(move);
    if (capture_square != NO_SQUARE)
    {
        GamePiece captured_piece = remove_game_peice(state, capture_square);
        ASSERT(!is_empty(captured_piece) && captured_piece == get_captured_piece(move));
    }

    Square square_from = get_from(move);
    Square square_to = get_to(move);
    GamePiece piece = remove_game_peice(state, square_from);
    GameSideEnum side = get_side(piece);
    GamePieceTypeEnum piece_type = get_piece_type(piece);
    add_game_piece(state, square_to, piece);

    GameMoveTypeEnum move_type = get_move_type(move);
    if (move_type == GameMoveType::castling)
    {
        GameMove rook_move = get_castling_rook_move(move);
        Square rook_square_from = get_from(rook_move);
        Square rook_square_to = get_to(rook_move);
        GamePiece rook_piece = remove_game_peice(state, rook_square_from);
        ASSERT(get_piece_type(rook_piece) == GamePieceType::rook);
        add_game_piece(state, rook_square_to, rook_piece);
    }

    if (move_type == GameMoveType::promotion)
    {
        ASSERT(piece_type == GamePieceType::pawn);
        GamePieceTypeEnum promotion_piece_type = promotion_list[get_promotion_index(move)];
        GamePiece promotion_piece = get_game_piece(side, promotion_piece_type);
        add_game_piece(state, square_to, promotion_piece);
    }

    Square king_square = side == GameSide::white ? 4 : 4 + 56;
    Square rook_square_queen_side = side == GameSide::white ? 0 : 0 + 56;
    Square rook_square_king_side = side == GameSide::white ? 7 : 7 + 56;
    if (square_from == king_square || square_to == king_square)
    {
        state->castling -= castling_for(state->castling, side, GameCastlingMask::both_side);
    }
    if (square_from == rook_square_queen_side || square_to == rook_square_queen_side)
    {
        state->castling -= castling_for(state->castling, side, GameCastlingMask::queen_side);
    }
    if (square_from == rook_square_king_side || square_to == rook_square_king_side)
    {
        state->castling -= castling_for(state->castling, side, GameCastlingMask::king_side);
    }

    if (piece_type == GamePieceType::pawn && ABS(square_to - square_from) == 16)
    {
        state->en_passant = (square_from + square_to) / 2;
    }

    state->current_side = oppose(state->current_side);
}

Void record_game_move_with_history(GameState *state, GameMove move)
{
    record_game_move(state, move);
    push_game_history(state, move);
}

Void rollback_game_move(GameState *state, GameMove move)
{
    Square square_from = get_from(move);
    Square square_to = get_to(move);
    GameMoveTypeEnum move_type = get_move_type(move);
    GamePiece piece = state->board[square_to];
    if (move_type == GameMoveType::promotion)
    {
        remove_game_peice(state, square_to);
        GameSideEnum side = get_side(move);
        GamePiece pawn_piece = get_game_piece(side, GamePieceType::pawn);
        add_game_piece(state, square_to, pawn_piece);
    }

    if (move_type == GameMoveType::castling)
    {
        GameMove rook_move = get_castling_rook_move(move);
        Square rook_square_from = get_from(rook_move);
        Square rook_square_to = get_to(rook_move);
        GamePiece rook_piece = remove_game_peice(state, rook_square_to);
        ASSERT(get_piece_type(rook_piece) == GamePieceType::rook);
        add_game_piece(state, rook_square_from, rook_piece);
    }

    piece = remove_game_peice(state, square_to);
    add_game_piece(state, square_from, piece);

    Square capture_square = get_capture_square(move);
    if (capture_square != NO_SQUARE)
    {
        GamePiece captured_piece = get_captured_piece(move);
        add_game_piece(state, capture_square, captured_piece);
    }

    state->castling = get_castling(move);
    state->en_passant = get_en_passant(move);

    state->current_side = oppose(state->current_side);
}

Bool is_move_legal(GameState *state, GameMove move)
{
    Square square_from = get_from(move);
    Square square_to = get_to(move);
    GameMoveTypeEnum move_type = get_move_type(move);

    GamePiece piece = state->board[square_from];
    GameSideEnum side = get_side(piece);
    if (move_type == GameMoveType::castling)
    {
        Int step = square_to > square_from ? 1 : -1;
        for (Square square = square_from; square <= square_to; square += step)
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
        Square king_square = first_set(get_occupancy(state, side, GamePieceType::king));
        ASSERT(king_square != NO_SQUARE);
        BitBoard attack_by = check_attack_by(state, king_square, side);
        rollback_game_move(state, move);
        return !attack_by;
    }
}

Bool undo(GameState *state, GameMove *move)
{
    if (state->undo_count < state->history_count)
    {
        ASSERT(state->undo_count >= 0);
        state->undo_count++;
        Int history_index = wrap_history_index(state->history_index - state->undo_count);
        *move = state->history[history_index];
        rollback_game_move(state, *move);
        return true;
    }
    else
    {
        return false;
    }
}

Bool redo(GameState *state, GameMove *move)
{
    if (state->undo_count)
    {
        ASSERT(state->undo_count <= state->history_count);
        Int history_index = wrap_history_index(state->history_index - state->undo_count);
        state->undo_count--;
        *move = state->history[history_index];
        record_game_move(state, *move);
        return true;
    }
    else
    {
        return false;
    }
}
