#pragma once
#include "io/MaxScene.h"
#include "scene/Object.h"
#include <memory>
#include <string>
#include <vector>

// A .max file's objects (docs/reference/max-format.md §6–§7): parameter blocks (old
// ParamBlock and ParamBlock2) and the ten standard primitives, created with
// scene::MakePrimitive so they are the same objects the Create panel makes. No Windows
// dependency (unit-tested in tests/test_max_objects.cpp against the samples).
namespace io {

struct MaxParam {
    enum class Type { Float, Int, Bool };
    int index = -1;               // the parameter's index (old blocks) or id (ParamBlock2)
    Type type = Type::Float;
    double value = 0;
};

// The parameters of the block at scene-list index `block` (ParamBlock or ParamBlock2).
// Values it cannot decode are skipped and reported in `notes`.
bool ReadParamBlock(const MaxScene& scene, size_t block, std::vector<MaxParam>& out, std::vector<std::string>& notes);

// maxlike's primitive type (scene::MakePrimitive) for a class, or -1. Matched on both
// class-ID words (GeoSphere's significant word is the second).
int PrimitiveTypeOf(const ClassEntry& c);

// The primitive at scene-list index `object`, with its parameters; null (and a note) if
// it is not one of the ten or its block cannot be read.
std::shared_ptr<scene::Object> ReadPrimitive(const MaxScene& scene, size_t object, std::vector<std::string>& notes);

} // namespace io
