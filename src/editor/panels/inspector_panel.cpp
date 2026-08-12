#include "inspector_panel.h"
#include "../editor_state.h"
#include "../file_dialog.h"
#include "sprite_frames_panel.h"
#include "../../core/object/class_db.h"
#include "../../scene/2d/camera_2d.h"
#include "../../scene/2d/animated_sprite_2d.h"
#include <imgui.h>
#include <imgui_stdlib.h>
#include <iostream>


namespace RetroNode {

void InspectorPanel::draw() {
    ImGui::Begin("Inspector");

    Object* target = EditorState::get()->get_selected_object();
    if (!target) {
        ImGui::TextDisabled("No node selected");
        ImGui::End();
        return;
    }

    Node* node_target = dynamic_cast<Node*>(target);

    // Node Name Input
    if (node_target) {
        char name_buf[128];
        snprintf(name_buf, sizeof(name_buf), "%s", node_target->get_name().c_str());
        if (ImGui::InputText("Name", name_buf, sizeof(name_buf))) {
            node_target->set_name(name_buf);
        }
        std::string class_display = target->get_class_name().as_string();
        if (node_target->has_script() && class_display == "PlayerController") {
            class_display = "CharacterBody2D";
        }
        ImGui::TextDisabled("Class: %s", class_display.c_str());


        if (node_target->is_instanced_subscene()) {
            ImGui::TextColored(ImVec4(0.3f, 0.75f, 1.0f, 1.0f), "Instanced Scene: %s", node_target->get_scene_instance_path().c_str());
        }

        if (node_target->has_script()) {
            std::string s_info = node_target->get_script_path();
            if (s_info.empty()) s_info = target->get_class_name().as_string();
            ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.3f, 1.0f), "Script: %s", s_info.c_str());
            ImGui::SameLine();
            if (ImGui::Button("Detach")) {
                node_target->set_script_path("");
                EditorState::get()->push_undo_snapshot();
            }
        } else {
            static char script_buf[128] = "";
            ImGui::SetNextItemWidth(140.0f);
            ImGui::InputTextWithHint("##ScriptPath", "Script class/path...", script_buf, sizeof(script_buf));
            ImGui::SameLine();
            if (ImGui::Button("Attach Script")) {
                if (script_buf[0] != '\0') {
                    node_target->set_script_path(script_buf);
                    EditorState::get()->push_undo_snapshot();
                }
            }
        }

        AnimatedSprite2D* anim_sprite = dynamic_cast<AnimatedSprite2D*>(node_target);
        if (anim_sprite) {
            ImGui::Spacing();
            if (ImGui::Button("Edit SpriteFrames...", ImVec2(-1, 28))) {
                SpriteFramesPanel::open = true;
                ImGui::SetWindowFocus("SpriteFrames");
            }
        }

