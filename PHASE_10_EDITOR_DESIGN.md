# RetroNode Engine — Phase 10: The RetroNode Editor (ImGui)

This document specifies **Phase 10**: adding a Godot-style visual editor to RetroNode, built on
**Dear ImGui**. It covers a Scene Tree / FileSystem **Explorer**, a generic **Inspector**, and a
**Viewport** with selection outlines and transform **gizmos**.

Everything below was written against the actual `main` branch source (`src/`, `CMakeLists.txt`,
`ARCHITECTURAL_ISSUES.md`, `CODE_REVIEW.md`) rather than the engine as originally envisioned in
`TECHNICAL_DESIGN_AND_MVP.md`. Where the real code diverges from what an editor needs, that gap is
called out explicitly and folded into a milestone — this doc is meant to be buildable, not aspirational.

---

## 0. Grounding: What Phase 10 Is Actually Starting From

A full read of `src/` turned up five load-bearing facts that shape every decision below:

1. **There is no property reflection system yet.** `ClassDB` (`src/core/object/class_db.h`) only
   does name → factory registration (`register_class` / `instantiate`). There is no `Variant` type,
   no `PropertyInfo`, no `Object::get()`/`set()`, and no way to enumerate a class's properties.
   `SceneLoader::parse_node_internal` (`src/scene/main/scene_loader.cpp`) instead hand-writes a
   ~300-line chain of `dynamic_cast<T*>` checks, one per node type, to pull fields out of JSON.
   **A generic Inspector cannot be built on top of this as-is.** Reflection is not optional
   prerequisite reading for Phase 10 — it *is* the first milestone (§2).
2. **`ClassDB` can't be enumerated.** `creation_map` is private with no iterator/accessor, and two
   independent registration paths exist: an explicit list in `register_engine_classes()`
   (`src/platform/main.cpp`) and self-registering static initializers (e.g.
   `RN_REGISTER_CLASS(CollisionShape2D)` at file scope in `collision_shape_2d.cpp`). An editor
   "Create Node" popup needs to see the union of both.
3. **`Node2D::rotation` and `Node2D::scale` are dead fields.** `Sprite2D::_process`
   (`src/scene/2d/sprite_2d.cpp`) submits only position/size to `VisualServer::submit_draw_sprite`;
   `DrawCommand` (`src/servers/visual_server.h`) has no rotation/scale members, and
   `VisualServer::render()` calls plain `SDL_RenderTexture`, never `SDL_RenderTextureRotated`. A
   Rotate/Scale gizmo would compile and drag happily and **change nothing on screen**. This is
   scoped into §7 as a small, contained engine fix rather than left as a silent trap.
4. **The game currently owns the whole window.** `VisualServer::render(alpha)` draws the scene into
   a low-res `virtual_framebuffer` texture, then immediately integer-scales and blits *that same
   texture* across the full window and calls `SDL_RenderPresent`, all in one function
   (`src/servers/visual_server.cpp`). For a docked editor, the framebuffer needs to be displayed
   *inside* an ImGui panel instead — this only requires **splitting one existing function**, not
   rewriting the renderer (§4).
5. **There's no live-object safety net.** `Object` assigns a `uint64_t instance_id` on construction
   (`src/core/object/object.cpp`) but nothing indexes IDs back to live pointers. Combined with the
   use-after-free hot-reload issue already flagged in `ARCHITECTURAL_ISSUES.md` (Blocker 2), an
   editor that naively holds a raw `Node*` for "the selected node" across frames is one `queue_free()`
   or one DLL hot-reload away from a crash. §8 adds a minimal `ObjectDB`.

Everything else in `src/` — the `Object`/`Node`/`Node2D` hierarchy, `SceneTree`, `StringName`
interning, the SDL3 + `SDL_Renderer` platform layer, and the FetchContent-based CMake dependency
pattern — is solid ground to build on directly.

---

## 1. Architecture & Guiding Principles

Phase 10 adds a new top-level module, `src/editor/`, that sits **above** `core/`, `scene/`, and
`servers/` — mirroring the existing rule that `core/` never depends on `scene/`. The editor is
allowed to depend on everything; nothing in `core/scene/servers` is allowed to depend on the editor.

