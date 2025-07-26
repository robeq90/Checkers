#pragma once
#include <stdlib.h>

// POS_T type is used to store board cell coordinates
// It is an 8-bit signed integer (int8_t)
typedef int8_t POS_T;

// The struct move_pos describes a move of a checker on the board
struct move_pos
{
    POS_T x, y;             // Starting position (from) - cell coordinates
    POS_T x2, y2;           // Ending position (to) - cell coordinates
    POS_T xb = -1, yb = -1; // Coordinates of the beaten checker (captured piece), default -1 means no piece beaten

    // Constructor for a move without capturing a piece
    // Takes starting and ending coordinates
    move_pos(const POS_T x, const POS_T y, const POS_T x2, const POS_T y2)
        : x(x), y(y), x2(x2), y2(y2)
    {
    }

    // Constructor for a move with capturing a piece
    // Takes starting and ending coordinates and the coordinates of the beaten checker
    move_pos(const POS_T x, const POS_T y, const POS_T x2, const POS_T y2, const POS_T xb, const POS_T yb)
        : x(x), y(y), x2(x2), y2(y2), xb(xb), yb(yb)
    {
    }

};