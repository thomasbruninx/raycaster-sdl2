# Raycaster-SDL2

Minimal Wolf3D-style raycaster written in C++ using SDL2.

## Build

Requires SDL2, SDL2_image development headers and pkg-config.

```bash
make all
```

Clean up previous builds

```bash
make clean
```

## Run

```bash
./raycaster
```

Controls: `W/S` or `Up/Down` to move, `A/D` or arrow keys to turn, `Space` for action, hold `Shift` while moving to run, `M` to toggle the minimap, `TAB` to open the console, `Esc` to exit.

Textures are loaded from `resources/textures/*.png` (redbrick, greystone, wood, bluestone, door).
