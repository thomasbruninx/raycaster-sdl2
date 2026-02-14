#include <SDL2/SDL.h>
#include <algorithm>

#include "doors.h"
#include "game_types.h"
#include "input.h"
#include "map.h"
#include "renderer.h"
#include "sdl_context.h"
#include "textures.h"
#include "console.h"

int main(int argc, char* argv[]) {
    (void)argc;
    (void)argv;

    Config cfg{};
    SDLContext ctx{};
    if (!initSDL(ctx, cfg)) {
        shutdownSDL(ctx);
        return 1;
    }

    Map map = createRandomMap();
    std::vector<Door> doors = extractDoors(map);
    std::vector<Sprite> sprites = createSprites(map);
    TextureManager textures = loadTextures();
    auto spawn = pickSpawnPoint(map);
    Player player{spawn.first, spawn.second, -1.0, 0.0, 0.0, 0.66};

    sprites.erase(std::remove_if(sprites.begin(), sprites.end(), [&](const Sprite& s) {
                      double dx = s.x - player.x;
                      double dy = s.y - player.y;
                      return (dx * dx + dy * dy) < 4.0;
                  }),
                  sprites.end());

    ConsoleState console{};
    bool minimapVisible = true;
    double fps = 0.0;

    bool running = true;
    Uint32 lastTicks = SDL_GetTicks();
    while (running) {
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) {
                running = false;
            } else if (e.type == SDL_KEYDOWN) {
                if (e.key.keysym.sym == SDLK_ESCAPE) {
                    running = false;
                } else if (e.key.repeat == 0 && e.key.keysym.sym == SDLK_TAB) {
                    setConsoleOpen(console, !console.open);
                } else if (e.key.repeat == 0 && e.key.keysym.sym == SDLK_m) {
                    if (!console.open) minimapVisible = !minimapVisible;
                }
            }
            handleConsoleEvent(console, e, cfg, player, running);
        }

        Uint32 currentTicks = SDL_GetTicks();
        double dt = (currentTicks - lastTicks) / 1000.0;
        lastTicks = currentTicks;
        double instFps = (dt > 0.0) ? (1.0 / dt) : fps;
        fps = fps * 0.9 + instFps * 0.1;

        const Uint8* keystate = SDL_GetKeyboardState(nullptr);
        if (!console.open) {
            handleInput(keystate, map, doors, player, cfg, dt);
        }
        updateDoors(doors, player, dt);

        renderFrame(map, doors, sprites, player, cfg, ctx.renderer, textures, console, minimapVisible, fps);
    }

    setConsoleOpen(console, false);
    freeTextures(textures);
    shutdownSDL(ctx);
    return 0;
}
