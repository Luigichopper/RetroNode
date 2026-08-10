#ifndef RETRONODE_SCENE_LOADER_H
#define RETRONODE_SCENE_LOADER_H

#include "node.h"
#include <string>

namespace RetroNode {

class RN_API SceneLoader {
public:
    static Node* load_scene_from_file(const std::string& filepath);
    static Node* load_scene_from_binary(const std::string& filepath);
    static Node* load_scene_from_json_string(const std::string& json_str);
    static std::string serialize_node_to_json_string(const Node* node);
    static bool save_scene_to_file(const Node* node, const std::string& filepath);
};

} // namespace RetroNode

#endif // RETRONODE_SCENE_LOADER_H
