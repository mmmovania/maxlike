#include "scene/Node.h"

namespace scene {

math::Mat4 PRS::Matrix() const {
    return math::Translation(position) * rotation.ToMat4() * math::Scaling(scale);
}

math::Mat4 Node::WorldMatrix() const {
    const math::Mat4 local = transform_.Matrix();
    return parent_ ? parent_->WorldMatrix() * local : local;
}

math::Mat4 Node::ObjectMatrix() const {
    return WorldMatrix() * objectOffset_.Matrix();
}

} // namespace scene
