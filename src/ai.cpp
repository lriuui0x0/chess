#pragma once

#include "game.cpp"

Void generate_all_moves(GameState *state, Buffer<GameMove> *moves)
{
    BitBoard all_pieces = state->occupancy_side[state->current_side];
    while (all_pieces)
    {
        Square square_from = first_set(all_pieces);
        all_pieces -= bit_square(square_from);

        BitBoard all_moves = check_game_move(state, square_from);
        while (all_moves)
        {
            Square square_to = first_set(all_moves);
            all_moves -= bit_square(square_to);

            GameMove move = get_game_move(state, square_from, square_to);
            if (is_move_legal(state, move))
            {
                moves->data[moves->count++] = move;
            }
        }
    }
}
