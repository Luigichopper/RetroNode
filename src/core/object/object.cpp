#include "object.h"
#include "object_db.h"

namespace RetroNode {

uint64_t Object::next_instance_id = 1;

Object::Object() {
    instance_id = next_instance_id++;
    ObjectDB::get()->register_object(this);
}

Object::~Object() {
    ObjectDB::get()->unregister_object(this);
}

} // namespace RetroNode
