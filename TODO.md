# Project TODO and Roadmap - Win11 Dynamic Island

Central task board and backlog for the Win11-Dynamic-Island Windhawk mod.
Agents and developers must inspect this document during startup and keep task statuses updated.

---

## Priority and Status Legend

- [P0] Critical / Blocker: Must be addressed immediately (crashes, visual corruption, broken hooks).
- [P1] High Priority: Core feature, performance fixes, real system events, or prototype parity.
- [P2] Normal Priority: Visual polish, secondary interactions, or UX enhancements.
- [P3] Low / Nice-to-have: Optional tweaks, edge cases, and future explorations.
- Status markers:
  - [ ] Not Started
  - [/] In Progress
  - [x] Completed
  - [-] Cancelled / Deferred

---

## Current Focus (In Progress)

- [ ] [P0] Decouple window resizing and D2D backbuffer reallocation from animation ticks to eliminate stutter
- [ ] [P0] Overhaul click listener and hit-testing engine (fix jittery hitboxes, prevent accidental collapse, add DPI scaling)
- [ ] [P1] Implement real GSMTC media integration (real track title, artist, playback controls, and timeline)
- [ ] [P1] Implement center-cropped 1:1 square album art/thumbnail renderer (no distortion/squishing for rectangular art)
- [ ] [P1] Implement real-time WASAPI audio loopback visualizer (RMS envelope, circular buffer, symmetrical bars)
- [ ] [P1] Refactor microphone indicator into a transient pop-up HUD on app capture activation (not always visible)

---

## Task Backlog

### 1. UI, Direct2D Rendering and Smooth Animations
- [x] [P0] Replace colorkey transparency and implement layered alpha squircle rendering
- [x] [P0] DirectWrite trimming: prevent vertical text wrapping with ellipsis (...) truncation
- [ ] [P0] Decouple HWND resizing and ID2D1HwndRenderTarget::Resize from 16ms animation ticks
- [ ] [P0] Eliminate per-frame SetWindowRgn GDI calls during animation to prevent DWM composition stalls
- [ ] [P1] High-precision animation timer (timeBeginPeriod / multimedia timer / QPC delta timing)
- [ ] [P1] Symmetrical vertical waveform bars: render via FillRoundedRectangle expanding from centerY
- [ ] [P1] Waveform paused state: render clean static dots via FillEllipse instead of flat lines
- [ ] [P1] Reconcile vector element spacing and control positions with settled layouts
- [ ] [P2] Button hover, pressed, and active feedback states for media and timer controls
- [ ] [P2] High-DPI scaling validation across 100%, 125%, 150%, and 200% display scaling factors

### 2. Interaction, Hit-Testing and Input Handling
- [ ] [P0] Overhaul hit-test coordinate mapping (use settled target layout metrics instead of interpolating width)
- [ ] [P0] Implement robust WM_LBUTTONDOWN / WM_LBUTTONUP state machine with SetCapture / ReleaseCapture
- [ ] [P0] Fix premature collapse: clicking content body or dragging must not dismiss the island
- [ ] [P1] Support compact island click-to-expand across all valid event states
- [x] [P1] Global test hotkeys (Ctrl + Win + 1..9, 0) for manual HUD and scenario simulation
- [x] [P1] Context menu / right-click demo scenario cycling

### 3. Real System Event Integrations (No Placeholders)
- [ ] [P1] Media (GSMTC / WinRT):
  - [ ] Real-time session monitoring via GlobalSystemMediaTransportControlsSessionManager
  - [ ] Track title, artist, and playback status extraction
  - [ ] Real transport controls (Play, Pause, Skip Next, Skip Previous)
  - [ ] Stream album art thumbnail retrieval via IRandomAccessStreamReference
- [ ] [P1] Album Art 1:1 Square Cropping:
  - [ ] Aspect ratio calculation and center-crop algorithm for rectangular art
  - [ ] Distortion-free squircle rendering in compact (18x18) and expanded (48x48) states
