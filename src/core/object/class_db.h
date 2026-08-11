#ifndef RETRONODE_CLASS_DB_H
#define RETRONODE_CLASS_DB_H

#include "object.h"
#include "variant.h"

#include "../string_names.h"
#include <string>
#include <functional>
#include <unordered_map>
#include <vector>
#include <iostream>
#include <type_traits>

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

struct PropertyAccessor {
    PropertyInfo info;
    std::function<Variant(const Object*)> getter;
    std::function<void(Object*, const Variant&)> setter;
};

class RN_API ClassDB {
public:
    using CreationFunc = std::function<Object*()>;

private:
    std::unordered_map<StringName, CreationFunc> creation_map;
    std::unordered_map<StringName, std::unordered_map<StringName, PropertyAccessor>> property_map;
    std::unordered_map<StringName, std::vector<PropertyInfo>> property_list_map;

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

    std::vector<StringName> get_registered_classes() const {
        std::vector<StringName> result;
        for (const auto& [name, func] : creation_map) {
            result.push_back(name);
        }
        return result;
    }

    void add_property_accessor(const StringName& class_name, const PropertyAccessor& acc) {
        property_map[class_name][acc.info.name] = acc;
        property_list_map[class_name].push_back(acc.info);
    }

    template <typename T, typename SetterP, typename GetterP>
    static void register_property(
        const StringName& p_class_name,
        const PropertyInfo& p_info,
        void (T::*p_setter)(SetterP),
        GetterP (T::*p_getter)() const
    ) {
        PropertyAccessor acc;
        acc.info = p_info;

        acc.getter = [p_getter](const Object* p_obj) -> Variant {
            const T* typed_obj = dynamic_cast<const T*>(p_obj);
            if (typed_obj) {
                return Variant((typed_obj->*p_getter)());
            }
            return Variant();
        };

        acc.setter = [p_setter](Object* p_obj, const Variant& p_val) {
            T* typed_obj = dynamic_cast<T*>(p_obj);
            if (typed_obj) {
                using ParamType = std::decay_t<SetterP>;
                if constexpr (std::is_same_v<ParamType, int>) {
                    (typed_obj->*p_setter)(p_val.to_int());
                } else if constexpr (std::is_same_v<ParamType, Fixed16>) {
                    (typed_obj->*p_setter)(p_val.to_fixed16());
                } else if constexpr (std::is_same_v<ParamType, float>) {
                    (typed_obj->*p_setter)(p_val.to_float());
                } else if constexpr (std::is_same_v<ParamType, bool>) {
                    (typed_obj->*p_setter)(p_val.to_bool());
                } else if constexpr (std::is_same_v<ParamType, std::string>) {
                    (typed_obj->*p_setter)(p_val.to_string());
                } else if constexpr (std::is_same_v<ParamType, Vector2Fixed>) {
                    (typed_obj->*p_setter)(p_val.to_vector2());
                } else if constexpr (std::is_same_v<ParamType, Rect2Fixed>) {
                    (typed_obj->*p_setter)(p_val.to_rect2());
                } else if constexpr (std::is_same_v<ParamType, SDL_Color>) {
                    (typed_obj->*p_setter)(p_val.to_color());
                }
            }
        };

        get()->add_property_accessor(p_class_name, acc);
    }

    static void set_property(Object* p_obj, const StringName& p_prop, const Variant& p_value);
    static Variant get_property(const Object* p_obj, const StringName& p_prop);
    static std::vector<PropertyInfo> get_property_list(const StringName& p_class_name);
};

template <typename T>
struct ClassRegistrar {
    ClassRegistrar(const char* class_name) {
        ClassDB::get()->register_class(StringName(class_name), []() -> Object* {
            return new T();
        });
    }
};

#define RN_CLASS(m_class, m_inherits) \
public: \
    virtual StringName get_class_name() const override { return StringName(#m_class); } \
    virtual bool is_class(const StringName& p_class) const override { \
        return p_class == StringName(#m_class) || m_inherits::is_class(p_class); \
    }

#define RN_REGISTER_CLASS(m_class) \
    static RetroNode::ClassRegistrar<m_class> _registrar_##m_class(#m_class);

} // namespace RetroNode

#endif // RETRONODE_CLASS_DB_H
