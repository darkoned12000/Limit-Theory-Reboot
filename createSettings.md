// Copyright (C) 2025 darkoned12000
// SPDX-License-Identifier: GPL-3.0-or-later
// Part of the ltheory-old-test modernization effort (Revamp Work).
// See NOTICE and LICENSE.GPL. Original engine (c) Josh Parnell, public domain.

# Settings System Creation Plan

## 1. Goal

Create a fully functional settings system for Limit Theory that:
- Is accessible from every app via the existing ESC → GameMenu workflow
- Provides video, audio, and control configuration screens
- Uses Limit Theory's visual style/theming (dark panels, cyan accents, compact layout)
- Persists values between sessions to `settings.bin` or a LTSL-accessible config file
- Prepares infrastructure for Phase 6 visual effects controls

---

## 2. Current State Assessment

### What Exists (Broken/Incomplete)

| File | Status | Notes |
|------|--------|-------|
| `Widget/GameMenu.lts` | Partially working | Shows on ESC, has "SETTINGS" button but `onPress = 0` — does nothing. Only "EXIT GAME" works. |
| `Widget/Settings.lts` | Broken | Calls C++ `Widget_Settings` which recurses the settings node tree, but every `GetWidget()` returns `nullptr`. Renders an empty panel. |
| `Module/Settings.cpp/.h` | Incomplete | Defines entry types (Bool, Float, Axis, Color, Button), saves to `settings.bin`, but never exposes interactive widgets or LTSL bindings. |
| `DevPanel.lts` | Read-only | Shows FPS, poly count, tasks list; no editable controls. Not wired to any hotkey. |

### What's Missing from C++ ScriptAPI

Window and audio functions exist in the engine code but are **not bound** to LTSL:
- No `Window_GetFullscreen()`, `Window_SetFullscreen(bool)`
- No `Window_GetSize()`, `Window_SetSize(V2U)` or resolution enumeration
- No `Window_GetVSync()`, `Window_SetVSync(bool)`
- No master volume getter/setter (`Sound_SetVolume` exists per-sound only)
- No draw state / renderer control functions

---

## 3. Desired User Experience

### Workflow (ESC → Settings)

```
Player presses ESC in any app:
    ↓
GameMenu popup appears (existing widget):
┌─────────────────────┐
│   LIMIT THEORY      │
│     v0.82.2         │
│                     │
│  [ SAVE GAME ]      │ ← placeholder
│  [ SETTINGS ]       │ ← NOW WORKS — opens SettingsPanel
│  [ HELP ]           │ ← placeholder
│  [ EXIT TO MENU ]   │ ← placeholder (reload launcher app)
│  [ EXIT GAME ]      │ ← already works
└─────────────────────┘
    ↓ player clicks SETTINGS
SettingsPanel appears as overlay:
┌──────────────────────────────────┐
│ Settings                    [X]  │
├──────────────────────────────────┤
│ Tabs: VIDEO | AUDIO | CONTROLS   │
├──────────────────────────────────┤
│ (tab contents — see §4 below)    │
└──────────────────────────────────┘
```

### Persistence Model

Settings are saved to `settings.bin` in the user data directory (`OS_GetUserDataPath()`) via the existing C++ Settings system. On startup, all apps call a single LTSL function that loads defaults from this file and applies them immediately:
- Video settings → applied once at app initialization (window mode) or live (VSync toggle)
- Audio settings → applied live when changed

---

## 4. Settings Panel Layout & Content

### Theme Guidelines (Limit Theory Style)

The panel must match the existing LT aesthetic observed in DevPanel, HUD, and other widgets:
- **Background:** Dark semi-transparent panels (`0x1a1e23` or `Color(0.075f, 0.95f)` backdrop style used by Settings.lts)
- **Accents:** Cyan/teal highlights for active elements (DevPanel uses cyan task dots; HUD uses similar tones)
- **Typography:** Compact sans-serif labels, small text density, left-aligned
- **Layout:** Tabbed columns with clear section headers; sliders and toggles use existing widget primitives

### Video Tab

```
┌──────────── VIDEO ────────────┐
│                                │
│  Display Mode                  │
│    [ Fullscreen Toggle ]       │ ← Boolean toggle button
│                                │
│  Resolution                    │
│    Width:  [1920] Height: [1080]   ← two integer text fields or sliders
│                                │
│  VSync                         │
│    [ Enabled Toggle ]          │ ← Boolean toggle button
│                                │
│  (Phase 6 prep — future)       │
│  Bloom Intensity: [slider]     │
│  Chromatic Aberration: [toggle]│
│  Motion Blur Strength:[slider] │
└───────────────────────────────┘
```

### Audio Tab

