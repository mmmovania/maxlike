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
    explicit Node(std::string name) : name_(std::move(name)) {}

    const std::string& Name() const { return name_; }
    void SetName(std::string name) { name_ = std::move(name); }

    uint32_t WireColor() const { return wireColor_; }  // 0xRRGGBB
    void SetWireColor(uint32_t rgb) { wireColor_ = rgb; }

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

    std::string name_;
    uint32_t wireColor_ = 0x808080;
    PRS transform_;
    PRS objectOffset_;
    Node* parent_ = nullptr;
    std::vector<Node*> children_;
    std::shared_ptr<Object> object_;
};

} // namespace scene
