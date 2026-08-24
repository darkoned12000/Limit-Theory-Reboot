// Copyright (C) 2025  darkoned12000
// SPDX-License-Identifier: GPL-3.0-or-later
// Part of the ltheory-old-test modernization effort (Revamp Work).
// See NOTICE and LICENSE.GPL. Original engine (c) Josh Parnell, public domain.

# JSON Ships Review — Research for ships.json Implementation

Full research on how ships, weapons, stations, and their sub-items are
created in the engine. Covers hardcoded values, factory signatures, mesh
generation, spawning patterns, and the dead-parameter inventory. Reference
for the ships.json effort (ROADMAP §2.3 Phase 2).

---

## 1. Item_ShipType — The Ship Factory

**File:** `src/liblt/Game/Item/ShipType.cpp`

### 1.1 Signature

```cpp
// Items.h line 188
Item_ShipType_Args:
  double  value          // total "budget" in abstract Resource Units (RU)
  uint    seed           // RNG seed for procedural generation
  float   capacity       // cargo capacity multiplier (default 1.0)
  float   compactness    // mass multiplier (default 1.0)
  float   integrity      // health multiplier (default 1.0)
  float   propulsion     // thruster budget multiplier (default 1.0)  [DEAD — never read]
  float   systems        // generator/scanner budget multiplier (default 1.0)  [DEAD]
  float   turrets        // turret count multiplier (default 1.0)  [DEAD]
```

Convenience 2-arg overload: `Item_ShipType(value, seed)` passes all floats
as 1. Returns `Item` (Reference<ItemT>).

### 1.2 How It Works (lines 185–263)

1. **RNG seeded** from `args.seed` via `RNG_MTG`.
2. **Value budget split** (HARDCODED ratios):
   - `scannerValue = 1000.0` (fixed constant)
   - `hullValue = 0.6 * valueRemaining` (60% of total budget)
   - `thrusterValue = Saturate(0.5 / Sqrt(Sqrt(valueRemaining / 10000.0))) * valueRemaining`
   - `generatorValue = valueRemaining` (whatever is left)
3. **Derived stats** via `Constants.h`:
   - `capacity = Constant_ValueToCapacity(hullValue, args.capacity)` = `(hullValue * capacity / compactness) / 100`
   - `integrity = Constant_ValueToIntegrity(hullValue, args.integrity)` = `(hullValue * strength / compactness) / 100`
   - `mass = Constant_ValueToMass(hullValue, args.compactness)` = `(hullValue / compactness) / 10`
   - `scale = Constant_MassToScale(mass)` = `Pow(mass / 1000, 0.5)`
4. **Hull mesh** — calls LTSL `Item/ShipType/Generate:Main(scale, seed)`.
5. **Socket placement** — ray-casts against mesh bounding box for surface normals.

### 1.3 Hardcoded Values

| Parameter | Value | Where |
|-----------|-------|-------|
| Hull value fraction | 60% of budget | ShipType.cpp:191 |
| Scanner value | 1000.0 (fixed) | ShipType.cpp:189 |
| Scanner subtracted from budget | **NO** — line 190 is commented out | ShipType.cpp:190 |
| Thruster count | 2 base pairs + directional extras | ShipType.cpp:214 |
| Turret count | 4 (args.turrets ignored) | ShipType.cpp:215 |
| Generator count | `int(rng(1,2) + logScale)` | ShipType.cpp:216 |
| Interior count | `2 * int(logScale / log(10))` | ShipType.cpp:217 |
| Thruster placement tolerance | 0.8 dot product, 10 attempts | ShipType.cpp:223+ |
| Turret placement attempts | 100 per turret | ShipType.cpp:240+ |
| Ship name | Always "Ship" | ShipType.cpp:260 |
| Material | `Material_Metal` | ShipType.cpp:209 |

### 1.3a Thruster Value Formula

The thruster budget is NOT a fixed fraction — it's inversely scaled by
ship size:

```
thrusterValue = Saturate(0.5 / Sqrt(Sqrt(valueRemaining / 10000.0)))
                * valueRemaining
```

| Value | valueRemaining (after hull) | Thruster ratio | Thruster RU | Generator RU |
|-------|---------------------------|----------------|-------------|--------------|
| 10,000 | 4,000 | 50.0% | 2,000 | 2,000 |
| 100,000 | 40,000 | 34.3% | 13,726 | 26,274 |
| 1,000,000 | 400,000 | 18.7% | 74,787 | 325,213 |
| 10,000,000 | 4,000,000 | 10.0% | 400,000 | 3,600,000 |

Result: Small ships (fighters) get roughly equal thruster/generator budgets,
while large ships (battleships/capitals) get 90% generator and only 10%
thruster — consistent with them being slow and energy-heavy.

**Decision:** This formula stays in C++ — it's a smooth curve that doesn't
benefit from being data-driven. JSON controls the `compactnessMult` which
is the primary maneuverability lever; the thruster ratio is an emergent
property of ship size.

### 1.4 Dead Parameters

`propulsion`, `systems`, `turrets` are declared in the Args struct and
bound to LTSL, but the C++ implementation **never reads them**. They are
completely ignored.

### 1.5 Standard Equipment (Instantiate, line 59–65)

When a ship is instantiated from its type:
- All thruster sockets filled by calling `ship->Plug(standardThruster)`
- One generator plugged into a generator socket
- One scanner plugged into a generator socket

---

## 2. Item_WeaponType — The Weapon Factory

**File:** `src/liblt/Game/Item/WeaponType.cpp`

### 2.1 Signature

Unlike all other Item_*Type functions, `Item_WeaponType` takes **only a
single integer seed**:

```cpp
Item_WeaponType(int id)
```

**No value/budget parameter.** All stats are derived from hardcoded tables
+ RNG.

### 2.2 How It Works (lines 147–241)

1. **Renderable** is a static singleton (generated once via
   `Item/WeaponType:Generate` script).
2. **Weapon class selection** via cumulative probability from
   `kAmmoProbabilityMult`:
   - Beam (0): probability 0.0
   - Missile (1): probability 0.0
   - Pulse (2): probability 1.0
   - Rail (3): probability 0.0
   - **Result: ALL weapons are Pulse type.** Beam, Missile, and Rail are
     effectively disabled.
3. **Color:** Random from hue-saturation range (0.25 white base + random hue).
4. **Damage:** `Constant_AmmoDamageMult() * kAmmoDamageMult[type] * Sigfigs(1 + 15*exp, 2)`.

### 2.3 Per-Class Multiplier Tables (HARDCODED)

| Table | Beam | Missile | Pulse | Rail |
|-------|------|---------|-------|------|
| MagazineSizeMult | 0 | 1 | 6 | 10 |
| MagazineProbability | 0 | 1 | 0.1 | 0.9 |
| PowerDrainMult | 5 | 0 | 2 | 1 |
| RateMult | 1 | 0.01 | 1 | 1 |
| SpreadMult | 0 | 1 | 2 | 5 |
| WeightMult | 5 | 3 | 2 | 1 |
| AmmoDamageMult | 5 | 20 | 2 | 1 |
| AmmoLifeMult | 2.5 | 10 | 1.25 | 1 |
| AmmoProbabilityMult | 0.0 | 0.0 | 1.0 | 0.0 |
| AmmoSpeedMult | 1e10 | 1 | 1 | 1e10 |

### 2.4 Other Hardcoded Weapon Values

| Parameter | Value | Where |
|-----------|-------|-------|
| Integrity | 100 (fixed) | WeaponType.cpp:186 |
| Scale | 0.5 (fixed) | WeaponType.cpp:187 |
| Offset | V3(0, 0.5, 4) (fixed) | WeaponType.cpp:188 |
| Magazine time | `Round(rng(6, 10))` if uses > 0 | WeaponType.cpp:223 |
| Rate boost | 1.5× if weapon has magazine | WeaponType.cpp:233 |
| Name | Generated via `Grammar_Get "$weapon"` | WeaponType.cpp:214 |
| Sound | Only Pulse weapons get sound (6 hardcoded .ogg files) | WeaponType.cpp:192-201 |
| Fire volume | 0.1 (very quiet — §13 root cause) | WeaponType.cpp:90 |
| Color formula | `0.25 * White + HSV(rng, 0.6-0.99 sat, 0.2-0.6 val)` | WeaponType.cpp:166-169 |

