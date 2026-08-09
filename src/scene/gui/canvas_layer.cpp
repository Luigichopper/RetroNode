#include "canvas_layer.h"

namespace RetroNode {

CanvasLayer::CanvasLayer() {
    name = "CanvasLayer";
}

void CanvasLayer::_process(float delta) {
    Node::_process(delta);
}

} // namespace RetroNode
