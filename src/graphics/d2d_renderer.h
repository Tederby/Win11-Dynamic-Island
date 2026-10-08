#pragma once

#include <windows.h>
#include <d2d1.h>
#include <dwrite.h>
#include <wrl/client.h>
#include <string>
#include "../common/defs.h"

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

    // Drawing primitives
    void Clear(D2D1_COLOR_F color);
    void DrawRoundedPill(const D2D1_ROUNDED_RECT& pill, D2D1_COLOR_F fillColor, D2D1_COLOR_F borderColor, float strokeWidth = 1.0f);
    void DrawTextString(const std::wstring& text, const D2D1_RECT_F& rect, D2D1_COLOR_F color, float fontSize = 12.0f, bool bold = false);
    void DrawProgressBar(const D2D1_RECT_F& barRect, float progressFraction, D2D1_COLOR_F bgColor, D2D1_COLOR_F fgColor);
    void DrawProgressRing(D2D1_POINT_2F center, float radius, float progressFraction, D2D1_COLOR_F ringColor, float strokeWidth = 2.0f);
    void DrawStatusDot(D2D1_POINT_2F center, float radius, D2D1_COLOR_F color);
    void DrawEqualizerWaves(D2D1_POINT_2F origin, float height, float progress);

    ID2D1RenderTarget* GetRenderTarget() const { return m_renderTarget.Get(); }
    ID2D1Factory* GetFactory() const { return m_d2dFactory.Get(); }
    IDWriteFactory* GetDWriteFactory() const { return m_dwriteFactory.Get(); }

private:
    HWND m_hwnd = nullptr;
    ComPtr<ID2D1Factory> m_d2dFactory;
    ComPtr<IDWriteFactory> m_dwriteFactory;
    ComPtr<ID2D1HwndRenderTarget> m_renderTarget;
    ComPtr<ID2D1SolidColorBrush> m_solidBrush;
    ComPtr<IDWriteTextFormat> m_textFormatRegular;
    ComPtr<IDWriteTextFormat> m_textFormatBold;

    bool CreateDeviceResources();
    void DiscardDeviceResources();
};

} // namespace Graphics
} // namespace DynamicIsland
