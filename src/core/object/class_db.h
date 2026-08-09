#ifndef RETRONODE_CLASS_DB_H
#define RETRONODE_CLASS_DB_H

#include "object.h"
#include <unordered_map>
#include <functional>
#include <memory>
#include <iostream>

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

class RN_API ClassDB {
public:
    using CreationFunc = std::function<Object*()>;

private:
    std::unordered_map<StringName, CreationFunc> creation_map;
    static ClassDB* instance;

public:
    ClassDB();

    static ClassDB* get() {
        if (!instance) {
            instance = new ClassDB();
        }
        return instance;
    }

    void register_class(const StringName& name, CreationFunc func) {
        creation_map[name] = func;
    }

    Object* instantiate(const StringName& name) {
        auto it = creation_map.find(name);
        if (it != creation_map.end() && it->second) {
            return it->second();
        }
        std::cerr << "[ClassDB] Warning: Could not instantiate unregistered class '" << name.c_str() << "'" << std::endl;
        return nullptr;
    }

    bool is_registered(const StringName& name) const {
        return creation_map.find(name) != creation_map.end();
    }
};

template <typename T>
struct ClassRegistrar {
    ClassRegistrar(const char* class_name) {
        ClassDB::get()->register_class(StringName(class_name), []() -> Object* {
            return new T();
        });
    }
};

} // namespace RetroNode

#define RN_CLASS(m_class, m_inherits) \
public: \
    virtual RetroNode::StringName get_class_name() const override { return RetroNode::StringName(#m_class); } \
    virtual bool is_class(const RetroNode::StringName& p_class) const override { \
        if (p_class == RetroNode::StringName(#m_class)) return true; \
        return m_inherits::is_class(p_class); \
    }

#define RN_REGISTER_CLASS(m_class) \
    static RetroNode::ClassRegistrar<m_class> _registrar_##m_class(#m_class);

#endif // RETRONODE_CLASS_DB_H
