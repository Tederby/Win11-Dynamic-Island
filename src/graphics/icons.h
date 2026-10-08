#pragma once

#include <windows.h>
#include <d2d1.h>
#include <wrl/client.h>

namespace DynamicIsland {
namespace Graphics {

// Icon identifiers corresponding to the SVG paths in prototype.html
enum class IconType {
    Play,
    Pause,
    Previous,
    Next,
    Volume,
    CapsLock,
    Bolt,
    Bluetooth,
    Battery,
    Checkmark
};

// SVG path definitions from prototype.html
namespace IconPaths {
    constexpr const char* PLAY      = "M8 5v14l11-7z";
    constexpr const char* PAUSE     = "M6 5h4v14H6zm8 0h4v14h-4z";
    constexpr const char* PREV      = "M6 6h2v12H6zm3 6 9-6v12z";
    constexpr const char* NEXT      = "M16 6h2v12h-2zM6 18V6l9 6z";
    constexpr const char* VOLUME    = "M4 9v6h4l5 4V5L8 9z";
    constexpr const char* CAPS_LOCK = "M12 4 4 13h5v5h6v-5h5z";
    constexpr const char* BOLT      = "M13 2 5 14h6l-1 8 8-12h-6z";
    constexpr const char* BLUETOOTH = "M7 7l10 10-5 5V2l5 5L7 17";
    constexpr const char* BATTERY   = "M3 8h16v8H3zM20 11h2v2h-2z";
    constexpr const char* CHECKMARK = "M5 12l5 5 9-10-2-2-7 8-3-3z";
}

void DrawIcon(
    ID2D1RenderTarget* rt,
    ID2D1Factory* factory,
    ID2D1SolidColorBrush* brush,
    IconType type,
    const D2D1_RECT_F& bounds,
    D2D1_COLOR_F color
);

} // namespace Graphics
} // namespace DynamicIsland
