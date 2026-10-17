#include "ui/Theme.h"

namespace theme {

HFONT CreateUiFont(UINT dpi) {
    LOGFONTW lf{};
    lf.lfHeight  = -MulDiv(8, static_cast<int>(dpi), 72);
    lf.lfWeight  = FW_NORMAL;
    lf.lfCharSet = DEFAULT_CHARSET;
    lf.lfQuality = (dpi == 96) ? DEFAULT_QUALITY : CLEARTYPE_QUALITY;
    wcscpy_s(lf.lfFaceName, dpi == 96 ? L"MS Sans Serif" : L"Microsoft Sans Serif");
    return CreateFontIndirectW(&lf);
}

} // namespace theme
