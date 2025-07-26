#pragma once
#include <chrono>
#include <thread>

#include "../Models/Project_path.h"
#include "Board.h"
#include "Config.h"
#include "Hand.h"
#include "Logic.h"
#include "../Models/Response.h"
class Game
{
public:
    Game() : board(config("WindowSize", "Width"), config("WindowSize", "Hight")), hand(&board), logic(&board, &config)
    {
        std::ofstream fout(project_path + "log.txt", std::ios_base::trunc);
        fout.close();
    }

    // Starts and runs the main game loop for the checkers game
    int play()
    {
        // Record start time to measure game duration
        auto start = chrono::steady_clock::now();

        // If replay requested, reset logic and settings, redraw board
        if (is_replay)
        {
            logic = Logic(&board, &config);
            config.reload();
            board.redraw();
        }
        else
        {
            // First-time start – draw initial board state
            board.start_draw();
        }
        // Clear replay flag
        is_replay = false;

        int turn_num = -1;       // Current turn number
        bool is_quit = false;    // Flag if the player quits
        const int Max_turns = config("Game", "MaxNumTurns");  // Maximum allowed turns

        // Main game loop: runs until max turns reached or game ends earlier
        while (++turn_num < Max_turns)
        {
            beat_series = 0;  // Reset consecutive capture count
            logic.find_turns(turn_num % 2);  // Find possible moves for current player (0 or 1)

            // If no possible moves, game ends
            if (logic.turns.empty())
                break;

            // Set AI depth (difficulty) from config based on player color
            logic.Max_depth = config("Bot", std::string((turn_num % 2) ? "Black" : "White") + std::string("BotLevel"));

            // Check if current player is human or bot
            if (!config("Bot", std::string("Is") + std::string((turn_num % 2) ? "Black" : "White") + std::string("Bot")))
            {
                // Human player turn - wait for player response
                auto resp = player_turn(turn_num % 2);

                // Handle player commands: quit, replay or undo
                if (resp == Response::QUIT)
                {
                    is_quit = true;
                    break;
                }
                else if (resp == Response::REPLAY)
                {
                    is_replay = true;
                    break;
                }
                else if (resp == Response::BACK)
                {
                    // Undo logic depending on bot presence and capture series
                    if (config("Bot", std::string("Is") + std::string((1 - turn_num % 2) ? "Black" : "White") + std::string("Bot")) &&
                        !beat_series && board.history_mtx.size() > 2)
                    {
                        board.rollback();
                        --turn_num;
                    }
                    if (!beat_series)
                        --turn_num;

                    board.rollback();
                    --turn_num;
                    beat_series = 0;
                }
            }
            else
            {
                // Bot player executes moves automatically
                bot_turn(turn_num % 2);
            }
        }

        // Log total game time
        auto end = chrono::steady_clock::now();
        std::ofstream fout(project_path + "log.txt", std::ios_base::app);
        fout << "Game time: " << (int)chrono::duration<double, milli>(end - start).count() << " millisec\n";
        fout.close();

        // If replay requested, restart the game recursively
        if (is_replay)
            return play();
        if (is_quit)
            return 0;

        int res = 2;  // Default: game ended without winner

        // Determine result based on turn count and player
        if (turn_num == Max_turns)
        {
            res = 0;  // Draw (max turns reached)
        }
        else if (turn_num % 2)
        {
            res = 1;  // Player 1 wins
        }

        // Show final board state with result
        board.show_final(res);

        // Wait for user response (e.g., replay request)
        auto resp = hand.wait();
        if (resp == Response::REPLAY)
        {
            is_replay = true;
            return play();
        }
        return res;
    };
 private:
    void bot_turn(const bool color)
    {
        auto start = chrono::steady_clock::now();

        auto delay_ms = config("Bot", "BotDelayMS");
        // new thread for equal delay for each turn
        thread th(SDL_Delay, delay_ms);
        auto turns = logic.find_best_turns(color);
        th.join();
        bool is_first = true;
        // making moves
        for (auto turn : turns)
        {
            if (!is_first)
            {
                SDL_Delay(delay_ms);
            }
            is_first = false;
            beat_series += (turn.xb != -1);
            board.move_piece(turn, beat_series);
        }

        auto end = chrono::steady_clock::now();
        ofstream fout(project_path + "log.txt", ios_base::app);
        fout << "Bot turn time: " << (int)chrono::duration<double, milli>(end - start).count() << " millisec\n";
        fout.close();
    }

