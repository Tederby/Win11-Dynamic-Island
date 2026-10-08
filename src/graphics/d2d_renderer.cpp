#include "d2d_renderer.h"
#include "../common/log.h"
#include <cmath>

namespace DynamicIsland {
namespace Graphics {

D2DRenderer::D2DRenderer() = default;

D2DRenderer::~D2DRenderer() {
    Cleanup();
}

bool D2DRenderer::Initialize(HWND hwnd) {
    m_hwnd = hwnd;

    HRESULT hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, m_d2dFactory.GetAddressOf());
    if (FAILED(hr)) {
        LogError(L"Failed to create Direct2D Factory, hr=0x%08X", hr);
        return false;
    }

    hr = DWriteCreateFactory(
        DWRITE_FACTORY_TYPE_SHARED,
        __uuidof(IDWriteFactory),
        reinterpret_cast<IUnknown**>(m_dwriteFactory.GetAddressOf())
    );
    if (FAILED(hr)) {
        LogError(L"Failed to create DirectWrite Factory, hr=0x%08X", hr);
        return false;
    }

    hr = m_dwriteFactory->CreateTextFormat(
        L"Segoe UI Variable",
        nullptr,
        DWRITE_FONT_WEIGHT_NORMAL,
        DWRITE_FONT_STYLE_NORMAL,
        DWRITE_FONT_STRETCH_NORMAL,
        12.0f,
        L"en-US",
        m_textFormatRegular.GetAddressOf()
    );

    if (SUCCEEDED(hr)) {
        m_dwriteFactory->CreateTextFormat(
            L"Segoe UI Variable",
            nullptr,
            DWRITE_FONT_WEIGHT_SEMI_BOLD,
            DWRITE_FONT_STYLE_NORMAL,
            DWRITE_FONT_STRETCH_NORMAL,
            12.0f,
            L"en-US",
            m_textFormatBold.GetAddressOf()
        );
    }

    return CreateDeviceResources();
}

bool D2DRenderer::CreateDeviceResources() {
    if (!m_hwnd || m_renderTarget) return true;

    RECT rc;
    GetClientRect(m_hwnd, &rc);
    D2D1_SIZE_U size = D2D1::SizeU(rc.right - rc.left, rc.bottom - rc.top);

    D2D1_RENDER_TARGET_PROPERTIES rtProps = D2D1::RenderTargetProperties(
        D2D1_RENDER_TARGET_TYPE_DEFAULT,
        D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED)
    );

    D2D1_HWND_RENDER_TARGET_PROPERTIES hwndProps = D2D1::HwndRenderTargetProperties(
        m_hwnd,
        size,
        D2D1_PRESENT_OPTIONS_IMMEDIATELY
    );

    HRESULT hr = m_d2dFactory->CreateHwndRenderTarget(
        rtProps,
        hwndProps,
        m_renderTarget.GetAddressOf()
    );

    if (FAILED(hr)) {
        LogError(L"Failed to create HwndRenderTarget, hr=0x%08X", hr);
        return false;
    }

    m_renderTarget->CreateSolidColorBrush(
        D2D1::ColorF(D2D1::ColorF::White),
        m_solidBrush.GetAddressOf()
    );

    return true;
}

void D2DRenderer::DiscardDeviceResources() {
    m_solidBrush.Reset();
    m_renderTarget.Reset();
}

void D2DRenderer::Cleanup() {
    DiscardDeviceResources();
    m_textFormatBold.Reset();
    m_textFormatRegular.Reset();
    m_dwriteFactory.Reset();
    m_d2dFactory.Reset();
}

void D2DRenderer::Resize(UINT width, UINT height) {
    if (m_renderTarget) {
        m_renderTarget->Resize(D2D1::SizeU(width, height));
    }
}

void D2DRenderer::BeginDraw() {
    CreateDeviceResources();
    if (m_renderTarget) {
        m_renderTarget->BeginDraw();
    }
}

HRESULT D2DRenderer::EndDraw() {
    if (!m_renderTarget) return E_FAIL;
    HRESULT hr = m_renderTarget->EndDraw();
    if (hr == D2DERR_RECREATE_TARGET) {
        DiscardDeviceResources();
    }
    return hr;
}

void D2DRenderer::Clear(D2D1_COLOR_F color) {
    if (m_renderTarget) {
        m_renderTarget->Clear(color);
    }
}

