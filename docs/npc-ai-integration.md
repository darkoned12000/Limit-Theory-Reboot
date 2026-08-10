// Copyright (C) 2025  darkoned12000
// SPDX-License-Identifier: GPL-3.0-or-later
// Part of the ltheory-old-test modernization effort (Revamp Work).
// See NOTICE and LICENSE.GPL. Original engine (c) Josh Parnell, public domain.

# NPC AI Integration — Wiring Existing Tasks into System.lts

Technical reference for integrating the existing C++ task system into scripted
system population (`resource/script/Object/System.lts`) to create visible NPC
behaviors: mining operations, trade routes, pirate raids, station patrols, and
complex economic loops. All tasks are already implemented in `src/liblt/Game/Task/`;
this document discusses how to wire them without modifying C++ code.

---

## 1. Current State Assessment

### What Exists (C++)

The engine has a rich task system (`src/liblt/Game/Tasks.h`, ~20 tasks) that is
mostly unused in scripts. Key available tasks:

| Task | File | Behavior | Visual Result |
|------|------|----------|---------------|
| `Task_Mine(zone)` | `Game/Task/Mine.cpp` | Fires weapons at mineable objects in zone, extracts resources | Mining ships orbiting asteroids, firing beams/lasers |
| `Task_Pirate(zone)` | `Game/Task/Pirate.cpp` | Attacks cargo-carrying ships in zone (not human-owned) | Pirate ships chasing/engaging traders |
| `Task_Transport(source, dest, item)` | `Game/Task/Transport.cpp` | Ships dock at source → load → travel to dest → unload | Cargo haulers flying between stations on rails |
| `Task_Dock(target)` | `Game/Task/Dock.cpp` | Ship flies to and docks at station/object | Ships entering station docking bays |
| `Task_Patrol(zone)` | `Game/Task/Patrol.cpp` | Ship patrols zone boundaries | Patrol ships moving in patterns around zones |
| `Task_Buy(station, item, qty, next_task)` | `Game/Task/Buy.cpp` | Buys items from station market | Traders visiting stations to purchase goods |
| `Task_Sell(station, item, qty, next_task)` | `Game/Task/Sell.cpp` | Sells items to station market | Miners/traders offloading resources at stations |

### What's Missing (Scripts)

In `resource/script/Object/System.lts`:

1. **Stations aren't tracked** — created but not stored in a list for AI targeting
2. **Pirates are static combat** — lines 172-190: pirates just fight each other via `Task_Destroy`, no dynamic hunting
3. **No mining activity** — zones exist (lines 68-84) but no ships actively mine them
4. **No trade routes** — cargo ships don't transport between locations using `Task_Transport`

---

## 2. Integration Points in System.lts

### 2.1 Tracking Arrays (Required Foundation)

At the top of `Init()` (~line 7), add tracking lists:

```lts
var stations List        # Track all stations for AI targeting
var spawnZones List      # Track zones for mining/patrol assignment
```

When creating stations (~line 148-150), append to list:

```lts
self.AddInterior station
stations += station      # NEW LINE — track station for AI
```

When creating zones (~line 169), append to list:

```lts
spawnZones += zone       # NEW LINE — track zone for mining/patrols
```

### 2.2 Mining Operations (Visible Resource Extraction)

**Location:** After zone creation section (~after line 170)

**Discussion:** Replace static pirate/patrol spawning with active mining ships that use `Task_Mine`. Each zone gets 2-4 mining ships equipped with drill weapons, orbiting asteroids and firing.

```lts
desc "Mining operations"
  var mineShipType (Item_ShipType 500000 rng.Int 1 1 1 1 1 1)
  
  for zoneIndex 0 zoneIndex < spawnZones.Size zoneIndex.++
    ref zone (cast Object (spawnZones.Get zoneIndex))
    
    var minerCount (rng.Int 2 4)
    for i 0 i < minerCount i.++
      var miner mineShipType.Instantiate
      miner.SetPos zone.GetPos + rng.Sphere * zone.GetScale
      
      # Give miners mining weapons:
      var drillWeapon (Item_WeaponType rng.Int + 10)
      while (miner.Plug drillWeapon) 0
      
      # Task chain: Mine → Sell at station → Repeat:
      miner.PushTask (Task_Mine zone)
      
      self.AddInterior miner
```

