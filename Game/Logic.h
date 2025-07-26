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


    vector<move_pos> find_best_turns(const bool color)
    {
        next_best_state.clear();
        next_move.clear();

        find_first_best_turn(board->get_board(), color, -1, -1, 0);

        int cur_state = 0;
        vector<move_pos> res;
        do
        {
            res.push_back(next_move[cur_state]);
            cur_state = next_best_state[cur_state];
        } while (cur_state != -1 && next_move[cur_state].x != -1);
        return res;
    }

private:
    // Applies the specified move 'turn' to the given board matrix 'mtx'.
// If the move includes capturing, removes the beaten piece from the board.
// Also handles promoting a piece to a queen if it reaches the opposite end of the board.
// Returns the updated board matrix after applying the move.
    vector<vector<POS_T>> make_turn(vector<vector<POS_T>> mtx, move_pos turn) const
    {
        // Remove beaten piece from the board if move includes a capture
        if (turn.xb != -1)
            mtx[turn.xb][turn.yb] = 0;

        // Promote to queen if piece reaches the opposite end row
        if ((mtx[turn.x][turn.y] == 1 && turn.x2 == 0) || (mtx[turn.x][turn.y] == 2 && turn.x2 == 7))
            mtx[turn.x][turn.y] += 2;

        // Move the piece to the new position on the board
        mtx[turn.x2][turn.y2] = mtx[turn.x][turn.y];

        // Clear the old position
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

        // Count number of white pieces and white queens,
        // as well as black pieces and black queens, on the board
        for (POS_T i = 0; i < 8; ++i)
        {
            for (POS_T j = 0; j < 8; ++j)
            {
                w += (mtx[i][j] == 1);
                wq += (mtx[i][j] == 3);
                b += (mtx[i][j] == 2);
                bq += (mtx[i][j] == 4);

                // If scoring mode includes positional potential, add small bonus based on piece's position
                if (scoring_mode == "NumberAndPotential")
                {
                    // White pieces get more points the closer they are to promotion row (row 0)
                    w += 0.05 * (mtx[i][j] == 1) * (7 - i);
                    // Black pieces get more points the closer they are to promotion row (row 7)
                    b += 0.05 * (mtx[i][j] == 2) * (i);
                }
            }
        }

        // If the bot player is black (not first_bot_color),
        // swap white and black totals to evaluate from bot's point of view
        if (!first_bot_color)
        {
            swap(b, w);
            swap(bq, wq);
        }

        // If one side has no pieces left, return win/loss scores
        if (w + wq == 0)
            return INF;   // Bot lost (white side has no pieces)
        if (b + bq == 0)
            return 0;     // Opponent lost

        int q_coef = 4;   // Queen's value coefficient

        // Adjust queen coefficient if positional potential scoring is enabled
        if (scoring_mode == "NumberAndPotential")
        {
            q_coef = 5;
        }

        // Calculate and return the evaluation score as a ratio of opponent's piece value to bot's
        return (b + bq * q_coef) / (w + wq * q_coef);
    }



    double find_first_best_turn(vector<vector<POS_T>> mtx, const bool color, const POS_T x, const POS_T y, size_t state,
                                double alpha = -1)
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

        vector<move_pos> best_moves;
        vector<int> best_states;

        for (auto turn : turns_now)
        {
            size_t next_state = next_move.size();
            double score;
            if (have_beats_now)
            {
                score = find_first_best_turn(make_turn(mtx, turn), color, turn.x2, turn.y2, next_state, best_score);
            }
            else
            {
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

    double find_best_turns_rec(vector<vector<POS_T>> mtx, const bool color, const size_t depth, double alpha = -1,
                               double beta = INF + 1, const POS_T x = -1, const POS_T y = -1)
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
            find_turns(color, mtx);
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
            min_score = min(min_score, score);
            max_score = max(max_score, score);
            // alpha-beta pruning
            if (depth % 2)
                alpha = max(alpha, max_score);
            else
                beta = min(beta, min_score);
            if (optimization != "O0" && alpha >= beta)
                return (depth % 2 ? max_score + 1 : min_score - 1);
        }
        return (depth % 2 ? max_score : min_score);
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
    void find_turns(const bool color, const vector<vector<POS_T>> &mtx)
    {
        vector<move_pos> res_turns;
        bool have_beats_before = false;
        for (POS_T i = 0; i < 8; ++i)
        {
            for (POS_T j = 0; j < 8; ++j)
            {
                if (mtx[i][j] && mtx[i][j] % 2 != color)
                {
                    find_turns(i, j, mtx);
                    if (have_beats && !have_beats_before)
                    {
                        have_beats_before = true;
                        res_turns.clear();
                    }
                    if ((have_beats_before && have_beats) || !have_beats_before)
                    {
                        res_turns.insert(res_turns.end(), turns.begin(), turns.end());
                    }
                }
            }
        }
        turns = res_turns;
        shuffle(turns.begin(), turns.end(), rand_eng);
        have_beats = have_beats_before;
    }

    // Find all possible moves for the entire board for a given player color.
// This function populates the vector 'turns' with valid moves the player can make.
// It also sets 'have_beats' flag if any capture moves are available.
// Uses the current board state retrieved from the Board object.
    void find_turns(const bool color)
    {
        // Calls the private version with the board matrix from the Board class
        find_turns(color, board->get_board());
    }

    // Find all possible moves for a single piece located at (x, y).
    // This function populates the vector 'turns' with valid moves for the specified piece.
    // It considers capture moves (beats) first, then normal moves if no captures.
    // Uses the current board state retrieved from the Board object.
    void find_turns(const POS_T x, const POS_T y)
    {
        // Calls the private version with the board matrix from the Board class
        find_turns(x, y, board->get_board());
    }

    // Private helper function:
    // Finds all possible moves for all pieces of the given color on the board state 'mtx'.
    // It gathers capture moves first if any exist (mandatory capture rule).
    // Stores all found moves in 'turns' vector and sets 'have_beats' accordingly.
    // Shuffles moves randomly to add variation to AI move order.
    void find_turns(const bool color, const vector<vector<POS_T>>& mtx)
    {
        vector<move_pos> res_turns;
        bool have_beats_before = false;
        for (POS_T i = 0; i < 8; ++i)
        {
            for (POS_T j = 0; j < 8; ++j)
            {
                // If square is occupied by piece of specified color (odd/even check for color)
                if (mtx[i][j] && mtx[i][j] % 2 != color)
                {
                    // Find moves from this piece's position
                    find_turns(i, j, mtx);

                    // Update capture move info and maintain only capture moves if any
                    if (have_beats && !have_beats_before)
                    {
                        have_beats_before = true;
                        res_turns.clear(); // Clear previously collected normal moves
                    }
                    // Add moves if captures are allowed or no captures found yet
                    if ((have_beats_before && have_beats) || !have_beats_before)
                    {
                        res_turns.insert(res_turns.end(), turns.begin(), turns.end());
                    }
                }
            }
        }
        turns = res_turns;
        // Randomly shuffle moves to add non-determinism
        shuffle(turns.begin(), turns.end(), rand_eng);
        have_beats = have_beats_before;
    }
    // Private helper function:
    // Finds all valid moves for a piece at position (x,y) on the given board matrix 'mtx'.
    // It checks for capture ("beat") moves first, which are mandatory in checkers.
    // Separate logic is applied for normal pieces (1 - white, 2 - black)
    // and queens (3,4), since queens can move multiple steps diagonally.
    // The found moves are stored in the 'turns' vector and the capture flag is set.
    void find_turns(const POS_T x, const POS_T y, const vector<vector<POS_T>>& mtx)
    {
        turns.clear();      // Clear previous moves from vector
        have_beats = false; // Reset capture availability flag

        POS_T type = mtx[x][y]; // Get piece type at the position

        // Check for capture moves first, as they have priority
        switch (type)
        {
        case 1:
        case 2:
            // Normal piece capture logic:
            // Check the four possible jump positions two steps diagonally
            for (POS_T i = x - 2; i <= x + 2; i += 4)
            {
                for (POS_T j = y - 2; j <= y + 2; j += 4)
                {
                    // Skip out-of-bound positions
                    if (i < 0 || i > 7 || j < 0 || j > 7)
                        continue;

                    // Coordinates of the piece being "jumped over"
                    POS_T xb = (x + i) / 2, yb = (y + j) / 2;

                    // Conditions for valid capture:
                    // - target cell empty (mtx[i][j] == 0)
                    // - jumped piece present and belongs to opponent (!= type%2)
                    if (mtx[i][j] || !mtx[xb][yb] || mtx[xb][yb] % 2 == type % 2)
                        continue;

                    // Add capture move to vector with beaten piece coordinates
                    turns.emplace_back(x, y, i, j, xb, yb);
                }
            }
            break;

        default:
            // Queen capture logic:
            // Queens can move and capture multiple squares diagonally
            for (POS_T i = -1; i <= 1; i += 2)
            {
                for (POS_T j = -1; j <= 1; j += 2)
                {
                    POS_T xb = -1, yb = -1; // Coordinates of opponent's piece being potentially captured
                    for (POS_T i2 = x + i, j2 = y + j; i2 >= 0 && i2 < 8 && j2 >= 0 && j2 < 8; i2 += i, j2 += j)
                    {
                        if (mtx[i2][j2])
                        {
                            // Cannot jump own piece or jump over more than one opponent piece
                            if (mtx[i2][j2] % 2 == type % 2 || (mtx[i2][j2] % 2 != type % 2 && xb != -1))
                            {
                                break;
                            }
                            xb = i2;
                            yb = j2;
                        }
                        // If a capture is possible after opponent piece, add move
                        if (xb != -1 && (i2 != xb))
                        {
                            turns.emplace_back(x, y, i2, j2, xb, yb);
                        }
                    }
                }
            }
            break;
        }

        // If capture moves found, skip normal moves (mandatory capture rule)
        if (!turns.empty())
        {
            have_beats = true;
            return;
        }

        // If no captures, find normal (non-capturing) moves next

        switch (type)
        {
        case 1:
        case 2:
            // Normal piece moves:
            // Move one step diagonally forward (direction depends on piece color)
        {
            POS_T i = ((type % 2) ? x - 1 : x + 1); // Move direction depends on color
            for (POS_T j = y - 1; j <= y + 1; j += 2)
            {
                if (i < 0 || i > 7 || j < 0 || j > 7 || mtx[i][j])
                    continue;
                // Add normal move (no beat coordinates)
                turns.emplace_back(x, y, i, j);
            }
            break;
        }
        default:
            // Queen normal moves:
            // Move any number of empty squares diagonally
            for (POS_T i = -1; i <= 1; i += 2)
            {
                for (POS_T j = -1; j <= 1; j += 2)
                {
                    for (POS_T i2 = x + i, j2 = y + j; i2 >= 0 && i2 < 8 && j2 >= 0 && j2 < 8; i2 += i, j2 += j)
                    {
                        if (mtx[i2][j2])
                            break; // Blocked by any piece
                        // Add legal move positions
                        turns.emplace_back(x, y, i2, j2);
                    }
                }
            }
            break;
        }
    }



  public:
      // Vector holding all currently found valid moves for the active player.
      vector<move_pos> turns;

      // Flag indicating whether there are any capturing moves available.
      bool have_beats;

      // Maximum recursion depth for the minimax search used by the bot.
      int Max_depth;

  private:
      // Random number engine used to shuffle moves to introduce randomness in AI decisions.
      default_random_engine rand_eng;

      // String specifying the scoring method used during board evaluation.
      // Examples might be "NumberAndPotential" or other scoring schemes.
      string scoring_mode;

      // String indicating the optimization level or technique applied during search.
      string optimization;

      // Vector storing the best next moves for minimax states during search.
      vector<move_pos> next_move;

      // Vector storing the indices of the best next states in the minimax search tree.
      vector<int> next_best_state;

      // Pointer to the game Board object that holds the current game state.
      Board* board;

      // Pointer to the Config object that holds game and bot configuration values.
      Config  *config;
};

