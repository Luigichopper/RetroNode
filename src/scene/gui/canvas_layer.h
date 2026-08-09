#ifndef RETRONODE_CANVAS_LAYER_H
#define RETRONODE_CANVAS_LAYER_H

#include "../main/node.h"

namespace RetroNode {

class RN_API CanvasLayer : public Node {
    RN_CLASS(CanvasLayer, Node)

public:
    int layer = 1;

    CanvasLayer();
    virtual ~CanvasLayer() = default;

    void set_layer(int p_layer) { layer = p_layer; }
    int get_layer() const { return layer; }

    virtual void _process(float delta) override;
};

} // namespace RetroNode

#endif // RETRONODE_CANVAS_LAYER_H
