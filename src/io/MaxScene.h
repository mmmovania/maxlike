#pragma once
#include "io/Cfb.h"
#include "io/Chunks.h"
#include <cstdint>
#include <string>
#include <vector>

// A .max file's directories and scene list (docs/reference/max-format.md §2.2, §2.3, §3):
// the classes it uses, the plug-in DLLs, and every object in the Scene stream with its
// class and its references to other objects. No Windows dependency (unit-tested in
// tests/test_max_scene.cpp against the R3.1 samples).
namespace io {

struct ClassEntry {
    int32_t dll = -1;                   // index into the DLL directory; -1 = built into Max
    uint32_t classIdA = 0, classIdB = 0;
    uint32_t superClassId = 0;
    std::string name;
};

struct DllEntry {
    std::string description;            // chunk 0x2039 (e.g. "Standard primitive objects")
    std::string file;                   // chunk 0x2037 (e.g. "prim.dlo")
};

class MaxScene {
public:
    bool Open(std::vector<uint8_t> fileBytes, std::string& error);

    const std::vector<ClassEntry>& Classes() const { return classes_; }
    const std::vector<DllEntry>& Dlls() const { return dlls_; }

    // The scene list: object i is the i-th child of the Scene stream's top container.
    size_t ObjectCount() const { return objects_.size(); }
    uint16_t Type(size_t i) const { return objects_[i]->type; }   // a class index, or 0x2032 / 0x2033
    bool IsDerivedObject(size_t i) const { return Type(i) == 0x2032 || Type(i) == 0x2033; }
    const ClassEntry* ClassOf(size_t i) const;                   // null for Derived Objects or unknown types
    const Chunk& ObjectChunk(size_t i) const { return *objects_[i]; }
    const std::vector<uint8_t>& SceneBytes() const { return scene_; }

    // Object i's references, slot by slot (-1 = empty), from chunk 0x2034 (positional) or
    // 0x2035 (the slot count, then (slot, index) pairs for the filled slots).
    const std::vector<int32_t>& Refs(size_t i) const { return refs_[i]; }

    // Objects whose class has this name, in scene-list order.
    std::vector<size_t> FindByClassName(const std::string& name) const;

private:
    std::vector<ClassEntry> classes_;
    std::vector<DllEntry> dlls_;
    std::vector<uint8_t> scene_;
    std::vector<Chunk> sceneChunks_;
    std::vector<const Chunk*> objects_;
    std::vector<std::vector<int32_t>> refs_;
};

} // namespace io
