# RetroNode Engineering Guidelines

This document is the single source of truth for how RetroNode is organized, named, and extended. It's written from the actual state of the codebase (not aspirational design docs) so that following it keeps new code consistent with what's already here. Where the codebase currently violates one of these rules, that's called out explicitly as a known gap to fix, not a pattern to copy.

---

## 1. What RetroNode is

RetroNode is a **game-agnostic 2D retro engine** — a reusable foundation for building era-accurate pixel-art games (NES/SNES-caliber presentation: fixed virtual resolution, limited simultaneous effects, tile- and sprite-based rendering), not an engine built around any one game. `MyRPG/` is a *test case* that exercises the engine, in the same way a game studio's first title exercises its in-house engine — it is not, and must never become, part of the engine's identity.

Four pillars every change should be checked against:

1. **Determinism.** Gameplay-affecting state (position, physics, anything that must reproduce identically across machines) is advanced on the fixed 60 Hz `_physics_process` step using `Fixed16` (Q16.16) fixed-point math — never `float`/`double` and never the variable-rate `_process` step. Rendering is the one place variable-rate interpolation (`previous_position` → `position` blended by `alpha`) is allowed, because it only affects what's drawn, not what's simulated.
2. **Zero-overhead by default.** No hidden per-frame heap allocations, no locking in code paths that don't need it (the engine is single-threaded end to end — there is currently no `std::thread`/`std::async` use anywhere in `src/`, so synchronization primitives should not appear without a specific, stated reason), no O(n) scans over global state where a spatial/indexed lookup is available.
3. **Server-Node split.** User-facing scene graph classes (`Node`, `Node2D`, `Sprite2D`, `CharacterBody2D`, ...) hold *data and identity*. Backend singletons (`VisualServer`, `PhysicsServer2D`, `AudioServer`, `TextureServer`, `Input`) hold *systems and hardware state*. Nodes talk to servers through their `::get()` singleton accessor; servers never reach into node-tree internals directly — they operate on IDs/handles and the data nodes hand them.
4. **Engine/game separation.** Game code lives entirely outside `src/` and is compiled to a hot-reloadable shared library (`game.dll` / `libgame.so`) loaded by the engine executable. The engine never needs to be recompiled to change gameplay.

### Non-goals

Don't add: 3D rendering, a general-purpose scripting VM, networking, or anything whose primary justification is "modern engines have it." If a feature doesn't serve 2D retro-style games well within the four pillars above, it doesn't belong in `src/`.

---

## 2. The game-agnosticism rule

This is the rule most likely to get silently violated by well-intentioned code, so it gets its own section.

**Nothing under `src/` or in the root `CMakeLists.txt` may reference a specific game project by name, path, or asset layout.** No `"MyRPG"`, no `"MyRPG/scenes/player.json"`, no assumptions about a game's folder structure baked into engine code.

- If the engine needs to find assets, it resolves paths through the *current project's* configured search roots (`project.rnode` / the active project directory), not through hardcoded relative-path guesses.
- Build targets that compile or process a specific game's assets (asset packers, map compilers) belong in **that game's own `CMakeLists.txt`** (e.g. `MyRPG/CMakeLists.txt`), guarded by that project existing — never as an unconditional target in the root build script.
- If you're tempted to add a fallback path like `"../MyRPG/" + path` to make loading "just work" for the one game you're testing with, that's a sign the path-resolution system itself is missing a feature (a configurable search-root list) — fix that instead.

When reviewing a PR, "does this compile and run with `MyRPG/` deleted and some other game project dropped in instead?" is the test.

---

## 3. Repository layout

```
src/
├── core/       # Object model, reflection (ClassDB/ObjectDB), StringName, Variant,
│               # fixed-point math (Fixed16, Vector2Fixed, Rect2Fixed), resources
│               # (Gradient, Curve), serialization support. No SDL, no scene tree.
├── scene/      # User-facing Node hierarchy: main/ (Node, SceneTree, loader),
│               # 2d/, physics/, gui/, audio/, animation/. Depends on core/ and servers/.
├── servers/    # Backend singletons: VisualServer, PhysicsServer2D, AudioServer,
│               # TextureServer, Input. Depends on core/ only, never on scene/.
├── platform/   # SDL3 integration, main loop, entry point, game-module hot-reload.
├── editor/     # ImGui-based editor (Phase 10+). Depends on scene/ + servers/,
│               # compiled only when RN_BUILD_EDITOR is on.
└── tools/      # Asset pipeline scripts that are genuinely engine-generic.
```

Dependency direction is one-way: `platform → editor → scene → servers → core`. A file in `core/` including anything from `scene/` or `servers/` is a layering violation.

