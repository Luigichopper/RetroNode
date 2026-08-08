#ifndef RETRONODE_STRING_NAMES_H
#define RETRONODE_STRING_NAMES_H

#include <string>
#include <functional>

namespace RetroNode {

class StringName {
private:
    std::string name;
    size_t hash_value;

public:
    StringName() : name(""), hash_value(std::hash<std::string>{}("")) {}
    StringName(const char* str) : name(str ? str : ""), hash_value(std::hash<std::string>{}(name)) {}
    StringName(const std::string& str) : name(str), hash_value(std::hash<std::string>{}(name)) {}

    const std::string& c_str() const { return name; }
    const std::string& str() const { return name; }
    size_t hash() const { return hash_value; }

    bool operator==(const StringName& o) const {
        return hash_value == o.hash_value && name == o.name;
    }

    bool operator!=(const StringName& o) const {
        return !(*this == o);
    }

    bool operator<(const StringName& o) const {
        return name < o.name;
    }
};

} // namespace RetroNode

namespace std {
    template<>
    struct hash<RetroNode::StringName> {
        size_t operator()(const RetroNode::StringName& sn) const noexcept {
            return sn.hash();
        }
    };
}

#endif // RETRONODE_STRING_NAMES_H
