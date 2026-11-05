#include "scene/ParamBlock.h"
#include <cmath>

namespace scene {

void ParamBlock::Define(size_t i, ParamType type, double def) {
    if (i >= params_.size()) { params_.resize(i + 1); defined_.resize(i + 1, false); }
    params_[i].type = type;
    defined_[i] = true;
    Set(i, def);
}

double ParamBlock::Float(size_t i, double fallback) const { return Has(i) ? params_[i].value : fallback; }
int ParamBlock::Int(size_t i, int fallback) const { return Has(i) ? static_cast<int>(params_[i].value) : fallback; }
bool ParamBlock::Bool(size_t i, bool fallback) const { return Has(i) ? params_[i].value != 0 : fallback; }

void ParamBlock::Set(size_t i, double value) {
    if (!Has(i)) return;  // undefined parameters are ignored, as an importer would skip them
    switch (params_[i].type) {
    case ParamType::Float: params_[i].value = value; break;
    case ParamType::Int:   params_[i].value = std::round(value); break;
    case ParamType::Bool:  params_[i].value = value != 0 ? 1 : 0; break;
    }
}

} // namespace scene
