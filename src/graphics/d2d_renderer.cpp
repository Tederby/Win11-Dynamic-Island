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

    if (m_dwriteFactory) {
        m_dwriteFactory->CreateTextFormat(
            L"Segoe UI Variable",
            nullptr,
            DWRITE_FONT_WEIGHT_SEMI_BOLD,
            DWRITE_FONT_STYLE_NORMAL,
            DWRITE_FONT_STRETCH_NORMAL,
            32.0f,
            L"en-US",
            m_textFormatBig.GetAddressOf()
        );

        m_dwriteFactory->CreateTextFormat(
            L"Segoe UI Variable",
            nullptr,
            DWRITE_FONT_WEIGHT_NORMAL,
            DWRITE_FONT_STYLE_NORMAL,
            DWRITE_FONT_STRETCH_NORMAL,
            11.0f,
            L"en-US",
            m_textFormatSmall.GetAddressOf()
        );
    }

    auto configureNoWrapAndEllipsis = [this](IDWriteTextFormat* format) {
        if (!format || !m_dwriteFactory) return;
        format->SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP);
        DWRITE_TRIMMING trimming{};
        trimming.granularity = DWRITE_TRIMMING_GRANULARITY_CHARACTER;
        ComPtr<IDWriteInlineObject> ellipsisSign;
        if (SUCCEEDED(m_dwriteFactory->CreateEllipsisTrimmingSign(format, ellipsisSign.GetAddressOf()))) {
            format->SetTrimming(&trimming, ellipsisSign.Get());
        }
    };

    configureNoWrapAndEllipsis(m_textFormatRegular.Get());
    configureNoWrapAndEllipsis(m_textFormatBold.Get());
    configureNoWrapAndEllipsis(m_textFormatSmall.Get());
    configureNoWrapAndEllipsis(m_textFormatBig.Get());

    m_memDC = CreateCompatibleDC(nullptr);
    if (!m_memDC) {
        LogError(L"Failed to create compatible memory DC for layered window");
        return false;
    }

    return CreateDeviceResources();
}

bool D2DRenderer::CreateDeviceResources() {
    if (m_dcRenderTarget) return true;

    D2D1_RENDER_TARGET_PROPERTIES rtProps = D2D1::RenderTargetProperties(
        D2D1_RENDER_TARGET_TYPE_DEFAULT,
        D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED)
    );

    HRESULT hr = m_d2dFactory->CreateDCRenderTarget(
        &rtProps,
        m_dcRenderTarget.GetAddressOf()
    );

    if (FAILED(hr)) {
        LogError(L"Failed to create DCRenderTarget, hr=0x%08X", hr);
        return false;
    }

    m_dcRenderTarget->SetAntialiasMode(D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);
    m_dcRenderTarget->SetTextAntialiasMode(D2D1_TEXT_ANTIALIAS_MODE_CLEARTYPE);

    m_dcRenderTarget->CreateSolidColorBrush(
        D2D1::ColorF(D2D1::ColorF::White),
        m_solidBrush.GetAddressOf()
    );

    return true;
}

void D2DRenderer::DiscardDeviceResources() {
    m_albumArtBitmap.Reset();
    m_solidBrush.Reset();
    m_dcRenderTarget.Reset();
}

void D2DRenderer::Cleanup() {
    DiscardDeviceResources();
    m_marqueeCache.clear();
    if (m_hOldBitmap && m_memDC) {
        SelectObject(m_memDC, m_hOldBitmap);
        m_hOldBitmap = nullptr;
    }
    if (m_hBitmap) {
        DeleteObject(m_hBitmap);
        m_hBitmap = nullptr;
    }
    if (m_memDC) {
        DeleteDC(m_memDC);
        m_memDC = nullptr;
    }
    m_pixelBits = nullptr;
    m_width = 0;
    m_height = 0;

    m_textFormatSmall.Reset();
    m_textFormatBig.Reset();
    m_textFormatBold.Reset();
    m_textFormatRegular.Reset();
    m_dwriteFactory.Reset();
    m_d2dFactory.Reset();
}

