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
| Thruster count | 2 base pairs + directional extras | ShipType.cpp:214 |
| Turret count | 4 (args.turrets ignored) | ShipType.cpp:215 |
| Generator count | `int(rng(1,2) + logScale)` | ShipType.cpp:216 |
| Interior count | `2 * int(logScale / log(10))` | ShipType.cpp:217 |
| Thruster placement tolerance | 0.8 dot product, 10 attempts | ShipType.cpp:223+ |
| Turret placement attempts | 100 per turret | ShipType.cpp:240+ |
| Ship name | Always "Ship" | ShipType.cpp:260 |
| Material | `Material_Metal` | ShipType.cpp:209 |

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
| Magazine time | `Round(rng(6, 10))` if uses > 0 | WeaponType.cpp:189 |
| Rate boost | 1.5× if weapon has magazine | WeaponType.cpp:190 |
| Name | Generated via `Grammar_Get "$weapon"` | WeaponType.cpp |
| Sound | Only Pulse weapons get sound (6 hardcoded .ogg files) | WeaponType.cpp |

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

## 11. What ships.json Would Cover

### Scope Decision: ALL Ships

The same `Item_ShipType` factory creates both player and AI ships. There
is no separate "player ship" vs "AI ship" path — the only difference is
the `value` and `seed` arguments passed at the call site. Therefore
`ships.json` should define **ship archetypes** that apply to all ships.

### Proposed Schema

```json
{
  "version": 1,
  "defaults": {
    "hullValueRatio": 0.6,
    "scannerValue": 1000,
    "thrusterCount": 2,
    "turretCount": 4,
    "interiorCountFormula": "2 * (logScale / log10)",
    "generatorCountFormula": "1..2 + logScale",
    "material": "Metal",
    "meshQuality": 12
  },
  "archetypes": {
    "scout": {
      "name": "Scout",
      "valueRange": [5000, 15000],
      "capacityMult": 0.5,
      "compactnessMult": 0.8,
      "integrityMult": 0.7,
      "turretCount": 2,
      "description": "Fast, lightly armed scout vessel"
    },
    "fighter": {
      "name": "Fighter",
      "valueRange": [10000, 50000],
      "capacityMult": 1.0,
      "compactnessMult": 1.0,
      "integrityMult": 1.0,
      "turretCount": 2,
      "description": "Standard combat fighter"
    },
    "cruiser": {
      "name": "Cruiser",
      "valueRange": [100000, 500000],
      "capacityMult": 1.5,
      "compactnessMult": 1.2,
      "integrityMult": 1.5,
      "turretCount": 6,
      "description": "Balanced multi-role combat vessel"
    },
    "capital": {
      "name": "Capital Ship",
      "valueRange": [1000000, 10000000],
      "capacityMult": 3.0,
      "compactnessMult": 2.0,
      "integrityMult": 3.0,
      "turretCount": 8,
      "description": "Massive heavily armed capital ship"
    }
  },
  "classes": {
    "beam": {
      "probability": 0.0,
      "magazineSizeMult": 0,
      "magazineProbability": 0,
      "powerDrainMult": 5,
      "rateMult": 1,
      "spreadMult": 0,
      "weightMult": 5,
      "ammoDamageMult": 5,
      "ammoLifeMult": 2.5,
      "ammoSpeedMult": 1e10
    },
    "missile": {
      "probability": 0.0,
      "magazineSizeMult": 1,
      "magazineProbability": 1,
      "powerDrainMult": 0,
      "rateMult": 0.01,
      "spreadMult": 1,
      "weightMult": 3,
      "ammoDamageMult": 20,
      "ammoLifeMult": 10,
      "ammoSpeedMult": 1
    },
    "pulse": {
      "probability": 1.0,
      "magazineSizeMult": 6,
      "magazineProbability": 0.1,
      "powerDrainMult": 2,
      "rateMult": 1,
      "spreadMult": 2,
      "weightMult": 2,
      "ammoDamageMult": 2,
      "ammoLifeMult": 1.25,
      "ammoSpeedMult": 1
    },
    "rail": {
      "probability": 0.0,
      "magazineSizeMult": 10,
      "magazineProbability": 0.9,
      "powerDrainMult": 1,
      "rateMult": 1,
      "spreadMult": 5,
      "weightMult": 1,
      "ammoDamageMult": 1,
      "ammoLifeMult": 1,
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

### C++ Changes Required

1. **`ShipType.cpp`** — Read `defaults` and `archetypes` from
   `ships.json` instead of hardcoded ratios. The archetype selection
   would be based on the `value` argument falling within an archetype's
   `valueRange`. Dead parameters (`propulsion`, `systems`, `turrets`)
   would be removed.

2. **`WeaponType.cpp`** — Read `classes` and `balance` from `ships.json`
   (or separate `weapons.json`) instead of hardcoded `k*` arrays.
   Weapon class selection would use the `probability` field instead of
   the rigged `kAmmoProbabilityMult`.

3. **`StationType.cpp`** — Read from `stations.json` (separate file)
   instead of hardcoded `dockCapacity = 100`.

4. **Dead parameter cleanup** — Remove `propulsion`/`systems`/`turrets`
   from `Item_ShipType_Args` and `Item_StationType_Args`. Update LTSL
   bindings to match.

### Migration Strategy

**Phase 1: Read-only (no behavior change)**
- Create `ships.json` with current hardcoded values as defaults
- C++ factories read from JSON, fall back to hardcoded values on missing
- All existing apps produce identical results

**Phase 2: Enable new archetypes**
- Add new ship archetypes with different stat distributions
- Enable Beam/Missile/Rail weapon classes by setting non-zero probabilities
- Apps choose archetypes by name instead of raw value numbers

**Phase 3: LTSL-facing API**
- New binding `ShipType_GetArchetype(name)` returns an Item configured
  from the named archetype
- Apps call `ShipType_GetArchetype "fighter"` instead of
  `Item_ShipType 10000 20 1 1 1 1 1 1`
