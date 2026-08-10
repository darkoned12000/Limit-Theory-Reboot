---
name: ltheory
description: >-
  Master reference for the Limit Theory engine (ltheory-old-test) and LTSL
  scripting. Load when working on or answering questions about engine
  subsystems (LTE, rendering, shaders, physics, audio, UI, procedural
  universe generation), writing or debugging LTSL scripts, the LSP tooling,
  or the game vision/PRD. Also references AGENTS.md, the LSP guide,
  ltsl-docs, and the PRD.
---

# SKILL.md - Limit Theory Engine & LTSL Scripting

**Purpose:** Master reference for AI agents helping with Limit Theory engine development and gameplay scripting.  
**Version:** 2.2.0 (current: quick save/load + toasts, Config.lts extraction, 492 checks)  
**Last Updated:** 2026-08-10

---

## 0. Game Vision & Theme

### What is Limit Theory?

**"A limitless procedural universe where every star, planet, ship, and story is generated from a seed — and every player's journey is unique."**

**Core Concept:**
- **100% procedural game** — no pre-made art assets, everything runtime-generated from algorithms
- **Single-player space simulation** — trade, combat, mine, explore, build empire
- **Infinite variety** — entire galaxies generated from single seed value
- **Smallest game in genre** — ~500MB file size (vs 150GB for Starfield)
- **Open-source engine** — community can mod/extend everything

**Gameplay Pillars:**
1. **Space Combat** — Dogfighting with procedurally-generated ships
2. **Trading Economy** — Buy low, sell high across star systems
3. **Mining & Resources** — Extract ore from asteroids
4. **Exploration** — Discover new systems, planets, phenomena
5. **Empire Building** — Own stations, manage fleets, control territory

**Technical Philosophy:**
- **Algorithmic art** — Ships/stations/planets = procedural geometry + shaders
- **Data-driven design** — JSON configs for balance, not hardcoded values
- **Moddable everything** — Scripts, shaders, configs all open to modding

---

## 1. Quick Reference - Most Used APIs

### LTSL Object Creation (Real Examples from war.lts)
```lts
# Create star system (from war.lts)
var system (Object_System (Vec3 15.012) 33)   # Position + seed → star + nebula + starfield

# Create player ship (from war.lts)
var shipType (Item_ShipType 10000 20 1 1 1 1 1 1)  # seed value hp thrust mass...
var ship shipType.Instantiate                 # Create ship from type
ship.SetPos kOrigin                           # Set position (Vec3)
system.AddInterior ship                       # Add to system hierarchy

# Add weapons (from war.lts)
var weaponType (Item_WeaponType 54)          # Weapon seed
for i 0 i < 4 i.++                           # Loop 4 times
  ship.Plug weaponType                        # Attach weapon to hardpoint

# Create AI ships (from war.lts)
for i 0 i < ShipCount i.++                   # ShipCount defined at top
  var ship type1.Instantiate
  ship.SetPos 5000.0 * rng.Direction         # Random position on sphere
  system.AddInterior ship
  ship.PushTask (Task_Destroy targetShip)    # Give AI task

# Create asteroid with mining (from ltheory-main.lts)
var asteroid (Object_Asteroid (Item_AsteroidType seed) 150.0)  # Type + scale
asteroid.SetPos (planetRadius * 1.6 * rng.Direction)  # Random position
asteroid.SetMineable item quantity             # Make mineable
root.AddInterior asteroid

# Create planet (from ltheory-main.lts)
var planet (Object_Planet (Item_PlanetType (rng.Int + 8)))
planet.SetPos (orbitalR * rng.Direction)       # Real 3D orbit (not Vec3_Cylinder!)
planet.SetName (Grammar_Get "$system")         # Procedural name
root.AddInterior planet
```

### LTSL Variables & Control Flow (Real Patterns)
```lts
# Variables (from war.lts)
var kOrigin (Vec3d 0 0 0)                     # Constant-style naming with 'k' prefix
var rng (RNG_MTG 0)                           # Create RNG (Mersenne Twister)
static paused false                           # Static variable (persists across calls)

# Complex expressions ALWAYS need parens
var choice (rng.Int 1 4)                      # Call with args → parens required
var outputMult (self.GetSpeed / 1000.0)      # Math expression → parens required

# Conditionals (from war.lts)
if Key_T.Pressed                              # Simple condition, no parens
  Shader_RecompileAll

var ship                                      # Declare variable
  switch                                      # Switch returns value!
    choice == 1 type1.Instantiate
    choice == 2 type2.Instantiate
    choice == 3 type3.Instantiate
    otherwise   type4.Instantiate

# Loops (from war.lts)
for i 0 i < ShipCount i.++                    # C-style for loop with i.++
  var ship type1.Instantiate
  # ... body

for it (cargo) it.HasMore it.Advance {        # Iterator pattern (verbose!)
  var item (it.GetItem)
  var quantity (it.GetQuantity)
}

# Speed modifier pattern (from war.lts)
var dt FrameTimer_Get
dt = (Min dt 0.1)                             # Clamp max delta time
if Key_H.Down dt *= 20.0                      # Speed up time with 'H' key

if paused.!                                   # .! = boolean NOT operator
  system.Update dt
```

### LTSL Math & Vectors
```lts
# Vec3 (float precision)
var v (Vec3 x y z)                            # Create vector
v.x, v.y, v.z                                 # Components
v.Length                                      # Magnitude
Vec3_Length v                                 # Same as v.Length
Vec3_Normalize v                              # Unit vector
Vec3_Distance a b                             # Distance between points
v1 + v2, v1 - v2, v * scalar                  # Operators work!

# Position (V3D - double precision, used for world coords)
var pos (Position x y z)                      # Create position
# ⚠️ WarpNode requires Position, not Vec3!

# Common functions
Abs x                                         # Absolute value
Min a b, Max a b                              # Min/max
Clamp x min max                               # Clamp value
Sqrt x, Pow x exp                             # Math
Sin x, Cos x, Tan x                           # Trig (radians)
Rand, Rand min max                            # Random [0,1] or [min,max]
```

### LTSL Components (ECS)
```lts
# Cargo (FULLY IMPLEMENTED!)
ship.AddItem item quantity                    # Returns bool (false if full)
ship.RemoveItem item quantity                 # Returns bool
ship.GetItemCount item                        # Get quantity of item
ship.GetCapacity                              # Total cargo space
ship.GetUsedCapacity                          # Current usage
for it (ship.GetCargo) it.HasMore it.Advance { # Iterate cargo
  var item (it.GetItem)
  var qty (it.GetQuantity)
}

# Physics
obj.SetPos position                           # Set position (Vec3 or Position)
obj.GetPos                                    # Get position
obj.SetVel velocity                           # Set velocity (Vec3)
obj.GetVel                                    # Get velocity
obj.SetMass mass                              # Set mass
```

