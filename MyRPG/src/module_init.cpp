#include <iostream>
#include "player_controller.h"

#if defined(_WIN32)
  #define RN_EXPORT __declspec(dllexport)
#else
  #define RN_EXPORT __attribute__((visibility("default")))
#endif

extern "C" {
    RN_EXPORT void retronode_register_types() {
        std::cout << "[MyRPG] Registering game classes into engine ClassDB..." << std::endl;
    }
}