```
                 ┌─────────────────────────────┐
                 │           editor/            │  new in Phase 10
                 │  panels, gizmos, EditorState  │
                 └───────────────┬──────────────┘
                                 │ depends on
                 ┌───────────────▼──────────────┐
                 │      scene/  (Node, Node2D…)  │
                 └───────────────┬──────────────┘
                                 │ depends on
                 ┌───────────────▼──────────────┐
                 │  servers/ (VisualServer, …)   │
                 └───────────────┬──────────────┘
                                 │ depends on
                 ┌───────────────▼──────────────┐
                 │   core/  (Object, ClassDB…)   │
                 └───────────────────────────────┘
```

Three principles carried through every milestone:

- **The Inspector must never know about a specific node class.** Every widget it draws comes from
  walking a generic property list. Per-type UI code is a smell — if the Inspector needs a
  `dynamic_cast`, the reflection layer is missing something.
- **The editor is a tool, not a runtime dependency.** `game.dll` / `libgame.so` never links against
  ImGui or `src/editor/`. Shipping builds should be able to compile the engine executable with
  editor support entirely compiled out.
- **Edit mode and Play mode are the same render pipeline, different tick sources.** The Viewport
  always displays `VisualServer`'s framebuffer texture; the only thing "Play" toggles is whether
  `SceneTree::physics_process` / `process` are being called that frame. This is what makes gizmos,
  selection, and the Inspector keep working uninterrupted when you hit Play.

---

## 2. Milestone 10.0 — Reflection & Variant Foundation (blocking prerequisite)

### 2.1 `Variant`

A small tagged union covering exactly the types RetroNode's nodes currently use as public fields
(cross-referenced against every `props.contains(...)` branch in `scene_loader.cpp`):

```cpp
// src/core/object/variant.h
enum class VariantType {
    NIL, BOOL, INT, FLOAT16,      // FLOAT16 = Fixed16, engine's native numeric type
    VECTOR2, RECT2, COLOR, STRING, STRING_NAME, OBJECT_REF
};

class RN_API Variant {
    VariantType type = VariantType::NIL;
    union {
        bool b;
        int64_t i;
        Fixed16 f16;
        Vector2Fixed v2;
        Rect2Fixed r2;
        SDL_Color color;
        uint64_t object_instance_id;
    };
    std::string str; // STRING / STRING_NAME payload (outside the union — non-trivial)

public:
    Variant() = default;
    Variant(bool v) : type(VariantType::BOOL), b(v) {}
    Variant(Fixed16 v) : type(VariantType::FLOAT16), f16(v) {}
    Variant(Vector2Fixed v) : type(VariantType::VECTOR2), v2(v) {}
    Variant(Rect2Fixed v) : type(VariantType::RECT2), r2(v) {}
    Variant(SDL_Color v) : type(VariantType::COLOR), color(v) {}
    Variant(const std::string& v) : type(VariantType::STRING), str(v) {}

    VariantType get_type() const { return type; }
    bool as_bool() const;
    Fixed16 as_fixed16() const;
    Vector2Fixed as_vector2() const;
    // ... etc, with permissive coercion (INT<->FLOAT16, etc.) the same way
    // scene_loader.cpp already does `props.value("z_index", 100)` today.
};
```

### 2.2 `PropertyInfo` and the get/set/list triad on `Object`

```cpp
// src/core/object/property_info.h
enum class PropertyHint { NONE, RANGE, FILE_PATH, MULTILINE_TEXT, ENUM, COLOR_NO_ALPHA };

struct PropertyInfo {
    StringName name;
    VariantType type;
    PropertyHint hint = PropertyHint::NONE;
    std::string hint_string; // "0,100,1" for RANGE, "*.png,*.json" for FILE_PATH, "IDLE,WALK,RUN" for ENUM
};
```

```cpp
// src/core/object/object.h — additive to the existing class
virtual void get_property_list(std::vector<PropertyInfo>& out_list) const {
    // base Object exposes nothing; Node overrides and chains upward
}
virtual Variant get(const StringName& p_name) const { return Variant(); }
virtual bool set(const StringName& p_name, const Variant& p_value) { return false; }
```

Each leaf class overrides all three and **chains to its parent first**, the same way `RN_CLASS`
already chains `is_class()`:

