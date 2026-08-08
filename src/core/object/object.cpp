#include "object.h"

namespace RetroNode {

uint64_t Object::next_instance_id = 1;

Object::Object() {
    instance_id = next_instance_id++;
}

Object::~Object() {}

} // namespace RetroNode