```
┌──────────── AUDIO ────────────┐
│                                │
│  Master Volume                 │
│    [████████░░] (80%)          │ ← slider widget, float 0.0–1.0
│                                │
│  SFX Volume                    │
│    [██████████] (100%)         │ ← placeholder for future per-category mixers
│                                │
│  Music Volume                  │
│    [█████░░░░░] (50%)          │ ← placeholder for future per-category mixers
└───────────────────────────────┘
```

### Controls Tab (Phase 1 — Read-Only Display)

The current engine does not expose key bindings to LTSL. This tab starts as a read-only display of default controls and becomes editable in a later phase:

```
┌──────────── CONTROLS ─────────┐
│                                │
│  Movement                      │
│    Thrust Forward     W        │
│    Thrust Backward    S        │
│    Strafe Left        A        │
│    Strafe Right       D        │
│    Roll Left          Q        │
│    Roll Right         E        │
│                                │
│  Camera                        │
│    Look Around      Mouse      │
│    Zoom In          Scroll+    │
│    Zoom Out         Scroll-    │
│                                │
│  Misc                          │
│    Pause Game       ESC        │
│    Dev Panel        F2         │
│    Debug Scene      F3         │
└───────────────────────────────┘
```

---

## 5. Implementation Plan (Phased)

### Phase A — C++ ScriptAPI Bindings (Required Foundation)

Add `DefineFunction` bindings in the appropriate module (`Module/Window.cpp`, `Module/SoundEngine.cpp`) so LTSL can control window/audio settings:

| Function | Signature | Purpose | File to Modify |
|----------|-----------|---------|----------------|
| `windowGetFullscreen` | `→ Bool` | Query current fullscreen mode | Module/Window.cpp or new SettingsScriptAPI.cpp |
| `windowSetFullscreen` | `(Bool) → Void` | Toggle fullscreen/windowed | Module/Window.cpp |
| `windowGetSize` | `→ V2U` | Get current resolution | Module/Window.cpp |
| `windowSetSize` | `(Int width, Int height) → Void` | Set resolution (or `V2U`) | Module/Window.cpp |
| `windowGetVSync` | `→ Bool` | Query current vsync state | Module/Window.cpp |
| `windowSetVSync` | `(Bool) → Void` | Enable/disable vsync | Module/Window.cpp |
| `soundGetMasterVolume` | `→ Float` | Get master volume (0.0–1.0) | Module/SoundEngine.cpp or new AudioSettingsScriptAPI.cpp |
| `soundSetMasterVolume` | `(Float) → Void` | Set master volume; propagate to all active sounds | Same as above |

