#include "object_db.h"

namespace RetroNode {

ObjectDB* ObjectDB::instance = nullptr;

void ObjectDB::register_object(Object* obj) {
    if (obj) {
        objects[obj->get_instance_id()] = obj;
    }
}

void ObjectDB::unregister_object(Object* obj) {
    if (obj) {
        objects.erase(obj->get_instance_id());
    }
}

Object* ObjectDB::get_object(uint64_t instance_id) const {
    if (instance_id == 0) return nullptr;
    auto it = objects.find(instance_id);
    if (it != objects.end()) {
        return it->second;
    }
    return nullptr;
}

bool ObjectDB::is_valid(uint64_t instance_id) const {
    return get_object(instance_id) != nullptr;
}

} // namespace RetroNode
