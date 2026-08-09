#include "string_names.h"
#include <unordered_set>
#include <mutex>

namespace RetroNode {

static std::unordered_set<std::string> string_pool;
static std::mutex pool_mutex;
static const std::string empty_string = "";

const std::string* StringName::intern(const std::string& str) {
    if (str.empty()) {
        return &empty_string;
    }
    std::lock_guard<std::mutex> lock(pool_mutex);
    auto res = string_pool.insert(str);
    return &(*res.first);
}

const std::string& StringName::get_empty_string() {
    return empty_string;
}

} // namespace RetroNode
