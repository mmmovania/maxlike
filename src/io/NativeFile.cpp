#include "io/NativeFile.h"
#include "io/Chunks.h"
#include "scene/CreationTool.h"
#include "scene/Modifier.h"
#include "scene/Primitives.h"
#include <algorithm>
#include <cstring>
#include <functional>
#include <map>

namespace io {

namespace {

constexpr char kMagic[8] = {'M', 'A', 'X', 'L', 'I', 'K', 'E', 0};
constexpr uint32_t kVersion = 1;

// ---- writing ------------------------------------------------------------------------

class Writer {
public:
    std::vector<uint8_t> bytes;
    template <typename T> void Put(T v) { const auto* p = reinterpret_cast<const uint8_t*>(&v); bytes.insert(bytes.end(), p, p + sizeof(T)); }
    void Text(const std::string& s) { bytes.insert(bytes.end(), s.begin(), s.end()); }
    // A chunk: header now, size patched when it ends.
    size_t Begin(uint16_t type) { Put(type); Put(uint32_t{0}); return bytes.size() - 6; }
    void End(size_t at, bool container) {
        const uint32_t size = static_cast<uint32_t>(bytes.size() - at) | (container ? 0x80000000u : 0);
        std::memcpy(&bytes[at + 2], &size, 4);
    }
    template <typename F> void Leaf(uint16_t type, F body) { const size_t at = Begin(type); body(); End(at, false); }
    template <typename F> void Container(uint16_t type, F body) { const size_t at = Begin(type); body(); End(at, true); }
};

void PutParams(Writer& w, const scene::ParamBlock& p) {
    w.Leaf(0x0020, [&] {
        uint32_t n = 0;
        for (size_t i = 0; i < p.Count(); ++i) n += p.Has(i);
        w.Put(n);
        for (size_t i = 0; i < p.Count(); ++i)
            if (p.Has(i)) { w.Put(static_cast<uint32_t>(i)); w.Put(p.Float(i)); }
    });
}

void PutPrs(Writer& w, uint16_t type, const scene::PRS& t) {
    w.Leaf(type, [&] {
        for (double v : {t.position.x, t.position.y, t.position.z, t.rotation.x, t.rotation.y, t.rotation.z, t.rotation.w,
                         t.scale.x, t.scale.y, t.scale.z})
            w.Put(v);
    });
}

int PrimitiveType(const scene::Object& o) {
    for (int t = 0; t < 10; ++t)
        if (o.ClassName() == scene::PrimitiveBaseName(t)) return t;
    return -1;
}

// ---- reading ------------------------------------------------------------------------

template <typename T> bool Get(const std::vector<uint8_t>& d, size_t& at, size_t end, T& v) {
    if (end < at || end - at < sizeof(T)) return false;
    std::memcpy(&v, &d[at], sizeof(T));
    at += sizeof(T);
    return true;
}

const Chunk* Child(const Chunk& c, uint16_t type) {
    for (const Chunk& k : c.children) if (k.type == type) return &k;
    return nullptr;
}

bool GetParams(const std::vector<uint8_t>& d, const Chunk* k, scene::ParamBlock& p) {
    if (!k) return true;
    size_t at = k->begin;
    uint32_t n = 0;
    if (!Get(d, at, k->end, n)) return false;
    for (uint32_t i = 0; i < n; ++i) {
        uint32_t index; double value;
        if (!Get(d, at, k->end, index) || !Get(d, at, k->end, value)) return false;
        if (p.Has(index)) p.Set(index, value);
    }
    return true;
}

bool GetPrs(const std::vector<uint8_t>& d, const Chunk* k, scene::PRS& t) {
    if (!k) return true;
    size_t at = k->begin;
    double v[10];
    for (double& x : v) if (!Get(d, at, k->end, x)) return false;
    t.position = {v[0], v[1], v[2]};
    t.rotation = {v[3], v[4], v[5], v[6]};
    t.scale = {v[7], v[8], v[9]};
    return true;
}

std::string Str(const std::vector<uint8_t>& d, const Chunk* k) {
    return k ? std::string(d.begin() + static_cast<std::ptrdiff_t>(k->begin), d.begin() + static_cast<std::ptrdiff_t>(k->end)) : std::string();
}

} // namespace

bool IsNativeFile(const std::vector<uint8_t>& bytes) {
    return bytes.size() >= 12 && std::memcmp(bytes.data(), kMagic, 8) == 0;
}

std::vector<uint8_t> SaveNative(const scene::Scene& scene) {
    std::vector<scene::Node::Id> all;
    for (const auto& n : scene.Nodes()) all.push_back(n->GetId());
    return SaveNative(scene, all);
}

std::vector<uint8_t> SaveNative(const scene::Scene& scene, const std::vector<scene::Node::Id>& only) {
    std::vector<const scene::Node*> saved;
    for (const auto& n : scene.Nodes())
        if (std::find(only.begin(), only.end(), n->GetId()) != only.end()) saved.push_back(n.get());
    Writer w;
    w.bytes.insert(w.bytes.end(), kMagic, kMagic + 8);
    w.Put(kVersion);
    // Objects, each once (bases before the Derived Objects that use them).
    std::map<const scene::Object*, int32_t> index;
    std::vector<const scene::Object*> order;
    std::function<void(const scene::Object*)> visit = [&](const scene::Object* o) {
        if (!o || index.count(o)) return;
        if (auto* d = dynamic_cast<const scene::DerivedObject*>(o)) visit(d->Base().get());
        index[o] = static_cast<int32_t>(order.size());
        order.push_back(o);
    };
    for (const scene::Node* n : saved) visit(n->GetObject().get());
    w.Container(0x1100, [&] {
        for (const scene::Object* o : order)
            w.Container(0x1110, [&] {
                if (auto* d = dynamic_cast<const scene::DerivedObject*>(o)) {
                    w.Leaf(0x0001, [&] { w.Put(uint32_t{3}); });
                    w.Leaf(0x0040, [&] { w.Put(static_cast<uint32_t>(index.at(d->Base().get()))); });
                    w.Container(0x0041, [&] {
                        for (const auto& m : d->Modifiers())
                            w.Container(0x0042, [&] {
                                w.Leaf(0x0001, [&] { w.Text(m->ClassName()); });
                                w.Leaf(0x0002, [&] { w.Put(static_cast<uint8_t>(m->Enabled())); });
                                PutParams(w, m->Params());
                                if (auto* em = dynamic_cast<const scene::EditMeshModifier*>(m.get())) {
                                    w.Leaf(0x0050, [&] { w.Put(static_cast<uint64_t>(em->ExpectedVertexCount())); });
                                    w.Leaf(0x0051, [&] {
                                        w.Put(static_cast<uint32_t>(em->Moves().size()));
                                        for (const auto& mv : em->Moves()) { w.Put(mv.vertex); w.Put(mv.delta.x); w.Put(mv.delta.y); w.Put(mv.delta.z); }
                                    });
                                    auto list = [&](uint16_t type, const std::vector<uint32_t>& v) {
                                        w.Leaf(type, [&] { w.Put(static_cast<uint32_t>(v.size())); for (uint32_t x : v) w.Put(x); });
                                    };
                                    list(0x0052, em->SelectedVertices());
                                    list(0x0053, em->SelectedFaces());      // Phase 9.4
                                    list(0x0054, em->DeletedFaces());
                                    list(0x0055, em->DeletedVertices());
                                }
                            });
                    });
                } else if (const int type = PrimitiveType(*o); type >= 0) {
                    w.Leaf(0x0001, [&] { w.Put(uint32_t{1}); });
                    w.Leaf(0x0010, [&] { w.Put(static_cast<uint32_t>(type)); });
                    PutParams(w, o->Params());
                } else {
                    const scene::Mesh m = o->Evaluate();       // an Editable Mesh (or anything else, as a mesh)
                    w.Leaf(0x0001, [&] { w.Put(uint32_t{2}); });
                    w.Leaf(0x0030, [&] {
                        w.Put(static_cast<uint32_t>(m.verts.size()));
                        for (const auto& v : m.verts) { w.Put(v.x); w.Put(v.y); w.Put(v.z); }
                    });
                    w.Leaf(0x0031, [&] {
                        w.Put(static_cast<uint32_t>(m.faces.size()));
                        for (const auto& f : m.faces) {
                            w.Put(f.v[0]); w.Put(f.v[1]); w.Put(f.v[2]);
                            w.Put(f.smoothing); w.Put(f.materialId); w.Put(f.edgeVisible); w.Put(uint8_t{0});
                        }
                    });
                }
            });
    });
    // Nodes, with parents as positions in this list.
    std::map<const scene::Node*, int32_t> nodeIndex;
    for (size_t i = 0; i < saved.size(); ++i) nodeIndex[saved[i]] = static_cast<int32_t>(i);
    w.Container(0x1200, [&] {
        for (const scene::Node* n : saved)
            w.Container(0x1210, [&] {
                const bool parentSaved = n->Parent() && nodeIndex.count(n->Parent());
                w.Leaf(0x0001, [&] { w.Text(n->Name()); });
                w.Leaf(0x0002, [&] { w.Put(n->WireColor()); });
                w.Leaf(0x0003, [&] { w.Put(parentSaved ? nodeIndex.at(n->Parent()) : int32_t{-1}); });
                w.Leaf(0x0004, [&] { w.Put(n->GetObject() ? index.at(n->GetObject().get()) : int32_t{-1}); });
                if (parentSaved || !n->Parent()) {
                    PutPrs(w, 0x0005, n->Transform());
                } else {
                    // Its parent is left out: its world transform becomes its own,
                    // decomposed by the scene's own rule on a scratch top-level node.
                    scene::Scene scratch;
                    scene::Node* t = scratch.CreateNode("t");
                    scene::Scene::SetWorldMatrix(t, n->WorldMatrix());
                    PutPrs(w, 0x0005, t->Transform());
                }
                PutPrs(w, 0x0006, n->ObjectOffset());
            });
    });
    return w.bytes;
}

bool LoadNative(const std::vector<uint8_t>& d, scene::Scene& scene, std::string& error) {
    if (!IsNativeFile(d)) { error = "not a maxlike scene file"; return false; }
    uint32_t version = 0;
    std::memcpy(&version, &d[8], 4);
    if (version != kVersion) { error = "maxlike scene file version " + std::to_string(version) + " is not supported"; return false; }
    std::vector<Chunk> top;
    if (!ParseChunks(d, 12, d.size(), top, error)) return false;
    const Chunk* objectsChunk = nullptr;
    const Chunk* nodesChunk = nullptr;
    for (const Chunk& c : top) {
        if (c.type == 0x1100 && c.container) objectsChunk = &c;
        if (c.type == 0x1200 && c.container) nodesChunk = &c;
    }
    if (!objectsChunk || !nodesChunk) { error = "the file has no objects or nodes"; return false; }
    auto bad = [&](const std::string& what) { error = what; return false; };

    std::vector<std::shared_ptr<scene::Object>> objects;
    for (const Chunk& o : objectsChunk->children) {
        if (o.type != 0x1110) continue;
        uint32_t kind = 0;
        if (const Chunk* k = Child(o, 0x0001)) { size_t at = k->begin; Get(d, at, k->end, kind); }
        std::shared_ptr<scene::Object> obj;
        if (kind == 1) {
            uint32_t type = 99;
            if (const Chunk* k = Child(o, 0x0010)) { size_t at = k->begin; Get(d, at, k->end, type); }
            obj = type < 10 ? scene::MakePrimitive(static_cast<int>(type)) : nullptr;
            if (!obj) return bad("an unknown primitive type");
            if (!GetParams(d, Child(o, 0x0020), obj->Params())) return bad("damaged parameters");
        } else if (kind == 2) {
            scene::Mesh m;
            const Chunk* vk = Child(o, 0x0030);
            const Chunk* fk = Child(o, 0x0031);
            if (!vk || !fk) return bad("a mesh without vertices or faces");
            size_t at = vk->begin;
            uint32_t n = 0;
            if (!Get(d, at, vk->end, n) || static_cast<uint64_t>(n) * 24 > vk->end - at) return bad("damaged vertices");
            for (uint32_t i = 0; i < n; ++i) { math::Vec3 v; Get(d, at, vk->end, v.x); Get(d, at, vk->end, v.y); Get(d, at, vk->end, v.z); m.verts.push_back(v); }
            at = fk->begin;
            if (!Get(d, at, fk->end, n) || static_cast<uint64_t>(n) * 20 > fk->end - at) return bad("damaged faces");
            for (uint32_t i = 0; i < n; ++i) {
                scene::Face f; uint8_t pad = 0;
                Get(d, at, fk->end, f.v[0]); Get(d, at, fk->end, f.v[1]); Get(d, at, fk->end, f.v[2]);
                Get(d, at, fk->end, f.smoothing); Get(d, at, fk->end, f.materialId); Get(d, at, fk->end, f.edgeVisible); Get(d, at, fk->end, pad);
                for (uint32_t v : f.v) if (v >= m.verts.size()) return bad("a face refers to a missing vertex");
                m.faces.push_back(f);
            }
            obj = std::make_shared<scene::MeshObject>(std::move(m));
        } else if (kind == 3) {
            uint32_t base = 0;
            const Chunk* bk = Child(o, 0x0040);
            size_t at = bk ? bk->begin : 0;
            if (!bk || !Get(d, at, bk->end, base) || base >= objects.size()) return bad("a modified object without its base");
            auto derived = std::make_shared<scene::DerivedObject>(objects[base]);
            if (const Chunk* mods = Child(o, 0x0041))
                for (const Chunk& mk : mods->children) {
                    if (mk.type != 0x0042) continue;
                    const std::string cls = Str(d, Child(mk, 0x0001));
                    std::shared_ptr<scene::Modifier> mod;
                    if (cls == "Bend") mod = std::make_shared<scene::BendModifier>();
                    else if (cls == "Taper") mod = std::make_shared<scene::TaperModifier>();
                    else if (cls == "Twist") mod = std::make_shared<scene::TwistModifier>();
                    else if (cls == "Edit Mesh") {
                        uint64_t expected = 0;
                        std::vector<scene::EditMeshModifier::Move> moves;
                        std::vector<uint32_t> selected;
                        if (const Chunk* k = Child(mk, 0x0050)) { size_t p = k->begin; Get(d, p, k->end, expected); }
                        if (const Chunk* k = Child(mk, 0x0051)) {
                            size_t p = k->begin; uint32_t n = 0;
                            if (!Get(d, p, k->end, n) || static_cast<uint64_t>(n) * 28 > k->end - p) return bad("damaged Edit Mesh moves");
                            for (uint32_t i = 0; i < n; ++i) { scene::EditMeshModifier::Move mv; Get(d, p, k->end, mv.vertex); Get(d, p, k->end, mv.delta.x); Get(d, p, k->end, mv.delta.y); Get(d, p, k->end, mv.delta.z); moves.push_back(mv); }
                        }
                        auto readList = [&](uint16_t type, std::vector<uint32_t>& out) {
                            const Chunk* k = Child(mk, type);
                            if (!k) return true;
                            size_t p = k->begin; uint32_t n = 0;
                            if (!Get(d, p, k->end, n) || static_cast<uint64_t>(n) * 4 > k->end - p) return false;
                            for (uint32_t i = 0; i < n; ++i) { uint32_t v = 0; Get(d, p, k->end, v); out.push_back(v); }
                            return true;
                        };
                        scene::EditMeshModifier::State state;
                        if (!readList(0x0052, selected) || !readList(0x0053, state.selectedFaces) ||
                            !readList(0x0054, state.deletedFaces) || !readList(0x0055, state.deletedVertices)) return bad("damaged Edit Mesh data");
                        auto em = std::make_shared<scene::EditMeshModifier>(static_cast<size_t>(expected), moves);
                        state.moves = moves;
                        state.selectedVertices = selected;
                        em->SetState(state);
                        mod = em;
                    } else {
                        return bad("an unknown modifier \"" + cls + "\"");
                    }
                    if (!GetParams(d, Child(mk, 0x0020), mod->Params())) return bad("damaged modifier parameters");
                    if (const Chunk* k = Child(mk, 0x0002)) { size_t p = k->begin; uint8_t on = 1; Get(d, p, k->end, on); mod->SetEnabled(on != 0); }
                    derived->AddModifier(mod);
                }
            obj = derived;
        } else {
            return bad("an unknown object kind");
        }
        objects.push_back(obj);
    }

    // Nodes: created, then linked while every transform is identity (Scene::Link keeps
    // world positions), then their transforms set — as io::ImportMax does.
    struct Pending { scene::Node* node; int32_t parent; scene::PRS transform, offset; };
    std::vector<Pending> nodes;
    for (const Chunk& nk : nodesChunk->children) {
        if (nk.type != 0x1210) continue;
        Pending p{scene.CreateNode(Str(d, Child(nk, 0x0001))), -1, {}, {}};
        if (const Chunk* k = Child(nk, 0x0002)) { size_t at = k->begin; uint32_t c = 0; if (Get(d, at, k->end, c)) p.node->SetWireColor(c); }
        if (const Chunk* k = Child(nk, 0x0003)) { size_t at = k->begin; Get(d, at, k->end, p.parent); }
        if (const Chunk* k = Child(nk, 0x0004)) {
            size_t at = k->begin; int32_t o = -1;
            if (Get(d, at, k->end, o) && o >= 0 && static_cast<size_t>(o) < objects.size()) p.node->SetObject(objects[static_cast<size_t>(o)]);
        }
        if (!GetPrs(d, Child(nk, 0x0005), p.transform) || !GetPrs(d, Child(nk, 0x0006), p.offset)) return bad("damaged transforms");
        nodes.push_back(p);
    }
    for (const Pending& p : nodes)
        if (p.parent >= 0 && static_cast<size_t>(p.parent) < nodes.size()) scene.Link(p.node, nodes[static_cast<size_t>(p.parent)].node);
    for (const Pending& p : nodes) { p.node->Transform() = p.transform; p.node->ObjectOffset() = p.offset; }
    return true;
}

} // namespace io
