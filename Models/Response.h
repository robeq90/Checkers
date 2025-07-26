#pragma once

// Enum representing possible responses or states in the game
enum class Reponse
{
    OK,     // Successful operation or confirmation of an action
    BACK,   // Undo or return to the previous state/step
    REPLAY, // Repeat a game round or restart the current part
    QUIT,   // Exit the game or end the session
    CELL    // Action related to selecting or processing a cell on the game board
};