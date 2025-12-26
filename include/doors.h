#pragma once

#include <vector>

#include "game_types.h"

Door makeDoor(int x, int y, const Map& map);
std::vector<Door> extractDoors(const Map& map);
Door* findDoor(std::vector<Door>& doors, int x, int y);
const Door* findDoor(const std::vector<Door>& doors, int x, int y);
bool computeDoorHit(const Door& door, const Player& player, double rayDirX, double rayDirY, double& dist, bool& side);
bool playerInDoorway(const Door& door, const Player& player);
Door* doorInFront(Player& player, const Map& map, std::vector<Door>& doors);
void updateDoors(std::vector<Door>& doors, const Player& player, double dt);
