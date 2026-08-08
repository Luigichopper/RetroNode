#ifndef RETRONODE_SCENE_TREE_H
#define RETRONODE_SCENE_TREE_H

#include "node.h"

namespace RetroNode {

class SceneTree {
private:
    static SceneTree* instance;
    Node* root_node = nullptr;

public:
    SceneTree();
    ~SceneTree();

    static SceneTree* get() {
        if (!instance) {
            instance = new SceneTree();
        }
        return instance;
    }

    void set_root(Node* node);
    Node* get_root() const { return root_node; }

    void physics_process(Fixed16 delta);
    void process(float delta);
};

} // namespace RetroNode

#endif // RETRONODE_SCENE_TREE_H
