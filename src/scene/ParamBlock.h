#pragma once
#include <cstddef>
#include <vector>

// A parametric object's settings, addressed by index like Max's parameter blocks
// (max-format.md §6): e.g. Box 0 = length, 1 = width, 2 = height, ...
namespace scene {

enum class ParamType { Float, Int, Bool };

struct Param {
    ParamType type = ParamType::Float;
    double value = 0;   // ints and bools are stored exactly in a double
};

class ParamBlock {
public:
    void Define(size_t index, ParamType type, double defaultValue);

    double Float(size_t index, double fallback = 0) const;
    int    Int(size_t index, int fallback = 0) const;
    bool   Bool(size_t index, bool fallback = false) const;
    void   Set(size_t index, double value);    // ints are rounded, bools become 0/1
    bool   Has(size_t index) const { return index < params_.size() && defined_[index]; }
    size_t Count() const { return params_.size(); }

private:
    std::vector<Param> params_;
    std::vector<bool> defined_;
};

} // namespace scene
