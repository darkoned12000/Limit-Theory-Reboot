# LTSL Documentation (Limit Theory Scripting Language)

> A practical, example-driven reference for **LTSL** — the homegrown scripting
> language that drives the Limit Theory engine. LTSL has no upstream docs, so
> this file is reverse-engineered from the working `resource/script/**/*.lts`
> sources (mostly `war.lts`, `loading.lts`, `Widget/*`, `Fonts.lts`).
>
> **Convention used in this doc:** every example is taken (or lightly adapted)
> from a real script. File paths are given so you can read the originals.
> Where the engine is picky, the gotcha is called out explicitly.
>
> **Want to understand LTSL internals?** See [LTSL-ARCHITECTURE-AND-IMPROVEMENTS.md](../LTSL-ARCHITECTURE-AND-IMPROVEMENTS.md) 
> for a deep-dive into the tree-walking interpreter, the 26 expression types, 
> compilation pipeline, and how to make LTSL more user-friendly.

---

## 0. The 30-second mental model

An LTSL "app" is a single `.lts` file in `resource/script/App/`. The launcher
(`src/launch/launch.cpp`) looks for a `Main` function and runs it.

There are **two distinct app shapes**, and mixing them up is the #1 source of
"my app shows nothing / crashes":

1. **Driven app** (works): `Main` returns an `App` *value*, and the app defines
   `Initialize()` and `Update()`. The engine calls `Update()` every frame and
   you manually drive your interfaces. See `war.lts` / `ltheory-main.lts`.
 2. **Self-widget app** (deprecated / not driven): `Main` returns a `Data` and
    calls `widget:Create Layer ...`. These are *not* ticked by the engine the
    same way and many of them currently render nothing or crash
    (`loading.lts`, `ui.lts`, `platemesh.lts`, `strukt.lts`).
    **Do not start new UI work from these.** Use the driven pattern instead.

    > NOTE: `launcher.lts` used to be a broken self-widget app, but it has since
    > been converted to a working **driven** app (see §14). It is a good minimal
    > reference for a pure-2D UI driven app.

The minimal driven app:

```lts
# resource/script/App/hello.lts
type App
  Interface ui

  function Void Initialize ()
    ui = (Interface_Create "UI")

  function Void Update ()
    ui.Update
    ui.Draw

function App Main ()
  var self App
  self
```

Run it with: `python3 configure.py run hello`

---

## 1. App lifecycle (`Main` / `Initialize` / `Update`)

```lts
type App
  Interface ui
  Interface gameView
  Camera camera
  Player player
  Object root          # any extra members go here (see §7 limit)

  function Void Initialize ()
    # called ONCE before the first Update. Build your world here.
    camera   = Camera_Create
    camera.Push                       # make this camera active
    ui       = (Interface_Create "UI")
    gameView = (Interface_Create "Game View")

  function Void Update ()
    # called EVERY frame. Drive interfaces, step simulation, draw.
    var dt (Min 0.1 FrameTimer_Get)   # clamp dt so big stalls don't explode
    root.Update dt
    camera.SetTarget player.GetPiloting
    ui.Update
    gameView.Update
    gameView.Draw                     # gameView is what actually gets drawn

function App Main ()
  var self App
  self
```

Key facts:

- `Main` **must return an `App` value**. The engine stores it and calls
  `Initialize()` then `Update()` on it. (If `Update` is missing the engine sets
  `deleted = true` and the app exits immediately — `src/launch/launch.cpp`.)
- `gameView.Draw` is the real draw call. `ui` is usually *not* drawn directly;
  it is composited into the game view through `RenderPass_Interface ui` (§3).
- `FrameTimer_Get` is the frame delta in seconds. `FrameTimer_GetEMA10` is a
  smoothed average (handy for an FPS readout).

---

## 2. Interfaces — your screen layers

An `Interface` is a container of widgets. You create it, add widgets to it, and
tick it each frame.

```lts
ui       = (Interface_Create "UI")          # war.lts:19
gameView = (Interface_Create "Game View")   # war.lts:20

ui.Add (Widget/HUD:Create player)           # war.lts:79  (add a widget)
gameView.Add (Widget_Rendered passes)       # war.lts:30  (add a 3D pass)
gameView.Clear                               # remove everything currently in it
```

Interface methods (all verified):

| Method | Purpose |
|---|---|
| `Interface_Create "name"` | Construct an interface |
| `.Add (Widget)` | Add a widget (also `.Add (Custom Widget X)`, `.Add (Widget_Rendered passes)`) |
| `.Update` | Per-frame logic tick |
| `.Draw` | Render this interface |
| `.Clear` | Remove all content (e.g. swap a loading screen for the game) |

> **Pattern — loading screen then game:** build the loading widget in
> `Initialize`, draw it for N seconds in `Update`, then `gameView.Clear` and
> `gameView.Add (Widget_Rendered passes)` once. This is exactly what
> `ltheory-main.lts` does (see §9). Swapping widgets at runtime is fine; trying
> to *build the whole world inside `Update`* is what crashed earlier — build in
> `Initialize`.

---

## 3. Rendering a 3D game view (`Widget_Rendered` + `RenderPass*`)

The 3D world is drawn by composing a list of render passes into a
`Vector<Reference<RenderPassT>>` and wrapping it in `Widget_Rendered`.

```lts
# war.lts:23-30
var passes Vector<Reference<RenderPassT>>
passes.Append (RenderPass_Clear (Vec4 0.0))        # clear to black
passes.Append (RenderPass_Camera camera)
passes.Append (RenderPass_SMAA)                    # antialiasing
passes.Append (RenderPass_Interface ui)            # composite the UI on top
passes.Append (RenderPass_PostFilter "post/dither.jsl")
gameView.Add (Widget_Rendered passes)
```

`RenderPass_Interface ui` is what makes your `ui` interface visible over the 3D
scene. Without it the UI won't show up in the game view.

> **CRITICAL — `RenderPass_Clear` is required, even for pure-2D UI.** If you
> draw a 2D UI with a bare `gameView.Draw` (no `RenderPass_Clear` pass), the
> framebuffer is *never cleared between frames*. Anything that moves — most
> obviously a cursor — will **smear into a trail** as old frames accumulate.
> (This is what bit `launcher.lts`: drawing the cursor directly in `App::Update`
> with bare `gameView.Draw` left a snake-like smear. The fix was to render
> through a `RenderPass_Clear -> RenderPass_Camera -> RenderPass_Interface(ui)
> -> RenderPass_PostFilter` pipeline, exactly like `war.lts`.) The list rows
> looked fine only because they are drawn at *fixed* positions and overwrite
> themselves every frame; the moving cursor exposed the missing clear.
>
> If you want a 2D-only driven app (no 3D world), you can still use the full
> pipeline — `RenderPass_Camera` with an empty scene is harmless. See the
> `launcher.lts` example in §14.

---

## 4. Widgets — what they are and how to make one

A **widget** is a value of type `Widget`. Most widgets are produced by a
`Create` function in a `Widget/<Name>.lts` file and added to an interface:

```lts
ui.Add (Widget/HUD:Create player)            # HUD.lts:133  -> function Widget Create (Player player)
ui.Add (Widget/GameMenu:Create)              # GameMenu.lts:37 -> function Widget Create ()
ui.Add (Widget/SplashScreen:Create)          # SplashScreen.lts:50 -> function Widget Create ()
```

### 4.1 Stock widget `Create` signatures

