#ifndef RETRONODE_SCENE_LOADER_H
#define RETRONODE_SCENE_LOADER_H

#include "node.h"
#include <string>

namespace RetroNode {

class RN_API SceneLoader {
public:
    static Node* load_scene_from_file(const std::string& filepath);
    static Node* load_scene_from_binary(const std::string& filepath);
};

} // namespace RetroNode

#endif // RETRONODE_SCENE_LOADER_H
