#include "io/Chunks.h"

namespace io {

bool ParseChunks(const std::vector<uint8_t>& d, size_t begin, size_t end, std::vector<Chunk>& out, std::string& error,
                 int maxDepth) {
    out.clear();
    if (end > d.size() || begin > end) { error = "chunk range outside the stream"; return false; }
    if (maxDepth < 0) { error = "chunks nested too deeply"; return false; }
    size_t p = begin;
    while (p < end) {
        if (end - p < 6) { error = "truncated chunk header"; return false; }
        Chunk c;
        c.type = static_cast<uint16_t>(d[p] | (d[p + 1] << 8));
        uint64_t size = static_cast<uint64_t>(d[p + 2]) | (static_cast<uint64_t>(d[p + 3]) << 8) |
                        (static_cast<uint64_t>(d[p + 4]) << 16) | (static_cast<uint64_t>(d[p + 5]) << 24);
        size_t header = 6;
        if (size == 0) {                                        // the 64-bit form
            if (end - p < 14) { error = "truncated 64-bit chunk header"; return false; }
            size = 0;
            for (int i = 0; i < 8; ++i) size |= static_cast<uint64_t>(d[p + 6 + i]) << (8 * i);
            c.container = (size >> 63) != 0;
            size &= ~(1ull << 63);
            header = 14;
        } else {
            c.container = (size >> 31) != 0;
            size &= 0x7FFFFFFFull;
        }
        if (size < header) { error = "chunk smaller than its header"; return false; }
        if (size > end - p) { error = "chunk runs past its parent"; return false; }
        c.begin = p + header;
        c.end = p + static_cast<size_t>(size);
        if (c.container && !ParseChunks(d, c.begin, c.end, c.children, error, maxDepth - 1)) return false;
        out.push_back(std::move(c));
        p += static_cast<size_t>(size);
    }
    return true;
}

} // namespace io
