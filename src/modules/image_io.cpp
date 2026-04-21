#include "image_io.h"

#include <commdlg.h>
#include <gdiplus.h>

#include <memory>
#include <string>

#include "corrector.h"
#include "state.h"

using Gdiplus::Image;
using Gdiplus::Status;

namespace {
using app::g_draggingPointIndex;
using app::g_ellipse;
using app::g_ellipseDragMode;
using app::g_isPanning;
using app::g_leftPanX;
using app::g_leftPanY;
using app::g_leftZoom;
using app::g_loadedImage;
using app::g_selectedPointsImageSpace;

using app::EllipseDragMode;

void ShowErrorMessage(HWND hwnd, const wchar_t* message) {
    MessageBoxW(hwnd, message, L"Error", MB_OK | MB_ICONERROR);
}

bool LoadImageFromPath(const std::wstring& path) {
    auto image = std::make_unique<Image>(path.c_str());
    if (image->GetLastStatus() != Status::Ok) {
        return false;
    }
    g_loadedImage = std::move(image);
    return true;
}
}  // namespace

bool OpenAndLoadImage(HWND hwnd) {
    wchar_t filePath[MAX_PATH] = {0};
    OPENFILENAMEW ofn = {};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hwnd;
    ofn.lpstrFile = filePath;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrFilter =
        L"Image Files (*.png;*.jpg;*.jpeg)\0*.png;*.jpg;*.jpeg\0"
        L"PNG Files (*.png)\0*.png\0"
        L"JPEG Files (*.jpg;*.jpeg)\0*.jpg;*.jpeg\0"
        L"All Files (*.*)\0*.*\0";
    ofn.nFilterIndex = 1;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
    ofn.lpstrDefExt = L"png";

    if (!GetOpenFileNameW(&ofn)) {
        return false;
    }

    if (!LoadImageFromPath(filePath)) {
        ShowErrorMessage(hwnd, L"Failed to load image. Please choose a valid PNG or JPEG file.");
        return false;
    }

    g_selectedPointsImageSpace.clear();
    g_draggingPointIndex = -1;
    g_leftZoom = 1.0;
    g_leftPanX = 0;
    g_leftPanY = 0;
    g_isPanning = false;
    g_ellipse = {};
    g_ellipseDragMode = EllipseDragMode::None;
    app::corrector::UpdateCorrectedPreview();
    return true;
}