```cpp
// src/scene/2d/sprite_2d.cpp — illustrative
void Sprite2D::get_property_list(std::vector<PropertyInfo>& out_list) const {
    Node2D::get_property_list(out_list);
    out_list.push_back({"texture", VariantType::STRING, PropertyHint::FILE_PATH, "*.png"});
    out_list.push_back({"z_index", VariantType::INT});
    out_list.push_back({"modulate", VariantType::COLOR});
}
Variant Sprite2D::get(const StringName& p_name) const {
    if (p_name == StringName("texture")) return Variant(texture_path);
    if (p_name == StringName("z_index")) return Variant((int64_t)z_index);
    if (p_name == StringName("modulate")) return Variant(modulate);
    return Node2D::get(p_name);
}
bool Sprite2D::set(const StringName& p_name, const Variant& p_value) {
    if (p_name == StringName("texture")) { set_texture_path(p_value.as_string()); return true; }
    if (p_name == StringName("z_index")) { z_index = (int)p_value.as_int(); return true; }
    if (p_name == StringName("modulate")) { modulate = p_value.as_color(); return true; }
    return Node2D::set(p_name, p_value);
}
```

This is boilerplate-heavy by hand — recommend a `RN_PROPERTY(name, type, member)` macro to cut the
per-property line count once the pattern above is validated on 2–3 classes, rather than designing
the macro up front against zero real usage.

### 2.3 Two direct payoffs, beyond the Inspector

- **`scene_loader.cpp`'s ~300-line `dynamic_cast` chain collapses** to one generic loop:
  `for (auto& [key, val] : props.items()) node->set(StringName(key), json_to_variant(val));` —
  this is a real simplification of existing code, not just new editor surface area.
- **`Object::to_json()` falls out for free** by walking `get_property_list()` the other direction.
  This was already on the pre-editor checklist as needed for hot-reload state serialization; Phase
  10 needs it too, for the editor's "Save Scene" and for Play-mode sandboxing (§9).

### 2.4 `ClassDB` enumeration

```cpp
// src/core/object/class_db.h — additive
std::vector<StringName> get_registered_class_names() const {
    std::vector<StringName> names;
    names.reserve(creation_map.size());
    for (auto& [name, fn] : creation_map) names.push_back(name);
    return names;
}
```

