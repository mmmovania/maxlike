#include "scene/Creation.h"
#include "scene/Primitives.h"
#include <cmath>

namespace scene {

PRS ConstructionPlane::NodeTransform(double pu, double pv) const {
    PRS t;
    t.position = ToWorld(pu, pv);
    // Rotation whose columns are u, v, normal -> quaternion.
    const math::Vec3 X = u, Y = v, Z = normal;
    const double tr = X.x + Y.y + Z.z;
    math::Quat q;
    if (tr > 0) {
        const double s = std::sqrt(tr + 1.0) * 2;
        q = {(Y.z - Z.y) / s, (Z.x - X.z) / s, (X.y - Y.x) / s, 0.25 * s};
    } else if (X.x > Y.y && X.x > Z.z) {
        const double s = std::sqrt(1.0 + X.x - Y.y - Z.z) * 2;
        q = {0.25 * s, (Y.x + X.y) / s, (Z.x + X.z) / s, (Y.z - Z.y) / s};
    } else if (Y.y > Z.z) {
        const double s = std::sqrt(1.0 + Y.y - X.x - Z.z) * 2;
        q = {(Y.x + X.y) / s, 0.25 * s, (Z.y + Y.z) / s, (Z.x - X.z) / s};
    } else {
        const double s = std::sqrt(1.0 + Z.z - X.x - Y.y) * 2;
        q = {(Z.x + X.z) / s, (Z.y + Y.z) / s, 0.25 * s, (X.y - Y.x) / s};
    }
    t.rotation = q.Normalized();
    return t;
}

ConstructionPlane TopPlane() { return {{1, 0, 0}, {0, 1, 0}, {0, 0, 1}}; }
ConstructionPlane FrontPlane() { return {{1, 0, 0}, {0, 0, 1}, {0, -1, 0}}; }
ConstructionPlane LeftPlane() { return {{0, -1, 0}, {0, 0, 1}, {-1, 0, 0}}; }

Creator::Creator(Primitive p, double unitsPerPixel) : primitive_(p), unitsPerPixel_(unitsPerPixel) {
    using K = Kind;
    switch (p) {
    case Primitive::Box:
    case Primitive::Pyramid:   steps_ = {K::DragRect, K::MoveHeight}; break;
    case Primitive::Plane:     steps_ = {K::DragRect}; break;
    case Primitive::Sphere:
    case Primitive::GeoSphere:
    case Primitive::Teapot:    steps_ = {K::DragRadius}; break;
    case Primitive::Cylinder:  steps_ = {K::DragRadius, K::MoveHeight}; break;
    case Primitive::Cone:      steps_ = {K::DragRadius, K::MoveHeight, K::MoveRadius2}; break;
    case Primitive::Tube:      steps_ = {K::DragRadius, K::MoveRadius2, K::MoveHeight}; break;
    case Primitive::Torus:     steps_ = {K::DragRadius, K::MoveRadius2}; break;
    }
}

void Creator::Set(size_t index, double value) {
    for (auto& [i, v] : params_)
        if (i == index) { v = value; return; }
    params_.push_back({index, value});
}

void Creator::Feed(Event e, double u, double v, int screenY) {
    if (state_ == State::Done || state_ == State::Cancelled) return;
    if (state_ == State::Waiting) {
        if (e != Event::Press) return;
        state_ = State::Running;
        au_ = cu_ = u;
        av_ = cv_ = v;
        pressU_ = u; pressV_ = v; pressY_ = screenY;
        stepStartY_ = screenY;
        Update(u, v, screenY);
        return;
    }
    const Kind k = steps_[static_cast<size_t>(step_)];
    const bool drag = k == Kind::DragRect || k == Kind::DragRadius;
    if (e == Event::Move) { Update(u, v, screenY); return; }
    if (drag && e == Event::Release) {
        Update(u, v, screenY);
        // A click without a drag creates nothing (as in Max).
        const double moved = std::hypot(u - pressU_, v - pressV_) / unitsPerPixel_;
        if (moved < kMinDragPixels && std::abs(screenY - pressY_) < kMinDragPixels) { state_ = State::Cancelled; return; }
        stepStartY_ = screenY;
        Advance();
        return;
    }
    if (!drag && e == Event::Press) {
        Update(u, v, screenY);
        Advance();
    }
}

void Creator::Advance() {
    if (++step_ >= static_cast<int>(steps_.size())) state_ = State::Done;
}

// Parameters for the current step (indices from Primitives.h).
void Creator::Update(double u, double v, int screenY) {
    const Kind k = steps_[static_cast<size_t>(step_)];
    const double radius = std::hypot(u - au_, v - av_);
    const double rise = (stepStartY_ - screenY) * unitsPerPixel_;   // mouse up = positive
    switch (k) {
    case Kind::DragRect: {
        // Corner to corner; the pivot is the base's centre, sizes positive.
        cu_ = (au_ + u) / 2;
        cv_ = (av_ + v) / 2;
        const double w = std::fabs(u - au_), l = std::fabs(v - av_);
        if (primitive_ == Primitive::Box) {
            Set(BoxObject::kWidth, w); Set(BoxObject::kLength, l); Set(BoxObject::kHeight, 0);
        } else if (primitive_ == Primitive::Pyramid) {
            Set(PyramidObject::kWidth, w); Set(PyramidObject::kDepth, l); Set(PyramidObject::kHeight, 0);
        } else {
            Set(PlaneObject::kWidth, w); Set(PlaneObject::kLength, l);
        }
        break;
    }
    case Kind::DragRadius:
        switch (primitive_) {
        case Primitive::Sphere:    Set(SphereObject::kRadius, radius); break;
        case Primitive::GeoSphere: Set(GeoSphereObject::kRadius, radius); break;
        case Primitive::Teapot:    Set(TeapotObject::kRadius, radius); break;
        case Primitive::Cylinder:  Set(CylinderObject::kRadius, radius); Set(CylinderObject::kHeight, 0); break;
        case Primitive::Cone:      // NOTE: radius 2 starts at 0 until its own step
            Set(ConeObject::kRadius1, radius); Set(ConeObject::kRadius2, 0); Set(ConeObject::kHeight, 0); break;
        case Primitive::Tube:      // radius 2 follows radius 1 until its own step
            Set(TubeObject::kRadius1, radius); Set(TubeObject::kRadius2, radius); Set(TubeObject::kHeight, 0); break;
        case Primitive::Torus:     // NOTE: the tube radius before its step is unverified
            Set(TorusObject::kRadius1, radius); Set(TorusObject::kRadius2, 0); break;
        default: break;
        }
        break;
    case Kind::MoveHeight:
        switch (primitive_) {
        case Primitive::Box:      Set(BoxObject::kHeight, rise); break;
        case Primitive::Pyramid:  Set(PyramidObject::kHeight, rise); break;
        case Primitive::Cylinder: Set(CylinderObject::kHeight, rise); break;
        case Primitive::Cone:     Set(ConeObject::kHeight, rise); break;
        case Primitive::Tube:     Set(TubeObject::kHeight, rise); break;
        default: break;
        }
        break;
    case Kind::MoveRadius2:
        // NOTE: R3 may use vertical mouse movement here; the distance from the centre
        // is used for now.
        switch (primitive_) {
        case Primitive::Cone:  Set(ConeObject::kRadius2, radius); break;
        case Primitive::Tube:  Set(TubeObject::kRadius2, radius); break;
        case Primitive::Torus: {
            // The tube's radius is the distance from the ring circle (radius 1).
            double r1 = 0;
            for (const auto& [i, val] : params_) if (i == TorusObject::kRadius1) r1 = val;
            Set(TorusObject::kRadius2, std::fabs(radius - r1));
            break;
        }
        default: break;
        }
        break;
    }
}

} // namespace scene