**Visual Impact:** High — mining ships orbiting asteroids, firing weapons/beams is immediately obvious resource extraction activity.

### 2.3 Trade Routes Between Stations (Visible Cargo Transport)

**Location:** After station creation (~after line 170)

**Discussion:** Create cargo haulers that use `Task_Transport` to fly between stations on warp rails, creating visible trade routes. Each pair of stations gets a dedicated transport ship with storage capacity.

```lts
desc "Trade routes"
  var cargoShipType (Item_ShipType 3000000 rng.Int 1 1 1 1 1 1)
  
  for i 0 i < pois.Size - 1 i.++
    ref a (pois.Get i)
    ref b (pois.Get i + 1)
    
    var cargoShip cargoShipType.Instantiate
    cargoShip.SetPos a.pos
    
    # Give cargo ship storage capacity:
    var transferUnit (Item_TransferUnitType 50 rng.Int 1 1 1 1)
    for j 0 j < 8 j.++
      cargoShip.Plug transferUnit
    
    # Transport goods between stations continuously:
    var tradeItem (Item_WeaponType rng.Int + 20)
    cargoShip.PushTask (Task_Transport a b tradeItem)
    
    self.AddInterior cargoShip
```

**Visual Impact:** High — cargo ships flying between stations on warp rails shows active trade routes and economic activity.

### 2.4 Pirate Raids on Trade Routes (Dynamic Combat)

**Location:** Replace current pirate section (~lines 171-190)

**Discussion:** Current pirates just fight each other statically via `Task_Destroy`. Change to use `Task_Pirate` so they actively hunt cargo ships in zones, creating dynamic combat encounters.

```lts
desc "Pirate raiders"
  var pirateShipType (Item_ShipType 100000 rng.Int 1 1 1 1 1 1)
  
  for zoneIndex 0 zoneIndex < spawnZones.Size zoneIndex.++
    ref zone (cast Object (spawnZones.Get zoneIndex))
    
    var pirateCount (rng.Int 2 5)
    for i 0 i < pirateCount i.++
      var pirate pirateShipType.Instantiate
      
      # Give pirates weapons:
      var weapon (Item_WeaponType rng.Int + 4)
      while (pirate.Plug weapon) 0
      
      pirate.SetPos zone.GetPos + rng.Sphere * zone.GetScale
      
      # Pirate task: hunt ships in this zone:
      pirate.PushTask (Task_Pirate zone)
      
      self.AddInterior pirate
```

**Visual Impact:** High — pirates chasing/attacking cargo ships creates dynamic combat encounters instead of static fights.

### 2.5 Station Patrols (Defensive Ships)

**Location:** After station creation (~after line 170)

**Discussion:** Add patrol ships using `Task_Patrol` to protect stations, creating defensive presence around each outpost. Each station gets a dedicated patrol zone and 2-3 patrol ships.

```lts
desc "Station patrols"
  var patrolShipType (Item_ShipType 500000 rng.Int 1 1 1 1 1 1)
  
  for stationIndex 0 stationIndex < stations.Size stationIndex.++
    ref station (cast Object (stations.Get stationIndex))
    
    # Create patrol zone around each station:
    var patrolZone (Object_Zone self rng.Int station.GetPos station.GetScale * 2 SDF_Sphere 0 1 96 1.0 0.0 0.0 0.0)
    self.AddInterior patrolZone
    
    # Spawn 2-3 patrol ships per station:
    var patrolCount (rng.Int 2 3)
    for i 0 i < patrolCount i.++
      var patrol patrolShipType.Instantiate
      
      var weapon (Item_WeaponType rng.Int + 5)
      while (patrol.Plug weapon) 0
      
      patrol.SetPos station.GetPos + rng.Sphere * station.GetScale * 1.5
      
      # Patrol the zone around station:
      patrol.PushTask (Task_Patrol patrolZone)
      
      self.AddInterior patrol
```

**Visual Impact:** Medium — patrol ships moving around stations shows defensive presence and security.

### 2.6 Complex AI Chains (Mining → Transport → Sell Loop)

**Location:** After mining operations (~after line 170+)