# Widget composition is like building a tree (from Button.lts)
```lts
function Widget Create (Data onPress String text Float size)
  Components:CaptureMouse                     # Outer wrapper (captures mouse)
    Components:Padding (Vec2 6 6) 6           # Add padding around
      Stack                                   # Stack layers (z-order)
        Custom Widget                         # Custom widget type
          Button onPress text size            # Your widget instance

# Custom widget structure (from Button.lts)
type Button
  Data onPress                                # Callback data
  String text                                 # Button text
  Float size                                  # Font size
  Bool focus false                            # Focus state

  function List CreateChildren (Widget self) {  # Build child widgets
    var l List
    l += Components:AlignCenter               # Add centered text
      Custom Widget
        Text:Text Fonts:Subheading text size (Vec3 1)
    l                                         # Return list
  }

  function Void PreDraw (Widget self) {       # Draw background
    var c 
      switch
        focus 2.5 * Colors:Primary            # Bright when focused
        otherwise (Vec3 0.15)                 # Dark otherwise
    DrawPanel self.pos self.size c 0.5 1 0   # pos size color innerAlpha outerAlpha bevel
  }

  function Void PostUpdate (Widget self) {    # Handle interactions
    if self.focusMouse && (! focus)           # Mouse enter
      focus = true
      self.Rebuild                            # Rebuild to change color
      Sound_Play "ui/hover.wav" 0.008         # Play hover sound

    if self.focusMouse                        # If mouse over
      if Mouse_LeftPressed                    # And clicked
        self.SendUp onPress                   # Send message up tree
  }

# Real HUD from war.lts
ui.Add (Widget/HUD:Create player)             # Add HUD widget to interf
Widgets:Text text size color                  # Text label
Widgets:Button text onPress                   # Clickable button
Widgets:Icon icon size color                  # Icon graphic
Widgets:Spacer size                           # Empty space

# Layout containers
Widgets:Column list                           # Stack vertically
Widgets:Row list                              # Stack horizontally
Widgets:Stack list                            # Overlay (z-order)

# Components (wrap other widgets)
Components:AlignCenter widget                 # Center in parent
Components:Padding inner outer widget         # Add padding
Components:Backdrop color alpha widget        # Background panel
Components:CaptureMouse widget                # Capture mouse events
Components:Expand widget                      # Fill parent space
```

### LTSL String Operations
```lts
# String operations
var str "Hello"                               # String literal
var combined ("Hello " name)                  # Juxtaposition (NOT +)
String_Length str                             # Length
String_Substring str start count             # Extract substring
str.UpperCase                                 # Convert to uppercase

# ⚠️ LTSL has NO + for string concatenation!
# ❌ Wrong: "Hello " + name
# ✅ Right: ("Hello " name)
```

### LTSL Audio (Real Patterns from Ship.lts)
```lts
# From Object/Ship.lts - Ship audio script
type ShipUpdate
  Sound interior Sound                        # Interior ambiance sound
  Sound engine Sound                          # Engine loop sound

  function Bool Update (Object self) {
    # Play interior ambiance (lazy init pattern)
    if interior.IsNull
      interior =
        Sound_PlayLooped
          "ship/ambiance/interior/1.wav"      # Asset path
          self                                # Source object (3D position)
          0                                   # Channel (0 = auto)
          0.15                                # Volume
          1.0                                 # Pitch
      interior.RandomizePosition              # Randomize start position in loop

    # Play engine loop (lazy init pattern)
    if engine.IsNull
      engine =
        Sound_PlayLooped
          "thruster/loop1.ogg"
          self
          0
          0.0                                 # Start at volume 0
          Sqrt self.GetRadius                 # Pitch based on size
      engine.RandomizePosition

    # Dynamic volume/pitch based on speed
    var outputMult (self.GetSpeed / 1000.0)
    engine.SetVolume 0.5 * (Saturate outputMult)  # Louder when moving
    engine.SetPitch 1.0
    interior.SetPitch (1.0 + outputMult)      # Higher pitch when moving
    true                                      # Return true = keep updating
  }

# Attach audio script to ship
function Void Init (Object self)
  self.AddScript ShipUpdate                   # Scripts run every frame

# One-shot sounds (from Button.lts)
Sound_Play "ui/hover.wav" 0.008               # 2D UI sound
Sound_Play "ui/click.wav" 0.5                 # Higher volume

# 3D positioned sound (hypothetical combat)
var pos (ship.GetPos)
var vel (ship.GetVel)
Sound_Play3D "explosion/boom.wav" pos vel 1.0 # Position + velocity for doppler
```

---

## 1.5 Shader Usage Patterns (GLSL 4.60 Core)

### Shader Basics

**All shaders are `.jsl` files** (GLSL with JSL preprocessor):
- **Location:** `resource/shader/{vertex,fragment}/name.jsl`
- **Version:** GLSL 4.60 core (force-prefixed by engine)
- **Preprocessor:** `#include` and `#output` directives

**Engine automatically provides these uniforms:**
```glsl
uniform mat4 mWorld;      // World matrix
uniform mat4 mView;       // View matrix  
uniform mat4 mProj;       // Projection matrix
uniform mat4 mWVP;        // World * View * Projection
uniform mat4 mWorldIT;    // World inverse-transpose (for normals)
uniform vec3 eye;         // Camera position
uniform float t;          // Time (seconds)
```

### Real Shader Example: Beam Effect (beam.jsl)

**Vertex shader:** `resource/shader/vertex/npm.jsl` (Normal-Position-Material)
```glsl
#version 460 core
layout(location=13) VERT_OUT vec3 position;
layout(location=15) VERT_OUT vec3 normal;

void main() {
  position = (mWorld * vec4(vPosition, 1.0)).xyz;
  normal = normalize((mWorldIT * vec4(vNormal, 0.0)).xyz);
  gl_Position = mWVP * vec4(vPosition, 1.0);
}
```

**Fragment shader:** `resource/shader/fragment/beam.jsl`
```glsl
#include frag.jsl      // Common fragment functions
#include math.jsl      // Math utilities
#include noise.jsl     // Noise functions

layout(location=13) FRAG_IN vec3 position;
layout(location=15) FRAG_IN vec3 normal;

uniform vec3 baseColor;
uniform vec2 size;
uniform float thinness;

void main() {
  float u = uv.x;      // uv provided by frag.jsl
  float v = uv.y;

  // Gaussian falloff from center
  float alpha = 0.25 * exp(-abs(u) * thinness);

  // Pulse animation along beam
  const float pulseFactor = 0.3;
  alpha *= (1.0 - pulseFactor) + pulseFactor * sin(v - 15.0 * t);

  // Add glow layers
  alpha += 0.25 * exp(-abs(u) * thinness / 4.0);
  alpha += 0.125 * exp(-abs(u) * thinness / 8.0);

  // Head fade (fade in at start)
  alpha *= (1.0 - exp(-5.0 * v));

  // Tail fade (fade out at end)
  alpha *= (1.0 - exp(40.0 * (v - 1.0)));

  vec3 color = baseColor;
  color += vec3(1.0) * exp(-abs(u * 4.0));  // Hot center

  RETURN(vec4(color, 1.0) * alpha);  // RETURN macro from frag.jsl
}
```