void D2DRenderer::Resize(UINT width, UINT height) {
    if (width == 0) width = 1;
    if (height == 0) height = 1;

    if (m_width == width && m_height == height && m_hBitmap) {
        return;
    }

    m_width = width;
    m_height = height;

    if (m_hOldBitmap && m_memDC) {
        SelectObject(m_memDC, m_hOldBitmap);
        m_hOldBitmap = nullptr;
    }
    if (m_hBitmap) {
        DeleteObject(m_hBitmap);
        m_hBitmap = nullptr;
    }
    m_pixelBits = nullptr;

    if (!m_memDC) return;

    BITMAPINFO bmi{};
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = static_cast<LONG>(width);
    bmi.bmiHeader.biHeight = -static_cast<LONG>(height); // Top-down DIB
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    m_hBitmap = CreateDIBSection(m_memDC, &bmi, DIB_RGB_COLORS, &m_pixelBits, nullptr, 0);
    if (m_hBitmap) {
        m_hOldBitmap = static_cast<HBITMAP>(SelectObject(m_memDC, m_hBitmap));
    }
}

void D2DRenderer::BeginDraw() {
    CreateDeviceResources();
    if (!m_dcRenderTarget || !m_memDC || !m_hBitmap) return;

    RECT rc = { 0, 0, static_cast<LONG>(m_width), static_cast<LONG>(m_height) };
    HRESULT hr = m_dcRenderTarget->BindDC(m_memDC, &rc);
    if (SUCCEEDED(hr)) {
        m_dcRenderTarget->BeginDraw();
    }
}

HRESULT D2DRenderer::EndDraw() {
    if (!m_dcRenderTarget) return E_FAIL;
    HRESULT hr = m_dcRenderTarget->EndDraw();
    if (hr == D2DERR_RECREATE_TARGET) {
        DiscardDeviceResources();
    }
    return hr;
}

bool D2DRenderer::PresentLayeredWindow(HWND hwnd, int winX, int winY) {
    if (!hwnd || !m_memDC || !m_hBitmap || m_width == 0 || m_height == 0) {
        return false;
    }

    POINT ptSrc = { 0, 0 };
    SIZE sizeWnd = { static_cast<LONG>(m_width), static_cast<LONG>(m_height) };
    POINT ptDst = { winX, winY };
    BLENDFUNCTION blend{};
    blend.BlendOp = AC_SRC_OVER;
    blend.BlendFlags = 0;
    blend.SourceConstantAlpha = 255;
    blend.AlphaFormat = AC_SRC_ALPHA;

    HDC screenDC = GetDC(nullptr);
    BOOL ok = UpdateLayeredWindow(
        hwnd,
        screenDC,
        &ptDst,
        &sizeWnd,
        m_memDC,
        &ptSrc,
        0,
        &blend,
        ULW_ALPHA
    );
    ReleaseDC(nullptr, screenDC);
    return (ok != FALSE);
}

void D2DRenderer::Clear(D2D1_COLOR_F color) {
    if (m_dcRenderTarget) {
        m_dcRenderTarget->Clear(color);
    }
}

void D2DRenderer::DrawRoundedPill(const D2D1_ROUNDED_RECT& pill, D2D1_COLOR_F fillColor, D2D1_COLOR_F borderColor, float strokeWidth) {
    if (!m_dcRenderTarget || !m_solidBrush) return;

    m_solidBrush->SetColor(fillColor);
    m_dcRenderTarget->FillRoundedRectangle(pill, m_solidBrush.Get());

    if (strokeWidth > 0.0f && borderColor.a > 0.0f) {
        m_solidBrush->SetColor(borderColor);
        m_dcRenderTarget->DrawRoundedRectangle(pill, m_solidBrush.Get(), strokeWidth);
    }
}

