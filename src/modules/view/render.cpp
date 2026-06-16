#include "render.h"

#include "geometry.h"
#include "state.h"
#include "ui.h"
#include "view.h"

#include <gdiplus.h>
#include <cmath>

using Gdiplus::ColorMatrix;
using Gdiplus::ImageAttributes;

namespace app::render {
namespace {

ColorMatrix BuildColorAdjustMatrix() {
    const float bright = static_cast<float>(app::g_brightness - 100) / 100.0f;  // -1..+1
    ColorMatrix brightness = {{
        {1, 0, 0, 0, 0},
        {0, 1, 0, 0, 0},
        {0, 0, 1, 0, 0},
        {0, 0, 0, 1, 0},
        {bright, bright, bright, 0, 1},
    }};
    return brightness;
}

}  // namespace

void DrawSelectionOverlay(HWND hwnd, Gdiplus::Graphics* graphics) {
    const app::ImageDisplayInfo info = app::view::GetLeftImageDisplayInfo(hwnd);
    if (!info.valid || !graphics) {
        return;
    }

    if (app::g_selectionMode == app::SelectionMode::Ellipse) {
        if (!app::g_ellipse.valid) {
            return;
        }

        const double cx = static_cast<double>(info.rect.X) + app::g_ellipse.cx * info.scale;
        const double cy = static_cast<double>(info.rect.Y) + app::g_ellipse.cy * info.scale;
        const double a = app::g_ellipse.a * info.scale;
        const double b = app::g_ellipse.b * info.scale;
        if (a < 1.0 || b < 1.0) {
            return;
        }

        const double angleDeg = app::g_ellipse.theta * 180.0 / 3.141592653589793;
        RECT clientRectRaw = {};
        GetClientRect(hwnd, &clientRectRaw);
        Gdiplus::Region outsideRegion(Gdiplus::Rect(
            clientRectRaw.left,
            clientRectRaw.top,
            clientRectRaw.right - clientRectRaw.left,
            clientRectRaw.bottom - clientRectRaw.top));

        Gdiplus::GraphicsPath ellipsePath;
        ellipsePath.AddEllipse(static_cast<Gdiplus::REAL>(-a),
                               static_cast<Gdiplus::REAL>(-b),
                               static_cast<Gdiplus::REAL>(a * 2.0),
                               static_cast<Gdiplus::REAL>(b * 2.0));
        Gdiplus::Matrix m;
        m.Rotate(static_cast<Gdiplus::REAL>(angleDeg));
        m.Translate(static_cast<Gdiplus::REAL>(cx), static_cast<Gdiplus::REAL>(cy), Gdiplus::MatrixOrderAppend);
        ellipsePath.Transform(&m);
        outsideRegion.Exclude(&ellipsePath);

        Gdiplus::SolidBrush outsideBrush(Gdiplus::Color(130, 0, 0, 0));
        graphics->FillRegion(&outsideBrush, &outsideRegion);

        Gdiplus::Pen borderPen(Gdiplus::Color(255, 16, 96, 192), 2.0f);
        graphics->DrawPath(&borderPen, &ellipsePath);

        const double ct = std::cos(app::g_ellipse.theta);
        const double st = std::sin(app::g_ellipse.theta);
        const double hxR_x = cx + a * ct;
        const double hxR_y = cy + a * st;
        const double hxL_x = cx - a * ct;
        const double hxL_y = cy - a * st;
        const double hyT_x = cx - b * st;
        const double hyT_y = cy + b * ct;
        const double hyB_x = cx + b * st;
        const double hyB_y = cy - b * ct;
        const double rotOffset = 30.0;
        const double hrot_x = hyT_x - rotOffset * st;
        const double hrot_y = hyT_y + rotOffset * ct;

        Gdiplus::SolidBrush handleBrush(Gdiplus::Color(255, 255, 80, 80));
        Gdiplus::Pen handlePen(Gdiplus::Color(255, 180, 0, 0), 1.0f);
        auto drawHandle = [&](double x, double y) {
            const float r = 5.0f;
            graphics->FillEllipse(&handleBrush, static_cast<float>(x - r), static_cast<float>(y - r), r * 2.0f, r * 2.0f);
            graphics->DrawEllipse(&handlePen, static_cast<float>(x - r), static_cast<float>(y - r), r * 2.0f, r * 2.0f);
        };
        drawHandle(hxL_x, hxL_y);
        drawHandle(hxR_x, hxR_y);
        drawHandle(hyT_x, hyT_y);
        drawHandle(hyB_x, hyB_y);
        drawHandle(hrot_x, hrot_y);

        Gdiplus::Pen guidePen(Gdiplus::Color(180, 255, 255, 255), 1.5f);
        graphics->DrawLine(&guidePen, static_cast<float>(hyT_x), static_cast<float>(hyT_y), static_cast<float>(hrot_x), static_cast<float>(hrot_y));
        return;
    }

    if (app::g_selectedPointsImageSpace.empty()) {
        return;
    }

    std::vector<Gdiplus::PointF> clientPoints;
    clientPoints.reserve(app::g_selectedPointsImageSpace.size());
    for (const auto& p : app::g_selectedPointsImageSpace) {
        clientPoints.push_back(app::view::ImagePointToClientPoint(info, p));
    }

    if (clientPoints.size() == 4) {
        std::array<Gdiplus::PointF, 4> orderedImagePts = {};
        if (app::geom::BuildOrderedQuadrilateral(app::g_selectedPointsImageSpace, &orderedImagePts)) {
            std::array<Gdiplus::PointF, 4> orderedClientPts = {
                app::view::ImagePointToClientPoint(info, orderedImagePts[0]),
                app::view::ImagePointToClientPoint(info, orderedImagePts[1]),
                app::view::ImagePointToClientPoint(info, orderedImagePts[2]),
                app::view::ImagePointToClientPoint(info, orderedImagePts[3]),
            };

            Gdiplus::Pen borderPen(Gdiplus::Color(255, 16, 96, 192), 2.0f);

            RECT clientRectRaw = {};
            GetClientRect(hwnd, &clientRectRaw);
            Gdiplus::Region outsideRegion(Gdiplus::Rect(
                clientRectRaw.left,
                clientRectRaw.top,
                clientRectRaw.right - clientRectRaw.left,
                clientRectRaw.bottom - clientRectRaw.top));

            Gdiplus::GraphicsPath polygonPath;
            polygonPath.AddPolygon(orderedClientPts.data(), 4);
            outsideRegion.Exclude(&polygonPath);

            Gdiplus::SolidBrush outsideBrush(Gdiplus::Color(130, 0, 0, 0));
            graphics->FillRegion(&outsideBrush, &outsideRegion);
            graphics->DrawPolygon(&borderPen, orderedClientPts.data(), 4);
        } else {
            Gdiplus::Pen edgePen(Gdiplus::Color(255, 64, 160, 255), 2.0f);
            graphics->DrawPolygon(&edgePen, clientPoints.data(), 4);
        }
    } else if (clientPoints.size() >= 2) {
        Gdiplus::Pen edgePen(Gdiplus::Color(255, 64, 160, 255), 2.0f);
        graphics->DrawLines(&edgePen, clientPoints.data(), static_cast<INT>(clientPoints.size()));
    }

    Gdiplus::SolidBrush pointBrush(Gdiplus::Color(255, 255, 80, 80));
    Gdiplus::Pen pointBorder(Gdiplus::Color(255, 180, 0, 0), 1.0f);
    for (const auto& p : clientPoints) {
        const float radius = 5.0f;
        graphics->FillEllipse(&pointBrush, p.X - radius, p.Y - radius, radius * 2.0f, radius * 2.0f);
        graphics->DrawEllipse(&pointBorder, p.X - radius, p.Y - radius, radius * 2.0f, radius * 2.0f);
    }
}

void DrawLoadedImage(HWND hwnd, HDC hdc) {
    RECT clientRectRaw = {};
    GetClientRect(hwnd, &clientRectRaw);
    const int clientWidth = clientRectRaw.right - clientRectRaw.left;
    const int clientHeight = clientRectRaw.bottom - clientRectRaw.top;
    if (clientWidth <= 0 || clientHeight <= 0) {
        return;
    }

    Gdiplus::Bitmap backBuffer(clientWidth, clientHeight, PixelFormat32bppPARGB);
    Gdiplus::Graphics backGraphics(&backBuffer);
    backGraphics.Clear(Gdiplus::Color(245, 245, 245));

    const app::ViewLayout layout = app::ui::GetViewLayout(hwnd);
    Gdiplus::SolidBrush leftPaneBrush(Gdiplus::Color(255, 238, 238, 238));
    Gdiplus::SolidBrush rightPaneBrush(Gdiplus::Color(255, 225, 225, 225));
    backGraphics.FillRectangle(&leftPaneBrush, layout.leftPane);
    backGraphics.FillRectangle(&rightPaneBrush, layout.rightPane);

    const app::ImageDisplayInfo leftInfo = app::view::GetLeftImageDisplayInfo(hwnd);
    if (leftInfo.valid) {
        Gdiplus::Region oldClip;
        backGraphics.GetClip(&oldClip);
        backGraphics.SetClip(layout.leftImagePane);
        DrawImageWithAdjustments(&backGraphics, app::g_loadedImage.get(), leftInfo.rect);
        DrawSelectionOverlay(hwnd, &backGraphics);
        backGraphics.SetClip(&oldClip, Gdiplus::CombineModeReplace);
    } else {
        DrawSelectionOverlay(hwnd, &backGraphics);
    }

    const app::ImageDisplayInfo rightInfo = app::view::FitImageToPane(layout.rightImagePane, app::g_correctedImage.get());
    if (rightInfo.valid) {
        Gdiplus::Region oldClip;
        backGraphics.GetClip(&oldClip);
        backGraphics.SetClip(layout.rightImagePane);
        const float pivotX = rightInfo.rect.X + rightInfo.rect.Width * 0.5f;
        const float pivotY = rightInfo.rect.Y + rightInfo.rect.Height * 0.5f;
        DrawImageWithAdjustmentsAndRotation(
            &backGraphics,
            app::g_correctedImage.get(),
            rightInfo.rect,
            static_cast<float>(app::g_outputRotateDeg),
            pivotX,
            pivotY);
        Gdiplus::Pen borderPen(Gdiplus::Color(255, 80, 80, 80), 1.0f);
        backGraphics.DrawRectangle(&borderPen, rightInfo.rect);
        backGraphics.SetClip(&oldClip, Gdiplus::CombineModeReplace);
    }

    Gdiplus::Graphics frontGraphics(hdc);
    frontGraphics.DrawImage(&backBuffer, 0, 0);
}

void DrawImageWithAdjustments(Gdiplus::Graphics* g, Gdiplus::Image* image, const Gdiplus::Rect& destRect) {
    if (!g || !image) {
        return;
    }
    ImageAttributes attrs;
    const ColorMatrix m = BuildColorAdjustMatrix();
    attrs.SetColorMatrix(&m);
    g->SetPageUnit(Gdiplus::UnitPixel);
    g->DrawImage(
        image,
        destRect,
        0,
        0,
        static_cast<INT>(image->GetWidth()),
        static_cast<INT>(image->GetHeight()),
        Gdiplus::UnitPixel,
        &attrs);
}

void DrawImageWithAdjustmentsAndRotation(
    Gdiplus::Graphics* g,
    Gdiplus::Image* image,
    const Gdiplus::Rect& destRect,
    float angleDeg,
    float pivotX,
    float pivotY) {
    if (!g || !image) {
        return;
    }
    ImageAttributes attrs;
    const ColorMatrix m = BuildColorAdjustMatrix();
    attrs.SetColorMatrix(&m);

    Gdiplus::Matrix old;
    g->GetTransform(&old);

    g->TranslateTransform(pivotX, pivotY);
    g->RotateTransform(angleDeg);
    g->TranslateTransform(-pivotX, -pivotY);

    g->SetPageUnit(Gdiplus::UnitPixel);
    g->DrawImage(
        image,
        destRect,
        0,
        0,
        static_cast<INT>(image->GetWidth()),
        static_cast<INT>(image->GetHeight()),
        Gdiplus::UnitPixel,
        &attrs);

    g->SetTransform(&old);
}

}  // namespace app::render

