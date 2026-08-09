#ifndef RETRONODE_STRING_NAMES_H
#define RETRONODE_STRING_NAMES_H

#include <string>
#include <functional>

namespace RetroNode {

/**
 * @brief High-performance interned string identifier for ClassDB, Scene Tree, and Input actions.
 */
class StringName {
private:
    std::string name;
    size_t hash_value;

public:
    StringName() noexcept : name(""), hash_value(std::hash<std::string>{}("")) {}
    StringName(const char* str) : name(str ? str : ""), hash_value(std::hash<std::string>{}(name)) {}
    StringName(const std::string& str) : name(str), hash_value(std::hash<std::string>{}(name)) {}
    StringName(std::string&& str) noexcept : name(std::move(str)), hash_value(std::hash<std::string>{}(name)) {}

    /// Returns standard null-terminated C-string pointer
    const char* c_str() const noexcept { return name.c_str(); }
    
    /// Returns reference to internal std::string
    const std::string& as_string() const noexcept { return name; }
    const std::string& str() const noexcept { return name; }

    size_t hash() const noexcept { return hash_value; }

    bool operator==(const StringName& o) const noexcept {
        return hash_value == o.hash_value && name == o.name;
    }

    bool operator!=(const StringName& o) const noexcept {
        return !(*this == o);
    }

    bool operator<(const StringName& o) const noexcept {
        return name < o.name;
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