Sufficient for a flat "Create Node" list. A parent-class lookup (for a categorized/tree popup like
Godot's) needs each `RN_REGISTER_CLASS` to also record its base name — a small macro change, worth
doing in this milestone while `ClassDB` is already being touched, but not required for v1.

---

## 3. Milestone 10.1 — Vendoring Dear ImGui

Follow the exact `FetchContent` pattern already used for SDL3/nlohmann_json/stb. Use the **docking**
branch — panels, dockspace, and floating/tabbed layout all depend on it.

```cmake
# CMakeLists.txt — additive
option(RN_BUILD_EDITOR "Build the RetroNode Editor (ImGui)" ON)

if (RN_BUILD_EDITOR)
    FetchContent_Declare(
        imgui
        GIT_REPOSITORY https://github.com/ocornut/imgui.git
        GIT_TAG        docking
        GIT_SHALLOW    TRUE
    )
    FetchContent_MakeAvailable(imgui)

    add_library(imgui STATIC
        ${imgui_SOURCE_DIR}/imgui.cpp
        ${imgui_SOURCE_DIR}/imgui_draw.cpp
        ${imgui_SOURCE_DIR}/imgui_tables.cpp
        ${imgui_SOURCE_DIR}/imgui_widgets.cpp
        ${imgui_SOURCE_DIR}/imgui_demo.cpp
        ${imgui_SOURCE_DIR}/backends/imgui_impl_sdl3.cpp
        ${imgui_SOURCE_DIR}/backends/imgui_impl_sdlrenderer3.cpp
    )
    target_include_directories(imgui PUBLIC ${imgui_SOURCE_DIR} ${imgui_SOURCE_DIR}/backends)
    target_link_libraries(imgui PUBLIC SDL3::SDL3-shared)
endif()
```

**Backend choice matters here:** RetroNode renders through `SDL_Renderer`, not raw OpenGL
(`SDL_CreateRenderer` in `main.cpp`), so the correct pairing is
`imgui_impl_sdl3` + `imgui_impl_sdlrenderer3` — *not* `imgui_impl_opengl3`. This also means the
`virtual_framebuffer` `SDL_Texture*` can be handed to `ImGui::Image()` directly by casting the
pointer to `ImTextureID`, with no extra texture-copy or GL interop step:

```cpp
ImGui::Image((ImTextureID)(intptr_t)VisualServer::get()->get_framebuffer_texture(),
             ImVec2((float)vw, (float)vh));
```

The `retronode_editor` executable target links `retronode_core` + `imgui`; the plain `retronode`
runtime target (and `game.dll`) never link ImGui, satisfying the "editor is a tool" principle from
§1. Simplest v1: keep one `retronode` executable, gate editor compilation with `RN_BUILD_EDITOR`,
and a `--editor` CLI flag chooses which code path `main()` takes at startup — this avoids
maintaining two entry points while the split is still new.

---

## 4. Milestone 10.2 — Splitting `VisualServer::render()`

Today `render(alpha)` does draw-to-texture *and* blit-to-window *and* `SDL_RenderPresent` in one
call. The editor needs the first part only, so it can composite the game inside a dockable panel
instead of across the whole window.

```cpp
// src/servers/visual_server.h — additive
void render_scene(float alpha);              // was the first half of render()
void present_fullscreen(int win_w, int win_h); // the existing blit+scale+Present logic
SDL_Texture* get_framebuffer_texture() const { return virtual_framebuffer; }

// keep render() as a thin wrapper for the non-editor build path:
void render(float alpha) { render_scene(alpha); int w,h; SDL_GetRenderOutputSize(renderer,&w,&h); present_fullscreen(w,h); }
```

Editor main loop then does, per frame:

```cpp
VisualServer::get()->clear_render_queue();
SceneTree::get()->process(delta);                 // unchanged
VisualServer::get()->render_scene(render_alpha);   // draws into virtual_framebuffer only

ImGui_ImplSDLRenderer3_NewFrame();
ImGui_ImplSDL3_NewFrame();
ImGui::NewFrame();
draw_editor_dockspace_and_panels();                // §5–§7, includes ImGui::Image() of the framebuffer
ImGui::Render();

SDL_SetRenderDrawColor(renderer, 20,20,20,255);
SDL_RenderClear(renderer);
ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), renderer);
SDL_RenderPresent(renderer);                       // editor build presents once, here
```

Input routing changes at the same point: `ImGui_ImplSDL3_ProcessEvent(&event)` runs first for every
polled `SDL_Event`; the event is only forwarded to `Input::get()->handle_event(event)` when
`!ImGui::GetIO().WantCaptureKeyboard/WantCaptureMouse` **and** the engine is in Play mode **and**
the Viewport panel has focus. `Input` currently has no mouse tracking at all (`src/servers/input.h`
is keyboard-action-only) — mouse position for picking/gizmos is read directly from
`ImGui::GetIO().MousePos` inside the Viewport panel rather than added to `Input`, since it's
editor-only concern, not gameplay input.

---

## 5. Milestone 10.3 — Explorer Dock

Two sections in one panel, matching Godot's Scene / FileSystem docks:

**Scene Tree** — recursive `ImGui::TreeNodeEx` over `SceneTree::get()->get_root()`:

```cpp
void draw_scene_tree_recursive(Node* node) {
    if (!node) return;
    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;
    if (node->get_children().empty()) flags |= ImGuiTreeNodeFlags_Leaf;
    if (EditorState::get()->is_selected(node)) flags |= ImGuiTreeNodeFlags_Selected;

    bool open = ImGui::TreeNodeEx(node, flags, "%s  (%s)",
                                   node->get_name().c_str(), node->get_class_name().c_str());
    if (ImGui::IsItemClicked()) EditorState::get()->select(node);
    // right-click: context menu → Add Child (from ClassDB::get_registered_class_names()),
    //              Rename (inline InputText), Duplicate, Delete (queue_free())
    if (open) {
        for (Node* child : node->get_children()) draw_scene_tree_recursive(child);
        ImGui::TreePop();
    }
}
```

Using the `Node*` itself as the `ImGui::TreeNodeEx` ID is safe *within a single frame* (ImGui only
needs ID stability frame-to-frame for the same node, which holds as long as the node isn't
destroyed) — but note §8: **selection state stored between frames must not be a raw `Node*`.**

**FileSystem** — a `std::filesystem::recursive_directory_iterator` over the resolved project
directory (same `project_dir` `main.cpp` already computes), showing `assets/`, `scenes/`, `maps/`.
Double-clicking a `.json`/`.rnb` calls `SceneLoader::load_scene_from_file` +
`SceneTree::get()->set_root(...)`. Dragging a file path out via `ImGui::SetDragDropPayload` is the
mechanism the Inspector's `FILE_PATH`-hinted fields (§6) accept drops from.

---

## 6. Milestone 10.4 — Inspector Dock

Entirely generic, built once §2 exists — this panel should never need editing again when a new node
type is added, only that type's `get_property_list()`/`get`/`set`.

```cpp
void draw_inspector(Object* obj) {
    if (!obj) { ImGui::TextDisabled("No selection."); return; }

    ImGui::Text("%s", obj->get_class_name().c_str());
    ImGui::Separator();

    std::vector<PropertyInfo> props;
    obj->get_property_list(props);

    for (auto& prop : props) {
        ImGui::PushID(prop.name.c_str());
        Variant value = obj->get(prop.name);
        bool changed = false;
        Variant new_value = value;

        switch (prop.type) {
            case VariantType::BOOL: {
                bool b = value.as_bool();
                changed = ImGui::Checkbox(prop.name.c_str(), &b);
                new_value = Variant(b);
                break;
            }
            case VariantType::FLOAT16: {
                float f = value.as_fixed16().to_float();
                changed = ImGui::DragFloat(prop.name.c_str(), &f, 0.5f);
                new_value = Variant(Fixed16::from_float(f));
                break;
            }
            case VariantType::VECTOR2: {
                float v[2] = { value.as_vector2().x.to_float(), value.as_vector2().y.to_float() };
                changed = ImGui::DragFloat2(prop.name.c_str(), v, 0.5f);
                new_value = Variant(Vector2Fixed::from_floats(v[0], v[1]));
                break;
            }
            case VariantType::COLOR: {
                SDL_Color c = value.as_color();
                float col[4] = { c.r/255.f, c.g/255.f, c.b/255.f, c.a/255.f };
                changed = ImGui::ColorEdit4(prop.name.c_str(), col);
                new_value = Variant(SDL_Color{ (Uint8)(col[0]*255), (Uint8)(col[1]*255),
                                                (Uint8)(col[2]*255), (Uint8)(col[3]*255) });
                break;
            }
            case VariantType::STRING: {
                std::string s = value.as_string();
                char buf[256]; strncpy(buf, s.c_str(), sizeof(buf));
                if (prop.hint == PropertyHint::FILE_PATH) {
                    changed = ImGui::InputText(prop.name.c_str(), buf, sizeof(buf));
                    if (ImGui::BeginDragDropTarget()) {
                        if (auto* payload = ImGui::AcceptDragDropPayload("RN_ASSET_PATH")) {
                            new_value = Variant(std::string((const char*)payload->Data));
                            changed = true;
                        }
                        ImGui::EndDragDropTarget();
                    }
                } else {
                    changed = ImGui::InputText(prop.name.c_str(), buf, sizeof(buf));
                }
                if (changed && prop.hint != PropertyHint::FILE_PATH) new_value = Variant(std::string(buf));
                break;
            }
            default: break; // INT / RECT2 / STRING_NAME / OBJECT_REF follow the same shape
        }

        // Commit on release, not every drag tick, so undo history isn't spammed with
        // one entry per pixel of mouse movement.
        if (ImGui::IsItemDeactivatedAfterEdit()) {
            EditorState::get()->push_undo_property_change(obj, prop.name, value, new_value);
            obj->set(prop.name, new_value);
        } else if (changed && prop.type == VariantType::BOOL) {
            obj->set(prop.name, new_value); // checkboxes/combos commit immediately, no drag phase
        }
        ImGui::PopID();
    }
}
```

---

## 7. Milestone 10.5 — Viewport Dock & Gizmos

### 7.1 Displaying the game inside a panel

```cpp
ImGui::Begin("Viewport");
ImVec2 avail = ImGui::GetContentRegionAvail();
ImVec2 panel_screen_pos = ImGui::GetCursorScreenPos();
ImGui::Image((ImTextureID)(intptr_t)VisualServer::get()->get_framebuffer_texture(), avail);
ImDrawList* draw = ImGui::GetWindowDrawList(); // gizmos drawn as an overlay on top of the image
```

### 7.2 A dedicated editor camera, decoupled from any in-scene `Camera2D`

`VisualServer::camera_offset` currently follows whatever `Camera2D` node is active
(`Camera2D::_process`, `src/scene/2d/camera_2d.cpp`). The *editor's* pan/zoom must not fight the
*game's* camera. Recommend a small `EditorCamera2D` state (not a `Node`, just POD in
`EditorState`) — `editor_pan: Vector2Fixed`, `editor_zoom: float` — that the Viewport panel uses for
its own `world_to_screen`/`screen_to_world` math, driven by middle-mouse-drag (pan) and scroll
(zoom), independent of `VisualServer::camera_offset`. The game's own camera and integer scaling
(`present_fullscreen`, §4) stay reserved for the actual Play-mode/shipped view.

