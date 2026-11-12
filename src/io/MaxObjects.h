#pragma once
#include "math/Mat4.h"
#include "io/MaxScene.h"
#include "scene/Modifier.h"
#include "scene/Object.h"
#include <map>
#include <memory>
#include <string>
#include <vector>

// A .max file's objects (docs/reference/max-format.md §6–§7): parameter blocks (old
// ParamBlock and ParamBlock2) and the ten standard primitives, created with
// scene::MakePrimitive so they are the same objects the Create panel makes. No Windows
// dependency (unit-tested in tests/test_max_objects.cpp against the samples).
namespace io {

struct MaxParam {
    enum class Type { Float, Int, Bool, Color };
    int index = -1;               // the parameter's index (old blocks) or id (ParamBlock2)
    Type type = Type::Float;
    double value = 0;
    math::Vec3 color{0, 0, 0};    // Color (ParamBlock2 types 2 and 3: an RGB or a point, 11.4b)
};

// A Standard material's diffuse colour (11.4b, material.max): the material's shader
// (reference 2, Blinn) holds it in its ParamBlock2 as colour parameter 1.
bool ReadStandardDiffuse(const MaxScene& scene, size_t material, math::Vec3& diffuse, std::vector<std::string>& notes);

// The parameters of the block at scene-list index `block` (ParamBlock or ParamBlock2).
// Values it cannot decode are skipped and reported in `notes`.
bool ReadParamBlock(const MaxScene& scene, size_t block, std::vector<MaxParam>& out, std::vector<std::string>& notes);

// maxlike's primitive type (scene::MakePrimitive) for a class, or -1. Matched on both
// class-ID words (GeoSphere's significant word is the second).
int PrimitiveTypeOf(const ClassEntry& c);

// The primitive at scene-list index `object`, with its parameters; null (and a note) if
// it is not one of the ten or its block cannot be read.
std::shared_ptr<scene::Object> ReadPrimitive(const MaxScene& scene, size_t object, std::vector<std::string>& notes);

// An Editable Mesh (class 0xE44F10B3; §7.3): its stored vertices and faces, with
// smoothing groups, material IDs (the face flags' high 16 bits) and edge visibility (the
// low 3 bits), as a scene::MeshObject. Null and a note if it cannot be read.
std::shared_ptr<scene::Object> ReadEditableMesh(const MaxScene& scene, size_t object, std::vector<std::string>& notes);

// Reads any supported object (a primitive, an Editable Mesh, or a Derived Object with Edit
// Mesh's vertex moves; §8), building each scene-list index once: nodes that reference the
// same object get the same scene::Object, so instances stay instances.
class ObjectReader {
public:
    explicit ObjectReader(const MaxScene& scene) : scene_(scene) {}
    std::shared_ptr<scene::Object> Read(size_t object);
    const std::vector<std::string>& Notes() const { return notes_; }

private:
    std::shared_ptr<scene::Object> Build(size_t object, int depth);
    std::shared_ptr<scene::Modifier> ReadModifier(size_t object, const std::vector<Chunk>& context);

    const MaxScene& scene_;
    std::map<size_t, std::shared_ptr<scene::Object>> built_;
    std::vector<std::string> notes_;
};

} // namespace io