void D2DRenderer::DrawTextString(const std::wstring& text, const D2D1_RECT_F& rect, D2D1_COLOR_F color, float fontSize, bool bold) {
    if (!m_dcRenderTarget || !m_solidBrush) return;

    IDWriteTextFormat* format = bold ? m_textFormatBold.Get() : m_textFormatRegular.Get();
    if (!format) return;

    m_solidBrush->SetColor(color);
    m_dcRenderTarget->DrawText(
        text.c_str(),
        static_cast<UINT32>(text.length()),
        format,
        rect,
        m_solidBrush.Get()
    );
}

void D2DRenderer::DrawMarqueeText(const std::wstring& text, const D2D1_RECT_F& rect, D2D1_COLOR_F color, float offset, bool bold) {
    if (!m_dcRenderTarget || !m_solidBrush || !m_dwriteFactory || text.empty()) return;

    IDWriteTextFormat* format = bold ? m_textFormatBold.Get() : m_textFormatRegular.Get();
    if (!format) return;

    float availW = rect.right - rect.left;
    float availH = rect.bottom - rect.top;
    if (availW <= 0.0f || availH <= 0.0f) return;

    // Look up in layout cache to avoid expensive CreateTextLayout calls on every frame
    CachedMarquee* found = nullptr;
    for (auto& item : m_marqueeCache) {
        if (item.text == text && item.bold == bold && std::abs(item.targetHeight - availH) < 0.5f) {
            found = &item;
            break;
        }
    }

    if (!found) {
        CachedMarquee newItem;
        newItem.text = text;
        newItem.bold = bold;
        newItem.targetHeight = availH;

        HRESULT hr = m_dwriteFactory->CreateTextLayout(
            text.c_str(),
            static_cast<UINT32>(text.length()),
            format,
            10000.0f,
            availH,
            newItem.layout.GetAddressOf()
        );
        if (SUCCEEDED(hr) && newItem.layout) {
            DWRITE_TEXT_METRICS tm;
            newItem.layout->GetMetrics(&tm);
            newItem.textWidth = tm.width;
        }

        std::wstring loopText = text + L"    \x2022    ";
        hr = m_dwriteFactory->CreateTextLayout(
            loopText.c_str(),
            static_cast<UINT32>(loopText.length()),
            format,
            10000.0f,
            availH,
            newItem.loopLayout.GetAddressOf()
        );
        if (SUCCEEDED(hr) && newItem.loopLayout) {
            DWRITE_TEXT_METRICS loopMetrics;
            newItem.loopLayout->GetMetrics(&loopMetrics);
            newItem.loopUnitWidth = loopMetrics.width;
        }

        if (m_marqueeCache.size() >= 8) {
            m_marqueeCache.erase(m_marqueeCache.begin());
        }
        m_marqueeCache.push_back(std::move(newItem));
        found = &m_marqueeCache.back();
    }

    if (!found || !found->layout) {
        DrawTextString(text, rect, color, 12.0f, bold);
        return;
    }

    m_dcRenderTarget->PushAxisAlignedClip(rect, D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);
    m_solidBrush->SetColor(color);

    if (found->textWidth <= availW) {
        m_dcRenderTarget->DrawTextLayout(
            D2D1::Point2F(rect.left, rect.top),
            found->layout.Get(),
            m_solidBrush.Get()
        );
    } else if (found->loopLayout && found->loopUnitWidth > 0.0f) {
        float unitWidth = found->loopUnitWidth;
        float shift = std::fmod(offset, unitWidth);
        if (shift < 0.0f) shift += unitWidth;

        m_dcRenderTarget->DrawTextLayout(
            D2D1::Point2F(rect.left - shift, rect.top),
            found->loopLayout.Get(),
            m_solidBrush.Get()
        );
        m_dcRenderTarget->DrawTextLayout(
            D2D1::Point2F(rect.left - shift + unitWidth, rect.top),
            found->loopLayout.Get(),
            m_solidBrush.Get()
        );
    } else {
        m_dcRenderTarget->DrawTextLayout(
            D2D1::Point2F(rect.left, rect.top),
            found->layout.Get(),
            m_solidBrush.Get()
        );
    }

    m_dcRenderTarget->PopAxisAlignedClip();
}

