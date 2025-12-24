#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <algorithm>
#include <array>
#include <cmath>
#include <iostream>
#include <vector>

constexpr int DOOR_TILE = 5;

struct Color {
    Uint8 r, g, b;
};

struct Map {
    int width;
    int height;
    std::vector<int> tiles; // 0 = empty, >0 = wall id

    int at(int x, int y) const {
        if (x < 0 || x >= width || y < 0 || y >= height) {
            return 1;
        }
        return tiles[y * width + x];
    }
};

struct Door {
    int x;
    int y;
    bool vertical;          // true when corridor runs left/right
    double openAmount = 0;  // 0 closed, 1 fully open
    bool targetOpen = false;
    double timeFullyOpen = 0.0;

    Door (int x_, int y_,  bool vertical_)
        : x(x_), y(y_), vertical(vertical_) {}
};

struct Player {
    double x;
    double y;
    double dirX;
    double dirY;
    double planeX;
    double planeY;
};

struct Config {
    int screenWidth = 960;
    int screenHeight = 640;
    double moveSpeed = 3.0;      // units per second
    double rotSpeed = 1.8;       // radians per second
    double wallHeight = 1.0;
};

struct SDLContext {
    SDL_Window* window = nullptr;
    SDL_Renderer* renderer = nullptr;
};

struct TextureManager {
    std::vector<SDL_Surface*> textures; // index by tile id
};

bool initSDL(SDLContext& ctx, const Config& cfg) {
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        std::cerr << "SDL_Init Error: " << SDL_GetError() << "\n";
        return false;
    }

    int imgFlags = IMG_INIT_PNG;
    if ((IMG_Init(imgFlags) & imgFlags) != imgFlags) {
        std::cerr << "IMG_Init Error: " << IMG_GetError() << "\n";
        return false;
    }

    ctx.window = SDL_CreateWindow(
        "Raycaster", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        cfg.screenWidth, cfg.screenHeight, SDL_WINDOW_SHOWN);
    if (!ctx.window) {
        std::cerr << "SDL_CreateWindow Error: " << SDL_GetError() << "\n";
        return false;
    }

    ctx.renderer = SDL_CreateRenderer(
        ctx.window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!ctx.renderer) {
        std::cerr << "SDL_CreateRenderer Error: " << SDL_GetError() << "\n";
        return false;
    }

    SDL_SetRenderDrawBlendMode(ctx.renderer, SDL_BLENDMODE_BLEND);
    return true;
}

void shutdownSDL(SDLContext& ctx) {
    if (ctx.renderer) {
        SDL_DestroyRenderer(ctx.renderer);
    }
    if (ctx.window) {
        SDL_DestroyWindow(ctx.window);
    }
    IMG_Quit();
    SDL_Quit();
}

Map createDemoMap() {
    // Simple level with rooms and corridors; 1..4 used to color walls, 5 = door
    const int w = 16;
    const int h = 16;
    const std::array<int, w * h> data = {{
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
        1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 5, 0, 1,
        1, 0, 2, 2, 0, 0, 0, 0, 0, 3, 3, 0, 0, 4, 0, 1,
        1, 0, 2, 0, 0, 1, 0, 1, 0, 3, 0, 0, 4, 0, 0, 1,
        1, 0, 2, 0, 0, 1, 0, 1, 0, 3, 0, 0, 4, 0, 0, 1,
        1, 0, 2, 0, 0, 1, 0, 1, 0, 3, 0, 0, 4, 4, 0, 1,
        1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 4, 0, 1,
        1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 4, 0, 1,
        1, 0, 2, 2, 0, 0, 0, 0, 0, 3, 3, 0, 0, 4, 0, 1,
        1, 0, 2, 0, 0, 1, 0, 1, 0, 3, 0, 0, 4, 0, 0, 1,
        1, 5, 2, 0, 0, 1, 0, 1, 0, 3, 0, 0, 4, 0, 0, 1,
        1, 0, 2, 0, 0, 1, 0, 1, 0, 3, 0, 0, 4, 4, 0, 1,
        1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 4, 0, 1,
        1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 4, 0, 1,
        1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    }};

    Map m{};
    m.width = w;
    m.height = h;
    m.tiles.assign(data.begin(), data.end());
    // Inject a door between two walls (above/below) near the center.
    int doorX = 8;
    int doorY = 8;
    m.tiles[doorY * w + doorX] = DOOR_TILE;
    m.tiles[(doorY - 1) * w + doorX] = 1;
    m.tiles[(doorY + 1) * w + doorX] = 1;
    m.tiles[doorY * w + (doorX - 1)] = 0;
    m.tiles[doorY * w + (doorX + 1)] = 0;
    return m;
}

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

