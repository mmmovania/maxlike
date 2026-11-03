#include "ui/CommandTable.h"

namespace ui {

uint16_t CommandTable::Register(const std::string& name) {
    if (auto it = ids_.find(name); it != ids_.end())
        return it->second;
    const auto id = static_cast<uint16_t>(kFirstId + names_.size());
    ids_.emplace(name, id);
    names_.push_back(name);
    return id;
}

uint16_t CommandTable::Find(const std::string& name) const {
    auto it = ids_.find(name);
    return it == ids_.end() ? 0 : it->second;
}

const std::string& CommandTable::Name(uint16_t id) const {
    static const std::string empty;
    if (id < kFirstId)
        return empty;
    const size_t index = static_cast<size_t>(id - kFirstId);
    return index < names_.size() ? names_[index] : empty;
}

} // namespace ui
