#include "renderer.h"

#include <algorithm>
#include <array>
#include <cmath>

#include "doors.h"
#include "textures.h"

namespace {
Color wallColor(int id, bool isSideHit) {
    static const std::array<Color, 6> palette = {{
        {0, 0, 0},       // unused
        {200, 60, 60},   // red
        {60, 160, 200},  // blue
        {60, 200, 120},  // green
        {220, 200, 80},  // yellow
        {160, 160, 180}, // door color fallback
    }};
    Color c = palette[std::min(id, static_cast<int>(palette.size() - 1))];
    if (isSideHit) {
        c.r = static_cast<Uint8>(c.r * 0.7);
        c.g = static_cast<Uint8>(c.g * 0.7);
        c.b = static_cast<Uint8>(c.b * 0.7);
    }
    return c;
}

Color doorRenderColor(const Door& door, bool isSideHit) {
    Color base{150, 170, 190};
    double visibility = 1.0 - door.openAmount * 0.7; // fade as it opens
    base.r = static_cast<Uint8>(base.r * visibility);
    base.g = static_cast<Uint8>(base.g * visibility);
    base.b = static_cast<Uint8>(base.b * visibility);
    if (isSideHit) {
        base.r = static_cast<Uint8>(base.r * 0.8);
        base.g = static_cast<Uint8>(base.g * 0.8);
        base.b = static_cast<Uint8>(base.b * 0.8);
    }
    return base;
}
} // namespace

void renderFrame(const Map& map, const std::vector<Door>& doors, const Player& player, const Config& cfg, SDL_Renderer* renderer, const TextureManager& tm) {
    SDL_SetRenderDrawColor(renderer, 30, 30, 30, 255);
    SDL_RenderClear(renderer);

    SDL_Rect skyRect{0, 0, cfg.screenWidth, cfg.screenHeight / 2};
    SDL_SetRenderDrawColor(renderer, 60, 60, 90, 255);
    SDL_RenderFillRect(renderer, &skyRect);

    // Floor gradient
    for (int y = cfg.screenHeight / 2; y < cfg.screenHeight; ++y) {
        Uint8 shade = static_cast<Uint8>(40 + 80.0 * (y - cfg.screenHeight / 2) / (cfg.screenHeight / 2));
        SDL_SetRenderDrawColor(renderer, shade, shade, shade, 255);
        SDL_RenderDrawLine(renderer, 0, y, cfg.screenWidth, y);
    }

    for (int x = 0; x < cfg.screenWidth; ++x) {
        double cameraX = 2.0 * x / cfg.screenWidth - 1.0;
        double rayDirX = player.dirX + player.planeX * cameraX;
        double rayDirY = player.dirY + player.planeY * cameraX;

        int mapX = static_cast<int>(player.x);
        int mapY = static_cast<int>(player.y);

        double deltaDistX = (rayDirX == 0) ? 1e30 : std::abs(1.0 / rayDirX);
        double deltaDistY = (rayDirY == 0) ? 1e30 : std::abs(1.0 / rayDirY);

        double sideDistX;
        double sideDistY;
        int stepX;
        int stepY;

        if (rayDirX < 0) {
            stepX = -1;
            sideDistX = (player.x - mapX) * deltaDistX;
        } else {
            stepX = 1;
            sideDistX = (mapX + 1.0 - player.x) * deltaDistX;
        }

        if (rayDirY < 0) {
            stepY = -1;
            sideDistY = (player.y - mapY) * deltaDistY;
        } else {
            stepY = 1;
            sideDistY = (mapY + 1.0 - player.y) * deltaDistY;
        }

        bool hit = false;
        bool side = false;
        int wallId = 0;
        double doorHitDist = -1.0;
        const Door* hitDoor = nullptr;
        while (!hit) {
            if (sideDistX < sideDistY) {
                sideDistX += deltaDistX;
                mapX += stepX;
                side = false;
            } else {
                sideDistY += deltaDistY;
                mapY += stepY;
                side = true;
            }

            int tile = map.at(mapX, mapY);
            if (tile == DOOR_TILE) {
                const Door* door = findDoor(doors, mapX, mapY);
                if (door && door->openAmount < 0.99) {
                    double dist;
                    bool doorSide;
                    if (computeDoorHit(*door, player, rayDirX, rayDirY, dist, doorSide)) {
                        hit = true;
                        wallId = DOOR_TILE;
                        doorHitDist = dist;
                        side = doorSide;
                        hitDoor = door;
                    }
                }
                if (!hit) {
                    continue; // fully open or no intersection; keep marching
                }
            }

            if (tile > 0 && tile != DOOR_TILE) {
                hit = true;
                wallId = tile;
            }
        }

        double perpWallDist = (doorHitDist > 0.0) ? doorHitDist
                                                 : (side ? (sideDistY - deltaDistY) : (sideDistX - deltaDistX));
        if (perpWallDist <= 0.0001) {
            perpWallDist = 0.0001;
        }
        int lineHeight = static_cast<int>(cfg.wallHeight * cfg.screenHeight / perpWallDist);
        int drawStart = -lineHeight / 2 + cfg.screenHeight / 2;
        int drawEnd = lineHeight / 2 + cfg.screenHeight / 2;

        double hitX = player.x + perpWallDist * rayDirX;
        double hitY = player.y + perpWallDist * rayDirY;
        double wallX;
        if (hitDoor) {
            if (hitDoor->vertical) {
                double offsetY = hitDoor->y + hitDoor->openAmount;
                wallX = hitY - offsetY;
            } else {
                double offsetX = hitDoor->x + hitDoor->openAmount;
                wallX = hitX - offsetX;
            }
            wallX -= std::floor(wallX);
        } else {
            wallX = side ? hitX : hitY;
            wallX -= std::floor(wallX);
        }

        SDL_Surface* surf = nullptr;
        if (wallId >= 0 && wallId < static_cast<int>(tm.textures.size())) {
            surf = tm.textures[wallId];
        }
        int texW = surf ? surf->w : 1;
        int texH = surf ? surf->h : 1;
        int texX = static_cast<int>(wallX * texW);
        if (!side && rayDirX > 0) {
            texX = texW - texX - 1;
        }
        if (side && rayDirY < 0) {
            texX = texW - texX - 1;
        }

        double texStep = static_cast<double>(texH) / lineHeight;
        double texPos = (drawStart - cfg.screenHeight / 2 + lineHeight / 2) * texStep;

        for (int y = drawStart; y <= drawEnd; ++y) {
            if (y < 0 || y >= cfg.screenHeight) {
                texPos += texStep;
                continue;
            }
            int texY = static_cast<int>(texPos) & (texH - 1);
            texPos += texStep;
            Color c = surf ? sampleTexture(surf, texX, texY)
                           : (hitDoor ? doorRenderColor(*hitDoor, side) : wallColor(wallId, side));
            if (side) {
                c.r = static_cast<Uint8>(c.r * 0.7);
                c.g = static_cast<Uint8>(c.g * 0.7);
                c.b = static_cast<Uint8>(c.b * 0.7);
            }
            SDL_SetRenderDrawColor(renderer, c.r, c.g, c.b, 255);
            SDL_RenderDrawPoint(renderer, x, y);
        }
    }

    SDL_RenderPresent(renderer);
}
