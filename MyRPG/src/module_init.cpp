#include <iostream>
#include "player_controller.h"
#include "core/object/class_db.h"

#if defined(_WIN32)
  #define RN_EXPORT __declspec(dllexport)
#else
  #define RN_EXPORT __attribute__((visibility("default")))
#endif

extern "C" {
    RN_EXPORT void retronode_register_types(RetroNode::ClassDB* db) {
        std::cout << "[MyRPG] Registering PlayerController into ClassDB..." << std::endl;
        if (db) {
            db->register_class(RetroNode::StringName("PlayerController"), []() -> RetroNode::Object* {
                return new PlayerController();
            });
        }
    }
}