The color formula is HSL-based: starts with a desaturated white base and
adds a seeded random hue. Not data-driven — could be per-class in Phase 2
but currently produces similar colors for all weapon types.

### 2.5 Instantiation (line 141–145)

Creates a `TurretType` with 1 socket, `kPi` tracking speed, then plugs an
`Object_Weapon` into it.

### 2.6 Rendering Model

Two mesh groups — mesh 0 = barrel (full motion), mesh 1 = base (yaw only).
Visual is a simple 2-box PlateMesh from `Item/WeaponType.lts`.

---

## 3. Item_StationType — The Station Factory

**File:** `src/liblt/Game/Item/StationType.cpp`

### 3.1 Signature

```cpp
Item_StationType_Args:
  double  value
  uint    seed
  float   capacity    // cargo multiplier
  float   integrity   // health multiplier
  float   systems     // [DEAD — never read]
  float   turrets     // [DEAD — never read]
```

### 3.2 Hardcoded Values

| Parameter | Value | Where |
|-----------|-------|-------|
| Mass | `10.0 * Constant_ValueToMass(args.value)` (10× normal) | StationType.cpp:195 |
| Dock capacity | 100 (fixed) | StationType.cpp:202 |
| Dock offset | V3(0, 5, 2) (fixed) | StationType.cpp:203 |
| Name | Always "Station" | StationType.cpp:210 |
| Material | `Material_Metal` | StationType.cpp |
| Hull mesh | `Item/StationType/Generate:Main` (LTSL script) | StationType.cpp |
| Interior model | Shell SDF with cylindrical hole | StationType.cpp |

### 3.3 Dead Parameters

`systems` and `turrets` — declared in Args, bound to LTSL, never read in C++.

---

## 4. Sub-Item Factories

### 4.1 Item_PowerGeneratorType

**File:** `src/liblt/Game/Item/PowerGeneratorType.cpp`

- Args: `(value, seed)`
- Capability: `Capability_Power(Constant_ValueToOutput(value))` = `value * rate`
- Name: "Power Generator"
- Simplest factory — just sets value, icon, name.

### 4.2 Item_ScannerType

**File:** `src/liblt/Game/Item/ScannerType.cpp`

- Args: `(value, seed, range=1)`
- Range: `Constant_RangeRatio(range)` = `range * 1000`
- Power drain: hardcoded `1.0f`
- Name: "Scanner"

### 4.3 Item_ThrusterType

**File:** `src/liblt/Game/Item/ThrusterType.cpp`

- Args: `(value, seed, compactness=1, efficiency=1, integrity=1, rate=1)`
- Output: `kThrustMult * Constant_ValueToOutput(value, rate)` = `100 * value * rate`
- Color: hardcoded `Color(1.0, 0.4, 0.1)` (orange)
- Visual: small PlateMesh from `Item/ThrusterType.lts`

### 4.4 Item_TurretType

**File:** `src/liblt/Game/Item/TurretType.cpp`

- Args: `(sockets, trackingSpeed)` but **completely ignored** — returns a singleton
- Always has 1 socket with `JointType::AxisX`
- Visual: SDF-based model from `Item/TurretType:Generate`

---

## 5. Constants.h — Derived Stat Formulas

| Function | Formula | Purpose |
|----------|---------|---------|
| `Constant_ValueToMass(x, compactness)` | `(x / compactness) / 10` | RU to mass |
| `Constant_ValueToCapacity(x, capacity)` | `(x * capacity / compactness) / 100` | RU to cargo capacity |
| `Constant_ValueToIntegrity(x, strength)` | `(x * strength / compactness) / 100` | RU to health |
| `Constant_MassToScale(x)` | `Pow(x / 1000, 0.5)` | Mass to visual scale |
| `Constant_ValueToOutput(x, rate)` | `x * rate` | RU to power/thrust output |
| `Constant_AmmoDamageMult()` | `2` | Weapon damage baseline |
| `Constant_AmmoLifeMult()` | `4` | Projectile lifetime baseline |
| `Constant_AmmoSpeedMult()` | `500` | Projectile speed baseline |
| `Constant_WeaponRateMult()` | `0.1` | Fire rate baseline |
| `Constant_WeaponSpreadMult()` | `0.001` | Spread baseline |
| `Constant_RangeRatio(range)` | `range * 1000` | Scanner range |

---

## 6. Ship Hull Mesh Generation

**File:** `resource/script/Item/ShipType/Generate.lts`

The function `Main(Float scale, Int seed)` procedurally generates a ship
hull using PlateMesh:

1. **Plate count:** `plates = 2 + int(Sqrt(scale))` — more plates for
   larger ships.
2. **Starts with 2 base boxes** at origin (one wide/flat `2.0 x 0.5 x 0.5`,
   one tall/narrow `0.5 x 1.0 x 2.0`).
3. **Iteratively grows** by picking random existing boxes, choosing a random
   axis face, and placing a new box adjacent to that face (1–5 repetitions
   for longer segments). Boxes constrained to `[-3,-2,-6]` to `[3,2,6]`.
4. **Passive boxes** (50% size) added in parallel for surface detail.
5. **Symmetry:** Each box added twice — once at `+X` and once at `-X` (0.99
   scale factor for mirror) — bilateral symmetry.
6. **Wings:** If `scale >= 10.0`, extra boxes added below hull at `-0.5Y`
   with wider dimensions.
7. **Warp operations:**
   - `VerticalCompress` — compresses bottom for flatter underside
   - `HExpand` — widens front section
   - `Warp_AttractorPoint` pairs — currently disabled (`rng.Float < 0.0`)
8. **Mesh quality:** `MeshQuality()` returns 12 (hardcoded).
9. **Material:** `Material_Metal` (hardcoded).

There is a commented-out decal system (Chinese text overlay) at lines 91–100.

---

## 7. How Ships Are Spawned in Apps

### 7.1 war.lts

```lts
# Player ship
var shipType (Item_ShipType 10000 20 1 1 1 1 1 1)  # value=10000, seed=20
var ship shipType.Instantiate
# 4x weapons plugged in

# AI ships (32 total, hardcoded ShipCount = 32)
var type1 (Item_ShipType 10000 20 1 1 1 1 1 1)    # small
var type2 (Item_ShipType 100000 20 1 1 1 1 1 1)   # medium
var type3 (Item_ShipType 1000000 20 1 1 1 1 1 1)  # large
var type4 (Item_ShipType 10000000 20 1 1 1 1 1 1) # capital
# Each AI gets 4x random weapons, Task_Destroy targeting random other ship
```

### 7.2 ltheory-main.lts

```lts
# Player ship
var shipType (Item_ShipType shipHull 55 1 1 1 1 1 1)  # shipHull from config
var ship shipType.Instantiate
# All turret sockets filled via while loop

# AI ships (12 total)
var type1 (Item_ShipType 10000 20 1 1 1 1 1 1)
var type2 (Item_ShipType 100000 20 1 1 1 1 1 1)
var type3 (Item_ShipType 1000000 20 1 1 1 1 1 1)
# Each AI gets all turret sockets filled
```

### 7.3 SystemPopulate.lts

Does **NOT** create any AI ships. Only creates:
- 1 planet
- 30,000 asteroids in 3 shell layers

AI ships are created directly in the app's `Initialize` function.

---

## 8. Ship Object Creation

**File:** `src/liblt/Game/Object/Ship.cpp`

`Object_Ship(Item const& type)`:
1. Creates a `Ship` object with 22 components (Affectable, Asset,
   BoundingBox, Cargo, Collidable, Crew, Cullable, Database, Detectable,
   Drawable, Explodable, Integrity, Motion, MotionControl, Nameable,
   Orientation, Pilotable, Scriptable, Sockets, Supertyped, Targets, Tasks).
