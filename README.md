# ZS1999

## C++ WASD Movement Script

This repository includes `player_movement.cpp`, a simple player movement script that supports:

- `W` / `A` / `S` / `D` directional movement
- **Expansion:** sprint (`Shift` state)
- **Expansion:** dash (`Space` press)
- **Expansion:** acceleration/deceleration smoothing

## Do I need to install expansions like CMake?

**No.** You only need a C++ compiler with C++17 support (for example, `g++` or `clang++`).

- The movement “expansions” (sprint/dash/smoothing) are already implemented in the `.cpp` file.
- You do **not** need CMake to build this example.
- CMake is optional and only useful if you want a larger multi-file project setup.

### Build (no CMake)

```bash
g++ -std=c++17 -Wall -Wextra -pedantic player_movement.cpp -o player_movement
```

### Run

```bash
./player_movement
```

You can replace the demo loop with your own engine's input system by filling `InputState` each frame and calling:

```cpp
player.Update(deltaTime, input);
```