### Shader Assignment in LTSL (from war.lts)

```lts
# Create object with custom shader
var beam (Object_Create "MiningBeam")
beam.SetMesh (Generator_Beam startPos endPos 2.0)    # Generate beam mesh
beam.SetShader (Shader "identity.jsl" "beam.jsl")    # Vertex + fragment
beam.SetColor (Vec3 0.2 1.0 0.2)                     # Green tint

# Post-processing (from war.lts)
var passes Vector<Reference<RenderPassT>>
passes.Append (RenderPass_Clear (Vec4 0.0))           # Clear to black
passes.Append (RenderPass_Camera camera)              # Render scene
passes.Append (RenderPass_SMAA)                       # Anti-aliasing
passes.Append (RenderPass_Interface ui)               # Draw UI
passes.Append (RenderPass_PostFilter "post/dither.jsl")  # Dither post-effect
gameView.Add (Widget_Rendered passes)                 # Final render target
```

### Available Shader Pairs

**Common combinations:**
- `identity.jsl` + `solidcolor.jsl` — Flat colored mesh
- `npm.jsl` + `default.jsl` — Lit mesh with normals
- `particle.jsl` + `particle_radial.jsl` — Radial particle effect
- `imposter.jsl` + `imposter1.jsl` — Billboard sprites
- `ui.jsl` + `ui/*.jsl` — UI rendering

**Post-effects** (fragment only, applied to full screen):
- `post/bloom.jsl` — HDR bloom
- `post/ssao.jsl` — Screen-space ambient occlusion
- `post/motionblur.jsl` — Motion blur
- `post/dither.jsl` — Dithering (reduces banding)
- `post/lensflare.jsl` — Lens flare

---

## 1.6 Galaxy Map Implementation Guide

### Running the Map App

```bash
python configure.py run map
```

**What it does:**
- Creates test star system with procedural objects
- Shows 2D top-down map view (X-Z plane, Y is vertical in 3D)
- Objects rendered as icons with dynamic sizing based on zoom
- WASD to pan, mouse scroll to zoom, double-click object to follow

### Map Architecture

**Three-layer structure:**
1. **MapApp** (App/map.lts) — App shell with render passes
2. **MapWidget** (App/map.lts) — Creates test world + player
3. **Map + MapObjects** (Widget/Map.lts) — Actual map visualization

```lts
# Map widget structure (Widget/Map.lts)
type MapObjects
  Player player
  Object container          # System to visualize
  Object target Object      # Focused object (double-click)
  Vec2 center 0             # Current pan position (smooth)
  Vec2 centerTarget 0       # Target pan position (WASD sets this)
  Float zoom 1000000        # Current zoom level
  Float zoomTarget 1000000  # Target zoom level (scroll sets this)

  function List CreateChildren (Widget self) {
    # Iterate all interior objects in system
    for it container.GetInteriorObjects it.HasMore it.Advance
      var object it.Get
      # Filter: only show navigable/pilotable/resources/interior objects
      if object.HasComponentInterior ||
         object.HasComponentNavigable ||
         object.HasComponentPilotable ||
         object.HasComponentResources ||
         object.IsCustom
        l += (HUD/WorldObject:Create player object)  # Create icon widget
        
        # Show planet children (moons)
        if object.IsPlanet
          for it object.GetChildren it.HasMore it.Advance
            l += (HUD/WorldObject:Create player it.Get)
    l
  }

  function Void PostPosition (Widget self) {
    # Position child widgets based on 3D world coords
    for it self.GetChildren it.HasMore it.Advance
      var widget it.Get
      var object (Messages:GetObject widget)
      var pos object.GetPos
      
      # Project 3D position to 2D map (X-Z plane)
      var ss ((Vec2 pos.x pos.z) - center) / zoom
      ss /= (self.size / (Min self.size.x self.size.y))  # Aspect correction
      ss = self.Center + 0.5 * self.size * ss
      
      # Size objects based on scale + zoom
      var size object.GetScale.Length * (10000.0 / zoom)
      size = (Clamp size 12.0 64.0)  # Min 12px, max 64px
      
      widget.pos = ss - size / 2.0
      widget.size = size
  }

  function Void PostUpdate (Widget self) {
    var dt FrameTimer_Get
    var mult 4.0 * zoom * dt
    
    # WASD panning (moves centerTarget)
    if Key_W.Down centerTarget.y -= mult  # Up (negative Z)
    if Key_S.Down centerTarget.y += mult  # Down (positive Z)
    if Key_A.Down centerTarget.x -= mult  # Left (negative X)
    if Key_D.Down centerTarget.x += mult  # Right (positive X)

    # Double-click to follow object
    if Mouse_DoubleClicked
      for it self.GetChildren it.HasMore it.Advance
        var widget it.Get
        if widget.focusMouse
          target = (Messages:GetObject widget)  # Set follow target

    # Auto-follow target (if set)
    if target.IsNotNull
      var pos target.GetPos
      centerTarget = (Vec2 pos.x pos.z)

    # Mouse scroll to zoom
    zoomTarget *= (1.0 - 0.1 * Mouse_GetScrollDelta)

    # Smooth interpolation (exponential decay)
    zoom = (Mix zoom zoomTarget 1.0 - (ExpDecay dt 0.125))
    center = (Mix center centerTarget 1.0 - (ExpDecay dt 0.125))
  }
}
```

### How to Add Map to Your App

```lts
# In your HUD (Widget/HUD.lts pattern)
type HUD
  Player player
  
  function List CreateChildren (Widget self) {
    var l List
    # ... other HUD elements
    
    # Add minimap (top-right corner)
    l +=
      Components:AlignTopRight
        Components:Sizing (Vec2 256 256) (Vec2 256 256)
          HUD/Minimap:Create player
    l
  }
}

# In your app's Update() - toggle fullscreen map with hotkey
function Void Update () {
  static mapOpen false
  
  if Key_M.Pressed                    # Press M to toggle map
    mapOpen = mapOpen.!
    if mapOpen
      ui.Add                          # Add fullscreen map overlay
        Widget/Map:Create player system
    else
      # Remove map (need to track widget reference)
  
  # ... rest of update
}
```

### Common Map Issues & Fixes

**Issue 1: "Objects not showing on map"**
- **Cause:** Objects missing required components
- **Fix:** Objects need one of: `HasComponentNavigable`, `HasComponentPilotable`, `HasComponentResources`, or `HasComponentInterior`

**Issue 2: "Map is blank/empty"**
- **Cause:** Container has no interior objects
- **Fix:** Ensure system populated with `system.AddInterior object`

**Issue 3: "Can't pan/zoom"**
- **Cause:** Widget not capturing input
- **Fix:** Wrap Map in `Components:CaptureMouse` or ensure it's top-level

**Issue 4: "Object positions wrong"**
- **Cause:** Map uses X-Z plane projection (Y is vertical)
- **Fix:** Check `Object.GetPos` has correct X/Z values (Y ignored)

**Issue 5: "Map crashes on startup"**
- **Cause:** Missing HUD/WorldObject widget
- **Fix:** Ensure `resource/script/Widget/HUD/WorldObject.lts` exists

