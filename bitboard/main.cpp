#include "../lib/util.hpp"
#include <cstdio>
#include <stdlib.h>

typedef UInt64 BitBoard;

UInt64 get_bit_count(UInt64 x)
{
    UInt64 result = 0;
    while (x)
    {
        UInt64 lsb = x & -x;
        x -= lsb;
        result++;
    }
    return result;
}

Void print_bit_board(BitBoard board)
{
    for (Int i = 63; i >= 0; i--)
    {
        printf("%d", board & (1ull << i) ? 1 : 0);
        if (i % 8 == 0)
        {
            printf("\n");
        }
    }
}

UInt64 random64()
{
    UInt64 u1, u2, u3, u4;
    u1 = (UInt64)(rand()) & 0xFFFF;
    u2 = (UInt64)(rand()) & 0xFFFF;
    u3 = (UInt64)(rand()) & 0xFFFF;
    u4 = (UInt64)(rand()) & 0xFFFF;
    return u1 | (u2 << 16) | (u3 << 32) | (u4 << 48);
}

BitBoard get_square(BitBoard row, BitBoard column)
{
    ASSERT(row >= 0 && row < 8);
    ASSERT(column >= 0 && column < 8);
    return 1ull << (row * 8ull + column);
}

BitBoard get_blocker(BitBoard square, UInt64 blocker_mask, UInt64 blocker_bit_count, Int blocker_i)
{
    BitBoard blocker = 0;
    for (Int bit = 0; bit < (Int)blocker_bit_count; bit++)
    {
        BitBoard blocker_bit = blocker_mask & -blocker_mask;
        blocker_mask -= blocker_bit;
        if ((UInt64)blocker_i & (1ull << bit))
        {
            blocker |= blocker_bit;
        }
    }
    return blocker;
}

UInt64 calc_hash(BitBoard blocker, UInt64 guess, UInt64 blocker_bit_count)
{
    UInt64 hash = (blocker * guess) >> (64 - blocker_bit_count);
    return hash;
}

BitBoard get_rook_blocker_mask(BitBoard square)
{
    Int square_row = square / 8;
    Int square_column = square % 8;
    BitBoard result = 0;
    for (Int row = square_row + 1; row < 7; row++)
    {
        BitBoard square = get_square(row, square_column);
        result |= square;
    }
    for (Int row = square_row - 1; row >= 1; row--)
    {
        BitBoard square = get_square(row, square_column);
        result |= square;
    }
    for (Int column = square_column + 1; column < 7; column++)
    {
        BitBoard square = get_square(square_row, column);
        result |= square;
    }
    for (Int column = square_column - 1; column >= 1; column--)
    {
        BitBoard square = get_square(square_row, column);
        result |= square;
    }
    return result;
}

BitBoard get_rook_move(BitBoard square, BitBoard blocker)
{
    Int square_row = square / 8;
    Int square_column = square % 8;
    BitBoard result = 0;
    for (Int row = square_row + 1; row < 8; row++)
    {
        BitBoard square = get_square(row, square_column);
        result |= square;
        if (blocker & square)
        {
            break;
        }
    }
    for (Int row = square_row - 1; row >= 0; row--)
    {
        BitBoard square = get_square(row, square_column);
        result |= square;
        if (blocker & square)
        {
            break;
        }
    }
    for (Int column = square_column + 1; column < 8; column++)
    {
        BitBoard square = get_square(square_row, column);
        result |= square;
        if (blocker & square)
        {
            break;
        }
    }
    for (Int column = square_column - 1; column >= 0; column--)
    {
        BitBoard square = get_square(square_row, column);
        result |= square;
        if (blocker & square)
        {
            break;
        }
    }
    return result;
}

struct SlidingPiece
{
    BitBoard blocker_mask[64];
    UInt64 blocker_bit_count[64];
    BitBoard blocker[64][1 << 13];
    BitBoard move[64][1 << 13];
    UInt64 magic[64];

    BitBoard (*get_blocker_mask)(BitBoard square);
    BitBoard (*get_move)(BitBoard square, BitBoard blocker);
};

BitBoard get_bishop_blocker_mask(BitBoard square)
{
    Int square_row = square / 8;
    Int square_column = square % 8;
    BitBoard result = 0;
    for (Int row = square_row + 1, column = square_column + 1; row < 7 && column < 7; row++, column++)
    {
        BitBoard square = get_square(row, column);
        result |= square;
    }
    for (Int row = square_row - 1, column = square_column - 1; row >= 1 && column >= 1; row--, column--)
    {
        BitBoard square = get_square(row, column);
        result |= square;
    }
    for (Int row = square_row + 1, column = square_column - 1; row < 7 && column >= 1; row++, column--)
    {
        BitBoard square = get_square(row, column);
        result |= square;
    }
    for (Int row = square_row - 1, column = square_column + 1; row >= 1 && column < 7; row--, column++)
    {
        BitBoard square = get_square(row, column);
        result |= square;
    }
    return result;
}

