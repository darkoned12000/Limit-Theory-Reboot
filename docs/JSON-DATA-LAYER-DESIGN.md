// Copyright (C) 2025  darkoned12000
// SPDX-License-Identifier: GPL-3.0-or-later
// Part of the ltheory-old-test modernization effort (Revamp Work).
// See NOTICE and LICENSE.GPL. Original engine (c) Josh Parnell, public domain.

# JSON Data Layer Design — ROADMAP 2.3 + 2.3b

Comprehensive plan for making the engine data-driven via JSON configuration
files. This document covers scope, architecture, schemas, phased delivery,
risks, LTSL implications, and engine visual enhancements (2.3b).

---

## 1. What This Solves

### Current Problem

All game balance, type definitions, and tuning parameters are hardcoded in
C++ or baked into GLSL shaders:

- **WeaponType.cpp** — 10 multiplier tables (damage, fire rate, spread, etc.)
  indexed by `WeaponClass` enum, plus hardcoded color formulas, magazine
  times, scale, integrity, and 6 sound file paths.
- **ShipType.cpp** — hardcoded allocation ratios (60% hull, scanner=1000),
  socket counts (thrusters=2, turrets=4), and the name "Ship".
- **PlanetType.cpp** — hardcoded scale (100000), atmosphere ranges,
  color palettes, ring probability (60%).
- **StationType.cpp** — hardcoded dock capacity (100), name "Station".
- **Constants.h** — all balance formulas (`Constant_ValueToMass`,
  `Constant_AmmoDamageMult`, etc.) as inline functions.
- **GLSL shaders** — nebula constants (`kDepth=4.0`, `kEmission=3.0`,
  `kSamples=256`), scattering coefficients (`kRayleigh`, `kMie`), ocean
  color, fog density, gamma — all baked into `.jsl` files.
- **Star** — invisible (light source only), no surface shader, position/
  size/brightness hardcoded, lens flare exists but unwired.
- **Planets** — static surface (no rotation), static clouds (no animation),
  no moons, no biome categorization.
- **NPC AI** — tasks are imperative ("do X"), no configurable behavior
  parameters, no tuning knobs for aggression/mining/trading behavior.
- **gameConfig.txt** — flat `key:value` text, 23 keys, no hierarchy.

**Impact:** Every balance change, new weapon class, planet biome variation,
visual tuning, or NPC behavior tweak requires a C++ or GLSL recompile.
Iteration is slow. Modding is impossible without source access.

### What a JSON Layer Unlocks

1. **Instant iteration** — designers tweak JSON, reload, see changes.
   No recompile cycle.
2. **Modding foundation** — modders drop JSON files into `mods/` to add
   ships, weapons, biomes, tweak visuals, tune NPC behavior without
   touching C++.
3. **Content diversity** — easy to create dozens of ship/weapon/planet
   variants by tweaking numbers, not rewriting code.
4. **Visual authoring** — artists control nebula density, scattering
   haze, dust colors, post-processing strengths, star appearance —
   all via JSON, not by hunting through shader source.
5. **NPC tuning** — gameplay designers adjust aggression, mining rates,
   trade thresholds, fleet composition without recompiling.
6. **Separation of concerns** — game logic in C++, content data in JSON,
   visual parameters in JSON. Each evolves independently.
7. **Testing** — JSON schemas are self-documenting; validation catches
   errors at load time instead of runtime.

---

## 1.1 Progress (updated 2026-08-19)

### Infrastructure — **DONE**
- `DatabaseManager` singleton (`src/liblt/Game/DatabaseManager.{h,cpp}`)
- `JsonDatabase` wrapper (`src/liblt/Game/JsonDatabase.{h,cpp}`)
- LTSL bindings: `Database_Load`, `Database_Get`, `Database_GetPath`,
  `Database_Has`, `Database_HasDatabase`, `Database_Keys`, `Database_Reload`
  (`src/liblt/Game/ScriptAPI/Database.cpp`)
- `JsonHelpers.h` — shared `JGet`, `JColor`, `JRange`, `JFloat`, `JBool`,
  `JInt`, `JVec3` (`src/liblt/Game/JsonHelpers.h`)
- Unit tests: `TestJsonDatabase.cpp` (17 tests), `TestPlanets.cpp` (21
  tests), `TestStars.cpp` (17 tests)

### Stars — **DONE**
- `resource/gamedata/stars.json` — 7 spectral classes (O/B/A/F/G/K/M) with
  color, brightness, radius; defaults with classWeights, brightnessRange,
  radiusRange, starFieldRange, pulseSpeedRange, pulseAmplitudeRange
- `System.cpp` — `GenerateStar()` reads stars.json, picks class via weighted
  random, applies per-instance variation (brightness, radius, pulse)
- `Star.cpp` — `AutoClassDerived` gains `baseBrightness`, `pulseSpeed`,
  `pulseAmplitude`, `age`. `OnUpdate` oscillates `lightBrightness` via
  sine wave: `baseBrightness + baseBrightness * pulseAmplitude * Sin(age * pulseSpeed)`
- Per-class lens flare textures — `gen/lensflare.jsl` generates 7 textures
  with varying `coreTightness`, `streakIntensity`, `glowWidth`. Auto-detects
  star class from light color.
- `global.jsl` — Phong lighting now uses `starColor` multiplier
- `starbg.jsl` — Skybox output scaled by `starColor` luminance
- Nebula cubemap — color2 derived from star class color via RGB rotation

### Planets — **PARTIAL**
- `resource/gamedata/planets.json` — Normalized 5-biome schema with ranges,
  hex colors, biomeOrder
