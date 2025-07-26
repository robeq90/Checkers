#pragma once
#include <random>
#include <vector>

#include "../Models/Move.h"
#include "Board.h"
#include "Config.h"

const int INF = 1e9;

class Logic
{
public:
    // Constructor initializes Logic instance with pointers to the Board and Config objects.
    // Also initializes the random engine based on the "NoRandom" config flag,
    // and sets up scoring and optimization modes according to configuration.
    Logic(Board* board, Config* config)
        : board(board), config(config)
    {
        rand_eng = std::default_random_engine(
            !((*config)("Bot", "NoRandom")) ? unsigned(time(0)) : 0);
        scoring_mode = (*config)("Bot", "BotScoringType");
        optimization = (*config)("Bot", "Optimization");
    }

    // Declaration only: find_best_turns finds the best move sequence for the bot (to be implemented)
    vector<move_pos> find_best_turns(const bool color);

private:
    // Declaration only: find_first_best_turn for minimax search, to be reimplemented
    double find_first_best_turn(vector<vector<POS_T>> mtx, const bool color, const POS_T x, const POS_T y,
        size_t state, double alpha = -1);

    // Declaration only: recursive minimax evaluation function, to be reimplemented
    double find_best_turns_rec(vector<vector<POS_T>> mtx, const bool color, const size_t depth,
        double alpha = -1, double beta = INF + 1, const POS_T x = -1, const POS_T y = -1);

    // Applies the specified move 'turn' to the given board matrix 'mtx'.
    // If the move includes capturing, removes the beaten piece from the board.
    // Also handles promoting a piece to a queen if it reaches the opposite end of the board.
    // Returns the updated board matrix after applying the move.
    vector<vector<POS_T>> make_turn(vector<vector<POS_T>> mtx, move_pos turn) const
    {
        if (turn.xb != -1)
            mtx[turn.xb][turn.yb] = 0;

        if ((mtx[turn.x][turn.y] == 1 && turn.x2 == 0) || (mtx[turn.x][turn.y] == 2 && turn.x2 == 7))
            mtx[turn.x][turn.y] += 2;

        mtx[turn.x2][turn.y2] = mtx[turn.x][turn.y];
        mtx[turn.x][turn.y] = 0;

        return mtx;
    }

    // Calculates and returns a score evaluating the given board state 'mtx' from the perspective of 'first_bot_color'.
    // Considers the number of regular pieces and queens for each side.
    // Optionally incorporates positional potential if enabled by scoring_mode.
    // Lower values favor 'first_bot_color', higher values favor the opponent.
    double calc_score(const vector<vector<POS_T>>& mtx, const bool first_bot_color) const
    {
        double w = 0, wq = 0, b = 0, bq = 0;

        for (POS_T i = 0; i < 8; ++i)
        {
            for (POS_T j = 0; j < 8; ++j)
            {
                w += (mtx[i][j] == 1);
                wq += (mtx[i][j] == 3);
                b += (mtx[i][j] == 2);
                bq += (mtx[i][j] == 4);

                if (scoring_mode == "NumberAndPotential")
                {
                    w += 0.05 * (mtx[i][j] == 1) * (7 - i);
                    b += 0.05 * (mtx[i][j] == 2) * (i);
                }
            }
        }

        if (!first_bot_color)
        {
            swap(b, w);
            swap(bq, wq);
        }

        if (w + wq == 0)
            return INF;
        if (b + bq == 0)
            return 0;

        int q_coef = 4;
        if (scoring_mode == "NumberAndPotential")
            q_coef = 5;

        return (b + bq * q_coef) / (w + wq * q_coef);
    }

public:
    void find_turns(const bool color)
    {
        find_turns(color, board->get_board());
    }

    void find_turns(const POS_T x, const POS_T y)
    {
        find_turns(x, y, board->get_board());
    }

private:
    void find_turns(const bool color, const vector<vector<POS_T>>& mtx);
    void find_turns(const POS_T x, const POS_T y, const vector<vector<POS_T>>& mtx);

public:
    vector<move_pos> turns;
    bool have_beats;
    int Max_depth;

private:
    std::default_random_engine rand_eng;
    std::string scoring_mode;
    std::string optimization;
    std::vector<move_pos> next_move;
    std::vector<int> next_best_state;
    Board* board;
    Config* config;
};