2. Sets the ship type as its supertype.
3. Calls `Object/Ship:Init` script which adds the `ShipUpdate` scriptable
   component (ambient interior sound + engine loop sound).
4. Motion mass is computed per frame: `GetSupertype()->GetMass() + Cargo.currentMass`.

---

## 9. Dead / Ignored Parameters Summary

| Factory | Dead Params |
|---------|-------------|
| `Item_ShipType` | `propulsion`, `systems`, `turrets` — declared in Args, bound to LTSL, never read |
| `Item_StationType` | `systems`, `turrets` — same situation |
| `Item_TurretType` | `sockets`, `trackingSpeed` — returns a singleton regardless |
| `Item_WeaponType` | No dead params (but entire class selection is rigged: 100% Pulse) |

---

## 10. Ship Editor / Creation Tool

**There is no ship editor or creation tool in this codebase.** Ships are
entirely procedurally generated from `(value, seed, <multipliers>)`
parameters. The only visual tweaking possible would be through the F2
DevPanel (documented as a future goal in AGENTS.md §8.3 Pass C).

The hull mesh generator (`Item/ShipType/Generate.lts`) is the closest
thing to a "ship creation tool" — it takes `(scale, seed)` and produces a
PlateMesh via iterative box placement with bilateral symmetry.

---

## 11. ships.json — Schema & Implementation Plan

**Status:** JSON file created (`resource/gamedata/ships.json`, schema v1).
C++ wiring not yet implemented — see Migration Strategy below.

### Scope Decision: ALL Ships

The same `Item_ShipType` factory creates both player and AI ships. There
is no separate "player ship" vs "AI ship" path — the only difference is
the `value` and `seed` arguments passed at the call site. Therefore
`ships.json` should define **ship archetypes** that apply to all ships.

### Ship Physics Model (for reference)

The existing physics model already uses mass for movement:
- `mass = hullValue / (compactness * 10)` + cargo weight (dynamic)
- `inertia = mass^1.25` (superlinear — heavier ships turn much slower)
- Top speed = `totalThrust / (0.8 * mass)`
- Max angular acceleration = `(totalThrust / 32) / mass^1.25`
- Angular terminal velocity = `torque / (inertia * 2.0)`

The `compactness` parameter is the primary maneuverability lever —
higher compactness = less mass for same value = better thrust-to-weight.
Cargo adds mass every frame, degrading all maneuverability.

### Proposed Schema

```json
{
  "version": 2,
  "defaults": {
    "hullValueRatio": 0.6,
    "scannerValue": 1000,
    "thrusterCount": 2,
    "turretCount": 4,
    "interiorCountFormula": "2 * (logScale / log10)",
    "generatorCountFormula": "1..2 + logScale",
    "material": "Metal",
    "meshQuality": 12,
    "armorRating": 0,
    "shieldIntegrityMult": 0.0,
    "shieldChargeTime": 60.0,
    "shieldRestoreFraction": 0.25,
    "shieldColor": [0.3, 0.6, 1.8],
    "shieldIdleOpacity": 0.0,
    "hullTint": [1.0, 1.0, 1.0],
    "thrusterColor": [1.0, 0.4, 0.1]
  },
  "shipArcheTypes": {
    "scout": {
      "name": "Scout",
      "valueRange": [5000, 15000],
      "capacityMult": 0.5,
      "compactnessMult": 0.7,
      "integrityMult": 0.6,
      "armorRating": 0,
      "shieldIntegrityMult": 0.5,
      "turretCount": 1,
      "hullTint": [0.8, 0.9, 1.0],
      "thrusterColor": [0.4, 0.7, 1.0],
      "description": "Fast, lightly armored scout vessel. Low hull, minimal shields, high agility."
    },
    "fighter": {
      "name": "Fighter",
      "valueRange": [10000, 50000],
      "capacityMult": 1.0,
      "compactnessMult": 1.0,
      "integrityMult": 1.0,
      "armorRating": 1,
      "shieldIntegrityMult": 1.0,
      "turretCount": 2,
      "hullTint": [1.0, 1.0, 1.0],
      "thrusterColor": [1.0, 0.4, 0.1],
      "description": "Standard combat fighter. Balanced hull, shields, and maneuverability."
    },
    "corvette": {
      "name": "Corvette",
      "valueRange": [50000, 150000],
      "capacityMult": 1.2,
      "compactnessMult": 1.1,
      "integrityMult": 1.8,
      "armorRating": 3,
      "shieldIntegrityMult": 1.5,
      "turretCount": 4,
      "hullTint": [0.9, 0.9, 0.95],
      "thrusterColor": [1.0, 0.5, 0.2],
      "description": "Light combat vessel. Heavier than a fighter, tougher hull, more hardpoints."
    },
    "freighter": {
      "name": "Freighter",
      "valueRange": [80000, 300000],
      "capacityMult": 3.0,
      "compactnessMult": 1.3,
      "integrityMult": 1.5,
      "armorRating": 2,
      "shieldIntegrityMult": 1.0,
      "turretCount": 2,
      "hullTint": [0.85, 0.8, 0.7],
      "thrusterColor": [0.8, 0.6, 0.3],
      "description": "Cargo hauler. Large capacity, moderate armor, slow and heavy."
    },
    "cruiser": {
      "name": "Cruiser",
      "valueRange": [200000, 800000],
      "capacityMult": 1.5,
      "compactnessMult": 1.2,
      "integrityMult": 2.5,
      "armorRating": 5,
      "shieldIntegrityMult": 2.0,
      "turretCount": 6,
      "hullTint": [0.7, 0.75, 0.85],
      "thrusterColor": [0.5, 0.6, 1.0],
      "description": "Mid-size combat vessel. Strong armor, good shields, multiple weapon arcs."
    },
    "battleship": {
      "name": "Battleship",
      "valueRange": [800000, 3000000],
      "capacityMult": 2.0,
      "compactnessMult": 1.5,
      "integrityMult": 4.0,
      "armorRating": 8,
      "shieldIntegrityMult": 3.0,
      "turretCount": 8,
      "hullTint": [0.6, 0.6, 0.7],
      "thrusterColor": [0.3, 0.5, 1.0],
      "description": "Heavy warship. Massive armor, strong shields, many turrets. Slow but devastating."
    },
    "capital": {
      "name": "Capital Ship",
      "valueRange": [3000000, 10000000],
      "capacityMult": 4.0,
      "compactnessMult": 2.0,
      "integrityMult": 6.0,
      "armorRating": 12,
      "shieldIntegrityMult": 5.0,
      "turretCount": 12,
      "hullTint": [0.5, 0.5, 0.6],
      "thrusterColor": [0.2, 0.4, 0.8],
      "description": "Massive capital ship. Maximum armor and shields, overwhelming firepower. Very slow."
    }
  },
  "weaponClasses": {
    "beam": {
      "probability": 0.0,
      "valueRange": [5000, 50000],
      "effectiveRange": 2000,
      "magazineSizeMult": 0,
      "magazineProbability": 0,
      "powerDrainMult": 5,
      "rateMult": 1,
      "spreadMult": 0,
      "weightMult": 5,
      "ammoDamageMult": 5,
      "ammoLifeMult": 2.5,
      "ammoSpeedMult": 1e10,
      "description": "Continuous energy beam. High damage, high power draw, no spread."
    },
    "missile": {
      "probability": 0.0,
      "valueRange": [3000, 30000],
      "effectiveRange": 15000,
      "magazineSizeMult": 1,
      "magazineProbability": 1,
      "powerDrainMult": 0,
      "rateMult": 0.01,
      "spreadMult": 1,
      "weightMult": 3,
      "ammoDamageMult": 20,
      "ammoLifeMult": 10,
      "ammoSpeedMult": 1,
      "description": "Guided missile. Slow fire rate, high damage, long range, tracks target."
    },
    "pulse": {
      "probability": 1.0,
      "valueRange": [1000, 20000],
      "effectiveRange": 5000,
      "magazineSizeMult": 6,
      "magazineProbability": 0.1,
      "powerDrainMult": 2,
      "rateMult": 1,
      "spreadMult": 2,
      "weightMult": 2,
      "ammoDamageMult": 2,
      "ammoLifeMult": 1.25,
      "ammoSpeedMult": 1,
      "description": "Energy pulse. Fast fire rate, moderate damage, short-medium range."
    },
    "rail": {
      "probability": 0.0,
      "valueRange": [10000, 80000],
      "effectiveRange": 20000,
      "magazineSizeMult": 10,
      "magazineProbability": 0.9,
      "powerDrainMult": 1,
      "rateMult": 1,
      "spreadMult": 5,
      "weightMult": 1,
      "ammoDamageMult": 1,
      "ammoLifeMult": 1,
      "ammoSpeedMult": 1e10,
      "description": "Kinetic rail round. Instant hit, high velocity, long range, low damage per shot."
    }
  },
  "balance": {
    "magazineTimeRange": [6, 10],
    "defaultScale": 0.5,
    "defaultOffset": [0, 0.5, 4],
    "defaultIntegrity": 100,
    "colorFormula": "0.25 * white + seeded",
    "armorDamageReduction": 0.05,
    "maxArmorRating": 20
  }
}
```

