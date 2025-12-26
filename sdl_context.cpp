#include "sdl_context.h"

#include <SDL2/SDL_image.h>
#include <iostream>

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