SDL_Surface* loadSurface(const std::string& path) {
    SDL_Surface* loaded = IMG_Load(path.c_str());
    if (!loaded) {
        std::cerr << "Failed to load texture " << path << ": " << IMG_GetError() << "\n";
        return nullptr;
    }
    SDL_Surface* converted = SDL_ConvertSurfaceFormat(loaded, SDL_PIXELFORMAT_ARGB8888, 0);
    SDL_FreeSurface(loaded);
    if (!converted) {
        std::cerr << "Failed to convert texture " << path << ": " << SDL_GetError() << "\n";
    }
    return converted;
}

TextureManager loadTextures() {
    TextureManager tm{};
    tm.textures.resize(6, nullptr);
    tm.textures[1] = loadSurface("resources/textures/redbrick.png");
    tm.textures[2] = loadSurface("resources/textures/greystone.png");
    tm.textures[3] = loadSurface("resources/textures/wood.png");
    tm.textures[4] = loadSurface("resources/textures/bluestone.png");
    tm.textures[DOOR_TILE] = loadSurface("resources/textures/eagle.png");
    return tm;
}

void freeTextures(TextureManager& tm) {
    for (auto* surf : tm.textures) {
        if (surf) {
            SDL_FreeSurface(surf);
        }
    }
    tm.textures.clear();
}

Color sampleTexture(SDL_Surface* surf, int x, int y) {
    if (!surf) {
        return {255, 0, 255}; // magenta fallback
    }
    x = std::max(0, std::min(x, surf->w - 1));
    y = std::max(0, std::min(y, surf->h - 1));
    Uint8* pixelBase = static_cast<Uint8*>(surf->pixels);
    Uint32* pixels = reinterpret_cast<Uint32*>(pixelBase);
    Uint32 pixel = pixels[y * surf->w + x];
    Uint8 r, g, b, a;
    SDL_GetRGBA(pixel, surf->format, &r, &g, &b, &a);
    return {r, g, b};
}

void renderFrame(const Map& map, const std::vector<Door>& doors, const Player& player, const Config& cfg, SDL_Renderer* renderer, const TextureManager& tm) {
    SDL_SetRenderDrawColor(renderer, 30, 30, 30, 255);
    SDL_RenderClear(renderer);

    // Sky
    SDL_Rect skyRect{0, 0, cfg.screenWidth, cfg.screenHeight / 2};
    SDL_SetRenderDrawColor(renderer, 60, 60, 90, 255);
    SDL_RenderFillRect(renderer, &skyRect);

    // Floor gradient
    for (int y = cfg.screenHeight / 2; y < cfg.screenHeight; ++y) {
        Uint8 shade = static_cast<Uint8>(40 + 80.0 * (y - cfg.screenHeight / 2) / (cfg.screenHeight / 2));
        SDL_SetRenderDrawColor(renderer, shade, shade, shade, 255);
        SDL_RenderDrawLine(renderer, 0, y, cfg.screenWidth, y);
    }

    // Raycasting per column
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
        double wallX = side ? hitX : hitY;
        wallX -= std::floor(wallX);

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
            SDL_SetRenderDrawColor(renderer, c.r, c.g, c.b, 255);
            SDL_RenderDrawPoint(renderer, x, y);
        }
    }

    SDL_RenderPresent(renderer);
}

bool playerInDoorway(const Door& door, const Player& player) {
    return player.x >= door.x && player.x <= door.x + 1.0 &&
           player.y >= door.y && player.y <= door.y + 1.0;
}

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

int main(int argc, char* argv[]) {
    (void)argc;
    (void)argv;

    Config cfg{};
    SDLContext ctx{};
    if (!initSDL(ctx, cfg)) {
        shutdownSDL(ctx);
        return 1;
    }

    Map map = createDemoMap();
    std::vector<Door> doors = extractDoors(map);
    TextureManager textures = loadTextures();
    Player player{4.5, 4.5, -1.0, 0.0, 0.0, 0.66};

    bool running = true;
    Uint32 lastTicks = SDL_GetTicks();
    while (running) {
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) {
                running = false;
            } else if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE) {
                running = false;
            } else if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_SPACE) {
                Door* target = doorInFront(player, map, doors);
                if (target) {
                    target->targetOpen = !target->targetOpen;
                }
            }
        }

        Uint32 currentTicks = SDL_GetTicks();
        double dt = (currentTicks - lastTicks) / 1000.0;
        lastTicks = currentTicks;

        const Uint8* keystate = SDL_GetKeyboardState(nullptr);
        handleInput(keystate, map, doors, player, cfg, dt);
        updateDoors(doors, player, dt);

        renderFrame(map, doors, player, cfg, ctx.renderer, textures);
    }

    freeTextures(textures);
    shutdownSDL(ctx);
    return 0;
}