**Discussion:** Create full economic loops with visible activity: mining ships that mine resources, dock at stations to sell, then repeat the cycle using chained tasks (`Task_Mine` → `Task_Dock` → `Task_Sell` → back to `Task_Mine`).

```lts
desc "Economic loop"
  var miningShipType (Item_ShipType 800000 rng.Int 1 1 1 1 1 1)
  
  for zoneIndex 0 zoneIndex < spawnZones.Size zoneIndex.++
    ref zone (cast Object (spawnZones.Get zoneIndex))
    
    # Mining ship that mines → docks at station to sell → repeats:
    var miner miningShipType.Instantiate
    miner.SetPos zone.GetPos
    
    var drillWeapon (Item_WeaponType rng.Int + 10)
    while (miner.Plug drillWeapon) 0
    
    # Task chain: Mine resources, then dock at nearest station to sell:
    ref station (cast Object (stations.GetRandom rng))
    
    miner.PushTask (Task_Mine zone)
    miner.PushTask (Task_Dock station)
    miner.PushTask (Task_Sell station (Item_WeaponType 10) 5 Task_Mine(zone))
    
    self.AddInterior miner
```

**Visual Impact:** High — shows complete economic cycle: mining → transport → selling → repeat.

---

## 3. Visual Impact Priority

For maximum visible activity in test scenarios, implement in this order:

### Priority 1 (Immediate Visual Payoff)

1. **Mining ships** — orbiting asteroids, firing weapons → very obvious resource extraction
2. **Cargo haulers on rails** — flying between stations on warp paths → shows trade routes
3. **Pirate raids** — pirates chasing/attacking cargo ships → dynamic combat encounters

### Priority 2 (Supporting Activity)

4. **Station patrols** — ships moving around stations → defensive presence
5. **Complex AI chains** — mining → transport → sell loops → economic cycles

All use existing C++ tasks; no new code needed beyond wiring into `System.lts`.

---

## 4. Performance Considerations

### Object Count Targets (Per System)

| Element | Target Count | Purpose |
|---------|-------------|---------|
| Mining ships | 6-12 total | Visible resource extraction activity |
| Cargo haulers | 3-5 total | Trade route visibility |
| Pirate raiders | 4-8 total | Dynamic combat encounters |
| Patrol ships | 4-9 total | Station defense presence |

### Task Update Overhead

Tasks are updated via `root.Update dt` in the app's main loop (`ltheory-unitest.lts:290`). Each task's `OnUpdate()` is called per frame. For high object counts:

- Consider LOD-based task updates (use `Task_LOD(owner, task)` wrapper)
- Group AI updates every N frames instead of every frame for non-critical tasks
- Use distance-based culling for ships far from player camera

---

## 5. Testing Checklist

### Rendering Stress Test

- [ ] All NPC ships rendering without frame drops (100+ total across systems)
- [ ] Mining activity visible: ships orbiting asteroids, firing weapons
- [ ] Trade routes visible: cargo ships flying between stations on rails
- [ ] Combat encounters visible: pirates chasing/attacking traders

### AI/Physics Stress Test

- [ ] Ships following warp rail routes correctly via `Task_Transport`
- [ ] Mining ships extracting resources from asteroids via `Task_Mine`
- [ ] Pirate ships hunting cargo ships via `Task_Pirate` (not static combat)
- [ ] Patrol ships moving around stations via `Task_Patrol`

### Procedural Generation Test

- [ ] Each system has unique NPC distribution via seed variation
- [ ] Mining/trade/combat activity differs per system based on zone count
- [ ] Station patrols scale with station count
- [ ] Pirate density varies by zone type

---

## 6. Next Steps (Concrete Implementation Order)

1. **Add tracking arrays** — `stations` and `spawnZones` lists in `System.lts:Init()`
2. **Implement mining ships** — use `Task_Mine` for visible resource extraction
3. **Implement trade routes** — use `Task_Transport` between stations on rails
4. **Replace static pirates** — use `Task_Pirate` for dynamic hunting behavior
5. **Add station patrols** — use `Task_Patrol` around each station zone
6. **Test performance** — monitor frame rates with 100+ NPC ships active

All changes are script-only; no C++ modifications required beyond existing task implementations.