### Field Descriptions

**Ship Archetype Fields:**
- `valueRange` — [min, max] budget in RU. Archetype auto-selected by value.
- `compactnessMult` — mass multiplier. Higher = lighter = more agile.
  Directly affects top speed and turn rate via `mass = hullValue / (compactness * 10)`.
- `integrityMult` — hull health multiplier. More HP = survives longer.
- `armorRating` — flat damage reduction per hit. `damageDealt = max(1,
  damage - armorRating)`. Armor 0 = no reduction. Armor 12 = capital ship.
- `shieldIntegrityMult` — shield health multiplier. 0 = no shield.
  Shield plugs into `SocketType_Generator` socket, absorbs damage first.
- `shieldChargeTime` — seconds to fully recharge depleted shield (default 60).
- `shieldRestoreFraction` — fraction of shield HP restored on recharge
  after depletion (default 0.25 = 25%).
- `shieldColor` — RGB tint for shield ripple shader (default blue).
- `shieldIdleOpacity` — 0 = invisible when not hit (current behavior),
  >0 = visible energy field at rest.
- `hullTint` — RGB multiplier for `Material_Metal` albedo. [1,1,1] = default
  grey. [0.7,0.75,0.85] = bluish cruiser. Applied as shader uniform.
- `thrusterColor` — RGB for thruster trail and glow. Per-archetype visual
  identity (scouts blue, fighters orange, capitals cool blue).

**Weapon Class Fields:**
- `probability` — weight for random class selection. 0 = disabled.
  Currently 100% Pulse; JSON enables mixing.
- `valueRange` — [min, max] weapon value in RU. Determines damage scaling.
- `effectiveRange` — range at which damage falls off (units). Beyond this,
  damage reduced by distance falloff. Short-range weapons (pulse=5000)
  vs long-range (rail=20000, missile=15000).
- `magazineSizeMult` — magazine capacity multiplier (0 = no magazine).
- `magazineProbability` — chance weapon has a magazine (0-1).
- `powerDrainMult` — power consumption multiplier.
- `rateMult` — fire rate multiplier (higher = faster).
- `spreadMult` — accuracy spread multiplier (0 = pinpoint).
- `weightMult` — mass contribution multiplier.
- `ammoDamageMult` — base damage multiplier.
- `ammoLifeMult` — projectile lifetime multiplier.
- `ammoSpeedMult` — projectile velocity multiplier.

**Balance Fields:**
- `armorDamageReduction` — percentage reduction per armor rating point
  (0.05 = 5% per point). Armor 8 = 40% reduction.
- `maxArmorRating` — cap for armor stacking (prevents invulnerability).

### C++ Changes Required

1. **✅ `ShipType.cpp`** — Reads `defaults` and `shipArcheTypes` from
   `ships.json` via DatabaseManager. Archetype matched by `valueRange`.
   Multipliers (`capacityMult`, `compactnessMult`, `integrityMult`)
   applied on top of user args. `turretCount` overridable per archetype.
   Dead parameters kept in Args for LTSL backward compat.

2. **✅ `WeaponType.cpp`** — Reads `weaponClasses` from `ships.json`.
   Multiplier tables loaded once via `LoadWeaponClassTables()`.
   Hardcoded defaults used if JSON unavailable. `kAmmoProbabilityMult`
   now data-driven (still 100% Pulse in default config).

3. **`StationType.cpp`** — Read from `stations.json` (separate file)
   instead of hardcoded `dockCapacity = 100`.

4. **`Damager.cpp`** — Add hull hit sound (`Sound_Play3D`), physics
   impulse (`ApplyForce`/`ApplyTorque`), and larger `Effect_SmallPlume`
   on hit. See §13 for full hit feedback improvements.

5. **`Shield.cpp`** — Read `shieldColor`, `shieldIdleOpacity`,
   `shieldChargeTime`, `shieldRestoreFraction` from ship archetype.
   Add idle visualization shader when `idleOpacity > 0`.

6. **Dead parameter cleanup** — Remove `propulsion`/`systems`/`turrets`
   from `Item_ShipType_Args` and `Item_StationType_Args`. Update LTSL
   bindings to match.

### Migration Strategy

**Phase 1: Read-only (no behavior change) — COMPLETE**
- ✅ Create `ships.json` with current hardcoded values as defaults
- ✅ ShipType.cpp reads `defaults` + `shipArcheTypes` from JSON, falls back to hardcoded on missing
- ✅ WeaponType.cpp reads `weaponClasses` from JSON, falls back to hardcoded on missing
- ⬜ Remove dead parameters (`propulsion`, `systems`, `turrets`) from Args structs and LTSL bindings
- ✅ All existing apps produce identical results (1009 checks, 0 failures; ltheory-main verified)

**Phase 2: Enable new archetypes + weapons**
- Add new ship archetypes (scout, corvette, freighter, battleship)
- Enable Beam/Missile/Rail weapon classes by setting non-zero probabilities
- Add `effectiveRange` damage falloff
- Apps choose archetypes by name instead of raw value numbers

**Phase 3: Visual + audio feedback**
- Hull hit sound + physics impulse (§13)
- Per-archetype hull tinting and thruster color
- Shield idle visualization
- Armor damage reduction

**Phase 4: LTSL-facing API**
- New binding `ShipType_GetArchetype(name)` returns an Item configured
  from the named archetype
- Apps call `ShipType_GetArchetype "fighter"` instead of
  `Item_ShipType 10000 20 1 1 1 1 1 1`

---

## 12. Ship Enhancement Research — What Could Be Added

Full inventory of existing subsystems and what's missing. Organized by
visual effects, gameplay systems, ship variety, and audio.

---

### 12.1 Shield System — What Exists vs What's Missing

**Exists (Shield.cpp, 228 lines):**
- Shield is a pluggable socket child (plugs into `SocketType_Generator`)
- `SmoothHull` mesh auto-generated from parent ship's collision mesh
- Hit ripple shader (`shield.jsl`): up to 16 simultaneous ripples, blue
  `(0.3, 0.6, 1.8)`, expanding concentric rings at impact points
- Recharge: charges at `parent->GetPowerFraction() / 60s`, restores 25%
  health on depletion
- Shield destruction: blue plasma explosion (`shield_explosion.jsl`)
- Sound: `shield/hit.ogg` on impact, `shield/explosion.ogg` on destruction

**Missing — Enhancement Opportunities:**
- **No visible idle state** — shield only renders when being hit. No
  visible bubble/barrier/field when idle. This is the biggest visual gap.
- **Fixed blue color** — no per-shield-type tinting. Could be data-driven
  via `shields.json` with color, opacity, ripple speed, idle visibility.
- **No directional shielding** — uniform absorption from all directions.
  Could add front-heavy vs omni shielding as an archetype variant.
- **No shield capacity/regen rate tuning** at script level — hardcoded
  `kChargeTime = 60`, `kRestoreFraction = 0.25`.
- **No shield hover/wobble animation** when idle — the mesh is static.
- **No shield color variety** — always blue regardless of faction/class.

---

