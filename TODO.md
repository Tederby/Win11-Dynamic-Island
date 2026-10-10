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

## Current Focus (Dual-Mode Interactive Architecture & Polish)

- [ ] [P0] Media Pause Solid-Black Bugfix:
  - Prevent island from dropping live activity and turning solid black when paused while expanded.
  - Preserve expanded media layout during pause state with seamless toggle of the play/pause button icon (showing Play icon instead of Pause) and freezing marquee/waveform.
- [ ] [P1] Media Progress Bar Removal & Layout Rebalance:
  - Remove buggy media timeline progress bar completely from expanded media rendering.
  - Rebalance vertical spacing and control button hit-test areas below track title and artist.
- [ ] [P1] Idle Click-to-Expand (Clock & Date View):
  - Enable compact idle pill click event to expand island into a rich Time and Date view (large clock, day of week, full date).
- [ ] [P1] Two-Tier Physical Interaction Architecture (Collapsed vs Expanded) with Mouse Wheel Scrolling:
  - **Collapsed Mode (Compact Pill):** Implement `WM_MOUSEWHEEL` handling to cycle active compact widgets (e.g., Clock & Weather -> Hardware Monitoring -> etc.).
  - **Expanded Mode (Multi-Page Island):** Implement `WM_MOUSEWHEEL` handling to smoothly switch between expanded pages (Page 1: Time & Date Summary; Page 2: Weather & Calendar; etc.).
  - **Non-blocking Media Navigation:** When media is active and expanded, allow mouse wheel scrolling to inspect other pages (Time/Date, Weather) rather than locking the user exclusively into the media view.
- [ ] [P2] Idle Weather Integration & Windhawk Formatting Settings:
  - Integrate lightweight weather service fetching current temperature (location-based auto detection or manual setting).
  - Add Windhawk mod settings for Temperature Unit (Celsius vs Fahrenheit) and Clock Format (12-Hour AM/PM vs 24-Hour).

---

## Task Backlog

### 1. UI, Direct2D Rendering and Smooth Animations
- [x] [P0] True Per-Pixel Alpha Composition: Eliminate opaque black rectangular backdrop (~367x166px) while maintaining hardware anti-aliasing on squircle edges
- [x] [P0] Pure Pitch-Black Island Styling: Island background strictly `#000000` with 0 outline stroke
- [x] [P1] High-precision animation timer: QPC delta timing and `timeBeginPeriod(1)` supporting 60Hz, 120Hz, 144Hz, and 240Hz displays
- [x] [P1] Always-visible idle content: Minimal digital clock (HH:mm)
- [x] [P1] Idle Pill Visual Enhancement: Precisely center digital clock typography vertically and horizontally; enrich idle pill with sleek aesthetics and subtle status indicator dots
- [x] [P1] Void Entry Animation (`events_only` mode): Organic spring scale-up and opacity fade-in transition when island awakens from hidden state instead of abruptly popping in
- [x] [P1] Same-Size Event Transition & Crossfade: Implement smooth content crossfade/morph when transitioning between events of identical size (e.g. CapsLock to Volume)
- [x] [P1] Continuous Looping Marquee: Convert title/artist marquee to seamless continuous wrapping loop and fix acceleration glitch caused by concurrent background event ticks
- [x] [P0] DirectWrite trimming: prevent vertical text wrapping with ellipsis (...) truncation
- [x] [P0] Decouple HWND resizing and ID2D1HwndRenderTarget::Resize from 16ms animation ticks
- [x] [P1] Symmetrical vertical waveform bars: render via FillRoundedRectangle expanding from centerY
- [x] [P1] Waveform paused state: render clean static dots via FillEllipse instead of flat lines
- [ ] [P1] Reconcile vector element spacing and control positions after removing media progress bar
- [ ] [P1] Expanded Idle Page Rendering: Render high-aesthetic Time, Day, and Date typography on Page 1
- [ ] [P1] Expanded Weather Page Rendering: Render weather temperature, conditions icon, and forecast snippet
- [ ] [P1] Page Transition Animation: Implement smooth horizontal slide or fade transition between expanded pages
- [ ] [P2] Button hover, pressed, and active feedback states for media and timer controls
- [ ] [P2] High-DPI scaling validation across 100%, 125%, 150%, and 200% display scaling factors

