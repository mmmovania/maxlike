#include "io/MaxNodes.h"
#include "io/Utf16.h"
#include <cmath>
#include <cstring>

namespace io {

namespace {

const Chunk* Child(const Chunk& c, uint16_t type) {
    for (const Chunk& k : c.children)
        if (k.type == type) return &k;
    return nullptr;
}

// `count` floats from a chunk's payload; false if it is too short.
bool Floats(const std::vector<uint8_t>& d, const Chunk* k, float* out, size_t count) {
    if (!k || k->end - k->begin < 4 * count) return false;
    std::memcpy(out, &d[k->begin], 4 * count);
    return true;
}

bool IsIdentity(const math::Quat& q) { return std::fabs(q.x) < 1e-6 && std::fabs(q.y) < 1e-6 && std::fabs(q.z) < 1e-6; }

// Position, rotation and scale from the three value chunks' layouts (shared by the
// controllers and the node's object offset).
void ReadPosition(const std::vector<uint8_t>& d, const Chunk* k, MaxTransform& t, bool& ok) {
    float f[3];
    if ((ok = Floats(d, k, f, 3))) t.position = {f[0], f[1], f[2]};
}
void ReadRotation(const std::vector<uint8_t>& d, const Chunk* k, MaxTransform& t, bool& ok) {
    float f[4];
    if ((ok = Floats(d, k, f, 4))) t.rotation = math::Quat::FromMaxFile(f[0], f[1], f[2], f[3]);
}
void ReadScale(const std::vector<uint8_t>& d, const Chunk* k, MaxTransform& t, bool& ok) {
    float f[7];
    if ((ok = Floats(d, k, f, 7))) {
        t.scale = {f[0], f[1], f[2]};
        t.scaleAxis = math::Quat::FromMaxFile(f[3], f[4], f[5], f[6]);
    }
}

} // namespace

bool ReadPRSController(const MaxScene& s, int controller, MaxTransform& t, std::vector<std::string>& notes) {
    // A Position/Rotation/Scale controller over Bezier Position, TCB Rotation and Bezier
    // Scale (the verified types); other sub-controllers are reported and their defaults kept.
    const ClassEntry* ctl = controller >= 0 && static_cast<size_t>(controller) < s.ObjectCount() ? s.ClassOf(static_cast<size_t>(controller)) : nullptr;
    if (!ctl || ctl->classIdA != 0x2005 || ctl->classIdB != 0) {
        notes.push_back("controller \"" + (ctl ? ctl->name : std::string("?")) + "\" is not a Position/Rotation/Scale");
        return false;
    }
    const std::vector<uint8_t>& d = s.SceneBytes();
    const auto& sub = s.Refs(static_cast<size_t>(controller));
    struct Part { uint32_t classId; uint16_t chunk; void (*read)(const std::vector<uint8_t>&, const Chunk*, MaxTransform&, bool&); };
    const Part parts[3] = {{0x2008, 0x2503, ReadPosition}, {0x442313, 0x2504, ReadRotation}, {0x2010, 0x2505, ReadScale}};
    for (size_t i = 0; i < 3; ++i) {
        if (i >= sub.size() || sub[i] < 0 || static_cast<size_t>(sub[i]) >= s.ObjectCount()) continue;
        const ClassEntry* sc = s.ClassOf(static_cast<size_t>(sub[i]));
        if (!sc || sc->classIdA != parts[i].classId || sc->classIdB != 0) {
            notes.push_back("controller \"" + (sc ? sc->name : std::string("?")) + "\" not supported; its default used");
            continue;
        }
        bool readOk = true;
        parts[i].read(d, Child(s.ObjectChunk(static_cast<size_t>(sub[i])), parts[i].chunk), t, readOk);
        if (!readOk) notes.push_back(sc->name + ": no value chunk");
    }
    return true;
}

bool ReadPositionController(const MaxScene& s, int controller, math::Vec3& p, std::vector<std::string>& notes) {
    const ClassEntry* c = controller >= 0 && static_cast<size_t>(controller) < s.ObjectCount() ? s.ClassOf(static_cast<size_t>(controller)) : nullptr;
    if (!c || c->classIdA != 0x2008 || c->classIdB != 0) {
        notes.push_back("position controller \"" + (c ? c->name : std::string("?")) + "\" not supported");
        return false;
    }
    MaxTransform t;
    bool ok = true;
    ReadPosition(s.SceneBytes(), Child(s.ObjectChunk(static_cast<size_t>(controller)), 0x2503), t, ok);
    if (ok) p = t.position;
    return ok;
}

std::vector<MaxNode> ReadNodes(const MaxScene& s) {
    const std::vector<uint8_t>& d = s.SceneBytes();
    const std::vector<size_t> indices = s.FindByClassName("Node");
    std::vector<MaxNode> nodes;
    std::vector<int> parentIndex;                       // scene-list index of each node's parent
    for (size_t idx : indices) {
        MaxNode n;
        n.index = idx;
        const Chunk& c = s.ObjectChunk(idx);
        if (const Chunk* k = Child(c, 0x0962)) n.name = Utf16BytesToUtf8(d, k->begin, k->end);
        int parent = -1;
        if (const Chunk* k = Child(c, 0x0960); k && k->end - k->begin >= 4) std::memcpy(&parent, &d[k->begin], 4);
        if (const Chunk* k = Child(c, 0x0960); k && k->end - k->begin >= 8) std::memcpy(&n.flags, &d[k->begin + 4], 4);
        parentIndex.push_back(parent);
        if (const Chunk* k = Child(c, 0x0974); k && k->end - k->begin >= 3) {
            n.hasWireColor = true;
            for (int i = 0; i < 3; ++i) n.wire[i] = d[k->begin + static_cast<size_t>(i)];
        }
        const auto& refs = s.Refs(idx);
        if (refs.size() > 0) n.controller = refs[0];
        if (refs.size() > 1) n.object = refs[1];
        if (refs.size() > 3) n.material = refs[3];

        // The object offset (always present in the samples; identity if absent).
        bool ok = true;
        if (const Chunk* k = Child(c, 0x096A)) ReadPosition(d, k, n.objectOffset, ok);
        if (const Chunk* k = Child(c, 0x096B)) ReadRotation(d, k, n.objectOffset, ok);
        if (const Chunk* k = Child(c, 0x096C)) ReadScale(d, k, n.objectOffset, ok);
        if (!ok) n.notes.push_back("object offset: a value chunk is too short");

        // The transform: a Position/Rotation/Scale controller over Bezier Position, TCB
        // Rotation and Bezier Scale (the verified types); others are reported.
        if (!ReadPRSController(s, n.controller, n.transform, n.notes))
            n.notes.push_back("transform controller not supported; identity used");
        if (!IsIdentity(n.transform.scaleAxis) || !IsIdentity(n.objectOffset.scaleAxis))
            n.notes.push_back("a scale axis other than identity is not applied");
        nodes.push_back(std::move(n));
    }
    // Parents as positions in `nodes` (the RootNode, or anything that is not a node: -1).
    for (size_t i = 0; i < nodes.size(); ++i)
        for (size_t j = 0; j < nodes.size(); ++j)
            if (parentIndex[i] >= 0 && nodes[j].index == static_cast<size_t>(parentIndex[i]) && j != i) nodes[i].parent = static_cast<int>(j);
    return nodes;
}

math::Mat4 LocalMatrix(const MaxTransform& t) {
    return math::Translation(t.position) * t.rotation.ToMat4() * math::Scaling(t.scale);
}

math::Mat4 WorldMatrix(const std::vector<MaxNode>& nodes, size_t i) {
    math::Mat4 m = LocalMatrix(nodes[i].transform);
    // Walk up the chain (bounded, so a damaged file's loop cannot hang it).
    int p = nodes[i].parent;
    for (size_t guard = 0; p >= 0 && guard < nodes.size(); ++guard) {
        m = LocalMatrix(nodes[static_cast<size_t>(p)].transform) * m;
        p = nodes[static_cast<size_t>(p)].parent;
    }
    return m;
}

math::Mat4 ObjectMatrix(const std::vector<MaxNode>& nodes, size_t i) {
    return WorldMatrix(nodes, i) * LocalMatrix(nodes[i].objectOffset);
}

} // namespace io
