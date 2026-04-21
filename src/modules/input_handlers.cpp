#include "input_handlers.h"

#include <windowsx.h>

#include <algorithm>
#include <cmath>
#include <string>

#include "corrector.h"
#include "state.h"
#include "ui.h"
#include "view.h"

using Gdiplus::PointF;

namespace {
using app::g_brightness;
using app::g_brightnessSlider;
using app::g_draggingPointIndex;
using app::g_ellipse;
using app::g_ellipseDragMode;
using app::g_ellipseDrawStart;
using app::g_ellipseMoveOffset;
using app::g_ellipseRotateOffset;
using app::g_isPanning;
using app::g_lastPanPoint;
using app::g_leftPanX;
using app::g_leftPanY;
using app::g_leftZoom;
using app::g_loadedImage;
using app::g_outputHeight;
using app::g_outputRotateDeg;
using app::g_outputWidth;
using app::g_selectedPointsImageSpace;
using app::g_selectionMode;

using app::EllipseDragMode;
using app::ImageDisplayInfo;
using app::SelectionMode;
using app::ViewLayout;

static void UpdateColorLabels() {
    if (app::g_brightnessLabel) {
        SetWindowTextW(app::g_brightnessLabel, (L"Brightness: " + std::to_wstring(app::g_brightness - 100)).c_str());
    }
    if (app::g_brightnessEdit) {
        SetWindowTextW(app::g_brightnessEdit, std::to_wstring(app::g_brightness - 100).c_str());
    }
}

static bool PointInEllipseClientSpace(const app::EllipseParams& e, const ImageDisplayInfo& info, int clientX, int clientY) {
    if (!e.valid || !info.valid) {
        return false;
    }
    const double cx = static_cast<double>(info.rect.X) + e.cx * info.scale;
    const double cy = static_cast<double>(info.rect.Y) + e.cy * info.scale;
    const double a = e.a * info.scale;
    const double b = e.b * info.scale;
    if (a < 1.0 || b < 1.0) {
        return false;
    }
    const double dx = static_cast<double>(clientX) - cx;
    const double dy = static_cast<double>(clientY) - cy;
    const double ct = std::cos(e.theta);
    const double st = std::sin(e.theta);
    const double lx = dx * ct + dy * st;
    const double ly = -dx * st + dy * ct;
    const double nx = lx / a;
    const double ny = ly / b;
    return (nx * nx + ny * ny) <= 1.0;
}

static EllipseDragMode HitTestEllipseHandle(HWND hwnd, int clientX, int clientY) {
    if (g_selectionMode != SelectionMode::Ellipse || !g_ellipse.valid) {
        return EllipseDragMode::None;
    }
    const ImageDisplayInfo info = app::view::GetLeftImageDisplayInfo(hwnd);
    if (!info.valid) {
        return EllipseDragMode::None;
    }

    const double cx = static_cast<double>(info.rect.X) + g_ellipse.cx * info.scale;
    const double cy = static_cast<double>(info.rect.Y) + g_ellipse.cy * info.scale;
    const double a = g_ellipse.a * info.scale;
    const double b = g_ellipse.b * info.scale;
    const double ct = std::cos(g_ellipse.theta);
    const double st = std::sin(g_ellipse.theta);

    auto dist2 = [&](double x, double y) {
        const double dx = x - clientX;
        const double dy = y - clientY;
        return dx * dx + dy * dy;
    };
    const double r = 10.0;
    const double r2 = r * r;

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

    if (dist2(hrot_x, hrot_y) <= r2) return EllipseDragMode::Rotate;
    if (dist2(hxL_x, hxL_y) <= r2) return EllipseDragMode::ResizeLeft;
    if (dist2(hxR_x, hxR_y) <= r2) return EllipseDragMode::ResizeRight;
    if (dist2(hyT_x, hyT_y) <= r2) return EllipseDragMode::ResizeTop;
    if (dist2(hyB_x, hyB_y) <= r2) return EllipseDragMode::ResizeBottom;
    return EllipseDragMode::None;
}

static bool ClientToImageInLeftPane(HWND hwnd, int clientX, int clientY, PointF* outImagePoint) {
    return app::view::ClientPointToImagePoint(hwnd, clientX, clientY, outImagePoint);
}

static bool ClientPointToImagePointClamped(HWND hwnd, int clientX, int clientY, PointF* outImagePoint) {
    return app::view::ClientPointToImagePointClamped(hwnd, clientX, clientY, outImagePoint);
}

static int HitTestControlPointIndex(HWND hwnd, int clientX, int clientY) {
    return app::view::HitTestControlPointIndex(hwnd, clientX, clientY);
}
}  // namespace