### 12.2 Damage / Integrity System — What Exists vs What's Missing

**Exists (Integrity.cpp, Event/Damage.cpp):**
- Two-phase damage routing: shields absorb first, remainder goes to hull
- `ApplyDamage` returns leftover damage (overflow)
- `OnDeath` propagates to children (tasks cancelled)
- `Event_Damage` fires `OnAttacked` for both owners
- `GetTotalHealth/MaxHealth` recursively sums hull + shields
- `Integrity` component: `health`, `maxHealth`, `dataDamaged`, `dataDestroyed`
- Bindings: `GetHealth`, `GetMaxHealth`, `GetHealthNormalized`, `SetHealth`

**Missing — Enhancement Opportunities:**
- **No armor/defense modifier** — damage goes straight to health, no
  damage resistance. Could add `armorRating` per ship archetype.
- **No damage types** — just raw `int32 Damage`. Could add kinetic,
  energy, explosive with per-type resistances.
- **No per-component damage** — hull is one pool. No subsystem targeting
  (engines, weapons, shields individually).
- **No hull damage visualization** — no bullet holes, scorch marks,
  missing plates, smoke from damaged areas.
- **Collision damage is disabled** — `Collidable.cpp` line 38: `if (false
  && ...)`. Code exists but commented out. Enabling it would add
  raming/velocity-based damage.
- **No death sequence** — destroyed ship gets explosions on surface but
  no wreckage, no debris, no fragmentation.
- **No hull regen** — only shields recharge. Could add slow hull repair
  for certain archetypes.

---

### 12.3 Weapon Effects — What Exists vs What's Missing

**Exists:**
- **Pulse**: billboard glow + axis trail, raycast hit detection, `Effect_SmallPlume` on impact
- **Missile**: physics-driven with guidance, ribbon trail (`trail.jsl`), 5 fire explosions on hit, looping sound
- **Rail**: instant hit-cast, flash beam (`rail.jsl`), 0.25s visual, `Effect_BeamHit`
- **Beam**: FULLY CODED but **disabled** (`#if 0` in WeaponType.cpp:104). Would create persistent beam with `beam.jsl` shader, continuous damage, looping sound
- **Muzzle flash**: dynamic light on weapon, fades via exponential decay
- **Weapon classes all rigged to 100% Pulse** — Beam/Missile/Rail probability = 0

**Missing — Enhancement Opportunities:**
- **Beam weapon disabled** — just needs `#if 0` removed and a few fixes. This is the easiest win.
- **No weapon trail particles** — pulse and rail projectiles have no particle trails, only shader-based billboards.
- **No weapon charge-up animation** — fire is instant. Could add a visible charge glow before firing.
- **No impact flash variety** — all weapons use the same `Effect_SmallPlume`. Could differentiate per weapon class.
- **No tracers** — pulse projectiles are glowing orbs but no tracer-style visual.
- **No weapon color variety at render level** — color comes from weapon type but all pulse weapons look the same shape-wise.
- **No fragmentation/area-of-effect** on missile impact — 5 separate explosions, no unified blast wave.

---

### 12.4 Thruster Visuals — What Exists vs What's Missing

**Exists (Thruster.cpp, 208 lines):**
- Billboard trail using `thruster_trail.jsl` with noise-based turbulence
- Trail axis follows thruster look direction, scales with activation
- Dynamic point light at thruster position
- Activation smoothly interpolates via exponential easing
- Cruise mode: 100x power draw for boost
- Color lerp to `kBoostColor = V3(0.2, 0.7, 1.0)` when overcharged

**Missing — Enhancement Opportunities:**
- **All thrusters same color** — hardcoded orange `Color(1.0, 0.4, 0.1)`. Could be per-ship-type or per-faction via JSON.
- **Single shared trail shader** — no visual differentiation between thruster types.
- **No thruster glow/bloom effect** — just additive billboard + point light.
- **No thrust-dependent trail shape** — trail scales with activation but shape is fixed.
- **No overcharge visual** beyond color lerp — no size increase, no particle burst.
- **No engine exhaust particles** — only the billboard trail, no particle system.
- **No thruster sound variety** — all ships share one `thruster/loop1.ogg`.

---

### 12.5 Ship Hull Variety — What Exists vs What's Missing

**Exists (Generate.lts, ShipType.cpp):**
- Procedural PlateMesh hull from `(scale, seed)` — box-stacking with bilateral symmetry
- Plate count = `2 + Sqrt(scale)` — more plates for larger ships
- Two warp deformations: `VerticalCompress` + `HExpand` (identical for all ships)
- `Material_Metal()` — procedural plating texture, same for all ships
- Scale determines mass, health, cargo, thruster count, turret count

**Missing — Enhancement Opportunities:**
- **No hull class archetypes** — all ships use the same algorithm. No visual distinction between "sleek fighter" vs "bulky freighter" beyond size.
- **No hull color/paint system** — `Material_Metal()` is uniform grey. Could add color tinting per faction or per archetype.
- **No faction-specific hull styles** — all factions produce identical-looking ships.
- **No procedural detail features** — no antenna, fins, sensor arrays, wing details.
- **No hull damage visualization** — no bullet holes, scorch marks, missing plates, smoke from damaged areas.
- **No decal system** — commented out in Generate.lts (lines 92-100, Chinese text overlay).
- **No emissive hull elements** — no running lights, no window glow, no navigation lights.
- **All hulls symmetric on X axis** — no asymmetric designs possible.
- **No LOD system** — `lodLevel` field exists but no actual LOD mesh generation.

---

### 12.6 Particle System — What Exists vs What's Missing

**Exists (ParticleSystem.h, Particles.cpp, Effects.cpp):**
- GPU-instanced billboard particles via SSBO
- 2 particle types: `Fire` (radial glow, 0.5s) and `Firefly` (subtle glow, 1-2s)
- `Effect_SmallPlume`: 15 fire particles for weapon impacts
- `Effect_BeamHit`: single firefly for rail/beam hits
- `Effect_MultiExplosionRadial`: 15 explosions on object surface
- Particle shaders: `particle_radial.jsl`, `particle_radialtextured.jsl`

**Missing — Enhancement Opportunities:**
- **Only 2 particle types** — no sparks, smoke, debris, energy wisps, shield particles.
- **No weapon trail particles** — pulse/missile/rail have no particle trails.
- **No engine exhaust particles** — only the billboard trail.
- **No shield impact particles** — just ripples on the mesh.
- **No debris particles** on ship destruction.
- **No script-facing particle creation API** — particles only created from C++ `Effect_*` helpers.

---

### 12.7 Power/Energy System — What Exists vs What's Missing

**Exists (PowerGenerator.cpp, Pluggable.h):**
- PowerGenerator: capacitor charges at 2.0/s, discharges at 4.0/s during boost
- Power allocation: sums `priority * powerRequest` from all consumers, distributes proportionally
- Consumers: thruster (basePower, 100x during cruise), shield (reads parent fraction), scanner, weapon
- Boost mechanic: capacitor energy temporarily added to output
- `GetPowerFraction()`: how much of requested power was received (0-1)

**Missing — Enhancement Opportunities:**
- **No power HUD** — no way to see energy allocation in-game.
- **No power routing UI** — priority is fixed at 1.0 for all consumers.
- **No per-system power toggling** — can't divert power from weapons to shields.
- **No overcharge/overload mechanic** beyond thruster boost.
- **No power failure cascade** when overloaded — systems just get less power proportionally.
- **No battery/energy storage** beyond the single capacitor.

---

### 12.8 Ship Audio — What Exists vs What's Missing

**Exists (Ship.lts, weapon sounds, shield sounds):**
- Interior ambiance: `ship/ambiance/interior/1.wav` (looped, volume 0.15)
- Engine loop: `thruster/loop1.ogg` (volume scales with speed)
- Weapon sounds: pulse (6 variants), missile (fire + loop), rail (fire)
- Shield sounds: hit (`shield/hit.ogg`), explosion (`shield/explosion.ogg`)
- Explosion sounds: `explosion/altsmall3.ogg`