| Widget | File | Signature |
|---|---|---|
| HUD | `Widget/HUD.lts:133` | `function Widget Create (Player player)` |
| Button | `Widget/Button.lts:46` | `function Widget Create (Data onPress String text Float size)` |
| ExitButton | `Widget/ExitButton.lts:41` | `function Widget Create (String text Float size)` |
| Window | `Widget/Window.lts:69` | `function Widget Create (String title Widget contents)` |
| Text | `Widget/Text.lts` (via `Widgets.lts:241`) | `function Widget Text (Font font String text Float size Vec3 color)` |
| SplashScreen | `Widget/SplashScreen.lts:50` | `function Widget Create ()` |
| GameMenu | `Widget/GameMenu.lts:37` | `function Widget Create ()` |
| Settings | `Widget/Settings.lts` | `function Widget Create (Widget widget)` / `function Widget Create ()` |
| TextEditor | `Widget/TextEditor.lts:20` | `function Widget Create (String path)` |
| TextField | `Widget/TextField.lts:61` | `function Widget Create (String label Float size (Pointer String) contents)` |
| ToggleButton | `Widget/ToggleButton.lts:18` | `function Widget Create ((Pointer Bool) value String text)` |
| ObjectInfo | `Widget/ObjectInfo.lts` | `function Widget Create (Player player Object object)` |
| Map | `Widget/Map.lts:88` | `function Widget Create (Player player Object container)` |
| Handling | `Widget/Handling.lts:276` | `function Widget Create (Object object)` |
| Slider | `Widget/Slider.lts` | `Widget/Slider:Create "name" min max` |
| IconButton | `Widget/IconButton.lts` | `function Widget IconButton (Data onPress Icon icon)` |
| IconTextButton | `Widget/IconButton.lts` (via `Widgets.lts:48`) | `function Widget IconTextButton (Data onPress Icon icon String text)` |
| Browser | `Widget/Browser.lts:83` | `function Widget Create (Widget widget)` |
| Observatory | `Widget/Observatory.lts:177` | `function Widget Create (Camera camera)` |
| FocusWindow | `Widget/Window.lts` (via `Widgets.lts`) | `function Widget FocusWindow (Float opacity Widget)` |

`Widgets:` and `Widget/Components:` are convenience aliases (see §5, §6).

### 4.2 Anatomy of a custom widget (`Custom Widget`)

You make your own widget by defining a `type`, then wrapping an instance with
`Custom Widget <Type> [args]`. The engine calls optional *hook* functions on
your type at the right times.

```lts
type MyWidget
  Float time 0

  function Void Create (Widget self)          # one-time init (optional)
    # e.g. build child data here

  function List CreateChildren (Widget self)  # return a List of child widgets (optional)
    var l List
    l += (Widgets:Text Fonts:Default "Hello" 24.0 1.0)
    l

  function Void PreUpdate (Widget self)       # input/logic, before children update
    if Key_Escape.Pressed
      Exit

  function Void PostUpdate (Widget self)      # input/logic, after children update
    # e.g. read Mouse_LeftPressed, fire callbacks

  function Void PreDraw (Widget self)         # draw UNDER children
    DrawPanel self.pos self.size 0.1 1 1 0

  function Void PostDraw (Widget self)        # draw OVER children
    DrawText Fonts:Default "FPS: " + (FrameTimer_GetEMA10 * 1000.0) self.pos 16 1 1 false

# usage:
ui.Add (Custom Widget MyWidget)
```

**Hook summary** (all optional; only define what you need):

| Hook | When | Typical use |
|---|---|---|
| `Create (Widget self)` | once, on creation | allocate child state (`nodes`, lists) |
| `CreateChildren (Widget self) -> List` | once, builds sub-widgets | lay out the widget's contents |
| `PreUpdate (Widget self)` | each frame, before children | keyboard input, escape-to-exit |
| `PostUpdate (Widget self)` | each frame, after children | mouse hit-test, fire `onPress` |
| `PreDraw (Widget self)` | each frame, before children draw | background panel |
| `PostDraw (Widget self)` | each frame, after children draw | foreground text, cursor, overlays |

**Self fields you can read/set:** `self.pos` (Vec2), `self.size` (Vec2),
`self.Center`, `self.TopLeft`, `self.TopRight`, `self.alpha`, `self.focusMouse`,
`self.focusKey`, `self.minSize`.

**Self methods:** `self.Rebuild`, `self.AddChild (Widget)`,
`self.SendUp (Data)`, `self.SendDown (Data)`, `self.Delete`,
`self.GetPoint (Float x Float y) -> Vec2`.

> **Gotcha — `block` scopes locals:** if you declare `var point (Glyph_Circle ...)`
> *inside* a `block { ... }`, it is **not visible** outside that block. Declare
> draw temporaries at function scope, or the later `Draw point` will reference an
> undefined name. (This was the bug that crashed `ltheory-main.lts`'s loading
> screen.)

---

## 5. Layout helpers (`Widgets:` alias → `Widget/Widgets.lts`)

These free constructors arrange child widgets. They are not `function`
declarations — just use them inline.

| Helper | Meaning |
|---|---|
| `Stack` | vertical stack of children |
| `ListV <spacing>` | vertical list with spacing (e.g. `ListV 8`) |
| `ListH <spacing>` | horizontal list |
| `ListH3 <spacing>` | 3-column horizontal list |
| `SpacerH <w>` / `SpacerV <h>` | fixed empty space |
| `Grid <cell> <gap> <minCols> <widget>` | grid layout |
| `Tab <group> <text>` | a tab button |
| `RadialList <radius> <widget>` | radial arrangement |
| `ScrollFrame (Bool vertical) <widget>` | scrollable region |
| `Text <font> <text> <size> <color>` | a text widget |
| `TextGlow <font> <text> <size> <color>` | glowing text |
| `Icon <icon> <color>` | icon widget |
| `IconButton <onPress> <icon>` | icon button |
| `IconTextButton <onPress> <icon> <text>` | icon + label button |
| `FocusWindow <opacity> <widget>` | modal window that captures mouse/key/scroll |

Example (GameMenu.lts:15-31):

```lts
function List CreateChildren (Widget self)
  var l List
  l +=
    Components:AlignBottomCenter
      Components:Margin 8 8
        Widgets:Text Fonts:Heading "LIMIT THEORY v0.82.2" 24.0 1.0
  l +=
    Components:AlignCenter
      Components:Padding 8 8
        ListV 8
          Custom Widget Buttons
  l
```

---

## 6. Decorators / components (`Widget/Components:` → `Widget/Components.lts`)

These wrap a widget to give it behavior or styling. Chain them:

| Component | Effect |
|---|---|
| `Components:Expand` | fill available space |
| `Components:ExpandX` / `Components:ExpandY` | fill one axis |
| `Components:AlignCenter` / `AlignTopLeft` / `AlignBottomRight` / `AlignBottomCenter` | alignment |
| `Components:Margin <in> <out>` / `Components:Padding <in> <out>` | spacing |
| `Components:Positioned (Vec2 pos) <widget>` | fixed position |
| `Components:Backdrop <color> <inA> <a> <bevel> <widget>` | panel background |
| `Components:BackdropGrid <color> <alpha> <widget>` | grid backdrop |
| `Components:CaptureMouse` / `CaptureKey` / `CaptureScroll` | grab input |
| `Components:Clickable <message> <widget> <receiver>` | click → send message |
| `Components:Draggable` | make draggable |
| `Components:Named <name> <widget>` | name for lookup |
| `Components:MinSize` / `Components:MaxSize` | size constraints |
| `Components:Tooltip <widget>` | hover tooltip |

`Button.lts:46-51` shows the canonical "capture mouse + pad + stack + custom":

```lts
function Widget Create (Data onPress String text Float size)
  Components:CaptureMouse
    Components:Padding (Vec2 6 6) 6
      Stack
        Custom Widget
          Button onPress text size
```

---

## 7. Buttons and callbacks (`onPress` is a `Data`)

A button's `onPress` is a deferred script call of type `Data`. `0` is the
no-op value. The button fires it when the mouse is over it and pressed
(`Button.lts:42-44`):

