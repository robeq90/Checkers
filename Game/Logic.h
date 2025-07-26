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

    // Finds the best sequence of moves for the player of specified color using minimax search.
    vector<move_pos> find_best_turns(const bool color)
    {
        next_best_state.clear();
        next_move.clear();

        // Запускаем поиск с начальными параметрами. Координаты -1, -1 означают поиск со всей доски.
        find_first_best_turn(board->get_board(), color, -1, -1, 0);

        int cur_state = 0;
        vector<move_pos> res;
        // Собираем цепочку ходов из next_move и next_best_state
        do
        {
            res.push_back(next_move[cur_state]);
            cur_state = next_best_state[cur_state];
        } while (cur_state != -1 && next_move[cur_state].x != -1);
        return res;
    }

private:
    // Первичная часть поиска с учётом обязательных ходов со взятием
    double find_first_best_turn(vector<vector<POS_T>> mtx, const bool color, const POS_T x, const POS_T y,
        size_t state, double alpha = -1)
    {
        next_best_state.push_back(-1);
        next_move.emplace_back(-1, -1, -1, -1);
        double best_score = -1;

        if (state != 0)
            find_turns(x, y, mtx);

        auto turns_now = turns;
        bool have_beats_now = have_beats;

        if (!have_beats_now && state != 0)
        {
            return find_best_turns_rec(mtx, 1 - color, 0, alpha);
        }

        for (auto turn : turns_now)
        {
            size_t next_state = next_move.size();
            double score;

            if (have_beats_now)
            {
                // Рекурсивный вызов с тем же цветом при взятии
                score = find_first_best_turn(make_turn(mtx, turn), color, turn.x2, turn.y2, next_state, best_score);
            }
            else
            {
                // Ход без взятия — рекурсивный вызов с другим цветом и глубиной 0
                score = find_best_turns_rec(make_turn(mtx, turn), 1 - color, 0, best_score);
            }

            if (score > best_score)
            {
                best_score = score;
                next_best_state[state] = (have_beats_now ? int(next_state) : -1);
                next_move[state] = turn;
            }
        }
        return best_score;
    }

    // Рекурсивный minimax с alpha-beta
    double find_best_turns_rec(vector<vector<POS_T>> mtx, const bool color, const size_t depth,
        double alpha = -1, double beta = INF + 1, const POS_T x = -1, const POS_T y = -1)
    {
        if (depth == Max_depth)
        {
            return calc_score(mtx, (depth % 2 == color));
        }

        if (x != -1)
        {
            find_turns(x, y, mtx);
        }
        else
        {
            find_turns(color, mtx);
        }

        auto turns_now = turns;
        bool have_beats_now = have_beats;

        if (!have_beats_now && x != -1)
        {
            return find_best_turns_rec(mtx, 1 - color, depth + 1, alpha, beta);
        }

        if (turns.empty())
            return (depth % 2 ? 0 : INF);

        double min_score = INF + 1;
        double max_score = -1;

        for (auto turn : turns_now)
        {
            double score = 0.0;

            if (!have_beats_now && x == -1)
            {
                score = find_best_turns_rec(make_turn(mtx, turn), 1 - color, depth + 1, alpha, beta);
            }
            else
            {
                score = find_best_turns_rec(make_turn(mtx, turn), color, depth, alpha, beta, turn.x2, turn.y2);
            }

            min_score = std::min(min_score, score);
            max_score = std::max(max_score, score);

            // Alpha-beta отсечение
            if (depth % 2)
                alpha = std::max(alpha, max_score);
            else
                beta = std::min(beta, min_score);

            if (optimization != "O0" && alpha >= beta)
                return (depth % 2 ? max_score + 1 : min_score - 1);
        }

        return (depth % 2 ? max_score : min_score);
    }

    // Прочие функции и переменные остаются без изменений

    // Прочие методы...

    // Applies the specified move 'turn' to the given board matrix 'mtx'.
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

    // Calculates score of the board from bot perspective
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