**Tooling scripts live in exactly one place.** The repo currently has both a top-level `tools/` (the scripts the build actually invokes) and empty placeholder directories under `src/tools/`. Pick one location — top-level `tools/` matches what the build references — and remove the other rather than letting both exist.

**One subsystem, one implementation.** Before adding a new manager/cache/loader, check whether an existing server or `core/` class already owns that responsibility. RetroNode should not accumulate a second texture cache or a second undo system alongside an existing one that already does the job — extend the existing one.

---

## 4. Naming conventions

Everything lives in the `RetroNode` namespace.

| Element | Convention | Examples |
|---|---|---|
| Classes / structs | `PascalCase` | `Node2D`, `CharacterBody2D`, `PhysicsServer2D`, `ClassDB` |
| Files | `snake_case`, matching the primary class, `.h`/`.cpp` pairs | `node_2d.h/.cpp`, `character_body_2d.h/.cpp` |
| Methods / free functions | `snake_case`, verb-first | `get_global_position()`, `queue_free()`, `register_active_box()` |
| Getters / setters | `get_x()` / `set_x()` — required for anything reflected via `ClassDB::register_property`, which binds exactly this pair | `get_position()` / `set_position()` |
| Boolean accessors | `is_x()` / `has_x()`, no bare-field boolean exposed as if it were the accessor | `is_visible()`, `has_script()`, `is_playing()` |
| Member variables | `snake_case`, no Hungarian prefixes (`m_`, `p_` on members, etc.) | `position`, `previous_position`, `active_bodies` |
| Macros | `RN_` prefix, `ALL_CAPS` | `RN_API`, `RN_CLASS`, `RN_REGISTER_CLASS`, `RN_BUILD_EDITOR` |
| Enums | `PascalCase` type name, `PascalCase` or `ALL_CAPS` values consistent within the enum | `VariantType::VECTOR2`, `PropertyHint::FILE_PATH` |
| Reflected property string names | `snake_case`, matches the C++ member name it mirrors | `"position"`, `"texture_path"`, `"one_shot"` |

A few things to actively avoid because they've crept in before:

- **Don't suffix booleans with `_flag`** (`is_playing_flag`). If the field needs to be distinguishable from its accessor, name the accessor `is_playing()` and let the field be a private implementation detail — don't expose the awkward name publicly.
- **Don't hand-roll a synthetic ID by adding an offset to `get_instance_id()`.** Instance IDs come from one global counter shared by every `Object` in the process; arithmetic on them can collide with a real object's ID. If a subsystem needs many sub-IDs per owning object (e.g. one per tile in a tilemap), give it its own ID space.

---

## 5. The object model and reflection (`ClassDB`)

Every scene-graph class:

1. Inherits from `Node` (directly or transitively) and declares `RN_CLASS(ThisClass, ParentClass)` in its header.
2. Is registered exactly once, in `src/platform/main.cpp`'s startup registration block, via `RN_REGISTER_CLASS(ThisClass)`.
3. Registers each editor-visible property immediately after, via `ClassDB::register_property("ThisClass", PropertyInfo{...}, &ThisClass::setter, &ThisClass::getter)`, in the same order the fields are declared in the header.
4. Overrides the three property virtuals — `get_property_list()`, `get()`, `set()` — and **calls the parent class's implementation first** before adding its own properties (see `Node2D::get_property_list()` for the pattern). This is what makes the Inspector see a full, correctly-inherited property list for any node.
5. Caches every `StringName` used for property-name comparison as a function-local `static const StringName`, never constructs one inline in a hot path (see §7).
6. Adds its class name to `Node::has_script()`'s exclusion list in `node.h` if it's a built-in engine class. **This list is currently incomplete** (`Area2D`, `StaticBody2D`, `CollisionShape2D` are missing, causing the Inspector to falsely report "Script Attached" for them) — check it every time you register a new built-in class, and prefer converting this to a `ClassDB`-driven flag over time rather than adding another manual entry.

### A known limitation to design around

