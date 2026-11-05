#pragma once
#include <windows.h>
#include <string>
#include <vector>

namespace ui {

// The pixels of a short text, drawn by Windows (GDI) into an off-screen bitmap and
// read back, so OpenGL can draw them as single pixels. Positioned by the ink: the
// top-most lit row lands on `inkTop`, the text starts at x = `left`.
struct TextPixels {
    std::vector<POINT> pixels;
};

TextPixels RenderText(const std::wstring& text, HFONT font, int left, int inkTop);

// The viewport label font (measured: 10-px capitals with 2-px strokes; most likely
// MS Sans Serif Bold 10 pt, still to be verified on Windows). Caller owns the font.
HFONT CreateViewportLabelFont(UINT dpi);

} // namespace ui
