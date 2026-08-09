# RetroNode Engine: Technical Design & MVP Implementation Guide

This document outlines the architecture, implementation strategy, and MVP specifications for **RetroNode**, a 2D engine engineered for era-accurate pixel art games. It prioritizes deterministic physics, zero-overhead memory management, and a Server-Node architectural split inspired by Godot, but implemented purely in C++ for maximum performance.

---

## 1. Core Architecture: The Server-Node Split

The fundamental design relies on decoupling the user-facing scene graph (Nodes) from the underlying hardware execution (Servers).

* **Nodes (`Node2D`, `Sprite2D`, `CharacterBody2D`):** These act strictly as data containers and logic controllers. A `Sprite2D` does not make OpenGL calls; it submits a render command (Texture ID, Quad Coordinates, Z-Index) to the `VisualServer`.
* **Servers (`VisualServer`, `PhysicsServer2D`, `AudioServer`):** Singletons that process bulk data. The `PhysicsServer2D` maintains a contiguous array of active AABBs and resolves collisions in a tight, cache-friendly loop, entirely unaware of the Node hierarchy.

---

## 2. Deterministic Core: Fixed-Point Math & The Game Loop

To ensure cross-platform determinism (critical for replays, speedrunning, and rollback netcode), the MVP cannot rely on floating-point arithmetic for gameplay physics.

### Q16.16 Fixed-Point Implementation

The engine utilizes a custom `Fixed` struct. A 32-bit integer represents the number, where the upper 16 bits store the integer portion and the lower 16 bits store the fractional portion.

```cpp
struct Fixed16 {
    int32_t raw;
    
    constexpr Fixed16() : raw(0) {}
    constexpr explicit Fixed16(int32_t r) : raw(r) {}
    
    // Float conversion for rendering only
    static constexpr Fixed16 from_float(float f) {
        return Fixed16(static_cast<int32_t>(f * 65536.0f));
    }
    float to_float() const {
        return static_cast<float>(raw) / 65536.0f;
    }

    // Operator overloads for determinism
    Fixed16 operator+(const Fixed16& o) const { return Fixed16(raw + o.raw); }
    Fixed16 operator*(const Fixed16& o) const { 
        return Fixed16(static_cast<int32_t>((static_cast<int64_t>(raw) * o.raw) >> 16)); 
    }
};

```

### The Fixed Timestep Loop

The main loop decouples rendering framerate from the physics tick using an accumulator.

```cpp
const Fixed16 FIXED_DT = Fixed16::from_float(1.0f / 60.0f);
Fixed16 accumulator = Fixed16(0);

void engine_loop() {
    while (running) {
        Fixed16 frame_time = get_delta_time();
        accumulator = accumulator + frame_time;

        while (accumulator.raw >= FIXED_DT.raw) {
            PhysicsServer2D::get()->tick(FIXED_DT);
            SceneTree::get()->physics_process(FIXED_DT);
            accumulator.raw -= FIXED_DT.raw;
        }

        // Interpolate for rendering based on remaining accumulator fraction
        VisualServer::get()->render(accumulator.to_float() / FIXED_DT.to_float());
    }
}

```

---

## 3. Rendering Pipeline (`VisualServer`)

The `VisualServer` wraps an SDL3 backend. It renders strictly to a low-resolution internal buffer before upscaling to the user's monitor.

### Virtual Framebuffer & PAR Correction

1. **Render Target:** All sprites are batched and drawn to an off-screen `SDL_Texture` sized to native retro resolutions (e.g., $256 \times 224$).
2. **Sub-pixel Truncation:** When `Sprite2D` pushes its transform to the render queue, the `VisualServer` applies `floor()` to the interpolated fixed-point coordinates. This guarantees sprites always align perfectly with the screen's pixel grid.
3. **Final Blit:** The buffer is scaled to the window size. If Pixel Aspect Ratio (PAR) correction is enabled (to simulate 4:3 CRT stretching on a $256 \times 240$ NES buffer), the final shader uses bilinear interpolation *only* on the fractional remainder of the integer scale to prevent shimmering.

