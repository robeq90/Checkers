#pragma once
#include <tuple>

#include "../Models/Move.h"
#include "../Models/Response.h"
#include "Board.h"

// The Hand class is responsible for handling player input (mouse, window events)
// and interacting with the Board object.
class Hand
{
public:
    // Constructor takes a pointer to the Board to access its properties and methods
    Hand(Board* board) : board(board)
    {
    }

    // The get_cell() method listens and processes events (window and mouse events),
    // waits for the player's action,
    // returns a tuple of three elements:
    //    - Response: event type (cell selected, quit, replay, undo, etc.)
    //    - POS_T xc, POS_T yc: coordinates of the selected cell on the board,
    //      or -1 if no valid cell was selected.
    tuple<Response, POS_T, POS_T> get_cell() const
    {
        SDL_Event windowEvent;           // SDL event object to receive events (mouse, window etc.)
        Response resp = Response::OK;    // Initial response status "OK" - keep listening
        int x = -1, y = -1;              // Mouse pixel coordinates
        int xc = -1, yc = -1;            // Board cell coordinates, -1 if invalid

        // Infinite loop to process events until needed event is received
        while (true)
        {
            if (SDL_PollEvent(&windowEvent))  // Poll for new SDL event in queue
            {
                switch (windowEvent.type)     // Handle event type
                {
                case SDL_QUIT:
                    // User closed the window
                    resp = Response::QUIT;
                    break;

                case SDL_MOUSEBUTTONDOWN:
                    // Mouse button pressed — get pixel coordinates
                    x = windowEvent.motion.x;
                    y = windowEvent.motion.y;

                    // Convert pixel coordinates to board cell coordinates
                    xc = int(y / (board->H / 10) - 1);
                    yc = int(x / (board->W / 10) - 1);

                    // Check if player clicked on special areas or valid cells:
                    if (xc == -1 && yc == -1 && board->history_mtx.size() > 1)
                    {
                        // Click outside the board with history — undo move
                        resp = Response::BACK;
                    }
                    else if (xc == -1 && yc == 8)
                    {
                        // Click on "replay" area — restart game replay
                        resp = Response::REPLAY;
                    }
                    else if (xc >= 0 && xc < 8 && yc >= 0 && yc < 8)
                    {
                        // Clicked on a valid board cell — return cell coordinates
                        resp = Response::CELL;
                    }
                    else
                    {
                        // Invalid click — reset coordinates to ignore
                        xc = -1;
                        yc = -1;
                    }
                    break;

                case SDL_WINDOWEVENT:
                    // Window event - if window resized, reset board size parameters
                    if (windowEvent.window.event == SDL_WINDOWEVENT_SIZE_CHANGED)
                    {
                        board->reset_window_size();
                        break;
                    }
                }

                // If event is not OK (waiting state), break the loop and return result
                if (resp != Response::OK)
                    break;
            }
        }
        return { resp, xc, yc };  // Return event type and cell coordinates
    }

    // The wait() method waits indefinitely for a user event,
    // returning a Response depending on user actions (e.g. quit or replay)
    Response wait() const
    {
        SDL_Event windowEvent;
        Response resp = Response::OK;

        while (true)
        {
            if (SDL_PollEvent(&windowEvent))
            {
                switch (windowEvent.type)
                {
                case SDL_QUIT:
                    // Window closed by user
                    resp = Response::QUIT;
                    break;

                case SDL_WINDOWEVENT_SIZE_CHANGED:
                    // Window resized - update board size
                    board->reset_window_size();
                    break;

                case SDL_MOUSEBUTTONDOWN:
                {
                    int x = windowEvent.motion.x;
                    int y = windowEvent.motion.y;
                    int xc = int(y / (board->H / 10) - 1);
                    int yc = int(x / (board->W / 10) - 1);

                    // If clicked on the replay area
                    if (xc == -1 && yc == 8)
                        resp = Response::REPLAY;
                }
                break;
                }

                if (resp != Response::OK)
                    break;  // Break loop if any event other than OK detected
            }
        }
        return resp;  // Return the detected event type
    }

private:
    Board* board;  // Pointer to the Board object, used for size and history access, and board management
};

