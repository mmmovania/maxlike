#include "view/Picking.h"
#include <algorithm>
#include <cmath>

namespace view {

double DistanceToSegment(double px, double py, const Segment2& s) {
    const double dx = s.x1 - s.x0, dy = s.y1 - s.y0;
    const double len2 = dx * dx + dy * dy;
    double t = len2 > 0 ? ((px - s.x0) * dx + (py - s.y0) * dy) / len2 : 0;
    t = std::clamp(t, 0.0, 1.0);
    return std::hypot(px - (s.x0 + t * dx), py - (s.y0 + t * dy));
}

int PickNearest(const std::vector<std::vector<Segment2>>& outlines, double x, double y, double tolerance) {
    int best = -1;
    double bestDist = tolerance;
    for (size_t i = 0; i < outlines.size(); ++i)
        for (const Segment2& s : outlines[i]) {
            const double d = DistanceToSegment(x, y, s);
            if (d <= bestDist) { bestDist = d; best = static_cast<int>(i); }
        }
    return best;
}

int PickSurface(const std::vector<std::vector<Triangle2>>& surfaces, double x, double y) {
    int best = -1;
    double bestDepth = 1e300;
    for (size_t i = 0; i < surfaces.size(); ++i)
        for (const Triangle2& t : surfaces[i]) {
            // Same side of all three edges (either winding).
            double sign = 0;
            bool inside = true;
            for (int k = 0; k < 3 && inside; ++k) {
                const int j = (k + 1) % 3;
                const double c = (t.x[j] - t.x[k]) * (y - t.y[k]) - (t.y[j] - t.y[k]) * (x - t.x[k]);
                if (c == 0) continue;
                if (sign == 0) sign = c;
                else if ((c > 0) != (sign > 0)) inside = false;
            }
            if (inside && sign != 0 && t.depth < bestDepth) { bestDepth = t.depth; best = static_cast<int>(i); }
        }
    return best;
}

// Does a segment touch an axis-aligned rectangle? (Liang-Barsky clipping.)
static bool SegmentTouchesRect(const Segment2& s, double x0, double y0, double x1, double y1) {
    double t0 = 0, t1 = 1;
    const double dx = s.x1 - s.x0, dy = s.y1 - s.y0;
    const double p[4] = {-dx, dx, -dy, dy};
    const double q[4] = {s.x0 - x0, x1 - s.x0, s.y0 - y0, y1 - s.y0};
    for (int i = 0; i < 4; ++i) {
        if (p[i] == 0) { if (q[i] < 0) return false; continue; }
        const double r = q[i] / p[i];
        if (p[i] < 0) t0 = std::max(t0, r); else t1 = std::min(t1, r);
        if (t0 > t1) return false;
    }
    return true;
}

std::vector<int> PickRegion(const std::vector<std::vector<Segment2>>& outlines,
                            double x0, double y0, double x1, double y1) {
    if (x1 < x0) std::swap(x0, x1);
    if (y1 < y0) std::swap(y0, y1);
    std::vector<int> picked;
    for (size_t i = 0; i < outlines.size(); ++i)
        for (const Segment2& s : outlines[i])
            if (SegmentTouchesRect(s, x0, y0, x1, y1)) { picked.push_back(static_cast<int>(i)); break; }
    return picked;
}

int PickNearestPoint(const std::vector<Point2>& points, double x, double y, double tolerance) {
    int best = -1;
    double bestDist = tolerance;
    for (size_t i = 0; i < points.size(); ++i) {
        const double d = std::hypot(points[i].x - x, points[i].y - y);
        if (d <= bestDist) { bestDist = d; best = static_cast<int>(i); }
    }
    return best;
}

std::vector<int> PickPointsInRect(const std::vector<Point2>& points, double x0, double y0, double x1, double y1) {
    if (x1 < x0) std::swap(x0, x1);
    if (y1 < y0) std::swap(y0, y1);
    std::vector<int> out;
    for (size_t i = 0; i < points.size(); ++i)
        if (points[i].x >= x0 && points[i].x <= x1 && points[i].y >= y0 && points[i].y <= y1) out.push_back(static_cast<int>(i));
    return out;
}

} // namespace view
