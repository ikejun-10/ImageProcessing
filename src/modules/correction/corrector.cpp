#include "corrector.h"

#include "geometry.h"
#include "state.h"

#include <gdiplus.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <memory>

namespace app::corrector {

using Gdiplus::Bitmap;
using Gdiplus::BitmapData;
using Gdiplus::Graphics;
using Gdiplus::ImageLockModeRead;
using Gdiplus::ImageLockModeWrite;
using Gdiplus::PointF;
using Gdiplus::Rect;

void UpdateCorrectedPreview() {
    const bool hasEnoughPoints =
        app::g_selectionMode == app::SelectionMode::Ellipse ? app::g_ellipse.valid : (app::g_selectedPointsImageSpace.size() == 4);
    if (!app::g_loadedImage || !hasEnoughPoints) {
        app::g_correctedImage.reset();
        return;
    }

    const int srcW = static_cast<int>(app::g_loadedImage->GetWidth());
    const int srcH = static_cast<int>(app::g_loadedImage->GetHeight());
    if (srcW <= 1 || srcH <= 1) {
        return;
    }

    const int outW = std::clamp(app::g_outputWidth, app::kSliderMin, app::kSliderMax);
    const int outH = std::clamp(app::g_outputHeight, app::kSliderMin, app::kSliderMax);

    // Build source bitmap in a predictable pixel format.
    Bitmap srcBmp(srcW, srcH, PixelFormat32bppPARGB);
    {
        Graphics g(&srcBmp);
        g.SetPageUnit(Gdiplus::UnitPixel);
        g.DrawImage(
            app::g_loadedImage.get(),
            Rect(0, 0, srcW, srcH),
            0,
            0,
            srcW,
            srcH,
            Gdiplus::UnitPixel);
    }

    auto dstBmp = std::make_unique<Bitmap>(outW, outH, PixelFormat32bppPARGB);

    BitmapData srcData = {};
    BitmapData dstData = {};
    Rect srcRect(0, 0, srcW, srcH);
    Rect dstRect(0, 0, outW, outH);
    if (srcBmp.LockBits(&srcRect, ImageLockModeRead, PixelFormat32bppPARGB, &srcData) != Gdiplus::Ok) {
        return;
    }
    if (dstBmp->LockBits(&dstRect, ImageLockModeWrite, PixelFormat32bppPARGB, &dstData) != Gdiplus::Ok) {
        srcBmp.UnlockBits(&srcData);
        return;
    }

    auto readPixel = [&](int x, int y, unsigned char* outBgra) {
        const unsigned char* row = static_cast<const unsigned char*>(srcData.Scan0) + y * srcData.Stride;
        const unsigned char* p = row + x * 4;
        outBgra[0] = p[0];
        outBgra[1] = p[1];
        outBgra[2] = p[2];
        outBgra[3] = p[3];
    };

    auto writePixel = [&](int x, int y, const unsigned char* bgra) {
        unsigned char* row = static_cast<unsigned char*>(dstData.Scan0) + y * dstData.Stride;
        unsigned char* p = row + x * 4;
        p[0] = bgra[0];
        p[1] = bgra[1];
        p[2] = bgra[2];
        p[3] = bgra[3];
    };

    auto clamp255 = [](double v) -> unsigned char {
        if (v < 0.0) return 0;
        if (v > 255.0) return 255;
        return static_cast<unsigned char>(v + 0.5);
    };

    const unsigned char blackOpaque[4] = {0, 0, 0, 255};
    const unsigned char transparent[4] = {0, 0, 0, 0};

    if (app::g_selectionMode == app::SelectionMode::Ellipse) {
        const app::EllipseParams& e = app::g_ellipse;
        if (!e.valid || e.a < 1.0 || e.b < 1.0) {
            dstBmp->UnlockBits(&dstData);
            srcBmp.UnlockBits(&srcData);
            return;
        }
        const double ct = std::cos(e.theta);
        const double st = std::sin(e.theta);

        for (int y = 0; y < outH; ++y) {
            for (int x = 0; x < outW; ++x) {
                const double u = (static_cast<double>(x) + 0.5) / static_cast<double>(outW);
                const double v = (static_cast<double>(y) + 0.5) / static_cast<double>(outH);
                // Map output rectangle -> ellipse local coords (axis-aligned in local frame).
                const double lx = (u - 0.5) * 2.0 * e.a;
                const double ly = (v - 0.5) * 2.0 * e.b;
                // Rotate local -> image
                const double srcX = e.cx + lx * ct - ly * st;
                const double srcY = e.cy + lx * st + ly * ct;

                const double nx = lx / e.a;
                const double ny = ly / e.b;
                if ((nx * nx + ny * ny) > 1.0) {
                    // Outside ellipse should be transparent (for PNG export).
                    writePixel(x, y, transparent);
                    continue;
                }

                const int x0 = static_cast<int>(std::floor(srcX));
                const int y0 = static_cast<int>(std::floor(srcY));
                const int x1 = x0 + 1;
                const int y1 = y0 + 1;
                const double fx = srcX - static_cast<double>(x0);
                const double fy = srcY - static_cast<double>(y0);
                if (x0 < 0 || y0 < 0 || x1 >= srcW || y1 >= srcH) {
                    writePixel(x, y, transparent);
                    continue;
                }

                unsigned char c00[4], c10[4], c01[4], c11[4];
                readPixel(x0, y0, c00);
                readPixel(x1, y0, c10);
                readPixel(x0, y1, c01);
                readPixel(x1, y1, c11);

                const double w00 = (1.0 - fx) * (1.0 - fy);
                const double w10 = fx * (1.0 - fy);
                const double w01 = (1.0 - fx) * fy;
                const double w11 = fx * fy;

                unsigned char out[4];
                for (int k = 0; k < 4; ++k) {
                    const double val = c00[k] * w00 + c10[k] * w10 + c01[k] * w01 + c11[k] * w11;
                    out[k] = clamp255(val);
                }
                writePixel(x, y, out);
            }
        }
    } else {
        std::array<PointF, 4> srcPts = {};
        if (!app::geom::BuildOrderedQuadrilateral(app::g_selectedPointsImageSpace, &srcPts)) {
            dstBmp->UnlockBits(&dstData);
            srcBmp.UnlockBits(&srcData);
            return;
        }
        std::array<double, 8> h = {};
        if (!app::geom::ComputeHomographyDestToSrcRect(srcPts, outW, outH, &h)) {
            dstBmp->UnlockBits(&dstData);
            srcBmp.UnlockBits(&srcData);
            return;
        }

        for (int y = 0; y < outH; ++y) {
            for (int x = 0; x < outW; ++x) {
                const double X = static_cast<double>(x) + 0.5;
                const double Y = static_cast<double>(y) + 0.5;
                const double denom = h[6] * X + h[7] * Y + 1.0;
                if (std::fabs(denom) < 1e-12) {
                    writePixel(x, y, blackOpaque);
                    continue;
                }
                const double srcX = (h[0] * X + h[1] * Y + h[2]) / denom;
                const double srcY = (h[3] * X + h[4] * Y + h[5]) / denom;

                const int x0 = static_cast<int>(std::floor(srcX));
                const int y0 = static_cast<int>(std::floor(srcY));
                const int x1 = x0 + 1;
                const int y1 = y0 + 1;
                const double fx = srcX - static_cast<double>(x0);
                const double fy = srcY - static_cast<double>(y0);

                if (x0 < 0 || y0 < 0 || x1 >= srcW || y1 >= srcH) {
                    writePixel(x, y, blackOpaque);
                    continue;
                }

                unsigned char c00[4], c10[4], c01[4], c11[4];
                readPixel(x0, y0, c00);
                readPixel(x1, y0, c10);
                readPixel(x0, y1, c01);
                readPixel(x1, y1, c11);

                const double w00 = (1.0 - fx) * (1.0 - fy);
                const double w10 = fx * (1.0 - fy);
                const double w01 = (1.0 - fx) * fy;
                const double w11 = fx * fy;

                unsigned char out[4];
                for (int k = 0; k < 4; ++k) {
                    const double val = c00[k] * w00 + c10[k] * w10 + c01[k] * w01 + c11[k] * w11;
                    out[k] = clamp255(val);
                }
                writePixel(x, y, out);
            }
        }
    }

    dstBmp->UnlockBits(&dstData);
    srcBmp.UnlockBits(&srcData);
    app::g_correctedImage = std::move(dstBmp);
}

}  // namespace app::corrector

