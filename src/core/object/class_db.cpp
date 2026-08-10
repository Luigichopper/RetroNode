#include "class_db.h"

namespace RetroNode {

ClassDB* ClassDB::instance = nullptr;

ClassDB::ClassDB() {}

void ClassDB::set_property(Object* p_obj, const StringName& p_prop, const Variant& p_value) {
    if (!p_obj) return;
    StringName cls_name = p_obj->get_class_name();

    auto class_it = get()->property_map.find(cls_name);
    if (class_it != get()->property_map.end()) {
        auto prop_it = class_it->second.find(p_prop);
        if (prop_it != class_it->second.end() && prop_it->second.setter) {
            prop_it->second.setter(p_obj, p_value);
        }
    }
}

Variant ClassDB::get_property(const Object* p_obj, const StringName& p_prop) {
    if (!p_obj) return Variant();
    StringName cls_name = p_obj->get_class_name();

    auto class_it = get()->property_map.find(cls_name);
    if (class_it != get()->property_map.end()) {
        auto prop_it = class_it->second.find(p_prop);
        if (prop_it != class_it->second.end() && prop_it->second.getter) {
            return prop_it->second.getter(p_obj);
        }
    }
    return Variant();
}

std::vector<PropertyInfo> ClassDB::get_property_list(const StringName& p_class_name) {
    auto it = get()->property_list_map.find(p_class_name);
    if (it != get()->property_list_map.end()) {
        return it->second;
    }
    return {};
}

} // namespace RetroNode
