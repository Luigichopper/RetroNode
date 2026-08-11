# RetroNode — Independent Code Audit

Fresh, from-source review of [Luigichopper/RetroNode](https://github.com/Luigichopper/RetroNode) (current `main`). This document does **not** rely on or reference any prior analysis files already in the repo (`CODE_REVIEW.md`, `PERFORMANCE_ANALYSIS.md`, `ARCHITECTURAL_ISSUES.md`) — everything below was found by re-reading the source directly. File paths and line numbers refer to the state of the repo at clone time.

Severity key: 🔴 Critical (crash / corruption / silently wrong behavior) · 🟠 Performance · 🟡 Unprofessional / architecture smell

---

## 1. Critical bugs

### 1.1 🔴 Use-after-free: `PhysicsServer2D::active_bodies` is never cleaned up
`CharacterBody2D::move_and_slide()` (`src/scene/physics/character_body_2d.cpp:53`) calls `register_active_body(id, this, bounds)` every physics tick, storing a raw `Node2D*` in `PhysicsServer2D::active_bodies`. `unregister_active_body()` exists (`src/servers/physics_server.cpp:44`) but **is never called anywhere in the codebase** — not from `Node`/`CharacterBody2D` destructors, not from `SceneTree::cleanup_queued_nodes`. The moment any `CharacterBody2D` is deleted (enemy dies, `queue_free()`, scene reload) its stale pointer remains in the map. The next `Area2D::_physics_process()` tick calls `get_overlapping_bodies_for_box()`, which iterates `active_bodies` and hands the dangling `Node2D*` straight back to gameplay code. This is a guaranteed crash/corruption path in any scene that combines a killable/despawning `CharacterBody2D` with any `Area2D` trigger — i.e. normal gameplay.

### 1.2 🔴 Memory leak: `CPUParticles2D::color_ramp` / `scale_amount_curve`
`Gradient*`/`Curve*` are heap-allocated with raw `new` in `src/scene/main/scene_loader.cpp:351,367` and assigned to `CPUParticles2D` member pointers (`src/scene/2d/cpu_particles_2d.h:102-103`). `CPUParticles2D` declares `virtual ~CPUParticles2D() = default;` — nothing ever deletes these. `Gradient`/`Curve` are plain `Object`s, not `Node`s, so the scene tree's recursive child-deletion in `Node::~Node()` doesn't reach them either. Every particle effect with a color ramp or scale curve leaks on node deletion/scene reload.

### 1.3 🔴 `Node2D::get_global_position()` ignores parent rotation and scale
`src/scene/2d/node_2d.cpp:51-62` computes global position by summing every ancestor's *local* `position`, with no rotation or scale applied to child offsets. `get_global_scale()` correctly multiplies and `get_global_rotation()` correctly sums (2D rotation is additive), but position must be transformed through each ancestor's accumulated rotation/scale — it currently isn't. Any node under a rotated or scaled parent (rotating platform, scaled camera rig, parented sprite on a spinning object) will report and render at the wrong world position. `get_global_previous_position()` has the identical bug, so interpolated rendering is wrong in the same cases.

### 1.4 🔴 `has_script()` false positive for built-in physics nodes
`Node::has_script()` (`src/scene/main/node.h:47-57`) decides whether a node has a user script by string-comparing its class name against a hardcoded exclusion list of "known engine classes". `Area2D`, `StaticBody2D`, and `CollisionShape2D` are all registered engine classes (`RN_REGISTER_CLASS` in their respective `.cpp` files) but are **missing from the exclusion list**. Result: the Inspector panel (`inspector_panel.cpp:32`) shows a spurious "Script Attached: Area2D" (etc.) for every plain, script-less physics node — actively misleading during editing.

### 1.5 🔴 Inspector property edits are not undoable
`EditorState::push_undo_snapshot()` is called from viewport dragging, scene-tree reparenting/deletion, and the main menu (`viewport_panel.cpp:211`, `scene_tree_panel.cpp:55/70/95/176`, `main_menu_bar.cpp:35`) — but **never** from `inspector_panel.cpp`. Editing any property value in the Inspector (position, color, texture, emission settings, etc.) silently bypasses the undo system entirely; Ctrl+Z will not revert it. This is exactly the gap the dead `UndoRedo::add_do_property/add_undo_property` API (see §3.2) looks like it was built to close, but it was never wired up.

### 1.6 🔴 Dangling raw pointers in `UndoRedo` (if it's ever used)
`UndoRedo::add_do_property`/`add_undo_property` (`src/core/undo_redo.cpp:12-24`) capture a raw `Object* p_target` by value inside a `std::function` stored indefinitely in undo/redo history, with no liveness check before invoking `p_target->set(...)` in `undo()`/`redo()`. `clear_history()` exists but is never called (not even on new-scene or node deletion). Currently this class isn't wired into the editor (§3.2), so the bug is latent — but it's a use-after-free waiting to happen the moment someone connects it, and it's evidence the pattern from §1.1 repeats wherever this codebase caches raw object pointers.

### 1.7 🔴 Particle scale is re-randomized every physics tick, not once at spawn
In `CPUParticles2D::_physics_process` (`src/scene/2d/cpu_particles_2d.cpp:291-296`), `current_scale` is drawn from `randf_range(scale_amount_min, scale_amount_max)` **inside the per-frame update loop**, then multiplied by the scale curve. `spawn_particle()` already sets a per-particle `p.scale` once at spawn (`cpu_particles_2d.cpp:217-218`), which this code silently discards every tick. Net effect: every active particle's size re-rolls randomly 60 times a second instead of following a smooth per-particle curve — visibly flickering/pulsing particles instead of the intended animated scale-over-lifetime.

### 1.8 🔴 Particle recycling always evicts slot 0, not the oldest particle
When the particle pool is full, `CPUParticles2D::_physics_process` calls `spawn_particle(particles[0])` (`cpu_particles_2d.cpp:248`), with a comment claiming this "recycles the oldest particle". It doesn't — it unconditionally clobbers index 0 regardless of that particle's actual remaining lifetime, which can kill a freshly-spawned particle while a genuinely expiring one elsewhere in the array survives.

### 1.9 🔴 Unbounded catch-up loop in particle spawning
The `while (time_accumulator.to_float() >= spawn_interval)` loop (`cpu_particles_2d.cpp:239-250`) has no cap on iterations. After any large frame hitch (debugger pause, asset load stall, alt-tab) combined with a high `amount`/low `lifetime` configuration (small `spawn_interval`), this can spin for a very large number of iterations in one frame, adding to the stall instead of recovering from it.

### 1.10 🔴 Tile static-body IDs can collide with real object instance IDs
`TileMapLayer::_ready()` registers each solid tile with `PhysicsServer2D::add_static_box(get_instance_id() + idx, bounds)` (`src/scene/2d/tile_map_layer.cpp:125`). `get_instance_id()` comes from a single global monotonically-increasing counter shared by *every* `Object` in the engine (`src/core/object/object.cpp:9`). Adding an arbitrary tile index to it produces an ID with no guarantee of uniqueness against other live objects' real instance IDs. `KinematicCollision2D::collided_body_id` (`physics_server.cpp:115,137`) is a public field seemingly intended for gameplay code to resolve "what did I hit" — any future use of `ObjectDB::get_object(collided_body_id)` against a tile collision can silently resolve to an unrelated live object.

### 1.11 🟠🔴 `move_and_slide` resolves against the first colliding tile found, not the closest
Both the X and Y collision passes in `PhysicsServer2D::move_and_slide` (`physics_server.cpp:110-126`, `133-152`) `break` on the first entry in `get_nearby_body_indices()` that intersects, rather than picking the nearest/most relevant collider. Since that list's order depends on spatial-grid cell iteration order rather than distance, movement resolution at multi-tile boundaries (corners, tunnels) can pick an arbitrary one of several valid colliders, producing inconsistent snapping/tunneling-adjacent behavior.

### 1.12 🔴 `set_position()` silently defeats render interpolation
`Node2D::set_position()` (`node_2d.h:31-34`) and `Control::set_position()` (`control.h:29-32`) both set `previous_position = pos` on every call. `previous_position` is what `VisualServer::render_scene()` interpolates *from* to smooth motion between fixed physics steps (`visual_server.cpp:200-203`). Any code path that moves a node via `set_position()` — the property-system setter used by the Inspector and by `duplicate()`-placement (`editor_state.cpp:230`) — collapses `previous_position` to the current position, so those nodes never interpolate. Meanwhile `CharacterBody2D::move_and_slide()` writes `position` directly as a bare field (`character_body_2d.cpp:46`), bypassing the setter and *not* touching `previous_position` — so the two "normal" ways of moving a node have silently different interpolation behavior, with no comment anywhere explaining why.

### 1.13 🔴 Camera is not interpolated, but the world it films is
`Camera2D::_process()` (`camera_2d.cpp:11-36`) sets `VisualServer`'s camera offset from the camera's raw, current `get_global_position()` every rendered frame. World sprites, by contrast, are drawn from `previous_position` blended toward `position` by `alpha` (§1.12's mechanism). A camera that snaps to the true fixed-step position every frame while everything it's filming is smoothly interpolated will visibly judder relative to its own tracked target (e.g. following the player) — a classic symptom of interpolating only part of a fixed-timestep pipeline.

### 1.14 🔴 `AnimationPlayer` target resolution is order-dependent and un-refreshed
`AnimationPlayer::_ready()` (`animation_player.cpp:18-32`) looks for a sibling `Sprite2D` **only among children that already exist on the parent at that moment**. If the `Sprite2D` is added to the parent after the `AnimationPlayer`, `target_sprite` stays `nullptr` forever with no re-resolution or error. It also caches a raw `Sprite2D*` with no lifetime tracking, the same pattern as §1.1/§1.6.

### 1.15 🟡🔴 Animation playback runs on variable-rate `_process`, breaking the engine's own determinism goal
`AnimationPlayer::_process(float delta)` (`animation_player.cpp:58-95`) advances `frame_timer` using the variable, floating-point `_process` delta rather than the fixed `Fixed16` 60 Hz `_physics_process` step the rest of the engine is built around (per the repo's own stated goal of "deterministic physics" via a fixed timestep accumulator). Animation state will differ run-to-run and machine-to-machine depending on frame pacing, undermining anything that depends on deterministic playback (replays, frame-linked hitboxes/hurtboxes).

---

## 2. Performance issues

### 2.1 🟠 `Area2D` overlap checks don't use the spatial grid at all
`PhysicsServer2D::get_overlapping_bodies_for_box()` (`physics_server.cpp:85-95`) does a flat linear scan over **every** entry in `active_bodies` for every collision shape of every `Area2D`, every physics tick (`area_2d.cpp:60-71`). Static-body collision in the same file correctly uses `spatial_grid` for O(cell) lookups — Area2D overlap detection was simply never routed through it, making trigger-heavy scenes scale as O(areas × shapes × active bodies) per tick instead of O(areas × nearby bodies).

### 2.2 🟠 `TileMapLayer` walks every cell in the map every frame
`TileMapLayer::_process()` (`tile_map_layer.cpp:162-205`) loops `rows × columns` unconditionally, computing world coordinates for each cell before checking it against camera bounds and `continue`-ing if off-screen. The visible column/row range could be derived directly from the camera rect and looped over exclusively, turning an O(map size) per-frame cost into O(visible tiles). This gets worse as maps grow, independent of how much is actually on screen. Related: `_ready()` (`tile_map_layer.cpp:109-129`) registers **one static collider per solid tile** instead of merging adjacent solid tiles into larger rectangles, which multiplies the number of entries the spatial grid (and every query against it) has to carry for large solid regions like floors and walls.

### 2.3 🟠 `StringName` construction is far from the "O(1)" the class advertises
`StringName`'s comparison is genuinely O(1) (interned pointer compare), but *construction* of a non-cached `StringName` (`string_names.cpp:11-18`) takes a `std::mutex` lock, does an `unordered_set` insert/lookup, and then the constructor computes `std::hash<std::string>` **again** on top of that (`string_names.h:33-37`) — the hash is effectively computed twice per construction. Several `get_property_list()` overrides build these inline, uncached, every single call instead of caching them as `static const StringName` the way the sibling `get()`/`set()` methods in the *same files* correctly do (compare `cpu_particles_2d.cpp`'s `get()`/`set()`, which cache 18 `static const StringName`s, against its `get_property_list()`, which builds all 18 fresh every call). `InspectorPanel::draw()` — an ImGui panel redrawn every rendered frame while any node is selected — calls `target->get_property_list()` every frame (`inspector_panel.cpp:44`) and additionally constructs a brand-new throwaway `StringName("name")` on every property in the loop just to skip it (`inspector_panel.cpp:47`). This turns "compare a name" into a mutex-guarded hash-set operation, every property, every frame, while the editor is open.

### 2.4 🟠 Mutex-guarded interning in a codebase with no threads
`grep`ing the entire source tree turns up zero uses of `std::thread`, `std::async`, or `pthread` anywhere — the engine is single-threaded end to end. The `std::mutex` in `StringName::intern()` (`string_names.cpp:8,15`) is therefore pure, unconditional overhead on every non-cached `StringName` construction (see §2.3) for a hazard that can't occur.

### 2.5 🟠 Full child-vector copy on every propagate call, for every node
`Node::propagate_ready/propagate_physics_process/propagate_process` (`node.cpp:97-125`) each do `auto children_copy = children;` before recursing — a heap-allocating copy of the children vector, at every node in the tree, on every physics tick and every rendered frame. This guards against mutation during iteration, which is a reasonable goal, but a copy-on-every-call is a more expensive way to achieve it than e.g. deferring structural changes to the existing `cleanup_queued_nodes` pass.

### 2.6 🟠 Editor undo re-serializes the entire scene to JSON on every undoable action
`EditorState::push_undo_snapshot()` (`editor_state.cpp:54-...`) calls `SceneLoader::serialize_node_to_json_string(root)` — a full recursive JSON dump of the *entire* scene tree — and pushes the resulting string onto `undo_stack` for every reparent/delete/drag action. This is the actual, wired-up undo system (see §3.2 for the unused alternative that exists alongside it), so its cost scales with total scene size on every action, not with the size of the edit.

### 2.7 🟠 Draw order isn't stable, and there's no draw batching
`VisualServer::render_scene()`/`render_editor_scene()` sort the whole render queue with `std::sort` by `z_index` (`visual_server.cpp:135,184`). `std::sort` has no ordering guarantee for equal keys, and most sprites share a default `z_index`, so same-layer draw order can vary frame to frame as the queue's contents change (particles spawning/despawning, etc.) — a plain swap to `std::stable_sort` would fix visible order flicker for co-planar sprites at effectively no cost. Separately, draws are issued one texture bind/mod call at a time in sorted order with no grouping by texture, so interleaved sprites from different textures pay repeated state-change overhead that batching-by-texture would avoid.

### 2.8 🟠 Texture loading probes the filesystem with hardcoded candidate paths, and never evicts
`TextureServer::load_texture()` builds a list of up to 6 candidate paths (including `"./MyRPG/" + resolved_path"`, `"../MyRPG/..."`, `"../../MyRPG/..."`) and calls `fs::exists()` against each until one hits (`texture_server.cpp:59-74`). There is no `unload_texture`, no reference counting, and no eviction — every texture ever loaded (through any path string, including two different path strings that happen to resolve to the same file) stays resident for the process lifetime.

### 2.9 🟡🟠 No optimization or warning flags anywhere in the build
`CMakeLists.txt` sets `CMAKE_CXX_STANDARD` but never sets a default `CMAKE_BUILD_TYPE`, never adds `-O2`/`-O3`, and never enables `-Wall -Wextra` (or MSVC equivalents). On common single-config generators (Makefiles/Ninja without an explicit `-DCMAKE_BUILD_TYPE=Release`), this engine builds fully unoptimized by default — a significant, invisible tax on top of every algorithmic issue listed above. Both `retronode_core` and `retronode`'s output directories are hardcoded to `.../Debug` (`CMakeLists.txt:165-167,190`) regardless of the build type actually selected, which is also just misleading.

---

## 3. Unprofessional techniques / architecture smells

### 3.1 🟡 Two independent texture-loading/caching subsystems
`ResourceManager` (`src/core/resource_manager.h/.cpp`) is a complete, separate texture loader/cache — `stbi_load` → `SDL_Texture` → `unordered_map` cache — that duplicates everything `TextureServer` already does. It's compiled into every build (`CMakeLists.txt:86`) but **grepping the whole `src/` tree shows nothing outside its own two files ever references `ResourceManager`.** It's dead code that doubles the maintenance surface for texture loading and would double-load/double-cache the same asset under two different keying schemes if anyone ever did wire it up.

### 3.2 🟡 Two independent undo/redo systems, and the good one is unused
`UndoRedo` (`src/core/undo_redo.h/.cpp`) is a proper command-pattern undo stack with `add_do_property`/`add_undo_property`/`add_do_method`/`add_undo_method`. Nothing outside `undo_redo.cpp` itself includes `undo_redo.h` or references `UndoRedo` anywhere in the repository — it's dead code, compiled for nothing. The editor instead uses `EditorState`'s own from-scratch, full-tree-JSON-snapshot undo system (§2.6), which is heavier per action and — per §1.5 — isn't even hooked up to the Inspector, the one place `UndoRedo::add_do_property` looks purpose-built for.

### 3.3 🟡 Two `variant.h` headers, included inconsistently
`src/core/variant.h` is a 6-line forwarding header whose only content is `#include "object/variant.h"`, which is where `Variant` is actually defined. Different files reach the same class through different paths — `object.h` includes the real one directly (`"variant.h"` relative to `core/object/`), while `class_db.h` goes through the forwarding header (`"../variant.h"`). Not a functional bug (include guards prevent redefinition), but it's needless indirection that makes "where is `Variant` actually defined" a two-hop question for no benefit.

### 3.4 🟡 Reusable engine code hardcodes the one example game's name
`TextureServer::load_texture()`'s candidate-path list bakes in `"./MyRPG/"`, `"../MyRPG/"`, `"../../MyRPG/"` literally (`texture_server.cpp:62-64`) — inside the *engine's* core module, not the example project. The root `CMakeLists.txt`'s `compile_game_assets` custom target (`CMakeLists.txt:173-179`) is even more specific: it unconditionally invokes the asset-compiler tools against `MyRPG/scenes/player.json` and `MyRPG/scenes/overworld.json` by name, as part of the default `ALL` target, with no existence guard (unlike the `add_subdirectory(MyRPG)` call a few lines later, which *is* guarded by `if (EXISTS .../MyRPG/CMakeLists.txt)`). Anyone using RetroNode as a template for a different game — the entire point of it being an engine — gets a build that references a project that isn't theirs.

### 3.5 🟡 `has_script()` reimplements "is this a built-in class" via a hand-maintained string blocklist
`node.h:47-57` compares the class name string against ~17 hardcoded literals every call, instead of e.g. a bit set on class registration in `ClassDB`. Every new built-in node type requires remembering to add it here, and forgetting produces exactly the false-positive bug shown in §1.4 — this isn't hypothetical, it's already happened three times.

### 3.6 🟡 Nine near-identical hand-rolled singletons, none of them cleaned up
`ClassDB`, `ObjectDB`, `SceneTree`, `PhysicsServer2D`, `VisualServer`, `AudioServer`, `TextureServer`, `Input`, `EditorState`, and `EditorMain` all repeat the same `static X* instance; static X* get() { if (!instance) instance = new X(); return instance; }` pattern by hand, and none of them are ever deleted — every singleton in the engine leaks at process exit (harmless at exit, but it means none of these classes' destructors are exercised in normal operation, including `VisualServer`'s and `TextureServer`'s, which hold real GPU resources). A single shared singleton template would remove the duplication and give one place to add proper teardown.

### 3.7 🟡 `render_scene()` and `render_editor_scene()` are ~90% duplicated and have already drifted
`VisualServer::render_scene()` (`visual_server.cpp:177-241`) and `render_editor_scene()` (`visual_server.cpp:125-175`) are near-identical copy-pasted draw loops. They've already diverged: the editor version ignores its `alpha` parameter entirely (`(void)alpha;`, line 126) and skips interpolation altogether, while the gameplay version implements it. Any future fix to one (e.g. the `std::sort` stability issue in §2.7) has to be remembered and reapplied to the other by hand.

### 3.8 🟡 Confusing/duplicated tools layout
There are two `tools`-named locations: the real, build-invoked scripts live at top-level `/tools/map_compiler.py` and `/tools/texture_packer.py` (referenced by `CMakeLists.txt`), while `/src/tools/map_compiler/` and `/src/tools/texture_packer/` exist as empty directories containing only a `.gitkeep` file each. Nothing in the build or source references the `src/tools` versions — they read as either abandoned scaffolding or a layout the project meant to migrate to and didn't, either way it's misleading to anyone looking for "where do the tools live."

---

## Summary

| # | Severity | Issue | Where |
|---|---|---|---|
|1.1| 🔴 Critical | Dangling `Node2D*` in `PhysicsServer2D::active_bodies`, never unregistered | physics_server.cpp, character_body_2d.cpp |
|1.2| 🔴 Critical | `Gradient`/`Curve` leaked by every `CPUParticles2D` | cpu_particles_2d.h, scene_loader.cpp |
|1.3| 🔴 Critical | `get_global_position()` ignores parent rotation/scale | node_2d.cpp |
|1.4| 🔴 Critical | `has_script()` false-positives on Area2D/StaticBody2D/CollisionShape2D | node.h |
|1.5| 🔴 Critical | Inspector edits bypass undo entirely | inspector_panel.cpp |
|1.6| 🔴 Critical | `UndoRedo` stores raw `Object*` with no liveness check | undo_redo.cpp |
|1.7| 🔴 Critical | Particle scale re-randomized every tick instead of set once | cpu_particles_2d.cpp |
|1.8| 🔴 Critical | Particle recycling always evicts slot 0, not oldest | cpu_particles_2d.cpp |
|1.9| 🔴 Critical | Unbounded particle spawn catch-up loop | cpu_particles_2d.cpp |
|1.10| 🔴 Critical | Tile static-body IDs can collide with real instance IDs | tile_map_layer.cpp |
|1.11| 🔴 Critical | Collision resolves against first hit, not closest | physics_server.cpp |
|1.12| 🔴 Critical | `set_position()` silently defeats render interpolation | node_2d.h, control.h |
|1.13| 🔴 Critical | Camera not interpolated while world is | camera_2d.cpp |
|1.14| 🔴 Critical | `AnimationPlayer` sibling lookup is order-dependent, never refreshed | animation_player.cpp |
|1.15| 🔴 Critical | Animation timing breaks the engine's own determinism model | animation_player.cpp |
|2.1| 🟠 Perf | Area2D overlap scans all bodies instead of the spatial grid | physics_server.cpp, area_2d.cpp |
|2.2| 🟠 Perf | TileMapLayer iterates the whole grid every frame; one collider per tile | tile_map_layer.cpp |
|2.3| 🟠 Perf | Uncached `StringName` construction in hot per-frame paths | cpu_particles_2d.cpp, inspector_panel.cpp |
|2.4| 🟠 Perf | Mutex-guarded interning in a single-threaded engine | string_names.cpp |
|2.5| 🟠 Perf | Full child-vector copy on every propagate call | node.cpp |
|2.6| 🟠 Perf | Undo snapshots serialize the whole scene to JSON per action | editor_state.cpp |
|2.7| 🟠 Perf | Non-stable sort + no draw batching | visual_server.cpp |
|2.8| 🟠 Perf | Hardcoded path probing + unbounded texture cache | texture_server.cpp |
|2.9| 🟡🟠 | No optimization/warning flags in the build | CMakeLists.txt |
|3.1| 🟡 Smell | Dead, duplicate `ResourceManager` texture system | resource_manager.cpp |
|3.2| 🟡 Smell | Dead, duplicate `UndoRedo` system alongside the real one | undo_redo.cpp, editor_state.cpp |
|3.3| 🟡 Smell | Two `variant.h` headers, included inconsistently | core/variant.h, core/object/variant.h |
|3.4| 🟡 Smell | Engine core hardcodes the "MyRPG" example project's name | texture_server.cpp, CMakeLists.txt |
|3.5| 🟡 Smell | `has_script()` via hand-maintained string blocklist | node.h |
|3.6| 🟡 Smell | Nine duplicated leaky singletons | (engine-wide) |
|3.7| 🟡 Smell | Near-duplicate, already-diverged render paths | visual_server.cpp |
|3.8| 🟡 Smell | Confusing duplicate `tools/` layout | tools/, src/tools/ |