BitBoard get_bishop_move(BitBoard square, BitBoard blocker)
{
    Int square_row = square / 8;
    Int square_column = square % 8;
    BitBoard result = 0;
    for (Int row = square_row + 1, column = square_column + 1; row < 8 && column < 8; row++, column++)
    {
        BitBoard square = get_square(row, column);
        result |= square;
        if (blocker & square)
        {
            break;
        }
    }
    for (Int row = square_row - 1, column = square_column - 1; row >= 0 && column >= 0; row--, column--)
    {
        BitBoard square = get_square(row, column);
        result |= square;
        if (blocker & square)
        {
            break;
        }
    }
    for (Int row = square_row + 1, column = square_column - 1; row < 8 && column >= 0; row++, column--)
    {
        BitBoard square = get_square(row, column);
        result |= square;
        if (blocker & square)
        {
            break;
        }
    }
    for (Int row = square_row - 1, column = square_column + 1; row >= 0 && column < 8; row--, column++)
    {
        BitBoard square = get_square(row, column);
        result |= square;
        if (blocker & square)
        {
            break;
        }
    }
    return result;
}

BitBoard get_king_move(BitBoard square)
{
    Int square_row = square / 8;
    Int square_column = square % 8;
    Int row;
    Int column;
    BitBoard result = 0;

    row = square_row + 2;
    column = square_column + 1;
    if (row >= 0 && row < 8 && column >= 0 && column < 8)
    {
        BitBoard square = get_square(row, column);
        result |= square;
    }
    row = square_row + 1;
    column = square_column + 2;
    if (row >= 0 && row < 8 && column >= 0 && column < 8)
    {
        BitBoard square = get_square(row, column);
        result |= square;
    }
    row = square_row + 2;
    column = square_column - 1;
    if (row >= 0 && row < 8 && column >= 0 && column < 8)
    {
        BitBoard square = get_square(row, column);
        result |= square;
    }
    row = square_row + 1;
    column = square_column - 2;
    if (row >= 0 && row < 8 && column >= 0 && column < 8)
    {
        BitBoard square = get_square(row, column);
        result |= square;
    }
    row = square_row - 2;
    column = square_column - 1;
    if (row >= 0 && row < 8 && column >= 0 && column < 8)
    {
        BitBoard square = get_square(row, column);
        result |= square;
    }
    row = square_row - 1;
    column = square_column - 2;
    if (row >= 0 && row < 8 && column >= 0 && column < 8)
    {
        BitBoard square = get_square(row, column);
        result |= square;
    }
    row = square_row - 2;
    column = square_column + 1;
    if (row >= 0 && row < 8 && column >= 0 && column < 8)
    {
        BitBoard square = get_square(row, column);
        result |= square;
    }
    row = square_row - 1;
    column = square_column + 2;
    if (row >= 0 && row < 8 && column >= 0 && column < 8)
    {
        BitBoard square = get_square(row, column);
        result |= square;
    }
    return result;
}

struct Pawn
{
    BitBoard move[64];
    BitBoard capture[64];
};

BitBoard get_pawn_move(BitBoard square, Int direction)
{
    Int square_row = square / 8;
    Int square_column = square % 8;
    BitBoard result = 0;

    if ((8 + direction * 2) % 8 == 0)
    {
        for (Int step = 1; step <= 2; step++)
        {
            Int row = square_row + step * direction;
            if (row >= 0 && row < 8)
            {
                BitBoard square = get_square(row, square_column);
                result |= square;
            }
        }
    }
    else
    {
        Int row = square_row + direction;
        if (row >= 0 && row < 8)
        {
            BitBoard square = get_square(row, square_column);
            result |= square;
        }
    }
    return result;
}

BitBoard get_pawn_capture(BitBoard square, Int direction)
{
    Int square_row = square / 8;
    Int square_column = square % 8;
    BitBoard result = 0;

    Int row = square_row + direction;
    Int column;

    column = square_column - 1;
    if (row >= 0 && row < 8 && column >= 0 && column < 8)
    {
        BitBoard square = get_square(row, column);
        result |= square;
    }
    column = square_column + 1;
    if (row >= 0 && row < 8 && column >= 0 && column < 8)
    {
        BitBoard square = get_square(row, column);
        result |= square;
    }
    return result;
}

SlidingPiece sliding_pieces[2];
BitBoard knight[64];
BitBoard king[64];
Pawn pawns[2];