void D2DRenderer::DrawRoundedPill(const D2D1_ROUNDED_RECT& pill, D2D1_COLOR_F fillColor, D2D1_COLOR_F borderColor, float strokeWidth) {
    if (!m_renderTarget || !m_solidBrush) return;

    m_solidBrush->SetColor(fillColor);
    m_renderTarget->FillRoundedRectangle(pill, m_solidBrush.Get());

    if (strokeWidth > 0.0f && borderColor.a > 0.0f) {
        m_solidBrush->SetColor(borderColor);
        m_renderTarget->DrawRoundedRectangle(pill, m_solidBrush.Get(), strokeWidth);
    }
}

void D2DRenderer::DrawTextString(const std::wstring& text, const D2D1_RECT_F& rect, D2D1_COLOR_F color, float fontSize, bool bold) {
    if (!m_renderTarget || !m_solidBrush) return;

    IDWriteTextFormat* format = bold ? m_textFormatBold.Get() : m_textFormatRegular.Get();
    if (!format) return;

    m_solidBrush->SetColor(color);
    m_renderTarget->DrawText(
        text.c_str(),
        static_cast<UINT32>(text.length()),
        format,
        rect,
        m_solidBrush.Get()
    );
}

void D2DRenderer::DrawProgressBar(const D2D1_RECT_F& barRect, float progressFraction, D2D1_COLOR_F bgColor, D2D1_COLOR_F fgColor) {
    if (!m_renderTarget || !m_solidBrush) return;

    float radius = (barRect.bottom - barRect.top) / 2.0f;
    D2D1_ROUNDED_RECT bgRounded = D2D1::RoundedRect(barRect, radius, radius);

    m_solidBrush->SetColor(bgColor);
    m_renderTarget->FillRoundedRectangle(bgRounded, m_solidBrush.Get());

    if (progressFraction > 0.0f) {
        float fillWidth = (barRect.right - barRect.left) * (progressFraction > 1.0f ? 1.0f : progressFraction);
        D2D1_RECT_F fillRect = D2D1::RectF(barRect.left, barRect.top, barRect.left + fillWidth, barRect.bottom);
        D2D1_ROUNDED_RECT fillRounded = D2D1::RoundedRect(fillRect, radius, radius);

        m_solidBrush->SetColor(fgColor);
        m_renderTarget->FillRoundedRectangle(fillRounded, m_solidBrush.Get());
    }
}

void D2DRenderer::DrawProgressRing(D2D1_POINT_2F center, float radius, float progressFraction, D2D1_COLOR_F ringColor, float strokeWidth) {
    if (!m_renderTarget || !m_solidBrush) return;

    // Draw background track
    m_solidBrush->SetColor(D2D1::ColorF(1.0f, 1.0f, 1.0f, 0.2f));
    m_renderTarget->DrawEllipse(D2D1::Ellipse(center, radius, radius), m_solidBrush.Get(), strokeWidth);

    // Draw foreground ring arc
    if (progressFraction > 0.0f) {
        m_solidBrush->SetColor(ringColor);
        m_renderTarget->DrawEllipse(D2D1::Ellipse(center, radius, radius), m_solidBrush.Get(), strokeWidth);
    }
}

void D2DRenderer::DrawStatusDot(D2D1_POINT_2F center, float radius, D2D1_COLOR_F color) {
    if (!m_renderTarget || !m_solidBrush) return;

    m_solidBrush->SetColor(color);
    m_renderTarget->FillEllipse(D2D1::Ellipse(center, radius, radius), m_solidBrush.Get());
}

void D2DRenderer::DrawEqualizerWaves(D2D1_POINT_2F origin, float height, float progress) {
    if (!m_renderTarget || !m_solidBrush) return;

    // 3 animated equalizer vertical lines
    m_solidBrush->SetColor(D2D1::ColorF(0.49f, 0.88f, 0.76f, 1.0f)); // Mint green #7ee0c3

    float offsets[3] = {
        static_cast<float>(std::sin(progress * 6.28f) * 0.5f + 0.5f),
        static_cast<float>(std::sin(progress * 6.28f + 1.25f) * 0.5f + 0.5f),
        static_cast<float>(std::sin(progress * 6.28f + 2.5f) * 0.5f + 0.5f)
    };

    for (int i = 0; i < 3; ++i) {
        float barH = height * (0.3f + 0.7f * offsets[i]);
        D2D1_RECT_F r = D2D1::RectF(
            origin.x + i * 4.0f,
            origin.y + (height - barH) / 2.0f,
            origin.x + i * 4.0f + 2.0f,
            origin.y + (height + barH) / 2.0f
        );
        m_renderTarget->FillRectangle(r, m_solidBrush.Get());
    }
}

} // namespace Graphics
} // namespace DynamicIsland
