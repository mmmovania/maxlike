#include "ui/Cursors.h"
#include <cstdint>

namespace ui::cursors {
namespace {
// '#' black, '.' white, ' ' transparent; 16 x 16 drawn in the top-left of a 32 x 32 cursor.
HCURSOR Make(const char* const rows[16], int hotX, int hotY) {
    uint8_t andPlane[32 * 4], xorPlane[32 * 4];
    for (int i = 0; i < 32 * 4; ++i) { andPlane[i] = 0xFF; xorPlane[i] = 0x00; }
    for (int y = 0; y < 16; ++y)
        for (int x = 0; x < 16; ++x) {
            const char c = rows[y][x];
            if (c == ' ') continue;
            const int byte = y * 4 + x / 8;
            const uint8_t bit = static_cast<uint8_t>(0x80 >> (x % 8));
            andPlane[byte] = static_cast<uint8_t>(andPlane[byte] & ~bit);
            if (c == '.') xorPlane[byte] = static_cast<uint8_t>(xorPlane[byte] | bit);
        }
    return CreateCursor(GetModuleHandleW(nullptr), hotX, hotY, 32, 32, andPlane, xorPlane);
}
const char* const kSelect[16] = {
    "      ...       ", "      .#.       ", "      .#.       ", "      .#.       ",
    "      .#.       ", "       .        ", "......   ...... ", ".#####   #####. ",
    "......   ...... ", "       .        ", "      .#.       ", "      .#.       ",
    "      .#.       ", "      .#.       ", "      ...       ", "                "};
const char* const kRotate[16] = {
    "     ......     ", "    .######.    ", "   .#......#.   ", "  .#.      .#.  ",
    " .#.        .#. ", " .#.        .#. ", " .#.      ..... ", " .#.      .###. ",
    " .#.       .#.  ", " .#.        .   ", "  .#.           ", "   .#......     ", "    .######.    ",
    "     ......     ", "                ", "                "};
const char* const kScale[16] = {
    "       .        ", "      .#.       ", "      .#.       ", "     .#.#.      ",
    "     .#.#.      ", "    .#. .#.     ", "    .#. .#.     ", "   .#.   .#.    ",
    "   .#.   .#.    ", "  .#.     .#.   ", "  .#.     .#.   ", " .#.........#.  ", " .###########.  ",
    " .............  ", "                ", "                "};
} // namespace

HCURSOR ForTool(int kind) {
    static HCURSOR select = Make(kSelect, 7, 7), rotate = Make(kRotate, 7, 7), scale = Make(kScale, 7, 7);
    static HCURSOR move = LoadCursorW(nullptr, IDC_SIZEALL);
    switch (kind) {
    case 1: return move;
    case 2: return rotate ? rotate : move;
    case 3: return scale ? scale : move;
    default: return select ? select : LoadCursorW(nullptr, IDC_CROSS);
    }
}

} // namespace ui::cursors
