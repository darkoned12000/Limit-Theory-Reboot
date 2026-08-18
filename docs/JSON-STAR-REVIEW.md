# Star System — JSON Data Layer Review

## Current State (2026-08-18)

Stars are now data-driven via `resource/gamedata/stars.json`. Seven spectral
classes (O/B/A/F/G/K/M) control color, brightness, and radius. Seed picks a
class via weighted random, per-instance variation from `brightnessRange`/
`radiusRange`.

### Files

| File | Change |
|---|---|
| `resource/gamedata/stars.json` | 7 star classes, classWeights, brightnessRange, radiusRange |
| `src/liblt/Game/Object/System.cpp` | `GenerateStar()` reads stars.json, picks class, prints to console |
| `src/liblt/Game/Object/Star.cpp` | `lightBrightness`/`lightRadius` fields, uses JSON values |
| `src/liblt/Game/Objects.h` | `Object_Star(color, brightness, radius)` — 3-arg signature |
| `resource/shader/fragment/light/global.jsl` | Phong lighting now uses `starColor` (was hardcoded 8.0) |
| `resource/shader/fragment/starbg.jsl` | Skybox scaled by `starColor` luminance |
| `tests/TestStars.cpp` | 17 tests, 0 failures |
| `tests/CMakeLists.txt` | Added TestStars.cpp |

### How it works

1. Seed → weighted random picks a star class (O/B/A/F/G/K/M)
2. Class defines base color (hex), brightness, radius
3. Defaults `brightnessRange`/`radiusRange` multiply base values per-instance
4. Same seed = same class + same variation every time

### Class properties

| Class | Color | Brightness | Radius | Notes |
|---|---|---|---|---|
| O | #9bb0ff | 15.0 | 5000000 | Blue-white, massive, rare |
| B | #aabfff | 12.0 | 4000000 | Blue-white |
| A | #cad7ff | 10.0 | 3500000 | White |
| F | #f8f7ff | 8.0 | 3000000 | White, most common |
| G | #fff4ea | 6.0 | 2500000 | Yellow-white (sun-like) |
| K | #ffd2a1 | 4.0 | 2000000 | Orange |
| M | #ffcc6f | 2.0 | 1500000 | Red-orange, dimmest |

### Console output

```
GenerateStar class=G color=(1.00, 0.96, 0.92) brightness=5.4 radius=2700000
```

---

## Bugs found and fixed

### 1. Find() vs FindPath() — always returned F class

`DatabaseManager::Find("stars", "defaults.classWeights")` looked for a literal
key `"defaults.classWeights"` which doesn't exist. Always fell back to F class
(index 3). Fixed: use `FindPath()` for dot-notation lookups.

### 2. Global lighting ignored star color

`global.jsl` had hardcoded `light += 8.0 * l` for Phong materials — no
`starColor` multiplier. All star classes produced identical scene lighting.
Fixed: multiply by `starColor`.

### 3. Skybox ignored star brightness

`starbg.jsl` sampled nebula cubemap directly — no brightness scaling. Background
looked the same for all star classes. Fixed: multiply output by `starColor`
luminance.

### 4. Nebula color2 used legacy warm formula

`GenerateStarColor(rng)` always produced warm (orange/red) colors. Even with a
blue O-class star, nebula color2 was mixed 50% with warm formula. Fixed: derive
color2 from star class color by rotating RGB channels (`V3(sc.z, sc.x, sc.y)`).

### 5. starColor uniform blew out for bright stars

`MessageGetColor` was changed to return `lightBrightness * color`, but shaders
expect 0-1 color values. F-class (brightness=8.0) produced values > 1.0,
blowing out to white. Fixed: keep `starColor` as raw color (0-1), brightness
only affects Light object.

---

## Known issues / deferred work

### 1. Scene brightness doesn't scale with star class

`starColor` uniform is raw color (0-1) for all classes — atmosphere, fog, and
ambient lighting look similar for M and O class stars. The brightness difference
only shows in:
- Lens flare intensity (via `light->color`)
- Local lighting on nearby objects (via Light pass)
- Skybox luminance (via `starColor` luminance multiplier)

**Options for future fix:**
- Add a separate `starBrightness` uniform (0-1, normalized) that shaders can
  use when they want intensity, while keeping `starColor` as pure color
- Normalize `starColor` by max brightness: `starColor * (brightness / 15.0)`
  — keeps values in 0-1 but gives relative differences
- Leave as-is if the visual difference from Light object is sufficient

### 2. Star visual is only a lens flare

The star has no mesh/surface — it's a Light object that renders as a lens
flare billboard. The "lemon shape" distortion at screen edges is from
projection stretching (`up + right` direction). This is realistic lens flare
behavior.

### 3. Star position is fixed

Star is always at `Spherical(60000000, 1.25*kPi2, 0)` — same distance and
angle for all systems. Could vary per system.

### 4. No binary/multiple star systems

Currently single star only. All light comes from one source.

---

## Enhancements (prioritized)