### Creating Custom Map Visualizations

```lts
# Example: Distance rings around player
function Void PreDraw (Widget self)
  var playerPos player.GetPiloting.GetPos
  var center2D (Vec2 playerPos.x playerPos.z)
  
  # Draw grid
  var c center / 5000.0
  var s zoom / 1000000.0
  Draw (Glyph_Grid self.TopLeft self.BottomRight 1 1 c s) 0 1 Colors:Primary 0.1
  
  # Draw range rings (1000, 5000, 10000 units)
  for i 1 i <= 3 i.++
    var radius i * 1000.0
    var screenRadius radius / zoom * (Min self.size.x self.size.y)
    Draw (Glyph_Circle self.Center screenRadius) 0 1 Colors:Primary 0.2
```

---

## 2. Real App Patterns & Structure

### Driven App Pattern (THE RIGHT WAY - from war.lts)

```lts
# Top of file: Configuration
function Int ShipCount () 32                  # Constant-style function

type App
  Object system                               # Game world root
  Player player                               # Player reference
  Interface ui                                # UI layer
  Interface gameView                          # Game view layer
  Camera camera                               # Main camera

  function Void Initialize () {
    var kOrigin (Vec3d 0 0 0)                # 'k' prefix = constant
    var rng (RNG_MTG 0)                       # Seeded RNG

    # 1. Create camera
    camera = Camera_Create
    camera.Push                               # Make active camera

    # 2. Create interfaces
    ui = (Interface_Create "UI")
    gameView = (Interface_Create "Game View")

    # 3. Set up render passes (post-processing pipeline)
    var passes Vector<Reference<RenderPassT>>
    passes.Append (RenderPass_Clear (Vec4 0.0))
    passes.Append (RenderPass_Camera camera)
    passes.Append (RenderPass_SMAA)           # Anti-aliasing
    passes.Append (RenderPass_Interface ui)
    passes.Append (RenderPass_PostFilter "post/dither.jsl")
    gameView.Add (Widget_Rendered passes)

    # 4. Create game world
    system = (Object_System (Vec3 15.012) 33)

    # 5. Create player ship
    var shipType (Item_ShipType 10000 20 1 1 1 1 1 1)
    var ship shipType.Instantiate
    ship.SetPos kOrigin
    system.AddInterior ship

    # 6. Create player controller
    player = Player_Human
    player.AddAsset ship
    player.Pilot ship

    # 7. Populate world with AI ships
    for i 0 i < ShipCount i.++
      var ship type1.Instantiate
      ship.SetPos 5000.0 * rng.Direction
      system.AddInterior ship
      # ... add weapons, tasks, etc.

    # 8. Add UI
    ui.Add (Widget/HUD:Create player)
  }

  function Void Update () {
    # Developer hotkeys
    if Key_T.Pressed
      Shader_RecompileAll                     # Reload shaders (hot reload!)

    # Update camera to follow player
    camera.SetTarget player.GetPiloting

    # Get delta time with clamping
    var dt FrameTimer_Get
    dt = (Min dt 0.1)                         # Prevent huge jumps

    # Speed modifier (for testing)
    if Key_H.Down dt *= 20.0                  # Hold H = 20x speed

    # Pause support
    static paused false
    if Key_P.Pressed paused = paused.!        # Toggle pause
    if paused.!                               # If not paused
      system.Update dt                        # Update game world

    # Update and draw interfaces
    ui.Update
    gameView.Update
    gameView.Draw                             # CRITICAL: gameView.Draw, not ui.Draw!
  }
}

function App Main () {                        # Entry point
  var self App                                # Create app instance
  self                                        # Return it
}
```

### Loading Screen Pattern (from ltheory-main.lts)

```lts
# Config file reader (reusable!)
function String Config_Get (String key)
  var text (File_Read "resource/script/gameConfig.txt")
  var lines (SplitLines text)
  var klen (Length key)
  for i 0 i < lines.Size i.++
    var line (lines.Get i)
    if (Substring line 0 1) != "#"            # Skip comments
      if (Contains line key)
        if (Contains line ":")
          var llen (Length line)
          var vstart (klen + 1)
          var vlen (llen - vstart)
          Substring line vstart vlen
  ""                                          # Return empty if not found

# Loading screen widget with animation
type LoadingScreen
  Float time 0
  (Array Node) nodes (Array Node)             # Circular nodes

  function Void Create (Widget self) {
    # Create circular buffer of nodes
    for i 0 i < 512 i.++
      var node (Node 0 0)
      nodes += node
  }

  function Void PostUpdate (Widget self) {
    # Wave propagation simulation (elastic nodes)
    var factor (1.0 - (ExpDecay FrameTimer_Get 1.0))
    for i 0 i < nodes.Size i.++
      ref node1 (nodes.Get i)                 # ref = reference (modifiable)
      ref node2 (nodes.Get (Mod i + 1 nodes.Size))
      var delta (node1.energy - node2.energy)
      node2.v += elasticity * factor * delta  # Transfer energy
      node1.v -= elasticity * factor * delta
    
    # Random impulses
    if Float_Random < 0.01                    # 1% chance per frame
      var base (Mod (Abs Int_Random) nodes.Size)
      # ... add energy spike
  }

  function Void PostDraw (Widget self) {
    # Draw circular visualization
    for i 0 i < nodes.Size i.++
      var t i / nodes.Size
      var angle 2Pi * t
      var dir (Vec2 angle.Cos angle.Sin)
      var p self.Center + dir * 128
      Draw point p 8 color 1.0                # Draw node
  }
}

# App with loading screen
type App
  Widget loadingScreen Widget
  Float loadStartTime 0

  function Void Initialize () {
    loadStartTime = Time_GetReal
    loadingScreen = Custom Widget (LoadingScreen)
    ui.Add loadingScreen

    # Do heavy initialization...
    # Create universe, populate systems, etc.
  }

  function Void Update () {
    var elapsed (Time_GetReal - loadStartTime)
    var LOAD_TIME (ToFloat (Config:Get "loadTime"))   # shared loader: Config.lts
    
    if elapsed < LOAD_TIME
      # Still loading, show screen
      loadingScreen.Update
      loadingScreen.Draw
    else
      # Loading done, switch to game
      ui.Remove loadingScreen
      # ... normal game loop
  }
}
```

### Common Implementation Patterns

**Pattern 1: Lazy Initialization**
```lts
type MySystem
  Sound ambientSound Sound

  function Bool Update (Object self) {
    if ambientSound.IsNull                    # Only create once
      ambientSound = Sound_PlayLooped "ambient.ogg" self 0 0.5 1.0
    # ... use ambientSound
    true
  }
```

**Pattern 2: Reference Modifier (ref keyword)**
```lts
var list (List)
for i 0 i < list.Size i.++
  ref item (list.Get i)                       # ref = can modify in-place
  item.value = item.value * 2                 # Modifies original
```