        ImGui::Separator();

    }

    std::vector<PropertyInfo> props = target->get_property_list();
    static const StringName s_name("name");

    for (const auto& pinfo : props) {
        if (pinfo.name == s_name) continue; // Handled above

        std::string label = pinfo.name.as_string();
        Variant val = ClassDB::get_property(target, pinfo.name);
        if (val.is_nil()) {
            val = target->get(pinfo.name);
        }

        ImGui::PushID(label.c_str());
        bool value_changed = false;

        switch (pinfo.type) {
            case VariantType::BOOL: {
                bool b = val.as_bool();
                if (ImGui::Checkbox(label.c_str(), &b)) {
                    Variant new_val(b);
                    if (!target->set(pinfo.name, new_val)) {
                        ClassDB::set_property(target, pinfo.name, new_val);
                    }
                    value_changed = true;
                }
                break;
            }
            case VariantType::INT: {
                int i = static_cast<int>(val.as_int());
                if (label == "mode" && dynamic_cast<Camera2D*>(target)) {
                    const char* modes[] = { "0: FOLLOW (Smooth Target)", "1: ZELDA_ROOM (Grid Transition)", "2: SMB1_FORWARD (Scroll Right Only)", "3: ANCHORED (Fixed Position)" };
                    if (ImGui::Combo(label.c_str(), &i, modes, IM_ARRAYSIZE(modes))) {
                        Variant new_val((int64_t)i);
                        if (!target->set(pinfo.name, new_val)) {
                            ClassDB::set_property(target, pinfo.name, new_val);
                        }
                        value_changed = true;
                    }
                } else {
                    if (ImGui::DragInt(label.c_str(), &i)) {
                        Variant new_val((int64_t)i);
                        if (!target->set(pinfo.name, new_val)) {
                            ClassDB::set_property(target, pinfo.name, new_val);
                        }
                        value_changed = true;
                    }
                }
                break;
            }

            case VariantType::FLOAT16: {
                float f = val.as_fixed16().to_float();
                if (ImGui::DragFloat(label.c_str(), &f, 0.1f)) {
                    Variant new_val(Fixed16::from_float(f));
                    if (!target->set(pinfo.name, new_val)) {
                        ClassDB::set_property(target, pinfo.name, new_val);
                    }
                    value_changed = true;
                }
                break;
            }
            case VariantType::VECTOR2: {
                Vector2Fixed v = val.as_vector2();
                float fv[2] = { v.x.to_float(), v.y.to_float() };
                if (ImGui::DragFloat2(label.c_str(), fv, 0.5f)) {
                    Variant new_val(Vector2Fixed::from_floats(fv[0], fv[1]));
                    if (!target->set(pinfo.name, new_val)) {
                        ClassDB::set_property(target, pinfo.name, new_val);
                    }
                    value_changed = true;
                }
                break;
            }
            case VariantType::RECT2: {
                Rect2Fixed r = val.as_rect2();
                float fr[4] = { r.position.x.to_float(), r.position.y.to_float(), r.size.x.to_float(), r.size.y.to_float() };
                if (ImGui::DragFloat4(label.c_str(), fr, 0.5f)) {
                    Rect2Fixed new_r(Vector2Fixed::from_floats(fr[0], fr[1]), Vector2Fixed::from_floats(fr[2], fr[3]));
                    Variant new_val(new_r);
                    if (!target->set(pinfo.name, new_val)) {
                        ClassDB::set_property(target, pinfo.name, new_val);
                    }
                    value_changed = true;
                }
                break;
            }
            case VariantType::COLOR: {
                SDL_Color c = val.as_color();
                float col[4] = { c.r / 255.0f, c.g / 255.0f, c.b / 255.0f, c.a / 255.0f };
                if (ImGui::ColorEdit4(label.c_str(), col)) {
                    SDL_Color new_c = {
                        static_cast<Uint8>(col[0] * 255.0f),
                        static_cast<Uint8>(col[1] * 255.0f),
                        static_cast<Uint8>(col[2] * 255.0f),
                        static_cast<Uint8>(col[3] * 255.0f)
                    };
                    Variant new_val(new_c);
                    if (!target->set(pinfo.name, new_val)) {
                        ClassDB::set_property(target, pinfo.name, new_val);
                    }
                    value_changed = true;
                }
                break;
            }
            case VariantType::STRING:
            case VariantType::STRING_NAME: {
                std::string str_val = val.as_string();
                bool is_file_path = (pinfo.hint == PropertyHint::FILE_PATH) ||
                                    (label.find("path") != std::string::npos) ||
                                    (label.find("texture") != std::string::npos) ||
                                    (label.find("tileset") != std::string::npos) ||
                                    (label.find("stream") != std::string::npos);

                if (is_file_path) {
                    std::string filter = "*.*";
                    if (label.find("texture") != std::string::npos || label.find("tileset") != std::string::npos) {
                        filter = "*.png;*.jpg;*.jpeg;*.bmp";
                    } else if (label.find("stream") != std::string::npos || label.find("audio") != std::string::npos) {
                        filter = "*.wav;*.ogg;*.mp3";
                    } else if (pinfo.hint_string.length() > 0) {
                        filter = pinfo.hint_string;
                    }

                    if (FileDialog::draw_path_picker(label.c_str(), str_val, filter.c_str())) {
                        Variant new_val(str_val);
                        if (!target->set(pinfo.name, new_val)) {
                            ClassDB::set_property(target, pinfo.name, new_val);
                        }
                        value_changed = true;
                    }
                } else {
                    char str_buf[256];
                    snprintf(str_buf, sizeof(str_buf), "%s", str_val.c_str());
                    if (ImGui::InputText(label.c_str(), str_buf, sizeof(str_buf))) {
                        Variant new_val(str_buf);
                        if (!target->set(pinfo.name, new_val)) {
                            ClassDB::set_property(target, pinfo.name, new_val);
                        }
                        value_changed = true;
                    }

                    // Asset Drag-and-Drop Target
                    if (ImGui::BeginDragDropTarget()) {
                        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("RN_ASSET_PATH")) {
                            const char* asset_path = (const char*)payload->Data;
                            Variant new_val(asset_path);
                            if (!target->set(pinfo.name, new_val)) {
                                ClassDB::set_property(target, pinfo.name, new_val);
                            }
                            value_changed = true;
                        }
                        ImGui::EndDragDropTarget();
                    }
                }
                break;
            }
            default:
                break;
        }

        if (value_changed || ImGui::IsItemDeactivatedAfterEdit()) {
            EditorState::get()->push_undo_snapshot();
        }

        ImGui::PopID();
    }

    ImGui::End();
}

} // namespace RetroNode
