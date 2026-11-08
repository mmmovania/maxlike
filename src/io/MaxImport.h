#pragma once
#include "scene/Scene.h"
#include <cstdint>
#include <string>
#include <vector>

// Builds a .max file's scene into a scene::Scene (Phase 8): nodes with their names,
// hierarchy, wire colours, transforms and object offsets, and their objects (primitives,
// Editable Meshes, Derived Objects with Edit Mesh), shared where the file shares them.
// No Windows dependency (unit-tested in tests/test_max_import.cpp against the samples).
namespace io {

struct ImportReport {
    size_t nodes = 0;                 // nodes created
    size_t objects = 0;               // distinct objects (instances count once)
    std::vector<std::string> notes;   // what was skipped or approximated, per node
};

// Adds the file's nodes to `scene` (normally empty). False and `error` if the file
// cannot be read at all; otherwise every node is created, with notes for what could not
// be imported (a node whose object cannot be read is created without one).
bool ImportMax(const std::vector<uint8_t>& fileBytes, scene::Scene& scene, ImportReport& report, std::string& error);

} // namespace io
