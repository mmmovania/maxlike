#include <glad/gl.h>
#include <windowsx.h>
#include <algorithm>
#include "ui/Viewport.h"
#include "gl/GLContext.h"
#include "gl/LineRenderer.h"
#include "ui/Bevel.h"
#include "ui/Theme.h"
#include "view/Tripod.h"

namespace ui {

bool Viewport::Create(HWND parent, gl::GLContext& context, gl::LineRenderer& lines, ViewType type) {
    context_ = &context;
    lines_ = &lines;
    type_ = type;
    CreateParams p;
    p.title = L"Viewport";
    // An OpenGL window must not have its pixels drawn over by siblings or children.
    p.style = WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN;
    p.x = p.y = 0;
    p.width = p.height = 1;
    p.parent = parent;
    return win::Window::Create(p);
}

void Viewport::Render(HDC dc) {
    RECT rc;
    GetClientRect(Hwnd(), &rc);
    context_->MakeCurrent(dc);
    glViewport(0, 0, rc.right, rc.bottom);
    glClearColor(theme::kViewportBg.R(), theme::kViewportBg.G(), theme::kViewportBg.B(), 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // OpenGL objects must be created with the context current, so the shared line
    // renderer is created on the first paint of whichever viewport comes first.
    if (!lines_->IsValid()) lines_->Create();
    if (showGrid_) {
        if (type_ == ViewType::Perspective)
            DrawPerspectiveGrid(rc.right, rc.bottom);
        else
            DrawOrthoGrid(rc.right, rc.bottom);
    }
    DrawLabelAndTripod(rc.right, rc.bottom);

    SwapBuffers(dc); // show the finished back buffer
}

// Minor lines first, then major, then the axes, so darker lines are always on top.
static const theme::Rgb kGridColours[3] = {theme::kGridMinor, theme::kGridMajor, theme::kGridAxis};

// Ortho: lines computed in pixels (exactly R3's), drawn through pixel centres and
// extended past both ends so no end pixel is lost.
void Viewport::DrawOrthoGrid(int w, int h) {
    using Line = gl::LineRenderer::Line;
    std::vector<Line> batch[3];
    for (const view::GridLine& l : view::OrthoGridLines(w, h, ortho_)) {
        const float c = static_cast<float>(l.pixel) + 0.5f;
        const Line line = l.vertical ? Line{c, -1.0f, 0, c, static_cast<float>(h) + 1.0f, 0}
                                     : Line{-1.0f, c, 0, static_cast<float>(w) + 1.0f, c, 0};
        batch[static_cast<int>(l.kind)].push_back(line);
    }
    const math::Mat4 toClip = math::PixelToClip(w, h);
    for (int k = 0; k < 3; ++k)
        lines_->Draw(batch[k], toClip, kGridColours[k].R(), kGridColours[k].G(), kGridColours[k].B());
}

// Perspective: the home grid in world space, through the camera's matrices.
void Viewport::DrawPerspectiveGrid(int w, int h) {
    using Line = gl::LineRenderer::Line;
    std::vector<Line> batch[3];
    for (const view::WorldLine& l : view::HomeGridLines()) {
        batch[l.kind].push_back({static_cast<float>(l.a.x), static_cast<float>(l.a.y), static_cast<float>(l.a.z),
                                 static_cast<float>(l.b.x), static_cast<float>(l.b.y), static_cast<float>(l.b.z)});
    }
    const math::Mat4 viewProj = perspective_.ProjectionMatrix(w, h) * perspective_.ViewMatrix();
    for (int k = 0; k < 3; ++k)
        lines_->Draw(batch[k], viewProj, kGridColours[k].R(), kGridColours[k].G(), kGridColours[k].B());
}

const wchar_t* Viewport::Name() const {
    switch (type_) {
    case ViewType::Top:         return L"Top";
    case ViewType::Bottom:      return L"Bottom";
    case ViewType::Front:       return L"Front";
    case ViewType::Back:        return L"Back";
    case ViewType::Left:        return L"Left";
    case ViewType::Right:       return L"Right";
    case ViewType::Perspective: return L"Perspective";
    }
    return L"";
}

void Viewport::SetType(ViewType type) {
    if (type == type_) return;
    type_ = type;
    labelDpi_ = 0;                 // the label text changed: render it again
    InvalidateRect(Hwnd(), nullptr, FALSE);
}

void Viewport::SetShowGrid(bool show) {
    showGrid_ = show;
    InvalidateRect(Hwnd(), nullptr, FALSE);
}

// Right-clicking the label opens the menu. The area is the label's ink plus a few
// pixels around it.
bool Viewport::InLabel(int x, int y) const {
    if (label_.pixels.empty()) return false;
    LONG right = 0, bottom = 0;
    for (const POINT& p : label_.pixels) { right = std::max(right, p.x); bottom = std::max(bottom, p.y); }
    return x >= 0 && y >= 0 && x <= right + Scale(4) && y <= bottom + Scale(3);
}

// R3's viewport menu, reconstructed from memory: items, order and names are still to
// be verified against R3.1. Entries that need features from later phases are greyed.
// Keyboard-shortcut text (R3 shows e.g. "Top  T") is left out until shortcuts exist.
void Viewport::ShowLabelMenu(POINT screen) {
    enum : UINT {
        kSmoothHi = 1, kWire, kSmooth, kFacetsHi, kFacets, kLitWire, kBox,
        kEdgedFaces, kShowGrid, kShowBackground, kSafeFrame, kDisable, kTexCorrection,
        kTop, kBottom, kFront, kBack, kLeft, kRight, kUser, kPersp, kCamera, kLight, kShape, kGrid,
        kUndo, kRedo, kConfigure,
    };
    auto check = [](bool on) { return on ? MF_CHECKED : MF_UNCHECKED; };
    const UINT grey = MF_GRAYED;

    HMENU other = CreatePopupMenu();
    AppendMenuW(other, MF_STRING | check(shading_ == Shading::Smooth), kSmooth, L"Smooth");
    AppendMenuW(other, MF_STRING | check(shading_ == Shading::FacetsHighlights), kFacetsHi, L"Facets + Highlights");
    AppendMenuW(other, MF_STRING | check(shading_ == Shading::Facets), kFacets, L"Facets");
    AppendMenuW(other, MF_STRING | check(shading_ == Shading::LitWireframe), kLitWire, L"Lit Wireframes");
    AppendMenuW(other, MF_STRING | check(shading_ == Shading::BoundingBox), kBox, L"Bounding Box");

    HMENU views = CreatePopupMenu();
    AppendMenuW(views, MF_STRING | check(type_ == ViewType::Top), kTop, L"Top");
    AppendMenuW(views, MF_STRING | check(type_ == ViewType::Bottom), kBottom, L"Bottom");
    AppendMenuW(views, MF_STRING | check(type_ == ViewType::Front), kFront, L"Front");
    AppendMenuW(views, MF_STRING | check(type_ == ViewType::Back), kBack, L"Back");
    AppendMenuW(views, MF_STRING | check(type_ == ViewType::Left), kLeft, L"Left");
    AppendMenuW(views, MF_STRING | check(type_ == ViewType::Right), kRight, L"Right");
    AppendMenuW(views, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(views, MF_STRING | grey, kUser, L"User");
    AppendMenuW(views, MF_STRING | check(type_ == ViewType::Perspective), kPersp, L"Perspective");
    AppendMenuW(views, MF_STRING | grey, kCamera, L"Camera");
    AppendMenuW(views, MF_STRING | grey, kLight, L"Spot/Directional Light");
    AppendMenuW(views, MF_STRING | grey, kShape, L"Shape");
    AppendMenuW(views, MF_STRING | grey, kGrid, L"Grid");

    HMENU menu = CreatePopupMenu();
    AppendMenuW(menu, MF_STRING | check(shading_ == Shading::SmoothHighlights), kSmoothHi, L"Smooth + Highlights");
    AppendMenuW(menu, MF_STRING | check(shading_ == Shading::Wireframe), kWire, L"Wireframe");
    AppendMenuW(menu, MF_POPUP, reinterpret_cast<UINT_PTR>(other), L"Other");
    AppendMenuW(menu, MF_STRING | grey, kEdgedFaces, L"Edged Faces");
    AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(menu, MF_STRING | check(showGrid_), kShowGrid, L"Show Grid");
    AppendMenuW(menu, MF_STRING | grey, kShowBackground, L"Show Background");
    AppendMenuW(menu, MF_STRING | grey, kSafeFrame, L"Show Safe Frame");
    AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(menu, MF_STRING | grey, kDisable, L"Disable View");
    AppendMenuW(menu, MF_STRING | grey, kTexCorrection, L"Texture Correction");
    AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(menu, MF_POPUP, reinterpret_cast<UINT_PTR>(views), L"Views");
    AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(menu, MF_STRING | grey, kUndo, L"Undo View Change");
    AppendMenuW(menu, MF_STRING | grey, kRedo, L"Redo View Change");
    AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(menu, MF_STRING | grey, kConfigure, L"Configure...");

    // TPM_RETURNCMD: return the chosen item instead of sending WM_COMMAND.
    const UINT cmd = TrackPopupMenu(menu, TPM_RETURNCMD | TPM_RIGHTBUTTON | TPM_LEFTALIGN | TPM_TOPALIGN,
                                    screen.x, screen.y, 0, Hwnd(), nullptr);
    DestroyMenu(menu); // also destroys the submenus it owns

    switch (cmd) {
    case kSmoothHi: shading_ = Shading::SmoothHighlights; break;
    case kWire:     shading_ = Shading::Wireframe; break;
    case kSmooth:   shading_ = Shading::Smooth; break;
    case kFacetsHi: shading_ = Shading::FacetsHighlights; break;
    case kFacets:   shading_ = Shading::Facets; break;
    case kLitWire:  shading_ = Shading::LitWireframe; break;
    case kBox:      shading_ = Shading::BoundingBox; break;
    case kShowGrid: SetShowGrid(!showGrid_); break;
    case kTop:      SetType(ViewType::Top); break;
    case kBottom:   SetType(ViewType::Bottom); break;
    case kFront:    SetType(ViewType::Front); break;
    case kBack:     SetType(ViewType::Back); break;
    case kLeft:     SetType(ViewType::Left); break;
    case kRight:    SetType(ViewType::Right); break;
    case kPersp:    SetType(ViewType::Perspective); break;
    default: break; // cancelled (0) or not implemented yet
    }
}

// How world X, Y, Z appear on screen: components along screen-right and screen-up.
void Viewport::ScreenAxes(double out[3][2]) const {
    auto set = [&](double xr, double xu, double yr, double yu, double zr, double zu) {
        out[0][0] = xr; out[0][1] = xu; out[1][0] = yr; out[1][1] = yu; out[2][0] = zr; out[2][1] = zu;
    };
    switch (type_) {
    case ViewType::Top:    set(1, 0, 0, 1, 0, 0);   break; // looking down -Z
    case ViewType::Bottom: set(1, 0, 0, -1, 0, 0);  break; // looking up +Z: +Y points down
    case ViewType::Front:  set(1, 0, 0, 0, 0, 1);   break; // looking along +Y
    case ViewType::Back:   set(-1, 0, 0, 0, 0, 1);  break; // looking along -Y: +X points left
    case ViewType::Left:   set(0, 0, -1, 0, 0, 1);  break; // looking along +X: +Y points left
    case ViewType::Right:  set(0, 0, 1, 0, 0, 1);   break; // looking along -X: +Y points right
    case ViewType::Perspective: {
        // The camera's right and up directions, from its view matrix rows.
        const math::Mat4 v = perspective_.ViewMatrix();
        for (int a = 0; a < 3; ++a) {
            out[a][0] = v.m[a][0];   // world axis a . camera right
            out[a][1] = v.m[a][1];   // world axis a . camera up
        }
        break;
    }
    }
}

// The label (top-left) and the axis tripod (bottom-left), as single pixels drawn
// over the grid, in viewport pixel coordinates.
void Viewport::DrawLabelAndTripod(int w, int h) {
    const math::Mat4 toClip = math::PixelToClip(w, h);
    auto centre = [](int x, int y, std::vector<float>& v) {
        v.push_back(static_cast<float>(x) + 0.5f);
        v.push_back(static_cast<float>(y) + 0.5f);
        v.push_back(0.0f);
    };

    // Label: rendered once per DPI (measured: text starts at x 4, ink top at y 4).
    if (labelDpi_ != Dpi()) {
        HFONT font = CreateViewportLabelFont(Dpi());
        label_ = RenderText(Name(), font, Scale(4), Scale(4));
        DeleteObject(font);
        labelDpi_ = Dpi();
    }
    std::vector<float> text;
    for (const POINT& p : label_.pixels) centre(p.x, p.y, text);
    lines_->DrawPoints(text, toClip, theme::kViewportLabel.R(), theme::kViewportLabel.G(), theme::kViewportLabel.B());

    // Tripod: in axis order, so later axes cover earlier ones as in R3.
    double axes[3][2];
    ScreenAxes(axes);
    std::vector<float> pts[3];
    for (const view::TripodPixel& p : view::TripodPixels(axes, h)) centre(p.x, p.y, pts[p.axis]);
    const theme::Rgb colours[3] = {theme::kTripodX, theme::kTripodY, theme::kTripodZ};
    for (int a = 0; a < 3; ++a)
        lines_->DrawPoints(pts[a], toClip, colours[a].R(), colours[a].G(), colours[a].B());
}

void Viewport::PaintFallback(HDC dc) {
    RECT rc;
    GetClientRect(Hwnd(), &rc);
    FillSolid(dc, rc, theme::kViewportBg.Ref());
    const std::wstring msg = L"OpenGL 3.3 is not available";
    SetBkMode(dc, TRANSPARENT);
    SetTextColor(dc, theme::kViewportLabel.Ref());
    HGDIOBJ old = SelectObject(dc, GetStockObject(DEFAULT_GUI_FONT));
    DrawTextW(dc, msg.c_str(), -1, &rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    SelectObject(dc, old);
}

LRESULT Viewport::HandleMessage(UINT msg, WPARAM wp, LPARAM lp) {
    switch (msg) {
    case WM_CREATE:
        // Every window that will show OpenGL needs the context's pixel format.
        if (context_->IsValid()) context_->PrepareWindow(Hwnd());
        return 0;
    case WM_ERASEBKGND:
        return 1; // OpenGL fills every pixel
    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC dc = BeginPaint(Hwnd(), &ps);
        if (context_->IsValid()) Render(dc);
        else PaintFallback(dc);
        EndPaint(Hwnd(), &ps);
        return 0;
    }
    case WM_LBUTTONDOWN:
    case WM_MBUTTONDOWN:
        if (onActivate) onActivate();
        return 0;
    case WM_RBUTTONDOWN:
        if (onActivate) onActivate();
        return 0;
    case WM_RBUTTONUP:
        // The label menu opens on release, as context menus do on Windows.
        if (InLabel(GET_X_LPARAM(lp), GET_Y_LPARAM(lp))) {
            POINT pt{GET_X_LPARAM(lp), GET_Y_LPARAM(lp)};
            ClientToScreen(Hwnd(), &pt);
            ShowLabelMenu(pt);
        }
        return 0;
    }
    return win::Window::HandleMessage(msg, wp, lp);
}

} // namespace ui