void D2DRenderer::DrawProgressBar(const D2D1_RECT_F& barRect, float progressFraction, D2D1_COLOR_F bgColor, D2D1_COLOR_F fgColor) {
    if (!m_dcRenderTarget || !m_solidBrush) return;

    float radius = (barRect.bottom - barRect.top) / 2.0f;
    D2D1_ROUNDED_RECT bgRounded = D2D1::RoundedRect(barRect, radius, radius);

    m_solidBrush->SetColor(bgColor);
    m_dcRenderTarget->FillRoundedRectangle(bgRounded, m_solidBrush.Get());

    if (progressFraction > 0.0f) {
        float fillWidth = (barRect.right - barRect.left) * (progressFraction > 1.0f ? 1.0f : progressFraction);
        D2D1_RECT_F fillRect = D2D1::RectF(barRect.left, barRect.top, barRect.left + fillWidth, barRect.bottom);
        D2D1_ROUNDED_RECT fillRounded = D2D1::RoundedRect(fillRect, radius, radius);

        m_solidBrush->SetColor(fgColor);
        m_dcRenderTarget->FillRoundedRectangle(fillRounded, m_solidBrush.Get());
    }
}

void D2DRenderer::DrawProgressRing(D2D1_POINT_2F center, float radius, float progressFraction, D2D1_COLOR_F ringColor, float strokeWidth) {
    if (!m_dcRenderTarget || !m_solidBrush) return;

    // Draw background track
    m_solidBrush->SetColor(D2D1::ColorF(1.0f, 1.0f, 1.0f, 0.2f));
    m_dcRenderTarget->DrawEllipse(D2D1::Ellipse(center, radius, radius), m_solidBrush.Get(), strokeWidth);

    // Draw foreground ring arc
    if (progressFraction > 0.0f) {
        m_solidBrush->SetColor(ringColor);
        m_dcRenderTarget->DrawEllipse(D2D1::Ellipse(center, radius, radius), m_solidBrush.Get(), strokeWidth);
    }
}

void D2DRenderer::DrawStatusDot(D2D1_POINT_2F center, float radius, D2D1_COLOR_F color) {
    if (!m_dcRenderTarget || !m_solidBrush) return;

    m_solidBrush->SetColor(color);
    m_dcRenderTarget->FillEllipse(D2D1::Ellipse(center, radius, radius), m_solidBrush.Get());
}

void D2DRenderer::DrawEqualizerWaves(D2D1_POINT_2F origin, float height, float progress) {
    if (!m_dcRenderTarget || !m_solidBrush) return;

    // Mint green #7ee0c3
    m_solidBrush->SetColor(D2D1::ColorF(0.49f, 0.88f, 0.76f, 1.0f));

    float centerY = origin.y + height / 2.0f;

    if (progress <= 0.0f) {
        // Paused state: clean static dots via FillEllipse
        for (int i = 0; i < 3; ++i) {
            float cx = origin.x + i * 5.0f + 1.25f;
            m_dcRenderTarget->FillEllipse(D2D1::Ellipse(D2D1::Point2F(cx, centerY), 1.25f, 1.25f), m_solidBrush.Get());
        }
        return;
    }

    float offsets[3] = {
        static_cast<float>(std::sin(progress * 6.2831853f) * 0.5f + 0.5f),
        static_cast<float>(std::sin(progress * 6.2831853f + 1.25f) * 0.5f + 0.5f),
        static_cast<float>(std::sin(progress * 6.2831853f + 2.5f) * 0.5f + 0.5f)
    };

    for (int i = 0; i < 3; ++i) {
        float barH = height * (0.3f + 0.7f * offsets[i]);
        if (barH < 3.0f) barH = 3.0f;
        float barW = 2.5f;
        float barLeft = origin.x + i * 5.0f;
        D2D1_ROUNDED_RECT r = D2D1::RoundedRect(
            D2D1::RectF(barLeft, centerY - barH / 2.0f, barLeft + barW, centerY + barH / 2.0f),
            1.25f, 1.25f
        );
        m_dcRenderTarget->FillRoundedRectangle(r, m_solidBrush.Get());
    }
}

