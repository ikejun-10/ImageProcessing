#include <windows.h>
#include <commctrl.h>
#include <gdiplus.h>

#include "state.h"
#include "ui.h"
#include "window_proc.h"

#pragma comment(lib, "gdiplus.lib")
#pragma comment(lib, "comctl32.lib")

using Gdiplus::GdiplusShutdown;
using Gdiplus::GdiplusStartup;
using Gdiplus::GdiplusStartupInput;
using Gdiplus::Status;

namespace {
using app::g_gdiplusToken;
using app::g_loadedImage;
}  // namespace

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, PWSTR, int nCmdShow) {
    INITCOMMONCONTROLSEX icc = {};
    icc.dwSize = sizeof(icc);
    icc.dwICC = ICC_WIN95_CLASSES;
    InitCommonControlsEx(&icc);

    GdiplusStartupInput gdiplusStartupInput;
    if (GdiplusStartup(&g_gdiplusToken, &gdiplusStartupInput, nullptr) != Status::Ok) {
        MessageBoxW(nullptr, L"Failed to initialize GDI+.", L"Startup Error", MB_OK | MB_ICONERROR);
        return -1;
    }

    WNDCLASSW wc = {};
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = app::kWindowClassName;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);

    if (!RegisterClassW(&wc)) {
        GdiplusShutdown(g_gdiplusToken);
        MessageBoxW(nullptr, L"Failed to register window class.", L"Startup Error", MB_OK | MB_ICONERROR);
        return -1;
    }

    HWND hwnd = CreateWindowExW(
        0,
        app::kWindowClassName,
        app::kWindowTitle,
        WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        1024,
        768,
        nullptr,
        app::ui::CreateMainMenu(),
        hInstance,
        nullptr);

    if (!hwnd) {
        GdiplusShutdown(g_gdiplusToken);
        MessageBoxW(nullptr, L"Failed to create window.", L"Startup Error", MB_OK | MB_ICONERROR);
        return -1;
    }

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    MSG msg = {};
    while (GetMessageW(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    g_loadedImage.reset();
    GdiplusShutdown(g_gdiplusToken);
    return static_cast<int>(msg.wParam);
}
