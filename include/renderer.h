#pragma once

#include <vector>

#include "game_types.h"

void renderFrame(const Map& map, const std::vector<Door>& doors, const Player& player, const Config& cfg, SDL_Renderer* renderer, const TextureManager& tm);