void D2DRenderer::DrawBigText(const std::wstring& text, const D2D1_RECT_F& rect, D2D1_COLOR_F color) {
    if (!m_dcRenderTarget || !m_solidBrush) return;
    IDWriteTextFormat* format = m_textFormatBig.Get() ? m_textFormatBig.Get() : m_textFormatBold.Get();
    if (!format) return;

    m_solidBrush->SetColor(color);
    m_dcRenderTarget->DrawText(
        text.c_str(),
        static_cast<UINT32>(text.length()),
        format,
        rect,
        m_solidBrush.Get()
    );
}

void D2DRenderer::SetAlbumArtBitmap(const uint8_t* pixels, UINT width, UINT height) {
    if (!pixels || width == 0 || height == 0) {
        ClearAlbumArtBitmap();
        return;
    }
    CreateDeviceResources();
    if (!m_dcRenderTarget) return;

    D2D1_BITMAP_PROPERTIES props = D2D1::BitmapProperties(
        D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED)
    );
    D2D1_SIZE_U size = D2D1::SizeU(width, height);
    m_albumArtBitmap.Reset();
    HRESULT hr = m_dcRenderTarget->CreateBitmap(size, pixels, width * 4, props, m_albumArtBitmap.GetAddressOf());
    if (FAILED(hr)) {
        m_albumArtBitmap.Reset();
    }
}

void D2DRenderer::ClearAlbumArtBitmap() {
    m_albumArtBitmap.Reset();
}

void D2DRenderer::DrawAlbumArt(const D2D1_RECT_F& bounds, float cornerRadius) {
    if (!m_dcRenderTarget) return;

    float bw = bounds.right - bounds.left;
    float bh = bounds.bottom - bounds.top;
    float side = (bw < bh) ? bw : bh;
    float offsetX = bounds.left + (bw - side) / 2.0f;
    float offsetY = bounds.top + (bh - side) / 2.0f;
    D2D1_RECT_F squareBounds = D2D1::RectF(offsetX, offsetY, offsetX + side, offsetY + side);

    if (m_albumArtBitmap) {
        D2D1_SIZE_F bmpSize = m_albumArtBitmap->GetSize();
        if (bmpSize.width > 0.0f && bmpSize.height > 0.0f) {
            float scaleW = side / bmpSize.width;
            float scaleH = side / bmpSize.height;
            float scale = (scaleW > scaleH) ? scaleW : scaleH;
            float scaledW = bmpSize.width * scale;
            float scaledH = bmpSize.height * scale;
            float tx = offsetX + (side - scaledW) / 2.0f;
            float ty = offsetY + (side - scaledH) / 2.0f;

            ComPtr<ID2D1BitmapBrush> brush;
            D2D1_BITMAP_BRUSH_PROPERTIES brushProps = D2D1::BitmapBrushProperties(
                D2D1_EXTEND_MODE_CLAMP,
                D2D1_EXTEND_MODE_CLAMP,
                D2D1_BITMAP_INTERPOLATION_MODE_LINEAR
            );
            if (SUCCEEDED(m_dcRenderTarget->CreateBitmapBrush(m_albumArtBitmap.Get(), brushProps, brush.GetAddressOf()))) {
                brush->SetTransform(D2D1::Matrix3x2F::Scale(scale, scale) * D2D1::Matrix3x2F::Translation(tx, ty));
                D2D1_ROUNDED_RECT r = D2D1::RoundedRect(squareBounds, cornerRadius, cornerRadius);
                m_dcRenderTarget->FillRoundedRectangle(r, brush.Get());
                return;
            }
        }
    }

    // Dynamic gradient fallback (#ff7ab8 -> #6a5cff)
    D2D1_GRADIENT_STOP stops[2];
    stops[0].position = 0.0f;
    stops[0].color = D2D1::ColorF(1.0f, 0.478f, 0.722f, 1.0f); // #ff7ab8
    stops[1].position = 1.0f;
    stops[1].color = D2D1::ColorF(0.416f, 0.361f, 1.0f, 1.0f);   // #6a5cff

    ComPtr<ID2D1GradientStopCollection> stopCollection;
    if (SUCCEEDED(m_dcRenderTarget->CreateGradientStopCollection(stops, 2, stopCollection.GetAddressOf()))) {
        ComPtr<ID2D1LinearGradientBrush> gradBrush;
        D2D1_LINEAR_GRADIENT_BRUSH_PROPERTIES gradProps = D2D1::LinearGradientBrushProperties(
            D2D1::Point2F(squareBounds.left, squareBounds.top),
            D2D1::Point2F(squareBounds.right, squareBounds.bottom)
        );
        if (SUCCEEDED(m_dcRenderTarget->CreateLinearGradientBrush(gradProps, stopCollection.Get(), gradBrush.GetAddressOf()))) {
            D2D1_ROUNDED_RECT r = D2D1::RoundedRect(squareBounds, cornerRadius, cornerRadius);
            m_dcRenderTarget->FillRoundedRectangle(r, gradBrush.Get());
        }
    }
}

