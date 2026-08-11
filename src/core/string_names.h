#ifndef RETRONODE_STRING_NAMES_H
#define RETRONODE_STRING_NAMES_H

#include <string>
#include <functional>

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

/**
 * @brief High-performance interned string identifier for ClassDB, Scene Tree, and Input actions.
 */
class RN_API StringName {
private:
    const std::string* data_ptr = nullptr;
    size_t hash_value = 0;

    static const std::string* intern(const std::string& str);
    static const std::string& get_empty_string();

public:
    StringName() noexcept : data_ptr(intern("")), hash_value(reinterpret_cast<size_t>(data_ptr)) {}
    
    StringName(const char* str) 
        : data_ptr(intern(str ? str : "")), hash_value(reinterpret_cast<size_t>(data_ptr)) {}
    
    StringName(const std::string& str) 
        : data_ptr(intern(str)), hash_value(reinterpret_cast<size_t>(data_ptr)) {}

    /// Returns standard null-terminated C-string pointer
    const char* c_str() const noexcept { return data_ptr ? data_ptr->c_str() : ""; }
    
    /// Returns reference to internal std::string
    const std::string& as_string() const noexcept { return data_ptr ? *data_ptr : get_empty_string(); }
    const std::string& str() const noexcept { return as_string(); }

    size_t hash() const noexcept { return hash_value; }

    // O(1) fast pointer comparison for interned strings
    bool operator==(const StringName& o) const noexcept {
        return data_ptr == o.data_ptr;
    }

    bool operator!=(const StringName& o) const noexcept {
        return data_ptr != o.data_ptr;
    }

    bool operator<(const StringName& o) const noexcept {
        if (data_ptr == o.data_ptr) return false;
        return as_string() < o.as_string();
    }
};

} // namespace RetroNode

namespace std {
    /**
     * @brief Specialization of std::hash for RetroNode::StringName for unordered container support.
     */
    template<>
    struct hash<RetroNode::StringName> {
        size_t operator()(const RetroNode::StringName& sn) const noexcept {
            return sn.hash();
        }
    };
}

#endif // RETRONODE_STRING_NAMES_H
