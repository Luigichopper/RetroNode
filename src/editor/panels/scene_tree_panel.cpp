#include "scene_tree_panel.h"
#include "../editor_state.h"
#include "../../core/object/class_db.h"
#include <imgui.h>
#include <iostream>
#include <cstring>

namespace RetroNode {

static bool show_add_node_popup = false;
static bool show_rename_popup = false;
static Node* popup_target_node = nullptr;
static Node* renaming_node = nullptr;
static char rename_buf[128] = "";

void SceneTreePanel::draw_node_tree(Node* node) {
    if (!node) return;

    uint64_t instance_id = node->get_instance_id();
    uint64_t selected_id = EditorState::get()->get_selected_instance_id();

    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_DefaultOpen;
    if (instance_id == selected_id) {
        flags |= ImGuiTreeNodeFlags_Selected;
    }

    const auto& children = node->get_children();
    bool is_leaf = children.empty();
    if (is_leaf) {
        flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
    }

    std::string label = node->get_name() + " (" + node->get_class_name().as_string() + ")";
    if (node->is_instanced_subscene()) {
        label += "  [SubScene]";
    }
    if (node->has_script()) {
        label += "  [Script]";
    }
    bool node_open = ImGui::TreeNodeEx((void*)(uintptr_t)instance_id, flags, "%s", label.c_str());

    if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen()) {
        EditorState::get()->set_selected_instance_id(instance_id);
    }

    // Context Menu
    if (ImGui::BeginPopupContextItem()) {
        EditorState::get()->set_selected_instance_id(instance_id);
        if (ImGui::MenuItem("➕ Add Child Node...")) {
            popup_target_node = node;
            show_add_node_popup = true;
        }
        if (node->get_parent()) {
            if (ImGui::MenuItem("📋 Duplicate", "Ctrl+D")) {
                EditorState::get()->push_undo_snapshot();
                Node* dup = node->duplicate();
                if (dup) {
                    dup->set_name(node->get_name() + "_copy");
                    node->get_parent()->add_child(dup);
                    EditorState::get()->set_selected_instance_id(dup->get_instance_id());
                }
            }
            if (ImGui::MenuItem("✏️ Rename", "F2")) {
                renaming_node = node;
                strncpy(rename_buf, node->get_name().c_str(), sizeof(rename_buf));
                show_rename_popup = true;
            }
            ImGui::Separator();
            if (ImGui::MenuItem("🗑️ Delete Node", "Del")) {
                EditorState::get()->push_undo_snapshot();
                EditorState::get()->set_selected_instance_id(0);
                node->queue_free();
            }
        }
        ImGui::EndPopup();
    }

    if (node_open && !is_leaf) {
        for (size_t i = 0; i < children.size(); ++i) {
            draw_node_tree(children[i]);
        }
        ImGui::TreePop();
    }
}

void SceneTreePanel::draw() {
    ImGui::Begin("Scene Tree");

    Node* root = EditorState::get()->get_active_root();
    Node* selected_node = EditorState::get()->get_selected_node();
    ImGuiIO& io = ImGui::GetIO();

    // Node Shortcuts (Duplicate: Ctrl+D)
    if (selected_node && selected_node->get_parent() && ImGui::IsWindowFocused() && io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_D)) {
        EditorState::get()->push_undo_snapshot();
        Node* dup = selected_node->duplicate();
        if (dup) {
            dup->set_name(selected_node->get_name() + "_copy");
            selected_node->get_parent()->add_child(dup);
            EditorState::get()->set_selected_instance_id(dup->get_instance_id());
        }
    }

    if (root) {
        draw_node_tree(root);
    } else {
        ImGui::TextDisabled("No scene active");
        if (ImGui::Button("Create Root Node")) {
            EditorState::get()->new_scene();
        }
    }

    // Right-click background context menu
    if (ImGui::BeginPopupContextWindow(nullptr, ImGuiPopupFlags_NoOpenOverItems | ImGuiPopupFlags_MouseButtonRight)) {
        if (ImGui::MenuItem("Add Node to Root...")) {
            popup_target_node = root;
            show_add_node_popup = true;
        }
        ImGui::EndPopup();
    }

    // Add Child Node Modal / Popup
    if (show_add_node_popup) {
        ImGui::OpenPopup("Add Child Node");
        show_add_node_popup = false;
    }

    if (ImGui::BeginPopupModal("Add Child Node", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("Select Node Class to Add:");
        ImGui::Separator();

        static char filter[64] = "";
        ImGui::InputText("Filter", filter, sizeof(filter));

        std::vector<StringName> classes = ClassDB::get()->get_registered_classes();
        ImGui::BeginChild("ClassList", ImVec2(280, 200), true);
        for (const auto& cname : classes) {
            std::string name_str = cname.as_string();
            if (filter[0] != '\0' && name_str.find(filter) == std::string::npos) continue;

            if (ImGui::Selectable(name_str.c_str())) {
                Object* new_obj = ClassDB::get()->instantiate(cname);
                Node* new_node = dynamic_cast<Node*>(new_obj);
                if (new_node) {
                    new_node->set_name(cname.as_string());
                    if (popup_target_node) {
                        popup_target_node->add_child(new_node);
                    } else if (root) {
                        root->add_child(new_node);
                    }
                    EditorState::get()->set_selected_instance_id(new_node->get_instance_id());
                }
                ImGui::CloseCurrentPopup();
            }
        }
        ImGui::EndChild();

        if (ImGui::Button("Cancel", ImVec2(120, 0))) {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

    // Rename Node Modal / Popup
    if (show_rename_popup) {
        ImGui::OpenPopup("Rename Node");
        show_rename_popup = false;
    }

    if (ImGui::BeginPopupModal("Rename Node", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("New Node Name:");
        ImGui::InputText("##RenameInput", rename_buf, sizeof(rename_buf));
        ImGui::Separator();
        if (ImGui::Button("OK", ImVec2(100, 0)) && rename_buf[0] != '\0') {
            if (renaming_node) {
                EditorState::get()->push_undo_snapshot();
                renaming_node->set_name(rename_buf);
            }
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(100, 0))) {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

    ImGui::End();
}

} // namespace RetroNode
