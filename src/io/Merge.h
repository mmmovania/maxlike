#pragma once
#include "io/MaxImport.h"
#include <cstdint>
#include <string>
#include <vector>

// File > Merge (Phase 9): another file's objects (.max or .mlk) added to `scene`. Nothing
// changes if the file cannot be read. Names already in the scene get the next free number
// (R3 asks in a dialog; NOTE: not built), with a note. `added` lists the new nodes,
// parents before children (for scene::AddedNodesCommand). No Windows dependency.
namespace io {

bool MergeScene(const std::vector<uint8_t>& fileBytes, scene::Scene& scene, ImportReport& report,
                std::vector<scene::Node::Id>& added, std::string& error);

} // namespace io
