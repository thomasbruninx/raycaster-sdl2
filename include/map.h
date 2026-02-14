#pragma once

#include "game_types.h"
#include <utility>
#include <vector>

Map createRandomMap();
std::pair<double, double> pickSpawnPoint(const Map& map);
std::vector<Sprite> createSprites(const Map& map);
