# RetroNode Engine

**RetroNode** is a custom 2D retro game engine engineered for era-accurate pixel art games. It prioritizes deterministic physics, zero-overhead memory management, and a Server-Node architectural split inspired by Godot, implemented purely in modern C++ (C++20).

---

## 🌟 Key Features

* **Server-Node Split Architecture:** Decouples user-facing scene graph hierarchy (`Node2D`, `Sprite2D`, `CharacterBody2D`) from hardware execution singletons (`VisualServer`, `PhysicsServer2D`, `AudioServer`).
* **Deterministic Q16.16 Fixed-Point Math:** Custom `Fixed16` fixed-point math engine for cross-platform physics determinism.
* **Fixed 60Hz Timestep Accumulator Loop:** Decouples rendering framerate from physics tick rates with frame interpolation.
* **Hot-Reloadable Game Logic:** Game projects (`MyRPG/`) compile into shared dynamic libraries (`game.dll` / `libgame.so`) swapped at runtime without restarting the engine executable (`retronode.exe`).
* **SDL3 Platform Backend:** Low-level windowing, input, and hardware rendering using SDL3.

---

## 📁 Repository Structure

```text
RetroNode/
├── CMakeLists.txt            # Root engine CMake configuration
├── README.md                 # Project documentation
├── .gitignore                # Git ignore rules
├── LICENSE                   # MIT License
│
├── src/                      # Engine core source code
│   ├── core/                 # Fixed math, memory allocators, reflection registry
│   ├── scene/                # User-facing Node hierarchy (Node2D, Sprite2D, etc.)
│   ├── servers/              # Backend singletons (VisualServer, PhysicsServer2D)
│   ├── platform/             # SDL3 platform integration & main loop entry point
│   └── tools/                # Texture packer and map compiler pipelines
│
├── thirdparty/               # Minimal external dependencies
│
└── MyRPG/                    # Example test game project
    ├── project.rnode         # Game configuration file
    ├── CMakeLists.txt        # Shared library (game.dll) build configuration
    ├── assets/               # Sprites, tilesets, and audio
    ├── maps/                 # Level maps
    ├── scenes/               # JSON scene definitions
    └── src/                  # Game C++ scripts and registration module
```

---

## 🛠️ Building RetroNode

### Prerequisites

* **CMake** (v3.20 or higher)
* **C++20 Compiler** (Visual Studio 2022 MSVC, GCC 11+, or Clang 13+)
* **Git** (for FetchContent dependencies)

### Build Instructions (Windows / MSVC)

```powershell
# 1. Configure CMake (fetches SDL3 automatically)
cmake -B build -G "Visual Studio 17 2022" -A x64

# 2. Build Engine & Game Shared Library
cmake --build build --config Debug

# 3. Copy SDL3 runtime binary to output directory
Copy-Item -Path "build\_deps\sdl3-build\Debug\SDL3.dll" -Destination "build\Debug\" -Force

# 4. Run RetroNode Engine
.\build\Debug\retronode.exe
```

---

## 📄 License

This project is licensed under the [MIT License](LICENSE).
