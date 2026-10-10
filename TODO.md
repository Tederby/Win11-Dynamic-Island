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

## Current Focus (Interactive Polish & User Feedback Backlog)

- [x] [P1] Startup Media State & Placeholder Fix: Clean up uninitialized media defaults; prevent `MediaService` from forcing active playback ("Senbonzakura") on boot so island cleanly displays the digital clock in `always_visible` mode or stays hidden in `events_only` mode.
- [x] [P0] Top Taskbar Island Hit-Test & Clickability: Resolve click event interception when island is docked on top taskbar (island is not clickable on top taskbar due to AppBar / Shell_TrayWnd z-order conflict, while bottom taskbar works properly).
- [x] [P1] Media Album Art Thumbnail Streaming (Non-Blocking / Asynchronous):
  - [x] Stream album art thumbnail retrieval via WinRT `IRandomAccessStreamReference` -> WIC bitmap pipeline.
  - [x] CRITICAL: Retrieve thumbnail bytes on a dedicated background worker / asynchronous task to NEVER block the UI thread (preventing COM STA `.get()` deadlocks and freezing).
  - [x] Decode in-memory via `IWICImagingFactory` -> `IWICStream` -> `ID2D1Bitmap` (PBGRA).
  - [x] Render with distortion-free 1:1 center cover-crop (`object-fit: cover`) via `ID2D1BitmapBrush` in compact (18x18) and expanded (52x52) states, falling back gracefully to dynamic gradient.
- [x] [P1] Idle State Visual Polish & Centering:
  - [x] Digital clock (`HH:mm`) centering: fix vertical and horizontal alignment offsets so text sits dead-center within the idle pill.
  - [x] Idle aesthetics: enrich minimal idle pill with sleek typography and subtle status indicators (e.g. charging accent, audio activity micro-dot, or subtle seconds pulse).
- [x] [P1] Void Entry Animation (`events_only` mode):
  - [x] Eliminate abrupt popping: when the island awakens from hidden state (e.g., track starts, Caps Lock toggles, charger plugged), animate entry smoothly using cubic-bezier spring physics from zero scale / opacity (`scale: 0.35 -> 1.0`, `opacity: 0 -> 1.0`).
- [x] [P1] Same-Size Event Transition & Content Crossfade / Morphing:
  - [x] When switching between two events that share identical or similar dimensions (e.g. CapsLock to Volume, or successive transient HUDs), implement smooth content crossfade / alpha blending instead of snapping directly to new text/icons.
  - [x] Add subtle physical micro-pulse/punch (`scale: 1.0 -> 1.035 -> 1.0`) during same-size event changes to provide tactile visual feedback.
- [x] [P1] Continuous Looping Running Text (Marquee) & Acceleration Glitch Fix:
  - [x] Continuous Wrapping Loop: Replace ping-pong / hard reset with seamless infinite scrolling (duplicate title/artist string with separator bullet so the tail seamlessly loops back into view).
  - [x] Acceleration Fix: Drive marquee scrolling strictly by high-precision QPC delta elapsed time (`deltaTimeSeconds * SPEED_PX_PER_SEC`) instead of timer tick counts to eliminate the speed-up glitch when concurrent background events fire.
  - [x] Smooth 60 FPS Framerate & Readable Speed: Increased marquee refresh timer to 16ms (~60 FPS) and tuned scrolling speed to 16 px/s with hardware DirectWrite layout caching to eliminate stutter and choppiness.
- [x] [P1] Remove Test Right-Click Interaction:
  - [x] Remove legacy right-click scenario cycling handler (`WM_RBUTTONUP` / `OnRightClick()`) left over from early prototype testing. Retain global hotkeys (`Ctrl+Win+1..9, 0`).
- [x] [P1] Left-Click State Machine Refinement:
  - [x] Fix premature auto-collapse bouncing: clicking to expand should reliably lock the island open without immediately bouncing back to compact state (protected `IslandState::Expanded` against periodic `ServiceManager` polling overrides).

---

## Task Backlog

