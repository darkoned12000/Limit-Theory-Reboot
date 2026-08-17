// Copyright (C) 2025  darkoned12000
// SPDX-License-Identifier: GPL-3.0-or-later
// Part of the ltheory-old-test modernization effort (Revamp Work).
// See NOTICE and LICENSE.GPL. Original engine (c) Josh Parnell, public domain.

# JSON Data Layer Design — ROADMAP 2.3

Comprehensive plan for making the engine data-driven via JSON configuration
files. This document covers scope, architecture, schemas, phased delivery,
risks, and LTSL implications.

---

## 1. What This Solves

### Current Problem

All game balance, type definitions, and tuning parameters are hardcoded in
C++:

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
- **GLSL shaders** — scattering coefficients, fog density, gamma, ocean
  color all baked into `.jsl` files.
- **gameConfig.txt** — flat `key:value` text, 23 keys, no hierarchy.

**Impact:** Every balance change, new weapon class, or planet biome
variation requires a C++ recompile. Iteration is slow. Modding is
impossible without source access.

### What a JSON Layer Unlocks

1. **Instant iteration** — designers tweak JSON, reload, see changes.
   No recompile cycle.
2. **Modding foundation** — modders drop JSON files into `mods/` to add
   ships, weapons, biomes without touching C++.
3. **Content diversity** — easy to create dozens of ship/weapon variants
   by tweaking numbers, not rewriting code.
4. **Separation of concerns** — game logic in C++, content data in JSON.
   Each evolves independently.
5. **Testing** — JSON schemas are self-documenting; validation catches
   errors at load time instead of runtime.

---

## 2. Architecture

### 2.1 Layer Model