**Missing — Enhancement Opportunities:**
- **No thruster sound variety** — all ships share one engine loop.
- **No hull hit sound** — only shield hit has audio.
- **No shield recharge sound** — silent recharge.
- **No flyby/Doppler audio** — no approach/depart sound effects.
- **No weapon class-specific fire sounds** beyond the current set.
- **No ambient music system** visible in the engine.

---

### 12.9 Hardpoint / Socket System — What Exists vs What's Missing

**Exists (Sockets.h, Socket.h, ShipType.cpp):**
- 4 socket types: Generator, Interior, Thruster, Turret
- 5 joint types: AxisX, AxisY, AxisZ, Fixed, Free
- Thruster sockets: ray-cast to rear-facing surfaces, symmetric pairs
- Turret sockets: ray-cast to top/bottom surfaces, 4 attempts with mirrored pairs, AxisY tracking
- Generator/interior sockets: placed at origin (no spatial placement)
- `Plug`/`Unplug` API for adding/removing items from sockets

**Missing — Enhancement Opportunities:**
- **No weapon hardpoint customization** — weapons always in turrets, always in turret sockets. No ability to swap weapon types on existing hardpoints.
- **No shield socket type** — shields overload `SocketType_Generator`. Dedicated type would allow cleaner logic.
- **No socket damage** — destroying a socket doesn't remove its contents or disable it.
- **No runtime hardpoint modification** — sockets set at creation, not modifiable via LTSL.
- **No visual hardpoint indicators** — no mounting points visible on hull.
- **No socket capacity limits** — turrets always have 1 weapon socket.
- **Generator/interior sockets at origin** — no spatial placement, no visual representation.

---

### 12.10 Existing Shader Effects Inventory

| Shader | File | Used By | Purpose |
|--------|------|---------|---------|
| `beam.jsl` | `resource/shader/fragment/beam.jsl` | Beam weapon (disabled) | Pulsing energy beam with head/tail fade |
| `shield.jsl` | `resource/shader/fragment/shield.jsl` | Shield object | Hit ripple waves on shield mesh |
| `shield_explosion.jsl` | `resource/shader/fragment/shield_explosion.jsl` | Shield death | Blue plasma burst with ring |
| `explosion.jsl` | `resource/shader/fragment/explosion.jsl` | Fire explosions | Radial orange glow |
| `thruster_trail.jsl` | `resource/shader/fragment/thruster_trail.jsl` | Thruster trail | Billboard flame trail with noise |
| `pulse_head.jsl` | `resource/shader/fragment/pulse_head.jsl` | Pulse projectile | Radial glow point |
| `pulse_tail.jsl` | `resource/shader/fragment/pulse_tail.jsl` | Pulse trail | Axis-aligned glow |
| `rail.jsl` | `resource/shader/fragment/rail.jsl` | Rail projectile | Instant flash beam |
| `trail.jsl` | `resource/shader/fragment/trail.jsl` | Missile trail | Segmented ribbon trail |
| `particle_radial.jsl` | `resource/shader/fragment/particle_radial.jsl` | Fire/Firefly | Radial particle glow |

**No shaders exist for:** shield idle state, hull damage, hull glow, energy field, HUD effects, smoke, sparks, debris.

---

### 12.11 Enhancement Roadmap (Prioritized)

#### Tier 1 — High Impact, Low-Medium Effort (ships.json + shader/script work)

| Enhancement | What | How | Effort |
|-------------|------|-----|--------|
| **Enable Beam weapons** | Remove `#if 0` in WeaponType.cpp:104 | Fix a few references, test | 1-2 days |
| **Enable collision damage** | Remove `if (false &&` in Collidable.cpp:38 | Test, tune energy formula | 1 day |
| **Thruster color variety** | Add `color` to ThrusterType JSON or hardcoded per-type | Script + small C++ change | 2-3 days |
| **Weapon class probabilities** | ShipType/WeaponType reads from `ships.json` | Already designed in §11 | 1 week |
| **Shield idle visualization** | New `shield_idle.jsl` shader, subtle wireframe/energy field | GLSL shader work | 1 week |
| **Hull color tinting** | Add `albedoTint` uniform to `Material_Metal` | C++ + shader change | 3-5 days |
| **Ship archetypes** | `ships.json` archetypes with different stat distributions | Already designed in §11 | 1 week |

#### Tier 2 — High Impact, Medium-High Effort (engine changes)

| Enhancement | What | How | Effort |
|-------------|------|-----|--------|
| **Hull damage visualization** | Damage states (intact → damaged → critical), decals, smoke | New shader + component fields | 2-3 weeks |
| **Shield color variety** | Per-shield-type tinting via uniform | Shield.cpp + shield.jsl | 1 week |
| **Particle expansion** | Sparks, smoke, debris, energy wisps | New particle types in Particles.cpp | 1-2 weeks |
| **Weapon trails** | Particle trails for pulse/rail projectiles | Particle system + LTSL binding | 1 week |
| **Death sequence** | Wreckage persistence, debris, structured breakup | New component + particle system | 2-3 weeks |
| **Power HUD** | Visualize energy allocation (bars/graph) | New LTSL widget + bindings | 1-2 weeks |
| **Per-component damage** | Target engines/weapons/shields individually | Integrity per socket | 3-4 weeks |

#### Tier 3 — Medium Impact, High Effort (major systems)

| Enhancement | What | How | Effort |
|-------------|------|-----|--------|
| **Damage types** | Kinetic/energy/explosive with resistances | New enum + modifier system | 2-3 weeks |
| **Socket damage** | Destroy individual hardpoints | Sockets component + integrity | 2-3 weeks |
| **Directional shielding** | Front-heavy vs omni shielding | Shield component extension | 1-2 weeks |
| **Faction hull styles** | Different hull generation algorithms per faction | Generate.lts variants | 2-3 weeks |
| **Hull decals/paint** | Decal system for faction logos, damage marks | New component + shader | 2-3 weeks |
| **Emissive hull elements** | Running lights, window glow | New shader pass | 1-2 weeks |
| **Flyby/Doppler audio** | Approach/depart sound effects | Audio system extension | 1 week |
| **Hull repair mechanic** | Slow hull regen in safe zones | New action + component | 1-2 weeks |

---

### 12.12 JSON Integration — What Goes in ships.json

All enhancement parameters are configured through `ships.json` (see §11
for the full schema). The JSON layer makes all these parameters tunable
without recompilation. Shader uniforms are wired from the JSON values at
load time. The schema in §11 already includes hullTint, thrusterColor,
shieldColor, shieldIdleOpacity, armorRating, and shieldIntegrityMult
per archetype.

---

## 13. Weapon Hit Feedback Analysis — Why Pulse Lasers Feel Like They Do Nothing

This section documents the root causes of poor hit feedback and what
would need to change. Based on thorough code analysis of the full damage
pipeline.

### 13.1 The Damage Pipeline (What Happens When a Pulse Hits Something)

```
Pulse.Update()
  → raycast from lastPos to pos (Pulse.cpp:151-156)
  → Damager.Hit(dest, position) (Pulse.cpp:159)
    → Collidable.Collide(dest, self) → dest.OnCollide()  [EMPTY for asteroids/ships]
    → Effect_SmallPlume(position, velocity, color, scale)  [15 particles, 0.25s]
    → Event_Damage(source, dest, damage)
      → dest.ApplyDamage(damage)
        → check shields first (Object.cpp:240)
        → then Integrity component (Object.cpp:248)
        → if health ≤ 0 → OnDeath() + Explodable explosion
```

### 13.2 Root Causes of "No Impact" Feeling

**CRITICAL: No sound on hull/asteroid hits.**
`Damager::Hit()` (Damager.cpp:11-35) has zero `Sound_Play3D` calls.
Shield hits play `shield/hit.ogg` (Shield.cpp:209), but hull hits and
asteroid hits are completely silent. Sound is the primary confirmation
of physical impact — this alone accounts for most of the "no feedback"
feeling.

