#include "state.h"

namespace app {

ULONG_PTR g_gdiplusToken = 0;
std::unique_ptr<Gdiplus::Image> g_loadedImage;
std::vector<Gdiplus::PointF> g_selectedPointsImageSpace;
HWND g_resetButton = nullptr;
HWND g_modeButton = nullptr;
int g_draggingPointIndex = -1;
std::unique_ptr<Gdiplus::Bitmap> g_correctedImage;

HWND g_widthSlider = nullptr;
HWND g_heightSlider = nullptr;
HWND g_rotateSlider = nullptr;
HWND g_widthLabel = nullptr;
HWND g_heightLabel = nullptr;
HWND g_rotateLabel = nullptr;
HWND g_widthEdit = nullptr;
HWND g_heightEdit = nullptr;
HWND g_rotateEdit = nullptr;
int g_outputWidth = 512;
int g_outputHeight = 512;
int g_outputRotateDeg = 0;

HWND g_brightnessSlider = nullptr;
HWND g_brightnessLabel = nullptr;
HWND g_brightnessEdit = nullptr;
int g_brightness = kColorSliderDefault;

double g_leftZoom = 1.0;
int g_leftPanX = 0;
int g_leftPanY = 0;
bool g_isPanning = false;
POINT g_lastPanPoint = {0, 0};

SelectionMode g_selectionMode = SelectionMode::Quad;
EllipseParams g_ellipse = {};
EllipseDragMode g_ellipseDragMode = EllipseDragMode::None;
Gdiplus::PointF g_ellipseDrawStart = Gdiplus::PointF(0, 0);
Gdiplus::PointF g_ellipseMoveOffset = Gdiplus::PointF(0, 0);
double g_ellipseRotateOffset = 0.0;

}  // namespace app