### 2. Interaction, Hit-Testing and Input Handling
- [x] [P0] Event Preemption & Interruption: Superseding events immediately replace active HUD without queue delay
- [x] [P0] Overhaul hit-test coordinate mapping (use settled target layout metrics instead of interpolating width)
- [x] [P0] Implement robust WM_LBUTTONDOWN / WM_LBUTTONUP state machine with SetCapture / ReleaseCapture
- [x] [P0] Fix premature collapse: clicking content body or dragging must not dismiss the island
- [x] [P0] Top Taskbar Island Hit-Test & Clickability: Resolve click event interception when island is docked on top taskbar
- [x] [P1] Left-Click Interaction Refinement: Fix premature auto-collapse bounce after clicking to expand; implement press-and-hold interaction support
- [x] [P1] Support compact island click-to-expand across all valid event states
- [ ] [P1] Enable compact island click-to-expand during idle state (EventType::None)
- [ ] [P1] Mouse wheel scrolling support (`WM_MOUSEWHEEL`) in Collapsed state to cycle compact modules
- [ ] [P1] Mouse wheel scrolling support (`WM_MOUSEWHEEL`) in Expanded state to cycle full widget pages
- [x] [P1] Global test hotkeys (Ctrl + Win + 1..9, 0) for manual HUD and scenario simulation
- [x] [P1] Remove test right-click scenario cycling (deprecated prototype leftover in WM_RBUTTONUP)

### 3. Real System Event Integrations (No Placeholders)
- [x] [P1] Media (GSMTC / WinRT):
  - [x] Real-time session monitoring via GlobalSystemMediaTransportControlsSessionManager
  - [x] Track title, artist, and playback status extraction
  - [x] Real transport controls (Play, Pause, Skip Next, Skip Previous)
  - [x] Stream album art thumbnail retrieval via IRandomAccessStreamReference
  - [x] Smooth marquee / running text for overflowing title/artist
  - [x] Adaptive taskbar line layout
  - [-] Media timeline progress bar (removed due to platform desync bugs)
- [x] [P1] Album Art 1:1 Square Cropping: Aspect ratio calculation and center-crop squircle rendering
- [/] [P1] Waveform Visualizer & Audio Loopback Engine:
  - [x] WASAPI audio loopback capture on default endpoint
  - [x] Power efficiency gating: capture only runs when media is playing and UI is visible
  - [ ] Stall watchdog: auto-reconnect WASAPI client if media reports playing but stream receives no data > 5s
  - [x] RMS signal calculation and attack/release envelope filter
  - [ ] Enhance visualizer dynamics and responsiveness
- [x] [P1] Microphone Privacy Event: Transition detection via Registry / CoreAudio capture streams with orange dot
- [x] [P1] Keyboard: Real Caps Lock state toggle monitoring via GetKeyState polling and HUD badge
- [ ] [P2] Keyboard: Num Lock state toggle HUD support
- [x] [P1] Power / Battery: Real AC plug/unplug and low battery (<20%) alert HUD via GetSystemPowerStatus polling
- [ ] [P3] Power / Battery: Event-driven refactor using RegisterPowerSettingNotification
- [ ] [P1] Audio / Volume: Real MMDevice endpoint volume monitoring via IAudioEndpointVolumeCallback
- [ ] [P2] Bluetooth: Battery and connection state parsing for connected peripherals
- [ ] [P2] Weather Service: Fetch local temperature via Windows Location API or manual query

### 4. Shell Geometry and Taskbar Adaptation
- [x] [P1] Handle WM_SETTINGCHANGE, WM_DISPLAYCHANGE, WM_DPICHANGED, and TaskbarCreated in IslandWindow
- [x] [P1] Real-time taskbar edge and size change watcher
- [ ] [P2] Multi-monitor taskbar shift and primary display transition detection
- [ ] [P2] Auto-hide taskbar detection (ABM_GETTASKBARPOS) and tuck-away behavior

### 5. Visibility Policies and Configuration
- [x] [P1] Implement idleVisibilityMode setting (always_visible vs events_only)
- [x] [P1] Implement mediaVisibilityPolicy setting (always_visible vs track_change_only)
- [x] [P1] Implement auto-collapse delay setting (`autoCollapseDelayMs` / `autoCollapseSeconds`) and pause-on-hover logic
- [x] [P1] Synchronize src/metadata/mod_settings.h schema with runtime LoadSettings() in main.cpp
- [ ] [P2] Add temperature unit setting (`celsius` vs `fahrenheit`)
- [ ] [P2] Add clock display format setting (`24h` vs `12h`)
- [ ] [P2] Add weather location configuration settings (`auto` vs `manual` city/coordinates)

### 6. Stability, Performance and Verification
- [x] [P0] Establish Windhawk compilation guide and automated dual-target verification (scripts/verify.py, docs/COMPILER_GUIDE.md)
- [x] [P1] Verify bundle integrity with python scripts/bundle.py and python scripts/verify.py
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
- [x] Implement auto-collapse delay timer with pause-on-hover logic
- [x] Implement real Caps Lock state change HUD
- [x] Implement real AC power connect/disconnect and low battery alerts
- [x] Top taskbar z-order and clickability fix
- [x] Startup media placeholder clean boot fix
- [x] Asynchronous album art streaming and PBGRA decoding
- [x] Continuous infinite-loop marquee with QPC delta timing
- [x] Void entry spring physics for events-only mode
- [x] Same-size HUD content crossfade transition
