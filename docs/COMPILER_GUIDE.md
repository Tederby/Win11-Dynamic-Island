# Windhawk Compilation & Linker Guide - Win11 Dynamic Island

This document is the **definitive guide** for compiling and linking the Dynamic Island mod in Windhawk.
It documents all known compiler and linker pitfalls, root causes, required libraries, and verification workflows so that compilation errors never recur.

---

## 1. Windhawk Toolchain Architecture

Windhawk does not use MSVC (`cl.exe`). It uses a custom **LLVM Clang / MinGW-w64** toolchain:
- **Compiler**: `clang-20.exe` (located in `D:\Program Files\Windhawk\Compiler\bin` or `C:\Program Files\Windhawk\Compiler\bin`)
- **Linker**: `ld.lld.exe` (LLVM LLD linker)
- **Targets**: Windhawk compiles mods for **both 32-bit (`i686-w64-windows-gnu`) and 64-bit (`x86_64-w64-windows-gnu`)**, plus `aarch64` when supported.
- **Language Standard**: **C++20** (`-std=c++20`). C++/WinRT headers in Clang 20 require coroutine support; compiling with `-std=c++17` triggers fatal errors in `<winrt/base.h>`.
- **Pre-included Header**: Windhawk automatically injects `#include <windhawk_api.h>` and defines `WH_MOD`.
- **Compiler Options Override**: The metadata header `// @compilerOptions` **replaces or augments** compiler/linker arguments. If a Windows library is not explicitly specified in `@compilerOptions`, **`ld.lld` will NOT link it**.

---

## 2. Definitive `@compilerOptions` Reference

The mod header in `src/metadata/mod_header.h` (and the bundled `.wh.cpp`) **must include all of the following libraries**:

```cpp
// @compilerOptions -ld2d1 -ldwrite -lwindowscodecs -luxtheme -lole32 -loleaut32 -lruntimeobject -lwindowsapp -lshcore -lversion -lgdi32 -ldwmapi -luser32 -lshell32 -ladvapi32
```

### Breakdown of Required Libraries:

| Library | Flag | Key Symbols / APIs Used | Why It Is Mandatory |
|:---|:---|:---|:---|
| **Direct2D** | `-ld2d1` | `D2D1CreateFactory`, `ID2D1HwndRenderTarget` | Hardware-accelerated UI rendering |
| **DirectWrite** | `-ldwrite` | `DWriteCreateFactory`, `IDWriteTextFormat` | Text layout and rendering |
| **WIC** | `-lwindowscodecs` | `CLSID_WICImagingFactory`, `IWICBitmapDecoder` | Album art decoding and image conversion |
| **UxTheme** | `-luxtheme` | `SetWindowTheme` | Windows theme hooks |
| **OLE / COM** | `-lole32` | `CoInitializeEx`, `CoCreateInstance`, `PropVariantClear` | COM runtime, Core Audio / WASAPI |
| **OLE Automation** | `-loleaut32` | `SysFreeString`, `SysStringLen`, `_SysFreeString@4` | BSTR handling required by C++/WinRT `hresult_error` |
| **WinRT Runtime** | `-lruntimeobject` | `RoGetActivationFactory`, `RoOriginateLanguageException` | WinRT activation factory infrastructure |
| **Windows App** | `-lwindowsapp` | WinRT core runtime (`Windows.Media.Control`, GSMTC) | Windows Runtime umbrella import library |
| **Shell Core** | `-lshcore` | `GetDpiForMonitor` | Per-monitor DPI scaling |
| **Version** | `-lversion` | `GetFileVersionInfoW`, `VerQueryValueW` | Process and DLL version queries |
| **GDI** | `-lgdi32` | `CreateCompatibleDC`, `SelectObject`, `DeleteObject` | Fallback GDI primitives |
| **DWM** | `-ldwmapi` | `DwmExtendFrameIntoClientArea` | Hardware anti-aliased squircle window transparency |
| **User32** | `-luser32` | `FindWindowW`, `GetDesktopWindow`, `SetWindowLongW`, `DestroyWindow`, `SetCapture`, `ReleaseCapture`, `TrackMouseEvent` | Window management, hit-testing, and message pump |
| **Shell32** | `-lshell32` | `SHAppBarMessage` | Taskbar edge and auto-hide detection |
| **AdvAPI32** | `-ladvapi32` | `RegOpenKeyExW`, `RegQueryValueExW`, `RegCloseKey` | Microphone ConsentStore privacy capture detection |

---

## 3. Catalog of Compiler & Linker Pitfalls

### Pitfall 1: Win32 Core APIs Undefined (`_FindWindowW@8`, `_SetCapture@4`, `_SHAppBarMessage@8`)
- **Symptoms**:
  ```text
  ld.lld: error: undefined symbol: __declspec(dllimport) _FindWindowW@8
  ld.lld: error: undefined symbol: __declspec(dllimport) _SHAppBarMessage@8
  ld.lld: error: undefined symbol: __declspec(dllimport) _SetCapture@4
  ```
- **Root Cause**: `@compilerOptions` was missing `-luser32`, `-lshell32`, or `-ladvapi32`. In MinGW/LLVM toolchains, specifying `@compilerOptions` causes `ld.lld` to rely strictly on the explicitly listed libraries.
- **Rule**: Never remove `-luser32 -lshell32 -ladvapi32` from `@compilerOptions`.