**CRITICAL: No physics impulse from hits.**
`Damager::Hit()` calls `Collidable::Collide()` which invokes
`OnCollide()` — an empty virtual for ships and asteroids. There is no
`ApplyForce()` or `ApplyTorque()` anywhere in the damage pipeline. A
pulse projectile traveling at high velocity hits a target and applies
zero kinetic force. The target doesn't budge, doesn't rotate, nothing.

**CRITICAL: Asteroids are indestructible.**
`Object_Asteroid` has no `Component_Integrity` and no
`Component_Explodable`. When `ObjectT::ApplyDamage()` is called on an
asteroid, it checks for shields (none) and Integrity (none), then
silently discards the damage. Shooting asteroids is pointless — the
damage vanishes.

**Asteroid component list (Asteroid.cpp):**
```
BoundingBox, Collidable, Cullable, Drawable, Orientation, Seeded
```
No Integrity, no Explodable, no health tracking of any kind.

**`Effect_SmallPlume` is too small and too brief.**
- 15 particles with 0.25-0.5s lifetime (Effects.cpp:80-101)
- Uses `Particle_Fire()` at 0.5 opacity with fast fade-in/out
- Scale is based on the *shooter's* radius, not the target's
- At typical engagement distances (hundreds of meters), nearly invisible

**No intermediate damage visuals.**
`Explodable::Run()` (Explodable.cpp:7-15) only triggers at `health <= 0`.
Between 100% and 0% health, a ship looks pristine. No smoke, no sparks,
no discoloration. The jump from "looks fine" to "explodes" is instant.

**Weapon fire sound is very quiet.**
`WeaponType.cpp:90` plays fire sound at volume 0.1. Combined with no hit
sound, the entire combat audio is nearly silent.

### 13.3 Feedback Comparison Table

| Feedback | Shield Hit | Ship Hull Hit | Asteroid Hit |
|----------|-----------|---------------|--------------|
| Sound | `shield/hit.ogg` ✓ | **NONE** ✗ | **NONE** ✗ |
| Visual effect | Ripple shader + particles | 15 particles (tiny) | 15 particles (tiny) |
| Physics impulse | **NONE** ✗ | **NONE** ✗ | **NONE** ✗ |
| Damage applied | Reduces shield HP | Reduces hull HP | **NONE** (discarded) |
| Death explosion | Plasma (shield) | Fire (ship) | **NEVER** |
| Intermediate damage | Shield fades | **NOTHING** | N/A |

### 13.4 What Would Fix It (Prioritized)

**Tier 1 — Immediate fixes (small C++ changes):**

1. **Add hull hit sound to `Damager::Hit()`.** One `Sound_Play3D` call
   with a new `weapon/hit_hull.ogg` sound file. This is the single
   highest-impact change. Estimated: 1 line of code + 1 sound asset.

2. **Add physics impulse to `Damager::Hit()`.** After `Event_Damage`,
   apply force to the target: `dest->GetMotion()->force +=
   Normalize(position - source->GetPos()) * damage * impulseScale`.
   Ships and asteroids would visibly react to impacts. Estimated:
   5-10 lines of code.

3. **Increase `Effect_SmallPlume` size and duration.** Change particle
   count from 15 to 25-30, lifetime from 0.25-0.5s to 0.5-1.0s, and
   use target radius instead of source radius for scale. Estimated:
   3-5 lines of code.

4. **Increase weapon fire volume.** Change volume from 0.1 to 0.3-0.5
   in `WeaponType.cpp:90`. Estimated: 1 line.

**Tier 2 — Medium effort (script + shader):**

5. **Add `Component_Integrity` to asteroids.** Give asteroids health
   so they can be destroyed. Add `Explodable` for death explosions.
   ShipType.json could define `asteroidHealth` per value range.
   Estimated: 10-20 lines C++ + script changes.

6. **Add hit flash/spark effect.** New particle type or shader uniform
   flash on the target object at the hit point. A brief brightening of
   the hull material at the impact location.

7. **Add damage states to ships.** At 75%/50%/25% health, spawn
   progressively more smoke/fire particles. New `Component_DamageState`
   or extend `Explodable` with threshold callbacks.

**Tier 3 — Higher effort (new systems):**

8. **Hull damage visualization.** Decal system for bullet holes, scorch
   marks, missing plates. Requires new shader pass or texture overlay.

9. **Screen shake on hit.** Camera shake proportional to damage dealt.
   Requires new binding or camera effect system.

10. **Shield hit impulse.** Shield currently absorbs damage silently —
    add visible knockback when shield is hit.

### 13.5 Sound Assets Needed

| Sound | Purpose | Suggested Source |
|-------|---------|-----------------|
| `weapon/hit_hull.ogg` | Pulse/weapon hits ship hull | Metallic impact, short |
| `weapon/hit_asteroid.ogg` | Weapon hits asteroid | Rock impact, short |
| `weapon/hit_shield.ogg` | (existing) `shield/hit.ogg` | Energy field impact |
| `weapon/chargeup.ogg` | Weapon charge before fire | Rising energy tone |
| `ship/damage_loop.ogg` | Ambient damage at low health | Hissing/sparking loop |
| `weapon/beam_fire.ogg` | (existing) `weapon/beam1_fire.wav` | Continuous beam |
| `weapon/beam_loop.ogg` | (existing) `weapon/beam1_loop.wav` | Beam sustain |

---

## 14. Recommended Engine Changes — Tier 1-3

This section documents the remaining gaps between the ships.json schema and
actual engine behavior, organized by implementation priority. Each tier
groups changes by effort level and impact.

---

### Tier 1 — Must-Do (Small C++ Changes)

These are the minimum changes needed to call ships "done" from a JSON
standpoint. Each is self-contained and testable.

| # | Gap | File | What | Effort |
|---|-----|------|------|--------|
| 1.1 | ✅ **Shield creation in Instantiate()** | `ShipType.cpp:101` | `shieldValueRatio` is read from JSON but `shieldValue` is hardcoded to `0.0` (line 293). `Instantiate()` must compute shieldValue from the budget split and create + plug an `Item_ShieldType`. Remove LTSL shield creation from `ltheory-main.lts`. | ✅ Done |
| 1.2 | ✅ **Wire `armorRating`** | `Integrity.h`, `Integrity.cpp`, `ShipType.cpp` | `armorRating` added to `ComponentIntegrity`. `Instantiate()` reads archetype `armorRating` and sets it on the ship. `ApplyDamage()` reduces damage by `armorRating * armorDamageReduction`. | ✅ Done |
| 1.3 | ✅ **Hull tint via shader uniform** | `metal.jsl`, `Materials.cpp`, `Generate.lts` | `hullTint` added as `uniform vec3` to `metal.jsl`. New `Material_Metal_Tinted(diffuse, tint)` function. `Generate.lts` accepts tint param. `Item_ShipType()` reads archetype `hullTint` and passes to mesh generation. | ✅ Done |
| 1.4 | ✅ **Ship name from archetype** | `ShipType.cpp:366` | `self->name` now set from archetype JSON `name` field instead of hardcoded "Ship". | ✅ Done |
| 1.5 | ✅ **Balance knobs from JSON** | `ShipType.cpp:36-38` | `kThrusterAttempts`, `kTurretAttempts`, `kThrusterTolerance` now read from `ships.json balance` section on DB load. | ✅ Done |

### Tier 2 — Important (Medium C++)

These improve gameplay variety and data-driven control but are not blocking
basic functionality.

| # | Gap | File | What | Effort |
|---|-----|------|------|--------|
| 2.1 | ✅ **`ShipType_GetArchetype(name)` binding** | `ShipType.cpp` | New LTSL binding `ShipType_GetArchetype(name)` (aliased `GetArchetype`) returns an Item from a named archetype. Reads valueRange midpoint, random seed. | ✅ Done |
| 2.2 | **Thruster color per-archetype** | `ThrusterType.cpp` | `thrusterColor` exists in JSON but thrusters are always hardcoded orange `Color(1.0, 0.4, 0.1)`. Thread color through `Item_ThrusterType` and into `Thruster.cpp` render. | 1-2 days |
| 2.3 | **Shield runtime params** | `Shield.cpp:33-34` | `kChargeTime = 60` and `kRestoreFraction = 0.25` are hardcoded. Read `shieldChargeTime` and `shieldRestoreFraction` from ship archetype JSON. Requires Shield to store reference to its ship type's config. | 2 days |
| 2.4 | **Weapon effectiveRange falloff** | `Weapon.cpp` | `effectiveRange` is read per weapon class but never applied. Beyond `effectiveRange`, damage should fall off by `1.0 / (1.0 + distance / effectiveRange)`. | 1 day |
| 2.5 | **Station JSON wiring** | `StationType.cpp` | `Item_StationType_Args` has dead params `systems`/`turrets`. Create `stations.json` with dock capacity, name, mass multiplier. | 1-2 days |

