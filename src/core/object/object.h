#ifndef RETRONODE_OBJECT_H
#define RETRONODE_OBJECT_H

#include <cstdint>
#include <string>
#include "../string_names.h"

namespace RetroNode {

class Object {
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
};

} // namespace RetroNode

#endif // RETRONODE_OBJECT_H
