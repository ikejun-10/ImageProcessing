#pragma once

#include <windows.h>
#include <gdiplus.h>

#include "state.h"

namespace app::view {

// Image display geometry (left pane)
app::ImageDisplayInfo FitImageToPane(const Gdiplus::Rect& paneRect, Gdiplus::Image* image);
app::ImageDisplayInfo GetLeftImageDisplayInfo(HWND hwnd);

// Coordinate transforms (left pane)
Gdiplus::PointF ImagePointToClientPoint(const app::ImageDisplayInfo& info, const Gdiplus::PointF& imagePoint);
bool ClientPointToImagePoint(HWND hwnd, int clientX, int clientY, Gdiplus::PointF* outImagePoint);
bool ClientPointToImagePointClamped(HWND hwnd, int clientX, int clientY, Gdiplus::PointF* outImagePoint);

// Hit testing
int HitTestControlPointIndex(HWND hwnd, int clientX, int clientY);
bool PointInEllipseClientSpace(const app::EllipseParams& e, const app::ImageDisplayInfo& info, int clientX, int clientY);

}  // namespace app::view

