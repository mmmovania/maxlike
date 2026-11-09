#include "scene/SoftSelection.h"
#include <algorithm>
#include <limits>

namespace scene::soft {

double AffectRegion(double d, double falloff, double pinch, double bubble) {
    if (falloff <= 0) return d <= 0 ? 1.0 : 0.0;
    if (d >= falloff) return 0.0;
    if (d <= 0) return 1.0;
    const double u = (falloff - d) / falloff, u2 = u * u, s = 1.0 - u;
    return (3 * u * bubble * s + 3 * u2 * (1.0 - pinch)) * s + u * u2;
}

std::vector<double> Weights(const Mesh& m, const std::vector<uint32_t>& selected, double falloff, double pinch,
                            double bubble, int edgeLimit) {
    const size_t n = m.verts.size();
    std::vector<double> w(n, 0.0);
    std::vector<char> isSel(n, 0);
    for (uint32_t v : selected) if (v < n) { isSel[v] = 1; w[v] = 1.0; }
    // Edge Distance: vertices within edgeLimit edges of the selection (breadth first).
    std::vector<char> eligible(n, 1);
    if (edgeLimit >= 0) {
        std::vector<std::vector<uint32_t>> next(n);
        for (const Face& f : m.faces)
            for (int k = 0; k < 3; ++k) {
                const uint32_t a = f.v[k], b = f.v[(k + 1) % 3];
                if (a < n && b < n) { next[a].push_back(b); next[b].push_back(a); }
            }
        std::vector<int> hops(n, -1);
        std::vector<uint32_t> frontier;
        for (uint32_t v = 0; v < n; ++v) if (isSel[v]) { hops[v] = 0; frontier.push_back(v); }
        for (int h = 0; h < edgeLimit && !frontier.empty(); ++h) {
            std::vector<uint32_t> more;
            for (uint32_t v : frontier)
                for (uint32_t u : next[v]) if (hops[u] < 0) { hops[u] = h + 1; more.push_back(u); }
            frontier.swap(more);
        }
        for (uint32_t v = 0; v < n; ++v) eligible[v] = hops[v] >= 0;
    }
    for (uint32_t v = 0; v < n; ++v) {
        if (isSel[v] || !eligible[v]) continue;
        double d = std::numeric_limits<double>::max();
        for (uint32_t s : selected) if (s < n) d = std::min(d, math::Length(m.verts[v] - m.verts[s]));
        w[v] = AffectRegion(d, falloff, pinch, bubble);
    }
    return w;
}

} // namespace scene::soft
