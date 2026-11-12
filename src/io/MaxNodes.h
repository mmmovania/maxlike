#pragma once
#include "io/MaxScene.h"
#include "math/Quat.h"
#include <string>
#include <vector>

// A .max file's nodes (docs/reference/max-format.md §4–§5): name, parent, wire colour,
// the transform from its Position/Rotation/Scale controller and its object offset.
// Rotations are converted from Max's left-handed form on reading (Quat::FromMaxFile).
// No Windows dependency (unit-tested in tests/test_max_nodes.cpp against the samples).
namespace io {

struct MaxTransform {
    math::Vec3 position{0, 0, 0};
    math::Quat rotation{};
    math::Vec3 scale{1, 1, 1};
    math::Quat scaleAxis{};          // Max's scale axis; only identity is applied (see notes)
};

// A Position/Rotation/Scale controller's value (Bezier Position, TCB Rotation, Bezier
// Scale; Max's quaternion convention converted), and a Bezier Position's (11.4: also a
// modifier gizmo's and centre's). False if the controller is not of that kind.
bool ReadPRSController(const MaxScene& s, int controller, MaxTransform& t, std::vector<std::string>& notes);
bool ReadPositionController(const MaxScene& s, int controller, math::Vec3& p, std::vector<std::string>& notes);

struct MaxNode {
    size_t index = 0;                // in the scene list
    std::string name;
    int parent = -1;                 // position in the ReadNodes result; -1 = under the root
    bool hasWireColor = false;
    uint8_t wire[3] = {0, 0, 0};     // R, G, B
    int controller = -1, object = -1;   // scene-list indices (reference slots 0 and 1)
    MaxTransform transform;          // relative to the parent (the PRS controller at frame 0)
    MaxTransform objectOffset;       // between the node and its geometry (the pivot)
    std::vector<std::string> notes;  // what could not be read or applied
};

std::vector<MaxNode> ReadNodes(const MaxScene& scene);

// Translation * Rotation * Scale (column vectors), as scene::PRS::Matrix.
math::Mat4 LocalMatrix(const MaxTransform& t);
// The node's pivot in world space (parent chain * local), and its object space in world
// space (world * object offset), as scene::Node::WorldMatrix / ObjectMatrix.
math::Mat4 WorldMatrix(const std::vector<MaxNode>& nodes, size_t i);
math::Mat4 ObjectMatrix(const std::vector<MaxNode>& nodes, size_t i);

} // namespace io
