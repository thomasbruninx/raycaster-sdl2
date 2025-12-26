#include "map.h"

#include <array>

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
