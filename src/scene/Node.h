#pragma once
#include "math/Quat.h"
#include "scene/Object.h"
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

// A scene node: name, wireframe colour, place in the hierarchy, transform, and the
// object it shows (max-format.md §4, §5).
namespace scene {

// Position, rotation, scale, applied as scale, then rotation, then translation.
struct PRS {
    math::Vec3 position{0, 0, 0};
    math::Quat rotation{};
    math::Vec3 scale{1, 1, 1};

    math::Mat4 Matrix() const;   // Translation * Rotation * Scale
};

class Node {
public:
    using Id = uint64_t;   // stable for the node's life, never reused (undo refers to nodes by id)

    Node(Id id, std::string name) : id_(id), name_(std::move(name)) {}

    Id GetId() const { return id_; }

    const std::string& Name() const { return name_; }
    void SetName(std::string name) { name_ = std::move(name); }

    uint32_t WireColor() const { return wireColor_; }  // 0xRRGGBB
    void SetWireColor(uint32_t rgb) { wireColor_ = rgb; }
    // Groups (Phase 10): a group head is an object-less node its members are linked
    // under; while closed, clicking a member selects the whole group.
    bool IsGroupHead() const { return groupHead_; }
    bool GroupOpen() const { return groupOpen_; }
    void SetGroup(bool head, bool open) { groupHead_ = head; groupOpen_ = open; }
    // Object Properties' Backface Cull (11.4b): wireframe views leave out faces turned away;
    // on by default, as for every object in the R3.1 samples but one.
    bool BackfaceCull() const { return backfaceCull_; }
    void SetBackfaceCull(bool on) { backfaceCull_ = on; }
    // A material's diffuse colour (0xRRGGBB, 11.4b): shaded views use it instead of the wire
    // colour, as R3 does. maxlike has no materials beyond this colour yet.
    bool HasMaterialColor() const { return hasMaterial_; }
    uint32_t MaterialColor() const { return materialColor_; }
    void SetMaterialColor(bool has, uint32_t rgb = 0) { hasMaterial_ = has; materialColor_ = rgb; }

    // The transform relative to the parent (Max's PRS controller).
    PRS& Transform() { return transform_; }
    const PRS& Transform() const { return transform_; }

    // The object's offset from the node's pivot (Max's "object offset", changed by
    // Affect Pivot Only). Identity by default.
    PRS& ObjectOffset() { return objectOffset_; }
    const PRS& ObjectOffset() const { return objectOffset_; }

    Node* Parent() const { return parent_; }
    const std::vector<Node*>& Children() const { return children_; }

    const std::shared_ptr<Object>& GetObject() const { return object_; }
    void SetObject(std::shared_ptr<Object> object) { object_ = std::move(object); }

    math::Mat4 WorldMatrix() const;    // the pivot in world space: parent chain * transform
    math::Mat4 ObjectMatrix() const;   // object space -> world: WorldMatrix * object offset

private:
    friend class Scene;  // the scene keeps parent/child links consistent

    Id id_;
    std::string name_;
    uint32_t wireColor_ = 0x808080;
    bool backfaceCull_ = true;
    bool hasMaterial_ = false;
    uint32_t materialColor_ = 0;
    bool groupHead_ = false, groupOpen_ = false;
    PRS transform_;
    PRS objectOffset_;
    Node* parent_ = nullptr;
    std::vector<Node*> children_;
    std::shared_ptr<Object> object_;
};

} // namespace scene
