# Ship & Weapon Systems — Complete Guide

> **Purpose:** Complete HOW TO guide for procedural ship creation, weapon systems, combat mechanics, and JSON data integration.  
> **Audience:** Gameplay scripters, modders, AI agents  
> **Last Updated:** 2026-07-30

---

## Table of Contents

1. [Quick Start — Build Your First Combat Fleet](#1-quick-start--build-your-first-combat-fleet)
2. [Ship Creation Pipeline (Deep Dive)](#2-ship-creation-pipeline-deep-dive)
3. [Ship Size Tiers & Value Budgets](#3-ship-size-tiers--value-budgets)
4. [Weapon System (Complete Reference)](#4-weapon-system-complete-reference)
5. [JSON Data File Integration (Proposal)](#5-json-data-file-integration-proposal)
6. [AI & Task System](#6-ai--task-system)
7. [Combat Mechanics](#7-combat-mechanics)
8. [Ship Physics & Movement](#8-ship-physics--movement)
9. [Advanced: Custom Ship Archetypes](#9-advanced-custom-ship-archetypes)

---

## 1. Quick Start — Build Your First Combat Fleet

### Minimal Combat Scenario (Copy-Paste Ready)

```lts
# Create two opposing fleets that fight each other

function Void CreateFleet (Object system Player player Int count Float value Int seed)
  var rng (RNG_MTG seed)
  var shipType (Item_ShipType value (rng.Int))
  var weaponType (Item_WeaponType (rng.Int))
  
  for i 0 i < count i.++
    var ship shipType.Instantiate
    ship.SetPos 5000.0 * rng.Direction         # Random position on sphere
    system.AddInterior ship
    
    # Equip weapons (fill all turret slots)
    while (ship.Plug weaponType) 0
    
    # Assign to player faction
    player.AddAsset ship

# In your app's Initialize()
var playerRed Player_Human
var playerBlue (Player_AI (Traits_Aggressive))  # Aggressive AI

CreateFleet system playerRed 10 100000 1001     # 10 red corvettes
CreateFleet system playerBlue 10 100000 2002    # 10 blue corvettes

# Give them combat orders
for it system.GetInteriorObjects it.HasMore it.Advance
  var ship it.Get
  if ship.HasComponentPilotable
    if ship.GetOwner == playerRed
      # Red fleet: attack any blue ship
      var target FindClosestEnemy ship playerBlue
      if target.IsNotNull
        ship.PushTask (Task_Destroy target)
    else
      # Blue fleet: attack any red ship
      var target FindClosestEnemy ship playerRed
      if target.IsNotNull
        ship.PushTask (Task_Destroy target)
```

---

## 2. Ship Creation Pipeline (Deep Dive)

Ships are created in a **two-step process**: define a blueprint (`Item_ShipType`),
then instantiate a live object from it.

### Step 1: `Item_ShipType` — The Blueprint

```ltsl
var shipType (Item_ShipType value seed capacity compactness integrity propulsion systems turrets)
```

| Parameter      | Type    | Meaning                                      | Default |
|----------------|---------|----------------------------------------------|---------|
| `value`        | Float   | Total budget in credits (determines size)    | —       |
| `seed`         | Int     | RNG seed for procedural mesh generation      | —       |
| `capacity`     | Float   | Cargo capacity multiplier                    | 1.0     |
| `compactness`  | Float   | Mass density multiplier                      | 1.0     |
| `integrity`    | Float   | Health multiplier                            | 1.0     |
| `propulsion`   | Float   | Thrust multiplier                            | 1.0     |
| `systems`      | Float   | Generator power multiplier                   | 1.0     |
| `turrets`      | Float   | Turret count multiplier                      | 1.0     |

**Convenience overload** (defaults all tuning to 1.0):
```ltsl
Item_ShipType value seed
```

#### What happens under the hood

1. **Budget allocation** (`ShipType.cpp:188-197`):
   - 60% → hull value
   - Remaining 40% splits between thruster and generator value

2. **Derives stats from hull value**:
   - `capacity` = `Constant_ValueToCapacity(hullValue, capacity)`
   - `integrity` = `Constant_ValueToIntegrity(hullValue, integrity)`
   - `mass` = `Constant_ValueToMass(hullValue, compactness)`
   - `scale` = `Constant_MassToScale(mass)`

3. **Procedural mesh generation** — calls LTSL script `Item/ShipType/Generate:Main`:
   - Creates a `PlateMesh` (quality level 12)
   - Starts with 2 base boxes (wide-flat + tall-deep)
   - Iteratively adds `2 + sqrt(scale)` plates
   - Each plate: picks a random existing box, picks a random axis, creates a
     smaller box adjacent to it, repeats 1-5 times
   - For larger ships (`scale >= 10`): adds mirrored bottom panels
   - Applies warps: `VerticalCompress` (squish Y) and `HExpand` (widen X at tail)
   - Centers mesh, computes occlusion, creates `Model` with `Material_Metal`

4. **Socket placement**:
   - **Thruster sockets**: Up to 10 pairs on hull surface (rear, lateral, top, bottom, forward)
   - **Turret sockets**: 4 pairs at random surface positions (line-of-sight checked)
   - **Generator sockets**: 1-3 based on `logScale`
   - **Interior sockets**: Proportional to `logScale`

5. **Standard fitted components**:
   - `standardThruster` — default thruster (plugs into all thruster sockets)
   - `standardGenerator` — power generator
   - `standardScanner` — scanner

### Step 2: `.Instantiate` — Create the Live Ship

```ltsl
var ship shipType.Instantiate
```

This creates an `Object_Ship` with **22 components**:

| Component       | Purpose                                    |
|-----------------|--------------------------------------------|
| Affectable      | Runs affector list (e.g. player input)     |
| Asset           | Ownership tracking (which Player owns it)  |
| BoundingBox     | AABB for spatial queries                   |
| Cargo           | Item inventory (Map of Item→Quantity)       |
| Collidable      | Collision detection and resolution          |
| Crew            | Crew slots                                 |
| Cullable        | Distance-based rendering culling           |
| Database        | Data storage                               |
| Detectable      | Detection time tracking (for sensors)      |
| Drawable        | Holds Renderable (mesh) and draws it       |
| Explodable      | Creates explosion effect on death          |
| Integrity       | Health/maxHealth, ApplyDamage, OnDeath     |
| Motion          | Force/torque/velocity/mass/inertia         |
| MotionControl   | SDF-based autopilot steering               |
| Nameable        | Ship name                                  |
| Orientation     | Transform (position, look, up, scale)      |
| Pilotable       | Holds Player reference; calls pilot->Update |
| Scriptable      | Attaches LTSL scripts with Update/Receive  |
| Sockets         | Array of Socket slots for child objects    |
| Supertyped      | Links to the Item_ShipType blueprint       |
| Targets         | List of target objects (for weapons)       |
| Tasks           | Stack of TaskInstance objects (AI behavior) |

`.Instantiate` also auto-plugs:
- Standard thruster (fills all thruster sockets)
- Standard generator
- Standard scanner

---

## 3. Ship Size Tiers & Value Budgets

### Standard Tiers (From Existing Apps)

| Tier       | Value       | Mass (~) | Scale (~) | Health (~) | Cargo (~) | Use Case                    |
|------------|-------------|----------|-----------|------------|-----------|------------------------------|
| **Drone**  | 1,000       | ~10      | ~2        | ~100       | ~50       | Expendable swarm units       |
| **Fighter**| 10,000      | ~100     | ~4        | ~500       | ~200      | Fast interceptors, scouts    |
| **Corvette**| 100,000    | ~1,000   | ~8        | ~3,000     | ~1,000    | Multi-role combat ships      |
| **Frigate**| 1,000,000   | ~10,000  | ~16       | ~15,000    | ~5,000    | Heavy combat, escorts        |
| **Destroyer**| 10,000,000| ~100,000 | ~32       | ~80,000    | ~25,000   | Fleet anchors, siege         |
| **Capital**| 100,000,000 | ~1M      | ~64       | ~400,000   | ~100,000  | Flagships, station killers   |

**Value Budget Breakdown** (from `ShipType.cpp`):
```cpp
hullValue = totalValue * 0.6        // 60% to hull
remainingValue = totalValue * 0.4    // 40% split between:
  thrusterValue = remainingValue * 0.5  // Propulsion
  generatorValue = remainingValue * 0.5 // Power
```

### HOW TO: Choose Ship Values for Gameplay

**Balanced fleet composition:**
```lts
# Mix of ship sizes for interesting combat
var fighters    (Item_ShipType 10000 seed1)      # 10 cheap fast ships
var corvettes   (Item_ShipType 100000 seed2)     # 5 mid-tier ships
var frigates    (Item_ShipType 1000000 seed3)    # 1 heavy hitter
```

**Boss encounter:**
```lts
var bossShip    (Item_ShipType 100000000 seed)   # 1 massive capital ship
var escorts     (Item_ShipType 1000000 seed+1)   # 4 frigate escorts
```

**Swarm tactics:**
```lts
var drones      (Item_ShipType 1000 seed)        # 50+ expendable drones
# Low value = low health, but overwhelming numbers
```

---

## 4. Weapon System (Complete Reference)

### Overview

Weapons in Limit Theory are **procedurally generated** from a seed value. Each weapon has:
- **Class** (Pulse/Missile/Beam/Rail/Torpedo/Flak)
- **Stats** (damage, range, speed, fire rate)
- **Visual** (color, sound, VFX)
- **Socket compatibility** (all weapons use `SocketType_Turret`)

### Weapon Classes (Current + Proposed)

| Class      | Status    | Behavior                                      | DPS Model                 | Sound               |
|------------|-----------|-----------------------------------------------|---------------------------|---------------------|
| **Pulse**  | ✅ Active | Hitscan-like fast projectiles                 | `damage * rate`           | `weapon/pulse/*.ogg`|
| **Missile**| ✅ Active | Slow, homing, high damage                     | `damage / reload_time`    | `weapon/missile1_fire.ogg` |
| **Beam**   | ❌ Disabled| Continuous damage laser (needs rewrite)      | `damage * dt`             | `weapon/beam1_fire.wav` |
| **Rail**   | ✅ Active | Instant-hit, high spread, burst damage        | `damage * rate`           | `weapon/rail1_fire.ogg` |
| **Torpedo**| 📋 Proposed| Slow, long-range, high payload missiles      | `damage / (reload + flight_time)` | —             |
| **Flak**   | 📋 Proposed| Area-of-effect, anti-fighter                 | `damage * rate * aoe_mult`| —                   |
| **Laser**  | 📋 Proposed| Long-range precision energy weapons          | `damage * rate`           | —                   |

### Weapon Stats (How They're Generated)

From `WeaponType.cpp`:

```cpp
// Probability multipliers (controls class distribution)
kAmmoProbabilityMult[WeaponClass] = {
  1.0f,  // Pulse — 50% of all weapons
  0.0f,  // Missile — Rare (special spawn only)
  1.0f,  // Beam — 50% (but disabled via #if 0)
  0.0f   // Rail — Rare
};

// Stat multipliers per class
kAmmoDamageMult[WeaponClass] = { 5, 20, 2, 1 };     // Pulse: high, Missile: very high
kAmmoLifeMult[WeaponClass] = { 2.5, 10, 1.25, 1 }; // Missile: long range
kAmmoSpeedMult[WeaponClass] = { 1e10, 1, 1, 1e10 }; // Pulse/Rail: instant, Missile: slow
kWeaponRateMult[WeaponClass] = { 1, 0.01, 1, 1 };  // Missile: slow fire rate
```

**Actual stat calculation:**
```cpp
damage = AmmoDamageMult * kAmmoDamageMult[class] * Sigfigs(1 + 15*rng.Exp(), 2)
speed = AmmoSpeedMult * kAmmoSpeedMult[class] * (1.0 + 0.5*rng.Exp())
life = AmmoLifeMult * kAmmoLifeMult[class] * (1.0 + rng.Exp())
range = life * speed
color = 0.25*White + ToRGB(HSV(rng, 0.6-0.99 saturation, 0.2-0.6 value))
```

### Creating Weapons in LTSL

```lts
# Simple: Random procedural weapon
var weapon1 (Item_WeaponType 46)
var weapon2 (Item_WeaponType 74)

# Controlled: Seed-based generation (same seed = same weapon)
var rng (RNG_MTG 12345)
var pulseWeapon (Item_WeaponType (rng.Int))      # Likely pulse (50% chance)

# Mixed loadout
var ship (shipType.Instantiate)
ship.Plug (Item_WeaponType 100)   # Slot 1: Pulse
ship.Plug (Item_WeaponType 200)   # Slot 2: Rail
ship.Plug (Item_WeaponType 300)   # Slot 3: Pulse
ship.Plug (Item_WeaponType 400)   # Slot 4: Pulse
```

### HOW TO: Equip Weapons

**Pattern 1: Fill all slots with same weapon**
```lts
var weaponType (Item_WeaponType seed)
while (ship.Plug weaponType) 0    # Loop until .Plug returns false (no free slots)
```

**Pattern 2: Mixed weapon types**
```lts
var pulse (Item_WeaponType 46)
var missile (Item_WeaponType 74)
ship.Plug pulse      # Slot 1
ship.Plug pulse      # Slot 2
ship.Plug missile    # Slot 3
ship.Plug missile    # Slot 4
```

**Pattern 3: Proportional loadout (70% pulse, 30% missile)**
```lts
var rng (RNG_MTG seed)
for i 0 i < 10 i.++
  var weaponType
    switch
      (rng.GetFloat < 0.7) (Item_WeaponType (rng.Int))      # 70% pulse
      otherwise (Item_WeaponType (rng.Int + 1000))          # 30% missile
  if ship.Plug weaponType false break  # Stop when slots full
```

### Weapon DPS Calculation

```cpp
float GetDPS() const {
  float damage = (float)this->damage;
  if (uses)  // Has magazine (missiles/rail)
    damage /= (1.0/rate + magazineTime/(float)uses);  // Account for reload
  
  if (type != WeaponClass_Beam)
    damage *= rate;  // Projectile weapons: multiply by fire rate
  return damage;
}
```

**Interpretation:**
- **Pulse**: High rate of fire → high DPS
- **Missile**: Low rate, high per-shot damage → burst DPS
- **Rail**: Magazine-based → DPS drops during reload
- **Beam**: Continuous → DPS = raw damage value

---

## 5. JSON Data File Integration (Proposal)

### Why JSON?

**Current problems:**
- Magic numbers scattered in LTSL scripts (`10000`, `100000`, etc.)
- No central balance configuration
- Hard to iterate on gameplay without editing scripts

**JSON solution:**
- Central balance files: `resource/gamedata/ships.json`, `weapons.json`
- Hot-reloadable (change file → reload in-game)
- Moddable (community can create balance mods)
- Version-controllable (track balance changes via Git)

### Proposed File Structure

**`resource/gamedata/ships.json`**
```json
{
  "schema_version": "1.0.0",
  "ship_archetypes": {
    "fighter": {
      "display_name": "Fighter",
      "value": 10000,
      "tuning": {
        "capacity": 0.8,
        "compactness": 1.2,
        "integrity": 1.0,
        "propulsion": 1.5,
        "systems": 1.0,
        "turrets": 1.0
      },
      "role": "Fast interceptor, scout",
      "default_weapons": ["pulse_light", "pulse_light"],
      "seed_range": [1000, 9999]
    },
    "corvette": {
      "display_name": "Corvette",
      "value": 100000,
      "tuning": {
        "capacity": 1.0,
        "compactness": 1.0,
        "integrity": 1.2,
        "propulsion": 1.0,
        "systems": 1.2,
        "turrets": 1.2
      },
      "role": "Multi-role combat ship",
      "default_weapons": ["pulse_medium", "pulse_medium", "missile_light"],
      "seed_range": [10000, 19999]
    },
    "frigate": {
      "display_name": "Frigate",
      "value": 1000000,
      "tuning": {
        "capacity": 1.2,
        "compactness": 0.8,
        "integrity": 1.5,
        "propulsion": 0.8,
        "systems": 1.5,
        "turrets": 1.5
      },
      "role": "Heavy combat, fleet escort",
      "default_weapons": ["pulse_heavy", "pulse_heavy", "missile_medium", "rail_medium"],
      "seed_range": [20000, 29999]
    }
  }
}
```

**`resource/gamedata/weapons.json`**
```json
{
  "schema_version": "1.0.0",
  "weapon_archetypes": {
    "pulse_light": {
      "display_name": "Light Pulse Cannon",
      "class": "Pulse",
      "seed_base": 100,
      "stats": {
        "damage_mult": 1.0,
        "range_mult": 1.0,
        "rate_mult": 1.2,
        "spread_mult": 0.8
      },
      "description": "Fast-firing energy weapon, good against fighters"
    },
    "missile_light": {
      "display_name": "Light Missile Launcher",
      "class": "Missile",
      "seed_base": 500,
      "stats": {
        "damage_mult": 2.0,
        "range_mult": 1.5,
        "rate_mult": 0.5,
        "spread_mult": 0.0
      },
      "description": "Homing missiles, effective against slow targets"
    },
    "rail_medium": {
      "display_name": "Medium Railgun",
      "class": "Rail",
      "seed_base": 1000,
      "stats": {
        "damage_mult": 3.0,
        "range_mult": 2.0,
        "rate_mult": 0.3,
        "spread_mult": 2.0
      },
      "description": "High-velocity kinetic weapon, ignores shields"
    },
    "torpedo_heavy": {
      "display_name": "Heavy Torpedo Launcher",
      "class": "Torpedo",
      "seed_base": 2000,
      "stats": {
        "damage_mult": 10.0,
        "range_mult": 5.0,
        "rate_mult": 0.1,
        "spread_mult": 0.0
      },
      "description": "Devastating slow-moving warheads, anti-capital"
    },
    "flak_light": {
      "display_name": "Light Flak Cannon",
      "class": "Flak",
      "seed_base": 3000,
      "stats": {
        "damage_mult": 0.5,
        "range_mult": 0.8,
        "rate_mult": 2.0,
        "spread_mult": 5.0,
        "aoe_radius": 500.0
      },
      "description": "Area-of-effect weapon, shreds fighter swarms"
    }
  }
}
```

### LTSL Integration (Example)

```lts
# JSON loader (would need C++ binding for JSON parsing)
function Object LoadShipFromJSON (String archetype Int seed)
  var config (JSON_Parse (File_Read "resource/gamedata/ships.json"))
  var arch (config.Get "ship_archetypes" archetype)
  
  var value (ToFloat (arch.Get "value"))
  var tuning (arch.Get "tuning")
  
  var shipType (Item_ShipType
    value
    seed
    (ToFloat (tuning.Get "capacity"))
    (ToFloat (tuning.Get "compactness"))
    (ToFloat (tuning.Get "integrity"))
    (ToFloat (tuning.Get "propulsion"))
    (ToFloat (tuning.Get "systems"))
    (ToFloat (tuning.Get "turrets")))
  
  var ship shipType.Instantiate
  
  # Auto-equip default weapons
  var weapons (arch.Get "default_weapons")
  for i 0 i < weapons.Size i.++
    var weaponName (weapons.Get i)
    var weapon (LoadWeaponFromJSON weaponName)
    ship.Plug weapon
  
  ship

# Usage
var fighter (LoadShipFromJSON "fighter" 12345)
var corvette (LoadShipFromJSON "corvette" 67890)
```

### Implementation Plan

**Phase 1: C++ JSON Parser Binding**
- Add `nlohmann/json` library (header-only, no deps)
- Bind `JSON_Parse(String)` → returns LTSL `Data` object
- Bind `.Get(key)` accessor for nested lookups

**Phase 2: LTSL Helper Scripts**
- `resource/script/Data/ShipArchetype.lts` — loads ship configs
- `resource/script/Data/WeaponArchetype.lts` — loads weapon configs

**Phase 3: Migration**
- Replace hardcoded values in `war.lts`, `ltheory-main.lts`, etc.
- Create modding guide: "How to create ship archetypes"

---

## 6. AI & Task System

### Players

```ltsl
var player Player_Human        # Human-controlled player
var ai (Player_AI traits)      # AI player with personality traits
```

`Player_AI` takes a `Traits` struct with 7 float dimensions (0.0–1.0):
- `Aggressive` — combat tendency
- `Creative` — problem-solving
- `Explorative` — exploration drive
- `Greedy` — economic focus
- `Intellectual` — research focus
- `Lawless` — criminal behavior
- `Sociable` — cooperation tendency

### Piloting

```ltsl
player.Pilot ship       # Player takes control of the ship
player.Unpilot           # Release control
player.GetPiloting       # Get the ship being piloted
```

### Task System

Tasks are a **LIFO stack** — only the topmost task runs. When it finishes,
the next one resumes.

```ltsl
ship.PushTask (Task_Destroy target)         # Attack a target
ship.PushTask (Task_Goto destination 512)   # Navigate somewhere
ship.PushTask (Task_Patrol zone)            # Patrol an area
ship.PushTask (Task_Mine zone)              # Mine asteroids
ship.PushTask (Task_Dock station)           # Dock at station
ship.PushTask (Task_Wait 5.0)               # Wait 5 seconds
ship.PushTask (Task_Buy station item qty)   # Buy items
ship.PushTask (Task_Sell station item qty)  # Sell items
ship.PushTask (Task_Play player)            # Default AI behavior
ship.PushTask (Task_Pirate zone)            # Pirate behavior

ship.GetCurrentTask         # Get the active task
ship.ClearTasks              # Remove all tasks
```

### Task Details

#### `Task_Destroy target`
- If target is far away, pushes `Task_Goto(target, destroyDistance)` first
- Orbits target at `radius = 1000 + target.radius + self.radius`
- Pushes `SDF_Sphere` toward orbit point for steering
- Iterates all turret sockets, checks weapon range
- Computes intercept point via `ComputeImpact`
- Broadcasts `MessageTargetPosition`, `MessageTargetObject`, `MessageFire`
- Finished when target is dead

**HOW TO: Custom combat behavior**
```lts
# Kite behavior: attack from max range, retreat when damaged
function Void KiteTask (Object self Object target)
  var weaponRange self.GetMaxWeaponRange  # Get longest weapon range
  var distance self.GetDistance target
  var healthPercent self.GetHealth / self.GetMaxHealth
  
  if healthPercent < 0.5
    # Retreat if below 50% health
    var escapeDir (Normalize (self.GetPos - target.GetPos))
    self.PushTask (Task_Goto (self.GetPos + escapeDir * 10000) 1000)
  else
    # Maintain distance at 90% of max weapon range
    var optimalDist weaponRange * 0.9
    if distance < optimalDist
      # Too close, back off
      var dir (Normalize (self.GetPos - target.GetPos))
      self.PushTask (Task_Goto (target.GetPos + dir * optimalDist) 500)
    else
      # Attack
      self.PushTask (Task_Destroy target)
```

#### `Task_Goto target distance`
- Handles same-container travel via Dijkstra pathfinding through `ComponentNavigable` nodes
- For direct flight, pushes `SDF_Sphere(nextNode, 1.0)` into MotionControl
- Broadcasts `MessageBoost()` during flight
- Handles docking/undocking for sub-container navigation

**HOW TO: Formation flying**
```lts
# Squad of ships maintains formation around leader
function Void FormationTask (Object leader Array<Object> squad)
  var formations (Array Vec3)
  formations += (Vec3 -500 0 -500)   # Left rear
  formations += (Vec3 500 0 -500)    # Right rear
  formations += (Vec3 -500 0 0)      # Left flank
  formations += (Vec3 500 0 0)       # Right flank
  
  for i 0 i < squad.Size i.++
    var ship (squad.Get i)
    var offset (formations.Get (Mod i formations.Size))
    var targetPos (leader.GetPos + offset)
    ship.PushTask (Task_Goto targetPos 100)
```

#### `Task_Patrol zone`
- Picks random positions within zone bounds
- Steers via `SDF_Sphere(target, 1.0)`

**HOW TO: Sentry behavior (patrol until enemy detected)**
```lts
function Void SentryTask (Object self Object zone Player enemyFaction)
  static patrolPos Vec3
  static patrolTime 0.0
  
  # Check for enemies every 2 seconds
  patrolTime += FrameTimer_Get
  if patrolTime > 2.0
    patrolTime = 0.0
    var enemy (FindClosestEnemy self enemyFaction)
    if enemy.IsNotNull
      # Enemy detected, engage!
      self.ClearTasks
      self.PushTask (Task_Destroy enemy)
      return
  
  # Continue patrol
  if patrolPos.Length < 1.0  # Need new patrol point
    var rng (RNG_MTG (TimeMS))
    patrolPos = zone.GetPos + (rng.Direction * zone.GetRadius * 0.8)
  
  var distance self.GetDistance patrolPos
  if distance < 500
    patrolPos = (Vec3 0)  # Reached waypoint, pick new one next frame
  else
    self.PushTask (Task_Goto patrolPos 500)
```

#### `Task_Play player`
- Default "do something useful" behavior
- Spawns new ships when affordable
- Manages economy and fleet

---

## 7. Combat Mechanics

### Messages

| Message                    | Purpose                          | Sent By           | Handled By         |
|----------------------------|----------------------------------|-------------------|--------------------|
| `MessageFire`              | Fire all weapons                 | Task_Destroy      | Weapon             |
| `MessageTargetPosition`    | Where weapons should aim         | Task_Destroy      | Turret             |
| `MessageTargetObject`      | What weapons should track        | Task_Destroy      | Turret             |
| `MessageReload`            | Reload weapons                   | Player/Task       | Weapon             |
| `MessageThrustLinear`      | Apply linear thrust              | MotionControl     | Thruster           |
| `MessageThrustAngular`     | Apply rotational torque          | MotionControl     | Thruster           |

### Weapon Firing Sequence

1. **Task_Destroy** iterates all `SocketType_Turret` sockets
2. For each weapon, checks `range >= distance`
3. Computes intercept point: `ComputeImpact(targetPos, targetVel, weaponSpeed)`
4. Broadcasts `MessageTargetPosition(interceptPoint)`
5. Broadcasts `MessageFire()`
6. **Turret** receives messages:
   - Rotates toward target position (pitch/yaw limits apply)
   - If aligned (within tolerance), calls `WeaponType::Fire()`
7. **Fire()** creates projectile object (`Object_Pulse`/`Object_Missile`/`Object_Rail`)
8. Projectile travels until collision or lifetime expires
9. On hit, calls `Integrity::ApplyDamage(damage)`

### Damage Model

```cpp
void ApplyDamage(Damage amount) {
  health -= amount;
  if (health <= 0) {
    OnDeath();  // Triggers explosion, removes object
  }
}
```

**HOW TO: Custom damage effects**
```lts
# Shield system that absorbs damage
type ShieldSystem
  Float shieldHealth 1000
  Float shieldRegen 10  # HP/second
  Float shieldDelay 5   # Seconds after hit before regen starts
  Float timeSinceHit 0

  function Bool Update (Object self) {
    var dt FrameTimer_Get
    timeSinceHit += dt
    
    # Regenerate shields if not hit recently
    if timeSinceHit > shieldDelay
      shieldHealth = (Min shieldHealth + shieldRegen * dt 1000)
    
    true
  }

  function Void OnDamage (Float damage) {
    if shieldHealth > 0
      # Shields absorb damage
      var absorbed (Min damage shieldHealth)
      shieldHealth -= absorbed
      damage -= absorbed
      timeSinceHit = 0
    
    # Remaining damage to hull
    if damage > 0
      self.ApplyDamage damage
```

### Targeting & Intercept Calculation

From `Task/Destroy.cpp`:

```cpp
Position ComputeImpact(
  Position const& targetPos,
  V3 const& targetVel,
  float projectileSpeed)
{
  // Solve: |targetPos + targetVel*t - weaponPos| = projectileSpeed*t
  // Quadratic equation for intercept time t
  V3 relative = targetPos - weaponPos;
  float a = projectileSpeed*projectileSpeed - Dot(targetVel, targetVel);
  float b = -2.0f * Dot(relative, targetVel);
  float c = -Dot(relative, relative);
  
  float discriminant = b*b - 4*a*c;
  if (discriminant < 0) return targetPos;  // No solution, aim at current position
  
  float t = (-b - Sqrt(discriminant)) / (2*a);
  if (t < 0) t = (-b + Sqrt(discriminant)) / (2*a);
  
  return targetPos + targetVel * t;
}
```

**LTSL approximation (for scripting):**
```lts
function Vec3 PredictTargetPosition (Object target Float projectileSpeed)
  var targetPos target.GetPos
  var targetVel target.GetVel
  var distance (Length (targetPos - self.GetPos))
  var timeToHit (distance / projectileSpeed)
  targetPos + targetVel * timeToHit  # Simple linear prediction
```

---

## 8. Ship Physics & Movement

### Motion Component

State per ship:
- `force` — accumulated linear force this frame
- `torque` — accumulated angular force this frame
- `velocity` — linear velocity (m/s)
- `velocityA` — angular velocity (rad/s)
- `mass` — from ship supertype + cargo
- `inertia` — `mass^3.75 / 3`
- `speed` — `Length(velocity)`

Each frame:
```cpp
force -= velocity * (mass * kLinearDrag);      // Linear drag (0.8)
torque -= velocityA * (inertia * kAngularDrag); // Angular drag (2.0)
velocity += force * (dt / mass);                // F = ma
velocityA += torque * (dt / inertia);           // alpha = torque / I
position += velocity * dt;                      // Integrate position
rotation += velocityA * dt;                     // Integrate rotation
force = 0; torque = 0;                          // Reset accumulators
```

**Top speed**: `GetMaxThrust() / (kLinearDrag * mass)` — equilibrium where drag = thrust.

### Thruster Force Application

Each thruster receives `MessageThrustLinear` and `MessageThrustAngular`:

**Linear thrust:**
```cpp
float activation = Saturate(2.0 * Saturate(-Dot(look, dir)) - 0.5);
V3 force = -look * maxThrust * activation;  // Thrusters face backward
```

**Angular thrust:**
```cpp
V3 torque = Normalize(dir) * maxTorque * Saturate(amount);
```

### SDF-Based Steering (AI)

Tasks don't directly move ships. They push `SDF` (Signed Distance Field) elements
into `MotionControl.elements`. Each frame:

1. Compute SDF gradient at ship's predicted position (`pos + vel*dt`)
2. Gradient points toward nearest attractor
3. `thrustDir = Normalize(gradient)`, attenuated by heading alignment
4. Broadcast `MessageThrustLinear(thrustDir)`, `MessageThrustAngular(thrustDir)`

**Available SDF primitives:**
```lts
SDF_Sphere pos radius           # Point attractor/repulsor
SDF_Capsule pos1 pos2 radius    # Line attractor (for paths)
SDF_Plane normal distance       # Planar constraint
```

**HOW TO: Custom steering behavior**
```lts
# Flock behavior: cohesion + separation + alignment
function Void FlockSteering (Object self Array<Object> neighbors)
  var cohesion (Vec3 0)      # Steer toward center of flock
  var separation (Vec3 0)    # Avoid crowding neighbors
  var alignment (Vec3 0)     # Match neighbor velocities
  var count 0
  
  for i 0 i < neighbors.Size i.++
    var neighbor (neighbors.Get i)
    var offset (neighbor.GetPos - self.GetPos)
    var distance (Length offset)
    
    if distance < 2000  # Within flock range
      cohesion += neighbor.GetPos
      alignment += neighbor.GetVel
      count++
      
      if distance < 500  # Too close, add separation
        separation -= (Normalize offset) * (500 - distance)
  
  if count > 0
    cohesion = cohesion / count  # Average position
    cohesion -= self.GetPos      # Vector to center
    alignment = alignment / count  # Average velocity
  
  # Weighted combination
  var steerDir (Normalize
    cohesion * 0.3 +
    separation * 0.5 +
    alignment * 0.2)
  
  # Push SDF toward steering direction
  var targetPos (self.GetPos + steerDir * 1000)
  self.GetMotionControl.elements.Clear
  self.GetMotionControl.elements.Append (SDF_Sphere targetPos 1.0)
```

---

## 9. Advanced: Custom Ship Archetypes

### HOW TO: Create Specialized Ships

**Interceptor (high speed, low armor):**
```lts
var interceptor (Item_ShipType
  100000          # Value
  seed
  0.5             # capacity: Low cargo
  1.5             # compactness: Dense, small
  0.7             # integrity: Low health
  2.0             # propulsion: High thrust
  1.0             # systems: Normal power
  0.8)            # turrets: Few weapons

var ship interceptor.Instantiate
# Light weapons only
var pulseLight (Item_WeaponType 100)
while (ship.Plug pulseLight) 0
```

**Tank (slow, heavy armor):**
```lts
var tank (Item_ShipType
  1000000         # Value
  seed
  1.5             # capacity: High cargo
  0.7             # compactness: Spread out, large
  2.5             # integrity: Massive health pool
  0.5             # propulsion: Slow
  1.5             # systems: High power for shields
  1.5)            # turrets: Many weapons

var ship tank.Instantiate
# Mixed heavy weapons
var pulseHeavy (Item_WeaponType 500)
var missileHeavy (Item_WeaponType 600)
ship.Plug pulseHeavy
ship.Plug pulseHeavy
ship.Plug missileHeavy
ship.Plug missileHeavy
```

**Missile Boat (long-range artillery):**
```lts
var missileCruiser (Item_ShipType
  500000
  seed
  1.0             # capacity: Normal
  1.0             # compactness: Normal
  1.2             # integrity: Above average
  0.8             # propulsion: Slower
  1.5             # systems: High power for launchers
  2.0)            # turrets: Maximum weapon slots

var ship missileCruiser.Instantiate
# All missile launchers
var missileLauncher (Item_WeaponType 1000)
while (ship.Plug missileLauncher) 0
```

**Carrier (spawns drones):**
```lts
type CarrierScript
  Array<Object> drones (Array Object)
  Int maxDrones 20
  Float spawnInterval 5.0
  Float timeSinceSpawn 0.0

  function Bool Update (Object self) {
    var dt FrameTimer_Get
    timeSinceSpawn += dt
    
    # Spawn drones periodically
    if timeSinceSpawn > spawnInterval && drones.Size < maxDrones
      timeSinceSpawn = 0.0
      var droneType (Item_ShipType 1000 (TimeMS))
      var drone droneType.Instantiate
      drone.SetPos (self.GetPos + (RandomDirection * 500))
      self.GetContainer.AddInterior drone
      drones += drone
      
      # Give drone attack orders
      var enemy (FindClosestEnemy self self.GetOwner.GetEnemyFaction)
      if enemy.IsNotNull
        drone.PushTask (Task_Destroy enemy)
    
    # Remove dead drones
    var i 0
    while i < drones.Size
      var drone (drones.Get i)
      if drone.IsNull || drone.GetHealth <= 0
        drones.Remove i
      else
        i++
    
    true
  }

# In Initialize()
var carrier (Item_ShipType 10000000 seed).Instantiate
carrier.AddScript CarrierScript
```

### Procedural Fleet Composition

**HOW TO: Generate balanced fleets**
```lts
function Void CreateBalancedFleet (Object system Player player Int totalValue Int seed)
  var rng (RNG_MTG seed)
  var spent 0
  
  # 40% of budget on capital ships
  var capitalBudget totalValue * 0.4
  while spent < capitalBudget
    var value (10000000)
    if spent + value > capitalBudget break
    var ship (Item_ShipType value (rng.Int)).Instantiate
    system.AddInterior ship
    player.AddAsset ship
    spent += value
  
  # 35% on frigates
  var frigateBudget totalValue * 0.35
  while spent < capitalBudget + frigateBudget
    var value (1000000)
    if spent + value > capitalBudget + frigateBudget break
    var ship (Item_ShipType value (rng.Int)).Instantiate
    system.AddInterior ship
    player.AddAsset ship
    spent += value
  
  # 25% on fighters (fill remaining with cheap units)
  var fighterBudget totalValue * 0.25
  while spent < totalValue
    var value (10000)
    if spent + value > totalValue break
    var ship (Item_ShipType value (rng.Int)).Instantiate
    system.AddInterior ship
    player.AddAsset ship
    spent += value
  
  Log "Fleet created: " player.GetAssetCount " ships, total value: " spent
```

---

## Appendix: C++ Implementation Notes

### WeaponType.cpp Key Functions

```cpp
Object WeaponType::Fire(ObjectT* w, Position const& origin, V3 const& heading, Object const& target)
```
- Returns newly-created projectile object
- Sets `Damager` component with weapon stats
- Plays 3D positional sound

```cpp
float WeaponType::GetDPS() const
```
- Accounts for magazine reload time
- Beam weapons return raw damage (continuous)

```cpp
Object WeaponType::Instantiate(ObjectT* parent)
```
- Creates `Item_TurretType` mount
- Plugs `Object_Weapon` into turret
- Returns turret (not the weapon itself)

### Socket System

Sockets are **typed slots** on ships/stations:
- `SocketType_Turret` — Weapons
- `SocketType_Thruster` — Engines
- `SocketType_Generator` — Power plants
- `SocketType_Interior` — Cargo, equipment, crew

`.Plug(item)` iterates sockets, finds first matching free slot, instantiates item there.

---

**Last Updated:** 2026-07-30  
**Contributors:** darkoned12000, AI agents  
**See Also:** [SKILL.md](../.opencode/skills/ltheory/SKILL.md), [AGENTS.md](../AGENTS.md), [PRD-LIMIT-THEORY-REBOOT.md](../PRD-LIMIT-THEORY-REBOOT.md)

### Weapon Firing

Each weapon:
- Tracks `targetPos` and `targetObject` via messages
- Auto-aims (tracks target heading at 2π rad/sec)
- Checks line-of-fire (raycast from origin, skip if hits parent)
- Magazine reload with timers
- Cooldown system

### Projectile Types

| Type    | Visual            | Behavior                          |
|---------|-------------------|-----------------------------------|
| Pulse   | Fast projectile   | Hitscan-like, raycast hit detect  |
| Missile | Homing projectile | Thrust + guidance toward target   |
| Beam    | (disabled)        | Continuous damage                 |
| Rail    | Rail shot         | Hitscan-like, high damage         |

### Damage

- `Damager.type` and `Damager.source` set on projectiles
- `Event_Damage(source, dest, damage)` triggers on hit
- When `health <= 0`: `OnDeath()` → `Explodable` creates explosion effect

---

## 7. Rendering

### Procedural Mesh
- Generated by `Item/ShipType/Generate:Main` (LTSL script)
- Uses `PlateMesh` with quality level 12
- Material: `Material_Metal`
- Mesh is stored as `Renderable` on the ShipType item
- Copied to Ship's `Drawable` component on instantiation

### Runtime Drawing
```
ComponentDrawable.Draw:
  Set transform from Orientation
  renderable->Render(state)    # draws the ship's model
```

### Thruster Effects
- Each Thruster renders its mesh + billboard trail
- Trail uses `billboard_axis.jsl` + `thruster_trail.jsl` shaders
- Additive blending, size/color vary by activation level
- Creates a `Light` object for glow

### Weapon Effects
- Weapons create `Light` for muzzle flash
- Flash decays from 1→0 via exponential
- `light->color = 4.0 * flash * weapon_color`

---

## 8. Station System

### Creating Stations

```ltsl
var stationType (Item_StationType value seed capacity integrity compactness systems)
var station stationType.Instantiate
station.SetPos position
system.AddInterior station
```

### Station Components (22 total)

Same as ships plus:
- `Dockable` — hangars and ports for ships to dock
- `Interior` — interior view (first-person rendering)
- `Market` — buy/sell orders
- `MissionBoard` — mission generation
- `Storage` — item storage
- `Zoned` — zone management

### Station AI Manager

```ltsl
var manager (Player_AI traits)
manager.SetName station.GetName + " Manager"
manager.AddAsset station
manager.AddCredits 1000000
```

Stations are given an AI manager player, stocked with items, and have
market sell orders placed.

### Station Marketplace

```ltsl
# Stock the station with items
for i 0 i < 32 i.++
  manager.AddToStorageLocker (Item_CommodityType rng.Int)

# Place sell orders
for each item in storage
  station.AddMarketSellOrder item quantity price
```

---

## 9. Object Hierarchy

```
Universe
  └─ Region (recursive)
       └─ System (Object_System)
            ├─ Star
            ├─ Planet
            │    ├─ Colony
            │    └─ OrbitalStation (disabled)
            ├─ Zone (asteroid field)
            │    ├─ Asteroid
            │    └─ Station (outpost)
            ├─ Ship
            │    ├─ Turret (plugged into sockets)
            │    │    └─ Weapon (plugged into turret)
            │    ├─ Thruster (plugged into sockets)
            │    ├─ PowerGenerator (plugged into sockets)
            │    └─ Scanner (plugged into sockets)
            ├─ WarpNode / WarpRail
            └─ DustFlecks
```

Containment: `system.AddInterior ship` makes the ship a child of the system.

---

## 10. Complete Examples

### Minimal Ship (no weapons, no AI)

```ltsl
var shipType (Item_ShipType 100000 42)
var ship shipType.Instantiate
ship.SetPos (Vec3 1000 0 0)
system.AddInterior ship
```

### Armed Player Ship

```ltsl
var shipType (Item_ShipType 1000000 55 1 1 1 1 1 1)
var ship shipType.Instantiate
ship.SetPos kOrigin

var weaponType (Item_WeaponType 74)
for i 0 i < 4 i.++
  ship.Plug weaponType

system.AddInterior ship

var player Player_Human
player.AddAsset ship
player.Pilot ship
```

### AI Combat Fleet

```ltsl
var ships List

for i 0 i < 10 i.++
  var shipType (Item_ShipType 100000 rng.Int 1 1 1 1 1 1)
  var ship shipType.Instantiate
  ship.SetPos 5000.0 * rng.Direction
  system.AddInterior ship

  var weaponType (Item_WeaponType rng.Int)
  for j 0 j < 4 j.++
    ship.Plug weaponType

  # Each ship attacks a random other ship
  if ships.Size > 0
    ship.PushTask (Task_Destroy (Object (ships.Get (rng.Int ships.Size))))

  ships += ship
```

### AI with Patrol and Patrol Combat

```ltsl
# Create a patrol zone
var zone Object
# ... (zone creation code)

# Create a patrol ship
var patrolShip shipType.Instantiate
patrolShip.SetPos zone.GetCenter
system.AddInterior patrolShip

# Create a pirate
var pirate shipType2.Instantiate
pirate.SetPos zone.GetCenter + 1000 * rng.Direction
system.AddInterior pirate

# They fight each other
patrolShip.PushTask (Task_Destroy pirate)
pirate.PushTask (Task_Destroy patrolShip)
```

---

## 11. Key File Reference

### C++ Core

| File | Role |
|------|------|
| `src/liblt/Game/Object/Ship.cpp` | Ship object creation, component stack |
| `src/liblt/Game/Item/ShipType.cpp` | ShipType: budget, mesh gen, sockets |
| `src/liblt/Game/Item/WeaponType.cpp` | Weapon: class, damage, firing |
| `src/liblt/Game/Item/ThrusterType.cpp` | Thruster: thrust output, power drain |
| `src/liblt/Game/Object/Thruster.cpp` | Thruster: force, trail, messages |
| `src/liblt/Game/Object/Weapon.cpp` | Weapon: aiming, firing, projectiles |
| `src/liblt/Game/Object/Station.cpp` | Station: docks, market, interior |
| `src/liblt/Game/Player.cpp` | Player_Human, Player_AI, Pilot/Unpilot |
| `src/liblt/Game/Widget/HUD.cpp` | Human input: ShipAffector, camera |
| `src/liblt/Game/Messages.h` | All message types |
| `src/liblt/Game/Task/Destroy.cpp` | Attack task |
| `src/liblt/Game/Task/Goto.cpp` | Navigation task |
| `src/liblt/Game/Task/Patrol.cpp` | Patrol task |
| `src/liblt/Game/Task/Play.cpp` | Default AI behavior |
| `src/liblt/Component/Motion.cpp` | Physics integration |
| `src/liblt/Component/MotionControl.cpp` | SDF steering |
| `src/liblt/Component/Tasks.cpp` | Task stack execution |

### LTSL Scripts

| File | Role |
|------|------|
| `resource/script/Item/ShipType/Generate.lts` | Procedural mesh generation |
| `resource/script/Object/Ship.lts` | Ship init, ambient sounds |
| `resource/script/App/war.lts` | 32-ship free-for-all |
| `resource/script/App/dogfight.lts` | 10 enemies vs player |
| `resource/script/App/ltheory-main.lts` | Universe sandbox with ships |
