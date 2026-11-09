#pragma once
#include "scene/Scene.h"
#include <cstdint>
#include <string>
#include <vector>

// maxlike's own scene files (.mlk; Phase 8): the scene's nodes and objects, in the same
// chunk structure as .max files (io::ParseChunks reads them back). Objects are stored once
// and referenced by index, so instances stay shared. Values are doubles: a save and load
// round trip is exact. No Windows dependency (tests/test_native_file.cpp).
//
//   "MAXLIKE\0" (8 bytes), u32 format version (1), then chunks:
//   0x1100 objects: 0x1110 object { 0x0001 u32 kind (1 primitive, 2 mesh, 3 derived);
//     primitive: 0x0010 u32 type, 0x0020 params; mesh: 0x0030 vertices, 0x0031 faces;
//     derived: 0x0040 u32 base, 0x0041 { 0x0042 modifier { 0x0001 class, 0x0002 u8 on,
//     0x0020 params, Bend / Taper / Twist (Phase 10.3): 0x0060 gizmo transform, 0x0061
//     f64 centre[3]; Edit Mesh: 0x0050 u64 count, 0x0051 moves, 0x0052 selection,
//     Phase 9: 0x0053 selected faces, 0x0054 deleted faces, 0x0055 deleted vertices,
//     0x0056 selected edges, 0x0057 hidden faces, 0x0058 hidden vertices (each u32 n,
//     n x u32), 0x0059 welds (u32 n, n x (u32 from, u32 to)) } } }. Every Edit Mesh
//     index refers to the mesh coming into it; older files simply lack the newer chunks.
//   0x1200 nodes: 0x1210 node { 0x0001 name, 0x0002 u32 wire colour, 0x0003 i32 parent,
//     0x0004 i32 object, 0x0005 transform, 0x0006 object offset,
//     Phase 10: 0x0007 u8 group flags (1 group head, 2 open) }
//   params: u32 n, n x (u32 index, f64 value); transforms: f64 position[3], rotation[4]
//   (x, y, z, w), scale[3].
namespace io {

inline constexpr char kNativeExtension[] = ".mlk";

bool IsNativeFile(const std::vector<uint8_t>& bytes);
std::vector<uint8_t> SaveNative(const scene::Scene& scene);
// Only the nodes `only` (File > Save Selected): a node whose parent is not saved becomes
// top-level with its world transform, so it stays where it is.
std::vector<uint8_t> SaveNative(const scene::Scene& scene, const std::vector<scene::Node::Id>& only);
// Adds the file's nodes to `scene` (normally empty). False and `error` if it cannot be read.
bool LoadNative(const std::vector<uint8_t>& bytes, scene::Scene& scene, std::string& error);

} // namespace io