int main(Int argc, CStr *argv)
{
    argc--, argv++;
    ASSERT(argc == 1);
    CStr output_filename = argv[0];

    sliding_pieces[0].get_blocker_mask = get_rook_blocker_mask;
    sliding_pieces[0].get_move = get_rook_move;
    sliding_pieces[1].get_blocker_mask = get_bishop_blocker_mask;
    sliding_pieces[1].get_move = get_bishop_move;

    for (Int square = 0; square < 64; square++)
    {
        for (Int piece_type = 0; piece_type <= 0; piece_type++)
        {
            SlidingPiece *sliding_piece = &sliding_pieces[piece_type];

            BitBoard blocker_mask = get_rook_blocker_mask(square);
            sliding_piece->blocker_mask[square] = blocker_mask;

            UInt64 blocker_bit_count = get_bit_count(blocker_mask);
            sliding_piece->blocker_bit_count[square] = blocker_bit_count;

            UInt64 blocker_count = 1 << blocker_bit_count;
            for (Int blocker_i = 0; blocker_i < (Int)blocker_count; blocker_i++)
            {
                BitBoard blocker = get_blocker(square, blocker_mask, blocker_bit_count, blocker_i);
                sliding_piece->blocker[square][blocker_i] = blocker;
            }

            while (true)
            {
                UInt64 guess = random64() & random64() & random64();
                Bool success = true;
                Bool used[1 << 13];
                memset(used, 0, sizeof(used));
                for (Int blocker_i = 0; blocker_i < (Int)blocker_count; blocker_i++)
                {
                    BitBoard blocker = sliding_piece->blocker[square][blocker_i];
                    UInt64 hash = calc_hash(blocker, guess, blocker_bit_count);
                    if (used[hash])
                    {
                        success = false;
                        break;
                    }
                    used[hash] = 1;
                }

                if (success)
                {
                    for (Int blocker_i = 0; blocker_i < (Int)blocker_count; blocker_i++)
                    {
                        BitBoard blocker = sliding_piece->blocker[square][blocker_i];
                        UInt64 hash = calc_hash(blocker, guess, blocker_bit_count);
                        BitBoard move = sliding_piece->get_move(square, blocker);
                        sliding_piece->move[square][hash] = move;
                    }
                    sliding_piece->magic[square] = guess;
                    break;
                }
            }
        }

        knight[square] = get_king_move(square);
        king[square] = get_king_move(square);
        pawns[0].move[square] = get_pawn_move(square, 1);
        pawns[0].capture[square] = get_pawn_capture(square, 1);
        pawns[1].move[square] = get_pawn_move(square, -1);
        pawns[1].capture[square] = get_pawn_capture(square, -1);
    }

    FILE *output_file = fopen(output_filename, "wb");
    ASSERT(fseek(output_file, 0, SEEK_SET) == 0);

    for (Int square = 0; square < 64; square++)
    {
        ASSERT(fwrite(&sliding_pieces[0].blocker_mask[square], sizeof(UInt64), 1, output_file) == 1);
        ASSERT(fwrite(&sliding_pieces[0].blocker_bit_count[square], sizeof(UInt64), 1, output_file) == 1);
        ASSERT(fwrite(&sliding_pieces[0].magic[square], sizeof(UInt64), 1, output_file) == 1);
        Int blocker_count = 1 << sliding_pieces[0].blocker_bit_count[square];
        for (Int blocker_i = 0; blocker_i < blocker_count; blocker_i++)
        {
            ASSERT(fwrite(&sliding_pieces[0].move[square][blocker_i], sizeof(UInt64), 1, output_file) == 1);
        }
    }

    for (Int square = 0; square < 64; square++)
    {
        ASSERT(fwrite(&knight[0], sizeof(UInt64), 1, output_file) == 1);
    }

    for (Int square = 0; square < 64; square++)
    {
        ASSERT(fwrite(&sliding_pieces[1].blocker_mask[square], sizeof(UInt64), 1, output_file) == 1);
        ASSERT(fwrite(&sliding_pieces[1].blocker_bit_count[square], sizeof(UInt64), 1, output_file) == 1);
        ASSERT(fwrite(&sliding_pieces[1].magic[square], sizeof(UInt64), 1, output_file) == 1);
        Int blocker_count = 1 << sliding_pieces[1].blocker_bit_count[square];
        for (Int blocker_i = 0; blocker_i < blocker_count; blocker_i++)
        {
            ASSERT(fwrite(&sliding_pieces[1].move[square][blocker_i], sizeof(UInt64), 1, output_file) == 1);
        }
    }

    for (Int square = 0; square < 64; square++)
    {
        ASSERT(fwrite(&king[0], sizeof(UInt64), 1, output_file) == 1);
    }

    for (Int i = 0; i < 2; i++)
    {
        for (Int square = 0; square < 64; square++)
        {
            ASSERT(fwrite(&pawns[i].move[square], sizeof(UInt64), 1, output_file) == 1);
            ASSERT(fwrite(&pawns[i].capture[square], sizeof(UInt64), 1, output_file) == 1);
        }
    }
}