### Tier 3 — Polish (Higher Effort)

Visual and audio improvements that make ships feel alive.

| # | Gap | File | What | Effort |
|---|-----|------|------|--------|
| 3.1 | **Shield idle visualization** | `Shield.cpp`, new `shield_idle.jsl` | Shield is invisible when not being hit. Add optional visible energy field at rest using `shieldIdleOpacity` from JSON. New shader pass. | 3-5 days |
| 3.2 | **Shield color variety** | `Shield.cpp`, `shield.jsl` | Shield ripple color is always blue `(0.3, 0.6, 1.8)`. Read `shieldColor` from ship archetype and pass as uniform. | 1 day |
| 3.3 | **Weapon value scaling** | `WeaponType.cpp` | Weapon damage/stats should scale with the ship's value bracket, not just random seed. | 2 days |
| 3.4 | **Hull hit sound** | `Damager.cpp` | No `Sound_Play3D` on hull impact (only shield hits have audio). Add metallic impact sound. | 0.5 day |
| 3.5 | **Physics impulse on hit** | `Damager.cpp` | No `ApplyForce`/`ApplyTorque` from weapon impacts. Ships don't react to being hit. | 1-2 days |
| 3.6 | **Asteroid destructibility** | `Asteroid.cpp` | Asteroids have no `Integrity` or `Explodable` component — damage is silently discarded. Add health so asteroids can be destroyed. | 2 days |

---

### Current JSON Field Wiring Status

| Field | Read in C++ | Actually Used | Notes |
|-------|-------------|---------------|-------|
| `defaults.hullValueRatio` | ✅ ShipType.cpp:253 | ✅ Budget split | — |
| `defaults.shieldValueRatio` | ✅ ShipType.cpp:254 | ✅ Shield creation in Instantiate() | ✅ Tier 1.1 done |
| `defaults.scannerValue` | ✅ ShipType.cpp:255 | ✅ Scanner item | — |
| `defaults.thrusterCount` | ✅ ShipType.cpp:256 | ✅ Socket count | — |
| `defaults.turretCount` | ✅ ShipType.cpp:257 | ✅ Socket count | — |
| `defaults.shieldChargeTime` | ❌ Not read | ❌ Hardcoded 60 | **Tier 2.3** |
| `defaults.shieldRestoreFraction` | ❌ Not read | ❌ Hardcoded 0.25 | **Tier 2.3** |
| `defaults.shieldColor` | ❌ Not read | ❌ Hardcoded blue | **Tier 3.2** |
| `defaults.shieldIdleOpacity` | ❌ Not read | ❌ Always 0 | **Tier 3.1** |
| `defaults.hullTint` | ❌ Not read | ❌ No uniform | — Defaults only; archetype overrides handled per-type |
| `defaults.thrusterColor` | ❌ Not read | ❌ Hardcoded orange | **Tier 2.2** |
| `archetypes.*.capacityMult` | ✅ ShipType.cpp:268 | ✅ Capacity calc | — |
| `archetypes.*.compactnessMult` | ✅ ShipType.cpp:269 | ✅ Mass calc | — |
| `archetypes.*.integrityMult` | ✅ ShipType.cpp:270 | ✅ Health calc | — |
| `archetypes.*.shieldIntegrityMult` | ✅ ShipType.cpp:271 | ✅ Shield creation in Instantiate() | ✅ Tier 1.1 done |
| `archetypes.*.armorRating` | ✅ ShipType.cpp:Instantiate() | ✅ ComponentIntegrity.armorRating | ✅ Tier 1.2 done |
| `archetypes.*.turretCount` | ✅ ShipType.cpp:272 | ✅ Socket count | — |
| `archetypes.*.hullTint` | ✅ ShipType.cpp + Generate.lts | ✅ Material_Metal_Tinted uniform | ✅ Tier 1.3 done |
| `archetypes.*.thrusterColor` | ❌ Not read | ❌ | **Tier 2.2** |
| `archetypes.*.name` | ✅ ShipType.cpp | ✅ self->name | ✅ Tier 1.4 done |
| `weaponClasses.*.ammoProbabilityMult` | ✅ WeaponType.cpp | ✅ Class selection | Fixed 2026-08 |
| `weaponClasses.*.effectiveRange` | ❌ Not read | ❌ No falloff | **Tier 2.4** |
| `weaponClasses.*.rateMult` | ✅ WeaponType.cpp | ✅ Fire rate | — |
| `weaponClasses.*.ammoDamageMult` | ✅ WeaponType.cpp | ✅ Damage calc | — |
| `weaponClasses.*.ammoLifeMult` | ✅ WeaponType.cpp | ✅ Projectile life | — |
| `weaponClasses.*.ammoSpeedMult` | ✅ WeaponType.cpp | ✅ Projectile speed | — |
| `weaponClasses.*.spreadMult` | ✅ WeaponType.cpp | ✅ Accuracy | — |
| `weaponClasses.*.weightMult` | ✅ WeaponType.cpp | ✅ Mass | — |
| `weaponClasses.*.powerDrainMult` | ✅ WeaponType.cpp | ✅ Energy cost | — |
| `weaponClasses.*.magazineSizeMult` | ✅ WeaponType.cpp | ✅ Magazine cap | — |
| `weaponClasses.*.magazineProbability` | ✅ WeaponType.cpp | ✅ Magazine chance | — |
| `balance.armorDamageReduction` | ✅ Integrity.cpp:ApplyDamage() | ✅ Damage reduction formula | ✅ Tier 1.2 done |
| `balance.maxArmorRating` | ❌ Not read | ❌ No armor cap enforced | Validation only |
| `balance.thrusterAttempts` | ✅ ShipType.cpp:EnsureShipsDb() | ✅ Thruster socket placement | ✅ Tier 1.5 done |
| `balance.turretAttempts` | ✅ ShipType.cpp:EnsureShipsDb() | ✅ Turret socket placement | ✅ Tier 1.5 done |
| `balance.thrusterTolerance` | ✅ ShipType.cpp:EnsureShipsDb() | ✅ Thruster surface normal dot threshold | ✅ Tier 1.5 done |

---

## 15. Ship Viewer — Future Tool

**Status:** Design phase. Not yet implemented.

A lightweight LTSL app that lets developers preview ships by seed value
without restarting the engine. Designed to help tune archetypes, visually
verify seed determinism, and compare ship classes.

### 15.1 How It Would Work

1. **Input:** Text field for seed (uint), slider or text field for value (RU).
2. **Output:** Ship hull rendered in 3D with orbit camera.
3. **Seed entry:** Type a number, press Enter, ship regenerates from that seed.
4. **Value entry:** Change the value to shift which archetype is selected.
5. **Camera:** Orbit around ship center. Mouse drag to rotate, scroll to zoom.
6. **Info overlay:** Display archetype name, mass, hull HP, scale, weapon count.

### 15.2 Implementation Pattern

Follow the driven app pattern from `ltheory-main.lts`:
- `type App` with `Initialize` + `Update`
- Camera + Interface + single ship object
- TextField for seed input, Button for regenerate
- Hot-reload on Enter key or button press

### 15.3 Seed Presets

Save a handful of notable seeds (one per archetype) as config entries so
you can quickly flip between scout/fighter/corvette/etc. without remembering
the value ranges.

### 15.4 Implementation Effort

~2-3 days. Pure LTSL app, no C++ changes needed. Ships are already
procedurally generated — the viewer just creates them with user-provided
parameters and orbits the camera.