```lts
if self.focusMouse
  if Mouse_LeftPressed
    self.SendUp onPress
```

### 7.1 Creating buttons

```lts
# no-op button:
Components:Expand (Button:Create 0 "SAVE GAME" 18)        # GameMenu.lts:4

# button that sends a message when clicked:
Button:Create (MessageBuy item) "BUY" 16.0                # Market/RightPanel.lts:41
Button:Create Messages:MessageCancel "CANCEL" 16.0        # Market/Transaction.lts:156
Widget/Button:Create Messages:MessageClick "ADD" 16       # DevPanel/Tasks.lts:73

# icon button:
Widgets:IconTextButton onPress Icons:Person "Option"
```

`onPress` is built from a **message constructor** like `MessageBuy`,
`MessageCancel`, `MessageClick`, `SelectItem`, `TabSelect`, `SliderSet`, etc.
These live in `Widget/Messages.lts` and similar. When the button is clicked,
the message travels up the widget tree via `self.SendUp` until a widget's
`Receive (Widget self Data data)` hook handles it.

### 7.2 Handling a message (`Receive`)

```lts
function Void Receive (Widget self Data data)
  switch
    data == (MessageBuy item)  # ... do the buy
    data == (MessageCancel)    self.Delete
    otherwise                  # ignore
```

(`market.lts`, `HUD.lts`, `Window.lts` all use this pattern.)

### 7.3 ToggleButton / Slider (state-bound callbacks)

```lts
# binds to a Bool you can read elsewhere:
Widget/ToggleButton:Create (address gOptions.symmetryX) "X SYMMETRY"   # ModelEditor.lts:9

# slider: onPress fires SliderSet / SliderMin / etc.
Widget/Slider:Create "myslide" 0 128                                    # ui.lts:82
```

### 7.4 The `ExitButton` (quit the app)

`ExitButton` does not take an `onPress`; it calls the global `Exit` directly:

```lts
ExitButton:Create "EXIT GAME" 18        # GameMenu.lts:8
# internally: if self.focusMouse && Mouse_LeftPressed -> Exit
```

`Exit` is a zero-argument global ScriptAPI function that marks the running
`Program` for deletion (see AGENTS.md "Exit Game button" fix).

> **Gotcha — focus requires `CaptureMouse` + `CaptureFocus`.** A button only
> responds to clicks if its `focusMouse` flag is set, which happens via the
> `CaptureFocus` hook hit-testing `Cursor_Get` against `ClipRegion_GetMin/Max`
> (`Button.lts:26-30`). Always wrap interactive widgets in
> `Components:CaptureMouse` (or `FocusWindow` for a whole modal). Without it,
> clicks do nothing.

---

## 8. Making a Main Menu

There is no single "Main Menu" app yet, but `Widget/GameMenu.lts` and
`Widget/SplashScreen.lts` are the building blocks, and `ltheory-main.lts`
shows how to present one after a loading screen. A main menu is just:

1. a `FocusWindow` (modal) containing a `ListV` of `Button`s, and
2. an `App` whose `Update` shows it (e.g. toggled by Escape, like the HUD does).

### 8.1 The menu widget (adapted from `GameMenu.lts:14-42`)

```lts
type MainMenu
  function List CreateChildren (Widget self)
    var l List
    l +=
      Components:AlignBottomCenter
        Components:Margin 8 8
          Widgets:Text Fonts:Heading "MY GAME" 32.0 1.0
    l +=
      Components:AlignCenter
        Components:Padding 8 8
          ListV 8
            Components:Expand (Button:Create StartGame "START" 20)
            Components:Expand (Button:Create OpenSettings "SETTINGS" 20)
            Components:Expand (ExitButton:Create "QUIT" 20)
    l

  function Void Receive (Widget self Data data)
    switch
      data == StartGame   # swap the interface to the game view, etc.
      data == OpenSettings
      otherwise

  function Void PreUpdate (Widget self)
    if Key_Escape.Pressed
      self.Delete          # close the menu

function Widget Create ()
  Widgets:FocusWindow 0.8
    Components:Expand
      Stack
        Custom Widget MainMenu
```

### 8.2 Wiring it into the `App`

```lts
type App
  Interface ui
  Interface gameView
  Camera camera
  Bool inMenu true

  function Void Initialize ()
    camera = Camera_Create
    camera.Push
    ui = (Interface_Create "UI")
    ui.Add (Custom Widget MainMenu)     # show menu immediately

  function Void Update ()
    ui.Update
    ui.Draw

function App Main ()
  var self App
  self
```

(For the full "loading screen → game" flow, see §9 and `ltheory-main.lts`.)

---

 ## 9. Worked example: loading screen → game (`ltheory-main.lts`)

 This is the reference pattern for "show an animation while the world is
 prepared, then play." Key points:

 - Build the **whole world in `Initialize`** (do not build inside `Update` — that
   crashed `camera.SetTarget player.GetPiloting` with a null reference).
 - In `Update`, draw the loading widget for `loadTime` s, then `gameView.Clear`
   and add the `Widget_Rendered` passes once.
 - Use `static` for any per-frame state that must persist across `Update` calls
   (`static started false`, `static loadingElapsed 0.0`). **`static` needs an
   initializer: `static x false`, not `static x`.**
 - **Loader clock sync.** Sample `loadingElapsed += FrameTimer_Get` *after*
   `gameView.Draw`, and have the loading widget accumulate its own `time` inside
   `PostDraw` during the draw. The two clocks otherwise drift (~1.5–2x) so the
   % bar and the swap desync. (See `ltheory-main.lts`.)
 - **The C++ `Object_System` factory only makes the star + nebula + starfield.**
   It does **not** create planets/asteroids. A separate populator
   (`Object/SystemPopulate:Init root`) must add them. **Do not** reuse the
   upstream `Object/System.lts` — its `Init` places the planet at
   `Vec3_Cylinder 0 ...` (radius 0 → spawns at the origin, inside the camera)
   and has a broken `Orbital rail` block (`Object/WarpNode` fails to compile in
   this build). `Object/SystemPopulate.lts` is the working local replacement.
 - **Spawn the player outside the planet.** The planet is placed at a real
   orbital radius with `SetScale ~systemScale`, so spawning the ship at the
   origin (or 5000 units out) embeds it **inside** the planet mesh. `ltheory-main`
   spawns the player at `systemScale*4` (+Z, ~400k) and AI ships on a
   `systemScale*3` shell.
 - **Camera needs an offset.** `Camera_Create` puts the camera at the target's
   position with only a tiny default offset. Set `camera.SetRelativePos
   (Vec3 0 40 160)` + `camera.SetRelativeLookAt (Vec3 0 0 0)` so you view the
   ship from outside (otherwise you sit inside it and see only background).
 - **Planets have no collision bounding box in this build**, so you can fly
   through the mesh. Enforce a barrier in `Update` by scanning
   `root.GetInteriorObjects` for `o.GetType == "Planet"` and clamping the ship to
   `planetRadius * 1.1` (see §12.18).

 ```lts
 type App
   Interface ui
   Interface gameView
   Camera camera
   Player player
   Object root

   function Void Initialize ()
     camera = Camera_Create
     camera.Push
     camera.SetRelativePos (Vec3 0 40 160)          # third-person offset
     camera.SetRelativeLookAt (Vec3 0 0 0)
     ui = (Interface_Create "UI")
     gameView = (Interface_Create "Game View")

     var universeSeed (ToInt (Config_Get "seed"))    # from gameConfig.txt
     root = (Object_System (Vec3 15.012) universeSeed)
     Object/SystemPopulate:Init root                 # planet + 1000 asteroids
     var shipType (Item_ShipType (ToInt (Config_Get "shipHull")) 55 1 1 1 1 1 1)
     var ship shipType.Instantiate
     ship.SetPos (Vec3 0 0 (100000.0 * 4.0))         # outside the planet
     root.AddInterior ship
     player = Player_Human
     player.AddAsset ship
     player.Pilot ship
     ui.Add (Widget/HUD:Create player)
     gameView.Add (Custom Widget LoadingScreen)      # loading animation first

   function Void Update ()
     static started false
     static loadingElapsed 0.0
     var LOAD_TIME (ToFloat (Config_Get "loadTime"))
     if (LOAD_TIME < 0.5) LOAD_TIME = 18.0

     if (! started)
       gameView.Update
       gameView.Draw
       loadingElapsed += FrameTimer_Get             # sample AFTER Draw
       if (loadingElapsed >= LOAD_TIME)
         var passes Vector<Reference<RenderPassT>>
         passes.Append (RenderPass_Clear (Vec4 0.0))
         passes.Append (RenderPass_Camera camera)
         passes.Append (RenderPass_SMAA)
         passes.Append (RenderPass_Interface ui)
         passes.Append (RenderPass_PostFilter "post/dither.jsl")
         gameView.Clear
         gameView.Add (Widget_Rendered passes)
         started = true
       return

     var dt (Min 0.1 FrameTimer_Get)
     root.Update dt
     camera.SetTarget player.GetPiloting
     ui.Update
     gameView.Update
     gameView.Draw

 function App Main ()
   var self App
   self
 ```

 `Config_Get(key)` reads `key:value` lines from `resource/script/gameConfig.txt`
 (`#` comments, no spaces around `:`). LTSL's `String_Split` 2-arg overload is
 not bindable in this build, so the loader parses with `Substring`/`Length`
 (see `ltheory-main.lts`). The `LoadingScreen` custom widget (full version in
 `ltheory-main.lts`) animates 512 physics nodes and draws a `LIMIT THEORY`
 heading + a completion %; its `Create` builds the node array, `PostUpdate`
 steps the spring physics, and `PostDraw` renders the nodes with
 `Draw (Glyph_Circle ...)` / `Draw (Glyph_LineFade ...)`.

 ---

