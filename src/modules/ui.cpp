#include "ui.h"

#include "state.h"

#include <commdlg.h>
#include <gdiplus.h>

#include <algorithm>
#include <string>
#include <vector>

using Gdiplus::GetImageEncoders;
using Gdiplus::GetImageEncodersSize;
using Gdiplus::ImageCodecInfo;

namespace app::ui {

app::ViewLayout GetViewLayout(HWND hwnd) {
    RECT clientRect = {};
    GetClientRect(hwnd, &clientRect);
    const int clientWidth = clientRect.right - clientRect.left;
    const int clientHeight = clientRect.bottom - clientRect.top;
    if (clientWidth <= 0 || clientHeight <= 0) {
        return {};
    }

    const int splitX = clientWidth / 2;
    app::ViewLayout layout;
    layout.leftPane = Gdiplus::Rect(0, 0, splitX, clientHeight);
    layout.rightPane = Gdiplus::Rect(splitX, 0, clientWidth - splitX, clientHeight);

    constexpr int kTopBarHeight = 48;
    layout.topBar = Gdiplus::Rect(0, 0, clientWidth, kTopBarHeight);

    const int rightControlsHeight =
        (app::kSliderHeight * 3) + (app::kSliderGap * 2) + (app::kSliderMargin * 2) + 18 * 3;  // 3 labels/sliders
    const int leftControlsHeight = (app::kSliderMargin * 2) + (18 + app::kSliderHeight);        // 1 label/slider (brightness)

    const int leftAvailH = (std::max)(0, layout.leftPane.Height - layout.topBar.Height);
    const int leftImageHeight = (leftAvailH > leftControlsHeight) ? (leftAvailH - leftControlsHeight) : leftAvailH;
    layout.leftImagePane = Gdiplus::Rect(layout.leftPane.X, layout.leftPane.Y + layout.topBar.Height, layout.leftPane.Width, leftImageHeight);
    layout.leftControlsPane =
        Gdiplus::Rect(layout.leftPane.X, layout.leftImagePane.Y + leftImageHeight, layout.leftPane.Width, leftAvailH - leftImageHeight);

    const int rightAvailH = (std::max)(0, layout.rightPane.Height - layout.topBar.Height);
    const int rightImageHeight = (rightAvailH > rightControlsHeight) ? (rightAvailH - rightControlsHeight) : rightAvailH;
    layout.rightImagePane =
        Gdiplus::Rect(layout.rightPane.X, layout.rightPane.Y + layout.topBar.Height, layout.rightPane.Width, rightImageHeight);
    layout.rightControlsPane = Gdiplus::Rect(
        layout.rightPane.X, layout.rightImagePane.Y + rightImageHeight, layout.rightPane.Width, rightAvailH - rightImageHeight);

    return layout;
}

RECT ToWinRect(const Gdiplus::Rect& r) {
    RECT rc;
    rc.left = r.X;
    rc.top = r.Y;
    rc.right = r.X + r.Width;
    rc.bottom = r.Y + r.Height;
    return rc;
}

void InvalidateImageAreas(HWND hwnd, bool left, bool right) {
    const auto layout = GetViewLayout(hwnd);
    if (left) {
        RECT rc = ToWinRect(layout.leftImagePane);
        InvalidateRect(hwnd, &rc, FALSE);
    }
    if (right) {
        RECT rc = ToWinRect(layout.rightImagePane);
        InvalidateRect(hwnd, &rc, FALSE);
    }
}

static bool GetEncoderClsid(const wchar_t* mimeType, CLSID* outClsid) {
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

static std::wstring ToLower(std::wstring s) {
    for (auto& ch : s) {
        if (ch >= L'A' && ch <= L'Z') {
            ch = static_cast<wchar_t>(ch - L'A' + L'a');
        }
    }
    return s;
}

static std::wstring GetFileExtensionLower(const std::wstring& path) {
    const size_t dot = path.find_last_of(L'.');
    if (dot == std::wstring::npos) {
        return L"";
    }
    return ToLower(path.substr(dot));
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
        Gdiplus::Bitmap flattened(w, h, PixelFormat32bppPARGB);
        Gdiplus::Graphics g(&flattened);
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

HMENU CreateMainMenu() {
    HMENU mainMenu = CreateMenu();
    HMENU fileMenu = CreatePopupMenu();
    AppendMenuW(fileMenu, MF_STRING, app::kMenuImportPictureId, L"Import Picture...");
    AppendMenuW(fileMenu, MF_STRING, app::kMenuExportCorrectedId, L"Export Corrected...");
    AppendMenuW(mainMenu, MF_POPUP, reinterpret_cast<UINT_PTR>(fileMenu), L"File");
    return mainMenu;
}

void CreateOrUpdateResetAndModeButtons(HWND hwnd) {
    if (!app::g_resetButton) {
        app::g_resetButton = CreateWindowW(
            L"BUTTON",
            L"Reset Points",
            WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
            10,
            10,
            110,
            28,
            hwnd,
            reinterpret_cast<HMENU>(app::kResetButtonId),
            reinterpret_cast<HINSTANCE>(GetWindowLongPtrW(hwnd, GWLP_HINSTANCE)),
            nullptr);
    }
    if (app::g_resetButton) {
        SetWindowPos(app::g_resetButton, HWND_TOP, 10, 10, 110, 28, SWP_NOZORDER);
    }

    if (!app::g_modeButton) {
        app::g_modeButton = CreateWindowW(
            L"BUTTON",
            L"Mode: Quad",
            WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
            130,
            10,
            120,
            28,
            hwnd,
            reinterpret_cast<HMENU>(app::kModeToggleButtonId),
            reinterpret_cast<HINSTANCE>(GetWindowLongPtrW(hwnd, GWLP_HINSTANCE)),
            nullptr);
    }
    if (app::g_modeButton) {
        const wchar_t* label = (app::g_selectionMode == app::SelectionMode::Ellipse) ? L"Mode: Ellipse" : L"Mode: Quad";
        SetWindowTextW(app::g_modeButton, label);
        SetWindowPos(app::g_modeButton, HWND_TOP, 130, 10, 120, 28, SWP_NOZORDER);
    }
}

bool TryReadIntFromEdit(HWND edit, int* outValue) {
    if (!edit || !outValue) return false;
    wchar_t buf[64] = {0};
    GetWindowTextW(edit, buf, 63);
    wchar_t* end = nullptr;
    const long v = wcstol(buf, &end, 10);
    if (end == buf) return false;
    *outValue = static_cast<int>(v);
    return true;
}

LRESULT CALLBACK EditEnterSubclassProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR, DWORD_PTR refData) {
    if (msg == WM_KEYDOWN && wParam == VK_RETURN) {
        const UINT editId = static_cast<UINT>(refData);
        PostMessageW(GetParent(hwnd), app::kMsgApplyEditValue, editId, 0);
        return 0;
    }
    return DefSubclassProc(hwnd, msg, wParam, lParam);
}

void UpdateSliderLabels() {
    if (app::g_widthLabel) {
        std::wstring t = L"Width: " + std::to_wstring(app::g_outputWidth);
        SetWindowTextW(app::g_widthLabel, t.c_str());
    }
    if (app::g_heightLabel) {
        std::wstring t = L"Height: " + std::to_wstring(app::g_outputHeight);
        SetWindowTextW(app::g_heightLabel, t.c_str());
    }
    if (app::g_rotateLabel) {
        std::wstring t = L"Rotate: " + std::to_wstring(app::g_outputRotateDeg) + L"°";
        SetWindowTextW(app::g_rotateLabel, t.c_str());
    }
    if (app::g_widthEdit) SetWindowTextW(app::g_widthEdit, std::to_wstring(app::g_outputWidth).c_str());
    if (app::g_heightEdit) SetWindowTextW(app::g_heightEdit, std::to_wstring(app::g_outputHeight).c_str());
    if (app::g_rotateEdit) SetWindowTextW(app::g_rotateEdit, std::to_wstring(app::g_outputRotateDeg).c_str());
}

void UpdateBrightnessLabels() {
    if (app::g_brightnessLabel) {
        SetWindowTextW(app::g_brightnessLabel, (L"Brightness: " + std::to_wstring(app::g_brightness - 100)).c_str());
    }
    if (app::g_brightnessEdit) {
        SetWindowTextW(app::g_brightnessEdit, std::to_wstring(app::g_brightness - 100).c_str());
    }
}

void CreateOrUpdateSliders(HWND hwnd) {
    const auto layout = GetViewLayout(hwnd);
    const int paneX = layout.rightControlsPane.X;
    const int paneY = layout.rightControlsPane.Y;
    const int paneW = layout.rightControlsPane.Width;
    const int paneH = layout.rightControlsPane.Height;
    if (paneW <= 0 || paneH <= 0) return;

    const int x = paneX + app::kSliderMargin;
    const int editW = 72;
    const int w = (std::max)(0, paneW - app::kSliderMargin * 2);
    const int sliderW = (std::max)(0, w - editW - 8);
    int y = paneY + app::kSliderMargin;

    if (!app::g_widthLabel) {
        app::g_widthLabel = CreateWindowW(L"STATIC", L"Width:", WS_VISIBLE | WS_CHILD, x, y, w, 18, hwnd,
                                          reinterpret_cast<HMENU>(app::kWidthLabelId),
                                          reinterpret_cast<HINSTANCE>(GetWindowLongPtrW(hwnd, GWLP_HINSTANCE)), nullptr);
    }
    y += 18;
    if (!app::g_widthSlider) {
        app::g_widthSlider = CreateWindowExW(0, TRACKBAR_CLASSW, L"", WS_VISIBLE | WS_CHILD | TBS_AUTOTICKS, x, y, sliderW,
                                             app::kSliderHeight, hwnd, reinterpret_cast<HMENU>(app::kWidthSliderId),
                                             reinterpret_cast<HINSTANCE>(GetWindowLongPtrW(hwnd, GWLP_HINSTANCE)), nullptr);
        SendMessageW(app::g_widthSlider, TBM_SETRANGE, TRUE, MAKELPARAM(app::kSliderMin, app::kSliderMax));
        SendMessageW(app::g_widthSlider, TBM_SETPOS, TRUE, app::g_outputWidth);
    }
    if (!app::g_widthEdit) {
        app::g_widthEdit = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_VISIBLE | WS_CHILD | ES_AUTOHSCROLL, x + sliderW + 8, y + 2,
                                           editW, app::kSliderHeight - 4, hwnd, reinterpret_cast<HMENU>(app::kWidthEditId),
                                           reinterpret_cast<HINSTANCE>(GetWindowLongPtrW(hwnd, GWLP_HINSTANCE)), nullptr);
        SetWindowSubclass(app::g_widthEdit, EditEnterSubclassProc, 1, static_cast<DWORD_PTR>(app::kWidthEditId));
    }
    y += app::kSliderHeight + app::kSliderGap;

    if (!app::g_heightLabel) {
        app::g_heightLabel = CreateWindowW(L"STATIC", L"Height:", WS_VISIBLE | WS_CHILD, x, y, w, 18, hwnd,
                                           reinterpret_cast<HMENU>(app::kHeightLabelId),
                                           reinterpret_cast<HINSTANCE>(GetWindowLongPtrW(hwnd, GWLP_HINSTANCE)), nullptr);
    }
    y += 18;
    if (!app::g_heightSlider) {
        app::g_heightSlider = CreateWindowExW(0, TRACKBAR_CLASSW, L"", WS_VISIBLE | WS_CHILD | TBS_AUTOTICKS, x, y, sliderW,
                                              app::kSliderHeight, hwnd, reinterpret_cast<HMENU>(app::kHeightSliderId),
                                              reinterpret_cast<HINSTANCE>(GetWindowLongPtrW(hwnd, GWLP_HINSTANCE)), nullptr);
        SendMessageW(app::g_heightSlider, TBM_SETRANGE, TRUE, MAKELPARAM(app::kSliderMin, app::kSliderMax));
        SendMessageW(app::g_heightSlider, TBM_SETPOS, TRUE, app::g_outputHeight);
    }
    if (!app::g_heightEdit) {
        app::g_heightEdit =
            CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_VISIBLE | WS_CHILD | ES_AUTOHSCROLL, x + sliderW + 8, y + 2, editW,
                            app::kSliderHeight - 4, hwnd, reinterpret_cast<HMENU>(app::kHeightEditId),
                            reinterpret_cast<HINSTANCE>(GetWindowLongPtrW(hwnd, GWLP_HINSTANCE)), nullptr);
        SetWindowSubclass(app::g_heightEdit, EditEnterSubclassProc, 1, static_cast<DWORD_PTR>(app::kHeightEditId));
    }
    y += app::kSliderHeight + app::kSliderGap;

