#pragma once

#include <vector>

#include "game_types.h"

void handleInput(const Uint8* keystate, const Map& map, std::vector<Door>& doors, Player& player, const Config& cfg, double dt);