### 1. UI, Direct2D Rendering and Smooth Animations
- [x] [P0] True Per-Pixel Alpha Composition: Eliminate opaque black rectangular backdrop (~367x166px) while maintaining hardware anti-aliasing on squircle edges
- [x] [P0] Pure Pitch-Black Island Styling: Island background strictly `#000000` with 0 outline stroke
- [x] [P1] High-precision animation timer: QPC delta timing and `timeBeginPeriod(1)` supporting 60Hz, 120Hz, 144Hz, and 240Hz displays
- [x] [P1] Always-visible idle content: Minimal digital clock (HH:mm)
- [x] [P1] Idle Pill Visual Enhancement: Precisely center digital clock typography vertically and horizontally; enrich idle pill with sleek aesthetics and subtle status indicator dots
- [x] [P1] Void Entry Animation (`events_only` mode): Organic spring scale-up and opacity fade-in transition when island awakens from hidden state instead of abruptly popping in
- [x] [P1] Same-Size Event Transition & Crossfade: Implement smooth content crossfade/morph when transitioning between events of identical size (e.g. CapsLock to Volume) so content doesn't snap abruptly
- [x] [P1] Continuous Looping Marquee: Convert title/artist marquee to seamless continuous wrapping loop and fix acceleration glitch caused by concurrent background event ticks
- [x] [P0] DirectWrite trimming: prevent vertical text wrapping with ellipsis (...) truncation
- [x] [P0] Decouple HWND resizing and ID2D1HwndRenderTarget::Resize from 16ms animation ticks
- [x] [P1] Symmetrical vertical waveform bars: render via FillRoundedRectangle expanding from centerY
- [x] [P1] Waveform paused state: render clean static dots via FillEllipse instead of flat lines
- [ ] [P1] Reconcile vector element spacing and control positions with settled layouts
- [ ] [P2] Button hover, pressed, and active feedback states for media and timer controls
- [ ] [P2] High-DPI scaling validation across 100%, 125%, 150%, and 200% display scaling factors

### 2. Interaction, Hit-Testing and Input Handling
- [x] [P0] Event Preemption & Interruption: Superseding events (e.g. Caps Lock On -> Off, Volume changes) immediately replace active HUD without queue delay
- [x] [P0] Overhaul hit-test coordinate mapping (use settled target layout metrics instead of interpolating width)
- [x] [P0] Implement robust WM_LBUTTONDOWN / WM_LBUTTONUP state machine with SetCapture / ReleaseCapture
- [x] [P0] Fix premature collapse: clicking content body or dragging must not dismiss the island
- [x] [P0] Top Taskbar Island Hit-Test & Clickability: Resolve click event interception when island is docked on top taskbar (resolve AppBar / window z-order conflict while bottom taskbar works)
- [x] [P1] Left-Click Interaction Refinement: Fix premature auto-collapse bounce after clicking to expand; implement press-and-hold (long-press) interaction support
- [x] [P1] Support compact island click-to-expand across all valid event states
- [x] [P1] Global test hotkeys (Ctrl + Win + 1..9, 0) for manual HUD and scenario simulation
- [x] [P1] Remove test right-click scenario cycling (deprecated prototype leftover in WM_RBUTTONUP)

### 3. Real System Event Integrations (No Placeholders)
- [x] [P1] Media (GSMTC / WinRT):
  - [x] Real-time session monitoring via GlobalSystemMediaTransportControlsSessionManager
  - [x] Track title, artist, and playback status extraction
  - [x] Real transport controls (Play, Pause, Skip Next, Skip Previous)
  - [x] Stream album art thumbnail retrieval via IRandomAccessStreamReference
  - [x] Smooth marquee / running text for overflowing title/artist
  - [x] Adaptive taskbar line layout (1-line on compact taskbar, 2-line on normal taskbar)
- [x] [P1] Album Art 1:1 Square Cropping:
  - [x] Aspect ratio calculation and center-crop algorithm for rectangular art
  - [x] Distortion-free squircle rendering in compact (18x18) and expanded (48x48) states
- [/] [P1] Waveform Visualizer & Audio Loopback Engine:
  - [x] WASAPI audio loopback capture (AUDCLNT_STREAMFLAGS_LOOPBACK on default eRender/eConsole endpoint)
  - [x] Power efficiency gating: capture only runs when media is playing (playing == true) and media UI is visible
  - [ ] Stall watchdog: auto-reconnect WASAPI client if media reports playing but stream receives no data > 5s
  - [x] RMS signal calculation: per-chunk (64 frames) RMS = sqrt(sum(v^2)/N) * 4.0f, clamped to [0.0, 1.0]
  - [x] Attack / Release envelope filter: fast attack 0.7 (punchy beats) and soft release 0.85 (smooth decay)
  - [ ] Enhance visualizer dynamics and responsiveness
- [x] [P1] Microphone Privacy Event:
  - [x] Transition detection (app starts using microphone) via Registry / CoreAudio capture streams
  - [x] Display as transient pop-up HUD (2.8s timeout) with app name and orange privacy dot
  - [x] Automatic restoration of underlying media or idle state upon expiry (never locked in always-visible)
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
