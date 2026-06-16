#pragma once

#include <windows.h>
#include <gdiplus.h>

namespace app::render {

void DrawLoadedImage(HWND hwnd, HDC hdc);
void DrawSelectionOverlay(HWND hwnd, Gdiplus::Graphics* graphics);

void DrawImageWithAdjustments(Gdiplus::Graphics* g, Gdiplus::Image* image, const Gdiplus::Rect& destRect);
void DrawImageWithAdjustmentsAndRotation(
    Gdiplus::Graphics* g,
    Gdiplus::Image* image,
    const Gdiplus::Rect& destRect,
    float angleDeg,
    float pivotX,
    float pivotY);

}  // namespace app::render