void D2DRenderer::DrawIconImage(IconType type, const D2D1_RECT_F& bounds, D2D1_COLOR_F color) {
    if (!m_dcRenderTarget || !m_d2dFactory || !m_solidBrush) return;
    DrawIcon(m_dcRenderTarget.Get(), m_d2dFactory.Get(), m_solidBrush.Get(), type, bounds, color);
}

void D2DRenderer::DrawButtonPill(const D2D1_RECT_F& bounds, const std::wstring& text, bool isHovered) {
    if (!m_dcRenderTarget || !m_solidBrush) return;
    float radius = (bounds.bottom - bounds.top) / 2.0f;
    D2D1_ROUNDED_RECT pill = D2D1::RoundedRect(bounds, radius, radius);

    if (isHovered) {
        m_solidBrush->SetColor(D2D1::ColorF(1.0f, 1.0f, 1.0f, 0.15f));
        m_dcRenderTarget->FillRoundedRectangle(pill, m_solidBrush.Get());
    }

    m_solidBrush->SetColor(D2D1::ColorF(1.0f, 1.0f, 1.0f, 0.25f));
    m_dcRenderTarget->DrawRoundedRectangle(pill, m_solidBrush.Get(), 1.0f);

    m_solidBrush->SetColor(D2D1::ColorF(D2D1::ColorF::White));
    IDWriteTextFormat* format = m_textFormatRegular.Get();
    if (format) {
        format->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
        format->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
        m_dcRenderTarget->DrawText(text.c_str(), static_cast<UINT32>(text.length()), format, bounds, m_solidBrush.Get());
        format->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);
        format->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_NEAR);
    }
}

void D2DRenderer::DrawCircularButton(const D2D1_RECT_F& bounds, IconType icon, bool isHovered) {
    if (!m_dcRenderTarget || !m_solidBrush) return;
    float w = bounds.right - bounds.left;
    float h = bounds.bottom - bounds.top;
    D2D1_POINT_2F center = D2D1::Point2F(bounds.left + w / 2.0f, bounds.top + h / 2.0f);
    float radius = (w < h ? w : h) / 2.0f;

    if (isHovered) {
        m_solidBrush->SetColor(D2D1::ColorF(1.0f, 1.0f, 1.0f, 0.15f));
        m_dcRenderTarget->FillEllipse(D2D1::Ellipse(center, radius, radius), m_solidBrush.Get());
    }

    // Centered icon inside button (icon size 14x14)
    float iconSize = 14.0f;
    D2D1_RECT_F iconBounds = D2D1::RectF(
        center.x - iconSize / 2.0f,
        center.y - iconSize / 2.0f,
        center.x + iconSize / 2.0f,
        center.y + iconSize / 2.0f
    );
    DrawIconImage(icon, iconBounds, D2D1::ColorF(D2D1::ColorF::White));
}

} // namespace Graphics
} // namespace DynamicIsland
