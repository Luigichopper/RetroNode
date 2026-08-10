#include "scene_tree_panel.h"
#include "../editor_state.h"
#include "../../core/object/class_db.h"
#include <imgui.h>
#include <iostream>

namespace RetroNode {

static bool show_add_node_popup = false;
static Node* popup_target_node = nullptr;

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
        if (ImGui::MenuItem("Add Child Node...")) {
            popup_target_node = node;
            show_add_node_popup = true;
        }
        if (node->get_parent()) {
            if (ImGui::MenuItem("Delete Node")) {
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

    ImGui::End();
}

} // namespace RetroNode
