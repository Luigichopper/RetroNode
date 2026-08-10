#ifndef RETRONODE_OBJECT_H
#define RETRONODE_OBJECT_H

#include <cstdint>
#include <string>
#include <vector>
#include "../string_names.h"
#include "property_info.h"
#include "variant.h"

#if defined(_WIN32)
  #ifdef RN_BUILD_ENGINE
    #define RN_API __declspec(dllexport)
  #else
    #define RN_API __declspec(dllimport)
  #endif
#else
  #define RN_API __attribute__((visibility("default")))
#endif

namespace RetroNode {

class RN_API Object {
private:
    uint64_t instance_id;
    static uint64_t next_instance_id;

public:
    Object();
    virtual ~Object();

    uint64_t get_instance_id() const { return instance_id; }

    virtual StringName get_class_name() const {
        return StringName("Object");
    }

    virtual bool is_class(const StringName& p_class) const {
        return p_class == StringName("Object");
    }

    virtual void get_property_list(std::vector<PropertyInfo>& out_list) const {}
    virtual Variant get(const StringName& p_name) const { return Variant(); }
    virtual bool set(const StringName& p_name, const Variant& p_value) { return false; }

    std::vector<PropertyInfo> get_property_list() const {
        std::vector<PropertyInfo> list;
        get_property_list(list);
        return list;
    }
};

} // namespace RetroNode

#endif // RETRONODE_OBJECT_H
