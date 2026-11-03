#include <glad/gl.h>
#include "ui/Viewport.h"
#include "gl/GLContext.h"
#include "gl/LineRenderer.h"
#include "ui/Bevel.h"
#include "ui/Theme.h"

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
    if (type_ == ViewType::Perspective)
        DrawPerspectiveGrid(rc.right, rc.bottom);
    else
        DrawOrthoGrid(rc.right, rc.bottom);
    // Step 2.3: label and axis tripod.

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
    case WM_RBUTTONDOWN:
    case WM_MBUTTONDOWN:
        if (onActivate) onActivate();
        return 0;
    }
    return win::Window::HandleMessage(msg, wp, lp);
}

} // namespace ui