```cpp
ImVec2 world_to_screen(Vector2Fixed world_pos, ImVec2 panel_origin,
                        Vector2Fixed editor_pan, float editor_zoom) {
    float sx = (world_pos.x.to_float() - editor_pan.x.to_float()) * editor_zoom;
    float sy = (world_pos.y.to_float() - editor_pan.y.to_float()) * editor_zoom;
    return ImVec2(panel_origin.x + sx, panel_origin.y + sy);
}
```

### 7.3 Selection outline

```cpp
if (Node* sel = EditorState::get()->get_selected()) {
    if (auto* n2d = dynamic_cast<Node2D*>(sel)) {
        ImVec2 p = world_to_screen(n2d->get_global_position(), panel_screen_pos, pan, zoom);
        Vector2Fixed size = get_visual_bounds(sel); // Sprite2D -> texture_size, CollisionShape2D -> rect/radius, etc.
        ImVec2 p2 = world_to_screen(n2d->get_global_position() + size, panel_screen_pos, pan, zoom);
        draw->AddRect(p, p2, IM_COL32(255, 200, 0, 255), 0.0f, 0, 2.0f);
    }
}
```

`get_visual_bounds()` is necessarily per-type (Sprite2D → `texture_size`, CollisionShape2D →
`rect`/`radius`, Control → `size`) — this is the one place in the editor where a small
type-dispatch is legitimate, since "what counts as this node's bounding box" isn't really a
*property* in the reflection sense. Keep it isolated to this one helper function so it doesn't leak
into the Inspector.

