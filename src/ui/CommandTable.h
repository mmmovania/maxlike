#pragma once
#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

namespace ui {

// Maps command names ("file.open") to the 16-bit numeric IDs Windows sends in
// WM_COMMAND, and back. Code refers to commands by name; only menus and the
// message handler ever see the numbers. No Windows dependency (unit-tested).
class CommandTable {
public:
    static constexpr uint16_t kFirstId = 40001; // well clear of system IDs (SC_* >= 0xF000)

    // Returns the ID for `name`, registering it if new.
    uint16_t Register(const std::string& name);

    // 0 if unknown.
    uint16_t Find(const std::string& name) const;

    // Empty string if unknown.
    const std::string& Name(uint16_t id) const;

    size_t Size() const { return names_.size(); }

private:
    std::unordered_map<std::string, uint16_t> ids_;
    std::vector<std::string> names_; // index = id - kFirstId
};

} // namespace ui