`ClassDB`'s static registry (`ClassDB::get_property_list(class_name)`, used by `Node::duplicate()`) is keyed by the **exact** class name a property was registered under — it does not walk up to parent classes the way the virtual `get()`/`get_property_list()` chain does. In practice this means `Node::duplicate()` currently only copies properties registered directly under a node's own exact class name, not properties it inherited (e.g. a duplicated `Sprite2D` won't have its `position`/`rotation`/`scale` copied through this path, because those are registered under `"Node2D"`). Until `ClassDB`'s lookup walks the hierarchy, don't rely on `duplicate()` to carry inherited properties for new node types — either fix `ClassDB` (preferred, benefits everything) or re-register the property under the subclass name as a stopgap, and note in the PR which you did.

---

## 6. Ownership and lifetime rules

RetroNode uses raw pointers pervasively (no smart pointers in the scene graph), which makes explicit lifetime discipline mandatory:

- **The scene tree owns nodes.** A `Node*` is owned by its parent (`Node::~Node()` deletes all children recursively). Never `delete` a node directly if it's in the tree — call `queue_free()` and let `SceneTree::cleanup_queued_nodes()` handle it at the end of the frame.
- **Anything a node hands to a server by raw pointer must be unregistered on teardown.** If you add a `register_x(id, this, ...)` call to any server (following the pattern in `PhysicsServer2D::register_active_body`), you must add the matching `unregister_x(id)` call, and it must actually run — on `queue_free()`/destruction, not just exist as a dead function. (`PhysicsServer2D::active_bodies` currently violates this: bodies are registered every physics tick but never unregistered, so a deleted `CharacterBody2D` leaves a dangling pointer that `Area2D` overlap queries will dereference. Don't add a second instance of this bug — and if you're touching this code, fix it.)
- **`Object`-derived resources that aren't `Node`s (`Gradient`, `Curve`, textures, etc.) are not freed by the scene tree.** Whatever allocates one is responsible for freeing it — either give the owning node a real destructor that deletes it, or make the resource reference-counted/shared. A `= default` destructor next to a raw `new`'d member pointer is a leak; the compiler won't warn you, code review has to catch it.
- **Don't cache a raw pointer to another node across frames without a way to know if it's still valid.** If a node needs a long-lived reference to another node (`AnimationPlayer::target_sprite` is the existing example), re-resolve it defensively or check `ObjectDB::is_valid(instance_id)` before using it — don't assume a pointer captured once in `_ready()` is still good indefinitely.
- **Singletons are never destroyed, by design (`static X* instance; get() { if (!instance) instance = new X(); ... }`).** This is accepted for now since it only leaks at process exit, but it means a singleton's destructor is never exercised. Don't rely on `~VisualServer()`/`~TextureServer()`/etc. doing meaningful cleanup — if a singleton holds a hardware resource, it needs an explicit `shutdown()` called from `main.cpp`, not a destructor.

---

## 7. Performance rules

These exist because "zero-overhead" is a stated pillar (§1), not a suggestion:

- **Never construct a `StringName` from a string literal inside a function that runs every frame or every property access.** `StringName` construction takes a mutex and does an `unordered_set` lookup/insert; it is not free. Hoist it to a `static const StringName` at file or function scope, exactly like `Node2D::get()`/`set()` already do. `get_property_list()` overrides are a common place this gets missed — check them specifically.
- **Prefer the spatial grid over a linear scan for anything positional.** `PhysicsServer2D` already maintains a `spatial_grid` for static collision — new positional queries (overlap checks, proximity queries, area triggers) should be built on it, not on iterating every tracked body/node.
- **Cull before you iterate, not after.** For anything gridded (tilemaps, chunked worlds), compute the visible index range from the camera bounds first and loop only over that range — don't loop over the whole structure and skip entries that turn out to be off-screen.
- **Don't copy a container to iterate it safely; defer mutation instead.** If you need to guard a loop against structural changes mid-iteration (a node being freed while its siblings are processing), queue the removal and apply it after the loop, rather than copying the whole container up front on every call.
- **`std::sort` is not stable — use `std::stable_sort` for any ordering where equal keys are common** (z-index draw order being the standing example) so equal-key items don't visibly reshuffle frame to frame.
- **A `std::mutex` should only appear in code that is actually reachable from more than one thread.** RetroNode is single-threaded today; adding synchronization "to be safe" adds real cost for no benefit and signals (incorrectly) that the surrounding code is concurrency-sensitive. If you're introducing actual multithreading, say so explicitly in the PR and update this document.
- **Any unbounded `while` catch-up loop (fixed-timestep accumulators, spawn-rate accumulators) needs an iteration or time cap.** A large delta (hitch, breakpoint, alt-tab) should degrade gracefully, not turn into a bigger stall.

---

## 8. Adding a new Node type — checklist

1. Header in the right `scene/` subfolder (`2d/`, `physics/`, `gui/`, `audio/`, `animation/`), `RN_CLASS(NewType, ParentType)`.
2. Constructor sets `name = "NewType"`.
3. Override `get_property_list()` (call parent first, then push this class's own `PropertyInfo`s), `get()`, `set()` (chain to parent for unrecognized names) — using `static const StringName` for every name compared against.
4. Register in `main.cpp`: `RN_REGISTER_CLASS(NewType)` immediately followed by `ClassDB::register_property(...)` for each exposed property, matching the header's declaration order.
5. Add the class name to `Node::has_script()`'s exclusion list in `node.h` if it's an engine built-in (see §5.6).
6. If it registers anything with a server by raw pointer, add and wire up the matching unregister call (§6).
7. If it needs per-frame behavior, decide deliberately between `_physics_process` (fixed, deterministic, gameplay-affecting) and `_process` (variable-rate, presentation-only) — don't default to `_process` for something that affects simulated state.
8. Add it to `CMakeLists.txt`'s `ENGINE_CORE_SOURCES` list.

---

## 9. Adding a new server

1. Lives in `src/servers/`, depends only on `core/` (never on `scene/`).
2. Standard singleton accessor: `static X* instance; static X* get();`. If it owns a real hardware/OS resource, also add an explicit `shutdown()` and call it from `main.cpp`'s teardown — don't rely on the (never-run) destructor.
3. Operates on IDs and data handed to it by nodes, not on `Node*` internals — nodes call the server, the server doesn't reach into the tree.
4. Any per-tick/per-frame query the server exposes should be backed by an appropriate index (spatial grid, hash map by ID) from day one, not "linear scan for now, optimize later" — retrofitting this later means finding and fixing every call site that started depending on the slow behavior.

---

## 10. Editor (ImGui) conventions

- Editor code lives under `src/editor/`, compiles only when `RN_BUILD_EDITOR` is defined, and is the *only* place allowed to depend on ImGui.
- Panel `draw()` functions run every rendered frame — treat them as a hot path (§7 applies in full: no uncached `StringName` construction, no unnecessary allocation).
- Any action that mutates the scene from a panel (property edit, reparent, delete, move) must call `EditorState::push_undo_snapshot()` (or, once wired up, go through `UndoRedo`) so it's covered by undo. `inspector_panel.cpp` currently does not do this for property edits — that's a gap, not the pattern to follow for new panels.
- There is currently one fully-built, unused undo mechanism (`core/undo_redo.h/.cpp`, command-pattern with do/undo callbacks) sitting alongside the one actually in use (`EditorState`'s full-tree JSON snapshot stack). Don't add a third. If you're improving undo, either finish wiring up `UndoRedo` (and delete the snapshot stack) or delete `UndoRedo` — don't let both keep existing.

---

## 11. Build system conventions

- The root `CMakeLists.txt` is engine-and-editor only. Game-specific build steps belong in the game project's own `CMakeLists.txt` (see §2).
- Set an explicit optimized configuration — don't rely on generator defaults. At minimum, define a `Release` build with `-O2`/`/O2` and treat "does this still hit frame budget in an optimized build" as part of testing any perf-sensitive change; the debug build is not representative.
- Enable `-Wall -Wextra` (or `/W4` on MSVC) and fix warnings as they appear rather than letting them accumulate silently.
- Output directories should reflect the actual configuration being built, not be hardcoded to a `Debug` path regardless of `CMAKE_BUILD_TYPE`.

---

## 12. Documentation and comments

- A comment that describes *intended* behavior (`// recycles the oldest particle`) must be kept honest — if you change what the code does, update or remove the comment in the same commit. A misleading comment is worse than no comment.
- Design docs at the repo root (`README.md`, technical design docs) describe target/aspirational architecture and may describe systems more completely than they're currently implemented (e.g. swept AABB, frustum culling, spatial-hash-backed `Area2D`). When a design doc and the actual code disagree, the code is what ships — either fix the code to match the doc, or update the doc; don't leave both standing as if they agree.
- New systems get a short header-comment describing ownership (who allocates, who frees, who's allowed to call it) — this is the single biggest source of the lifetime bugs described in §6, and a one-line comment at the declaration is usually enough to prevent them.

---

## 13. Pull request checklist

Before opening a PR against RetroNode, confirm:

- [ ] No new code under `src/` or in the root `CMakeLists.txt` references a specific game project by name or path (§2).
- [ ] Gameplay-affecting state changes happen in `_physics_process` using `Fixed16`, not `_process`/`float` (§1).
- [ ] Any new raw pointer registered with a server/cache has a corresponding, actually-called unregister/teardown path (§6).
- [ ] Any new `Object`-derived (non-`Node`) allocation has a clear, exercised owner that frees it (§6).
- [ ] No `StringName` is constructed from a literal inside a per-frame/per-call hot path without being cached `static` (§7).
- [ ] New positional/overlap queries go through the spatial grid, not a linear scan (§7).
- [ ] New Node types follow the checklist in §8, including the `has_script()` exclusion list.
- [ ] No duplicate subsystem was added where an existing server/manager already owns the responsibility (§3).
- [ ] Builds warning-clean under `-Wall -Wextra` / `/W4` in an optimized configuration (§11).