- `Item_PlanetType()` — Wired to planets.json via `JsonHelpers.h`
- `planet.jsl` — `oceanLevel` uniform wired (was hardcoded `0.3`)
- Biome assignment, planet rotation, cloud animation — **NOT YET WIRED**
  (planets.json has the data; C++ doesn't read biome fields yet)

---

## 2. Architecture

### 2.1 Layer Model

```
┌─────────────────────────────────────────────────┐
│  LTSL Scripts (ltheory-main, SystemPopulate)    │
│  call Database_Get "ships" "fighter"             │
│  call Database_Get "graphics" "nebula.depth"     │
│  call Database_Get "npc" "mining.aggression"     │
└──────────────────────┬──────────────────────────┘
                       │ LTSL binding
┌──────────────────────▼──────────────────────────┐
│  C++ Database Loaders (ShipDatabase, etc.)       │
│  - Parse JSON on load                            │
│  - Populate in-memory lookup tables              │
│  - Expose to LTSL via Function_Bind              │
│  - Pass values to shader uniforms / C++ params   │
└──────────────────────┬──────────────────────────┘
                       │ nlohmann/json (already vendored)
┌──────────────────────▼──────────────────────────┐
│  JSON Files (resource/gamedata/*.json)           │
│  - ships.json, weapons.json, graphics.json, ...  │
│  - Loaded at app startup or on hot-reload        │
└─────────────────────────────────────────────────┘
```

### 2.2 Core Infrastructure

A single **`DatabaseManager`** singleton manages all JSON databases:

```cpp
// DatabaseManager.h
class DatabaseManager {
  Map<String, SharedPtr<JsonDatabase>> databases;
public:
  void Load(String const& name, String const& path);
  Json const* Get(String const& db, String const& key) const;
  Vector<String> ListKeys(String const& db) const;
  bool Has(String const& db, String const& key) const;
  void Reload(String const& db);  // for hot-reload (§2.4)
};
```

Each database is a **`JsonDatabase`** — a thin wrapper around a
`nlohmann::json` object with a lookup index:

```cpp
// JsonDatabase.h
class JsonDatabase {
  json data;                        // raw JSON
  Map<String, json*> index;        // key → pointer for O(1) lookup
  String path;                     // source file path
  int version;                     // schema version
public:
  bool Load(String const& path);
  json const* Find(String const& key) const;
  Vector<String> Keys() const;
  int Version() const;
};
```

### 2.3 LTSL Bindings

The database layer exposes 4 functions to LTSL scripts:

| Binding | Signature | Purpose |
|---------|-----------|---------|
| `Database_Load` | `Void Database_Load(String name, String path)` | Load a JSON file as a named database |
| `Database_Get` | `Json Database_Get(String db, String key)` | Get a JSON value by database + key |
| `Database_Has` | `Bool Database_Has(String db, String key)` | Check if a key exists |
| `Database_Keys` | `Array String Database_Keys(String db)` | List all keys in a database |

**`Database_Get` returns a JSON value** — this requires a new LTSL
`JsonValue` type (see §6 for LTSL improvements needed). Scripts access
fields via dot notation:

```
var ship (Database_Get "ships" "fighter")
var hullHP ship.hull.hp      # nested access
var thrusters ship.thrusters  # array access

var nebula (Database_Get "graphics" "nebula")
var depth nebula.depth        # shader constant
```

### 2.4 Hot-Reload (deferred to 2.4)

JSON files are loaded once at startup. A future `AssetWatcher` (ROADMAP
2.4) will monitor `resource/gamedata/` for changes and call
`DatabaseManager::Reload()` — enabling live tuning without restart.

For now, reload requires restarting the app or calling `Database_Load`
from a script.

### 2.5 File Organization

```
resource/gamedata/
  config.json          ← replaces gameConfig.txt (hierarchical)
  ships.json           ← ship type definitions
  weapons.json         ← weapon type definitions + balance tables
  planets.json         ← planet biome definitions + generation params + moons
  stations.json        ← station type definitions
  economy.json         ← commodity definitions + market parameters
  factions.json        ← faction definitions + relationships
  universe.json        ← region structure, connectivity, asteroid density
  graphics.json        ← ALL rendering settings (see §3.6)
  audio.json           ← sound definitions, music tracks, volume defaults
  npc.json             ← NPC behavior parameters (see §3.9)
```

---

## 3. JSON Schemas

### 3.1 ships.json

```json
{
  "version": 1,
  "ships": {
    "fighter": {
      "name": "Fighter",
      "class": "light",
      "hull": {
        "valueRatio": 0.6,
        "integrityMult": 1.0
      },
      "propulsion": {
        "thrusterCount": 2,
        "thrusterValueRatio": 0.15
      },
      "weapons": {
        "turretCount": 2,
        "turretValueRatio": 0.15
      },
      "scanner": {
        "value": 1000
      },
      "generator": {
        "minCount": 1,
        "maxCount": 3
      },
      "interior": {
        "countFormula": "2 * (logScale / log10)"
      },
      "description": "Fast, lightly armed scout vessel"
    },
    "cruiser": {
      "name": "Cruiser",
      "class": "medium",
      "hull": { "valueRatio": 0.55, "integrityMult": 1.2 },
      "propulsion": { "thrusterCount": 4, "thrusterValueRatio": 0.12 },
      "weapons": { "turretCount": 6, "turretValueRatio": 0.20 },
      "scanner": { "value": 2000 },
      "generator": { "minCount": 2, "maxCount": 5 },
      "interior": { "countFormula": "3 * (logScale / log10)" },
      "description": "Balanced multi-role combat vessel"
    }
  }
}
```

**Replaces:** hardcoded ratios in `ShipType.cpp` lines 188–217.

### 3.2 weapons.json

```json
{
  "version": 1,
  "classes": {
    "beam": {
      "magazineSizeMult": 0,
      "magazineProbability": 0,
      "powerDrainMult": 5,
      "rateMult": 1,
      "spreadMult": 0,
      "weightMult": 5,
      "ammoDamageMult": 5,
      "ammoLifeMult": 2.5,
      "ammoProbabilityMult": 0,
      "ammoSpeedMult": 1e10
    },
    "missile": {
      "magazineSizeMult": 1,
      "magazineProbability": 1,
      "powerDrainMult": 0,
      "rateMult": 0.01,
      "spreadMult": 1,
      "weightMult": 3,
      "ammoDamageMult": 20,
      "ammoLifeMult": 10,
      "ammoProbabilityMult": 0,
      "ammoSpeedMult": 1
    },
    "pulse": {
      "magazineSizeMult": 6,
      "magazineProbability": 0.1,
      "powerDrainMult": 2,
      "rateMult": 1,
      "spreadMult": 2,
      "weightMult": 2,
      "ammoDamageMult": 2,
      "ammoLifeMult": 1.25,
      "ammoProbabilityMult": 1,
      "ammoSpeedMult": 1
    },
    "rail": {
      "magazineSizeMult": 10,
      "magazineProbability": 0.9,
      "powerDrainMult": 1,
      "rateMult": 1,
      "spreadMult": 5,
      "weightMult": 1,
      "ammoDamageMult": 1,
      "ammoLifeMult": 1,
      "ammoProbabilityMult": 0,
      "ammoSpeedMult": 1e10
    }
  },
  "balance": {
    "magazineTimeRange": [6, 10],
    "defaultScale": 0.5,
    "defaultOffset": [0, 0.5, 4],
    "defaultIntegrity": 100,
    "colorFormula": "0.25 * white + seeded"
  }
}
```

**Replaces:** 10 `k*` arrays in `WeaponType.cpp` lines 34–70 + hardcoded
values in the generation function.

### 3.3 planets.json

```json
{
  "version": 1,
  "defaults": {
    "scale": 100000,
    "dockCapacity": -1,
    "atmoDensityRange": [0.0, 2.0],
    "atmoTintSaturationRange": [0.5, 1.0],
    "cloudLevelRange": [-0.2, 0.15],
    "ringProbability": 0.6,
    "wavelengthBase": [0.66, 0.53, 0.4]
  },
  "biomes": {
    "desert": {
      "name": "Desert",
      "surfaceTint": [0.8, 0.6, 0.3],
      "atmoDensityRange": [0.0, 0.5],
      "oceanLevel": 0.0,
      "cloudLevel": -0.1,
      "cloudWindSpeed": 0.0,
      "hasRings": false,
      "rotationSpeed": 0.0001,
      "description": "Arid, barren world with minimal atmosphere"
    },
    "terran": {
      "name": "Terran",
      "surfaceTint": [0.3, 0.6, 0.2],
      "atmoDensityRange": [0.5, 1.5],
      "oceanLevel": 0.3,
      "cloudLevel": 0.1,
      "cloudWindSpeed": 0.0005,
      "hasRings": false,
      "rotationSpeed": 0.0002,
      "description": "Earth-like world with oceans and vegetation"
    },
    "ice": {
      "name": "Ice",
      "surfaceTint": [0.7, 0.8, 0.9],
      "atmoDensityRange": [0.2, 0.8],
      "oceanLevel": 0.0,
      "cloudLevel": 0.0,
      "cloudWindSpeed": 0.0003,
      "hasRings": true,
      "rotationSpeed": 0.00015,
      "description": "Frozen world with thick ice crust"
    },
    "lava": {
      "name": "Lava",
      "surfaceTint": [0.9, 0.2, 0.1],
      "atmoDensityRange": [1.0, 2.0],
      "oceanLevel": 0.0,
      "cloudLevel": -0.2,
      "cloudWindSpeed": 0.001,
      "hasRings": false,
      "rotationSpeed": 0.0003,
      "description": "Volcanic world with molten surface"
    },
    "gas_giant": {
      "name": "Gas Giant",
      "surfaceTint": [0.6, 0.5, 0.4],
      "atmoDensityRange": [1.5, 2.0],
      "oceanLevel": 0.0,
      "cloudLevel": 0.15,
      "cloudWindSpeed": 0.002,
      "hasRings": true,
      "rotationSpeed": 0.0005,
      "description": "Massive gas giant with prominent ring system"
    }
  },
  "moons": {
    "enabled": true,
    "countRange": [0, 3],
    "orbitalRadiusRange": [150000, 500000],
    "sizeRange": [500, 5000],
    "moonScaleFormula": "Pow(mass / 1000, 0.5)"
  }
}
```

**Replaces:** hardcoded ranges in `PlanetType.cpp` lines 140–158 + adds
biome categorization (currently absent — all planets are "desert") + adds
moon generation parameters + cloud drift speed + planet rotation speed.

### 3.4 stations.json

```json
{
  "version": 1,
  "stations": {
    "outpost": {
      "name": "Outpost",
      "dockCapacity": 10,
      "massMultiplier": 0.5,
      "description": "Small frontier outpost"
    },
    "trading_post": {
      "name": "Trading Post",
      "dockCapacity": 50,
      "massMultiplier": 1.0,
      "description": "Mid-size commercial station"
    },
    "starport": {
      "name": "Starport",
      "dockCapacity": 100,
      "massMultiplier": 2.0,
      "description": "Major orbital starport"
    }
  }
}
```

**Replaces:** hardcoded `dockCapacity = 100` in `StationType.cpp`.

### 3.5 economy.json

```json
{
  "version": 1,
  "commodities": {
    "ore": { "name": "Ore", "basePrice": 10, "stackSize": 100 },
    "fuel": { "name": "Fuel Cells", "basePrice": 25, "stackSize": 50 },
    "components": { "name": "Components", "basePrice": 100, "stackSize": 20 },
    "rare_minerals": { "name": "Rare Minerals", "basePrice": 500, "stackSize": 10 },
    "food": { "name": "Food", "basePrice": 15, "stackSize": 80 },
    "technology": { "name": "Technology", "basePrice": 200, "stackSize": 30 }
  },
  "market": {
    "markupRange": [0.8, 1.5],
    "demandDecayRate": 0.01,
    "supplyReplenishRate": 0.005,
    "initialCredits": 100000
  },
  "trade": {
    "minProfitThreshold": 0.05,
    "maxCargoShipsPerRoute": 3,
    "routeRebalanceInterval": 60
  }
}
```

**Replaces:** `Item_Commodity` stub (currently `NOT_IMPLEMENTED`) +
hardcoded economy parameters in `Component_Economy`.

### 3.6 graphics.json (expanded — shader-level controls)

This is the most expanded schema. Every hardcoded shader constant is
exposed as a tunable JSON parameter.

```json
{
  "version": 1,

  "rendering": {
    "farPlane": 1000000,
    "nearPlane": 0.05,
    "gamma": 2.2,
    "vsync": false,
    "maxColorAttachments": 4,
    "framebufferCacheSize": 128
  },

  "nebula": {
    "depth": 4.0,
    "emission": 3.0,
    "samples": 256,
    "colorMixRatio": 0.5,
    "centralGlowFalloff": 512,
    "emissionFalloff": 5.0,
    "fractalIterations": 24
  },

  "scattering": {
    "densityMult": 50.0,
    "scale": 0.025,
    "planetRadius": 50000,
    "outerRadiusMult": 1.025,
    "depth": 0.125,
    "samples": 8,
    "rayleigh": [0.0025, 0.0045, 0.0020],
    "rayleighStrength": 0.4,
    "mie": [0.002, 0.002, 0.002],
    "mieG": 0.75
  },

  "dustfleck": {
    "opacityMult": 0.8,
    "distanceFade": 100.0,
    "defaultColor": [1.0, 1.0, 1.0],
    "shapeFalloff": 16.0
  },

  "dustcloud": {
    "maxClouds": 4,
    "defaultFogDensity": 0.0
  },

  "ocean": {
    "color": [0.03, 0.05, 0.10],
    "specularIntensity": 1.0
  },

  "star": {
    "renderDisk": true,
    "diskRadius": 3000000,
    "brightnessMult": 10.0,
    "position": [0, 60000000, 0],
    "lensFlareEnabled": true,
    "lensFlareOpacity": 0.8,
    "defaultColor": [1.0, 0.9, 0.8]
  },

  "fog": {
    "density": 0.0,
    "color": [0.0, 0.0, 0.0]
  },

  "bloom": {
    "enabled": false,
    "threshold": 1.0,
    "strength": 0.5
  },

  "ssao": {
    "enabled": false,
    "radius": 0.5,
    "intensity": 1.0,
    "kernelSize": 16
  },

  "vignette": {
    "hardness": 1.0,
    "opacity": 0.5
  },

  "motionBlur": {
    "enabled": false,
    "strength": 0.8
  },

  "chromaticAberration": {
    "enabled": false,
    "strength": 0.001
  },

  "filmGrain": {
    "enabled": false,
    "intensity": 0.05
  },

  "colorGrading": {
    "enabled": true,
    "warmHighlights": true
  },

  "lodFade": {
    "enabled": true
  }
}
```

**Replaces:** hardcoded constants across 15+ GLSL shader files:
- `nebula.jsl` → `kDepth`, `kEmission`, `kSamples`, fractal iterations
- `scattering.jsl` → `kAtmoDensityMult`, `kAtmoScale`, `kPlanetRadius`,
  `kOuterRadius`, `kDepth`, `kRcpSamples`, `kRayleigh`, `kMie`, `g`
- `dustfleck.jsl` → opacity multiplier, distance fade, color
- `dustclouds.jsl` → max cloud count, fog density
- `planet.jsl` → ocean color, specular intensity
- `global.jsl` → `farPlane`, `nearPlane`, `kGamma`
- `post/vignette.jsl` → hardness, opacity
- `post/motionblur.jsl` → strength
- `post/aberration.jsl` → strength
- `post/grain.jsl` → intensity
- `post/colorbalance.jsl` → warm highlights
- Star rendering (new — §3.10)

### 3.7 audio.json

```json
{
  "version": 1,
  "music": {
    "exploration": { "tracks": ["music/explore1.ogg"], "volume": 0.04 },
    "combat": { "tracks": ["music/combat1.ogg"], "volume": 0.08 },
    "docking": { "tracks": ["music/dock1.ogg"], "volume": 0.03 }
  },
  "sfx": {
    "weaponFire": {
      "beam": "sfx/beam_fire.ogg",
      "missile": "sfx/missile_fire.ogg",
      "pulse": "sfx/pulse_fire.ogg",
      "rail": "sfx/rail_fire.ogg"
    },
    "explosion": {
      "small": "sfx/explode_small.ogg",
      "large": "sfx/explode_large.ogg"
    },
    "ui": {
      "click": "sfx/ui_click.ogg",
      "hover": "sfx/ui_hover.ogg"
    },
    "ambient": {
      "system": "sfx/system_ambient.ogg",
      "station": "sfx/station_ambient.ogg"
    }
  },
  "pooling": {
    "maxSimultaneous": 10,
    "weaponFireBuffer": 10
  }
}
```

**Replaces:** hardcoded sound paths in `WeaponType.cpp` lines 192–199 +
volume levels scattered in `AUDIO-SYSTEM-GUIDE.md`.

### 3.8 config.json (replaces gameConfig.txt)

```json
{
  "version": 1,
  "game": {
    "seed": 1066799990,
    "loadTime": 18.0,
    "playerCredits": 10000,
    "shipHull": 20000
  },
  "universe": {
    "numOfPlanets": 1,
    "numOfShips": 12,
    "numOfAsteroids": 1000,
    "planetRingRatio": 0.6,
    "dustLevel": 1.0,
    "fogLevel": 0.0,
    "sunBrightness": 1.0,
    "sunColor": [1.0, 1.0, 1.0],
    "nebulaIntensity": 1.0
  },
  "ui": {
    "toastLifetime": 5,
    "toastBgColor": [0.02, 0.06, 0.10],
    "toastBgAlpha": 0.92,
    "toastFill": 0.65,
    "toastAccentColor": [0.2, 0.7, 1.0],
    "toastBevel": 1,
    "toastTextColor": [1.0, 1.0, 1.0],
    "toastLabelColor": [0.55, 0.60, 0.65],
    "toastValueColor": [0.50, 0.80, 1.0],
    "toastNoteColor": [1.0, 0.85, 0.2],
    "toastTitleFontSize": 21,
    "toastFontSize": 16,
    "toastNoteFontSize": 15
  }
}
```

**Replaces:** `gameConfig.txt` flat key-value format. Hierarchical,
typed, supports nested objects and arrays.

### 3.9 npc.json (NPC AI behavior tuning)

Derived from `docs/npc-ai-integration.md` — exposes the engine's existing
task system and economy allocator as tunable parameters.

```json
{
  "version": 1,

  "spawning": {
    "initialShips": 12,
    "minShips": 5,
    "maxShips": 50,
    "spawnRate": 0.01,
    "despawnDistance": 500000
  },

  "mining": {
    "enabled": true,
    "shipsPerZone": [2, 4],
    "miningRange": 5000,
    "extractRate": 1.0,
    "dockThreshold": 0.8,
    "sellThreshold": 0.5
  },

  "trading": {
    "enabled": true,
    "shipsPerRoute": [1, 3],
    "minProfitThreshold": 0.05,
    "maxCargoLoad": 100,
    "routeRebalanceInterval": 60
  },

  "piracy": {
    "enabled": true,
    "shipsPerZone": [2, 5],
    "aggroRange": 10000,
    "disengageHullPct": 0.3,
    "ignorePlayerOwned": true,
    "targetCargoPreference": 0.7
  },

  "patrol": {
    "enabled": true,
    "shipsPerStation": [2, 3],
    "patrolRadius": 20000,
    "aggroRange": 15000
  },

  "economy": {
    "allocatorEnabled": true,
    "profitRebalanceInterval": 30,
    "spawnThreshold": 0.1,
    "retireThreshold": -0.05,
    "maxFleetSize": 20
  },

  "combat": {
    "fleeHullPct": 0.25,
    "engageRange": 8000,
    "pursuitMaxDistance": 50000,
    "retreatDelay": 2.0
  },

  "behavior": {
    "reEvaluateInterval": 3.0,
    "threatAssessmentRange": 20000,
    "dockRepairHullPct": 0.3,
    "fullCargoSeekMarket": true
  }
}
```

**Replaces:** hardcoded task parameters in `Component_Economy` +
hardcoded spawning counts in `SystemPopulate.lts` + no existing NPC
behavior tuning. Derived from `docs/npc-ai-integration.md` Phase 1–3
task wiring.

### 3.10 Star customization (DONE — `stars.json`)

**Status:** Fully implemented. Star generation is data-driven via
`resource/gamedata/stars.json`. See §1.1 for details.

What's data-driven now:
- 7 spectral classes with color, brightness, radius
- Per-class lens flare textures (generated, auto-detected)
- Per-instance variation: brightness, radius, pulse speed, pulse amplitude
- Background star count range
- Light color/brightness fed to shaders (`starColor` uniform)
- Nebula cubemap color derived from star class

What remains for 2.3b (ROADMAP §2.5 — procedural star surface):
- Render star as visible glowing disk (not just light + billboard)
- Corona / prominence effects
- HDR/bloom integration

### 3.11 Planet rotation (2.3b — shader change)

Currently: planet surface is static — no `time` uniform in the planet
shader path.

**Change:** add `uniform float time` to `gen/planet.jsl` and `planet.jsl`.
The planet cubemap is generated once at creation time, but the lookup
direction can be rotated by `time * rotationSpeed` around the Y axis:

```glsl
// In planet.jsl, before cubemap lookup:
float rotAngle = time * rotationSpeed;  // from planets.json biome
vec3 rotatedDir = vec3(
  dir.x * cos(rotAngle) - dir.z * sin(rotAngle),
  dir.y,
  dir.x * sin(rotAngle) + dir.z * cos(rotAngle)
);
vec4 surf = texture(planetMap, rotatedDir);
```

The `rotationSpeed` comes from `planets.json` per biome (see §3.3).
Cloud layer rotates at `cloudWindSpeed` — same technique, different
speed.

**Engine changes:** pass `time` uniform to planet renderables in
`PlanetType.cpp`. The uniform is already available globally
(`DrawState_Link` provides `time` to all shaders that declare it).

### 3.12 Cloud animation (2.3b — shader change)

Currently: clouds are baked into the planet cubemap's B channel and
sampled with a static threshold. No drift.

**Change:** offset the cloud UV lookup by `time * cloudWindSpeed`:

```glsl
// In planet.jsl, cloud sampling:
vec3 cloudDir = rotatedDir + vec3(time * cloudWindSpeed * 0.001, 0.0, 0.0);
float cloud = texture(planetMap, cloudDir).b;
```

This creates visible cloud drift across the planet surface. The
`cloudWindSpeed` per biome comes from `planets.json` (§3.3). Gas giants
get fast winds (0.002), terran worlds get moderate (0.0005), deserts
get none (0.0).

### 3.13 Moons (2.3b — new object type)

Currently: no moon system exists.

**New `Object_Moon`:**
- Small body orbiting a planet at configurable radius
- Uses simplified planet shader (surface cubemap, no atmosphere)
- Orbital mechanics: simple circular orbit at `orbitalRadius` with
  seeded angular velocity
- Moon count, orbital radius range, size range from `planets.json`
  (§3.3 `moons` section)

**Engine changes:**
1. New `Object_Moon.{h,cpp}` — `AutoClassDerived(Moon, MoonBaseT, ...)`
   with `Component_Drawable`, `Component_Orientation`, `Component_Mass`
2. `MoonGenerator` function — called from `SystemPopulate.lts` after
   planet creation, spawns 0–3 moons per planet
3. Moon shader: simplified `planet.jsl` (surface only, no atmosphere
   scattering)
4. Orbital update in `System.cpp` — advance moon angle by `dt * speed`

**Risk:** new object type + orbital mechanics. Moderate complexity.
Defer to late 2.3b if planet rotation and cloud animation deliver
sufficient visual improvement first.

---

## 4. Phased Implementation

### 2.3 Phases (JSON layer)

#### Phase 1: Core Infrastructure (Week 1) — **DONE**

**Goal:** DatabaseManager + JsonDatabase + LTSL bindings working.

Delivered:
1. **`DatabaseManager.{h,cpp}`** — singleton, `Load`/`Find`/`FindPath`/`Keys`/`Erase`/`Reload`/`LoadFromString`
2. **`JsonDatabase.{h,cpp}`** — load, index, lookup (non-throwing parse, `json::parse(str, nullptr, false)`)
3. **LTSL bindings** — `Database_Load`, `Database_Get`, `Database_GetPath`, `Database_Has`, `Database_HasDatabase`, `Database_Keys`, `Database_Reload`
4. **`JsonHelpers.h`** — shared `JGet`, `JColor`, `JRange`, `JFloat`, `JBool`, `JInt`, `JVec3`
5. **Unit tests** — `TestJsonDatabase.cpp` (17 tests, 668+ checks)
6. **Wire into `System.cpp`** — `EnsureStarsLoaded()` lazy-loads `stars.json`

**Deliverable:** `Database_Load "stars" "resource/gamedata/stars.json"` works
from C++; `Database_GetPath "stars" "defaults.brightnessRange"` works from
LTSL. Stars and planets are data-driven.

#### Phase 2: Content Databases (Week 2–3) — **IN PROGRESS**

**Goal:** Ship/weapon/planet/station data moved to JSON.

1. **`ships.json`** — migrate `ShipType.cpp` hardcoded values
2. **`weapons.json`** — migrate the 10 multiplier tables
3. **`planets.json`** — migrate `PlanetType.cpp` ranges + add biome enum
4. **`stations.json`** — migrate `StationType.cpp` hardcoded values
5. **Update C++ factories** — `Item_ShipType()`, `Item_WeaponType()`,
   `Item_PlanetType()`, `Item_StationType()` read from databases instead
   of hardcoded constants
6. **Update `config.json`** — replace `gameConfig.txt` with JSON config
7. **Update `Config.lts`** — read JSON config instead of flat text
8. **Integration tests** — verify all apps still run with JSON-driven data

**Deliverable:** Changing a value in `weapons.json` changes weapon behavior
without recompile.

#### Phase 3: Graphics & NPC (Week 3–4)

**Goal:** Deep graphics tuning + NPC behavior parameters wired.

1. **`graphics.json`** — expand to shader-level constants (§3.6)
2. **Wire graphics settings** — pass JSON values to shader uniforms at
   init via `DrawState_Link` or per-renderable uniform sets
3. **`npc.json`** — NPC behavior parameters (§3.9)
4. **Wire NPC settings** — pass values to `Component_Economy` and task
   spawning in `SystemPopulate.lts`
5. **`economy.json`** — commodity definitions, market parameters (§3.5)
6. **`audio.json`** — sound definitions, music tracks (§3.7)

**Deliverable:** All visual and behavioral parameters tunable via JSON.

#### Phase 4: Polish & Validation (Week 4)

**Goal:** Robust error handling, documentation, testing.

1. **JSON schema validation** — validate on load, clear error messages
2. **Default fallbacks** — missing fields use sensible defaults
3. **Documentation** — JSON schema docs, migration guide from hardcoded
4. **Full test suite** — all databases round-trip, all apps verified
5. **API-DB refresh** — regenerate after new bindings

### 2.3b Phases (engine visual enhancements)

#### Phase A: Star Rendering (Week 5)

**Goal:** Star visible as glowing disk + lens flare.

1. **`Renderable_StarDisk`** — procedural glowing sphere shader
2. **Wire lens flare** — connect existing `lensflare.jsl` to star position
3. **Read `star.*` from `graphics.json`** — color, size, brightness
4. **Unit test** — star renders, lens flare visible

#### Phase B: Planet Rotation + Cloud Animation (Week 5–6)

**Goal:** Planets rotate, clouds drift.

1. **Add `rotationSpeed`/`cloudWindSpeed` to planet renderables** —
   read from `planets.json` biome data
2. **Modify `planet.jsl`** — add `time` uniform, rotate lookup direction,
   offset cloud UV
3. **Modify `PlanetType.cpp`** — pass `time` and `rotationSpeed` to
   shader uniforms
4. **Verify** — planets visibly rotate, clouds drift at different speeds
   per biome

#### Phase C: Moons (Week 6–7, deferred if time tight)

**Goal:** Small orbiting bodies around planets.

1. **New `Object_Moon`** — simplified planet object type
2. **`MoonGenerator`** — spawn 0–3 moons per planet from `planets.json`
3. **Moon shader** — simplified surface cubemap (no atmosphere)
4. **Orbital update** — advance moon position each frame
5. **Integration test** — moons orbit planets, visible from player ship

---

## 5. Shader Uniform Wiring

The JSON layer needs to push values into shaders. Two approaches:

### 5.1 Global uniforms via DrawState_Link

For uniforms shared across all shaders (fog, star, scattering):
- Add a `GraphicsConfig` struct populated from `graphics.json` at startup
- `DrawState_Link()` already pushes `fogDensity`, `starColor`, `starPos`
  — extend to push scattering, nebula, and other globals
- Shaders declare the uniform; the engine fills it from config

### 5.2 Per-renderable uniforms

For per-planet values (rotation speed, cloud speed, biome colors):
- `PlanetType.cpp` already sets per-planet uniforms (`atmoDensity`,
  `cloudLevel`, `color1..4`) — extend to include `rotationSpeed`,
  `cloudWindSpeed`
- These are read from `planets.json` at planet creation time

### 5.3 Post-processing uniforms

For bloom, SSAO, vignette, etc.:
- Post-processing chain reads config values at init
- Some (bloom threshold, SSAO radius) are set once; others (vignette
  hardness) can be updated per-frame if needed

---

## 6. LTSL Language Improvements Needed

### 6.1 JsonValue Type

The `Database_Get` function returns a JSON value. LTSL needs a type to
hold this:

```cpp
// In LTE: new AutoClass
AutoClass(JsonValue,
  String, rawJson,     // serialized JSON string
  int, typeHint)       // 0=null, 1=bool, 2=int, 3=float, 4=string, 5=array, 6=object
```

**Access patterns in LTSL:**

```
var val (Database_Get "ships" "fighter")
val.type                    # returns type hint int
val.AsString                # extract as string (if type==4)
val.AsInt                   # extract as int (if type==2)
val.AsFloat                 # extract as float (if type==3)
val.AsBool                  # extract as bool (if type==1)
```

**Nested access** (`val.hull.hp`) requires a **dot-dispatch** on
`JsonValue` — the LTSL runtime must:
1. Call `val.AsObject` to get the underlying JSON
2. Look up the field name in the object
3. Return a new `JsonValue` wrapping the result

This is a **new expression node** or an extension to
`ExpressionDynamicDispatch`. Estimated effort: 2–3 days.

### 6.2 Array Iteration for JsonValue

To iterate over JSON arrays in LTSL:

```
var weapons (Database_Get "ships" "fighter" "weapons")
for i 0 i < weapons.ArraySize i.++
  var w (weapons.ArrayGet i)
  Log w.name
```

Needs `JsonValue_ArraySize` and `JsonValue_ArrayGet` bindings.

### 6.3 Config Replacement

The current `Config.lts` reads flat `key:value` text. With JSON config:

```
# New pattern
var config (Database_Get "config" "game")
var seed config.seed           # direct access
var credits config.playerCredits
```

No more `Config_Get` string parsing — direct typed access.

### 6.4 NPC AI Config Access

NPC parameters are read once at system creation and passed to scripts:

```
var npcConfig (Database_Get "npc" "mining")
var minerCount (npcConfig.shipsPerZone.Get 0)  # min ships per zone
var maxMiners (npcConfig.shipsPerZone.Get 1)    # max ships per zone
```

**LTSL effort:** ~1 week total for JsonValue type + dot-dispatch + array
support. This is the biggest LTSL improvement in this work item.

---

## 7. Risks & Mitigations

| Risk | Severity | Mitigation |
|------|----------|------------|
| **LTSL JsonValue complexity** — dot-dispatch on dynamic types is new terrain for the interpreter | High | Start with flat access (`Database_GetString`, `Database_GetInt`) as fallback; add dot-dispatch in Phase 1.4, test thoroughly. |
| **Performance** — JSON parse at startup adds load time | Low | Parse once, index in memory. Hot-reload only re-parses changed files. Profile to confirm <100ms for all databases. |
| **Shader uniform wiring** — pushing JSON values to GLSL uniforms | Medium | Use existing `DrawState_Link` pattern for globals; per-renderable uniforms already have a path in `PlanetType.cpp`. |
| **Backward compatibility** — existing `gameConfig.txt` users | Low | Keep `Config.lts` working alongside `config.json` during transition. Deprecate after one release. |
| **Data validation** — bad JSON crashes the engine | Medium | Validate on load with clear error messages (line number, field path). Use `json::parse(str, nullptr, false)` (non-throwing) pattern already proven in SaveGameJSON. |
| **Merge conflicts** — multiple mods editing same JSON | Low | JSON files are per-database; mods override individual keys, not entire files. (Full mod support deferred to ModManager in §3.3.) |
| **Schema drift** — JSON fields diverge from C++ expectations | Medium | Version each JSON file. C++ loaders check `version` field and reject incompatible schemas. Unit tests verify schema compliance. |
| **Scope creep** — economy/graphics/audio/NPC expanding 2.3 | Medium | Phase 3 is explicitly deferred if time runs over. Core (Phase 1–2) delivers the most value: ship/weapon/planet tuning. NPC and graphics tuning are additive, not blocking. |
| **Planet rotation performance** — rotating cubemap lookup per frame | Low | Single mat3 multiply per fragment. Negligible cost on modern GPUs. |
| **Moon orbital mechanics** — new object type complexity | Medium | Defer to late 2.3b. Planet rotation + cloud animation deliver most visual impact without moon system. |

---

## 8. Scope Items — What's In and What's Deferred

### In Scope — 2.3 (JSON layer, Weeks 1–4)

| System | JSON File | Value | Status |
|--------|-----------|-------|--------|
| Ship types | `ships.json` | Tuning ship balance without C++ | todo |
| Weapon types | `weapons.json` | Tuning weapon balance without C++ | todo |
| Planet biomes | `planets.json` | Biome variety without shader edits | partial |
| Station types | `stations.json` | Station variety without C++ | todo |
| Game config | `config.json` | Replaces gameConfig.txt | todo |
| Deep graphics tuning | `graphics.json` | Shader constants (nebula, scattering, dustfleck, post-processing) | todo |
| NPC behavior | `npc.json` | Mining/trading/piracy/patrol parameters | todo |
| Economy/commodities | `economy.json` | Trade goods, market dynamics | todo |
| Audio config | `audio.json` | Sound definitions, music tracks | todo |
| Star customization | `stars.json` | Star classes, brightness, pulse, lens flare | **done** |
| Database infrastructure | C++ loaders + LTSL bindings | Foundation for all above | **done** |

### In Scope — 2.3b (engine visuals, Weeks 5–7)

| System | Source | Value |
|--------|--------|-------|
| Star rendering | New `Renderable_StarDisk` + lens flare wiring | Star visible as glowing disk |
| Planet rotation | `planet.jsl` shader change | Surface rotates at biome-configurable speed |
| Cloud animation | `planet.jsl` shader change | Clouds drift at biome-configurable speed |
| Moons | New `Object_Moon` + `MoonGenerator` | Orbiting bodies around planets |

### Deferred to Future Work

| System | Effort | Dependency |
|--------|--------|------------|
| ModManager (scan `mods/`, parse `mod.json`) | 3–4 wk | JSON layer (2.3) |
| Mod hooks (`onGameStart`, `onSectorGenerate`) | — | ModManager |
| Mod manager UI | — | ModManager |
| Input rebinding (JSON-driven) | 2–3 wk | JSON layer (2.3) |
| PBR material system | 2–3 wk | graphics.json |
| Shadow mapping | ~2 wk | graphics.json |
| Volumetric nebula compute | 1 wk | nebula config in graphics.json |

---

## 9. What This Enables Downstream

Once the JSON layer exists:

1. **ROADMAP 2.4 (Asset Hot-Reload)** — `AssetWatcher` monitors JSON files,
   calls `DatabaseManager::Reload()` on change. Live tuning without restart.

2. **ROADMAP §3.4 Pass A (Config-Driven Counts)** — `universe.json` holds
   `numOfPlanets`, `numOfShips`, `numOfAsteroids`. `SystemPopulate.lts`
   reads from JSON instead of `gameConfig.txt`.

3. **ROADMAP §3.4 Pass B (Biomes + Visual Knobs)** — `planets.json` defines
   biome types with color palettes, atmosphere ranges, ring probability,
   rotation speed, cloud wind speed. `PlanetType.cpp` reads biome data
   from JSON.

4. **ROADMAP §3.3 (Modding)** — modders create `mods/mymod/gamedata/ships.json`
   that merges with or overrides the base game's ship definitions.

5. **ROADMAP §3.2 (PBR / Graphics)** — `graphics.json` holds PBR parameters
   (roughness defaults, metallic ranges, light intensity). Artists tweak
   without shader recompile.

6. **ROADMAP §3.6 (Content Wiring)** — `economy.json` defines commodities
   and market parameters. `audio.json` defines sound mappings. Both wire
   into the gameplay systems.

7. **NPC AI integration** (`docs/npc-ai-integration.md`) — `npc.json` drives
   the Phase 1–3 AI wiring: mining rates, trading thresholds, piracy
   aggression, patrol distances. Phase 3 dispatcher re-evaluation interval
   is configurable. Phase 2 economy allocator gets spawn/retire thresholds.

8. **Visual authoring** — artists control the entire visual pipeline
   from one JSON file: nebula density, atmosphere haze, dust colors,
   star brightness, cloud drift speed, planet rotation — no shader
   hunting required.

---

## 10. Testing Strategy

### Unit Tests (`TestJsonDatabase.cpp`)

- Load valid JSON file → database populated
- Load malformed JSON → graceful error, empty database
- Lookup existing key → returns correct value
- Lookup missing key → returns null/empty
- List keys → returns all top-level keys
- Schema version check → rejects incompatible versions
- Nested field access → returns correct sub-object
- Array access → returns correct elements

### Integration Tests

- All apps (`war`, `ltheory-main`, `dogfight`, etc.) run with JSON-driven
  data (no hardcoded fallbacks needed)
- Changing a value in `weapons.json` produces different weapon behavior
  (regression guard)
- `config.json` replaces `gameConfig.txt` with identical behavior
- `graphics.json` parameters affect rendering (nebula density changes
  visible in screenshot comparison)

### Manual Verification

- `python3 configure.py build` — clean build
- `python3 configure.py test` — all tests pass
- LSP smoke — baseline diagnostics unchanged
- Launch `ltheory-main` — zero compilation errors, zero crashes
- Visual check: star renders as visible disk (2.3b Phase A)
- Visual check: planets rotate, clouds drift (2.3b Phase B)

---

## 11. Summary

| Aspect | Detail |
|--------|--------|
| **Problem** | All game balance, visuals, and NPC behavior hardcoded in C++/GLSL |
| **Solution** | JSON database layer with C++ loaders + LTSL bindings + shader uniform wiring |
| **Effort** | 2.3: 3–4 weeks (core + graphics/NPC); 2.3b: 2–3 weeks (star surface/planet visuals/moons) |
| **Infrastructure** | `DatabaseManager` singleton, `JsonDatabase` wrapper, `JsonHelpers.h` shared helpers — **DONE** |
| **JSON files** | `stars.json` (done), `planets.json` (partial), `ships.json` / `weapons.json` / `stations.json` / `config.json` / `graphics.json` / `npc.json` / `economy.json` / `audio.json` / `universe.json` / `factions.json` (todo) |
| **LTSL bindings** | `Database_Load`, `Database_Get`, `Database_GetPath`, `Database_Has`, `Database_HasDatabase`, `Database_Keys`, `Database_Reload` — **DONE** |
| **Graphics coverage** | Nebula (3 constants), scattering (10 params), dustfleck (4 params), post-processing (12 params), star (6 params), ocean (2 params) |
| **NPC coverage** | Spawning, mining, trading, piracy, patrol, economy allocator, combat, behavior timing |
| **Engine changes (2.3b)** | Star surface shader (ROADMAP §2.5), planet rotation, cloud animation, moon system |
| **Risks** | Scope creep (low if phased); JsonValue dot-dispatch deferred (flat `Database_Get`/`Database_GetPath` sufficient) |
| **Enables** | Hot-reload, modding, visual authoring, NPC tuning, config-driven generation, PBR, economy |
