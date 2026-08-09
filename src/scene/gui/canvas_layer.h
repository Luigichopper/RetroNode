#ifndef RETRONODE_CANVAS_LAYER_H
#define RETRONODE_CANVAS_LAYER_H

#include "../main/node.h"

namespace RetroNode {

class RN_API CanvasLayer : public Node {
    RN_CLASS(CanvasLayer, Node)

public:
    int layer = 1;
    bool visible = true;

    CanvasLayer();
    virtual ~CanvasLayer() = default;

    virtual void _process(float delta) override;
};

} // namespace RetroNode

#endif // RETRONODE_CANVAS_LAYER_H