## 10. Drawing primitives (immediate mode, from `PreDraw`/`PostDraw`)

All drawing happens inside a widget's `PreDraw`/`PostDraw`. Coordinates are
screen-space `Vec2` (pixels). Colors are `Vec3` (0..1) unless noted.

### `DrawPanel` — a filled rectangle / panel

```lts
DrawPanel self.pos self.size 0.1 1 1 0                 # Button.lts:23  (col, ?, ?, ?)
DrawPanel self.pos self.size 1.0 * (Vec3 0.1 0.2 0.3) 1 1 0   # launcher.lts:23
DrawPanel self.pos self.size 0.05 0.95 opacity * self.alpha 16 # Window.lts:39 (bevel arg)
```
Form: `DrawPanel <pos> <size> <color-ish> ...`. The exact middle args vary by
call site; copy a nearby usage. `color` can be a `Vec3` or a scalar.

### `DrawText` — text

```lts
DrawText Fonts:Heading "LIMIT THEORY" self.Center 72 0.1 1 true   # loading.lts:79
DrawText Font_Get "Gafata/Regular.ttf" "hello" self.pos 16 1 1 false
```
Form: `DrawText <font> <text> <pos> <size> <color> <alpha> <centered?>`.
`centered?` is a `Bool` (`true` centers on `<pos>`).

`DrawTextGlow <font> <text> <pos> <size> <color> <alpha> <centered?>` is the
glowing variant (`SplashScreen.lts`, `Text.lts:27`).

### `Draw` — glyphs, icons, textures

```lts
Draw (Glyph_Circle 0 1 1 1) center 16.0 Colors:Primary 1.0   # life.lts:58
Draw (Glyph_LineFade p1 p2 1 1) 0 1 c1 0.05                  # loading.lts:110
Draw icon self.Center 0.7 * r color self.alpha              # Icon.lts:8
Draw texture self.pos self.size 1.0                         # launcher.lts:24
```
Form: `Draw <glyph-or-icon-or-texture> <pos> <size> <color> <alpha>`.

> **There is no `DrawIcon` function** in this codebase. Icons are drawn with
> `Draw <icon> ...` or by building a glyph (`Glyph_Circle`, etc.).

### Available glyph constructors

| Glyph | Form |
|---|---|
| `Glyph_Circle (Vec2 c) <r> 1 1` | filled circle |
| `Glyph_Arc (Vec2 c) <r> <thick> 1 1 <a0> <a1>` | arc/pie |
| `Glyph_Line (Vec2 p1) (Vec2 p2) 1 1` | line |
| `Glyph_LineFade (Vec2 p1) (Vec2 p2) 1 1` | faded line |
| `Glyph_Box (Vec2 c) (Vec2 s) 1 1` | box |
| `Glyph_Triangle (Vec2 p1) (Vec2 p2) (Vec2 p3) 1 1` | triangle |
| `Glyph_Grid (Vec2 tl) (Vec2 br) 1 1 0 0.5` | grid |

(`Icons.lts`, `ui.lts`, `loading.lts`, `Icon/Cursors.lts`.)

Build custom icons by appending glyphs:

```lts
var icon Icon_Create
icon += (Glyph_Circle 0 1 1 1)        # SplashScreen.lts:41-42
```

### Fonts

```lts
Font_Get "Rajdhani/Light.ttf"         # by filename
Fonts:Heading                         # named accessor
Fonts:Default  Fonts:Medium  Fonts:Light  Fonts:Subheading  Fonts:Unicode  Fonts:ChineseSimp
```
Named accessors are defined in `resource/script/Fonts.lts`.

### Colors & cursors

```lts
Colors:Primary                        # a named color (Vec3)
```

**Drawing a cursor / mouse pointer:**

- **Reading the mouse position.** Use `Mouse_GetPosImmediate` to get the live
  SFML cursor position in window pixels. **Do NOT use `Cursor_Get` outside of a
  widget's draw/update context** — `Cursor_Get` reads from a *cursor stack* that
  `Interface_Update`/`Interface_Draw` push/pop (`Cursor_Push`) around their
  widgets. If you call `Cursor_Get` from `App::Update` (before/after the
  interface draws), the stack is empty and it returns garbage, so the cursor
  sits frozen in one spot (e.g. top-left) and won't follow the mouse.
  `Mouse_GetPosImmediate` queries the OS directly and is always valid.
- **Drawing the pointer glyph.** Two forms exist, with very different behavior:

  ```lts
  # Form A (icon) — smears! do not use for a live cursor:
  Draw Icon/Cursors:Pointer Cursor_Get 12 2.0 * Colors:Primary 1.0   # ltheory-main.lts:67

  # Form B (glyph) — clean, use this:
  Draw (Glyph_Circle 0 1 1 1) Mouse_GetPosImmediate 32 Colors:Primary 1.0
  ```

  `Draw Icon/Cursors:Pointer ...` renders the icon into a **persistent render
  target that is not cleared per frame**, so a moving cursor drawn this way
  smears into a trail (it does not get wiped by `RenderPass_Clear`). It happens
  to look OK in `ltheory-main.lts` only because that app switches to a real
  `Widget_Rendered` pipeline with its own clear — but for a live-following
  cursor it is fragile. **Prefer `Draw (Glyph_Circle ...)`** (or any `Glyph_*`)
  for a cursor: glyphs draw into the same cleared overlay buffer as `DrawText`/
  `DrawPanel`, so they do not smear. You can also scale the radius (e.g. `32`)
  to make the dot bigger.

> **Where to draw the cursor.** Draw it inside a widget's `PostDraw` hook (not
> from `App::Update`), so it is part of the cleared, per-frame overlay. Drawing
> `Draw*` calls directly in `App::Update` (outside the interface draw pass)
> also risks the trail / wrong depth.

