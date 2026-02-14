#include "map.h"

#include <algorithm>
#include <random>
#include <utility>
#include <vector>

namespace {
struct Room {
    int x;
    int y;
    int w;
    int h;

    int centerX() const { return x + w / 2; }
    int centerY() const { return y + h / 2; }
};

void fillRect(Map& m, int x, int y, int w, int h, int value) {
    for (int yy = y; yy < y + h; ++yy) {
        for (int xx = x; xx < x + w; ++xx) {
            m.tiles[yy * m.width + xx] = value;
        }
    }
}

bool intersects(const Room& a, const Room& b) {
    return a.x <= b.x + b.w && a.x + a.w >= b.x && a.y <= b.y + b.h && a.y + a.h >= b.y;
}

void carveHorizontalTunnel(Map& m, int x1, int x2, int y) {
    if (x1 > x2) std::swap(x1, x2);
    for (int x = x1; x <= x2; ++x) {
        m.tiles[y * m.width + x] = 0;
    }
}

void carveVerticalTunnel(Map& m, int y1, int y2, int x) {
    if (y1 > y2) std::swap(y1, y2);
    for (int y = y1; y <= y2; ++y) {
        m.tiles[y * m.width + x] = 0;
    }
}

bool validDoorSpot(const Map& m, int x, int y) {
    if (x <= 0 || y <= 0 || x >= m.width - 1 || y >= m.height - 1) {
        return false;
    }
    bool wallsLR = m.at(x - 1, y) > 0 && m.at(x + 1, y) > 0 && m.at(x, y - 1) == 0 && m.at(x, y + 1) == 0;
    bool wallsUD = m.at(x, y - 1) > 0 && m.at(x, y + 1) > 0 && m.at(x - 1, y) == 0 && m.at(x + 1, y) == 0;
    return wallsLR || wallsUD;
}

void addDoors(Map& m, std::mt19937& rng) {
    std::uniform_real_distribution<double> prob(0.0, 1.0);
    for (int y = 1; y < m.height - 1; ++y) {
        for (int x = 1; x < m.width - 1; ++x) {
            if (m.tiles[y * m.width + x] != 0) {
                continue;
            }
            if (validDoorSpot(m, x, y) && prob(rng) < 0.1) {
                m.tiles[y * m.width + x] = DOOR_TILE;
            }
        }
    }
}
} // namespace

Map createRandomMap() {
    std::random_device rd;
    std::mt19937 rng(rd());
    std::uniform_int_distribution<int> sizeDist(32, 256);
    int w = sizeDist(rng);
    int h = sizeDist(rng);

    Map m{};
    m.width = w;
    m.height = h;
    m.tiles.assign(w * h, 1);

    std::uniform_int_distribution<int> roomWDist(6, 14);
    std::uniform_int_distribution<int> roomHDist(6, 12);
    std::uniform_int_distribution<int> wallColorDist(1, 4);
    std::vector<Room> rooms;
    int attempts = 200;
    while (attempts-- > 0) {
        int rw = roomWDist(rng);
        int rh = roomHDist(rng);
        if (rw >= w - 2 || rh >= h - 2) {
            continue;
        }
        std::uniform_int_distribution<int> posXDist(1, w - rw - 2);
        std::uniform_int_distribution<int> posYDist(1, h - rh - 2);
        Room candidate{posXDist(rng), posYDist(rng), rw, rh};
        bool overlaps = false;
        for (const auto& r : rooms) {
            Room expanded{candidate.x - 1, candidate.y - 1, candidate.w + 2, candidate.h + 2};
            if (intersects(expanded, r)) {
                overlaps = true;
                break;
            }
        }
        if (overlaps) {
            continue;
        }
        rooms.push_back(candidate);
        fillRect(m, candidate.x, candidate.y, candidate.w, candidate.h, 0);
        // Give each room a random wall color on its perimeter.
        int color = wallColorDist(rng);
        for (int yy = candidate.y - 1; yy <= candidate.y + candidate.h; ++yy) {
            for (int xx = candidate.x - 1; xx <= candidate.x + candidate.w; ++xx) {
                if (xx < 0 || yy < 0 || xx >= w || yy >= h) continue;
                if (m.tiles[yy * w + xx] == 1) {
                    m.tiles[yy * w + xx] = color;
                }
            }
        }
    }

    if (rooms.empty()) {
        // Fallback to a small open box if random placement failed.
        int margin = 2;
        fillRect(m, margin, margin, w - margin * 2, h - margin * 2, 0);
    }

    // Connect rooms with corridors (simple chaining + some random links).
    std::uniform_int_distribution<int> coin(0, 1);
    for (size_t i = 1; i < rooms.size(); ++i) {
        int x1 = rooms[i - 1].centerX();
        int y1 = rooms[i - 1].centerY();
        int x2 = rooms[i].centerX();
        int y2 = rooms[i].centerY();
        if (coin(rng)) {
            carveHorizontalTunnel(m, x1, x2, y1);
            carveVerticalTunnel(m, y1, y2, x2);
        } else {
            carveVerticalTunnel(m, y1, y2, x1);
            carveHorizontalTunnel(m, x1, x2, y2);
        }
    }
    if (rooms.size() >= 3) {
        std::uniform_int_distribution<size_t> roomIdx(0, rooms.size() - 1);
        int extraLinks = static_cast<int>(rooms.size() / 3);
        while (extraLinks-- > 0) {
            const Room& a = rooms[roomIdx(rng)];
            const Room& b = rooms[roomIdx(rng)];
            if (&a == &b) continue;
            int x1 = a.centerX();
            int y1 = a.centerY();
            int x2 = b.centerX();
            int y2 = b.centerY();
            carveHorizontalTunnel(m, x1, x2, y1);
            carveVerticalTunnel(m, y1, y2, x2);
        }
    }

    // Ensure outer walls remain solid.
    for (int x = 0; x < w; ++x) {
        m.tiles[x] = 1;
        m.tiles[(h - 1) * w + x] = 1;
    }
    for (int y = 0; y < h; ++y) {
        m.tiles[y * w] = 1;
        m.tiles[y * w + (w - 1)] = 1;
    }

    addDoors(m, rng);
    return m;
}

std::pair<double, double> pickSpawnPoint(const Map& map) {
    for (int y = 1; y < map.height - 1; ++y) {
        for (int x = 1; x < map.width - 1; ++x) {
            if (map.at(x, y) == 0) {
                return {x + 0.5, y + 0.5};
            }
        }
    }
    return {1.5, 1.5};
}

std::vector<Sprite> createSprites(const Map& map) {
    std::vector<std::pair<int, int>> candidates;
    candidates.reserve((map.width - 2) * (map.height - 2));
    for (int y = 1; y < map.height - 1; ++y) {
        for (int x = 1; x < map.width - 1; ++x) {
            if (map.at(x, y) == 0) {
                candidates.push_back({x, y});
            }
        }
    }
    if (candidates.empty()) {
        return {};
    }

    std::random_device rd;
    std::mt19937 rng(rd());
    std::shuffle(candidates.begin(), candidates.end(), rng);

    int targetCount = std::clamp((map.width * map.height) / 180, 12, 96);
    int spriteCount = std::min(targetCount, static_cast<int>(candidates.size()));
    std::uniform_int_distribution<int> spriteTexDist(0, 2);

    std::vector<Sprite> sprites;
    sprites.reserve(spriteCount);
    for (int i = 0; i < spriteCount; ++i) {
        int x = candidates[i].first;
        int y = candidates[i].second;
        sprites.push_back({x + 0.5, y + 0.5, spriteTexDist(rng)});
    }
    return sprites;
}