    if (!app::g_rotateLabel) {
        app::g_rotateLabel = CreateWindowW(L"STATIC", L"Rotate:", WS_VISIBLE | WS_CHILD, x, y, w, 18, hwnd,
                                           reinterpret_cast<HMENU>(app::kRotateLabelId),
                                           reinterpret_cast<HINSTANCE>(GetWindowLongPtrW(hwnd, GWLP_HINSTANCE)), nullptr);
    }
    y += 18;
    if (!app::g_rotateSlider) {
        app::g_rotateSlider = CreateWindowExW(0, TRACKBAR_CLASSW, L"", WS_VISIBLE | WS_CHILD | TBS_AUTOTICKS, x, y, sliderW,
                                              app::kSliderHeight, hwnd, reinterpret_cast<HMENU>(app::kRotateSliderId),
                                              reinterpret_cast<HINSTANCE>(GetWindowLongPtrW(hwnd, GWLP_HINSTANCE)), nullptr);
        SendMessageW(app::g_rotateSlider, TBM_SETRANGE, TRUE, MAKELPARAM(-180, 180));
        SendMessageW(app::g_rotateSlider, TBM_SETPOS, TRUE, app::g_outputRotateDeg);
    }
    if (!app::g_rotateEdit) {
        app::g_rotateEdit =
            CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_VISIBLE | WS_CHILD | ES_AUTOHSCROLL, x + sliderW + 8, y + 2, editW,
                            app::kSliderHeight - 4, hwnd, reinterpret_cast<HMENU>(app::kRotateEditId),
                            reinterpret_cast<HINSTANCE>(GetWindowLongPtrW(hwnd, GWLP_HINSTANCE)), nullptr);
        SetWindowSubclass(app::g_rotateEdit, EditEnterSubclassProc, 1, static_cast<DWORD_PTR>(app::kRotateEditId));
    }

    // Position
    if (app::g_widthLabel) SetWindowPos(app::g_widthLabel, nullptr, x, paneY + app::kSliderMargin, w, 18, SWP_NOZORDER);
    if (app::g_widthSlider) SetWindowPos(app::g_widthSlider, nullptr, x, paneY + app::kSliderMargin + 18, sliderW, app::kSliderHeight, SWP_NOZORDER);
    if (app::g_widthEdit) SetWindowPos(app::g_widthEdit, nullptr, x + sliderW + 8, paneY + app::kSliderMargin + 20, editW, app::kSliderHeight - 4, SWP_NOZORDER);

    if (app::g_heightLabel)
        SetWindowPos(app::g_heightLabel, nullptr, x, paneY + app::kSliderMargin + 18 + app::kSliderHeight + app::kSliderGap, w, 18, SWP_NOZORDER);
    if (app::g_heightSlider)
        SetWindowPos(app::g_heightSlider, nullptr, x, paneY + app::kSliderMargin + 18 + app::kSliderHeight + app::kSliderGap + 18, sliderW,
                     app::kSliderHeight, SWP_NOZORDER);
    if (app::g_heightEdit)
        SetWindowPos(app::g_heightEdit, nullptr, x + sliderW + 8, paneY + app::kSliderMargin + 18 + app::kSliderHeight + app::kSliderGap + 20,
                     editW, app::kSliderHeight - 4, SWP_NOZORDER);

    if (app::g_rotateLabel)
        SetWindowPos(app::g_rotateLabel, nullptr, x, paneY + app::kSliderMargin + (18 + app::kSliderHeight + app::kSliderGap) * 2, w, 18, SWP_NOZORDER);
    if (app::g_rotateSlider)
        SetWindowPos(app::g_rotateSlider, nullptr, x, paneY + app::kSliderMargin + (18 + app::kSliderHeight + app::kSliderGap) * 2 + 18, sliderW,
                     app::kSliderHeight, SWP_NOZORDER);
    if (app::g_rotateEdit)
        SetWindowPos(app::g_rotateEdit, nullptr, x + sliderW + 8, paneY + app::kSliderMargin + (18 + app::kSliderHeight + app::kSliderGap) * 2 + 20,
                     editW, app::kSliderHeight - 4, SWP_NOZORDER);

    UpdateSliderLabels();
}