---

## 11. Input

| Input | Meaning |
|---|---|
| `Key_Escape.Pressed` / `Key_Y.Pressed` / `Key_U.Pressed` / `Key_H.Down` | key state (`Pressed` = this frame, `Down` = held) |
| `Mouse_LeftPressed` | left button pressed this frame |
| `Mouse_GetPosImmediate` | **live** cursor position (Vec2), queries the OS directly. Use this for hit-testing / drawing a cursor. |
| `Mouse_GetPos` / `Mouse_GetPosLast` | cursor position as tracked by the engine (may be stale by a frame inside `Update`). |
| `Cursor_Get` | cursor position from the *cursor stack*; only valid **inside a widget's draw/update hook** (pushed by `Interface_Draw`/`Interface_Update`). Returns garbage elsewhere — do not use from `App::Update`. |
| `ClipRegion_GetMin` / `ClipRegion_GetMax` | current clip rect (for hit-testing) |

> **Mouse position gotcha.** For hover hit-testing and cursor drawing, use
> `Mouse_GetPosImmediate`. `Cursor_Get` is only meaningful inside a widget hook;
> calling it from `App::Update` yields a frozen / wrong position (cursor won't
> follow the mouse). This was the original launcher bug: `Cursor_Get` returned a
> stale/fixed value because the cursor stack was empty outside the draw pass.

Keyboard key names follow the SFML enum, mapped through the engine's
`Key_Name` / `Key_Down` / `Key_Pressed` bindings in `Keyboard.cpp`.
The underlying SFML 3.x enums are scoped (`sf::Keyboard::Key::A`,
`sf::Keyboard::Key::Escape`, etc.), but the engine presents them to LTSL
as `Key_Escape`, `Key_Space`, `Key_F1`, etc. — **no change for script
authors**. The key-value integer mapping (scan codes) is unchanged
from SFML 2.x through 3.1.

---

## 12. Gotchas & engine quirks (learned the hard way)

  1. **Driven vs self-widget apps.** Only apps with `Main -> App` + `Initialize` +
     `Update` are ticked. Every script under `App/` that used the old
     `widget:Create Layer ...` shape has now been converted to a driven app
     (see §13.4). Start new UI from `war.lts` / `ltheory-main.lts` /
     `launcher.lts`. The canonical driven skeleton is:
     ```lts
     type App
       Interface ui
       Interface gameView
       Camera camera
       function Void Initialize ()
         camera = Camera_Create; camera.Push
         ui = (Interface_Create "UI")
         gameView = (Interface_Create "Game View")
         var passes Vector<Reference<RenderPassT>>
         passes.Append (RenderPass_Clear (Vec4 0.0))
         passes.Append (RenderPass_Camera camera)
         passes.Append (RenderPass_Interface ui)
         passes.Append (RenderPass_PostFilter "post/dither.jsl")
         gameView.Add (Widget_Rendered passes)
         ui.Add (Custom Widget YourScreen)
       function Void Update ()
         ui.Update; gameView.Update; gameView.Draw
     function App Main ()
       var self App
       self
     ```
  2. **Build the world in `Initialize`, not `Update`.** Building inside `Update`
    then calling `camera.SetTarget player.GetPiloting` crashed with a null
    reference. Build once up front.
  2b. **You CANNOT pass arguments to `Custom Widget`.** `Custom Widget MyScreen
    player` does **not** compile (it silently fails to compile, leaving `ui`
    empty → a black screen). If a screen needs state (a `Player`, an `Object`,
    …), build it **inside the widget's own `Create` method** and store it as a
    bare field (`player = Player_Human`), exactly like `objectinfo.lts` /
    `hud.lts` / `map.lts` / `market.lts` do. (Passing args works for real
    *factory functions* like `Widget/HUD:Create player` or
    `Widget/ObjectInfo:Create player object` — those are functions, not
    `Custom Widget`.)
  2c. **Set the camera target every frame.** Any app whose widgets read the
    camera (`Widget/HUD`, `Widget/Map`, `HUD/WorldObjects`, reticle/minimap…)
    requires `Camera_Get.SetTarget player.GetPiloting` (or
    `camera.SetTarget …`) called each frame, or the widget draws nothing (black
    screen). `war.lts` does this in `App::Update`; for self-contained widgets do
    it in the widget's `PreUpdate` via `Camera_Get.SetTarget player.GetPiloting`.
    Also note `Widget/HUD.lts` gates most of its children behind
    `if Camera_Get.IsNotNull`, so a missing camera target drops the whole HUD.
  2d. **Don't reference widget-built objects from `App::Update`.** If the screen
    widget builds `system`/`player` in its own `Create`, the `App` type does NOT
    have those fields — calling `system.Update …` from `App::Update` dereferences
    a null `system`. Move per-frame object updates (`system.Update dt`,
    `station.Update dt`) into the widget's `PreUpdate` instead.
3. **`static` needs an initializer.** `static x false` works; `static x` does
   not compile (`'static' expects 2 arguments`).
4. **Don't add App members beyond what works.** Adding extra typed members to
   the `App` type (e.g. a 6th field) has crashed instance construction with a
   null reference in `Update`. Keep the `App` type lean (the proven `war.lts`
   shape has 5 reference members). Store extra per-frame state in `static`
   variables inside `Update`, not as App members.
5. **Avoid a member named `system`.** Assigning to an App member literally
   called `system` silently failed (the value stayed null), crashing later
   reads. Rename it (e.g. `root`). `system` appears to be a contextual/reserved
   identifier.
6. **`block { }` scopes locals.** Declare draw temporaries (`point`, `c1`, …)
   at function scope, not inside a `block`, or later uses see undefined names.
7. **Buttons need `CaptureMouse` + `CaptureFocus`.** Without input capture the
   `focusMouse` flag is never set and clicks do nothing. Wrap interactive
   widgets in `Components:CaptureMouse` or `Widgets:FocusWindow`.
