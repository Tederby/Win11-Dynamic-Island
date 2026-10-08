#include "icons.h"

namespace DynamicIsland {
namespace Graphics {

namespace {

inline float ScaleX(float x, const D2D1_RECT_F& bounds) {
    return bounds.left + (x / 24.0f) * (bounds.right - bounds.left);
}

inline float ScaleY(float y, const D2D1_RECT_F& bounds) {
    return bounds.top + (y / 24.0f) * (bounds.bottom - bounds.top);
}

inline D2D1_POINT_2F Pt(float x, float y, const D2D1_RECT_F& bounds) {
    return D2D1::Point2F(ScaleX(x, bounds), ScaleY(y, bounds));
}

void DrawPolygon(
    ID2D1RenderTarget* rt,
    ID2D1Factory* factory,
    ID2D1SolidColorBrush* brush,
    const D2D1_POINT_2F* points,
    UINT count
) {
    if (!rt || !factory || !brush || count < 3) return;

    Microsoft::WRL::ComPtr<ID2D1PathGeometry> path;
    if (SUCCEEDED(factory->CreatePathGeometry(path.GetAddressOf()))) {
        Microsoft::WRL::ComPtr<ID2D1GeometrySink> sink;
        if (SUCCEEDED(path->Open(sink.GetAddressOf()))) {
            sink->BeginFigure(points[0], D2D1_FIGURE_BEGIN_FILLED);
            sink->AddLines(&points[1], count - 1);
            sink->EndFigure(D2D1_FIGURE_END_CLOSED);
            sink->Close();
            rt->FillGeometry(path.Get(), brush);
        }
    }
}

void DrawPolyline(
    ID2D1RenderTarget* rt,
    ID2D1Factory* factory,
    ID2D1SolidColorBrush* brush,
    const D2D1_POINT_2F* points,
    UINT count,
    float strokeWidth
) {
    if (!rt || !factory || !brush || count < 2) return;

    Microsoft::WRL::ComPtr<ID2D1PathGeometry> path;
    if (SUCCEEDED(factory->CreatePathGeometry(path.GetAddressOf()))) {
        Microsoft::WRL::ComPtr<ID2D1GeometrySink> sink;
        if (SUCCEEDED(path->Open(sink.GetAddressOf()))) {
            sink->BeginFigure(points[0], D2D1_FIGURE_BEGIN_HOLLOW);
            sink->AddLines(&points[1], count - 1);
            sink->EndFigure(D2D1_FIGURE_END_OPEN);
            sink->Close();
            rt->DrawGeometry(path.Get(), brush, strokeWidth);
        }
    }
}

} // anonymous namespace

void DrawIcon(
    ID2D1RenderTarget* rt,
    ID2D1Factory* factory,
    ID2D1SolidColorBrush* brush,
    IconType type,
    const D2D1_RECT_F& bounds,
    D2D1_COLOR_F color
) {
    if (!rt || !factory || !brush) return;

    brush->SetColor(color);

    switch (type) {
        case IconType::Play: {
            D2D1_POINT_2F pts[] = {
                Pt(8.0f, 5.0f, bounds),
                Pt(8.0f, 19.0f, bounds),
                Pt(19.0f, 12.0f, bounds)
            };
            DrawPolygon(rt, factory, brush, pts, 3);
            break;
        }

        case IconType::Pause: {
            D2D1_RECT_F r1 = D2D1::RectF(
                ScaleX(6.0f, bounds), ScaleY(5.0f, bounds),
                ScaleX(10.0f, bounds), ScaleY(19.0f, bounds)
            );
            D2D1_RECT_F r2 = D2D1::RectF(
                ScaleX(14.0f, bounds), ScaleY(5.0f, bounds),
                ScaleX(18.0f, bounds), ScaleY(19.0f, bounds)
            );
            rt->FillRectangle(r1, brush);
            rt->FillRectangle(r2, brush);
            break;
        }

        case IconType::Previous: {
            D2D1_RECT_F bar = D2D1::RectF(
                ScaleX(6.0f, bounds), ScaleY(6.0f, bounds),
                ScaleX(8.0f, bounds), ScaleY(18.0f, bounds)
            );
            rt->FillRectangle(bar, brush);

            D2D1_POINT_2F pts[] = {
                Pt(9.0f, 12.0f, bounds),
                Pt(18.0f, 6.0f, bounds),
                Pt(18.0f, 18.0f, bounds)
            };
            DrawPolygon(rt, factory, brush, pts, 3);
            break;
        }

        case IconType::Next: {
            D2D1_RECT_F bar = D2D1::RectF(
                ScaleX(16.0f, bounds), ScaleY(6.0f, bounds),
                ScaleX(18.0f, bounds), ScaleY(18.0f, bounds)
            );
            rt->FillRectangle(bar, brush);

            D2D1_POINT_2F pts[] = {
                Pt(6.0f, 6.0f, bounds),
                Pt(15.0f, 12.0f, bounds),
                Pt(6.0f, 18.0f, bounds)
            };
            DrawPolygon(rt, factory, brush, pts, 3);
            break;
        }

        case IconType::Volume: {
            D2D1_POINT_2F pts[] = {
                Pt(4.0f, 9.0f, bounds),
                Pt(8.0f, 9.0f, bounds),
                Pt(13.0f, 5.0f, bounds),
                Pt(13.0f, 19.0f, bounds),
                Pt(8.0f, 15.0f, bounds),
                Pt(4.0f, 15.0f, bounds)
            };
            DrawPolygon(rt, factory, brush, pts, 6);
            break;
        }

        case IconType::CapsLock: {
            D2D1_POINT_2F pts[] = {
                Pt(12.0f, 4.0f, bounds),
                Pt(4.0f, 13.0f, bounds),
                Pt(9.0f, 13.0f, bounds),
                Pt(9.0f, 18.0f, bounds),
                Pt(15.0f, 18.0f, bounds),
                Pt(15.0f, 13.0f, bounds),
                Pt(20.0f, 13.0f, bounds)
            };
            DrawPolygon(rt, factory, brush, pts, 7);
            break;
        }

        case IconType::Bolt: {
            D2D1_POINT_2F pts[] = {
                Pt(13.0f, 2.0f, bounds),
                Pt(5.0f, 14.0f, bounds),
                Pt(11.0f, 14.0f, bounds),
                Pt(10.0f, 22.0f, bounds),
                Pt(18.0f, 10.0f, bounds),
                Pt(12.0f, 10.0f, bounds)
            };
            DrawPolygon(rt, factory, brush, pts, 6);
            break;
        }

        case IconType::Bluetooth: {
            D2D1_POINT_2F pts[] = {
                Pt(7.0f, 7.0f, bounds),
                Pt(17.0f, 17.0f, bounds),
                Pt(12.0f, 22.0f, bounds),
                Pt(12.0f, 2.0f, bounds),
                Pt(17.0f, 7.0f, bounds),
                Pt(7.0f, 17.0f, bounds)
            };
            float strokeW = (bounds.right - bounds.left) * (1.8f / 24.0f);
            DrawPolyline(rt, factory, brush, pts, 6, strokeW > 1.0f ? strokeW : 1.0f);
            break;
        }

        case IconType::Battery: {
            // Main body outline
            D2D1_RECT_F body = D2D1::RectF(
                ScaleX(3.0f, bounds), ScaleY(8.0f, bounds),
                ScaleX(19.0f, bounds), ScaleY(16.0f, bounds)
            );
            D2D1_ROUNDED_RECT roundedBody = D2D1::RoundedRect(body, 2.0f, 2.0f);
            rt->DrawRoundedRectangle(roundedBody, brush, 1.5f);

            // Terminal tip
            D2D1_RECT_F tip = D2D1::RectF(
                ScaleX(19.5f, bounds), ScaleY(11.0f, bounds),
                ScaleX(21.5f, bounds), ScaleY(13.0f, bounds)
            );
            D2D1_ROUNDED_RECT roundedTip = D2D1::RoundedRect(tip, 0.8f, 0.8f);
            rt->FillRoundedRectangle(roundedTip, brush);

            // Fill partial bar inside
            D2D1_RECT_F fill = D2D1::RectF(
                ScaleX(4.5f, bounds), ScaleY(9.5f, bounds),
                ScaleX(11.0f, bounds), ScaleY(14.5f, bounds)
            );
            rt->FillRectangle(fill, brush);
            break;
        }

        case IconType::Checkmark: {
            D2D1_POINT_2F pts[] = {
                Pt(5.0f, 12.0f, bounds),
                Pt(10.0f, 17.0f, bounds),
                Pt(19.0f, 7.0f, bounds),
                Pt(17.0f, 5.0f, bounds),
                Pt(10.0f, 13.0f, bounds),
                Pt(7.0f, 10.0f, bounds)
            };
            DrawPolygon(rt, factory, brush, pts, 6);
            break;
        }
    }
}

} // namespace Graphics
} // namespace DynamicIsland
