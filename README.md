# Dynamic Island for Windows 11

[![Windhawk Mod](https://img.shields.io/badge/Windhawk-Mod-blue.svg)](https://windhawk.net/)
[![Platform](https://img.shields.io/badge/Platform-Windows%2011-0078D6.svg)](https://microsoft.com/windows)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)
[![Architecture](https://img.shields.io/badge/Architecture-Modular%20C%2B%2B-success.svg)](#architecture)

An interactive, fluid **Dynamic Island** overlay and taskbar companion for Windows 11. Designed as a native **Windhawk** mod, it brings smooth Apple-inspired micro-interactions, live activity indicators, and transient system notifications directly to the Windows desktop.

---

## 🌟 Overview & Vision

Inspired by Apple's Dynamic Island and the community project [`devcode90/Dynamic-Island-for-Windows`](https://github.com/devcode90/Dynamic-Island-for-Windows), this project expands the concept into a comprehensive, multi-scenario experience tailored specifically for Windows 11.

While previous implementations were limited in taskbar edge support and event handling, **Win11-Dynamic-Island** introduces:
1. **Adaptive Dual-Mode Placement**: Seamlessly integrates into top-docked taskbars as an embedded element, or floats above bottom/side taskbars as an independent, interactive pill.
2. **Interactive Live Activities**: Persistent, expandable cards for media playback, focus sessions, and privacy indicators.
3. **Smart HUD Notifications**: Low-latency, non-focus-stealing transient pills for volume changes, Caps Lock toggles, AC charging states, Bluetooth battery info, and more.
4. **Authentic Spring Physics**: Fluid easing based on `cubic-bezier(0.34, 1.3, 0.5, 1.0)` that morphs smoothly between compact and expanded states.
5. **Interactive UI Reference**: Bundled with [`Dynamic Island Win11 - preview.html`](./Dynamic%20Island%20Win11%20-%20preview.html) showcasing the intended visual design and behavior.

---

## 🎨 Interactive Prototype Reference

You can preview the interactive UI design and transition states directly in your browser:
- Open [`Dynamic Island Win11 - preview.html`](./Dynamic%20Island%20Win11%20-%20preview.html) in any modern browser.
- Test taskbar positions (Top, Bottom, Left, Right) and sizes (Normal 40px, Compact 34px).
- Toggle auto-hide, live scenarios (Media, Focus Timer, Microphone), and transient events (Volume, Caps Lock, Charger, Bluetooth, Low Battery).

---

## 🚀 Key Features

### 1. Adaptive Placement Engine
- **Top Taskbar**: The island embeds directly into the taskbar background, matching height and expanding downwards.
- **Bottom Taskbar**: Floats right above the taskbar in the center, expanding upwards with spring physics.
- **Side Taskbars (Left / Right)**: Dynamically aligns to the center of the available working area.
- **Auto-Hide Awareness**: Monitors taskbar hide states via `SHAppBarMessage` and tucks away cleanly to prevent screen obstruction.

### 2. Live Activities (Persistent & Expandable)
- **Now Playing**:
  - *Compact*: Album art thumbnail, marquee track title, and animated equalizer waves.
  - *Expanded*: Large artwork, song title and artist, interactive progress bar, previous/play/pause/next controls.
- **Focus / Pomodoro Timer**:
  - *Compact*: Radial progress ring and countdown (`MM:SS`).
  - *Expanded*: Large tabular-numeric timer display, session indicator, and pause/resume/stop buttons.
- **Microphone & Privacy Dot**:
  - Amber/green privacy indicator showing when recording streams or conferencing apps (Discord, Teams, Zoom) are active.

### 3. Transient HUD Events (Auto-dismissing)
- **Volume**: Clean slider bar with instant percentage feedback.
- **Caps Lock**: Pill indicator confirming `ON` or `OFF` status without full-screen overlays.
- **Power & Battery**: AC charger connected toast and battery low warnings (<20%).
- **Bluetooth Devices**: Connection badges showing peripheral name and battery level (e.g., WH-1000XM5 at 80%).
- **Timer Finished**: Confirmation badge upon focus session completion.

---

## 🏗️ Architecture & Modular Workflow

Windhawk compiles mods from a single monolithic `.wh.cpp` file. To maintain clean engineering practices, this repository is organized into a modular C++ structure and uses an automated bundler:

```
win11-dynamic-island/
├── src/
│   ├── metadata/            # Windhawk mod headers, readme, and YAML settings schema
│   │   ├── mod_header.h
│   │   ├── mod_readme.h
│   │   └── mod_settings.h
│   ├── common/              # Types, enums, constants, DPI scaling, and logging
│   │   ├── defs.h
│   │   ├── log.h
│   │   └── utils.h / utils.cpp
│   ├── platform/            # Windows taskbar geometry, monitors, and auto-hide
│   │   └── taskbar.h / taskbar.cpp
│   ├── graphics/            # Direct2D hardware rendering, spring animator, vector paths
│   │   ├── animation.h / animation.cpp
│   │   ├── d2d_renderer.h / d2d_renderer.cpp
│   │   └── icons.h
│   ├── overlay/             # Layered window management, layout calculations, click handling
│   │   ├── layout.h / layout.cpp
│   │   └── island_window.h / island_window.cpp
│   ├── services/            # Background service monitors & priority queue
│   │   ├── service_manager.h / service_manager.cpp
│   │   ├── media_service.h / media_service.cpp
│   │   ├── audio_service.h / audio_service.cpp
│   │   ├── power_service.h / power_service.cpp
│   │   ├── keyboard_service.h / keyboard_service.cpp
│   │   ├── bluetooth_service.h / bluetooth_service.cpp
│   │   └── timer_service.h / timer_service.cpp
│   └── main.cpp             # Main orchestrator & Windhawk entry hooks
├── scripts/
│   └── bundle.py            # Python bundler that resolves modular code into .wh.cpp
├── Dynamic Island Win11 - preview.html # Interactive HTML/CSS reference prototype
├── win11-dynamic-island.wh.cpp # Generated monolithic Windhawk mod
└── package.json             # NPM convenience scripts
```

For deeper technical details, see [ARCHITECTURE.md](ARCHITECTURE.md).

---

## 🛠️ Development & Building

### Prerequisites
- Windows 10/11
- [Windhawk](https://windhawk.net/) (for compiling and loading the mod)
- Python 3.8+ (for bundling)
- Node.js (optional, for `npm run` scripts)

### Bundling Modular Code into Monolithic Mod
Run the bundler script whenever you edit files in `src/`:

Using Python:
```bash
python scripts/bundle.py
```

Or using npm:
```bash
npm run bundle
```

### Auto-Watch Mode
Automatically re-bundle whenever any file in `src/` changes:
```bash
python scripts/bundle.py --watch
# or
npm run bundle:watch
```

---

## 📥 Loading into Windhawk

1. Open **Windhawk**.
2. Navigate to the **Development** tab.
3. Click **New Mod** or edit an existing mod.
4. Copy the entire contents of `win11-dynamic-island.wh.cpp` into the Windhawk code editor.
5. In mod settings, ensure target process is set to `explorer.exe`.
6. Click **Compile and run** (<kbd>Ctrl+B</kbd>).

---

## ⚙️ Configuration

The mod exposes extensive user customization options directly within the Windhawk UI:

| Setting | Type | Default | Description |
|---|---|---|---|
| `placementMode` | Enum | `auto` | Auto-detect taskbar position, force top-embed, or force floating. |
| `enableMedia` | Boolean | `true` | Show Now Playing track details, marquee, and playback controls. |
| `enableTimer` | Boolean | `true` | Enable Pomodoro / focus timer with countdown progress ring. |
| `enableMicStatus` | Boolean | `true` | Show privacy dot when microphone is in use. |
| `enableVolumeHUD` | Boolean | `true` | Show compact HUD when system volume changes. |
| `enableCapsLockHUD` | Boolean | `true` | Display HUD indicator on Caps Lock toggle. |
| `enablePowerHUD` | Boolean | `true` | Show charger connection toasts and low battery warnings. |
| `enableBluetoothHUD`| Boolean | `true` | Display connection status and peripheral battery level. |
| `autoCollapseSeconds`| Number | `5` | Inactivity delay in seconds before collapsing expanded view. |
| `animationSpeed` | Float | `1.0` | Multiplier for spring animation speed. |

---

## 📄 License

This project is licensed under the [MIT License](LICENSE).

## 🙏 Acknowledgements
- [Windhawk](https://windhawk.net/) by Ramen Software for the runtime and mod engine.
- [devcode90/Dynamic-Island-for-Windows](https://github.com/devcode90/Dynamic-Island-for-Windows) for the early inspiration and concept exploration.
