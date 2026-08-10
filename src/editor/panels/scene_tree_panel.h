#ifndef RETRONODE_SCENE_TREE_PANEL_H
#define RETRONODE_SCENE_TREE_PANEL_H

#include "../../scene/main/node.h"

namespace RetroNode {

class SceneTreePanel {
private:
    static void draw_node_tree(Node* node);

public:
    static void draw();
};

} // namespace RetroNode

#endif // RETRONODE_SCENE_TREE_PANEL_H
