#include "io/MaxObjects.h"
#include "scene/CreationTool.h"
#include <cstring>

namespace io {

namespace {

template <typename T> T Read(const std::vector<uint8_t>& d, size_t o) { T v; std::memcpy(&v, &d[o], sizeof(T)); return v; }

// An old ParamBlock record (0x0002): mini-chunks with 6-byte headers.
bool OldRecord(const std::vector<uint8_t>& d, const Chunk& k, MaxParam& p, std::vector<std::string>& notes) {
    bool haveIndex = false, haveValue = false;
    for (size_t at = k.begin; at + 6 <= k.end;) {
        const uint16_t type = Read<uint16_t>(d, at);
        const uint32_t size = Read<uint32_t>(d, at + 2);
        if (size < 6 || size > k.end - at) { notes.push_back("ParamBlock: a malformed record"); return false; }
        const size_t payload = size - 6, v = at + 6;
        if (type == 0x0003 && payload >= 4) { p.index = Read<int32_t>(d, v); haveIndex = true; }
        else if (type == 0x0100 && payload >= 4) { p.type = MaxParam::Type::Float; p.value = Read<float>(d, v); haveValue = true; }
        else if (type == 0x0101 && payload >= 4) { p.type = MaxParam::Type::Int; p.value = Read<int32_t>(d, v); haveValue = true; }
        else if (type == 0x0104 && payload >= 4) { p.type = MaxParam::Type::Bool; p.value = Read<int32_t>(d, v) != 0; haveValue = true; }
        at += size;
    }
    if (haveIndex && !haveValue) notes.push_back("ParamBlock: parameter " + std::to_string(p.index) + " has no value of a known type");
    return haveIndex && haveValue;
}

// A ParamBlock2 entry (0x000A): u16 id, u32 type, 5 bytes of flags, then the value.
bool Pb2Entry(const std::vector<uint8_t>& d, const Chunk& k, MaxParam& p, std::vector<std::string>& notes) {
    if (k.end - k.begin < 15) { notes.push_back("ParamBlock2: a short entry"); return false; }
    p.index = Read<uint16_t>(d, k.begin);
    const uint32_t type = Read<uint32_t>(d, k.begin + 2);
    switch (type) {
    case 0: case 5: case 6: case 7: p.type = MaxParam::Type::Float; p.value = Read<float>(d, k.begin + 11); return true;   // float, angle, percent, world
    case 1: p.type = MaxParam::Type::Int; p.value = Read<int32_t>(d, k.begin + 11); return true;
    case 4: p.type = MaxParam::Type::Bool; p.value = Read<int32_t>(d, k.begin + 11) != 0; return true;
    default: notes.push_back("ParamBlock2: parameter " + std::to_string(p.index) + " of type " + std::to_string(type) + " not read"); return false;
    }
}

} // namespace

bool ReadParamBlock(const MaxScene& s, size_t block, std::vector<MaxParam>& out, std::vector<std::string>& notes) {
    out.clear();
    if (block >= s.ObjectCount()) { notes.push_back("parameter block reference outside the scene"); return false; }
    const ClassEntry* c = s.ClassOf(block);
    const bool old = c && c->classIdA == 0x8 && c->classIdB == 0 && c->superClassId == 0x8;
    const bool pb2 = c && c->classIdA == 0x82 && c->classIdB == 0 && c->superClassId == 0x82;
    if (!old && !pb2) { notes.push_back("\"" + (c ? c->name : std::string("?")) + "\" is not a parameter block"); return false; }
    const std::vector<uint8_t>& d = s.SceneBytes();
    for (const Chunk& k : s.ObjectChunk(block).children) {
        MaxParam p;
        if (old && k.type == 0x0002 && OldRecord(d, k, p, notes)) out.push_back(p);
        if (pb2 && k.type == 0x000A && Pb2Entry(d, k, p, notes)) out.push_back(p);
    }
    return true;
}

int PrimitiveTypeOf(const ClassEntry& c) {
    if (c.superClassId != 0x10) return -1;
    // scene::MakePrimitive's types: Box, Cone, Sphere, GeoSphere, Cylinder, Tube, Torus,
    // Pyramid, Teapot, Plane. Class IDs as the samples record them.
    struct Id { uint32_t a, b; int type; };
    static const Id kIds[] = {{0x10, 0, 0}, {0xA86C23DD, 0, 1}, {0x11, 0, 2}, {0, 0x7F9E, 3}, {0x12, 0, 4},
                              {0x7B21, 0, 5}, {0x20, 0, 6}, {0x76BF318A, 0x4BF37B10, 7}, {0xACAD13D3, 0xACAD26D9, 8},
                              {0x081F1DFC, 0x77566F65, 9}};
    for (const Id& id : kIds)
        if (c.classIdA == id.a && c.classIdB == id.b) return id.type;
    return -1;
}

std::shared_ptr<scene::Object> ReadPrimitive(const MaxScene& s, size_t object, std::vector<std::string>& notes) {
    const ClassEntry* c = object < s.ObjectCount() ? s.ClassOf(object) : nullptr;
    const int type = c ? PrimitiveTypeOf(*c) : -1;
    if (type < 0) {
        notes.push_back("\"" + (c ? c->name : std::string("?")) + "\" is not a standard primitive");
        return nullptr;
    }
    const auto& refs = s.Refs(object);
    std::vector<MaxParam> params;
    if (refs.empty() || refs[0] < 0 || !ReadParamBlock(s, static_cast<size_t>(refs[0]), params, notes)) {
        notes.push_back(c->name + ": its parameter block cannot be read");
        return nullptr;
    }
    std::shared_ptr<scene::Object> obj = scene::MakePrimitive(type);
    if (!obj) return nullptr;
    // Radii are clamped to >= 0, as Max does when building (a Cone stored radius 2 = -20
    // while its panel showed 0; max-format.md §6.1). Indices per type.
    static const std::vector<int> kRadii[10] = {{}, {0, 1}, {0}, {0}, {0}, {0, 1}, {0, 1}, {}, {0}, {}};
    // "Slice On" (Cone, Sphere, Tube: index 7) changes the shape, which maxlike's
    // primitives cannot do: reported.
    static const int kSliceOn[10] = {-1, 7, 7, -1, -1, 7, -1, -1, -1, -1};
    scene::ParamBlock& block = obj->Params();
    for (const MaxParam& p : params) {
        if (p.index == kSliceOn[type] && p.value != 0) notes.push_back(c->name + ": Slice is on, which maxlike does not support");
        if (p.index < 0 || !block.Has(static_cast<size_t>(p.index))) continue;   // e.g. mapping coordinates
        double v = p.value;
        for (int r : kRadii[type])
            if (p.index == r && v < 0) v = 0;
        block.Set(static_cast<size_t>(p.index), v);
    }
    return obj;
}

} // namespace io
