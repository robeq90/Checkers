#include <SDL.h>
#include <SDL_image.h>
#include <nlohmann/json.hpp>
#include "Game/Game.h"

int main(int argc, char* argv[])
{
    Game g;
    g.play();

    return 0;
}
