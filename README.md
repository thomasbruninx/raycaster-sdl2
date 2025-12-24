# Raycaster-SDL2

Minimal Wolf3D-style raycaster written in C++ using SDL2.

## Build

Requires SDL2, SDL2_image development headers and pkg-config.

```bash
make all
```

## Run

```bash
./raycaster
```

Controls: `W/S` or `Up/Down` to move, `A/D` or arrow keys to turn, `Space` for action, `Esc` to exit.

Textures are loaded from `resources/textures/*.png` (redbrick, greystone, wood, bluestone, eagle).
