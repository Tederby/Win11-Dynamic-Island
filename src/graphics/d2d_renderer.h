#pragma once

#include <windows.h>
#include <d2d1.h>
#include <dwrite.h>
#include <wrl/client.h>
#include <string>
#include "../common/defs.h"

#include "icons.h"

namespace DynamicIsland {
namespace Graphics {

using Microsoft::WRL::ComPtr;

class D2DRenderer {
public:
    D2DRenderer();
    ~D2DRenderer();

    bool Initialize(HWND hwnd);
    void Cleanup();
    void Resize(UINT width, UINT height);

    void BeginDraw();
    HRESULT EndDraw();
    bool PresentLayeredWindow(HWND hwnd, int winX, int winY);

    // Drawing primitives
    void Clear(D2D1_COLOR_F color);
    void DrawRoundedPill(const D2D1_ROUNDED_RECT& pill, D2D1_COLOR_F fillColor, D2D1_COLOR_F borderColor, float strokeWidth = 1.0f);
    void DrawTextString(const std::wstring& text, const D2D1_RECT_F& rect, D2D1_COLOR_F color, float fontSize = 12.0f, bool bold = false);
    void DrawMarqueeText(const std::wstring& text, const D2D1_RECT_F& rect, D2D1_COLOR_F color, float offset, bool bold = false);
    void DrawBigText(const std::wstring& text, const D2D1_RECT_F& rect, D2D1_COLOR_F color);
    void DrawProgressBar(const D2D1_RECT_F& barRect, float progressFraction, D2D1_COLOR_F bgColor, D2D1_COLOR_F fgColor);
    void DrawProgressRing(D2D1_POINT_2F center, float radius, float progressFraction, D2D1_COLOR_F ringColor, float strokeWidth = 2.0f);
    void DrawStatusDot(D2D1_POINT_2F center, float radius, D2D1_COLOR_F color);
    void DrawEqualizerWaves(D2D1_POINT_2F origin, float height, float progress);
    void DrawAlbumArt(const D2D1_RECT_F& bounds, float cornerRadius);
    void DrawIconImage(IconType type, const D2D1_RECT_F& bounds, D2D1_COLOR_F color);
    void DrawButtonPill(const D2D1_RECT_F& bounds, const std::wstring& text, bool isHovered);
    void DrawCircularButton(const D2D1_RECT_F& bounds, IconType icon, bool isHovered);

    ID2D1RenderTarget* GetRenderTarget() const { return m_dcRenderTarget.Get(); }
    ID2D1Factory* GetFactory() const { return m_d2dFactory.Get(); }
    IDWriteFactory* GetDWriteFactory() const { return m_dwriteFactory.Get(); }

private:
    HWND m_hwnd = nullptr;
    ComPtr<ID2D1Factory> m_d2dFactory;
    ComPtr<IDWriteFactory> m_dwriteFactory;
    ComPtr<ID2D1DCRenderTarget> m_dcRenderTarget;
    ComPtr<ID2D1SolidColorBrush> m_solidBrush;
    ComPtr<IDWriteTextFormat> m_textFormatRegular;
    ComPtr<IDWriteTextFormat> m_textFormatBold;
    ComPtr<IDWriteTextFormat> m_textFormatBig;
    ComPtr<IDWriteTextFormat> m_textFormatSmall;

    HDC m_memDC = nullptr;
    HBITMAP m_hBitmap = nullptr;
    HBITMAP m_hOldBitmap = nullptr;
    void* m_pixelBits = nullptr;
    UINT m_width = 0;
    UINT m_height = 0;

    bool CreateDeviceResources();
    void DiscardDeviceResources();
};

} // namespace Graphics
} // namespace DynamicIsland
