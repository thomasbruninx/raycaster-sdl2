#pragma once

#include "game_types.h"
#include <utility>

Map createRandomMap();
std::pair<double, double> pickSpawnPoint(const Map& map);