### 7.4 Move gizmo — the one gizmo that's fully load-bearing today

Because `position` is the *only* `Node2D` transform field the renderer actually consumes right now
(§0.3), the move gizmo is where Phase 10 delivers immediate, real value:

```cpp
struct MoveGizmoState { bool dragging_x=false, dragging_y=false, dragging_center=false; };

void draw_and_handle_move_gizmo(Node2D* target, ImVec2 origin, ImVec2 panel_pos,
                                 Vector2Fixed pan, float zoom, MoveGizmoState& gz) {
    ImVec2 x_tip(origin.x + 40, origin.y);
    ImVec2 y_tip(origin.x, origin.y - 40);
    draw->AddLine(origin, x_tip, IM_COL32(220,60,60,255), 3.0f);
    draw->AddLine(origin, y_tip, IM_COL32(60,200,90,255), 3.0f);
    draw->AddCircleFilled(origin, 6.0f, IM_COL32(230,230,230,255));

    ImVec2 mouse = ImGui::GetIO().MousePos;
    if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
        if (dist_to_segment(mouse, origin, x_tip) < 6.0f) gz.dragging_x = true;
        else if (dist_to_segment(mouse, origin, y_tip) < 6.0f) gz.dragging_y = true;
        else if (dist(mouse, origin) < 8.0f) gz.dragging_center = true;
    }
    if (ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
        if (gz.dragging_x || gz.dragging_y || gz.dragging_center)
            EditorState::get()->push_undo_property_change(target, "position",
                                                            Variant(gz.drag_start_pos), Variant(target->position));
        gz = {};
    }
    if (gz.dragging_x || gz.dragging_y || gz.dragging_center) {
        ImVec2 delta_screen = ImGui::GetIO().MouseDelta;
        Vector2Fixed delta_world = Vector2Fixed::from_floats(delta_screen.x / zoom, delta_screen.y / zoom);
        if (gz.dragging_x)      target->position.x += delta_world.x;
        else if (gz.dragging_y) target->position.y += delta_world.y;
        else                    target->position += delta_world; // set_position() also resyncs previous_position
    }
}
```

Snap-to-grid: hold `Ctrl` and round `target->position` to the nearest multiple of the project's
tile size (16px default, matching the convention already used by `TileMapLayer` in
`scene_loader.cpp`) on mouse release.

### 7.5 Rotate / Scale gizmos — contingent on a small engine fix

