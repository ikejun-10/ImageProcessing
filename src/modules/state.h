#pragma once

#include <windows.h>
#include <commctrl.h>
#include <gdiplus.h>

#include <array>
#include <memory>
#include <string>
#include <vector>

namespace app {

// Window / control IDs
inline constexpr wchar_t kWindowClassName[] = L"ImageViewerWindowClass";
inline constexpr wchar_t kWindowTitle[] = L"Image Viewer";
inline constexpr UINT kMenuImportPictureId = 1001;
inline constexpr UINT kMenuExportCorrectedId = 1002;
inline constexpr UINT kResetButtonId = 2001;
inline constexpr UINT kModeToggleButtonId = 2002;

inline constexpr UINT kWidthSliderId = 3001;
inline constexpr UINT kHeightSliderId = 3002;
inline constexpr UINT kRotateSliderId = 3005;
inline constexpr UINT kWidthLabelId = 3003;
inline constexpr UINT kHeightLabelId = 3004;
inline constexpr UINT kRotateLabelId = 3006;
inline constexpr UINT kWidthEditId = 3011;
inline constexpr UINT kHeightEditId = 3012;
inline constexpr UINT kRotateEditId = 3015;

inline constexpr UINT kBrightnessSliderId = 4002;
inline constexpr UINT kBrightnessLabelId = 4102;
inline constexpr UINT kBrightnessEditId = 4112;

inline constexpr UINT kMsgApplyEditValue = WM_APP + 100;

// UI constants
inline constexpr float kPointHitRadiusPixels = 10.0f;
inline constexpr int kSliderMin = 64;
inline constexpr int kSliderMax = 2048;
inline constexpr int kSliderHeight = 28;
inline constexpr int kSliderMargin = 10;
inline constexpr int kSliderGap = 10;
inline constexpr int kColorSliderMin = 0;
inline constexpr int kColorSliderMax = 200;
inline constexpr int kColorSliderDefault = 100;

enum class SelectionMode { Quad = 0, Ellipse = 1 };

struct ImageDisplayInfo {
    bool valid = false;
    Gdiplus::Rect rect = Gdiplus::Rect(0, 0, 0, 0);
    double scale = 1.0;
    UINT imageWidth = 0;
    UINT imageHeight = 0;
};

struct ViewLayout {
    Gdiplus::Rect leftPane = Gdiplus::Rect(0, 0, 0, 0);
    Gdiplus::Rect rightPane = Gdiplus::Rect(0, 0, 0, 0);
    Gdiplus::Rect topBar = Gdiplus::Rect(0, 0, 0, 0);
    Gdiplus::Rect leftImagePane = Gdiplus::Rect(0, 0, 0, 0);
    Gdiplus::Rect leftControlsPane = Gdiplus::Rect(0, 0, 0, 0);
    Gdiplus::Rect rightImagePane = Gdiplus::Rect(0, 0, 0, 0);
    Gdiplus::Rect rightControlsPane = Gdiplus::Rect(0, 0, 0, 0);
};

struct EllipseParams {
    bool valid = false;
    double cx = 0.0;
    double cy = 0.0;
    double a = 0.0;
    double b = 0.0;
    double theta = 0.0;
};

enum class EllipseDragMode { None = 0, Draw = 1, Move = 2, ResizeLeft = 3, ResizeRight = 4, ResizeTop = 5, ResizeBottom = 6, Rotate = 7 };

// Global state (kept for minimal refactor risk)
extern ULONG_PTR g_gdiplusToken;
extern std::unique_ptr<Gdiplus::Image> g_loadedImage;
extern std::vector<Gdiplus::PointF> g_selectedPointsImageSpace;
extern HWND g_resetButton;
extern HWND g_modeButton;
extern int g_draggingPointIndex;
extern std::unique_ptr<Gdiplus::Bitmap> g_correctedImage;

extern HWND g_widthSlider;
extern HWND g_heightSlider;
extern HWND g_rotateSlider;
extern HWND g_widthLabel;
extern HWND g_heightLabel;
extern HWND g_rotateLabel;
extern HWND g_widthEdit;
extern HWND g_heightEdit;
extern HWND g_rotateEdit;
extern int g_outputWidth;
extern int g_outputHeight;
extern int g_outputRotateDeg;

extern HWND g_brightnessSlider;
extern HWND g_brightnessLabel;
extern HWND g_brightnessEdit;
extern int g_brightness;

extern double g_leftZoom;
extern int g_leftPanX;
extern int g_leftPanY;
extern bool g_isPanning;
extern POINT g_lastPanPoint;

extern SelectionMode g_selectionMode;
extern EllipseParams g_ellipse;
extern EllipseDragMode g_ellipseDragMode;
extern Gdiplus::PointF g_ellipseDrawStart;
extern Gdiplus::PointF g_ellipseMoveOffset;
extern double g_ellipseRotateOffset;

}  // namespace app

