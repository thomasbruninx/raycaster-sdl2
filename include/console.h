#pragma once

#include <SDL2/SDL.h>
#include <string>
#include <vector>

#include "game_types.h"

struct ConsoleState {
    bool open = false;
    std::string input;
    std::vector<std::string> history;
    int historyIndex = -1; // -1 means editing current input
    std::vector<std::string> log;
};

void setConsoleOpen(ConsoleState& console, bool open);
void handleConsoleEvent(ConsoleState& console, const SDL_Event& e, Config& cfg, Player& player, bool& running);
