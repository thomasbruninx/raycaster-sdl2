#pragma once

#include <vector>

#include "game_types.h"
#include "console.h"

void renderFrame(const Map& map, const std::vector<Door>& doors, const std::vector<Sprite>& sprites, const Player& player, const Config& cfg, SDL_Renderer* renderer, const TextureManager& tm, const ConsoleState& console, bool showMinimap, double fps);