    // Handles a human player's turn, receiving input and executing moves.
    Response player_turn(const bool color)
    {
        // Prepare vector with all possible cells to highlight (first moves)
        vector<pair<POS_T, POS_T>> cells;
        for (auto turn : logic.turns)
        {
            cells.emplace_back(turn.x, turn.y);
        }
        board.highlight_cells(cells);  // Highlight possible starting positions on the board

        move_pos pos = { -1, -1, -1, -1 };
        POS_T x = -1, y = -1;

        // Loop to get first move from player input (starting cell)
        while (true)
        {
            auto resp = hand.get_cell();          // Get user response (selected cell)
            if (std::get<0>(resp) != Response::CELL)
                return std::get<0>(resp);          // If special response (QUIT, REPLAY), return it

            pair<POS_T, POS_T> cell{ std::get<1>(resp), std::get<2>(resp) };

            bool is_correct = false;
            // Check if selected cell corresponds to a valid possible move start
            for (auto turn : logic.turns)
            {
                if (turn.x == cell.first && turn.y == cell.second)
                {
                    is_correct = true;
                    break;
                }
                if (turn == move_pos{ x, y, cell.first, cell.second })
                {
                    pos = turn;  // The player selected a valid target cell for moving from (x, y)
                    break;
                }
            }

            if (pos.x != -1)
                break;  // A complete move was selected, break loop

            if (!is_correct)
            {
                // If incorrect selection after choosing first cell,
                // reset highlights and states, allow re-selection
                if (x != -1)
                {
                    board.clear_active();
                    board.clear_highlight();
                    board.highlight_cells(cells);
                }
                x = -1; y = -1;
                continue;
            }

            // Remember selected starting position
            x = cell.first;
            y = cell.second;

            board.clear_highlight();
            board.set_active(x, y);  // Visually mark active cell for user clarity

            // Highlight possible destination cells from the chosen starting cell
            vector<pair<POS_T, POS_T>> cells2;
            for (auto turn : logic.turns)
            {
                if (turn.x == x && turn.y == y)
                {
                    cells2.emplace_back(turn.x2, turn.y2);
                }
            }
            board.highlight_cells(cells2);
        }

        board.clear_highlight();
        board.clear_active();

        // Execute the move on the board; pos.xb != -1 means a capture move
        board.move_piece(pos, pos.xb != -1);

        if (pos.xb == -1)  // If no capture, player's turn ends
            return Response::OK;

        // Continue capturing moves if possible (multi-capture turn)
        beat_series = 1;
        while (true)
        {
            logic.find_turns(pos.x2, pos.y2);  // Find further possible captures from new position
            if (!logic.have_beats)             // No more captures
                break;

            vector<pair<POS_T, POS_T>> cells;
            for (auto turn : logic.turns)
            {
                cells.emplace_back(turn.x2, turn.y2);
            }
            board.highlight_cells(cells);      // Highlight possible destinations for next capture
            board.set_active(pos.x2, pos.y2);  // Mark current active piece location

            // Loop to select next capture move from player input
            while (true)
            {
                auto resp = hand.get_cell();
                if (std::get<0>(resp) != Response::CELL)
                    return std::get<0>(resp);

                pair<POS_T, POS_T> cell{ std::get<1>(resp), std::get<2>(resp) };

                bool is_correct = false;
                for (auto turn : logic.turns)
                {
                    if (turn.x2 == cell.first && turn.y2 == cell.second)
                    {
                        is_correct = true;
                        pos = turn;
                        break;
                    }
                }
                if (!is_correct)
                    continue;

                board.clear_highlight();
                board.clear_active();
                beat_series += 1;                // Increase capture count
                board.move_piece(pos, beat_series);
                break;
            }
        }

        return Response::OK;  // Player finished move sequence
    }



  private:
    Config config;
    Board board;
    Hand hand;
    Logic logic;
    int beat_series;
    bool is_replay = false;
};
