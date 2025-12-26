#include <SDL2/SDL.h>

#include "doors.h"
#include "game_types.h"
#include "input.h"
#include "map.h"
#include "renderer.h"
#include "sdl_context.h"
#include "textures.h"

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
