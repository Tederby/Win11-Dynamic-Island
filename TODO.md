# Project TODO and Roadmap - Win11 Dynamic Island

Central task board and backlog for the Win11-Dynamic-Island Windhawk mod.
Agents and developers must inspect this document during startup and keep task statuses updated.

---

## Priority and Status Legend

- [P0] Critical / Blocker: Must be addressed immediately (crashes, visual corruption, broken hooks).
- [P1] High Priority: Core feature, real-time adaptation, or parity requirement from prototype.html / ARCHITECTURE.md.
- [P2] Normal Priority: Visual polish, secondary interactions, or UX enhancements.
- [P3] Low / Nice-to-have: Optional tweaks, edge cases, and future explorations.
- Status markers:
  - [ ] Not Started
  - [/] In Progress
  - [x] Completed
  - [-] Cancelled / Deferred

---

## Current Focus (In Progress)

- [x] [P0] Fix squircle alpha rendering (eliminate boxy and rectangular halo artifacts around rounded corners)
- [x] [P0] Fix text overflow and wrapping (set DWRITE_WORD_WRAPPING_NO_WRAP and ellipsis trimming)
- [x] [P1] Add manual test triggers (global hotkeys and click demo cycle for all HUD events)
- [x] [P1] Implement real-time taskbar movement and resize adaptation (WM_SETTINGCHANGE, WM_DISPLAYCHANGE, and geometry polling)
- [x] [P1] Implement Island visibility modes (Always Visible vs Events Only) and Media visibility policy (Always Visible vs Track Change Only)

---

## Task Backlog

### 1. UI, Direct2D Rendering and Animations
- [x] [P0] Replace colorkey transparency or implement rounded region clipping / per-pixel alpha to eliminate rectangular boundary artifacts
- [x] [P0] DirectWrite trimming: prevent vertical text wrapping with ellipsis (...) truncation
- [ ] [P1] Decouple window resizing from animation ticks to eliminate DWM stutter during spring transitions
- [ ] [P1] Reconcile vector element spacing with prototype.html (artwork, waveform, progress bar)
- [ ] [P2] High-DPI scaling validation across 100%, 125%, 150%, and 200% display scaling factors
- [ ] [P2] Premultiplied alpha and anti-aliasing checks for rounded pill borders and subtle glow

### 2. Testability and Debug Triggers
- [x] [P1] Implement global debug hotkeys (e.g. Ctrl + Win + 1..7) to simulate Volume, Caps Lock, Power, Bluetooth, and Media events
- [x] [P1] Implement right-click or context menu demo scenario cycle for immediate visual verification without external hardware triggers
- [ ] [P2] Add Windhawk debug toggle setting to simulate all HUD events sequentially

### 3. Shell Geometry and Real-Time Taskbar Adaptation
- [x] [P1] Handle WM_SETTINGCHANGE (SPI_SETWORKAREA), WM_DISPLAYCHANGE, and TaskbarCreated in IslandWindow
- [x] [P1] Implement real-time taskbar edge and size change watcher (auto-reposition dynamically without mod restart)
- [ ] [P2] Add fallback notification toast if layout change occurs or if explorer restart is detected
- [ ] [P2] Multi-monitor taskbar shift and primary display transition detection
- [ ] [P2] Auto-hide taskbar detection via SHAppBarMessage (ABM_GETTASKBARPOS) and tuck-away behavior

### 4. Visibility Policies and Configuration
- [x] [P1] Implement idleVisibilityMode setting:
  - always_visible: Compact pill remains visible on taskbar during idle state
  - events_only: Island remains hidden (0-opacity) when idle, only appearing during live activities or transient events
- [x] [P1] Implement mediaVisibilityPolicy setting:
  - always_visible: Island stays visible while media is actively playing, regardless of global idle visibility setting
  - track_change_only: Island pops up transiently for 3-4 seconds when track changes or playback starts, then collapses back to hidden
- [x] [P1] Synchronize src/metadata/mod_settings.h schema with runtime LoadSettings() in main.cpp
- [x] [P2] Test dynamic settings reload (Wh_ModSettingsChanged) without requiring mod restart
- [ ] [P2] Auto-collapse timer setting (autoCollapseSeconds) and pause-on-hover logic

### 5. Background Services and Windows Integration
- [ ] [P1] Media (GSMTC): Track metadata parsing, thumbnail caching, and playback controls hookup
- [ ] [P1] Audio: Volume change monitoring via IAudioEndpointVolumeCallback and mute status
- [ ] [P2] Power: AC adapter plug/unplug event handling and battery charge percentage polling
- [ ] [P2] Bluetooth: Peripheral battery status and device connection notification parsing
- [ ] [P2] Keyboard: Low-level hook or polling for Caps Lock / Num Lock state toggles
- [ ] [P2] Microphone: Active audio capture session detection for privacy indicator dot

### 6. Stability, Performance and Quality
- [ ] [P1] Verify bundle integrity with python scripts/bundle.py
- [ ] [P2] Memory footprint check (ensure Direct2D resources and WinRT objects are released properly on shutdown)
- [ ] [P2] Prevent UI thread stutter or frame drops during rapid system notifications

## Completed Tasks

- [x] Initial modular C++ structure setup with automated bundler script (scripts/bundle.py)
- [x] Implement interactive HTML/CSS reference prototype (prototype.html)
- [x] Base Direct2D layered window and animation spring physics implementation
- [x] Project task tracking setup and agent workflow integration (TODO.md)
- [x] Eliminate rectangular halo artifacts and colorkey fringe via dynamic squircle region clipping and layered alpha
- [x] Prevent vertical text wrapping and line overflow with DirectWrite no-wrap and ellipsis trimming
- [x] Implement global debug hotkeys (Ctrl+Win+1..9, 0) and enhanced demo scenario cycling
- [x] Implement real-time taskbar movement and display adaptation (WM_SETTINGCHANGE, WM_DISPLAYCHANGE, WM_DPICHANGED, TaskbarCreated, and geometry polling)
- [x] Implement idleVisibilityMode and mediaVisibilityPolicy configuration settings with runtime schema synchronization
