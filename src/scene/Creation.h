#pragma once
#include "math/Quat.h"
#include "scene/Node.h"
#include <cstddef>
#include <utility>
#include <vector>

// Interactive creation as pure state machines: mouse steps -> primitive parameters.
// No Windows dependency (unit-tested in tests/test_creation.cpp).
//
// Each primitive is built in one to three steps, as in R3 (from memory; NOTE: the
// exact sequences are to be verified against R3.1):
//   Box, Pyramid     drag the base (corner to corner), move for the height, click
//   Plane            drag the rectangle
//   Sphere, GeoSphere, Teapot   drag the radius from the centre
//   Cylinder         drag the radius, move for the height, click
//   Cone             drag radius 1, move for the height, click, move for radius 2, click
//   Tube             drag radius 1, move for radius 2, click, move for the height, click
//   Torus            drag radius 1, move for radius 2, click
// Drags end when the button is released; moves end with a click (button press).
namespace scene {

enum class Primitive { Box, Cone, Sphere, GeoSphere, Cylinder, Tube, Torus, Pyramid, Teapot, Plane };

// The construction plane of a view: points are (u, v) in the plane; the object's local
// Z is the plane's normal (towards the viewer in ortho views).
struct ConstructionPlane {
    math::Vec3 u{1, 0, 0}, v{0, 1, 0}, normal{0, 0, 1};   // orthonormal, right-handed

    math::Vec3 ToWorld(double pu, double pv) const { return u * pu + v * pv; }
    // A node standing on the plane at (pu, pv): local X = u, Y = v, Z = normal.
    PRS NodeTransform(double pu, double pv) const;
};

// Planes of the standard views (home grid). Top/Bottom/Perspective: XY; Front/Back: XZ;
// Left/Right: YZ; local Z faces the viewer in each ortho view.
ConstructionPlane TopPlane();     // u = +X, v = +Y, normal +Z
ConstructionPlane FrontPlane();   // u = +X, v = +Z, normal -Y
ConstructionPlane LeftPlane();    // u = -Y, v = +Z, normal -X

class Creator {
public:
    enum class Event { Press, Move, Release };

    // `unitsPerPixel`: world units per screen pixel, to turn vertical mouse movement
    // into height (moving up = positive).
    Creator(Primitive primitive, double unitsPerPixel);

    // Feed a mouse event at plane point (u, v) and screen row `screenY`.
    void Feed(Event e, double u, double v, int screenY);
    void Cancel() { state_ = State::Cancelled; }   // right-click or Esc

    bool Started() const { return state_ != State::Waiting && state_ != State::Cancelled; }
    bool Done() const { return state_ == State::Done; }
    bool Cancelled() const { return state_ == State::Cancelled; }
    int  Step() const { return step_; }             // 0-based, for prompts

    // The object so far: parameters by Max index, and where its pivot sits on the plane.
    const std::vector<std::pair<size_t, double>>& Params() const { return params_; }
    double CenterU() const { return cu_; }
    double CenterV() const { return cv_; }
    Primitive GetPrimitive() const { return primitive_; }

    // A press-and-release closer than this (pixels) cancels instead of creating, as a
    // click without a drag does in Max.
    static constexpr int kMinDragPixels = 2;

private:
    enum class State { Waiting, Running, Done, Cancelled };
    enum class Kind { DragRect, DragRadius, MoveHeight, MoveRadius2 };

    void Update(double u, double v, int screenY);
    void Set(size_t index, double value);
    void Advance();

    Primitive primitive_;
    double unitsPerPixel_;
    std::vector<Kind> steps_;
    State state_ = State::Waiting;
    int step_ = 0;
    double au_ = 0, av_ = 0;        // anchor: press point (rect corner, or circle centre)
    double cu_ = 0, cv_ = 0;        // pivot on the plane
    int stepStartY_ = 0;            // screen row when the current move step began
    int pressY_ = 0;
    double pressU_ = 0, pressV_ = 0;
    std::vector<std::pair<size_t, double>> params_;
};

} // namespace scene
