#include <SDL2/SDL.h>
#include <array>
#include <cmath>
#include <iostream>
#include <vector>

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

bool initSDL(SDLContext& ctx, const Config& cfg) {
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        std::cerr << "SDL_Init Error: " << SDL_GetError() << "\n";
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
    SDL_Quit();
}

Map createDemoMap() {
    // Simple level with rooms and corridors; 1..4 used to color walls
    const int w = 16;
    const int h = 16;
    const std::array<int, w * h> data = {{
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
        1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
        1, 0, 2, 2, 0, 0, 0, 0, 0, 3, 3, 0, 0, 4, 0, 1,
        1, 0, 2, 0, 0, 1, 0, 1, 0, 3, 0, 0, 4, 0, 0, 1,
        1, 0, 2, 0, 0, 1, 0, 1, 0, 3, 0, 0, 4, 0, 0, 1,
        1, 0, 2, 0, 0, 1, 0, 1, 0, 3, 0, 0, 4, 4, 0, 1,
        1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 4, 0, 1,
        1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 4, 0, 1,
        1, 0, 2, 2, 0, 0, 0, 0, 0, 3, 3, 0, 0, 4, 0, 1,
        1, 0, 2, 0, 0, 1, 0, 1, 0, 3, 0, 0, 4, 0, 0, 1,
        1, 0, 2, 0, 0, 1, 0, 1, 0, 3, 0, 0, 4, 0, 0, 1,
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
    return m;
}

Color wallColor(int id, bool isSideHit) {
    static const std::array<Color, 5> palette = {{
        {0, 0, 0},       // unused
        {200, 60, 60},   // red
        {60, 160, 200},  // blue
        {60, 200, 120},  // green
        {220, 200, 80},  // yellow
    }};
    Color c = palette[std::min(id, static_cast<int>(palette.size() - 1))];
    if (isSideHit) {
        c.r = static_cast<Uint8>(c.r * 0.7);
        c.g = static_cast<Uint8>(c.g * 0.7);
        c.b = static_cast<Uint8>(c.b * 0.7);
    }
    return c;
}

void renderFrame(const Map& map, const Player& player, const Config& cfg, SDL_Renderer* renderer) {
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
            wallId = map.at(mapX, mapY);
            if (wallId > 0) {
                hit = true;
            }
        }

        double perpWallDist = side ? (sideDistY - deltaDistY) : (sideDistX - deltaDistX);
        if (perpWallDist <= 0.0001) {
            perpWallDist = 0.0001;
        }
        int lineHeight = static_cast<int>(cfg.wallHeight * cfg.screenHeight / perpWallDist);
        int drawStart = -lineHeight / 2 + cfg.screenHeight / 2;
        int drawEnd = lineHeight / 2 + cfg.screenHeight / 2;

        Color c = wallColor(wallId, side);
        SDL_SetRenderDrawColor(renderer, c.r, c.g, c.b, 255);
        SDL_RenderDrawLine(renderer, x, drawStart, x, drawEnd);
    }

    SDL_RenderPresent(renderer);
}

void handleInput(const Uint8* keystate, const Map& map, Player& player, const Config& cfg, double dt) {
    double moveStep = cfg.moveSpeed * dt;
    double rotStep = cfg.rotSpeed * dt;

    if (keystate[SDL_SCANCODE_W] || keystate[SDL_SCANCODE_UP]) {
        double nextX = player.x + player.dirX * moveStep;
        double nextY = player.y + player.dirY * moveStep;
        if (map.at(static_cast<int>(nextX), static_cast<int>(player.y)) == 0) {
            player.x = nextX;
        }
        if (map.at(static_cast<int>(player.x), static_cast<int>(nextY)) == 0) {
            player.y = nextY;
        }
    }
    if (keystate[SDL_SCANCODE_S] || keystate[SDL_SCANCODE_DOWN]) {
        double nextX = player.x - player.dirX * moveStep;
        double nextY = player.y - player.dirY * moveStep;
        if (map.at(static_cast<int>(nextX), static_cast<int>(player.y)) == 0) {
            player.x = nextX;
        }
        if (map.at(static_cast<int>(player.x), static_cast<int>(nextY)) == 0) {
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
    Player player{8.0, 8.0, -1.0, 0.0, 0.0, 0.66};

    bool running = true;
    Uint32 lastTicks = SDL_GetTicks();
    while (running) {
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) {
                running = false;
            } else if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE) {
                running = false;
            }
        }

        Uint32 currentTicks = SDL_GetTicks();
        double dt = (currentTicks - lastTicks) / 1000.0;
        lastTicks = currentTicks;

        const Uint8* keystate = SDL_GetKeyboardState(nullptr);
        handleInput(keystate, map, player, cfg, dt);

        renderFrame(map, player, cfg, ctx.renderer);
    }

    shutdownSDL(ctx);
    return 0;
}
