#include "view.h"

#include "ui.h"

#include <algorithm>
#include <cmath>

namespace app::view {

app::ImageDisplayInfo FitImageToPane(const Gdiplus::Rect& paneRect, Gdiplus::Image* image) {
    app::ImageDisplayInfo info;
    if (!image || paneRect.Width <= 0 || paneRect.Height <= 0) {
        return info;
    }

    const UINT imageWidth = image->GetWidth();
    const UINT imageHeight = image->GetHeight();
    if (imageWidth == 0 || imageHeight == 0) {
        return info;
    }

    const double scaleX = static_cast<double>(paneRect.Width) / static_cast<double>(imageWidth);
    const double scaleY = static_cast<double>(paneRect.Height) / static_cast<double>(imageHeight);
    const double scale = (scaleX < scaleY) ? scaleX : scaleY;

    const int drawWidth = static_cast<int>(imageWidth * scale);
    const int drawHeight = static_cast<int>(imageHeight * scale);
    const int offsetX = paneRect.X + (paneRect.Width - drawWidth) / 2;
    const int offsetY = paneRect.Y + (paneRect.Height - drawHeight) / 2;

    info.valid = true;
    info.rect = Gdiplus::Rect(offsetX, offsetY, drawWidth, drawHeight);
    info.scale = scale;
    info.imageWidth = imageWidth;
    info.imageHeight = imageHeight;
    return info;
}

static app::ImageDisplayInfo FitImageToLeftPaneWithZoomPan(HWND hwnd) {
    const app::ViewLayout layout = app::ui::GetViewLayout(hwnd);
    app::ImageDisplayInfo base = FitImageToPane(layout.leftImagePane, app::g_loadedImage.get());
    if (!base.valid) {
        return base;
    }

    const double zoom = std::clamp(app::g_leftZoom, 0.1, 20.0);
    const double newScale = base.scale * zoom;
    const int drawWidth = static_cast<int>(base.imageWidth * newScale);
    const int drawHeight = static_cast<int>(base.imageHeight * newScale);

    const int centeredX = layout.leftImagePane.X + (layout.leftImagePane.Width - drawWidth) / 2;
    const int centeredY = layout.leftImagePane.Y + (layout.leftImagePane.Height - drawHeight) / 2;
    base.scale = newScale;
    base.rect = Gdiplus::Rect(centeredX + app::g_leftPanX, centeredY + app::g_leftPanY, drawWidth, drawHeight);
    return base;
}

app::ImageDisplayInfo GetLeftImageDisplayInfo(HWND hwnd) {
    return FitImageToLeftPaneWithZoomPan(hwnd);
}

Gdiplus::PointF ImagePointToClientPoint(const app::ImageDisplayInfo& info, const Gdiplus::PointF& imagePoint) {
    return Gdiplus::PointF(
        static_cast<Gdiplus::REAL>(info.rect.X + imagePoint.X * info.scale),
        static_cast<Gdiplus::REAL>(info.rect.Y + imagePoint.Y * info.scale));
}

bool ClientPointToImagePoint(HWND hwnd, int clientX, int clientY, Gdiplus::PointF* outImagePoint) {
    const app::ImageDisplayInfo info = GetLeftImageDisplayInfo(hwnd);
    if (!info.valid) {
        return false;
    }

    const bool insideImage = clientX >= info.rect.X && clientX < info.rect.X + info.rect.Width && clientY >= info.rect.Y &&
                             clientY < info.rect.Y + info.rect.Height;
    if (!insideImage || !outImagePoint) {
        return false;
    }

    const double xInImage = (static_cast<double>(clientX - info.rect.X)) / info.scale;
    const double yInImage = (static_cast<double>(clientY - info.rect.Y)) / info.scale;
    outImagePoint->X = static_cast<Gdiplus::REAL>(xInImage);
    outImagePoint->Y = static_cast<Gdiplus::REAL>(yInImage);
    return true;
}

bool ClientPointToImagePointClamped(HWND hwnd, int clientX, int clientY, Gdiplus::PointF* outImagePoint) {
    const app::ImageDisplayInfo info = GetLeftImageDisplayInfo(hwnd);
    if (!info.valid || !outImagePoint) {
        return false;
    }

    const int clampedX = std::clamp(clientX, info.rect.X, info.rect.X + info.rect.Width - 1);
    const int clampedY = std::clamp(clientY, info.rect.Y, info.rect.Y + info.rect.Height - 1);

    const double xInImage = (static_cast<double>(clampedX - info.rect.X)) / info.scale;
    const double yInImage = (static_cast<double>(clampedY - info.rect.Y)) / info.scale;
    outImagePoint->X = static_cast<Gdiplus::REAL>(xInImage);
    outImagePoint->Y = static_cast<Gdiplus::REAL>(yInImage);
    return true;
}

int HitTestControlPointIndex(HWND hwnd, int clientX, int clientY) {
    const app::ImageDisplayInfo info = GetLeftImageDisplayInfo(hwnd);
    if (!info.valid) {
        return -1;
    }

    const float hitRadiusSquared = app::kPointHitRadiusPixels * app::kPointHitRadiusPixels;
    for (size_t i = 0; i < app::g_selectedPointsImageSpace.size(); ++i) {
        const Gdiplus::PointF point = ImagePointToClientPoint(info, app::g_selectedPointsImageSpace[i]);
        const float dx = point.X - static_cast<float>(clientX);
        const float dy = point.Y - static_cast<float>(clientY);
        if ((dx * dx + dy * dy) <= hitRadiusSquared) {
            return static_cast<int>(i);
        }
    }
    return -1;
}

}  // namespace app::view

