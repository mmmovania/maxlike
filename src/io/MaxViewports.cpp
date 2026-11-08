#include "io/MaxViewports.h"
#include "io/Cfb.h"
#include "io/Chunks.h"
#include <cmath>
#include <cstring>

namespace io {

bool ReadViewports(const std::vector<uint8_t>& bytes, std::vector<MaxViewport>& out, std::string& error) {
    out.clear();
    CompoundFile cf;
    if (!cf.Open(bytes, error)) return false;
    std::vector<uint8_t> s;
    std::string ignored;
    if (!cf.ReadStream("Config", s, ignored)) return true;            // no saved viewports
    std::vector<Chunk> chunks;
    if (!ParseChunks(s, 0, s.size(), chunks, ignored)) return true;
    for (const Chunk& c : chunks) {
        if (c.type != 0x2080 || !c.container) continue;
        for (const Chunk& k : c.children) {
            if (k.type < 0x2810 || k.type > 0x2813 || k.end - k.begin < 64) continue;
            auto f32 = [&](size_t o) { float v; std::memcpy(&v, &s[k.begin + o], 4); return static_cast<double>(v); };
            auto i32 = [&](size_t o) { int32_t v; std::memcpy(&v, &s[k.begin + o], 4); return v; };
            MaxViewport v;
            v.slot = k.type - 0x2810;
            v.type = i32(4);
            for (int r = 0; r < 4; ++r)
                for (int col = 0; col < 3; ++col) v.tm[r][col] = f32(8 + 4 * static_cast<size_t>(r * 3 + col));
            if (v.type == 7) { v.fov = f32(56); v.distance = f32(60); }
            else v.zoom = f32(56);
            out.push_back(v);
        }
    }
    return true;
}

view::OrthoView ToOrthoView(const MaxViewport& v) {
    view::OrthoView o;
    o.width = 400.0 * v.zoom;
    o.centerU = -v.tm[3][0];
    o.centerV = -v.tm[3][1];
    return o;
}

view::PerspectiveView ToPerspectiveView(const MaxViewport& v) {
    const double (*R)[3] = v.tm;
    const double* T = v.tm[3];
    // w = (v - T) R^T: the eye is where v = 0; forward is view -Z.
    const math::Vec3 eye{-(T[0] * R[0][0] + T[1] * R[0][1] + T[2] * R[0][2]),
                         -(T[0] * R[1][0] + T[1] * R[1][1] + T[2] * R[1][2]),
                         -(T[0] * R[2][0] + T[1] * R[2][1] + T[2] * R[2][2])};
    const math::Vec3 forward = math::Normalize(math::Vec3{-R[0][2], -R[1][2], -R[2][2]});
    view::PerspectiveView p;
    p.distance = v.distance;
    p.fovDeg = v.fov * 180.0 / 3.14159265358979323846;
    p.elevationDeg = std::asin(std::fmax(-1.0, std::fmin(1.0, -forward.z))) * 180.0 / 3.14159265358979323846;
    p.azimuthDeg = std::atan2(-forward.x, forward.y) * 180.0 / 3.14159265358979323846;
    p.target = eye + forward * v.distance;
    return p;
}

} // namespace io
