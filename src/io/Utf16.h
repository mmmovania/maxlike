#pragma once
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

// UTF-16LE to UTF-8 (surrogate pairs combined; unpaired surrogates kept as written).
namespace io {

inline std::string Utf16ToUtf8(const std::vector<uint16_t>& w) {
    std::string s;
    for (size_t i = 0; i < w.size(); ++i) {
        uint32_t c = w[i];
        if (c >= 0xD800 && c < 0xDC00 && i + 1 < w.size() && w[i + 1] >= 0xDC00 && w[i + 1] < 0xE000)
            c = 0x10000 + ((c - 0xD800) << 10) + (w[++i] - 0xDC00u);
        if (c < 0x80) s += static_cast<char>(c);
        else if (c < 0x800) { s += static_cast<char>(0xC0 | (c >> 6)); s += static_cast<char>(0x80 | (c & 0x3F)); }
        else if (c < 0x10000) { s += static_cast<char>(0xE0 | (c >> 12)); s += static_cast<char>(0x80 | ((c >> 6) & 0x3F)); s += static_cast<char>(0x80 | (c & 0x3F)); }
        else { s += static_cast<char>(0xF0 | (c >> 18)); s += static_cast<char>(0x80 | ((c >> 12) & 0x3F)); s += static_cast<char>(0x80 | ((c >> 6) & 0x3F)); s += static_cast<char>(0x80 | (c & 0x3F)); }
    }
    return s;
}

// Bytes [begin, end) as UTF-16LE text (an odd trailing byte is ignored).
inline std::string Utf16BytesToUtf8(const std::vector<uint8_t>& d, size_t begin, size_t end) {
    std::vector<uint16_t> w;
    for (size_t i = begin; i + 1 < end && i + 1 < d.size(); i += 2) w.push_back(static_cast<uint16_t>(d[i] | (d[i + 1] << 8)));
    return Utf16ToUtf8(w);
}

} // namespace io
