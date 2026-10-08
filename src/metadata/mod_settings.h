// ==WindhawkModSettings==
/*
# Dynamic Island Settings Schema
- placementMode: auto
  $name: Placement Mode
  $description: Choose how the Dynamic Island positions itself.
  $options:
    - auto: Automatically detect taskbar position (embedded on top, floating on bottom/sides)
    - top_embed: Always embed directly into top edge
    - floating: Always float as an overlay window
- idleVisibilityMode: always_visible
  $name: Idle Visibility Mode
  $description: Choose island appearance when no transient HUD or live activity is active.
  $options:
    - always_visible: Compact pill remains visible on taskbar during idle state
    - events_only: Island remains hidden when idle, appearing only during activities or alerts
- mediaVisibilityPolicy: always_visible
  $name: Media Playback Visibility Policy
  $description: Determine how media playback displays on the island.
  $options:
    - always_visible: Island stays visible while media is actively playing
    - track_change_only: Island pops up transiently for 3-4 seconds on track change, then collapses back to hidden
- enableMedia: true
  $name: Media Controls & Now Playing
  $description: Show active media playback, track marquee, equalizer, and playback controls.
- enableTimer: true
  $name: Focus Timer
  $description: Enable focus session countdown timer with progress ring.
- enableMicStatus: true
  $name: Microphone Privacy Dot
  $description: Show indicator when an application is accessing the microphone.
- enableVolumeHUD: true
  $name: Volume Changes HUD
  $description: Show compact island badge when master volume changes.
- enableCapsLockHUD: true
  $name: Caps Lock HUD
  $description: Show badge when Caps Lock is toggled ON or OFF.
- enablePowerHUD: true
  $name: Power & Battery Alerts
  $description: Show alerts when charger is plugged/unplugged or battery drops below 20%.
- enableBluetoothHUD: true
  $name: Bluetooth HUD
  $description: Show badge when Bluetooth headphones/devices connect with battery status.
- autoCollapseSeconds: 5
  $name: Auto-collapse Delay
  $description: Seconds of inactivity before an expanded island collapses back to compact view.
- animationSpeed: 1.0
  $name: Animation Speed Multiplier
  $description: Adjust speed of spring transition animations (1.0 = default fluid physics).
- enableDebugHotkeys: true
  $name: Enable Debug Hotkeys
  $description: Enable global hotkeys (Ctrl+Win+1..9, 0) to simulate volume, battery, media, and other HUD events.
*/
// ==/WindhawkModSettings==