namespace app::input {

bool HandleMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, LRESULT* outResult) {
    if (!outResult) return false;

    switch (msg) {
        case WM_LBUTTONDOWN: {
            if (!g_loadedImage) {
                *outResult = 0;
                return true;
            }

            const int mouseX = GET_X_LPARAM(lParam);
            const int mouseY = GET_Y_LPARAM(lParam);
            const ViewLayout layout = app::ui::GetViewLayout(hwnd);
            const RECT leftPaneRect = app::ui::ToWinRect(layout.leftImagePane);
            if (!PtInRect(&leftPaneRect, POINT{mouseX, mouseY})) {
                *outResult = 0;
                return true;
            }

            if (g_selectionMode == SelectionMode::Ellipse) {
                const EllipseDragMode handle = HitTestEllipseHandle(hwnd, mouseX, mouseY);
                PointF imgPt;
                if (!ClientToImageInLeftPane(hwnd, mouseX, mouseY, &imgPt)) {
                    *outResult = 0;
                    return true;
                }

                if (handle != EllipseDragMode::None) {
                    g_ellipseDragMode = handle;
                    if (handle == EllipseDragMode::Rotate) {
                        const double ang =
                            std::atan2(static_cast<double>(imgPt.Y - g_ellipse.cy), static_cast<double>(imgPt.X - g_ellipse.cx));
                        g_ellipseRotateOffset = g_ellipse.theta - ang;
                    }
                    SetCapture(hwnd);
                    *outResult = 0;
                    return true;
                }

                if (g_ellipse.valid && PointInEllipseClientSpace(g_ellipse, app::view::GetLeftImageDisplayInfo(hwnd), mouseX, mouseY)) {
                    g_ellipseDragMode = EllipseDragMode::Move;
                    g_ellipseMoveOffset = PointF(static_cast<float>(imgPt.X - g_ellipse.cx), static_cast<float>(imgPt.Y - g_ellipse.cy));
                    SetCapture(hwnd);
                    *outResult = 0;
                    return true;
                }

                g_ellipseDragMode = EllipseDragMode::Draw;
                g_ellipseDrawStart = imgPt;
                g_ellipse.valid = true;
                g_ellipse.cx = imgPt.X;
                g_ellipse.cy = imgPt.Y;
                g_ellipse.a = 1.0;
                g_ellipse.b = 1.0;
                g_ellipse.theta = 0.0;
                SetCapture(hwnd);
                app::ui::InvalidateImageAreas(hwnd, true, false);
                *outResult = 0;
                return true;
            }

            const int hitPointIndex = HitTestControlPointIndex(hwnd, mouseX, mouseY);
            if (hitPointIndex >= 0) {
                g_draggingPointIndex = hitPointIndex;
                SetCapture(hwnd);
                *outResult = 0;
                return true;
            }
            if (g_selectedPointsImageSpace.size() >= 4) {
                *outResult = 0;
                return true;
            }
            PointF imagePoint;
            if (app::view::ClientPointToImagePoint(hwnd, mouseX, mouseY, &imagePoint)) {
                g_selectedPointsImageSpace.push_back(imagePoint);
                app::corrector::UpdateCorrectedPreview();
                app::ui::InvalidateImageAreas(hwnd, true, true);
            }
            *outResult = 0;
            return true;
        }
        case WM_MOUSEWHEEL: {
            if (!g_loadedImage) {
                *outResult = 0;
                return true;
            }

            POINT pt = {GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
            ScreenToClient(hwnd, &pt);

            const ViewLayout layout = app::ui::GetViewLayout(hwnd);
            const RECT leftPaneRect = app::ui::ToWinRect(layout.leftImagePane);
            if (!PtInRect(&leftPaneRect, pt)) {
                *outResult = 0;
                return true;
            }

            const int delta = GET_WHEEL_DELTA_WPARAM(wParam);
            if (delta == 0) {
                *outResult = 0;
                return true;
            }

            const ImageDisplayInfo oldInfo = app::view::GetLeftImageDisplayInfo(hwnd);
            if (!oldInfo.valid) {
                *outResult = 0;
                return true;
            }

            const double imgX = (static_cast<double>(pt.x - oldInfo.rect.X)) / oldInfo.scale;
            const double imgY = (static_cast<double>(pt.y - oldInfo.rect.Y)) / oldInfo.scale;

            const double step = 1.1;
            const double factor = std::pow(step, static_cast<double>(delta) / 120.0);
            g_leftZoom = std::clamp(g_leftZoom * factor, 0.1, 20.0);

            const ImageDisplayInfo newInfo = app::view::GetLeftImageDisplayInfo(hwnd);
            if (!newInfo.valid) {
                *outResult = 0;
                return true;
            }

            const ViewLayout layout2 = app::ui::GetViewLayout(hwnd);
            const int newDrawW = static_cast<int>(newInfo.imageWidth * newInfo.scale);
            const int newDrawH = static_cast<int>(newInfo.imageHeight * newInfo.scale);
            const int centeredX = layout2.leftImagePane.X + (layout2.leftImagePane.Width - newDrawW) / 2;
            const int centeredY = layout2.leftImagePane.Y + (layout2.leftImagePane.Height - newDrawH) / 2;

            const int desiredX = static_cast<int>(std::round(static_cast<double>(pt.x) - imgX * newInfo.scale));
            const int desiredY = static_cast<int>(std::round(static_cast<double>(pt.y) - imgY * newInfo.scale));

            g_leftPanX = desiredX - centeredX;
            g_leftPanY = desiredY - centeredY;

            app::ui::InvalidateImageAreas(hwnd, true, false);
            *outResult = 0;
            return true;
        }
        case WM_MBUTTONDOWN: {
            if (!g_loadedImage) {
                *outResult = 0;
                return true;
            }
            POINT pt = {GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
            const ViewLayout layout = app::ui::GetViewLayout(hwnd);
            const RECT leftPaneRect = app::ui::ToWinRect(layout.leftImagePane);
            if (!PtInRect(&leftPaneRect, pt)) {
                *outResult = 0;
                return true;
            }
            g_isPanning = true;
            g_lastPanPoint = pt;
            SetCapture(hwnd);
            *outResult = 0;
            return true;
        }
        case WM_MBUTTONUP:
            if (g_isPanning) {
                g_isPanning = false;
                ReleaseCapture();
                *outResult = 0;
                return true;
            }
            return false;
        case WM_MOUSEMOVE: {
            if (g_isPanning && g_loadedImage) {
                POINT pt = {GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
                const int dx = pt.x - g_lastPanPoint.x;
                const int dy = pt.y - g_lastPanPoint.y;
                g_leftPanX += dx;
                g_leftPanY += dy;
                g_lastPanPoint = pt;
                app::ui::InvalidateImageAreas(hwnd, true, false);
                *outResult = 0;
                return true;
            }

            if (g_selectionMode == SelectionMode::Ellipse && g_ellipseDragMode != EllipseDragMode::None && g_loadedImage) {
                const int mx = GET_X_LPARAM(lParam);
                const int my = GET_Y_LPARAM(lParam);
                PointF imgPt;
                if (!ClientPointToImagePointClamped(hwnd, mx, my, &imgPt)) {
                    *outResult = 0;
                    return true;
                }

                if (g_ellipseDragMode == EllipseDragMode::Draw) {
                    const double x0 = g_ellipseDrawStart.X;
                    const double y0 = g_ellipseDrawStart.Y;
                    const double x1 = imgPt.X;
                    const double y1 = imgPt.Y;
                    g_ellipse.cx = (x0 + x1) * 0.5;
                    g_ellipse.cy = (y0 + y1) * 0.5;
                    g_ellipse.a = (std::max)(1.0, std::fabs(x1 - x0) * 0.5);
                    g_ellipse.b = (std::max)(1.0, std::fabs(y1 - y0) * 0.5);
                    g_ellipse.theta = 0.0;
                    app::corrector::UpdateCorrectedPreview();
                    app::ui::InvalidateImageAreas(hwnd, true, true);
                    *outResult = 0;
                    return true;
                }

                if (g_ellipseDragMode == EllipseDragMode::Move) {
                    g_ellipse.cx = imgPt.X - g_ellipseMoveOffset.X;
                    g_ellipse.cy = imgPt.Y - g_ellipseMoveOffset.Y;
                    app::corrector::UpdateCorrectedPreview();
                    app::ui::InvalidateImageAreas(hwnd, true, true);
                    *outResult = 0;
                    return true;
                }

                if (g_ellipseDragMode == EllipseDragMode::Rotate) {
                    const double ang =
                        std::atan2(static_cast<double>(imgPt.Y - g_ellipse.cy), static_cast<double>(imgPt.X - g_ellipse.cx));
                    g_ellipse.theta = ang + g_ellipseRotateOffset;
                    app::corrector::UpdateCorrectedPreview();
                    app::ui::InvalidateImageAreas(hwnd, true, true);
                    *outResult = 0;
                    return true;
                }

                const double ct = std::cos(g_ellipse.theta);
                const double st = std::sin(g_ellipse.theta);
                const double dx = static_cast<double>(imgPt.X) - g_ellipse.cx;
                const double dy = static_cast<double>(imgPt.Y) - g_ellipse.cy;
                const double lx = dx * ct + dy * st;
                const double ly = -dx * st + dy * ct;

                if (g_ellipseDragMode == EllipseDragMode::ResizeLeft || g_ellipseDragMode == EllipseDragMode::ResizeRight) {
                    g_ellipse.a = (std::max)(1.0, std::fabs(lx));
                    app::corrector::UpdateCorrectedPreview();
                    app::ui::InvalidateImageAreas(hwnd, true, true);
                    *outResult = 0;
                    return true;
                }
                if (g_ellipseDragMode == EllipseDragMode::ResizeTop || g_ellipseDragMode == EllipseDragMode::ResizeBottom) {
                    g_ellipse.b = (std::max)(1.0, std::fabs(ly));
                    app::corrector::UpdateCorrectedPreview();
                    app::ui::InvalidateImageAreas(hwnd, true, true);
                    *outResult = 0;
                    return true;
                }
                *outResult = 0;
                return true;
            }

            if (g_draggingPointIndex < 0) {
                *outResult = 0;
                return true;
            }
            if ((wParam & MK_LBUTTON) == 0) {
                *outResult = 0;
                return true;
            }

            PointF imagePoint;
            if (ClientPointToImagePointClamped(hwnd, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam), &imagePoint)) {
                g_selectedPointsImageSpace[static_cast<size_t>(g_draggingPointIndex)] = imagePoint;
                app::corrector::UpdateCorrectedPreview();
                app::ui::InvalidateImageAreas(hwnd, true, true);
            }
            *outResult = 0;
            return true;
        }
        case WM_LBUTTONUP:
            if (g_selectionMode == SelectionMode::Ellipse && g_ellipseDragMode != EllipseDragMode::None) {
                g_ellipseDragMode = EllipseDragMode::None;
                ReleaseCapture();
                app::corrector::UpdateCorrectedPreview();
                app::ui::InvalidateImageAreas(hwnd, true, true);
                *outResult = 0;
                return true;
            }
            if (g_draggingPointIndex >= 0) {
                g_draggingPointIndex = -1;
                ReleaseCapture();
                app::corrector::UpdateCorrectedPreview();
                app::ui::InvalidateImageAreas(hwnd, true, true);
            }
            *outResult = 0;
            return true;
        case WM_CAPTURECHANGED:
            g_draggingPointIndex = -1;
            g_isPanning = false;
            g_ellipseDragMode = EllipseDragMode::None;
            *outResult = 0;
            return true;
        case WM_HSCROLL: {
            if (reinterpret_cast<HWND>(lParam) == app::g_widthSlider) {
                g_outputWidth = static_cast<int>(SendMessageW(app::g_widthSlider, TBM_GETPOS, 0, 0));
                app::ui::UpdateSliderLabels();
                app::corrector::UpdateCorrectedPreview();
                app::ui::InvalidateImageAreas(hwnd, false, true);
                *outResult = 0;
                return true;
            }
            if (reinterpret_cast<HWND>(lParam) == app::g_heightSlider) {
                g_outputHeight = static_cast<int>(SendMessageW(app::g_heightSlider, TBM_GETPOS, 0, 0));
                app::ui::UpdateSliderLabels();
                app::corrector::UpdateCorrectedPreview();
                app::ui::InvalidateImageAreas(hwnd, false, true);
                *outResult = 0;
                return true;
            }
            if (reinterpret_cast<HWND>(lParam) == app::g_rotateSlider) {
                g_outputRotateDeg = static_cast<int>(SendMessageW(app::g_rotateSlider, TBM_GETPOS, 0, 0));
                app::ui::UpdateSliderLabels();
                app::ui::InvalidateImageAreas(hwnd, false, true);
                *outResult = 0;
                return true;
            }
            if (reinterpret_cast<HWND>(lParam) == g_brightnessSlider) {
                g_brightness = static_cast<int>(SendMessageW(g_brightnessSlider, TBM_GETPOS, 0, 0));
                UpdateColorLabels();
                app::ui::InvalidateImageAreas(hwnd, true, true);
                *outResult = 0;
                return true;
            }
            return false;
        }
        default:
            return false;
    }
}

}  // namespace app::input