---

### Pitfall 2: WinRT Linker Undefined Symbols (`_SysFreeString@4`, `_RoGetActivationFactory@12`)
- **Symptoms**:
  ```text
  ld.lld: error: undefined symbol: _SysFreeString@4
  ld.lld: error: undefined symbol: _SysStringLen@4
  ld.lld: error: undefined symbol: _RoOriginateLanguageException@12
  ld.lld: error: undefined symbol: _RoGetActivationFactory@12
  ```
- **Root Cause**: C++/WinRT headers instantiate `winrt::hresult_error` and `winrt::param::hstring`, which call `SysFreeString` (in `oleaut32`) and `RoGetActivationFactory` / `RoOriginateLanguageException` (in `runtimeobject` and `windowsapp`).
- **Rule**: Whenever any `<winrt/...>` header is included, `@compilerOptions` MUST contain `-loleaut32 -lruntimeobject -lwindowsapp`.

---

### Pitfall 3: C++/WinRT Collection Range-Based For Loop (`begin()` Deduced Return Type Error)
- **Symptoms**:
  ```text
  <stdin>:3140:32: error: function 'begin' with deduced return type cannot be used before it is defined
   3140 |             for (auto const& s : sessions) {
        |                                ^
  note: 'begin' declared here: auto begin() const;
  ```
- **Root Cause**: Clang 20 cannot deduce the return type of `begin()` / `end()` on `IVectorView<T>` if template specializations are evaluated out of order, or if `<winrt/Windows.Foundation.Collections.h>` is not fully resolved in single-file translation units.
- **Rule**: **NEVER use range-based for loops over C++/WinRT collections** (`IVectorView`, `IVector`).
  Always iterate with index-based loops:
  ```cpp
  // CORRECT:
  uint32_t count = sessions.Size();
  for (uint32_t i = 0; i < count; ++i) {
      auto session = sessions.GetAt(i);
      // ...
  }

  // FORBIDDEN:
  for (auto const& s : sessions) { ... }
  ```

---

### Pitfall 4: C++/WinRT Missing Coroutine Support (`-std=c++20`)
- **Symptoms**:
  ```text
  D:/Program Files/Windhawk/Compiler/include/winrt/base.h:84:2: error: C++/WinRT requires coroutine support, which is currently missing. Try enabling C++20 in your compiler.
  ```
- **Root Cause**: C++/WinRT 2.0 headers require standard coroutines (`<coroutine>`), which became standard in C++20. Windhawk's compiler operates in C++20 mode.
- **Rule**: Any offline verification script MUST compile with `-std=c++20`.

---

### Pitfall 5: Bundler Hoisting Conditional `#include` Directives
- **Symptoms**:
  Headers inside `#if` or `#ifndef` blocks were being pulled to the top of `win11-dynamic-island.wh.cpp`, breaking conditional compilation.
- **Root Cause**: `scripts/bundle.py` was previously matching all `#include` lines globally with regex and moving them to the header block.
- **Rule**: The bundler must track `#if / #ifdef / #ifndef / #endif` nesting depth. Only unnested global `#include` directives may be hoisted; conditional includes must stay inside their `#if` blocks.

---

## 4. Verification Workflow for Developers & AI Agents

**DO NOT rely on `g++ -fsyntax-only` alone.** Always verify the bundled mod with Windhawk's native Clang toolchain before reporting success to the user.

### Automated Verification Command
Run the verification script:
```bash
python scripts/verify.py
```
Or via npm:
```bash
npm run verify
```

### Manual Verification Command
To manually verify against both 32-bit and 64-bit targets:

```powershell
# 1. 32-bit (i686) Target
& "D:\Program Files\Windhawk\Compiler\bin\clang++.exe" -shared -target i686-w64-windows-gnu `
    -std=c++20 -DUNICODE -D_UNICODE `
    -include "D:\Program Files\Windhawk\Compiler\include\windhawk_api.h" `
    "win11-dynamic-island.wh.cpp" `
    -ld2d1 -ldwrite -lwindowscodecs -luxtheme -lole32 -loleaut32 -lruntimeobject -lwindowsapp `
    -lshcore -lversion -lgdi32 -ldwmapi -luser32 -lshell32 -ladvapi32 `
    -o "$env:TEMP\test_mod_i686.dll"

# 2. 64-bit (x86_64) Target
& "D:\Program Files\Windhawk\Compiler\bin\clang++.exe" -shared -target x86_64-w64-windows-gnu `
    -std=c++20 -DUNICODE -D_UNICODE `
    -include "D:\Program Files\Windhawk\Compiler\include\windhawk_api.h" `
    "win11-dynamic-island.wh.cpp" `
    -ld2d1 -ldwrite -lwindowscodecs -luxtheme -lole32 -loleaut32 -lruntimeobject -lwindowsapp `
    -lshcore -lversion -lgdi32 -ldwmapi -luser32 -lshell32 -ladvapi32 `
    -o "$env:TEMP\test_mod_x64.dll"
```
Both commands **must exit with code 0**. Clean up temporary DLLs afterwards.