```
┌─────────────────────────────────────────────────┐
│  LTSL Scripts (ltheory-main, SystemPopulate)    │
│  call Database_Get "ships" "fighter"             │
└──────────────────────┬──────────────────────────┘
                       │ LTSL binding
┌──────────────────────▼──────────────────────────┐
│  C++ Database Loaders (ShipDatabase, etc.)       │
│  - Parse JSON on load                            │
│  - Populate in-memory lookup tables              │
│  - Expose to LTSL via Function_Bind              │
└──────────────────────┬──────────────────────────┘
                       │ nlohmann/json (already vendored)
┌──────────────────────▼──────────────────────────┐
│  JSON Files (resource/gamedata/*.json)           │
│  - ships.json, weapons.json, planets.json, ...   │
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
`JsonValue` type (see §5 for LTSL improvements needed). Scripts access
fields via dot notation:

```
var ship (Database_Get "ships" "fighter")
var hullHP ship.hull.hp      # nested access
var thrusters ship.thrusters  # array access
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
  planets.json         ← planet biome definitions + generation params
  stations.json        ← station type definitions
  economy.json         ← commodity definitions + market parameters
  factions.json        ← faction definitions + relationships
  universe.json        ← region structure, connectivity, asteroid density
  graphics.json        ← rendering settings (fog, scattering, bloom, etc.)
  audio.json           ← sound definitions, music tracks, volume defaults
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
      "hasRings": false,
      "description": "Arid, barren world with minimal atmosphere"
    },
    "terran": {
      "name": "Terran",
      "surfaceTint": [0.3, 0.6, 0.2],
      "atmoDensityRange": [0.5, 1.5],
      "oceanLevel": 0.3,
      "cloudLevel": 0.1,
      "hasRings": false,
      "description": "Earth-like world with oceans and vegetation"
    },
    "ice": {
      "name": "Ice",
      "surfaceTint": [0.7, 0.8, 0.9],
      "atmoDensityRange": [0.2, 0.8],
      "oceanLevel": 0.0,
      "cloudLevel": 0.0,
      "hasRings": true,
      "description": "Frozen world with thick ice crust"
    },
    "lava": {
      "name": "Lava",
      "surfaceTint": [0.9, 0.2, 0.1],
      "atmoDensityRange": [1.0, 2.0],
      "oceanLevel": 0.0,
      "cloudLevel": -0.2,
      "hasRings": false,
      "description": "Volcanic world with molten surface"
    },
    "gas_giant": {
      "name": "Gas Giant",
      "surfaceTint": [0.6, 0.5, 0.4],
      "atmoDensityRange": [1.5, 2.0],
      "oceanLevel": 0.0,
      "cloudLevel": 0.15,
      "hasRings": true,
      "description": "Massive gas giant with prominent ring system"
    }
  }
}
```

**Replaces:** hardcoded ranges in `PlanetType.cpp` lines 140–158 + adds
biome categorization (currently absent — all planets are "desert").

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

### 3.5 economy.json (deferred — scope item)

```json
{
  "version": 1,
  "commodities": {
    "ore": { "name": "Ore", "basePrice": 10, "stackSize": 100 },
    "fuel": { "name": "Fuel Cells", "basePrice": 25, "stackSize": 50 },
    "components": { "name": "Components", "basePrice": 100, "stackSize": 20 }
  },
  "market": {
    "markupRange": [0.8, 1.5],
    "demandDecayRate": 0.01,
    "supplyReplenishRate": 0.005
  }
}
```

**Replaces:** `Item_Commodity` stub (currently `NOT_IMPLEMENTED`).

### 3.6 graphics.json (deferred — scope item)

```json
{
  "version": 1,
  "rendering": {
    "farPlane": 1000000,
    "nearPlane": 0.05,
    "gamma": 2.2,
    "vsync": false
  },
  "fog": {
    "density": 0.0,
    "color": [0.0, 0.0, 0.0]
  },
  "scattering": {
    "densityMult": 50.0,
    "scale": 0.025,
    "planetRadius": 50000,
    "rayleigh": [0.005, 0.009, 0.004],
    "mie": [0.002, 0.002, 0.002]
  },
  "bloom": {
    "enabled": false,
    "threshold": 1.0,
    "strength": 0.5
  },
  "ssao": {
    "enabled": false,
    "radius": 0.5,
    "intensity": 1.0
  }
}
```

**Replaces:** hardcoded constants in GLSL shaders (`global.jsl`,
`lighting.jsl`, `scattering.jsl`, `planet.jsl`).

### 3.7 audio.json (deferred — scope item)

```json
{
  "version": 1,
  "music": {
    "exploration": { "tracks": ["music/explore1.ogg"], "volume": 0.04 },
    "combat": { "tracks": ["music/combat1.ogg"], "volume": 0.08 },
    "docking": { "tracks": ["music/dock1.ogg"], "volume": 0.03 }
  },
  "sfx": {
    "weaponFire": { "beam": "sfx/beam_fire.ogg", "missile": "sfx/missile_fire.ogg" },
    "explosion": { "small": "sfx/explode_small.ogg", "large": "sfx/explode_large.ogg" },
    "ui": { "click": "sfx/ui_click.ogg", "hover": "sfx/ui_hover.ogg" }
  }
}
```

**Replaces:** hardcoded sound paths in `WeaponType.cpp` lines 192–199.

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

---

## 4. Phased Implementation

### Phase 1: Core Infrastructure (Week 1)

**Goal:** DatabaseManager + JsonDatabase + LTSL bindings working.

1. **`DatabaseManager.{h,cpp}`** — singleton, `Load`/`Get`/`Has`/`Keys`
2. **`JsonDatabase.{h,cpp}`** — load, index, lookup
3. **LTSL bindings** — `Database_Load`, `Database_Get`, `Database_Has`,
   `Database_Keys`
4. **`JsonValue` type** — new LTSL type wrapping `nlohmann::json` (see §5)
5. **Unit tests** — `TestJsonDatabase.cpp` (load, lookup, missing key,
   version check, malformed JSON)
6. **Wire into `Launcher::Launch()`** — load all databases at startup

**Deliverable:** `Database_Load "ships" "gamedata/ships.json"` works from
LTSL; `Database_Get "ships" "fighter"` returns a value.

### Phase 2: Content Databases (Week 2–3)

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

### Phase 3: Scope Extensions (Week 3–4)

**Goal:** Economy, graphics, audio configs wired.

1. **`economy.json`** — commodity definitions, market parameters
2. **`graphics.json`** — rendering settings (fog, scattering, bloom, SSAO)
3. **`audio.json`** — sound definitions, music tracks, volume defaults
4. **`universe.json`** — region structure, connectivity, asteroid density
5. **`factions.json`** — faction definitions, relationships, colors
6. **Wire graphics settings** — pass JSON values to shader uniforms at init
7. **Wire audio settings** — load sound definitions from JSON

**Deliverable:** All major game parameters configurable via JSON.

### Phase 4: Polish & Validation (Week 4)

**Goal:** Robust error handling, documentation, testing.

1. **JSON schema validation** — validate on load, clear error messages
2. **Default fallbacks** — missing fields use sensible defaults
3. **Documentation** — JSON schema docs, migration guide from hardcoded
4. **Full test suite** — all databases round-trip, all apps verified
5. **API-DB refresh** — regenerate after new bindings

---

## 5. LTSL Language Improvements Needed

### 5.1 JsonValue Type

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

### 5.2 Array Iteration for JsonValue

To iterate over JSON arrays in LTSL:

```
var weapons (Database_Get "ships" "fighter" "weapons")
for i 0 i < weapons.ArraySize i.++
  var w (weapons.ArrayGet i)
  Log w.name
