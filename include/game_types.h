#pragma once

#include <SDL2/SDL.h>
#include <vector>

constexpr int DOOR_TILE = 5;

struct Color {
    Uint8 r;
    Uint8 g;
    Uint8 b;
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

    Door(int x_, int y_, bool vertical_)
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
