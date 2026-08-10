#ifndef RETRONODE_OBJECT_DB_H
#define RETRONODE_OBJECT_DB_H

#include "object.h"
#include <unordered_map>

namespace RetroNode {

class RN_API ObjectDB {
private:
    static ObjectDB* instance;
    std::unordered_map<uint64_t, Object*> objects;

public:
    ObjectDB() = default;
    ~ObjectDB() = default;

    static ObjectDB* get() {
        if (!instance) {
            instance = new ObjectDB();
        }
        return instance;
    }

    void register_object(Object* obj);
    void unregister_object(Object* obj);
    Object* get_object(uint64_t instance_id) const;
    bool is_valid(uint64_t instance_id) const;
};

} // namespace RetroNode

#endif // RETRONODE_OBJECT_DB_H