**Pattern 3: Static Variables (persist across calls)**
```lts
function Void ToggleDebug ()
  static debugMode false                      # Persists between calls
  debugMode = debugMode.!                     # Toggle
  Log ("Debug mode: " + debugMode)
```

**Pattern 4: Switch as Expression (returns value)**
```lts
var color
  switch shipType
    "Fighter" (Vec3 1 0 0)                    # Red
    "Freighter" (Vec3 0 1 0)                  # Green
    otherwise (Vec3 1 1 1)                    # White
```

**Pattern 5: Ternary Operator (? ... otherwise ...)**
```lts
var c (? (self.focusMouse (Vec3 0)) (otherwise (Vec3 1)))
# If focusMouse, return black, else white
```

---

### Key Directories
```
src/liblt/                    # Engine core (~60K LOC)
├── LTE/                      # Type system, LTSL interpreter, reflection
│   ├── Expression.cpp        # LTSL compiler dispatcher (26 expression types)
│   ├── Expression/           # 26 AST node types (If.cpp, For.cpp, etc.)
│   ├── Type.h                # Reflection system (Type_Get<T>())
│   └── Serializer.cpp        # Binary serialization (save/load)
├── Game/                     # Game objects, items, physics
│   ├── Object.cpp            # Core game object
│   ├── ScriptAPI/            # C++ → LTSL bindings
│   └── Action/               # Actions (Mine.cpp, etc.)
├── Component/                # ECS-style components
│   ├── Cargo.cpp             # ✅ Inventory system (FULLY IMPLEMENTED!)
│   ├── Collidable.cpp        # Physics collision
│   └── Drawable.cpp          # Rendering component
├── Module/                   # Engine subsystems
│   ├── SoundEngine.cpp       # FMOD audio wrapper
│   └── PhysicsEngine.cpp     # Physics simulation
├── UI/                       # UI rendering
│   └── Widget.cpp            # Widget system
└── Generator/                # Procedural generation
    ├── Asteroid.cpp          # Asteroid mesh generation
    └── Planet.cpp            # Planet surface generation

resource/script/              # LTSL gameplay code
├── App/                      # Game applications
│   ├── war.lts               # Combat sandbox (main working app)
│   ├── launcher.lts          # App launcher UI
│   └── ltheory-main.lts      # Universe sandbox (seed-driven)
├── Widget/                   # UI widgets (composable!)
│   ├── Button.lts            # Clickable button
│   ├── Window.lts            # Draggable window
│   ├── HUD.lts               # In-game HUD
│   └── Components.lts        # Widget decorators (Align, Padding, etc.)
├── Object/                   # Object factories
│   ├── Ship.lts              # Ship creation
│   └── SystemPopulate.lts    # Star system population
└── Fonts.lts                 # Font definitions

resource/shader/              # GLSL 4.60 core shaders (170 shaders!)
├── vertex/                   # Vertex shaders (.jsl)
└── fragment/                 # Fragment shaders (.jsl)
```

### Critical Files (Must Know!)
```
src/liblt/Component/Cargo.cpp           # ✅ Inventory system (complete!)
src/liblt/Game/Action/Mine.cpp          # ✅ Mining system (complete!)
src/liblt/LTE/Serializer.cpp            # ✅ Save/load infrastructure
src/liblt/Game/Widget/HUD.cpp           # Gamepad support (lines 227-281)
resource/script/App/war.lts             # Main working combat app
resource/script/Widget/HUD.lts          # Flight controls, UI overlay
resource/script/gameConfig.txt          # Game configuration (seed, etc.)
```

---

## 3. Known Issues & Critical Facts

### ✅ FULLY IMPLEMENTED (Don't Re-Implement!)

**Cargo/Inventory System** (`Component/Cargo.cpp`)
- Status: Production-ready, fully functional
- LTSL API: `ship.AddItem`, `ship.RemoveItem`, `ship.GetCargo`
- Missing: UI to expose to player (no 'I' hotkey)
- See: SAVE-LOAD-AND-INVENTORY.md Part 1

**Mining System** (`Game/Action/Mine.cpp`)
- Status: Working correctly, calls `ship.AddItem` automatically
- Missing: Not wired to player input (no 'M' key, no targeting)
- See: SAVE-LOAD-AND-INVENTORY.md Part 5

**Save/Load (JSON)** (`Game/SaveGameJSON.{h,cpp}` + `Game/SaveGame.cpp`)
- Status: Multi-slot JSON saves (`cache/saves/<slot>.json`, versioned schema +
  `dateCreated`), bindings `SaveGame_Create` (quicksave), `SaveGame_Load`
  (latest), `SaveGame_LoadSlot`, `SaveGame_ListSlots`. Covered by
  `TestSaveGameJSON.cpp` (7 tests).
- Wired into `ltheory-main` (2026-08-10): F6 quicksave / F7 quickload /
  launch auto-load, each with a two-tone config-driven toast.
- Missing: GameMenu "SAVE GAME" entry + save-browser widget (data already ready).

**Gamepad Support** (`Game/Widget/HUD.cpp`)
- Status: VERIFIED WORKING (lines 227-281)
- Supports: Analog sticks, buttons, triggers
- Missing: Rebinding UI
- See: ENGINE-STABILITY-AND-MODDING.md Part 4

**LTSL Language Server (LSP) for IntelliSense**
- Status: Complete and integrated for **ZED** (TypeScript server + ZED extension; engine-generated API DB feeds completions)
- Features: Autocomplete, hover tooltips, error checking, syntax highlighting, go-to-definition
- Implementation: TypeScript server + ZED extension adapter (`extensions/ltsl/`)
- See: AGENTS.md §6.2 + docs/LTSL-LSP-IMPLEMENTATION-GUIDE.md (setup, config, verification)

---

### ⚠️ NEEDS WIRING (Systems exist but not exposed)

**Inventory UI**
- Problem: No 'I' hotkey to open inventory panel
- Solution: Create `Widget/HUD/Inventory.lts` with press-I toggle
- Code provided in: SAVE-LOAD-AND-INVENTORY.md Part 4

**Mining Hotkey**
- Problem: `Action_Mine` exists but player never calls it
- Solution: Add 'M' hotkey + targeting system to HUD.lts
- Code provided in: SAVE-LOAD-AND-INVENTORY.md Part 5

**Save/Load UI (menu)**
- Status: Quick save/load wired (F6/F7 + launch auto-load, `Widget/Toast.lts`
  feedback) in `ltheory-main`.
- Remaining: main-menu "SAVE GAME" entry + save-browser widget + slot-naming
  dialog. `SaveGame_ListSlots`/`SaveGame_LoadSlot` provide the browser data.
- Trap: toast label/value text must be parallel `(Array String)` arrays — a
  cross-file `ToastLine` struct adds 8 analyzer warnings.

**Loot Drops**
- Problem: NPCs don't drop cargo on death
- Solution: Add OnDeath hook + cargo pod spawning
- Code provided in: SAVE-LOAD-AND-INVENTORY.md Part 5

---

### ❌ DO NOT DO (Known Bad Ideas)