8. **`Layer`/`Compositor` rendering path crashes when driven inside an
   `Interface`.** Use a plain `Custom Widget` in a `gameView` Interface instead
   of `Layer`/`Compositor_Basic` for in-app overlays. (`SplashScreen.lts` /
   `launcher.lts` use `Layer` because they are self-widget apps; don't copy that
   into a driven app.)
 9. **`if/otherwise` is invalid.** `otherwise` only works inside `switch`. In an
    `if`, use a nested `if` or a `switch` with a predicate.
 10. **No `DrawIcon`.** Use `Draw <icon>` / `Draw (Glyph_...)`.
 11. **A bare `gameView.Draw` smears moving draws.** Without a `RenderPass_Clear`
    pass the framebuffer is never cleared between frames, so a moving cursor
    leaves a trail (§3, §14). Always render through a `RenderPass` pipeline that
    starts with `RenderPass_Clear`.
 12. **`Cursor_Get` is only valid inside a widget hook.** Calling it from
    `App::Update` returns garbage/garbage (empty cursor stack) and the cursor
    freezes in one spot. Use `Mouse_GetPosImmediate` for the live mouse
    position (§11). `Cursor_Get` is fine inside `PreDraw`/`PostDraw`/`PreUpdate`/
    `PostUpdate` of a widget.
 13. **`Draw Icon/Cursors:Pointer` smears the cursor.** It renders to a
    persistent target not wiped by `RenderPass_Clear`. For a live-following
    cursor, draw `Draw (Glyph_Circle 0 1 1 1) Mouse_GetPosImmediate <r> color 1.0`
    instead (see §10 "Colors & cursors").
 14. **`Directory_List` returns a `Vector<String>`.** Read elements with
    `var name (files.Get i)` (value, not `ref`); use `.Size` for the count
    (§14.2). Path is relative to the engine's working directory.
  15. **LTSL cannot launch another app.** There is no script API to start a
     different `.lts`. A launcher can only highlight the selection (§14.3).
  16. **`return` keyword is now supported (Revamp Work).** A real `return`
      keyword was added (see AGENTS §8c.2 #4): `return expr` returns the value,
      bare `return` returns void, and it works from inside `if`/`switch`/blocks.
      The legacy behavior (functions return their last expression) is preserved
      for backward compatibility, so older scripts that omit `return` still work.
      `Texture/RandomScreenshot.lts` (which used `return`) was rewritten to load
      `resource/texture/splash.png` (its original `return` + hardcoded
      `~/Dropbox/...` path were both broken).
  17. **`switch -- case statement did not compile` at startup is benign.** It is
     printed twice (same as `war.lts`) and does not prevent the app from running.
     Ignore it.
  18. **`Object_System` does not create planets/asteroids.** The C++ factory only
      builds the star + nebula + starfield. To get a populated system you must
      call a populator (e.g. `Object/SystemPopulate:Init root`). Do **not** use
      the upstream `Object/System.lts` `Init` — it places the planet at
      `Vec3_Cylinder 0 ...` (radius 0, spawns at the origin **inside the camera**).
      Its `Orbital rail` block (which links warp nodes via `Object/WarpNode`/
      `Object/WarpRail`) **now compiles and runs** (the `Position` type gap was
      fixed — see AGENTS §8d #1), but the planet placement is still wrong, so keep
      using `Object/SystemPopulate.lts` as the populator. Seed everything from one
      `RNG_MTG(self.GetSeed)` so layouts are reproducible per seed.
  21. **`Position` is a fully working LTSL type now.** `Position` is bound to the
      engine's double-precision `Vec3d` (alias), so `SetLook(Normalize(p2 - p1))`,
      `GetPos self + (radius * dir)`, and `ps.Add(particle, pos, velocity, ...)`
      all work with `Position` values (e.g. warp-rail / `Object/WarpNode` scripts).
      Do **not** rely on a `Position -> V3` conversion or mixed-precision
      `Vec3d + V3F` *operator* overloads — those were tried and rejected because
      they corrupt the type registry (static-init SIOF) or create ambiguous
      overloads that break single-precision ship physics (NaN → `force.IsFinite()`
      abort in `Motion.cpp`). Prefer the existing `V3F -> V3D` conversion for
      mixed `Position + V3` arithmetic; it resolves cleanly.
  22. **LTSL dotted random calls are fragile.** `rng.Float` resolves to
      `(Float rng)` — i.e. *constructing the `Float` type from `rng`*, not calling
      an RNG method (because `Float` is a type name). So `rng.Float` and
      `rng.Float 0 1` (the 2-arg form) are both broken. Use the no-arg `rng.Exp`
      for an exponential random value, `rng.Int`, `rng.Direction`, `rng.Sphere`,
      or the explicit `Float (RNG_Float rng ...)` function form instead. This is
      why `Object/WarpRail.lts`'s `Update` was rewritten to use `rng.Exp` + plain
      parenthesized arithmetic.
  19. **Iterate an object's children with `GetInteriorObjects`.**
     `for it obj.GetInteriorObjects it.HasMore it.Advance` then `it.Get` gives
     each child `Object`. `obj.GetInteriorObjects` (no type arg) yields **all**
     types. To classify a child, use `o.GetType` (returns a string like
     `"Planet"`, `"Ship"`, `"Asteroid"`) or `o.GetName`. There is no script
     binding to read an object's *container* (`GetContainer` exists in C++ but is
     not exposed to LTSL).
  20. **`Texture/RandomScreenshot.lts` was broken in this fork.** It hardcoded
     Josh's `~/Dropbox/lt/screenshot` path and used `return`. It now simply
     loads `resource/texture/splash.png`, which also unblocks `Widget/DevPanel`
     (whose backdrop used it) and the apps that referenced it as a backdrop
     (`widget.lts`, `ui.lts`, `map.lts`, `image.lts`, `objectinfo.lts`,
     `market.lts`, `hud.lts`). If you see
     `'Texture/RandomScreenshot' -- failed to compile`, that file regressed.

---

## 14. Worked example: a pure-2D launcher UI (`launcher.lts`)

`launcher.lts` is a complete, minimal **driven** app that draws a 2D UI with no
3D world: it lists every `.lts` app in `resource/script/App/` as a hover-
highlighted, clickable list, with a title and a mouse cursor. It is the reference
for the "I just want a screen of UI" case and demonstrates several hard-won
correct patterns:

- **Render through a `RenderPass` pipeline (with `RenderPass_Clear`) even for 2D.**
  This is what stops the cursor from smearing (see §3 note).
- **Draw all UI inside a `Custom Widget`'s `PostDraw` hook**, so it lands in the
  cleared overlay buffer.
- **Read the mouse with `Mouse_GetPosImmediate`**, not `Cursor_Get`.
- **Draw the cursor as a `Glyph_Circle`**, not `Draw Icon/Cursors:Pointer`.

### 14.1 The app

```lts
type LauncherScreen
  function Void PostDraw (Widget self)
    var m Mouse_GetPosImmediate           # live mouse position

    static selected -1                    # persists across frames (needs init)
    static clicked -1

    var files (Directory_List "resource/script/App/")

    var y 96.0
    for i 0 i < files.Size i.++
      var name (files.Get i)              # value read: `var`, not `ref`
      var p (Vec2 64 y)
      var s (Vec2 512 28.0)
      var hover (p <= m && m <= p + s)    # Vec2 component-wise hit-test
      if hover
        selected = i
        if Mouse_LeftPressed
          clicked = i

      var c (Vec3 0.8)
      if hover
        c = Colors:Primary                # blue on hover
      if (clicked == i)
        c = (Vec3 1.0 1.0 0.4)            # yellow on clicked

      DrawPanel p s 0.04 1 1 0
      DrawText Fonts:Default name (p + (Vec2 12 6)) 18 c 1 false
      y = y + 28.0

    DrawText Fonts:Heading "LIMIT THEORY - LAUNCHER" (Vec2 64 32) 28 (Vec3 0.6 0.8 1.0) 1 false
    Draw (Glyph_Circle 0 1 1 1) Mouse_GetPosImmediate 32 Colors:Primary 1.0

type App
  Interface ui
  Interface gameView
  Camera camera

  function Void Initialize ()
    camera = Camera_Create
    camera.Push
    ui = (Interface_Create "UI")
    gameView = (Interface_Create "Game View")

    var passes Vector<Reference<RenderPassT>>
    passes.Append (RenderPass_Clear (Vec4 0.0))
    passes.Append (RenderPass_Camera camera)
    passes.Append (RenderPass_Interface ui)
    passes.Append (RenderPass_PostFilter "post/dither.jsl")
    gameView.Add (Widget_Rendered passes)

    ui.Add (Custom Widget LauncherScreen)

  function Void Update ()
    ui.Update
    gameView.Update
    gameView.Draw

function App Main ()
  var self App
  self
```

### 14.2 Reading a directory (`Directory_List`)

`Directory_List "relative/path/"` returns a `Vector<String>` of filenames
(relative to the project working directory). Read it with `.Size` and `.Get i`:

```lts
var files (Directory_List "resource/script/App/")
for i 0 i < files.Size i.++
  var name (files.Get i)        # CORRECT: `var` (value)
  # ref name (files.Get i)      # WRONG: `.Get` returns a value here
```

### 14.3 "Launching" another app — why it does nothing

**LTSL has no script-level API to start a different `.lts` app.** The engine
launches exactly one app by name (`launch <app>`) and scripts cannot spawn
another. So in `launcher.lts`, clicking a row can only *select/highlight* it
(turns yellow) — there is intentionally no launch code, because none can exist
in LTSL today. Hover→blue and click→yellow are the intended (and only possible)
behaviors.

If you want a click to *do* something, the realistic options are:
- **Script-side (possible today):** show a "selected: <name>" readout, or wire
  an `Exit` on a chosen entry (using the existing `Exit` global ScriptAPI fn).
- **C++ side (would be a real feature):** expose a ScriptAPI function such as
  `Program_Launch(name)` in `src/liblt/LTE/ScriptAPI/` that tells the `Launcher`
  to swap apps. That is an engine change, not a script change.

---

## 13.4 App status — what runs and how it was fixed

Every app under `resource/script/App/` is now a **driven** app (`Main -> App`
+ `Initialize`/`Update`) that renders through the `Widget_Rendered` /
`RenderPass` pipeline. The following were converted from the old
`widget:Create Layer ...` self-widget shape or had their render path fixed:

| App | Status | Notes / fix |
|---|---|---|
| `war` | runs | Canonical game app; already driven (reference). |
| `dogfight` | runs | Already driven. |
| `ltheory-main` | runs | Revamp Work sandbox; already driven (see AGENTS §7b). |
| `launcher` | runs | Pure-2D driven UI; reference for §14. |
| `threads` | runs | Converted from self-widget. `Thread_Create`/`GetResult` were crashing with an Access Violation — root cause was a **missing memory barrier** (the worker published `returnValue` before the non-atomic `finished` flag), not a worker-thread engine-state violation. Fixed in `LTE/Thread.cpp` (`std::atomic<bool> finished` with release/acquire ordering) and `LTE/ScriptAPI/Thread.cpp` (`Thread_GetResult` joins via `Wait()` before reading, plus a typed `Thread_GetResultInt`/`GetResultInt` accessor). `threads.lts` now spins up `MyJob.Main` on a real background thread and displays `49995000` (sum 0..9999). See AGENTS §8d #2. |
| `colony` | runs | Converted from self-widget. Builds `Object_Universe 42 0`, walks to the first `IsSystem`, and shows its `GetName`. The universe build runs inside `ColonyScreen:Create` (self-contained); custom fields are set **bare** inside the type's own methods (e.g. `systemName = ...`), not `self.systemName`. |
| `hnn` | runs | Converted from self-widget. Harmonic Neural Network sim; all logic in `HNNScreen` (`CreateChildren`/`PreUpdate`/`PreDraw`); `self.Rebuild` is kept and works. |
| `ui` | runs | Converted from self-widget. UI showcase; `UIScreen` loads `Texture/RandomScreenshot:Get` (splash.png) and builds windows + a `TextEditor`. |
| `platemesh` | runs | Converted from self-widget. `PlateScreen` hosts `Widget/ModelEditor:Create`. |
| `hud` | runs | Converted from self-widget. Originally searched the universe for a player (which doesn't exist — stations/players aren't auto-generated; see AGENTS §8b.1); now creates the player explicitly (`Player_Human` + `Item_ShipType` ship + `player.Pilot ship`) inside `HUDApp:Create`, and sets `Camera_Get.SetTarget player.GetPiloting` in `PreUpdate` (the HUD requires a live camera target — without it the whole HUD is dropped and the screen is black). |
| `objectinfo` | runs | Converted from self-widget. Already had all build logic in `Create` (self-contained); just wrapped in the driven `App`. |
| `map` | runs | Was already driven but used the **old `Layer`/`Compositor_Basic` + `ui.Draw` render path, which hangs/doesn't render** — fixed by switching to the `Widget_Rendered`/`RenderPass` pipeline. It also originally did `Custom Widget MapWidget player` (invalid — fails to compile, leaving `ui` empty/black) and scanned the universe for a player that doesn't exist. Now builds the player + system inside `MapWidget:Create` (self-contained) and sets the camera target in `PreUpdate`. |
| `market` | runs | Same `Layer`→`RenderPass` and `Custom Widget … args` fixes as `map`; the player/station/market build moved into `MarketWidget:Create` and `station.Update` into `PreUpdate`. |

**Remaining compile errors in all apps:** there are now **none** — the
`Object/WarpNode.lts` / `Object/WarpRail.lts` `Position`-math failures were
fixed (see AGENTS §8d #1). Warp nodes/rails now instantiate and the upstream
`Object/System.lts` `Orbital rail` block compiles as-is. `rails.lts` is a good
smoke-test app that builds a chain of warp nodes + rails.

**Conversion rules that worked (proven across all the above):**
- The main screen widget is its own `type XScreen` (or keeps its existing
  name). Inside its own methods, set custom fields **bare** (`systemName = ...`),
  not `self.field` — `self.<customfield>` parses as a function call and fails.
  Built-in widget fields (`pos`, `size`, `Center`) are accessed via `self.`.
- Build the world/objects inside `XScreen:Create` (or `App::Initialize` for
  state stored on the App) rather than in a `widget:Create` block — there is no
  `widget:Create` anymore.
- If the screen needs data computed in `Initialize` (App scope), build it
  inside the screen's own `Create` instead, since you cannot write custom
  fields of a `Custom Widget` from outside (the widget binds as base `Widget`).
- Drive everything with `ui.Update; gameView.Update; gameView.Draw` in
  `App::Update`, and render through `RenderPass_Clear` → `RenderPass_Camera` →
  `RenderPass_Interface ui` → `RenderPass_PostFilter "post/dither.jsl"`.
- Do NOT use the `Layer`/`Compositor_Basic`/`Mesh_Quad`/`Stack` path inside a
  driven app — it hangs. (That path is only valid in true self-widget apps like
  `SplashScreen.lts`.)

---

## 15. Language internals (parser → AST → evaluation)

This section is reverse-engineered from the engine source
(`src/liblt/LTE/LTSL.cpp`, `Expression*.{h,cpp}`, `Script*.{h,cpp}`,
`Expression.h`, `ScriptFunction.*`, `Type.h`). It is the reference for *why*
LTSL behaves the way it does (the gotchas in §12 all trace back here).

### 15.1 Pipeline overview

```
 source .lts
   │
   ▼
 LTSL::Tokenize            -> token stream
   │
   ▼
 LTSL_ApplyRewrites        -> rewrites `.` member access + infix operators
                              into explicit prefix/call forms, applying the
                              operator-precedence table (LTSL.cpp:4-82)
   │
   ▼
 Expression_Compile       -> builds the AST (one Expression* node per construct)
   │                         keyword dispatch: if/for/while/switch/return/...
   ▼
 AST of Expression*        -> 25 node types under LTE/Expression/
   │
   ▼
 ScriptFunctionT::Call    -> evaluates the body with an Environment&
                              (tree-walking interpreter; no bytecode/VM/JIT)
```

The interpreter is **tree-walking**: every AST node has a virtual
`Evaluate(void* out, Environment& env)` (`Expression.h:23`). There is no
bytecode, no JIT, no VM register file — each call to `Evaluate` writes its
result into `out` and recurses into children. This is why deeply nested scripts
are O(depth) per evaluation and why the heavy work is on the C++ side
(`ScriptAPI` functions), not in LTSL loops.

### 15.2 `LTSL_ApplyRewrites` — the infix/dot transform

Before compilation, `.` access and infix operators (`+`, `*`, `==`, `<`, …)
are rewritten into explicit forms so the compiler sees a uniform prefix grammar:

- `a.b` → member/field access on `a` (looked up via the type system / `FIELDS`).
- `a op b` → `(op a b)` for the operators in the precedence table.
- Parentheses and function calls stay as-is.
- Method calls `obj.Method arg` become `Method obj arg` (message passing) once
  rewritten — which is why `self.field` *outside* a method parses as a call to a
  function named `field` (and fails if none exists). This is the root of the
  §12.2b / §13.4 "set custom fields bare" rule: inside a method `self` is the
  receiver, but inside the type's own body `field = ...` is a bare local/field
  assignment, and `self.field` would be a *call*.

### 15.3 The 25 expression-node types

The AST is built from concrete `Expression_*` classes in `src/liblt/LTE/Expression/`.
The catalog (filenames = node kind):

| Node | Role |
|---|---|
| `Expression` (base) | virtual `Evaluate(out, env)` |
| `Block` | sequence of sub-expressions; runs each, stops early on `env.returnSignal` |
| `Declare` | `var x expr` / typed `var x T` |
| `Assign` | `x = expr` (also `+=`, field assignment) |
| `Dereference` | `x.field` / `ptr->field` |
| `Reference` | taking a `ref` / address of a variable |
| `Constructor` | `Type args` value construction |
| `Conversion` | explicit/numeric conversion between types |
| `FunctionCall` / `ExpressionCall` | calling a `ScriptFunction` or free function |
| `MemberFunctionCall` | `obj.Method args` (dynamic dispatch) |
| `DynamicDispatch` | resolves the call against the receiver's type |
| `If` | `if pred cons alt` |
| `Switch` | `switch / case / otherwise` (benign "did not compile" logs, see §12.17) |
| `For` | `for i start pred incr body` |
| `While` | `while pred body` |
| `Return` | `return expr` / bare `return` (Revamp Work, §12.16) |
| `Literal` | numeric / string / bool literal |
| `Variable` | a named local/parameter reference |
| `Type` | a type reference (e.g. in `var x T`) |
| `Cast` | type cast node |
| `Lambda` / `Closure` | function-valued expressions (`Data`/`onPress`) |
| `List` | `l += ...` list construction/append |
| `BinaryOp` / `UnaryOp` | arithmetic/logic primitives post-rewrite |
| `StringOp` | string concat (`+` on strings) |
| `Message` | a `Data` message value (`MessageBuy`, `SelectItem`, …) |

### 15.4 Dispatch & the type system

- **C++ functions** are bound through the `Function_Generated.h` macro family
  (`FreeFunctionN` / `MemberFunctionN` / `VoidFreeFunctionN`, ~894 bind sites in
  `ScriptAPI/`). At compile time `Expression_Compile` resolves a call name to a
  `Function` by argument types; if no exact match, it tries registered
  **conversions** (`DefineConversion`) and **operator overloads**.
- **`FunctionAlias(RNG_Int, Int)`** is why `(Int x)` is *not* a cast — it is the
  `RNG_Int` function (random int). See §12 and AGENTS §8d #2. Do not write
  `(Int result)` expecting a numeric cast of a `Data`/thread result; use a typed
  accessor like `Thread_GetResultInt`.
- **`Type_Get<T>()` / `Type_Ref<T>()`** drive reflection. Reflected types
  register a `Type` via a friend `_Type_Get(T const&)` (ADL) hooked from
  `REGISTER_TYPE` / `DeclareMetadata` / `DefineMetadata`. Abstract `RefCounted`
  bases used only through `Reference<X>` (`RNGT`, `InterfaceT`, `FontT`) must be
  reflected too, or `Type_Get<X>()` falls through to the generic `"unknown
  type"` fallback (null function pointers) and conversions crash. See AGENTS
  §7 / §8d #13.
- **Dynamic dispatch** for `obj.Method` walks the receiver's type metadata to
  find the member function; this is how widget hook methods (`Create`,
  `PreUpdate`, …) get called by the engine.

### 15.5 The `Environment`

`Environment` carries per-call state: local variables, the `returnSignal` bool,
and the `returnValue` void* slot (canonical output of the innermost function).
`ExpressionReturn::Evaluate` sets `env.returnValue` + `env.returnSignal`;
`ExpressionBlock::Evaluate` checks `returnSignal` after each sub-expression and
stops; `ScriptFunctionT::Call` and `ExpressionExpressionCall::Evaluate` reset
`returnSignal` to false after the body finishes so a `return` cannot leak into
the caller. (This is how early-exit works without exceptions — the engine is
`-fno-exceptions`.)

### 15.6 Known semantic rules (quick ref)

- **Functions return their last expression** (legacy). `return` is now available
  (§12.16) but is optional.
- **`switch` benign logs** are gated behind `env.detail` — they only print during
  the error-replay pass, never during normal runs (§12.17).
- **Operator resolution is type-driven**, not overload-by-name; adding operators
  for a new type (like `Position`) must be done via **late registration at
  `LTE_Initialize`**, never at static-init, or it corrupts the global type
  registry (the `Position`/`WarpNode` SIOF, §12.18, AGENTS §8d #1). The pragmatic
  fix that landed: `Position` is an **alias** of the already-populated `Vec3d`
  type (so it inherits all `Vec3d` operators) bound lazily via a dll-side
  `__attribute__((constructor))` + `Type_AddAlias(Type_Get<V3D>(), "Position")`.
  Do **not** add mixed-precision *operator* overloads (e.g. `Vec3d + V3F` aliased
  `+`) — they create ambiguous overload resolution that breaks single-precision
  ship physics (NaN → `force.IsFinite()` abort in `Motion.cpp`). Prefer the
  existing `V3F -> V3D` conversion for mixed `Position + V3` arithmetic.
- **No `return` keyword originally** — added as Revamp Work; do not rely on it in
  scripts you want to run on the unmodified upstream engine.

---

## 13. Where to look next (source map)

| I want to… | Read |
|---|---|
| A complete working game app | `App/war.lts`, `App/dogfight.lts` |
| Loading screen + game swap | `App/ltheory-main.lts` |
| A minimal pure-2D UI driven app | `App/launcher.lts` (see §14) |
| A menu widget | `Widget/GameMenu.lts`, `Widget/SplashScreen.lts` |
| Buttons / callbacks | `Widget/Button.lts`, `Widget/ExitButton.lts`, `Widget/IconButton.lts` |
| HUD | `Widget/HUD.lts` |
| Layout helpers | `Widget/Widgets.lts` |
| Decorators | `Widget/Components.lts` |
| Messages / `onPress` data | `Widget/Messages.lts` |
| Drawing reference | `Widget/Text.lts`, `Widget/Icon.lts`, `Icons.lts`, `Fonts.lts` |
| Cursors / mouse input | `App/launcher.lts` §14, `Mouse.cpp`, `Cursor.cpp`, `Widget/Button.lts` (`CaptureFocus`) |
| Render pipeline / clear | `App/war.lts` (passes), `src/liblt/UI/Interface.cpp`, `RenderPass*.cpp` |

---

## 16. Old Limit Theory Code resurrected

In the code base there is a './src/old/' folder and I wanted to find out was it useful, can I interact with it. Still need to research the area and try to get some of the code running, but I was able to get 'ltheory-app.cpp' running. And I still need to analyze this but this is very promising and might tell me how to create a universe that are populated with many sectors, planets, npcs, stations. The existing apps do not show you how to do this or demonstrate this. And I will try to take this knowledge and build an app to mimic this functionality and most importantly get it documented at least a little.

**NOTE**: I am not sure if I will keep this in the code, I may extract all the knowledge from what is in these folders and then remove them. This 'old' code is not called anywhere and I had to get it added to build it.

``` python
# Run all of these from the project root

# This is probably one of the early demo universes Josh used in his demos
# It has wormholes, several stations, planet, effects, npc ships
# Still need to figure out how to any features in the demo (like docking, traveling the wormholes)

# This demo takes about 20-30s to load
LD_LIBRARY_PATH=bin:extbin/linux64 ./bin/ltheory_app

# This is a audio demo, it launches have to figure out how to use/hear output
# Possible usage: keys 1-7/Q-U/A-J/Z-M play notes, S writes data.wav; 
# or ./bin/audio_demo 45 renders one note headless to data.wav
LD_LIBRARY_PATH=bin:extbin/linux64 ./bin/audio_demo
```
