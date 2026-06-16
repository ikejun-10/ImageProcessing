#include "image_io.h"

#include <commdlg.h>
#include <gdiplus.h>

#include <memory>
#include <string>
#include <vector>

#include "corrector.h"
#include "state.h"

namespace app::io {

using Gdiplus::Bitmap;
using Gdiplus::GetImageEncoders;
using Gdiplus::GetImageEncodersSize;
using Gdiplus::Graphics;
using Gdiplus::Image;
using Gdiplus::ImageCodecInfo;
using Gdiplus::Status;

namespace {

void ShowErrorMessage(HWND hwnd, const wchar_t* message) {
    MessageBoxW(hwnd, message, L"Error", MB_OK | MB_ICONERROR);
}

bool LoadImageFromPath(const std::wstring& path) {
    auto image = std::make_unique<Image>(path.c_str());
    if (image->GetLastStatus() != Status::Ok) {
        return false;
    }
    app::g_loadedImage = std::move(image);
    return true;
}

bool GetEncoderClsid(const wchar_t* mimeType, CLSID* outClsid) {
    if (!mimeType || !outClsid) {
        return false;
    }
    UINT num = 0;
    UINT size = 0;
    GetImageEncodersSize(&num, &size);
    if (size == 0) {
        return false;
    }
    std::vector<BYTE> buffer(size);
    ImageCodecInfo* codecs = reinterpret_cast<ImageCodecInfo*>(buffer.data());
    if (GetImageEncoders(num, size, codecs) != Gdiplus::Ok) {
        return false;
    }
    for (UINT i = 0; i < num; ++i) {
        if (codecs[i].MimeType && wcscmp(codecs[i].MimeType, mimeType) == 0) {
            *outClsid = codecs[i].Clsid;
            return true;
        }
    }
    return false;
}

std::wstring ToLower(std::wstring s) {
    for (auto& ch : s) {
        if (ch >= L'A' && ch <= L'Z') {
            ch = static_cast<wchar_t>(ch - L'A' + L'a');
        }
    }
    return s;
}

std::wstring GetFileExtensionLower(const std::wstring& path) {
    const size_t dot = path.find_last_of(L'.');
    if (dot == std::wstring::npos) {
        return L"";
    }
    return ToLower(path.substr(dot));
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

    app::g_selectedPointsImageSpace.clear();
    app::g_draggingPointIndex = -1;
    app::g_leftZoom = 1.0;
    app::g_leftPanX = 0;
    app::g_leftPanY = 0;
    app::g_isPanning = false;
    app::g_ellipse = {};
    app::g_ellipseDragMode = app::EllipseDragMode::None;
    app::corrector::UpdateCorrectedPreview();
    return true;
}

bool SaveCorrectedImageWithDialog(HWND hwnd) {
    if (!app::g_correctedImage) {
        MessageBoxW(hwnd, L"No corrected image to export yet.", L"Error", MB_OK | MB_ICONERROR);
        return false;
    }

    wchar_t filePath[MAX_PATH] = {0};
    OPENFILENAMEW ofn = {};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hwnd;
    ofn.lpstrFile = filePath;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrFilter =
        L"PNG (*.png)\0*.png\0"
        L"JPEG (*.jpg;*.jpeg)\0*.jpg;*.jpeg\0";
    ofn.nFilterIndex = 1;
    ofn.Flags = OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST;
    ofn.lpstrDefExt = L"png";

    if (!GetSaveFileNameW(&ofn)) {
        return false;
    }

    std::wstring path = filePath;
    const std::wstring ext = GetFileExtensionLower(path);
    const wchar_t* mime = L"image/png";
    if (ext == L".jpg" || ext == L".jpeg") {
        mime = L"image/jpeg";
    }

    CLSID clsid;
    if (!GetEncoderClsid(mime, &clsid)) {
        MessageBoxW(hwnd, L"Failed to find image encoder.", L"Error", MB_OK | MB_ICONERROR);
        return false;
    }

    if (wcscmp(mime, L"image/jpeg") == 0) {
        const int w = static_cast<int>(app::g_correctedImage->GetWidth());
        const int h = static_cast<int>(app::g_correctedImage->GetHeight());
        Bitmap flattened(w, h, PixelFormat32bppPARGB);
        Graphics g(&flattened);
        g.Clear(Gdiplus::Color(255, 0, 0, 0));
        g.DrawImage(app::g_correctedImage.get(), 0, 0);
        if (flattened.Save(path.c_str(), &clsid, nullptr) != Gdiplus::Ok) {
            MessageBoxW(hwnd, L"Failed to save image.", L"Error", MB_OK | MB_ICONERROR);
            return false;
        }
        return true;
    }

    if (app::g_correctedImage->Save(path.c_str(), &clsid, nullptr) != Gdiplus::Ok) {
        MessageBoxW(hwnd, L"Failed to save image.", L"Error", MB_OK | MB_ICONERROR);
        return false;
    }
    return true;
}

}  // namespace app::io