### Easy (mostly shader/data)

#### Variable stars (pulsating brightness)
- Oscillate `lightBrightness` in `Star::OnUpdate` using sine wave
- Add `pulseSpeed` and `pulseAmplitude` to stars.json
- ~10 lines of C++ in `Star::OnUpdate`
- Adds visual interest without engine changes

#### Lens flare patterns per class
- Currently one generated texture (`gen/lensflare.jsl`) for all stars
- Could vary flare parameters per class:
  - O/B: anamorphic streaks (horizontal/vertical), tighter core
  - G: classic circular flare
  - K/M: wider, softer glow
- Requires passing class info to lens flare shader or generating
  different textures per class

#### Star field density
- `Renderable_Starfield` in `System.cpp` uses fixed `kBaseStarCount = 100000`
- Could add `starFieldDensity` to stars.json or per-class

### Medium (C++ changes)

#### Binary/trinary star systems
- Two or three Light objects orbiting each other
- Requires orbital mechanics in `System.cpp` (periodic position update)
- Would affect lighting (two light sources), atmosphere (two starDir),
  and nebula (two colors)
- Significant but well-scoped change

#### Star rotation
- Rotate billboard mesh for surface detail animation
- Currently star has no mesh — would need a billboard mesh + rotating UV
- Or: animate lens flare rotation in shader

#### Solar wind particles
- Particle system attached to star (like DustFlecks but radial outward)
- Could affect nearby ships (hazard mechanic)
- Engine already has particle infrastructure

#### Corona shader
- Atmospheric glow effect around star edge
- Could be a separate render pass or added to lens flare
- Depends on star radius for scale

#### Star catalog / exploration
- Named stars, coordinates, discovery system
- Requires new component or data structure
- Gameplay-level feature, not just visual

### Hard (significant engine work)

#### Eclipse/shadow casting
- Planets blocking star light requires shadow mapping infrastructure
- Would need depth buffer pass from star's perspective
- Major engine addition

#### Black holes
- No light source, accretion disk shader, gravitational lensing
- Completely different rendering path
- Major feature, defer unless core to gameplay

#### Neutron stars/pulsars
- Beam effects (rotating light cone)
- Requires rotating spotlight or volumetric shader
- Medium-hard

---

## Testing

### Unit tests (TestStars.cpp)
- Schema validation: loads, has version, starClasses, defaults
- Class structure: all classes have color, brightness, radius
- Ordering: brightness and radius decrease O→M
- Defaults: classWeights array, brightnessRange, radiusRange
- Hex color parsing, range helpers, fallback behavior
- **17 tests, 1009 total checks, 0 failures**

### Manual testing
- Change `classWeights` to force specific class, verify console output
- Verify different seeds produce different classes with default weights
- Check lens flare brightness varies between classes
- Check skybox brightness varies between classes

### Test command
```bash
LD_LIBRARY_PATH=bin:extbin/linux64 bin/lte_tests
```

---

## JSON schema

```json
{
  "version": 1,
  "starClasses": {
    "O": { "color": "#9bb0ff", "brightness": 15.0, "radius": 5000000 },
    "B": { "color": "#aabfff", "brightness": 12.0, "radius": 4000000 },
    "A": { "color": "#cad7ff", "brightness": 10.0, "radius": 3500000 },
    "F": { "color": "#f8f7ff", "brightness": 8.0,  "radius": 3000000 },
    "G": { "color": "#fff4ea", "brightness": 6.0,  "radius": 2500000 },
    "K": { "color": "#ffd2a1", "brightness": 4.0,  "radius": 2000000 },
    "M": { "color": "#ffcc6f", "brightness": 2.0,  "radius": 1500000 }
  },
  "defaults": {
    "classWeights": [1, 2, 3, 4, 5, 3, 1],
    "brightnessRange": [0.8, 1.2],
    "radiusRange": [0.9, 1.1]
  }
}
```

### Field reference

| Field | Type | Description |
|---|---|---|
| `color` | #RRGGBB | Star surface color. O/B are blue-white, G is yellow-white, K/M are orange-red |
| `brightness` | float | Light intensity multiplier. Combined with per-instance brightnessRange variation |
| `radius` | float | Light reach distance. Combined with per-instance radiusRange variation |
| `classWeights` | [7 floats] | Relative probability weights for O/B/A/F/G/K/M. Higher = more common |
| `brightnessRange` | [min, max] | Per-instance brightness variation. Base brightness × random(min, max) |
| `radiusRange` | [min, max] | Per-instance radius variation. Base radius × random(min, max) |

### Tips

- **More bright stars:** Increase brightness on O/B classes
- **More red stars:** Increase M class weight, decrease O/B weights
- **Dimmer universe:** Lower all brightness values or widen brightnessRange downward
- **Bigger lights:** Increase radius values on desired classes
- **Force a class:** Set all classWeights to 0 except the desired one (e.g., `[1, 0, 0, 0, 0, 0, 0]` for O)