Draggable rotate/scale handles are cheap to add to the gizmo system above, but as established in
§0.3 they'll currently be **inert**: nothing reads `Node2D::rotation`/`scale` when rendering. Bundle
this into the same milestone as the gizmo, not deferred, since the editor is precisely what makes
the gap observable and testable:

```cpp
// src/servers/visual_server.h — DrawCommand, additive
struct DrawCommand {
    // ...existing fields...
    float rotation_degrees = 0.0f;
    Vector2Fixed node_scale = Vector2Fixed::from_floats(1.0f, 1.0f);
};

// src/servers/visual_server.cpp — render_scene(), replace the SDL_RenderTexture call
SDL_FRect dst_rect = { draw_x, draw_y, draw_w * cmd.node_scale.x.to_float(),
                                        draw_h * cmd.node_scale.y.to_float() };
if (tex) {
    SDL_RenderTextureRotated(renderer, tex, &src_frect, &dst_rect,
                              cmd.rotation_degrees, nullptr, SDL_FLIP_NONE);
}
```

`Sprite2D::_process` passes `rotation.to_float()` (converted to degrees) and `scale` through to
`submit_draw_sprite`'s now-extended signature. This is a small, self-contained change confined to
the sprite draw path — it does not touch physics, collision, or the fixed-point determinism
guarantees described in `TECHNICAL_DESIGN_AND_MVP.md` §2, since rotation/scale are purely visual
here (consistent with the engine's existing "float conversion for rendering only" rule).

---

## 8. Object Lifetime & Selection Safety

`EditorState`'s selection must not be a bare `Node*` held across frames. `Object` already stamps a
`uint64_t instance_id` on construction (`src/core/object/object.cpp`) — add a minimal side-table
that tracks liveness, and validate the selection against it every frame:

```cpp
// src/core/object/object_db.h
class RN_API ObjectDB {
    static std::unordered_map<uint64_t, Object*> live_objects;
public:
    static void register_instance(Object* o) { live_objects[o->get_instance_id()] = o; }
    static void unregister_instance(Object* o) { live_objects.erase(o->get_instance_id()); }
    static Object* get_instance(uint64_t id) {
        auto it = live_objects.find(id);
        return it != live_objects.end() ? it->second : nullptr;
    }
};
// hook into Object::Object() / Object::~Object()
```

```cpp
// EditorState — selection stored by ID, resolved fresh each frame
uint64_t selected_instance_id = 0;
Node* get_selected() const {
    return dynamic_cast<Node*>(ObjectDB::get_instance(selected_instance_id)); // nullptr if freed
}
```

This is cheap insurance directly against two documented risks: `queue_free()` deleting the selected
node mid-frame, and the hot-reload use-after-free already flagged as **Blocker 2** in
`ARCHITECTURAL_ISSUES.md`. If Blocker 2 is still unresolved when this milestone lands, recommend
explicitly **disabling hot-reload while the editor's Play mode is active** (or refusing to swap the
DLL and queuing the reload for the next Stop) until that blocker has a real fix — an editor crash is
a worse failure mode than a running game silently missing a code change for a few seconds.

---

## 9. Play Mode & Scene Sandboxing

With rendering already decoupled from present (§4), "Play" only needs to gate the tick, plus protect
the edited tree from being permanently mutated by gameplay:

1. **On Play:** serialize the current tree via `Object::to_json()` (§2.3) into an in-memory string,
   keep editing UI live (Inspector still works on live nodes — this is intentional, mirrors Godot),
   start calling `SceneTree::physics_process`/`process` again.
2. **On Stop:** discard the live tree, `SceneLoader`-parse the saved JSON string back into a fresh
   tree, `SceneTree::get()->set_root(...)` it. Selection (§8) naturally resolves to "nothing" since
   the old instance IDs no longer resolve — acceptable v1 behavior.

This is exactly the **"`Node::duplicate()` for play-mode scene sandboxing"** item already identified
as pre-editor prep work — serialize/deserialize through the same JSON path is a perfectly adequate
implementation of that requirement and reuses infrastructure this phase needs anyway, rather than a
separate deep-clone code path.

---

## 10. Suggested File Layout