**Technical notes:**
- Master volume: Currently the FMOD backend uses per-sound volumes. Add a global float member (`masterVolume`) to `SoundEngineT`. Each `Play*()` call multiplies its input volume by this factor. On `SetMasterVolume()`, iterate all active sounds and rescale their current volume (or use FMOD's system-level master if available).
- Resolution: SFML 3 provides `sf::VideoMode::getDesktopMode()` for current desktop resolution and `sf::VideoMode::getFullscreenModes()` for the list. For Phase A, manual width/height entry is sufficient; a dropdown can be added later.

### Phase B — Settings Persistence Layer (LTSL-Side)

Create or fix a mechanism so LTSL can read/write settings that persist between sessions:

**Option 1 (Preferred):** Fix existing C++ `Settings` system
- Currently saves to `settings.bin` but never exposes widgets/bindings
- Add C++ bindings: `settingsGetBool(name, default)`, `settingsSetBool(name, value)`, same for Float/Int/String
- This is the cleanest path and matches how Josh intended it

**Option 2 (Fallback):** LTSL-side JSON/text file parser
- Create a new config format (`resource/script/settings.json`) that LTSL parses at startup
- Slower to implement; requires writing a JSON parser in LTSL or adding C++ binding for one

### Phase C — SettingsPanel Widget (LTSL)

Rewrite `Widget/Settings.lts` as a proper interactive panel. Use existing widget infrastructure:

**Dependencies (already exist):**
- `Window`, `Tab`, `ListV`, `ListH`, `Button`, `ToggleButton`, `Slider`, `TextField`, `Text`, `Spacer`, `Components.lts` helpers (`Backdrop`, `Padding`, `Align`, etc.)

**Structure:**
```lts
type SettingsPanel
  Interface parentUI
  
  Bool fullscreen      // cached state
  Int resWidth, resHeight
  Bool vsyncEnabled
  Float masterVolume
  
  function Void Initialize ()
    fullscreen = Window_GetFullscreen()
    var sz (Window_GetSize())
    resWidth = sz.x; resHeight = sz.y
    vsyncEnabled = Window_GetVSync()
    masterVolume = Sound_GetMasterVolume()

  function List CreateChildren ()
    // Tab-based layout with VIDEO | AUDIO | CONTROLS tabs
    // Each tab's children build from cached state fields
    ...

  function Void ApplyVideoSettings (Bool commitNow)
    Window_SetVSync(vsyncEnabled)
    if (commitNow && fullscreen != Window_GetFullscreen()) {
      Window_SetSize(resWidth, resHeight)
      Window_SetFullscreen(fullscreen)
    }

function Widget Create ()
  Window:Create "Settings"
    SettingsPanel
```

**Key design decisions:**
- All state lives in the widget's type fields — read from C++ API on open, written back on change or close.
- Sliders and toggles update their corresponding field immediately; video changes are deferred until user clicks a "Apply" button (prevents resolution flicker).
- Tab switching is handled by existing `Tab` widget infrastructure.

### Phase D — Wire GameMenu → SettingsPanel

Modify `Widget/GameMenu.lts`:
1. Change the SETTINGS button's `onPress` from `0` to send a custom message (`MessageClick "openSettings"`)
2. Add a handler in whatever app/widget owns the GameMenu that:
   - Creates an instance of `Custom Widget SettingsPanel` and adds it as a child window to the game UI interface
   - Or replaces the GameMenu with the SettingsPanel temporarily

Alternative approach: Settings button opens a second-level popup from within GameMenu itself (nested windows). Simpler but may have focus/z-order issues.

### Phase E — Integrate into Apps

Every app's `Update()` loop or escape handler needs to be able to show/hide the settings panel via ESC. Two approaches:

**Approach 1:** Each app calls a shared LTSL function that shows GameMenu + Settings
- Create `Widget/GlobalMenu.lts` as a reusable overlay component
- Apps call it when ESC is pressed; it handles its own dismissal

**Approach 2:** App lifecycle hook (preferred long-term)
- Add a virtual method or convention: apps check for ESC, if handled return early from Update() with GameMenu/Settings displayed in front of everything else

For Phase A/D/E integration into `ltheory-main.lts` specifically:
1. In `Update()`, detect ESC keypress via existing input handling
2. Add/remove a GameMenu instance (which now has working Settings) as an overlay child of the UI interface

---

## 6. Phase 6 Preparation — Visual Effects Controls

Once the settings infrastructure is in place, Phase 6 visual effects can plug into it without rework:

**Required additions for Phase 6:**
1. New C++ bindings:
   - `drawStatePush(name, value)` — already exists; confirm LTSL binding works with shader uniform names
   - Or new functions: `shaderSetFloat(shaderName, uniformName, float)`, etc., if DrawState_Push is too global

2. SettingsPanel "Visuals" tab (added alongside Phase 6 implementation):
   - Bloom intensity slider → sets a draw state or render pass parameter
   - Chromatic aberration toggle/strength slider
   - Motion blur strength slider / toggle
   - Tonemapping mode selector (linear, Reinhard, ACES)

3. Render pipeline must support dynamic rebuild:
   - When user toggles an effect on/off, the app reconstructs its render pass vector and re-adds it to the interface in-place

---

## 7. Files to Create/Modify Summary

| File | Action | Purpose |
|------|--------|---------|
| `Module/Window.cpp` or new `SettingsScriptAPI.cpp` | Modify/Add | C++ bindings for window functions |
| `Module/SoundEngine.cpp` or new `AudioSettingsScriptAPI.cpp` | Modify/Add | Master volume getter/setter + global mixer tracking |
| `Module/Settings.cpp/.h` | Modify | Fix `GetWidget()` return values; expose settings to LTSL via new bindings (Option 1) |
| `resource/script/Widget/GameMenu.lts` | Modify | Wire SETTINGS button → opens SettingsPanel |
| `resource/script/Widget/Settings.lts` | Rewrite | Full interactive panel with VIDEO/AUDIO/CONTROLS tabs |
| `resource/script/App/ltheory-main.lts` | Modify (example) | Show GameMenu on ESC; wire settings into lifecycle |

---

## 8. Risks & Known Issues

- **Resolution change on Linux/XWayland:** SFML may not support hot-swapping resolution without recreating the window. May need to restart the app or defer fullscreen changes until "Apply" is clicked (with a warning).
- **Master volume propagation:** Existing sounds created before `SetMasterVolume()` won't automatically adjust unless we iterate all active FMOD channels and rescale their volumes. Consider using FMOD's system-level master if available.
- **Settings persistence race condition:** If multiple apps are run in the same session (unlikely but possible), settings.bin is overwritten on shutdown; ensure only one app instance writes it at exit.
