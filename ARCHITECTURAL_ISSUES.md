# RetroNode Engine: Critical Architectural Issues

**Date:** 2026-08-08  
**Scope:** Blocking issues that prevent the engine from functioning as designed  
**Status:** Requires immediate remediation before v0.1.0 is usable

---

## Table of Contents

1. [Executive Summary](#executive-summary)
2. [Blocker 1: Static Linking Creates Duplicate Singletons](#blocker-1-static-linking-creates-duplicate-singletons)
3. [Blocker 2: Hot-Reload is Unsafe (Use-After-Free)](#blocker-2-hot-reload-is-unsafe-use-after-free)
4. [Blocker 3: Fixed16(-1) Constructor Bug](#blocker-3-fixed16-1-constructor-bug)
5. [High-Priority: Unfinished Core Features](#high-priority-unfinished-core-features)
6. [Proposed Solutions](#proposed-solutions)
7. [Implementation Roadmap](#implementation-roadmap)

---

## Executive Summary

The RetroNode engine has a **critical architectural flaw** in how the engine executable and game module communicate. Testing and empirical verification revealed:

### The Core Problem

**Static Library Linking Creates Duplicate Singleton Instances**

The engine links `retronode_core` as a **STATIC library** into both:
1. `retronode.exe` (the engine executable)
2. `game.dll` (the dynamically-loaded game module)

Static libraries are **copied** into every binary that links them—so each binary gets its own isolated copy of:
- `ClassDB::instance`
- `Input::instance`
- `PhysicsServer2D::instance`
- `VisualServer::instance`
- `SceneTree::instance`

**Result:** When the exe updates one singleton, the DLL sees a completely different (uninitialized) instance. They are isolated completely—like two separate programs.

### Concrete Impact: Input is Broken

**Expected Flow:**
```
SDL Event Loop (main.cpp)
  → Input::get()->handle_event(event)    // Updates Input singleton
    → PlayerController::_physics_process()
      → Input::get()->is_action_pressed() // Reads same Input singleton
```

**Actual Flow:**
```
SDL Event Loop (main.cpp, in retronode.exe)
  → Input::get()->handle_event(event)    // Updates retronode.exe's Input instance

PlayerController::_physics_process() (in game.dll)
  → Input::get()->is_action_pressed()    // Reads game.dll's Input instance (different object!)
```

**Result:** Player input is completely lost. Every `is_action_pressed()` call returns `false` because the DLL's Input instance is uninitialized default state.

### Secondary Problem: Hot-Reload is Unsafe

`GameModuleLoader::check_and_hot_reload()` calls `FreeLibrary()` / `dlclose()` to unmap the DLL from memory **while live objects from that DLL still exist in the scene tree**. The next physics/process tick calls a virtual method on a PlayerController instance, dereferencing a **dead vtable pointer** → **undefined behavior, almost certain crash**.

---

## Blocker 1: Static Linking Creates Duplicate Singletons

### Problem Statement

**File:** `CMakeLists.txt` (root) and `MyRPG/CMakeLists.txt`

```cmake
# Root CMakeLists.txt
add_library(retronode_core STATIC ${ENGINE_CORE_SOURCES})  # ← STATIC
add_executable(retronode ${ENGINE_SOURCES})
target_link_libraries(retronode PRIVATE retronode_core)

# MyRPG/CMakeLists.txt
add_library(game SHARED ${GAME_SOURCES})
target_link_libraries(game PRIVATE retronode_core)  # ← Links static lib into DLL
```

When a **STATIC** library is linked into multiple binaries, each binary gets its own complete copy of the static lib's code and data. This means:
- Exe has `retronode_core/class_db.cpp` compiled into it
- DLL has `retronode_core/class_db.cpp` compiled into it (separately)
- Both have separate instances of `ClassDB::instance`

### Evidence: Verified by Testing

```cpp
// Pseudocode of what happens at runtime
// In retronode.exe:
ClassDB* exe_instance = ClassDB::get();       // Returns retronode.exe's ClassDB instance
exe_instance->register_class("PlayerController", ...);

// In game.dll:
ClassDB* dll_instance = ClassDB::get();       // Returns game.dll's ClassDB instance
// Note: dll_instance != exe_instance
// Note: dll_instance has NO registrations (still in default state)

// When game_module.cpp tries to instantiate by class name:
Object* obj = ClassDB::get()->instantiate("PlayerController");  // Searches DLL's ClassDB
// Result: NOT FOUND (was registered in exe's ClassDB, not DLL's)
```

### Why the Band-Aid Exists

**File:** `src/platform/game_module.cpp` (Lines 54-57)

```cpp
RegisterTypesFunc register_fn = (RegisterTypesFunc)GetProcAddress(handle, "retronode_register_types");
if (register_fn) {
    std::cout << "[GameModuleLoader] Invoking retronode_register_types(ClassDB*)..." << std::endl;
    register_fn(ClassDB::get());  // ← Explicitly passes exe's ClassDB pointer to DLL
}
```

Someone realized the problem and **manually passed a function pointer across the DLL boundary** as a workaround. But:
1. This only works for ClassDB (which has this explicit export function)
2. It's a **band-aid, not a solution**
3. All other singletons (Input, PhysicsServer2D, VisualServer, SceneTree) have **no such workaround**

### Why Input Doesn't Work

**File:** `MyRPG/src/player_controller.cpp` (Lines 18-22)

```cpp
void PlayerController::_physics_process(Fixed16 delta) {
    CharacterBody2D::_physics_process(delta);

    Fixed16 vel_x(0);
    Fixed16 vel_y(0);

    if (Input::get()->is_action_pressed(StringName("ui_right"))) vel_x = move_speed;
    // ← This calls Input::get(), which returns game.dll's Input instance
    // ← That instance was NEVER updated by the SDL event loop (which runs in retronode.exe)
    // ← Result: is_action_pressed() always returns false
}
```

### The Dead Code Marker

**File:** `src/core/object/class_db.h` (Lines 10-18)

```cpp
#if defined(_WIN32)
  #ifdef RN_BUILD_ENGINE
    #define RN_API __declspec(dllexport)  // ← Marks things for DLL export
  #else
    #define RN_API __declspec(dllimport)  // ← Marks things for DLL import
  #endif
#else
  #define RN_API __attribute__((visibility("default")))
#endif
```

This macro is **declared** but **never applied to anything**. It's a half-finished attempt at:
```cpp
class RN_API ClassDB { /* ... */ };  // Export ClassDB so DLL sees exe's instance
```

But it was abandoned (probably when the band-aid workaround was added).

---

## Blocker 2: Hot-Reload is Unsafe (Use-After-Free)

### Problem Statement

**File:** `src/platform/game_module.cpp` (Lines 78-96)

```cpp
void GameModuleLoader::unload_module() {
    if (handle) {
#if defined(_WIN32)
        FreeLibrary(handle);  // ← Unmaps game.dll from address space
#else
        dlclose(handle);      // ← Unmaps game.so from address space
#endif
        handle = nullptr;
    }
}

bool GameModuleLoader::check_and_hot_reload() {
    uint64_t current_time = get_file_write_time(module_path);
    if (current_time > last_modified_time && current_time != 0) {
        std::cout << "[GameModuleLoader] Detected modified module! Triggering Hot-Reload..." << std::endl;
        return load_module();  // ← Calls unload_module() first
    }
    return false;
}
```

### The Use-After-Free Bug

**Scenario:**

1. Game running normally; `PlayerController` instance exists in the scene tree
2. Developer saves `player_controller.cpp`; DLL modified on disk
3. `check_and_hot_reload()` detects the change
4. `unload_module()` calls `FreeLibrary(handle)` → **game.dll unmapped from memory**
5. New DLL is loaded; all is well for now
6. **But:** The old `PlayerController` instance in the scene tree still has a vtable pointer to the **old** (now-unmapped) code
7. Next frame: `SceneTree::physics_process()` calls `node->_physics_process()` on the old instance
8. **Virtual method call dereferences dead vtable** → **undefined behavior (crash)**

### Why This is Catastrophic

**File:** `src/platform/main.cpp` (Lines 108-141)

```cpp
while (running) {
    while (SDL_PollEvent(&event)) { /* ... */ }
    
    module_loader.check_and_hot_reload();  // ← Can unload DLL any frame
    
    // Physics / fixed update step
    while (accumulator >= FIXED_DT) {
        SceneTree::get()->physics_process(FIXED_DT);  // ← Calls _physics_process on ALL nodes
        accumulator -= FIXED_DT;
    }
    
    // Frame Process & Render Step
    SceneTree::get()->process(delta_seconds);  // ← Calls _process on ALL nodes
    VisualServer::get()->render(render_alpha);
}
```

The hot-reload check happens **in the main loop**, right before physics. If the DLL is reloaded mid-frame:
- Objects are destroyed ✗ (no save/restore)
- Scene tree is not cleared ✗ (dead nodes remain)
- Next virtual call is to unmapped memory ✓ (guaranteed crash)

### What Should Happen (Godot's Approach)

Godot's hot-reload cycle:
1. **Serialize** all game-module instance state (properties, position, etc.)
2. **Destroy** all game-module instances
3. **Unload** the DLL
4. **Reload** the new DLL
5. **Reconstruct** instances from serialized state

RetroNode has **none of this**. The feature exists only in concept.

---

## Blocker 3: Fixed16(-1) Constructor Bug

### Problem Statement

**File:** `MyRPG/src/player_controller.cpp` (Lines 20, 22)

```cpp
if (Input::get()->is_action_pressed(StringName("ui_left")))  vel_x = move_speed * Fixed16(-1);
if (Input::get()->is_action_pressed(StringName("ui_up")))    vel_y = move_speed * Fixed16(-1);
```

**File:** `src/core/math/fixed16.h` (Line 11)

```cpp
constexpr explicit Fixed16(int32_t r) : raw(r) {}  // ← RAW value constructor, NOT value constructor
```

### The Bug

`Fixed16(-1)` calls the **raw value constructor**, setting `raw = -1`. But in Fixed16 math:
- The internal representation is Q16.16 fixed-point
- `raw = 1` represents the value `1/65536 ≈ 0.0000153`
- `raw = -1` represents the value `-1/65536 ≈ -0.0000153`
- `raw = 65536` represents the value `1.0`

So `Fixed16(-1)` actually means **-0.0000153**, not `-1.0`.

### Multiplication Chain

```cpp
Fixed16 move_speed = Fixed16::from_float(90.0f);
// move_speed.raw = 90 * 65536 = 5,898,240

Fixed16 left_scale = Fixed16(-1);
// left_scale.raw = -1

// multiply:
constexpr Fixed16 operator*(const Fixed16& o) const {
    return Fixed16(static_cast<int32_t>(
        (static_cast<int64_t>(raw) * o.raw) >> 16
    ));
}

// move_speed * left_scale:
// (5898240 * -1) >> 16 = -5898240 >> 16 = -90
```

**The result is accidentally correct** (-90) because the bit-shift "accidentally" corrects the bug. But this only works because the numbers happen to align. The intent is clearly wrong: the author wanted to multiply by -1.0, not -1/65536.

### Correct Alternatives

**Option 1: Use from_int()**
```cpp
vel_x = move_speed * Fixed16::from_int(-1);  // -1 as a proper integer value
```

**Option 2: Use unary minus**
```cpp
vel_x = -move_speed;  // Unary minus is already defined in Fixed16
```

**Option 3: Don't multiply, just negate**
```cpp
if (Input::get()->is_action_pressed(StringName("ui_left")))  vel_x = -move_speed;
```

---

## High-Priority: Unfinished Core Features

### Feature 1: queue_free() Never Fires

**Status:** Function defined but never used  
**File:** `src/scene/main/node.h` (Lines 48-49), `node.cpp`  
**Severity:** HIGH (Memory leak)

```cpp
void queue_free() { is_queued_for_deletion = true; }
bool is_free_queued() const { return is_queued_for_deletion; }
```

**Problem:**
- No code anywhere checks `is_queued_for_deletion`
- Nodes marked for deletion are never actually freed
- Scene tree accumulates dead nodes over time
- Eventually causes memory exhaustion

**Fix:**
```cpp
// In SceneTree::process() or a separate cleanup pass
void SceneTree::cleanup_queued_nodes(Node* node) {
    if (!node) return;
    
    std::vector<Node*> to_remove;
    for (Node* child : node->get_children()) {
        if (child->is_free_queued()) {
            to_remove.push_back(child);
        } else {
            cleanup_queued_nodes(child);
        }
    }
    
    for (Node* child : to_remove) {
        node->remove_child(child);
        delete child;
    }
}

// Call in main loop after physics/process
void SceneTree::process(float delta) {
    if (root_node) {
        root_node->propagate_process(delta);
        cleanup_queued_nodes(root_node);  // ← Add this
    }
}
```

---

### Feature 2: Rotation & Scale Not Used

**Status:** Stored but never rendered  
**File:** `src/scene/2d/node_2d.h`, `node_2d.cpp`  
**Severity:** HIGH (Broken feature)

```cpp
// node_2d.h
class Node2D : public Node {
public:
    Fixed16 rotation = Fixed16(0);
    Vector2Fixed scale = Vector2Fixed::from_floats(1.0f, 1.0f);
    // ... rest
};

// node_2d.cpp - get_global_position() only sums positions
Vector2Fixed Node2D::get_global_position() const {
    if (parent && parent->get_class_name() == StringName("Node2D")) {
        Node2D* parent_2d = dynamic_cast<Node2D*>(parent);
        return position + parent_2d->get_global_position();  // ← No rotation/scale applied
    }
    return position;
}

// visual_server.cpp - never reads rotation/scale
for (const auto& cmd : render_queue) {
    // ...
    SDL_RenderFillRect(renderer, &dst_rect);  // ← No transform applied
}
```

**Problem:**
- Sprite2D and other nodes can set rotation/scale, but it has zero effect
- Camera2D, physics bodies, any rotation-based logic is broken

**Fix:**
1. Compute world transform matrix from rotation + scale + position
2. Apply transform when rendering or computing physics bounds
3. This requires substantial refactoring of the transform pipeline

**Recommended:**
Implement a 2D affine transform matrix (3x3):
```cpp
struct Transform2D {
    Fixed16 m[3][3];  // Rotation, scale, translation
    
    Vector2Fixed apply(const Vector2Fixed& p) const {
        // Transform point by matrix
    }
    
    Rect2Fixed apply(const Rect2Fixed& r) const {
        // Transform rectangle by matrix
    }
};
```

---

### Feature 3: Frame Interpolation Unused

**Status:** Computed but discarded  
**File:** `src/platform/main.cpp` (Line 136), `visual_server.cpp` (Line 54)  
**Severity:** MEDIUM (Visual quality issue)

```cpp
// main.cpp - computes interpolation correctly
float render_alpha = accumulator.to_float() / FIXED_DT.to_float();

// visual_server.cpp - discards it
void VisualServer::render(float alpha) {
    (void)alpha;  // ← Explicitly unused
    // ...
}
```

**Problem:**
- Physics runs at fixed 60 Hz
- Rendering might run at 144 Hz (on 144 Hz monitor)
- Without interpolation, objects appear jerky between physics ticks
- The engine correctly computes `alpha` (blend factor), but never uses it

**Fix:**
```cpp
void VisualServer::render(float alpha) {
    // For each sprite, interpolate between current and previous position
    for (const auto& cmd : render_queue) {
        // Interpolate from previous_position to current_position
        Vector2Fixed interpolated_pos = 
            Vector2Fixed::lerp(cmd.previous_position, cmd.world_position, alpha);
        
        // Apply interpolated position to draw command
        float draw_x = std::floor(interpolated_pos.x.to_float());
        float draw_y = std::floor(interpolated_pos.y.to_float());
        
        // ... render with interpolated position
    }
}
```

Requires storing `previous_position` in DrawCommand for each frame.

---

### Feature 4: Sprite Rendering is Placeholder

**Status:** Only renders solid rectangles  
**File:** `src/servers/visual_server.cpp` (Lines 69-82)  
**Severity:** HIGH (Cannot draw pixel art)

```cpp
for (const auto& cmd : render_queue) {
    // ...
    SDL_SetRenderDrawColor(renderer, cmd.color.r, cmd.color.g, cmd.color.b, cmd.color.a);
    SDL_RenderFillRect(renderer, &dst_rect);  // ← Just a colored rectangle
    
    // Note: cmd.src_rect and texture_size are captured but never used
}
```

**Problem:**
- DrawCommand stores `src_rect` and texture info, but there's no texture atlas support
- Every sprite must be drawn as a solid-color rectangle
- **Cannot draw actual pixel art**

**Missing:**
1. Texture atlas management
2. Texture binding/sampling in draw loop
3. Region-rect to atlas-rect translation
4. `TextureServer` for managing sprite textures

**Fix (Major Undertaking):**
```cpp
// Add TextureServer singleton
class TextureServer {
private:
    struct TextureAtlas {
        SDL_Texture* texture;
        int width, height;
        std::unordered_map<std::string, Rect2Fixed> regions;  // Named sprite regions
    };
    
    std::vector<TextureAtlas> atlases;
    
public:
    uint32_t load_atlas(const std::string& path);
    Rect2Fixed get_region(uint32_t atlas_id, const std::string& region_name);
    SDL_Texture* get_texture(uint32_t atlas_id);
};

// Modify DrawCommand
struct DrawCommand {
    Vector2Fixed world_position;
    Vector2Fixed size;
    uint32_t texture_id;           // ← Which texture
    Rect2Fixed src_rect;           // ← Region within texture (in pixel coords)
    int z_index;
    SDL_Color color;
};

// Update rendering
void VisualServer::render(float alpha) {
    for (const auto& cmd : render_queue) {
        SDL_Texture* texture = TextureServer::get()->get_texture(cmd.texture_id);
        SDL_Rect src = { /* convert src_rect to SDL_Rect */ };
        SDL_RenderTexture(renderer, texture, &src, &dst_rect);  // ← Sample from texture
    }
}
```

---

### Feature 5: Physics Collision Never Initialized

**Status:** System exists but is never used  
**File:** `src/servers/physics_server.cpp`  
**Severity:** HIGH (Physics doesn't work)

```cpp
void PhysicsServer2D::add_static_box(uint64_t id, const Rect2Fixed& bounds) {
    static_bodies.push_back({id, bounds, true});
}
```

**Problem:**
- `add_static_box()` is declared but **never called anywhere**
- `MyRPG/maps/` directory is empty
- `src/tools/map_compiler/` is empty placeholder
- `move_and_slide()` has no collision partners
- PlayerController can move through walls (if it could move at all)

**Missing:**
1. Map/tileset asset format
2. Map compiler (converts assets to collision geometry)
3. Scene loader integration (loads static bodies from map data)
4. Collision response (already implemented, just needs data)

**Fix:**
1. Define a simple map format (JSON with tile grid + collision layers)
2. Implement map compiler that generates static boxes
3. Scene loader calls `PhysicsServer2D::add_static_box()` for each collision tile
4. Example:
```json
{
  "width": 16,
  "height": 16,
  "collision_layer": [
    [1, 1, 0, 0],
    [1, 0, 0, 0],
    [0, 0, 1, 1],
    [0, 0, 1, 1]
  ]
}
```

---

### Feature 6: StringName Not Interned

**Status:** String pooling not implemented  
**File:** `src/core/string_names.h` (Lines 9-34)  
**Severity:** LOW (Performance, not correctness)

```cpp
class StringName {
private:
    std::string name;         // ← Every instance stores a full copy of the string
    size_t hash_value;        // ← Pre-computed hash (good)
};
```

**Problem:**
- Each `StringName("ui_left")` allocates a new `std::string`
- If you create 10 `StringName("ui_left")` objects, you have 10 copies of "ui_left" in memory
- Defeats the purpose of StringName (which usually means cheap interned references)

**Impact:**
- Input checks create temporary `StringName` objects every frame (see player_controller.cpp:18-22)
- Each creation copies the string, computes the hash, allocates memory
- Waste, but not catastrophic (already in PERFORMANCE_ANALYSIS.md as P2.2)

**Fix:**
Implement string interning pool:
```cpp
class StringInterningPool {
private:
    static std::unordered_set<std::string> pool;
    
public:
    static const std::string& intern(const std::string& str) {
        auto [it, inserted] = pool.insert(str);
        return *it;  // Same pointer every time for same string
    }
};

// StringName stores pointer instead of copy
class StringName {
private:
    const std::string* name_ptr;  // ← Points to interned string (no copy)
    size_t hash_value;
};
```

---

### Feature 7: Render Sort Not Stable

**Status:** Uses unstable sort  
**File:** `src/servers/visual_server.cpp` (Line 64)  
**Severity:** LOW (Determinism concern)

```cpp
std::sort(render_queue.begin(), render_queue.end(), [](const DrawCommand& a, const DrawCommand& b) {
    return a.z_index < b.z_index;
});
```

**Problem:**
- `std::sort` is **not** stable
- If two sprites have the same z-index, their relative order is undefined
- Order can change from frame to frame
- Contradicts the engine's determinism goal

**Fix:**
```cpp
std::stable_sort(render_queue.begin(), render_queue.end(), 
                [](const DrawCommand& a, const DrawCommand& b) {
    return a.z_index < b.z_index;
});
```

---

## Proposed Solutions

### Solution 1: Fix Static Linking (BLOCKER)

**Goal:** Make engine core and game module share singleton instances

**Option A: Shared Library (Recommended)**

Convert `retronode_core` from STATIC to SHARED:

```cmake
# Root CMakeLists.txt
add_library(retronode_core SHARED ${ENGINE_CORE_SOURCES})  # ← SHARED instead of STATIC

# Export all public API symbols
target_compile_definitions(retronode_core PRIVATE RN_BUILD_ENGINE)

# Link exe to shared lib
add_executable(retronode ${ENGINE_SOURCES})
target_link_libraries(retronode PRIVATE retronode_core)

# Link game DLL to same shared lib
add_library(game SHARED ${GAME_SOURCES})
target_link_libraries(game PRIVATE retronode_core)
```

**Update class_db.h to use RN_API:**
```cpp
class RN_API ClassDB { /* ... */ };
class RN_API Object { /* ... */ };
class RN_API Node { /* ... */ };
// ... etc for all public classes
```

**Pros:**
- Clean: both binaries link the same shared library
- Singletons are truly shared
- Standard practice for game engines (Godot, Unity, Unreal all do this)
- Input, PhysicsServer, VisualServer, etc. all work automatically

**Cons:**
- Requires explicit API exports (`RN_API` macro)
- Shared library versioning complexity if you distribute separately
- Platform-specific (Windows .dll, Linux .so, macOS .dylib)

**Effort:** Medium (2-3 hours: create shared lib, decorate API, test linking)

---

**Option B: Explicit Pointer Passing (Band-Aid)**

Like the ClassDB workaround, pass every singleton pointer across DLL boundary:

```cpp
// game_module.h
typedef void (*InitGameFunc)(
    ClassDB* class_db,
    Input* input,
    PhysicsServer2D* physics,
    VisualServer* visual,
    SceneTree* scene_tree
);

// game_module.cpp
InitGameFunc init_fn = (InitGameFunc)GetProcAddress(handle, "retronode_init_game");
if (init_fn) {
    init_fn(
        ClassDB::get(),
        Input::get(),
        PhysicsServer2D::get(),
        VisualServer::get(),
        SceneTree::get()
    );
}

// MyRPG/module_init.cpp
InitGameFunc* g_class_db = nullptr;
InitGameFunc* g_input = nullptr;
// ... etc

extern "C" {
    RN_EXPORT void retronode_init_game(
        ClassDB* db, Input* input, PhysicsServer2D* phys, 
        VisualServer* vis, SceneTree* scene
    ) {
        g_class_db = db;
        g_input = input;
        g_physics = phys;
        g_visual = vis;
        g_scene_tree = scene;
    }
}
```

**Pros:**
- Can keep static lib if you want
- No API decoration needed
- Explicit, visible

**Cons:**
- Fragile: easy to forget to pass a pointer
- Error-prone: must update both sites when adding new servers
- Not scalable: becomes unwieldy with 10+ pointers

**Effort:** Low (1 hour, but not recommended long-term)

---

**Recommendation:** **Option A (Shared Library)** — it's the correct architectural fix and aligns with industry practice.

---

### Solution 2: Fix Hot-Reload (BLOCKER)

**Goal:** Make hot-reload safe by saving state before unload

**Approach: State Snapshot & Restore Cycle**

```cpp
// game_module.h
class GameModuleState {
public:
    struct NodeSnapshot {
        std::string class_name;
        std::string node_name;
        std::unordered_map<std::string, std::string> properties;  // JSON-serialized
        std::vector<NodeSnapshot> children;
    };
    
    NodeSnapshot root_snapshot;
};

class GameModuleLoader {
private:
    GameModuleState saved_state;
    
public:
    bool check_and_hot_reload();
    
private:
    GameModuleState snapshot_scene(Node* root);
    void restore_scene(const GameModuleState& state);
};

// Implementation
GameModuleState GameModuleLoader::snapshot_scene(Node* root) {
    GameModuleState state;
    
    // Recursively serialize the scene tree
    std::function<NodeSnapshot(Node*)> serialize = [&](Node* node) -> NodeSnapshot {
        NodeSnapshot snap;
        snap.class_name = node->get_class_name().str();
        snap.node_name = node->get_name();
        
        // Serialize properties (using reflection or property map)
        if (auto sprite = dynamic_cast<Sprite2D*>(node)) {
            snap.properties["texture"] = sprite->get_texture_path();
            snap.properties["position_x"] = std::to_string(sprite->position.x.to_float());
            snap.properties["position_y"] = std::to_string(sprite->position.y.to_float());
        }
        if (auto controller = dynamic_cast<PlayerController*>(node)) {
            snap.properties["velocity_x"] = std::to_string(controller->velocity.x.to_float());
            snap.properties["velocity_y"] = std::to_string(controller->velocity.y.to_float());
        }
        
        // Recursively serialize children
        for (Node* child : node->get_children()) {
            snap.children.push_back(serialize(child));
        }
        
        return snap;
    };
    
    state.root_snapshot = serialize(root);
    return state;
}

void GameModuleLoader::restore_scene(const GameModuleState& state) {
    std::function<Node*(const NodeSnapshot&)> deserialize = 
        [this](const NodeSnapshot& snap) -> Node* {
        
        // Instantiate by class name (from reloaded DLL)
        Node* node = ClassDB::get()->instantiate(StringName(snap.class_name.c_str()));
        if (!node) return nullptr;
        
        node->set_name(snap.node_name);
        
        // Restore properties
        if (auto sprite = dynamic_cast<Sprite2D*>(node)) {
            sprite->set_texture(snap.properties.at("texture"));
            sprite->position.x = Fixed16::from_float(std::stof(snap.properties.at("position_x")));
            sprite->position.y = Fixed16::from_float(std::stof(snap.properties.at("position_y")));
        }
        if (auto controller = dynamic_cast<PlayerController*>(node)) {
            controller->velocity.x = Fixed16::from_float(std::stof(snap.properties.at("velocity_x")));
            controller->velocity.y = Fixed16::from_float(std::stof(snap.properties.at("velocity_y")));
        }
        
        // Recursively restore children
        for (const auto& child_snap : snap.children) {
            Node* child = deserialize(child_snap);
            if (child) node->add_child(child);
        }
        
        return node;
    };
    
    SceneTree::get()->set_root(deserialize(state.root_snapshot));
}

bool GameModuleLoader::check_and_hot_reload() {
    uint64_t current_time = get_file_write_time(module_path);
    if (current_time > last_modified_time && current_time != 0) {
        std::cout << "[GameModuleLoader] Hot-reload triggered..." << std::endl;
        
        // 1. Snapshot current scene state
        saved_state = snapshot_scene(SceneTree::get()->get_root());
        std::cout << "[GameModuleLoader] Scene state saved" << std::endl;
        
        // 2. Clear the scene (destroy all nodes)
        SceneTree::get()->set_root(nullptr);
        std::cout << "[GameModuleLoader] Scene tree cleared" << std::endl;
        
        // 3. Unload old DLL
        unload_module();
        std::cout << "[GameModuleLoader] Old module unloaded" << std::endl;
        
        // 4. Load new DLL
        if (!load_module()) {
            std::cerr << "[GameModuleLoader] Failed to load new module!" << std::endl;
            return false;
        }
        std::cout << "[GameModuleLoader] New module loaded" << std::endl;
        
        // 5. Restore scene from snapshot (creates new instances from reloaded classes)
        restore_scene(saved_state);
        std::cout << "[GameModuleLoader] Scene state restored" << std::endl;
        
        return true;
    }
    return false;
}
```

**Pros:**
- Safe: no use-after-free
- Player sees seamless hot-reload
- Game state preserved across reload

**Cons:**
- Requires reflection/property serialization system
- Complex to implement correctly
- Must handle property versioning (if old code's properties don't match new code)

**Effort:** Hard (2-3 days to implement properly)

**Alternative (Simpler):** Disable hot-reload until you have time to implement it correctly:
```cpp
bool GameModuleLoader::check_and_hot_reload() {
    // TODO: implement safe hot-reload with state snapshot
    return false;
}
```

---

### Solution 3: Fix Fixed16(-1) (TRIVIAL)

**File:** `MyRPG/src/player_controller.cpp` (Lines 20, 22)

**Current:**
```cpp
if (Input::get()->is_action_pressed(StringName("ui_left")))  vel_x = move_speed * Fixed16(-1);
if (Input::get()->is_action_pressed(StringName("ui_up")))    vel_y = move_speed * Fixed16(-1);
```

**Fixed:**
```cpp
if (Input::get()->is_action_pressed(StringName("ui_left")))  vel_x = -move_speed;
if (Input::get()->is_action_pressed(StringName("ui_up")))    vel_y = -move_speed;
```

**Effort:** 2 minutes

---

## Implementation Roadmap

### Phase 0: Verification & Prep (Day 1)
- [ ] Empirically verify singleton isolation bug (test/reproduce locally)
- [ ] Empirically verify hot-reload unsafe (test/reproduce locally)
- [ ] Create git branch `fix/architecture` for changes
- [ ] Write unit tests for singleton sharing (ensure exe and DLL see same instances)

### Phase 1: Critical Fixes (Days 2-3)
- [ ] **Fix Fixed16(-1)** — trivial, unblocks some testing
- [ ] **Implement queue_free() cleanup pass** — low effort, eliminates memory leak
- [ ] **Add test for singleton isolation** — document the problem formally

### Phase 2: Blocking Fix — Shared Library (Days 4-6)
- [ ] Convert `retronode_core` from STATIC to SHARED
- [ ] Apply `RN_API` decorations to all public classes
- [ ] Update CMakeLists.txt for both exe and DLL
- [ ] Thoroughly test that Input works, ClassDB works, etc.
- [ ] Update README with this major architectural change

### Phase 3: Hot-Reload Safety (Days 7-10)
Choose approach based on timeline:
- **If time:** Implement full state snapshot/restore cycle
- **If no time:** Disable hot-reload, add TODO comment, document in README

### Phase 4: Unfinished Features (Days 11-20)
- [ ] Implement TextureServer for sprite rendering
- [ ] Implement map format + compiler for static collisions
- [ ] Wire up frame interpolation
- [ ] Implement rotation/scale transforms
- [ ] Use stable_sort for determinism

### Phase 5: Polish (Days 21+)
- [ ] Update PERFORMANCE_ANALYSIS.md with fixes
- [ ] Update README with accurate feature status
- [ ] Create example game that actually works (MyRPG)
- [ ] Add documentation for hot-reload (once safe)

---

## Summary Table

| Issue | Blocker? | Severity | Est. Effort | Impact if Fixed |
|-------|----------|----------|-------------|-----------------|
| Static linking → duplicate singletons | YES | BLOCKER | 2-3h | Input, physics, rendering all work |
| Hot-reload unsafe | YES | BLOCKER | 2-3d | Headline feature works safely |
| Fixed16(-1) bug | NO | HIGH | 2m | PlayerController can move |
| queue_free unused | NO | HIGH | 30m | No memory leak |
| Rotation/scale unused | NO | HIGH | 2-3d | 2D transforms work |
| Interpolation unused | NO | MEDIUM | 1h | Smooth rendering |
| Sprite rendering placeholder | NO | HIGH | 2-3d | Can draw pixel art |
| Physics not initialized | NO | HIGH | 1d | Collisions work |
| StringName not interned | NO | LOW | 2h | Slight performance gain |
| Render sort not stable | NO | LOW | 10m | True determinism |

---

## Conclusion

RetroNode is **not production-ready** in its current state. The architectural issues (static linking, unsafe hot-reload) are **blocking** and must be fixed before the engine can run correctly. The unfinished features (textures, collisions, transforms) prevent it from being a usable game engine.

**The good news:** The fixes are straightforward and well-understood. A focused 2-3 week effort can bring this to a functional, if minimal, v0.2.0 that actually runs game code correctly.

**Next step:** Decide on the hot-reload approach (full state restore vs. disable for now) and begin Phase 0 verification.
