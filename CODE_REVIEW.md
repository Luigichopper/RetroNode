# RetroNode — Code Review: Critical Errors & Unprofessional Choices

**Repo:** [Luigichopper/RetroNode](https://github.com/Luigichopper/RetroNode)
**Commit reviewed:** `64bf942` ("feat: implement AnimationPlayer node and integrate into engine and player controller")
**Method:** Full read-through of `src/` and `MyRPG/`, plus an actual build (CMake + g++ on Linux, SDL3 fetched via `FetchContent`) and headless runtime test (`SDL_VIDEODRIVER=offscreen`). Items marked **(verified at runtime)** were reproduced by running the compiled engine, not just inferred from reading code.
**Status:** RESOLVED

---

## 1. Critical Errors

### 1.1 The game module can never be found on Linux/macOS — the whole example is unbuildable-and-runnable off Windows **(verified at runtime)**

`src/platform/main.cpp::find_game_dll()` hardcodes the literal filename `game.dll` in every search candidate:

```cpp
std::vector<std::string> candidates = {
    proj_dir + "/bin/Debug/game.dll",
    proj_dir + "/bin/game.dll",
    ...
    "./game.dll"
};
```

But the README itself documents the engine as cross-platform and says the build produces `game.dll` **or** `libgame.so`. `src/platform/game_module.cpp` even contains a full `dlopen`/`dlclose` code path guarded by `#if !defined(_WIN32)` — so someone clearly intended Linux/macOS support. The problem is the file-discovery step never looks for anything but `.dll`.

I built the project on Linux exactly per the README's CMake instructions. The build succeeds and produces `MyRPG/bin/game.so`. Running the engine then fails silently at the game-logic level:

```
[RetroNode Engine] Game module path:  /home/claude/RetroNode/MyRPG/bin/Debug/game.dll
[GameModuleLoader] Failed to dlopen shared object: .../game.dll: invalid ELF header
...
[ClassDB] Warning: Could not instantiate unregistered class 'PlayerController'
[SceneLoader] ClassDB fallback to base type 'CharacterBody2D' for script 'PlayerController'
```

The engine doesn't crash — it silently substitutes the base `CharacterBody2D` for the player, so the game "runs" with a player object that never moves, never plays sounds, and never animates, and nothing in the console output makes that obvious to a user. To confirm this was purely a naming problem (and not a deeper cross-platform issue), I copied the real `game.so` to the exact wrong path/name the loader expects (`bin/Debug/game.dll`) — `dlopen` doesn't care about extensions on Linux, and the module then loaded and ran correctly. The bug is entirely in the hardcoded discovery logic.

### 1.2 Hot-reload permanently blanks the game and never restores it **(verified at runtime)**

This is the engine's flagship advertised feature ("Hot-Reloadable Game Logic... swapped at runtime without restarting the engine executable"). In practice:

```cpp
// src/platform/game_module.cpp
bool GameModuleLoader::check_and_hot_reload() {
    uint64_t current_time = get_file_write_time(module_path);
    if (current_time > last_modified_time && current_time != 0) {
        std::cout << "[GameModuleLoader] Detected modified module! Safely resetting scene tree..." << std::endl;
        SceneTree::get()->set_root(nullptr);
        return load_module();
    }
    return false;
}
```

`set_root(nullptr)` destroys the entire live scene, and `load_module()` reloads the DLL — but **nothing ever calls `SceneLoader::load_scene_from_file()` again**. `load_scene_from_file` is only ever invoked once, from `main()`, before the loop starts. I confirmed this by running the engine and touching the DLL's mtime mid-session:

```
[RetroNode Engine] Loaded initial scene: .../overworld.rnb
[GameModuleLoader] Detected modified module! Safely resetting scene tree...
[MyRPG] Registering PlayerController into ClassDB...
[GameModuleLoader] Successfully loaded game module: .../game.dll
```

No further "Loaded binary scene file" or "Loaded initial scene" log line ever appears — the process keeps running, but the world is empty forever after. The one feature that gives this engine its name doesn't survive its own first use.

### 1.3 Every played sound effect leaks an `SDL_AudioStream`

`src/servers/audio_server.cpp::play_sound()`:

```cpp
SDL_AudioStream* sound_stream = SDL_CreateAudioStream(&s.spec, &dst_spec);
...
SDL_BindAudioStream(audio_device, sound_stream);
...
SDL_PutAudioStreamData(sound_stream, ...);
SDL_FlushAudioStream(sound_stream);
// <-- function ends here; sound_stream is never unbound or destroyed
```

`SDL_DestroyAudioStream` is called on the two early-`return` error paths, but never on the success path. Since `PlayerController::_physics_process` calls `hurt_audio->play()` on every "attack" keypress, every sound effect played over the life of the process permanently leaks a bound audio stream. In a real play session this accumulates without bound.

### 1.4 The `visible` flag on UI nodes does nothing

`Control` and `CanvasLayer` both declare `bool visible = true;`, and `DebugOverlay` even implements an F3-style toggle (`visible = !visible;`) with an early-return that skips its *own* FPS bookkeeping when hidden. But:

* `Node::propagate_process()` always recurses into every child unconditionally, regardless of whether the parent's `_process()` returned early.
* `Label::_process`, `NinePatchRect::_process`, `Sprite2D::_process`, and `TileMapLayer::_process` never check `visible` on themselves or any ancestor before calling `VisualServer::submit_draw_sprite()`.

So even setting `DebugOverlay::visible = false` only stops the overlay from recomputing its text — the `fps_label`/`info_label` children keep drawing every frame regardless. `visible` is present on two classes and referenced by one, but has zero actual effect on rendering anywhere in the engine.

### 1.5 Two input actions used in gameplay code are never bound to any key **(verified)**

```
$ grep is_action_pressed calls   →  action_attack, toggle_debug, ui_accept, ui_down, ui_left, ui_right, ui_up
$ grep key_mappings[...] in Input::Input()  →  ui_accept, ui_down, ui_left, ui_right, ui_up only
```

* `PlayerController::_physics_process` checks `is_action_pressed("action_attack")` — never bound, dead branch (only `ui_accept`/Space actually triggers the hurt sound; the comment and variable naming imply an attack action was intended to be independent).
* `DebugOverlay::_process` checks `is_action_pressed("toggle_debug")`, with a comment saying "Toggle debug overlay visibility with F3 key" — F3 is never mapped to `toggle_debug` (or to anything) in `Input::Input()`.

There is also no public API to add or remap actions at all (`key_mappings` is populated once, hardcoded, in the `Input` constructor, with no `bind_action`/config-file-driven equivalent) — so this isn't a one-off oversight, it reflects an input system that a game project genuinely cannot extend without editing engine source.

### 1.6 The binary scene loader silently desyncs on oversized strings instead of failing loudly

`src/scene/main/scene_loader.cpp::read_string_binary()`:

```cpp
static std::string read_string_binary(std::ifstream& in) {
    uint32_t len = 0;
    in.read(reinterpret_cast<char*>(&len), sizeof(len));
    if (len == 0 || len > 65536) return "";
    std::string str(len, '\0');
    in.read(&str[0], len);
    return str;
}
```

If a length-prefixed field is larger than 64KB, the function returns an empty string **without consuming the bytes it just declined to read**. Every subsequent field in the `.rnb` file is then read from the wrong offset, silently corrupting the rest of the tree with no error message (contrast with the JSON loading path, which at least catches and logs `JSON Parse Error`). The 64KB-per-field limit is plausible to hit today: the `properties` field embeds an entire JSON blob per node, and a `TileMapLayer`'s `tile_data`/`collision_data` arrays are serialized into that JSON — a moderately large hand-authored map can exceed 64KB in that one field. `tools/map_compiler.py` enforces no such cap on the write side, so nothing prevents a project from producing a file that its own engine can't safely read back.

---

## 2. Unprofessional / Repo-Hygiene Choices

### 2.1 Compiled binaries are committed to git, in direct violation of the repo's own `.gitignore`

`.gitignore` explicitly lists `*.dll`, `*.lib`, `*.exp`, `*.pdb`, and `MyRPG/bin/`. Yet `git ls-files` shows all four tracked and force-added:

```
MyRPG/bin/Debug/game.dll
MyRPG/bin/Debug/game.exp
MyRPG/bin/Debug/game.lib
MyRPG/bin/Debug/game.pdb
```

Worse, they're rebuilt and re-committed on nearly every commit (`git log --stat` shows the `.dll` changing size in commit after commit), permanently bloating repo history with binary diffs that the project's own ignore rules say shouldn't be there.

### 2.2 `project.rnode` is a decorative file — none of its fields are actually read

The README lists `project.rnode` as "Game configuration file," and it contains real-looking settings:

```json
{
  "name": "MyRPG",
  "main_scene": "res://scenes/overworld.json",
  "display": { "window_width": 1024, "window_height": 896,
               "virtual_width": 256, "virtual_height": 224, "target_fps": 60 }
}
```

`main.cpp` only ever checks whether the file *exists*, as a marker to help guess the project directory. `main_scene`, the window dimensions, the virtual resolution, and `target_fps` are all hardcoded separately in `main.cpp` (`WINDOW_WIDTH = 1024`, `VisualServer::get()->init(renderer, 256, 224)`, `FIXED_DT = 1.0f/60.0f`). They currently match the JSON by coincidence; editing the JSON changes nothing. A config file that looks authoritative but is entirely inert is a trap for anyone building on this engine.

### 2.3 Two lengthy self-authored analysis documents in the repo root are already stale

`ARCHITECTURAL_ISSUES.md` (1,010 lines) and `PERFORMANCE_ANALYSIS.md` (1,124 lines) both carry today's date and describe serious problems — e.g. static-linking causing duplicate singletons, and a busy-wait main loop burning 100% CPU. Both are already contradicted by the current code: `retronode_core` is built `SHARED`, not `STATIC` (root `CMakeLists.txt` line 64), and `main.cpp` explicitly calls `SDL_SetRenderVSync(renderer, 1)` with a comment noting it exists specifically "to cap framerate and eliminate 100% CPU busy-wait spin." Keeping large, authoritative-sounding audits in the repo root that no longer describe the code they're auditing is confusing for anyone landing on the project and taking them at face value — and it means real, still-open issues (like the ones in this document) are easy to miss under the noise of stale ones.

### 2.4 `StringName`'s doc-comment overpromises what the class does

```cpp
/**
 * @brief High-performance interned string identifier for ClassDB, Scene Tree, and Input actions.
 */
class StringName {
    std::string name;
    size_t hash_value;
    StringName(const char* str) : name(str ? str : ""), hash_value(std::hash<std::string>{}(name)) {}
    ...
```

This isn't interned at all — there's no dedup table, no canonical instance, nothing shared. It's a plain `std::string` plus a hash computed fresh on every construction, and equality still does a full string compare. That wouldn't be worth flagging on its own, except the class is then constructed fresh, every frame, inside the game's hottest path:

```cpp
// PlayerController::_physics_process, called every physics tick
if (Input::get()->is_action_pressed(StringName("ui_right"))) ...
if (Input::get()->is_action_pressed(StringName("ui_left"))) ...
if (Input::get()->is_action_pressed(StringName("ui_down"))) ...
if (Input::get()->is_action_pressed(StringName("ui_up"))) ...
```

Four `std::string` allocations and four `std::hash` computations from string literals, every physics step, in the class whose entire stated purpose is to avoid exactly that cost. The doc-comment describes the class the author presumably meant to write, not the one that's there.

### 2.5 `Fixed16`'s integer constructor is a raw-value footgun

```cpp
constexpr explicit Fixed16(int32_t r) noexcept : raw(r) {}
```

This takes the **raw** Q16.16 bit pattern, not a logical integer — `Fixed16(10)` doesn't mean "10", it means `10/65536 ≈ 0.00015`. Nothing about the constructor's name or signature signals this (compare to the correctly-named `from_int()`/`from_float()` factory functions that exist right next to it). Nobody trips over it inside the current small demo, but this engine's whole pitch is being a foundation other people build games on top of — and the very first thing a new contributor will likely do is write `Fixed16 gravity(30)` expecting 30, and get a number 200,000 times too small, with no compiler warning and no runtime check.

### 2.6 The "deterministic fixed-point physics" doesn't stay fixed-point where it matters

The README's headline feature is "Deterministic Q16.16 Fixed-Point Math... for cross-platform physics determinism." But the actual collision broadphase converts positions to `float` and does floating-point division to compute spatial-hash cells:

```cpp
// src/servers/physics_server.cpp
static inline int to_cell(float coord, int cell_sz) {
    return static_cast<int>(std::floor(coord / static_cast<float>(cell_sz)));
}
```

`Fixed16` already supports integer division and comparisons natively; routing the actual simulation step through `float` conversions undercuts the one guarantee the fixed-point system exists to provide.

### 2.7 Two competing, redundant class-registration mechanisms are used for the same class

`PlayerController` is registered twice, by two different means, for no apparent reason:

1. `RN_REGISTER_CLASS(PlayerController);` at the bottom of `player_controller.cpp` — a static object whose constructor runs automatically the moment the DLL is loaded (this is how every *engine* class gets registered, in `register_engine_classes()`).
2. `MyRPG/src/module_init.cpp`'s exported `retronode_register_types()`, called explicitly by `GameModuleLoader::load_module()`, which does the exact same `ClassDB::get()->register_class(...)` call again.

Since (1) alone is sufficient — static initializers already run at `dlopen`/`LoadLibrary` time — (2) is vestigial. Having both live side-by-side, doing the same thing through different mechanisms, with no comment explaining why, is the kind of thing that leaves future contributors unsure which one actually matters.

### 2.8 Dead code: `PhysicsServer2D::clear()` exists but is never called

`TileMapLayer::_ready()` registers static collision boxes via `add_static_box()`. `clear()` is the obvious counterpart for undoing that on scene teardown — but it's never invoked anywhere in the codebase. Currently harmless only because scenes never actually get reloaded (see §1.2); the moment hot-reload's missing scene-reload step gets fixed, every reload would re-register the same tiles on top of the old ones, since nothing ever clears the spatial grid.

### 2.9 `CanvasLayer` is a functionally empty stub presented as an architectural feature

```cpp
CanvasLayer::CanvasLayer() { name = "CanvasLayer"; }
void CanvasLayer::_process(float delta) { Node::_process(delta); }
```

It's registered in `register_engine_classes()` and given a `visible` field, implying it plays the same role as Godot's `CanvasLayer` (an independent UI render layer, immune to camera movement). It does none of that — screen-fixed UI positioning is actually achieved elsewhere, by `Label`/`NinePatchRect` manually adding the camera offset back in. `CanvasLayer` itself currently does nothing a plain `Node` wouldn't.

---

## 3. Lower-Confidence / Not-Yet-Triggered Risks

These didn't manifest in the current `MyRPG` demo (which is simple enough to avoid them), but are worth flagging for anyone extending the engine:

* **Scene-tree traversal isn't reentrant-safe.** `Node::propagate_process()`/`propagate_physics_process()` iterate `children` with a plain range-`for` while calling into arbitrary user code. If any `_process`/`_physics_process` override ever adds or removes a *sibling* node (spawning an enemy, a bullet dying, etc. — normal in basically any real game), the vector can reallocate or erase mid-iteration, which is undefined behavior. `queue_free()` + the deferred `cleanup_queued_nodes()` pass covers deletion safely, but direct `add_child`/`remove_child` calls from inside a process callback are not guarded at all.
* **`get_node<T>("name")` doesn't support paths.** Despite the Godot-inspired naming (`get_node("Path/To/Node")` in Godot resolves a hierarchical path), this implementation only checks direct children by exact name match.
* **`TileMapLayer` submits one draw call per tile, per frame, unconditionally** — no static batching for a layer that (in this demo) never moves, and no camera-based culling of off-screen tiles. Fine for a 16×14 demo map; won't scale to a larger one.

---

## Summary

The two most damaging issues — **hot-reload silently wiping the game** and **the game module being undiscoverable off Windows** — both directly contradict the two features the README leads with, and both are trivial to reproduce (I did, above). The rest is a fairly consistent pattern: features that are half-wired (an input action referenced but never bound, a `visible` flag nobody checks, a config file nobody reads, a `clear()` nobody calls) rather than outright missing, plus a couple of resource-management bugs (the audio stream leak, the desyncing binary reader) that are easy to miss without either running the code or reading it end-to-end.
