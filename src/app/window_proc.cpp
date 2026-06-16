#include "window_proc.h"

#include <windows.h>

#include "corrector.h"
#include "edit_apply.h"
#include "image_io.h"
#include "input_handlers.h"
#include "render.h"
#include "state.h"
#include "ui.h"

namespace {
using app::g_draggingPointIndex;
using app::g_ellipse;
using app::g_ellipseDragMode;
using app::g_modeButton;
using app::g_selectedPointsImageSpace;
using app::g_selectionMode;

using app::EllipseDragMode;
using app::SelectionMode;

using app::kMenuExportCorrectedId;
using app::kMenuImportPictureId;
using app::kModeToggleButtonId;
using app::kMsgApplyEditValue;
using app::kResetButtonId;
}  // namespace

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_CREATE:
            app::ui::CreateOrUpdateResetAndModeButtons(hwnd);
            app::ui::CreateOrUpdateSliders(hwnd);
            app::ui::CreateOrUpdateBrightnessControls(hwnd);
            return 0;

        case WM_COMMAND: {
            const WORD commandId = LOWORD(wParam);
            const WORD notifyCode = HIWORD(wParam);
            if (commandId == kMenuImportPictureId) {
                if (app::io::OpenAndLoadImage(hwnd)) {
                    app::ui::InvalidateImageAreas(hwnd, true, true);
                }
                return 0;
            }
            if (commandId == kMenuExportCorrectedId) {
                app::io::SaveCorrectedImageWithDialog(hwnd);
                return 0;
            }
            if (commandId == kModeToggleButtonId) {
                g_selectionMode = (g_selectionMode == SelectionMode::Quad) ? SelectionMode::Ellipse : SelectionMode::Quad;
                g_selectedPointsImageSpace.clear();
                g_draggingPointIndex = -1;
                g_ellipse = {};
                g_ellipseDragMode = EllipseDragMode::None;
                app::corrector::UpdateCorrectedPreview();
                app::ui::CreateOrUpdateResetAndModeButtons(hwnd);  // refresh label
                app::ui::InvalidateImageAreas(hwnd, true, true);
                return 0;
            }
            if (commandId == kResetButtonId) {
                g_selectedPointsImageSpace.clear();
                g_draggingPointIndex = -1;
                ReleaseCapture();
                g_ellipse = {};
                g_ellipseDragMode = EllipseDragMode::None;
                app::corrector::UpdateCorrectedPreview();
                app::ui::InvalidateImageAreas(hwnd, true, true);
                return 0;
            }

            if (notifyCode == EN_KILLFOCUS) {
                if (app::edit::ApplyNumericEditValue(hwnd, commandId)) {
                    return 0;
                }
            }
            break;
        }

        case kMsgApplyEditValue:
            if (app::edit::ApplyNumericEditValue(hwnd, static_cast<UINT>(wParam))) {
                return 0;
            }
            break;

        case WM_LBUTTONDOWN:
        case WM_LBUTTONUP:
        case WM_MOUSEMOVE:
        case WM_MOUSEWHEEL:
        case WM_MBUTTONDOWN:
        case WM_MBUTTONUP:
        case WM_HSCROLL:
        case WM_CAPTURECHANGED: {
            LRESULT result = 0;
            if (app::input::HandleMessage(hwnd, msg, wParam, lParam, &result)) {
                return result;
            }
            break;
        }

        case WM_SIZE:
            app::ui::CreateOrUpdateResetAndModeButtons(hwnd);
            app::ui::CreateOrUpdateSliders(hwnd);
            app::ui::CreateOrUpdateBrightnessControls(hwnd);
            InvalidateRect(hwnd, nullptr, FALSE);
            return 0;

        case WM_ERASEBKGND:
            return 1;

        case WM_PAINT: {
            PAINTSTRUCT ps = {};
            HDC hdc = BeginPaint(hwnd, &ps);
            app::render::DrawLoadedImage(hwnd, hdc);
            EndPaint(hwnd, &ps);
            return 0;
        }

        case WM_DESTROY:
            if (g_draggingPointIndex >= 0) {
                ReleaseCapture();
            }
            g_draggingPointIndex = -1;
            g_modeButton = nullptr;
            PostQuitMessage(0);
            return 0;

        default:
            break;
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

