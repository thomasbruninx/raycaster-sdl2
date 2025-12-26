#pragma once

#include "game_types.h"

TextureManager loadTextures();
void freeTextures(TextureManager& tm);
Color sampleTexture(SDL_Surface* surf, int x, int y);