### Hardware Palette Swapping

Instead of storing 32-bit RGBA sprites, characters are stored as 8-bit indexed PNGs (using only the red channel).

```glsl
// GLSL Fragment Shader for Palette Swapping
in vec2 TexCoords;
out vec4 FragColor;

uniform sampler2D indexed_texture;
uniform sampler1D palette_lut; // 256-color Lookup Table

void main() {
    float index = texture(indexed_texture, TexCoords).r;
    FragColor = texture(palette_lut, index);
}

```

At runtime, changing a character's outfit requires swapping a single 256-byte 1D texture reference, costing near-zero GPU bandwidth.

---

## 4. Physics & Collision (`PhysicsServer2D`)

The engine abandons box2D in favor of an optimized AABB and Grid-Sweep algorithm.

### Swept AABB (Continuous Collision Detection)

To prevent fast-moving projectiles or a falling player from passing through thin floors, the engine calculates the Time of Impact (TOI).

1. Calculate the bounding box encompassing the entity's position at $t=0$ and its projected position at $t=1$.
2. Query the `TileMapLayer` grid for any solid blocks within this expanded bounds.
3. Calculate the exact fraction of the frame ($0.0$ to $1.0$) where the entity's leading edge intersects the tile surface.
4. Move the entity to that exact point of impact and project the remaining velocity along the surface normal (sliding).

### Handling One-Way Platforms

A tile marked `ONE_WAY` only registers as a collision if:

1. The `CharacterBody2D`'s Y-velocity is positive (falling downward).
2. The bottom edge of the entity's AABB in the *previous* frame was entirely above the top edge of the tile.
If the player presses "Down + Jump", the engine temporarily ignores the `ONE_WAY` bitmask for that specific entity for $10$ frames, allowing them to drop through.

---

## 5. Hot-Reloading & Serialization (Asset Pipeline)

Because Phase 1 lacks a visual editor, rapid iteration is achieved via runtime C++ hot-reloading.

1. **Game Logic as a Shared Library:** The engine core is an executable (`retronode.exe`). The user's game code is compiled as a shared library (`game.dll` / `libgame.so`).
2. **State Serialization:** When the engine detects a recompile of `game.dll`, it fires a `pre_reload` event. Every active Node serializes its state (position, health, inventory) to a JSON string in memory.
3. **Library Swap:** The engine unloads `game.dll`, loads the newly compiled version, and instantiates the new Node classes.
4. **State Deserialization:** The JSON state is injected back into the new Nodes via a `post_reload` event. The developer sees their code changes instantly without the game closing.

---

## 6. Advanced Optimization Features

To ensure the engine can scale to handling thousands of entities (e.g., bullet hells or massive particle systems), several backend optimizations are implemented:

### Spatial Hash Grid for `Area2D`

Instead of checking every `Area2D` against every other `Area2D` ($O(N^2)$), the `PhysicsServer2D` partitions the world into a coarse grid (e.g., $64 \times 64$ pixel cells).

* When a bullet moves, it updates its cell index.
* When checking for overlap, it only tests against other entities currently registered in its cell and immediate neighbors, reducing checks to $O(1)$ per entity.

### View-Frustum Culling

The `VisualServer` automatically maintains an AABB of the current `Camera2D` viewport. Before iterating the render queue, it performs a simple AABB overlap test against the chunked `TileMapLayer` and active `Sprite2D` bounds, instantly discarding draw calls for off-screen geometry.

### Object Pooling Backend

Instantiating nodes at runtime fragments memory. The engine provides an `ObjectPool<T>` template that pre-allocates contiguous arrays of bullets, enemies, or damage numbers. When a Node calls `queue_free()`, it is simply marked as inactive and returned to the pool, ensuring zero dynamic allocations during active gameplay.

---

# How to Build a Game with RetroNode

This guide walks through creating a top-down JRPG-style scene with grid-based collision, a player character, and a camera. We will define the scene using data, write the controller in C++, and hot-reload it into the engine.

