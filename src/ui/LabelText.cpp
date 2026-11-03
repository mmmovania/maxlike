#include "ui/LabelText.h"

namespace ui {

HFONT CreateViewportLabelFont(UINT dpi) {
    LOGFONTW lf{};
    lf.lfHeight = -MulDiv(10, static_cast<int>(dpi), 72);
    lf.lfWeight = FW_BOLD;
    lf.lfCharSet = DEFAULT_CHARSET;
    lf.lfQuality = NONANTIALIASED_QUALITY; // crisp pixels: either on or off
    wcscpy_s(lf.lfFaceName, dpi == 96 ? L"MS Sans Serif" : L"Microsoft Sans Serif");
    return CreateFontIndirectW(&lf);
}

TextPixels RenderText(const std::wstring& text, HFONT font, int left, int inkTop) {
    TextPixels out;
    HDC screen = GetDC(nullptr);
    HDC dc = CreateCompatibleDC(screen);
    ReleaseDC(nullptr, screen);
    HGDIOBJ oldFont = SelectObject(dc, font);

    SIZE size{};
    GetTextExtentPoint32W(dc, text.c_str(), static_cast<int>(text.size()), &size);
    const int w = size.cx + 8, h = size.cy + 8;

    // A 32-bit top-down DIB: we get a pointer to its pixels.
    BITMAPINFO bi{};
    bi.bmiHeader.biSize = sizeof(bi.bmiHeader);
    bi.bmiHeader.biWidth = w;
    bi.bmiHeader.biHeight = -h;           // negative = top-down rows
    bi.bmiHeader.biPlanes = 1;
    bi.bmiHeader.biBitCount = 32;
    bi.bmiHeader.biCompression = BI_RGB;
    void* bits = nullptr;
    HBITMAP bmp = CreateDIBSection(dc, &bi, DIB_RGB_COLORS, &bits, nullptr, 0);
    if (!bmp || !bits) {
        SelectObject(dc, oldFont);
        DeleteDC(dc);
        return out;
    }
    HGDIOBJ oldBmp = SelectObject(dc, bmp);

    // White text on black, then collect the lit pixels.
    RECT all{0, 0, w, h};
    FillRect(dc, &all, static_cast<HBRUSH>(GetStockObject(BLACK_BRUSH)));
    SetTextColor(dc, RGB(255, 255, 255));
    SetBkMode(dc, TRANSPARENT);
    TextOutW(dc, 0, 0, text.c_str(), static_cast<int>(text.size()));
    GdiFlush(); // make sure GDI has finished drawing before we read the memory

    const auto* px = static_cast<const unsigned*>(bits);
    int top = -1;
    for (int y = 0; y < h && top < 0; ++y)
        for (int x = 0; x < w; ++x)
            if (px[y * w + x] & 0x00FFFFFF) { top = y; break; }
    if (top >= 0) {
        for (int y = 0; y < h; ++y)
            for (int x = 0; x < w; ++x)
                if (px[y * w + x] & 0x00FFFFFF)
                    out.pixels.push_back({left + x, inkTop + (y - top)});
    }

    SelectObject(dc, oldBmp);
    SelectObject(dc, oldFont);
    DeleteObject(bmp);
    DeleteDC(dc);
    return out;
}

} // namespace ui
