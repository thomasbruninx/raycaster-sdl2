#include "textures.h"

#include <SDL2/SDL_image.h>
#include <algorithm>
#include <iostream>
#include <string>

namespace {
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
} // namespace

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
