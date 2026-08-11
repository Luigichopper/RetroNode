---
trigger: always_on
---

# RetroNode Engineering Guidelines

This document is the source of truth for RetroNode's organization, architecture, and extension rules. Every contribution must adhere to these standards to maintain codebase consistency.

---

## 1. Core Architecture Pillars

RetroNode is a **game-agnostic 2D retro engine** designed for fixed virtual resolution, limited simultaneous effects, and tile/sprite-based rendering.

1. **Determinism:** Gameplay state (position, physics) is advanced on the fixed 60 Hz `_physics_process` step using `Fixed16` (Q16.16) fixed-point math — never `float`/`double` or `_process`. Variable-rate rendering interpolation (`previous_position` → `position` blended by `alpha`) is restricted to `_process`.
2. **Zero-Overhead:** Single-threaded execution end-to-end. No per-frame heap allocations, no mutexes/synchronization primitives without a benchmarked necessity, and no $O(n)$ global scans where indexed or spatial lookups exist.
3. **Server-Node Split:** User-facing scene graph classes (`Node`, `Node2D`, `Sprite2D`, `CharacterBody2D`) hold *data and identity*. Backend singletons (`VisualServer`, `PhysicsServer2D`, `AudioServer`, `TextureServer`, `Input`) hold *systems and hardware state*. Nodes access servers via `::get()`; servers operate on IDs/handles and never inspect node internals.
4. **Engine/Game Separation:** Game code resides outside `src/` and compiles to a hot-reloadable shared library. The engine executable remains game-agnostic.

### Non-Goals

3D rendering, general-purpose scripting VMs, networking, or features outside 2D retro engine scopes.

---

## 2. Game Agnosticism

Nothing under `src/` or in the root `CMakeLists.txt` may reference a specific game project by name, path, or asset layout.

* **Asset Paths:** Assets are resolved through configured project search roots (`project.rnode`).
* **Build Targets:** Game-specific asset processors and targets belong strictly in the game project's `CMakeLists.txt`.

---

## 3. Repository Layout & Layering

```
src/
├── core/       # Object model, reflection (ClassDB/ObjectDB), StringName, Variant,
│               # fixed-point math (Fixed16, Vector2Fixed), resources. No SDL, no scene tree.
├── scene/      # Scene graph nodes: main/, 2d/, physics/, gui/, audio/, animation/.
├── servers/    # Backend singletons: VisualServer, PhysicsServer2D, AudioServer,
│               # TextureServer, Input. Depends only on core/.
├── platform/   # SDL3 integration, main loop, hot-reload mechanism.
├── editor/     # ImGui editor (compiled when RN_BUILD_EDITOR is defined).
└── tools/      # Generic engine asset scripts.

```

Dependencies flow strictly one way: `platform → editor → scene → servers → core`.

---

## 4. Naming Conventions

All engine code lives within the `RetroNode` namespace.

| Element | Convention | Examples |
| --- | --- | --- |
| Classes / Structs | `PascalCase` | `Node2D`, `PhysicsServer2D`, `ClassDB` |
| Files | `snake_case` (`.h`/`.cpp`) | `node_2d.h`, `character_body_2d.cpp` |
| Methods | `snake_case`, verb-first | `get_global_position()`, `queue_free()` |
| Getters / Setters | `get_x()` / `set_x()` | `get_position()` / `set_position()` |
| Booleans | `is_x()` / `has_x()` | `is_visible()`, `has_script()` |
| Members | `snake_case` (no prefixes) | `position`, `active_bodies` |
| Macros | `RN_` prefix, `ALL_CAPS` | `RN_CLASS`, `RN_REGISTER_CLASS` |
| Reflected Properties | `snake_case` string | `"position"`, `"texture_path"` |

* Do not suffix boolean members with `_flag`.
* Do not generate synthetic IDs via arithmetic on `get_instance_id()`. Use a dedicated ID space for sub-elements.

---

## 5. Object Model & Reflection (`ClassDB`)

Scene graph classes must implement the following pattern:

1. Inherit from `Node` and declare `RN_CLASS(ThisClass, ParentClass)` in the header.
2. Register in `src/platform/main.cpp` using `RN_REGISTER_CLASS(ThisClass)`.
3. Register exposed properties in field declaration order via `ClassDB::register_property()`.
4. Override `get_property_list()`, `get()`, and `set()`, calling the parent implementation first.
5. Cache property lookup strings as `static const StringName` in local scopes.
6. Add the class name to `Node::has_script()` exclusion list in `node.h`.

---

## 6. Ownership & Lifetime Rules

* **Scene Tree:** Parents own child nodes (`Node::~Node()` deletes children recursively). Use `queue_free()` instead of manual `delete`.
* **Server Registration:** Any pointer or object registered with a server MUST be explicitly unregistered during destruction or `queue_free()`.
* **Non-Node Resources:** `Object`-derived non-nodes (`Gradient`, `Curve`, textures) must be freed by their declared owner or managed via reference counting.
* **Cross-Node References:** Do not store naked cross-node pointers across frames. Re-evaluate or validate via `ObjectDB::is_valid(instance_id)`.
* **Singletons:** Singletons rely on static allocation or process lifetime. Hardware resources require an explicit `shutdown()` call invoked from `main.cpp`.

---

## 7. Performance Rules

* **`StringName` Caching:** Never construct `StringName` from string literals inside per-frame loops or hot paths. Hoist to `static const StringName`.
* **Spatial Grid:** Use `PhysicsServer2D` spatial grid queries for positional lookups, overlaps, and proximity checks instead of linear node iteration.
* **Bounds Culling:** Compute visible bounds using camera view limits before iterating gridded structures (e.g., tilemaps).
* **Deferred Mutation:** Queue structural changes during container iteration rather than allocating temporary collection copies.
* **Sorting:** Use `std::stable_sort` for key-based orderings (e.g., z-index).
* **Timestep Caps:** Bound fixed-timestep physics iteration loops to prevent execution stalls under large delta spikes.

---

## 8. Extension Checklists

### Adding a Node Type

1. Create header/source files in `src/scene/<category>/` using `RN_CLASS(NewType, ParentType)`.
2. Assign `name = "NewType"` in the constructor.
3. Implement `get_property_list()`, `get()`, and `set()`, chaining calls to the base class.
4. Add `RN_REGISTER_CLASS(NewType)` and `ClassDB::register_property()` calls to `main.cpp`.
5. Add class name to `Node::has_script()` exclusion list.
6. Wire destructor/cleanup logic to unregister server references.
7. Implement deterministic logic inside `_physics_process` using `Fixed16`.
8. Register source files in root `CMakeLists.txt` under `ENGINE_CORE_SOURCES`.

### Adding a Server

1. Create source files in `src/servers/` (depend only on `core/`).
2. Provide a singleton accessor (`::get()`) and an explicit `shutdown()` method for cleanup.
3. Accept IDs and handles rather than direct `Node*` pointers.
4. Back spatial or lookup queries with hash maps or spatial grids.

---

## 9. Pull Request Checklist

* [ ] No references to specific game projects or paths exist under `src/` or root `CMakeLists.txt`.
* [ ] Gameplay state mutations occur exclusively within `_physics_process` using `Fixed16`.
* [ ] Registered server handles/pointers are unregistered on object destruction.
* [ ] Non-node heap allocations have an explicit cleanup owner.
* [ ] Hot-path `StringName` instances are cached as `static const`.
* [ ] Spatial queries utilize spatial grid indexing rather than linear scans.
* [ ] New nodes follow the checklist in §8 and update `Node::has_script()`.
* [ ] No redundant subsystems or managers were added.