```
src/editor/
├── editor_main.cpp            # entry point when --editor is passed; owns the ImGui-augmented loop
├── editor_state.h/.cpp        # selection (by instance id), mode (EDIT/PLAY), undo stack, camera pan/zoom
├── object_db.h/.cpp           # §8 — instance id -> Object* liveness table
├── panels/
│   ├── scene_tree_panel.h/.cpp
│   ├── filesystem_panel.h/.cpp
│   ├── inspector_panel.h/.cpp
│   └── viewport_panel.h/.cpp
└── gizmos/
    ├── gizmo.h                # shared hit-testing helpers (dist_to_segment, world_to_screen)
    ├── move_gizmo.cpp
    ├── rotate_scale_gizmo.cpp
    └── selection_outline.cpp

src/core/object/
├── variant.h/.cpp             # §2.1
├── property_info.h            # §2.2
└── object_db.h/.cpp           # (or under editor/ — it's small either way; core/ if other
                                #  engine systems besides the editor end up wanting liveness checks)
```

---

## 11. Suggested Milestone Sequencing

| # | Milestone | "Done" looks like |
|---|-----------|--------------------|
| 10.0 | Reflection & `Variant` foundation | `scene_loader.cpp`'s dynamic_cast chain replaced by a generic `set()` loop for at least 3 node types; `ClassDB::get_registered_class_names()` works |
| 10.1 | Vendor ImGui, `--editor` flag | Empty docking space renders over the game window; no gizmos/panels yet |
| 10.2 | Split `VisualServer::render()` | Game view renders **inside** a floating/dockable "Viewport" panel via `ImGui::Image()`, not fullscreen |
| 10.3 | Explorer (read-only) | Scene tree browsable, click selects, selection visible in a debug label |
| 10.4 | Inspector (generic) | Selecting any node shows its real properties; editing a field visibly changes the running scene |
| 10.5 | Selection outline + Move gizmo | Click a sprite in the viewport, drag it, position updates in both viewport and Inspector |
| 10.6 | Rotate/Scale gizmo + renderer fix | Same, and rotating actually rotates the sprite on screen (§7.5 landed) |
| 10.7 | Explorer write ops | Add Child / Delete / Rename / Duplicate from context menu |
| 10.8 | Save Scene | `Object::to_json()` round-trips a scene through the editor and back through `SceneLoader` |
| 10.9 | Play / Stop sandboxing | §9 implemented; editing during Play doesn't corrupt the saved-on-disk scene |
| 10.10 | FileSystem panel + drag-drop, undo/redo polish | Drag a `.png` from FileSystem onto a Sprite2D's `texture` field in the Inspector |

---

## 12. Explicitly Out of Scope for Phase 10

- Full affine transform composition (parent rotation/scale propagating to children) — a `core/scene`
  change, not an editor one; only relevant here because it caps what the Rotate/Scale gizmo can mean
  for nested nodes. Worth its own phase.
- Animation timeline editor, shader/material editor, multi-scene tabs, in-editor C++ script
  authoring, project settings UI, source-control integration. All standard Godot-editor features,
  all reasonable *later* phases once Explorer/Inspector/Viewport are solid.
- Anything to do with `PhysicsServer2D` debug shape rendering beyond `CollisionShape2D`'s own bounds
  in the selection outline — a physics debug-draw overlay is a nice Phase 11 addition, not required
  for gizmos to function.

---

## 13. Dependencies on Prior/Ongoing Work

- **`ARCHITECTURAL_ISSUES.md` Blocker 2 (hot-reload use-after-free)** should be confirmed fixed, or
  Play-mode hot-reload explicitly disabled, before shipping Play mode (§9, §8).
- **`CODE_REVIEW.md` §2.7 (two competing class-registration mechanisms)** means §2.4's
  `get_registered_class_names()` must be read from `ClassDB`'s actual map, not from
  `register_engine_classes()`'s hardcoded list, or self-registered types like `CollisionShape2D`,
  `StaticBody2D`, `Area2D`, and `CPUParticles2D` silently won't appear in the editor's "Create Node"
  menu.
- The rest of the pre-editor checklist (`TileMap2D`/`TileMapLayer`, `AnimatedSprite2D`, Control
  nodes, `AudioStreamPlayer`) is already present in `src/scene/` — confirmed during this review — so
  Phase 10 is not blocked on those. `Timer` and `Marker2D` were **not** found anywhere in `src/` and
  are still missing; they're small enough to add opportunistically while wiring up the Inspector
  (a utility node with 1–2 properties is a good first target to validate the §2 reflection pattern
  on before tackling something with many fields).
