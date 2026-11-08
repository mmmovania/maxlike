#include "io/MaxScene.h"
#include "io/Utf16.h"

namespace io {

namespace {

int32_t I32(const std::vector<uint8_t>& d, size_t o) {
    return static_cast<int32_t>(static_cast<uint32_t>(d[o]) | (static_cast<uint32_t>(d[o + 1]) << 8) |
                                (static_cast<uint32_t>(d[o + 2]) << 16) | (static_cast<uint32_t>(d[o + 3]) << 24));
}

bool ReadChunkedStream(const CompoundFile& cf, const char* name, std::vector<uint8_t>& bytes, std::vector<Chunk>& chunks,
                       std::string& error) {
    if (!cf.ReadStream(name, bytes, error)) return false;
    if (!ParseChunks(bytes, 0, bytes.size(), chunks, error)) { error = std::string(name) + ": " + error; return false; }
    return true;
}

} // namespace

bool MaxScene::Open(std::vector<uint8_t> fileBytes, std::string& error) {
    classes_.clear(); dlls_.clear(); scene_.clear(); sceneChunks_.clear(); objects_.clear(); refs_.clear();
    CompoundFile cf;
    if (!cf.Open(std::move(fileBytes), error)) return false;

    // Class directory: one entry per class, in order; older files name it differently.
    std::vector<uint8_t> bytes;
    std::vector<Chunk> chunks;
    bool haveClasses = false;
    for (const char* name : {"ClassDirectory3", "ClassDirectory2", "ClassDirectory"}) {
        std::string ignored;
        if (ReadChunkedStream(cf, name, bytes, chunks, ignored)) { haveClasses = true; break; }
    }
    if (!haveClasses) { error = "no class directory"; return false; }
    for (const Chunk& entry : chunks) {
        if (entry.type != 0x2040) continue;
        ClassEntry c;
        for (const Chunk& k : entry.children) {
            if (k.type == 0x2060 && k.end - k.begin >= 16) {
                c.dll = I32(bytes, k.begin);
                c.classIdA = static_cast<uint32_t>(I32(bytes, k.begin + 4));
                c.classIdB = static_cast<uint32_t>(I32(bytes, k.begin + 8));
                c.superClassId = static_cast<uint32_t>(I32(bytes, k.begin + 12));
            } else if (k.type == 0x2042) {
                c.name = Utf16BytesToUtf8(bytes, k.begin, k.end);
            }
        }
        classes_.push_back(c);
    }

    // DLL directory (measured: 0x2039 = description, 0x2037 = file name).
    if (ReadChunkedStream(cf, "DllDirectory", bytes, chunks, error)) {
        for (const Chunk& entry : chunks) {
            if (entry.type != 0x2038) continue;
            DllEntry d;
            for (const Chunk& k : entry.children) {
                if (k.type == 0x2039) d.description = Utf16BytesToUtf8(bytes, k.begin, k.end);
                if (k.type == 0x2037) d.file = Utf16BytesToUtf8(bytes, k.begin, k.end);
            }
            dlls_.push_back(d);
        }
    } else {
        return false;
    }

    // Scene: one top-level container whose children are the scene list.
    if (!ReadChunkedStream(cf, "Scene", scene_, sceneChunks_, error)) return false;
    if (sceneChunks_.size() != 1 || !sceneChunks_[0].container) { error = "Scene: not one top-level container"; return false; }
    for (const Chunk& o : sceneChunks_[0].children) {
        objects_.push_back(&o);
        std::vector<int32_t> refs;
        for (const Chunk& k : o.children) {
            if (k.type == 0x2034) {
                for (size_t p = k.begin; p + 4 <= k.end; p += 4) refs.push_back(I32(scene_, p));
            } else if (k.type == 0x2035 && k.end - k.begin >= 4) {
                const int32_t slots = I32(scene_, k.begin);
                if (slots >= 0 && slots < 100000) refs.assign(static_cast<size_t>(slots), -1);
                for (size_t p = k.begin + 4; p + 8 <= k.end; p += 8) {
                    const int32_t slot = I32(scene_, p), index = I32(scene_, p + 4);
                    if (slot < 0 || slot >= 100000) continue;
                    if (static_cast<size_t>(slot) >= refs.size()) refs.resize(static_cast<size_t>(slot) + 1, -1);
                    refs[static_cast<size_t>(slot)] = index;
                }
            }
        }
        refs_.push_back(std::move(refs));
    }
    return true;
}

const ClassEntry* MaxScene::ClassOf(size_t i) const {
    const uint16_t t = Type(i);
    if (t == 0x2032 || t == 0x2033 || t >= classes_.size()) return nullptr;
    return &classes_[t];
}

std::vector<size_t> MaxScene::FindByClassName(const std::string& name) const {
    std::vector<size_t> out;
    for (size_t i = 0; i < objects_.size(); ++i)
        if (const ClassEntry* c = ClassOf(i); c && c->name == name) out.push_back(i);
    return out;
}

} // namespace io