**Vulkan Migration**
- Why: 6 months work, 10-20% gain, 0% player benefit
- Verdict: Stay with OpenGL 4.6
- See: VULKAN-AND-SPACE-PHENOMENA.md Part 1

**Replace Reference<T> with std::shared_ptr**
- Why: Breaks reflection system (serialization depends on it)
- Verdict: Keep Reference<T>, it's load-bearing
- See: AGENTS.md §9.2

**Convert C to C++**
- Why: It's already C++17! GitHub classifier is wrong
- Verdict: No conversion needed
- See: ENGINE-STABILITY-AND-MODDING.md Part 2

**Add C++ Exceptions**
- Why: Engine uses `-fno-exceptions` intentionally
- Verdict: Keep error codes, no try/catch/throw
- See: AGENTS.md §3

**Self-Widget Apps**
- Why: Deprecated pattern, most don't render correctly
- Verdict: Use driven app pattern (Initialize + Update)
- See: docs/ltsl-docs.md §0

---

### 🚧 DOCUMENTED BUT NOT STARTED

**PBR Rendering**
- Status: Designed with full shader code
- Effort: 2-3 weeks
- See: GRAPHICS-TECH.md Part 2

**Volumetric Nebula**
- Status: Designed with raymarch shader
- Effort: 1-2 weeks
- See: VULKAN-AND-SPACE-PHENOMENA.md Part 2

**Modding System**
- Status: Architecture complete (mod folders, JSON, hooks)
- Effort: 2-3 weeks
- See: ENGINE-STABILITY-AND-MODDING.md Part 5

**List Methods (.Filter, .Map, .Reduce)**
- Status: Designed with C++ implementation
- Effort: 3 days
- See: LTSL-ARCHITECTURE-AND-IMPROVEMENTS.md Part 4

---

## 4. Common LTSL Gotchas & Pitfalls

### String Concatenation (Most Common Mistake!)
```lts
# ❌ WRONG - LTSL has no + for strings!
var msg "Hello " + name

# ✅ RIGHT - Use juxtaposition (space = concat)
var msg ("Hello " name)

# ✅ Also works
var msg "Hello " name        # No parens if simple
```

### Position vs Vec3 (Type Confusion)
```lts
# Position = V3D (double precision, used for world coords)
var worldPos (Position x y z)

# Vec3 = V3F (float precision, used for directions)
var direction (Vec3 x y z)

# ⚠️ WarpNode requires Position, not Vec3!
# ❌ WRONG: node.SetPos (Vec3 1000 0 0)
# ✅ RIGHT: node.SetPos (Position 1000 0 0)
```

### Parentheses for Complex Expressions
```lts
# ❌ WRONG - Parser gets confused
var result 5 + 3 * 2

# ✅ RIGHT - Use parens for nested expressions
var result (5 + (3 * 2))

# Simple cases work without parens
var x 10
var y 20
var sum x + y              # OK
```

### Iterator Pattern Verbosity
```lts
# Current (verbose but works)
for it (cargo) it.HasMore it.Advance {
  var item (it.GetItem)
  var quantity (it.GetQuantity)
  # Use item, quantity
}

# ⚠️ Can't simplify to foreach yet (not implemented)
# See: LTSL-ARCHITECTURE-AND-IMPROVEMENTS.md Part 4 for future improvements
```

### Function Return Values
```lts
# Functions return their last expression automatically
function Int Add (Int a Int b) {
  a + b                    # Implicit return
}

# But explicit return works too (Revamp Work addition)
function Int Add (Int a Int b) {
  return a + b             # Explicit return
}
```

### Object Creation Timing
```lts
# ❌ WRONG - Create objects outside Initialize()
var ship (Object_Ship ...)    # Global scope = crash!

# ✅ RIGHT - Create in Initialize()
type App
  Object ship
  
  function Void Initialize () {
    ship = (Object_Ship ...)  # Created at runtime
  }
```

---

## 5. Build & Run Commands

### Configure & Build
```bash
# Configure CMake (first time or after CMakeLists.txt changes)
python configure.py

# Build (parallel compilation, ~10s incremental)
python configure.py build

# Clean build
python configure.py clean

# Full rebuild
python configure.py clean
python configure.py
python configure.py build
```

### Run Applications
```bash
# Combat sandbox (main working app)
python configure.py run war

# App launcher UI
python configure.py run launcher

# Universe sandbox (seed-driven)
python configure.py run ltheory-main

# All apps (13 total)
# war, dogfight, ltheory-main, launcher, threads, colony
# hnn, ui, platemesh, hud, objectinfo, map, market
```

### Testing
```bash
# Run all unit tests (492 checks)
python configure.py test

# Check for errors in specific file
# (Use VS Code "Problems" panel)
```

### Development Workflow
```bash
# 1. Edit code (.cpp, .lts, .jsl)
# 2. Build
python configure.py build

# 3. Run & test
python configure.py run war

# 4. Check logs in terminal
# 5. Repeat
```

---

## 6. Common Questions & Answers

### Q: How do I create a new LTSL app?
**A:** Create `resource/script/App/myapp.lts` with driven pattern:
```lts
type App
  Interface ui
  Object root
  
  function Void Initialize () {
    # Setup world
    root = Object_Create "Root"
    ui = Interface_Create "UI"
  }
  
  function Void Update () {
    # Update every frame
    var dt FrameTimer_Get
    root.Update dt
    ui.Update
    ui.Draw
  }

function App Main () {
  var self App
  self
}
```

Then run: `python configure.py run myapp`

---

### Q: How do I spawn objects in a star system?
**A:** Use object factories + add to hierarchy:
```lts
# Create system container
var root (Object_System (Position 0 0 0) seed)

# Spawn asteroid
var asteroid (Object_Asteroid (Item_AsteroidType seed) 100.0)
asteroid.SetPos (Vec3 1000 0 500)
root.AddInterior asteroid

# Spawn ship
var ship (Object_Ship Item_ShipHull_Fighter "Terran" 1000)
ship.SetPos (Vec3 0 0 0)
root.AddInterior ship
```

---

### Q: How do I add cargo to a ship?
**A:** Use the existing cargo system (fully implemented!):
```lts
var ship (player.GetPiloting)

# Create item
var ironOre (Item_OreType 100)

# Add to cargo (returns false if full)
if (ship.AddItem ironOre 50) {
  Log "Added 50 iron ore to cargo"
} else {
  Log "Cargo is full!"
}

# Check cargo contents
for it (ship.GetCargo) it.HasMore it.Advance {
  var item (it.GetItem)
  var qty (it.GetQuantity)
  Log (item.GetName + ": " + qty)
}
```

---

### Q: Can players mine asteroids?
**A:** Yes! Mining system exists in `Action/Mine.cpp`, but not wired to player yet.

**Current status:**
- ✅ Mining code works (extracts ore, adds to cargo)
- ❌ No 'M' hotkey to trigger mining
- ❌ No targeting system for asteroids

**To implement:** See SAVE-LOAD-AND-INVENTORY.md Part 5 for complete code.

---

