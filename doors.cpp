#include "doors.h"

#include <algorithm>
#include <cmath>

Door makeDoor(int x, int y, const Map& map) {
    bool vertical = false;
    bool wallsLeftRight = map.at(x - 1, y) > 0 && map.at(x + 1, y) > 0;
    bool wallsUpDown = map.at(x, y - 1) > 0 && map.at(x, y + 1) > 0;
    // Swap orientation to rotate rendering 90 degrees but keep existing hit logic.
    if (wallsUpDown && !wallsLeftRight) {
        vertical = true;
    } else if (!wallsUpDown && wallsLeftRight) {
        vertical = false;
    } else {
        vertical = wallsUpDown;
    }
    return Door{x, y, vertical};
}

std::vector<Door> extractDoors(const Map& map) {
    std::vector<Door> doors;
    for (int y = 0; y < map.height; ++y) {
        for (int x = 0; x < map.width; ++x) {
            if (map.tiles[y * map.width + x] == DOOR_TILE) {
                doors.push_back(makeDoor(x, y, map));
            }
        }
    }
    return doors;
}

Door* findDoor(std::vector<Door>& doors, int x, int y) {
    for (auto& d : doors) {
        if (d.x == x && d.y == y) {
            return &d;
        }
    }
    return nullptr;
}

const Door* findDoor(const std::vector<Door>& doors, int x, int y) {
    for (const auto& d : doors) {
        if (d.x == x && d.y == y) {
            return &d;
        }
    }
    return nullptr;
}

bool computeDoorHit(const Door& door, const Player& player, double rayDirX, double rayDirY, double& dist, bool& side) {
    const double minDist = 0.0001;
    if (door.vertical) {
        // Corridor runs left/right (walls above/below). Door plane stays at x=const and slides into a wall along Y.
        if (std::abs(rayDirX) < 1e-6) {
            return false;
        }
        double planeX = door.x + 0.5;
        double t = (planeX - player.x) / rayDirX;
        if (t <= minDist) {
            return false;
        }
        double yHit = player.y + t * rayDirY;
        double minY = door.y + door.openAmount; // slides down as it opens (into bottom wall)
        double maxY = door.y + 1.0;
        if (yHit >= minY && yHit <= maxY) {
            dist = t;
            side = false; // east/west face
            return true;
        }
    } else {
        // Corridor runs up/down (walls left/right). Door plane stays at y=const and slides into a wall along X.
        if (std::abs(rayDirY) < 1e-6) {
            return false;
        }
        double planeY = door.y + 0.5;
        double t = (planeY - player.y) / rayDirY;
        if (t <= minDist) {
            return false;
        }
        double xHit = player.x + t * rayDirX;
        double minX = door.x + door.openAmount; // slides right as it opens (into right wall)
        double maxX = door.x + 1.0;
        if (xHit >= minX && xHit <= maxX) {
            dist = t;
            side = true; // north/south face
            return true;
        }
    }
    return false;
}

bool playerInDoorway(const Door& door, const Player& player) {
    return player.x >= door.x && player.x <= door.x + 1.0 &&
           player.y >= door.y && player.y <= door.y + 1.0;
}

Door* doorInFront(Player& player, const Map& map, std::vector<Door>& doors) {
    double probeDist = 1.2;
    double targetX = player.x + player.dirX * probeDist;
    double targetY = player.y + player.dirY * probeDist;
    int cellX = static_cast<int>(targetX);
    int cellY = static_cast<int>(targetY);
    if (map.at(cellX, cellY) == DOOR_TILE) {
        return findDoor(doors, cellX, cellY);
    }
    return nullptr;
}

void updateDoors(std::vector<Door>& doors, const Player& player, double dt) {
    const double openSpeed = 1.2; // fraction per second
    const double autoCloseDelay = 5.0;

    for (auto& door : doors) {
        bool playerBlocking = playerInDoorway(door, player);
        if (playerBlocking) {
            door.targetOpen = true;
            door.timeFullyOpen = 0.0;
        }

        if (door.targetOpen) {
            door.openAmount = std::min(1.0, door.openAmount + openSpeed * dt);
            if (door.openAmount >= 1.0) {
                door.timeFullyOpen += dt;
                if (!playerBlocking && door.timeFullyOpen >= autoCloseDelay) {
                    door.targetOpen = false;
                }
            }
        } else {
            door.timeFullyOpen = 0.0;
            door.openAmount = std::max(0.0, door.openAmount - openSpeed * dt);
        }
    }
}