## Step 1: Define the Scene Data

Since we are skipping the UI editor for the MVP, scenes are authored in a simple JSON format (`scene.json`). This tells the engine what Nodes to create on boot.

```json
{
  "name": "Overworld",
  "type": "Node2D",
  "children": [
    {
      "name": "WorldMap",
      "type": "TileMapLayer",
      "properties": {
        "tileset": "res://assets/overworld_tiles.png",
        "grid_size": 16,
        "map_data_path": "res://maps/town_01.ldtk"
      }
    },
    {
      "name": "Hero",
      "type": "CharacterBody2D",
      "script": "PlayerController",
      "properties": {
        "position": {"x": 128, "y": 128}
      },
      "children": [
        {
          "name": "Sprite",
          "type": "AnimatedSprite2D",
          "properties": {
            "sprite_frames": "res://assets/hero_anim.json",
            "current_anim": "idle_down"
          }
        },
        {
          "name": "Camera",
          "type": "Camera2D",
          "properties": {
            "mode": "DRAG_MARGIN",
            "margin": {"left": 64, "right": 64, "top": 64, "bottom": 64}
          }
        }
      ]
    }
  ]
}

```

## Step 2: Implement Game Logic in C++

Create a C++ file in your project directory (`src/player_controller.cpp`). This script links to the `CharacterBody2D` defined in the JSON. We will use the engine's fixed-point math and input singleton to handle movement.

```cpp
#include <retronode.h>

class PlayerController : public CharacterBody2D {
    RN_CLASS(PlayerController, CharacterBody2D);

private:
    Fixed16 move_speed = Fixed16::from_float(90.0f);
    AnimatedSprite2D* sprite = nullptr;

public:
    void _ready() override {
        // Retrieve the child sprite node defined in the JSON
        sprite = get_node<AnimatedSprite2D>("Sprite");
    }

    void _physics_process(Fixed16 delta) override {
        Fixed16 vel_x(0);
        Fixed16 vel_y(0);

        // Input Singleton handles standard WASD/D-Pad mapping
        if (Input::get()->is_action_pressed("ui_right")) vel_x = move_speed;
        if (Input::get()->is_action_pressed("ui_left"))  vel_x = move_speed * Fixed16(-1);
        if (Input::get()->is_action_pressed("ui_down"))  vel_y = move_speed;
        if (Input::get()->is_action_pressed("ui_up"))    vel_y = move_speed * Fixed16(-1);

        // Apply velocity to the CharacterBody2D
        this->velocity.x = vel_x;
        this->velocity.y = vel_y;

        // The Engine's PhysicsServer handles grid collision automatically here
        this->move_and_slide(); 

        // Update Animation State
        update_animation();
    }

    void update_animation() {
        if (this->velocity.x.raw > 0) sprite->play("walk_right");
        else if (this->velocity.x.raw < 0) sprite->play("walk_left");
        else if (this->velocity.y.raw > 0) sprite->play("walk_down");
        else if (this->velocity.y.raw < 0) sprite->play("walk_up");
        else sprite->play("idle_down");
    }
};

// Register the class so the engine can instantiate it from JSON
RN_REGISTER_CLASS(PlayerController);

```

## Step 3: Compiling and Hot-Reloading

RetroNode separates the engine runtime from your game code.

1. **Compile the Game:** Use CMake or your preferred compiler to build `src/player_controller.cpp` into a shared library (`game.dll` on Windows, `libgame.so` on Linux).
2. **Run the Engine:** Open your terminal and run the RetroNode executable, pointing it to your project directory:
```bash
retronode --project ./my_rpg_game

```


3. **Iterate:** The game window will open, loading `scene.json` and attaching your `PlayerController`. Walk your character around the map.
4. **Hot-Reload:** Keep the game running. Open `player_controller.cpp` in your editor, change `move_speed` to `150.0f`, and save. Recompile the DLL. RetroNode will automatically detect the file change, swap the library in memory, and your character will immediately start moving faster without you ever closing the game window.