### Q: How do I save the game?
**A:** Use the JSON save layer. In `ltheory-main`: **F6** quicksave, **F7**
quickload, and launch auto-loads the last save (after the loading screen).

**Current status:**
- ✅ Multi-slot JSON saves (`SaveGameJSON.{h,cpp}`), exception-free nlohmann/json
- ✅ `SaveGame_Create`/`SaveGame_Load`/`SaveGame_LoadSlot`/`SaveGame_ListSlots` bindings
- ✅ Wired in `ltheory-main` with F6/F7 + launch auto-load + toast feedback
- ❌ No main-menu save entry / save-browser widget yet

**Applying a load:** the engine only reads the file; the app applies state
itself (`player.SetName`, `player.SetCredits`, `ship.SetPos`, `ship.SetLook`).
`d.version == 0` means "no save" — drive the NO SAVE FOUND toast off that.

---

### Q: How do I create a custom UI widget?
**A:** Use composable widget pattern:
```lts
# Basic widget structure
type MyWidget
  String text
  
  function List CreateChildren (Widget self) {
    var l List
    l += Components:AlignCenter (Widgets:Text text 16 Colors:Primary)
    l
  }
  
  function Void PreDraw (Widget self) {
    # Draw background
    DrawPanel self.pos self.size Colors:Background 0.8 1.0 0
  }

# Factory function
function Widget Create (String text) {
  Stack
    Custom Widget
      MyWidget text
}
```

See Widget/Button.lts as complete example.

---

### Q: How do I add a post-processing effect?
**A:** Create shader pair + add to render pass list:
```lts
# In your app Update():
RenderPass_Clear                              # Clear framebuffer
Camera_Push camera                            # Render from camera
# ... scene rendering ...
Camera_Pop

# Post-processing
RenderPass_PostFilter "post/bloom.jsl"        # Apply bloom
RenderPass_PostFilter "post/dither.jsl"       # Apply dither
RenderPass_Interface ui                       # Draw UI on top
```

See GRAPHICS-TECH.md for shader examples.

---

### Q: How do procedural generators work?
**A:** They generate geometry on CPU, upload to GPU:
```lts
# Generate asteroid mesh (5-20ms)
var mesh (Generator_Asteroid size seed detail sharpness density)

# Generate planet surface (GPU-side, in shader)
var planet (Object_Planet planetType)  # Uses gen/planet.jsl

# Generate ship hull (2-10ms)
var mesh (Generator_ShipHull seed size)
```

See PROCEDURAL-GENERATION-GUIDE.md for deep-dive.

---

### Q: How do I play sounds?
**A:** Use Sound_Play functions:
```lts
# 2D sound (UI)
Sound_Play "ui/click.wav" 0.5                 # volume 0-1

# 2D looped (music)
Sound_PlayLooped "music/ambient.ogg" 0.3

# 3D positioned (explosions, weapons)
var pos (ship.GetPos)
var vel (ship.GetVel)
Sound_Play3D "explosion/boom.wav" pos vel 1.0
```

See AUDIO-SYSTEM-GUIDE.md for 300+ sound asset catalog.

---

## 7. Documentation Cross-Reference

### Quick Navigation by Topic