- [ ] [P1] Waveform Visualizer & Audio Loopback Engine:
  - [ ] WASAPI audio loopback capture (AUDCLNT_STREAMFLAGS_LOOPBACK on default eRender/eConsole endpoint)
  - [ ] Power efficiency gating: capture only runs when media is playing (playing == true) and media UI is visible
  - [ ] Stall watchdog: auto-reconnect WASAPI client if media reports playing but stream receives no data > 5s
  - [ ] RMS signal calculation: per-chunk (64 frames) RMS = sqrt(sum(v^2)/N) * 4.0f, clamped to [0.0, 1.0]
  - [ ] Attack / Release envelope filter: fast attack 0.7 (punchy beats) and soft release 0.85 (smooth decay)
  - [ ] Circular buffer history (std::array<float, 48>) with time-delay offset mapping across visualizer bars
  - [ ] Volume-reactive opacity: dynamic accent brush opacity based on momentary amplitude (0.45f + 0.5f * amp)
  - [ ] Frame rate synchronizer: maintain ~60 FPS during active playback, throttle down to idle rate when paused
- [ ] [P1] Microphone Privacy Event:
  - [ ] Transition detection (app starts using microphone) via Registry / CoreAudio capture streams
  - [ ] Display as transient pop-up HUD (2.8s timeout) with app name and orange privacy dot
  - [ ] Automatic restoration of underlying media or idle state upon expiry (never locked in always-visible)
- [ ] [P1] Audio / Volume: Real MMDevice endpoint volume monitoring via IAudioEndpointVolumeCallback
- [ ] [P2] Power / Battery: Real AC plug/unplug and charge percentage events via RegisterPowerSettingNotification
- [ ] [P2] Keyboard: Real Caps Lock and Num Lock state toggles via raw input / LL hook
- [ ] [P2] Bluetooth: Battery and connection state parsing for connected peripherals

### 4. Shell Geometry and Taskbar Adaptation
- [x] [P1] Handle WM_SETTINGCHANGE, WM_DISPLAYCHANGE, WM_DPICHANGED, and TaskbarCreated in IslandWindow
- [x] [P1] Real-time taskbar edge and size change watcher
- [ ] [P2] Multi-monitor taskbar shift and primary display transition detection
- [ ] [P2] Auto-hide taskbar detection (ABM_GETTASKBARPOS) and tuck-away behavior

### 5. Visibility Policies and Configuration
- [x] [P1] Implement idleVisibilityMode setting (always_visible vs events_only)
- [x] [P1] Implement mediaVisibilityPolicy setting (always_visible vs track_change_only)
- [x] [P1] Synchronize src/metadata/mod_settings.h schema with runtime LoadSettings() in main.cpp
- [ ] [P2] Auto-collapse timer setting (autoCollapseSeconds) and pause-on-hover logic

### 6. Stability, Performance and Verification
- [ ] [P1] Verify bundle integrity with python scripts/bundle.py
- [ ] [P1] Memory leak check for WinRT COM objects, WIC decoders, and Direct2D resources
- [ ] [P2] Audio thread cleanup and thread safety: ensure lock-free or lightweight mutex synchronization for waveform buffer
- [ ] [P2] Ensure zero UI thread frame drops during rapid system notifications

---

## Completed Tasks

- [x] Initial modular C++ structure setup with automated bundler script (scripts/bundle.py)
- [x] Base Direct2D layered window and animation spring physics implementation
- [x] Eliminate rectangular halo artifacts and colorkey fringe via dynamic squircle region clipping and layered alpha
- [x] Prevent vertical text wrapping and line overflow with DirectWrite no-wrap and ellipsis trimming
- [x] Implement global debug hotkeys (Ctrl+Win+1..9, 0) and enhanced demo scenario cycling
- [x] Implement real-time taskbar movement and display adaptation (WM_SETTINGCHANGE, WM_DISPLAYCHANGE, WM_DPICHANGED, TaskbarCreated, and geometry polling)
- [x] Implement idleVisibilityMode and mediaVisibilityPolicy configuration settings with runtime schema synchronization