```

Needs `JsonValue_ArraySize` and `JsonValue_ArrayGet` bindings.

### 5.3 Config Lts Replacement

The current `Config.lts` reads flat `key:value` text. With JSON config:

```
# New pattern
var config (Database_Get "config" "game")
var seed config.seed           # direct access
var credits config.playerCredits
```

No more `Config_Get` string parsing — direct typed access.

**LTSL effort:** ~1 week total for JsonValue type + dot-dispatch + array
support. This is the biggest LTSL improvement in this work item.

---

## 6. Risks & Mitigations

| Risk | Severity | Mitigation |
|------|----------|------------|
| **LTSL JsonValue complexity** — dot-dispatch on dynamic types is new terrain for the interpreter | High | Start with flat access (`Database_GetString`, `Database_GetInt`) as fallback; add dot-dispatch in Phase 1.4, test thoroughly. |
| **Performance** — JSON parse at startup adds load time | Low | Parse once, index in memory. Hot-reload only re-parses changed files. Profile to confirm <100ms for all databases. |
| **Backward compatibility** — existing `gameConfig.txt` users | Low | Keep `Config.lts` working alongside `config.json` during transition. Deprecate after one release. |
| **Data validation** — bad JSON crashes the engine | Medium | Validate on load with clear error messages (line number, field path). Use `json::parse(str, nullptr, false)` (non-throwing) pattern already proven in SaveGameJSON. |
| **Merge conflicts** — multiple mods editing same JSON | Low | JSON files are per-database; mods override individual keys, not entire files. (Full mod support deferred to ModManager in §3.3.) |
| **Schema drift** — JSON fields diverge from C++ expectations | Medium | Version each JSON file. C++ loaders check `version` field and reject incompatible schemas. Unit tests verify schema compliance. |
| **Scope creep** — economy/graphics/audio not originally in 2.3 | Medium | Phase 3 is explicitly deferred if time runs over. Core (Phase 1–2) delivers the most value: ship/weapon/planet tuning. |

---

## 7. Scope Items — What's In and What's Deferred

### In Scope (Phase 1–2)

| System | JSON File | Value |
|--------|-----------|-------|
| Ship types | `ships.json` | Tuning ship balance without C++ |
| Weapon types | `weapons.json` | Tuning weapon balance without C++ |
| Planet biomes | `planets.json` | Biome variety without shader edits |
| Station types | `stations.json` | Station variety without C++ |
| Game config | `config.json` | Replaces gameConfig.txt |
| Database infrastructure | C++ loaders + LTSL bindings | Foundation for all above |

### Deferred to Phase 3 (if time permits)

| System | JSON File | Value |
|--------|-----------|-------|
| Economy/commodities | `economy.json` | Trading, market dynamics |
| Graphics settings | `graphics.json` | Fog, scattering, bloom, SSAO tunable |
| Audio config | `audio.json` | Sound definitions, music tracks |
| Universe generation | `universe.json` | Region structure, asteroid density |
| Faction definitions | `factions.json` | Faction relationships, colors |

### Deferred to Future Work (ROADMAP §3.3)

| System | Effort | Dependency |
|--------|--------|------------|
| ModManager (scan `mods/`, parse `mod.json`) | 3–4 wk | JSON layer (this item) |
| Mod hooks (`onGameStart`, `onSectorGenerate`) | — | ModManager |
| Mod manager UI | — | ModManager |
| Input rebinding (JSON-driven) | 2–3 wk | JSON layer (this item) |

---

## 8. What This Enables Downstream

Once the JSON layer exists:

1. **ROADMAP 2.4 (Asset Hot-Reload)** — `AssetWatcher` monitors JSON files,
   calls `DatabaseManager::Reload()` on change. Live tuning without restart.

2. **ROADMAP §3.4 Pass A (Config-Driven Counts)** — `universe.json` holds
   `numOfPlanets`, `numOfShips`, `numOfAsteroids`. `SystemPopulate.lts`
   reads from JSON instead of `gameConfig.txt`.

3. **ROADMAP §3.4 Pass B (Biomes + Visual Knobs)** — `planets.json` defines
   biome types with color palettes, atmosphere ranges, ring probability.
   `PlanetType.cpp` reads biome data from JSON.

4. **ROADMAP §3.3 (Modding)** — modders create `mods/mymod/gamedata/ships.json`
   that merges with or overrides the base game's ship definitions.

5. **ROADMAP §3.2 (PBR / Graphics)** — `graphics.json` holds PBR parameters
   (roughness defaults, metallic ranges, light intensity). Artists tweak
   without shader recompile.

6. **ROADMAP §3.6 (Content Wiring)** — `economy.json` defines commodities
   and market parameters. `audio.json` defines sound mappings. Both wire
   into the gameplay systems.

---

## 9. Testing Strategy

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

### Manual Verification

- `python3 configure.py build` — clean build
- `python3 configure.py test` — all tests pass
- LSP smoke — baseline diagnostics unchanged
- Launch `ltheory-main` — zero compilation errors, zero crashes

---

## 10. Summary

| Aspect | Detail |
|--------|--------|
| **Problem** | All game balance hardcoded in C++; no iteration without recompile |
| **Solution** | JSON database layer with C++ loaders + LTSL bindings |
| **Effort** | 3–4 weeks (Phase 1–2 core; Phase 3–4 extensions) |
| **Infrastructure** | `DatabaseManager` singleton, `JsonDatabase` wrapper, `JsonValue` LTSL type |
| **JSON files** | 7–10 files in `resource/gamedata/` (ships, weapons, planets, stations, config, economy, graphics, audio, universe, factions) |
| **LTSL improvements** | `JsonValue` type with dot-dispatch, array access, type introspection |
| **Risks** | JsonValue complexity (medium), data validation (medium), scope creep (low if phased) |
| **Enables** | Hot-reload, modding, config-driven generation, PBR tuning, economy system |