**"I want to understand..."**
- **Engine architecture:** AGENTS.md
- **Graphics/shaders:** GRAPHICS-TECH.md (6,000 words)
- **Audio system:** AUDIO-SYSTEM-GUIDE.md (8,000 words)
- **Procedural generation:** PROCEDURAL-GENERATION-GUIDE.md (10,000 words)
- **LTSL language basics:** docs/ltsl-docs.md (8,000 words)
- **LTSL internals:** LTSL-ARCHITECTURE-AND-IMPROVEMENTS.md (15,000 words)
- **Save/load/inventory:** SAVE-LOAD-AND-INVENTORY.md (12,000 words)
- **Strategic roadmap:** PRD-LIMIT-THEORY-REBOOT.md (15,000 words)
- **Forward work plan:** ROADMAP.md (repo root — single source of truth, supersedes PRD Phase statuses)
- **Space phenomena:** VULKAN-AND-SPACE-PHENOMENA.md (12,000 words)
- **Modding/JSON:** ENGINE-STABILITY-AND-MODDING.md (15,000 words)
- **UI widgets:** Widget/Button.lts, Widget/Window.lts examples
- **AI assistance:** AI-ASSISTED-DEVELOPMENT-GUIDE.md (you're here!)

**"I want to implement..."**
- **New widget:** Use Widget/Button.lts as template
- **New object type:** Use Object/Ship.lts as template
- **New LTSL function:** Add to src/liblt/Game/ScriptAPI/*.cpp
- **New shader:** Create resource/shader/{vertex,fragment}/name.jsl (GLSL 4.60 core)
- **New mission type:** Extend Game/Mission/*.cpp
- **Inventory UI:** SAVE-LOAD-AND-INVENTORY.md Part 4
- **Mining hotkey:** SAVE-LOAD-AND-INVENTORY.md Part 5
- **Save/load system:** SAVE-LOAD-AND-INVENTORY.md Part 2-3
- **PBR rendering:** GRAPHICS-TECH.md Part 2
- **List methods:** LTSL-ARCHITECTURE-AND-IMPROVEMENTS.md Part 4

---

## 8. Code Style & Conventions

### C++ Style
- **Standard:** C++17, `-fno-exceptions`, `-msse -msse2`
- **Pointers:** Use `nullptr` (not `NULL`)
- **Smart pointers:** Use `Reference<T>` for refcounted types (not `std::shared_ptr`)
- **Reflection:** Use `AutoClass`/`FIELDS` macros for serializable types
- **Error handling:** Return codes or nullptr (no try/catch/throw)
- **Naming:** PascalCase for types, camelCase for functions/variables

### LTSL Style
- **Variables:** `var name value` (no type annotation)
- **Spacing:** `function Void Name (Type arg)` (space after type)
- **Braces:** K&R style (opening brace on same line)
- **Indentation:** 2 spaces
- **Operators:** Limited set (`+`, `-`, `*`, `/`, `>`, `<`, `==`, `!=`)
- **No exceptions:** No try/catch/throw equivalent (use error codes)

### File Naming
- **C++ headers:** `.h`
- **C++ source:** `.cpp`
- **LTSL scripts:** `.lts`
- **Shaders:** `.jsl` (GLSL 4.60 core with JSL preprocessor)

---

## 9. Performance Notes

### LTSL Performance
- **Architecture:** Tree-walking interpreter (26 expression types)
- **Speed:** ~3-5x slower than C++ for tight loops
- **Bottleneck:** Not gameplay code (fast enough)
- **Optimization:** Bytecode VM would be 3-5x faster but 2-3 month project (not needed yet)

### Graphics Performance
- **Context:** OpenGL 4.6, GLSL 4.60 core
- **Shaders:** 170 total (35 post-effects, 19 generators)
- **Draw calls:** Single global VAO, VBO uploads per frame
- **Bottleneck:** CPU-side (not GPU-bound)
- **Target:** 60 FPS on modern hardware (achieved)

### Memory
- **Refcounting:** Intrusive via `Reference<T>`
- **Allocation:** ~585 raw `new`/`delete` (most wrapped in Reference<T>)
- **Leaks:** None detected in 399 unit checks

---

## 10. When to Ask User for Help

### AI Can Help With ✅
- Understanding existing code structure
- Writing new LTSL scripts (apps, widgets)
- Implementing documented features (from guides)
- Debugging compiler errors
- Creating shaders (GLSL 4.60)
- Modifying UI widgets
- Explaining game mechanics

### Ask User For ❌
- **Architectural decisions:** "Should we migrate to Vulkan?" "Rewrite in Rust?"
- **Game design:** "What missions should we have?" "How should combat work?"
- **Art assets:** "What textures/models do we need?"
- **Performance profiling:** "Is X actually slow?" (need real data)
- **Platform-specific bugs:** "Works on Linux, crashes on Windows"
- **Project priorities:** "Should we do PBR or modding first?"

---

## 11. Testing & Validation

### Unit Tests
```bash
# Run all tests (492 checks)
python configure.py test

# Tests cover:
# - String operations
# - Vector/Array containers
# - Type system (Reference<T> refcounting)
# - Serialization round-trips
# - LTSL compilation (error reporting)
# - Script binding surface (Log/Log_Warn/Log_Error, Program_Exit, Mouse_SetPos)
```

### Selftest app (LTSL-level regression harness)
```bash
# 10 layout/focus assertions, compiled + run by the engine itself.
# Exits 0 on success (also usable as a CI gate).
python configure.py run selftest
```
The harness lives in `resource/script/App/selftest.lts` — add assertions there
instead of new C++ test scaffolding when the check can be expressed in LTSL.

### LSP diagnostics smoke (editing-time gate)
```bash
node script/ltsl-lsp/out/smoke.js $(find resource/script -name '*.lts' | sort)
# Expect exactly 8 diagnostics: 4 structural fixtures + 4 accepted warnings.
# See AGENTS.md §6.2 for the accepted list.
```

### Manual Testing
```bash
# Run each app to verify
python configure.py run war           # Combat (main test)
python configure.py run launcher      # UI test
python configure.py run ltheory-main  # Universe test

# Check for:
# - No crashes
# - No GL errors (if BUILD_STRICT defined)
# - Smooth 60 FPS
# - Audio plays correctly
```

### Shader Verification
All 170 shaders compile correctly under GLSL 4.60 core.
- Vertex shaders: 84 files
- Fragment shaders: 86 files
- No GL errors in any runnable app

---

## 12. System Requirements

### Development
- **OS:** Linux (Debian/Ubuntu tested), Windows (MSVC)
- **CMake:** >= 3.10
- **Compiler:** GCC 15+ or Clang 14+ (C++17 support)
- **SFML:** 3.1.0 (system-installed via apt)
- **GLEW:** 2.3.1 (built from source)
- **OpenGL:** 4.6 support required
- **RAM:** 8GB minimum (16GB recommended for large builds)

### Runtime
- **OS:** Linux (X11 or Wayland+XWayland)
- **GPU:** OpenGL 4.6 support (any modern GPU)
- **RAM:** 4GB minimum
- **Disk:** ~500MB (engine + resources)

---

## 13. Known Limitations & Future Work

### Current Limitations
- **No Wayland native** (SFML 3.1 is X11-only, runs via XWayland)
- **No bytecode VM** (LTSL interpreter is tree-walking)
- **No list methods** (no `.Filter()`, `.Map()`, `.Reduce()`)
- **No string interpolation** (planned: `$"Hello {name}"`)
- **No foreach loop** (must use iterator pattern)
- **No save browser** (F6/F7 quick save/load + launch auto-load work; main-menu
  save entry + slot browser remain)

### Planned Improvements (Documented)
See LTSL-ARCHITECTURE-AND-IMPROVEMENTS.md Part 7 for full roadmap:
- **Phase 1 (1-2 weeks):** List methods, foreach, string interpolation, lambdas
- **Phase 2 (2-3 weeks):** Pattern matching, destructuring, optional chaining
- **Phase 3 (2-3 months):** Bytecode VM (optional performance boost)

---

## 14. Success Metrics (How to Know You're Helping)

### Good AI Session
- ✅ User asks question → AI answers in <30 seconds
- ✅ AI provides working code on first try
- ✅ AI explains "why" not just "how"
- ✅ AI references correct documentation
- ✅ AI warns about known issues before user hits them
- ✅ User says "perfect!" or "exactly what I needed"

### Bad AI Session (Avoid!)
- ❌ AI searches for 5+ minutes before answering
- ❌ AI suggests things already documented as "don't do"
- ❌ AI provides code that doesn't compile
- ❌ AI ignores known limitations
- ❌ User has to repeat question 3+ times
- ❌ User says "never mind, I'll figure it out myself"

---

## 15. Revision History

**v2.2.0 (2026-08-10):** save/load + toast wiring session
- Save/Load entries updated: JSON save layer (A.10) + F6/F7 quicksave/quickload
  + launch auto-load wired into `ltheory-main`, two-tone config-driven toast
  (`Widget/Toast.lts`), `Config_Get` → `Config:Get` (Config.lts), added
  `Object_SetCredits` binding
- Test counts refreshed: 492 checks (lte_tests)
- Loading-screen pattern updated to `Config:Get`; "How do I save the game?" Q&A
  rewritten

**v2.1.0 (2026-08-08):** DX-hardening session follow-up
- Fixed a corrupted line in the LTSL Components section (garbled `obj.SetMass` line + broken code fence)
- LSP section updated: ZED extension (not VS Code)
- Test counts refreshed: 399 checks (lte_tests), selftest app added to §11
- Added the LSP diagnostics smoke gate (8 diagnostics expected)

**v2.0.0 (2026-07-30):** Enhanced with real codebase patterns (LTSL examples from war.lts/Button.lts, map guide, shader patterns)

**v1.0.0 (2026-07-30):** Initial comprehensive skill creation
- Quick reference API (LTSL object creation, math, components, widgets)
- File structure map with critical files highlighted
- Known issues (implemented vs needs wiring vs don't do)
- Common LTSL gotchas (string concat, Position vs Vec3, etc.)
- Build/run commands
- 13 common Q&A pairs
- Documentation cross-reference map
- Code style conventions
- Performance notes
- Testing guidelines
- System requirements
- Success metrics

---

## Quick Tips for AI Agents Using This Skill

1. **Always check "Known Issues" first** — don't suggest re-implementing what exists
2. **Use exact file paths** — user can navigate directly
3. **Provide working code** — copy-paste ready, not pseudocode
4. **Reference detailed docs** — this skill is overview, point to deep-dives
5. **Warn about gotchas** — string concat, Position vs Vec3, etc.
6. **Test suggestions mentally** — does this LTSL syntax work?
7. **Ask user for design decisions** — don't assume priorities
8. **Update this skill** — when you discover new patterns, tell user to add them

---

**End of SKILL.md**

**This skill should make AI agents 10x more helpful! Use it well!** 🚀✨