void CreateOrUpdateBrightnessControls(HWND hwnd) {
    const auto layout = GetViewLayout(hwnd);
    const int paneX = layout.leftControlsPane.X;
    const int paneY = layout.leftControlsPane.Y;
    const int paneW = layout.leftControlsPane.Width;
    const int paneH = layout.leftControlsPane.Height;
    if (paneW <= 0 || paneH <= 0) return;

    const int x = paneX + app::kSliderMargin;
    const int editW = 72;
    const int w = (std::max)(0, paneW - app::kSliderMargin * 2);
    const int sliderW = (std::max)(0, w - editW - 8);
    int y = paneY + app::kSliderMargin;

    if (!app::g_brightnessLabel) {
        app::g_brightnessLabel = CreateWindowW(L"STATIC", L"Brightness:", WS_VISIBLE | WS_CHILD, x, y, w, 18, hwnd,
                                               reinterpret_cast<HMENU>(app::kBrightnessLabelId),
                                               reinterpret_cast<HINSTANCE>(GetWindowLongPtrW(hwnd, GWLP_HINSTANCE)), nullptr);
    }
    y += 18;
    if (!app::g_brightnessSlider) {
        app::g_brightnessSlider = CreateWindowExW(0, TRACKBAR_CLASSW, L"", WS_VISIBLE | WS_CHILD | TBS_AUTOTICKS, x, y, sliderW, app::kSliderHeight, hwnd,
                                                  reinterpret_cast<HMENU>(app::kBrightnessSliderId),
                                                  reinterpret_cast<HINSTANCE>(GetWindowLongPtrW(hwnd, GWLP_HINSTANCE)), nullptr);
        SendMessageW(app::g_brightnessSlider, TBM_SETRANGE, TRUE, MAKELPARAM(app::kColorSliderMin, app::kColorSliderMax));
        SendMessageW(app::g_brightnessSlider, TBM_SETPOS, TRUE, app::g_brightness);
    }
    if (!app::g_brightnessEdit) {
        app::g_brightnessEdit =
            CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_VISIBLE | WS_CHILD | ES_AUTOHSCROLL, x + sliderW + 8, y + 2, editW,
                            app::kSliderHeight - 4, hwnd, reinterpret_cast<HMENU>(app::kBrightnessEditId),
                            reinterpret_cast<HINSTANCE>(GetWindowLongPtrW(hwnd, GWLP_HINSTANCE)), nullptr);
        SetWindowSubclass(app::g_brightnessEdit, EditEnterSubclassProc, 1, static_cast<DWORD_PTR>(app::kBrightnessEditId));
    }

    if (app::g_brightnessLabel) SetWindowPos(app::g_brightnessLabel, nullptr, x, paneY + app::kSliderMargin, w, 18, SWP_NOZORDER);
    if (app::g_brightnessSlider)
        SetWindowPos(app::g_brightnessSlider, nullptr, x, paneY + app::kSliderMargin + 18, sliderW, app::kSliderHeight, SWP_NOZORDER);
    if (app::g_brightnessEdit)
        SetWindowPos(app::g_brightnessEdit, nullptr, x + sliderW + 8, paneY + app::kSliderMargin + 20, editW, app::kSliderHeight - 4, SWP_NOZORDER);

    UpdateBrightnessLabels();
}

}  // namespace app::ui

