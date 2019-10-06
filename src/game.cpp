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
    BitBoard side_occupancy[2];
};

struct GameState
{
    GamePosition position;
    GameSideEnum player_side;
    GameSideEnum current_side;
    Int castling[2];
    Int en_passant[2];
};

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
    state->position.board[square] = piece;
    state->position.side_occupancy[side] |= bit_square(square);
}

GamePiece remove_game_peice(GameState *state, Int square)
{
    GamePiece piece = state->position.board[square];
    GameSideEnum side = get_side(piece);
    state->position.board[square] = NO_GAME_PIECE;
    state->position.side_occupancy[side] &= ~bit_square(square);
    return piece;
}

GameState get_initial_game_state()
{
    GameState state = {};
    memset(state.position.board, NO_GAME_PIECE, sizeof(state.position.board));
    state.player_side = GameSide::white;
    state.current_side = GameSide::white;
    state.castling[0] = state.castling[1] = GameCastlingMask::both_side;
    state.en_passant[0] = state.en_passant[1] = NO_SQUARE;

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
    BitBoard occupancy = state->position.side_occupancy[0] | state->position.side_occupancy[1];
    return occupancy;
}

BitBoard check_sliding_piece_move(GameState *state, Int square, GameSideEnum side, SlidingPieceTable *sliding_piece_table)
{
    SlidingPieceTableSquare *table_square = &sliding_piece_table->board[square];
    BitBoard occupancy = get_both_occupancy(state);
    BitBoard blocker_mask = table_square->blocker_mask;
    BitBoard blocker = blocker_mask & occupancy;
    UInt64 hash = (blocker * table_square->magic) >> (64 - table_square->blocker_bit_count);
    BitBoard move = table_square->move[hash] & ~state->position.side_occupancy[side];
    return move;
}

BitBoard check_simple_piece_move(GameState *state, Int square, GameSideEnum side, SimplePieceTable *simple_piece_table)
{
    BitBoard move = simple_piece_table->move[square] & ~state->position.side_occupancy[side];
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
    BitBoard square_bit = bit_square(square);
    BitBoard oppose_occupancy = state->position.side_occupancy[oppose(side)];
    Int en_passant_square = state->en_passant[oppose(side)];
    if (en_passant_square != NO_SQUARE)
    {
        oppose_occupancy |= bit_square(en_passant_square);
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
    BitBoard occupancy = get_both_occupancy(state);
    BitBoard move = 0;
    if (state->castling[side] & GameCastlingMask::queen_side)
    {
        BitBoard blocker_mask = side == GameSide::white ? 0xellu : 0xellu << 56;
        BitBoard occupancy = get_both_occupancy(state);
        if (!(blocker_mask & occupancy))
        {
            BitBoard move_to = side == GameSide::white ? 0x4llu : 0x4llu << 56;
            move |= move_to;
        }
    }

    if (state->castling[side] & GameCastlingMask::king_side)
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
    GamePieceTypeEnum piece_type = get_piece_type(piece);
    GameSideEnum side = get_side(piece);

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

BitBoard check_attack_by(GameState *state, Int square, GameSideEnum side, BitBoardTable *bit_board_table)
{
    return 0;
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

GameMove get_game_move(GameState *state, GamePiece piece, Int square_from, Int square_to)
{
    GameMove move = square_from | (square_to << 6);
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
    Int square_from = get_square_from(move);
    Int square_to = get_square_to(move);
    Int capture_square = get_capture_square(state, move);

    if (capture_square != NO_SQUARE)
    {
        remove_game_peice(state, capture_square);
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
    else if (move_type == GameMoveType::promotion)
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
        state->castling[side] &= ~GameCastlingMask::both_side;
    }
    if (square_from == rook_square_queen_side || square_to == rook_square_queen_side)
    {
        state->castling[side] &= ~GameCastlingMask::queen_side;
    }
    if (square_from == rook_square_king_side || square_to == rook_square_king_side)
    {
        state->castling[side] &= ~GameCastlingMask::king_side;
    }

    if (piece_type == GamePieceType::pawn && ABS(square_to - square_from) == 16)
    {
        state->en_passant[side] = (square_from + square_to) / 2;
    }
    else
    {
        state->en_passant[side] = NO_SQUARE;
    }
}
