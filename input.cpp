#include "input.h"

#include <cmath>

#include "doors.h"

namespace {
bool isWalkable(double x, double y, const Map& map, const std::vector<Door>& doors) {
    int cellX = static_cast<int>(x);
    int cellY = static_cast<int>(y);
    int tile = map.at(cellX, cellY);
    if (tile == 0) {
        return true;
    }
    if (tile == DOOR_TILE) {
        const Door* door = findDoor(doors, cellX, cellY);
        return door && door->openAmount > 0.8;
    }
    return false;
}
} // namespace

void handleInput(const Uint8* keystate, const Map& map, const std::vector<Door>& doors, Player& player, const Config& cfg, double dt) {
    double moveStep = cfg.moveSpeed * dt;
    double rotStep = cfg.rotSpeed * dt;

    if (keystate[SDL_SCANCODE_W] || keystate[SDL_SCANCODE_UP]) {
        double nextX = player.x + player.dirX * moveStep;
        double nextY = player.y + player.dirY * moveStep;
        if (isWalkable(nextX, player.y, map, doors)) {
            player.x = nextX;
        }
        if (isWalkable(player.x, nextY, map, doors)) {
            player.y = nextY;
        }
    }
    if (keystate[SDL_SCANCODE_S] || keystate[SDL_SCANCODE_DOWN]) {
        double nextX = player.x - player.dirX * moveStep;
        double nextY = player.y - player.dirY * moveStep;
        if (isWalkable(nextX, player.y, map, doors)) {
            player.x = nextX;
        }
        if (isWalkable(player.x, nextY, map, doors)) {
            player.y = nextY;
        }
    }
    if (keystate[SDL_SCANCODE_A] || keystate[SDL_SCANCODE_LEFT]) {
        double oldDirX = player.dirX;
        player.dirX = player.dirX * std::cos(rotStep) - player.dirY * std::sin(rotStep);
        player.dirY = oldDirX * std::sin(rotStep) + player.dirY * std::cos(rotStep);
        double oldPlaneX = player.planeX;
        player.planeX = player.planeX * std::cos(rotStep) - player.planeY * std::sin(rotStep);
        player.planeY = oldPlaneX * std::sin(rotStep) + player.planeY * std::cos(rotStep);
    }
    if (keystate[SDL_SCANCODE_D] || keystate[SDL_SCANCODE_RIGHT]) {
        double oldDirX = player.dirX;
        player.dirX = player.dirX * std::cos(-rotStep) - player.dirY * std::sin(-rotStep);
        player.dirY = oldDirX * std::sin(-rotStep) + player.dirY * std::cos(-rotStep);
        double oldPlaneX = player.planeX;
        player.planeX = player.planeX * std::cos(-rotStep) - player.planeY * std::sin(-rotStep);
        player.planeY = oldPlaneX * std::sin(-rotStep) + player.planeY * std::cos(-rotStep);
    }
}
