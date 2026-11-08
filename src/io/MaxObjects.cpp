#include "io/MaxObjects.h"
#include "scene/CreationTool.h"
#include <cmath>
#include <cstring>

namespace io {

namespace {

// A little-endian value at offset o (named LE: ObjectReader has a member called Read).
template <typename T> T LE(const std::vector<uint8_t>& d, size_t o) { T v; std::memcpy(&v, &d[o], sizeof(T)); return v; }

// An old ParamBlock record (0x0002): mini-chunks with 6-byte headers.
bool OldRecord(const std::vector<uint8_t>& d, const Chunk& k, MaxParam& p, std::vector<std::string>& notes) {
    bool haveIndex = false, haveValue = false;
    for (size_t at = k.begin; at + 6 <= k.end;) {
        const uint16_t type = LE<uint16_t>(d, at);
        const uint32_t size = LE<uint32_t>(d, at + 2);
        if (size < 6 || size > k.end - at) { notes.push_back("ParamBlock: a malformed record"); return false; }
        const size_t payload = size - 6, v = at + 6;
        if (type == 0x0003 && payload >= 4) { p.index = LE<int32_t>(d, v); haveIndex = true; }
        else if (type == 0x0100 && payload >= 4) { p.type = MaxParam::Type::Float; p.value = LE<float>(d, v); haveValue = true; }
        else if (type == 0x0101 && payload >= 4) { p.type = MaxParam::Type::Int; p.value = LE<int32_t>(d, v); haveValue = true; }
        else if (type == 0x0104 && payload >= 4) { p.type = MaxParam::Type::Bool; p.value = LE<int32_t>(d, v) != 0; haveValue = true; }
        at += size;
    }
    if (haveIndex && !haveValue) notes.push_back("ParamBlock: parameter " + std::to_string(p.index) + " has no value of a known type");
    return haveIndex && haveValue;
}

// A ParamBlock2 entry (0x000A): u16 id, u32 type, 5 bytes of flags, then the value.
bool Pb2Entry(const std::vector<uint8_t>& d, const Chunk& k, MaxParam& p, std::vector<std::string>& notes) {
    if (k.end - k.begin < 15) { notes.push_back("ParamBlock2: a short entry"); return false; }
    p.index = LE<uint16_t>(d, k.begin);
    const uint32_t type = LE<uint32_t>(d, k.begin + 2);
    switch (type) {
    case 0: case 5: case 6: case 7: p.type = MaxParam::Type::Float; p.value = LE<float>(d, k.begin + 11); return true;   // float, angle, percent, world
    case 1: p.type = MaxParam::Type::Int; p.value = LE<int32_t>(d, k.begin + 11); return true;
    case 4: p.type = MaxParam::Type::Bool; p.value = LE<int32_t>(d, k.begin + 11) != 0; return true;
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

std::shared_ptr<scene::Object> ReadEditableMesh(const MaxScene& s, size_t object, std::vector<std::string>& notes) {
    const ClassEntry* c = object < s.ObjectCount() ? s.ClassOf(object) : nullptr;
    if (!c || c->classIdA != 0xE44F10B3u || c->classIdB != 0 || c->superClassId != 0x10) {
        notes.push_back("\"" + (c ? c->name : std::string("?")) + "\" is not an Editable Mesh");
        return nullptr;
    }
    const std::vector<uint8_t>& d = s.SceneBytes();
    const Chunk* data = nullptr;
    for (const Chunk& k : s.ObjectChunk(object).children)
        if (k.type == 0x08FE && k.container) data = &k;
    if (!data) { notes.push_back("Editable Mesh: no mesh data (0x08FE)"); return nullptr; }
    scene::Mesh mesh;
    bool haveVerts = false, haveFaces = false;
    for (const Chunk& k : data->children) {
        const size_t size = k.end - k.begin;
        if (size < 4) continue;
        const uint32_t n = LE<uint32_t>(d, k.begin);
        if (k.type == 0x0914) {                                   // u32 count, count x f32[3]
            if (static_cast<uint64_t>(n) * 12 > size - 4) { notes.push_back("Editable Mesh: vertex list longer than its chunk"); return nullptr; }
            mesh.verts.reserve(n);
            for (uint32_t i = 0; i < n; ++i) {
                const size_t o = k.begin + 4 + 12 * static_cast<size_t>(i);
                mesh.verts.push_back({LE<float>(d, o), LE<float>(d, o + 4), LE<float>(d, o + 8)});
            }
            haveVerts = true;
        } else if (k.type == 0x0912) {                            // u32 count, count x (a, b, c, smoothing, flags)
            if (static_cast<uint64_t>(n) * 20 > size - 4) { notes.push_back("Editable Mesh: face list longer than its chunk"); return nullptr; }
            mesh.faces.reserve(n);
            for (uint32_t i = 0; i < n; ++i) {
                const size_t o = k.begin + 4 + 20 * static_cast<size_t>(i);
                scene::Face f;
                for (int j = 0; j < 3; ++j) f.v[j] = LE<uint32_t>(d, o + 4 * static_cast<size_t>(j));
                f.smoothing = LE<uint32_t>(d, o + 12);
                const uint32_t flags = LE<uint32_t>(d, o + 16);
                f.edgeVisible = static_cast<uint8_t>(flags & 0x7);     // EDGE_A, EDGE_B, EDGE_C
                f.materialId = static_cast<uint16_t>(flags >> 16);     // FACE_MATID_SHIFT
                mesh.faces.push_back(f);
            }
            haveFaces = true;
        }
    }
    if (!haveVerts || !haveFaces) { notes.push_back("Editable Mesh: vertices or faces missing"); return nullptr; }
    for (const scene::Face& f : mesh.faces)
        for (uint32_t v : f.v)
            if (v >= mesh.verts.size()) { notes.push_back("Editable Mesh: a face refers to a missing vertex"); return nullptr; }
    return std::make_shared<scene::MeshObject>(std::move(mesh));
}

std::shared_ptr<scene::Object> ObjectReader::Read(size_t object) { return Build(object, 0); }

std::shared_ptr<scene::Object> ObjectReader::Build(size_t object, int depth) {
    if (object >= scene_.ObjectCount()) { notes_.push_back("object reference outside the scene"); return nullptr; }
    if (const auto it = built_.find(object); it != built_.end()) return it->second;   // an instance
    if (depth > 32) { notes_.push_back("objects nested too deeply"); return nullptr; }
    std::shared_ptr<scene::Object> result;
    if (scene_.IsDerivedObject(object)) {
        // References: modifiers from the top of the stack down, then the base (§8.1);
        // the modifier contexts (0x2500) in the same order, each group from a 0x2510.
        const auto& refs = scene_.Refs(object);
        if (refs.empty() || refs.back() < 0) { notes_.push_back("Derived Object without a base"); return nullptr; }
        auto base = Build(static_cast<size_t>(refs.back()), depth + 1);
        if (!base) return nullptr;
        std::vector<std::vector<Chunk>> contexts;
        for (const Chunk& k : scene_.ObjectChunk(object).children) {
            if (k.type != 0x2500) continue;
            std::vector<Chunk> inner;
            std::string error;
            if (!ParseChunks(scene_.SceneBytes(), k.begin, k.end, inner, error)) { notes_.push_back("modifier contexts: " + error); break; }
            for (Chunk& c : inner) {
                if (c.type == 0x2510 || contexts.empty()) contexts.emplace_back();
                contexts.back().push_back(std::move(c));
            }
        }
        auto derived = std::make_shared<scene::DerivedObject>(base);
        for (size_t i = refs.size() - 1; i-- > 0;) {              // bottom of the stack first
            if (refs[i] < 0) continue;
            static const std::vector<Chunk> kNone;
            if (auto mod = ReadModifier(static_cast<size_t>(refs[i]), i < contexts.size() ? contexts[i] : kNone)) derived->AddModifier(mod);
        }
        result = derived;
    } else if (const ClassEntry* c = scene_.ClassOf(object); c && c->classIdA == 0xE44F10B3u && c->classIdB == 0) {
        result = ReadEditableMesh(scene_, object, notes_);
    } else {
        result = ReadPrimitive(scene_, object, notes_);
    }
    built_[object] = result;
    return result;
}

// Edit Mesh (class 0x50, superclass 0x810; §8.3): its local data in the modifier context
// (0x2512 > 0x4000): the input mesh's vertex count and the vertex moves. Other modifiers
// are reported and skipped.
std::shared_ptr<scene::Modifier> ObjectReader::ReadModifier(size_t object, const std::vector<Chunk>& context) {
    const ClassEntry* c = scene_.ClassOf(object);
    if (!c || c->classIdA != 0x50 || c->classIdB != 0 || c->superClassId != 0x810) {
        notes_.push_back("modifier \"" + (c ? c->name : std::string("?")) + "\" is not supported yet; skipped");
        return nullptr;
    }
    const std::vector<uint8_t>& d = scene_.SceneBytes();
    uint32_t expected = 0;
    bool haveCount = false;
    std::vector<scene::EditMeshModifier::Move> moves;
    for (const Chunk& local : context) {
        if (local.type == 0x2510 && local.end - local.begin >= 48) {
            // The modifier's context transform (3 x 4); identity in every sample. Edits are
            // applied in object space, so another transform is reported.
            static const float kIdentity[12] = {1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0};
            for (int i = 0; i < 12; ++i)
                if (std::fabs(LE<float>(d, local.begin + 4 * static_cast<size_t>(i)) - kIdentity[i]) > 1e-5f) {
                    notes_.push_back("Edit Mesh: a modifier context transform other than identity is not applied");
                    break;
                }
        }
        if (local.type != 0x2512) continue;
        for (const Chunk& k : local.children) {
            if (k.type != 0x4000) continue;
            for (const Chunk& e : k.children) {
                const size_t size = e.end - e.begin;
                if (e.type == 0x0100 && size >= 4) { expected = LE<uint32_t>(d, e.begin); haveCount = true; }
                else if (e.type == 0x0140 && size >= 4) {
                    const uint32_t n = LE<uint32_t>(d, e.begin);
                    if (static_cast<uint64_t>(n) * 16 > size - 4) { notes_.push_back("Edit Mesh: move list longer than its chunk"); continue; }
                    for (uint32_t i = 0; i < n; ++i) {
                        const size_t o = e.begin + 4 + 16 * static_cast<size_t>(i);
                        moves.push_back({LE<uint32_t>(d, o), {LE<float>(d, o + 4), LE<float>(d, o + 8), LE<float>(d, o + 12)}});
                    }
                } else if (e.type != 0x0110 && e.type != 0x0300) {
                    notes_.push_back("Edit Mesh: edits other than vertex moves are not imported");
                }
            }
        }
    }
    if (!haveCount) { notes_.push_back("Edit Mesh: no local data; imported without edits"); }
    return std::make_shared<scene::EditMeshModifier>(expected, moves);
}

} // namespace io
