// Copyright (C) 2025  darkoned12000
// SPDX-License-Identifier: GPL-3.0-or-later
// Part of the ltheory-old-test modernization effort (Revamp Work).
// See NOTICE and LICENSE.GPL. Original engine (c) Josh Parnell, public domain.

# NPC AI — State of Play & Integration Plan

Technical reference for making NPCs in `ltheory-main` actually behave: what
the engine already provides, what the current app does with it (almost
nothing), answers to the key capability questions, and a phased plan to wire
it all up. Verified against `src/` on 2026-08-16.

---

## 1. Executive Summary — Answers to the Key Questions

| Question | Status | What exists | What's missing |
|----------|--------|-------------|----------------|
| Do they have a job? | **Framework yes, wiring no** | Full C++ task system (19 tasks) + `Component_Economy`, which auto-assigns jobs and is **already running every frame** inside `Object_System` | The app pushes zero tasks; no stations/zones exist for the economy to work with |
| Can they decide to attack someone? | **Partially** | `Task_Pirate(zone)` autonomously hunts cargo ships in a zone (skips human-owned targets); `Task_Destroy(target)` for direct orders | No utility/goal AI — NPCs don't re-evaluate when threatened; no self-defense; "decisions" are made once at spawn time by script or the economy allocator |
| Can they mine asteroids? | **Yes, if zoned** | `Task_Mine(zone)` / `Task_Drill(target)` extract resources into ship cargo; full loop exists: Mine → Dock → Sell at station market | The 1000-rock belt in `SystemPopulate.lts` is bare objects with no `Object_Zone` wrapper, so nothing can target it; no stations exist to sell to |
| Can they communicate with players or other NPCs? | **No** | Closest things: station **mission board** (structured player↔NPC interaction), widget message system (`SendUp` — UI only), ownership model (players own ships) | No comms/message bus between agents, no hailing, no faction/reputation. This is the one genuinely greenfield area |
| Can they team up to accomplish a goal? | **No** | Central coordination exists via `Component_Economy` (it allocates work across all ships and spawns/retires them based on profit) — but that's top-down allocation, not peer cooperation | No faction/group abstraction, no shared goals, no task negotiation between NPCs. Would be new C++ (or a scripted "dispatcher" approximation) |

**Bottom line:** we do **not** have to start over from scratch. The engine has
a real AI substrate — tasks with chaining, an autonomous profit-driven economy
allocator that is already live in every system, stations with markets/docks/
mission boards, zones with resources, `Player_AI`, ship spawning, and a
project-management task. What's missing is (a) wiring the app to use it, and
(b) three genuinely new layers: reactive decision-making, communication, and
peer cooperation.

---

## 2. What NPCs Can Actually Do Right Now (`ltheory-main`)

Current state of `resource/script/App/ltheory-main.lts` (lines ~162-178):

- **12 AI ships** are spawned on a shell at `planetRadius * 4`, each with a
  random weapon plugged in, added to the system root.
- **No tasks are ever pushed.** No `PushTask` call anywhere in the app or in
  `SystemPopulate.lts`. The ships have physics (they drift) but no autopilot,
  no job, no goals. They are decorative.
- The player ship is piloted by `Player_Human`; AI ships are unowned husks.
- No stations exist in the system (`Object_System` only makes star + nebula +
  starfield + dust; `SystemPopulate.lts` adds one planet + 1000 bare
  asteroids).
- **Consequence:** `Component_Economy` runs every frame (see §3.2) and scans
  the system for markets, resource zones, blueprints — and finds nothing, so
  it does nothing. The whole job-assignment machinery is idling.

So today: NPCs can fly (physics), have weapons (can shoot if ordered), and
that's it. Everything below is latent capability waiting to be wired.

---

## 3. What the Engine Already Provides (C++)

### 3.1 The Task System (`src/liblt/Game/Task/`, `Tasks.h`)

19 task factories, all implemented, all script-bindable:

| Task | Args | Behavior |
|------|------|----------|
| `Task_Mine` | zone | Mines resources from a zone into ship cargo (firing/drilling at mineables) |
| `Task_Drill` | target | Drills a specific object |
| `Task_Pirate` | zone | **Autonomous** hunt: finds cargo-carrying ships in zone, attacks them; skips human-owned targets |
| `Task_Transport` | source, dest, item | Dock at source → load → travel to dest → unload. The workhorse trade task |
| `Task_Dock` | target | Fly to and dock at a station/object |
| `Task_Patrol` | zone | Patrol zone boundaries |
| `Task_Buy` | station, item, qty, next_task | Buy from station market; chains to `next_task` on completion |
| `Task_Sell` | station, item, qty, next_task | Sell to station market; chains to `next_task` |
| `Task_Destroy` | target | Attack/destroy a specific target |
| `Task_Goto` | position | Navigate to a point |
| `Task_Wait` | duration | Idle for N seconds |
| `Task_Spawn` | … | **Spawn new ships** — population growth built into the task system |
| `Task_Produce` | … | Produce items (used on assembly chips) |
| `Task_Mint` / `Task_Research` | … | Blueprint minting / research progression |
| `Task_Play` | … | Play a behavior script |
| `Task_LOD` | owner, task | Distance-based LOD wrapper — run inner task only when near |
| `Task_Manage` | project | Manage a `Project` object (closest thing to goal-directed work) |
| `Task_Custom` | Data | Scriptable task driven by a Data blob — the extension hook |

Key properties:

- **Chaining is native.** Tasks take `next_task` params (Buy/Sell explicitly);
  completion of one triggers the next. Sequential behavior chains are a first-
  class concept, not something we have to build.
- **Autonomy within a task.** `Task_Pirate` does its own target acquisition
  per update — it's not "go to point X and shoot"; it searches the zone for
  live targets. `Task_Mine` similarly picks mineable objects itself.
- **Tasks run per-frame** via the object update loop (each task's `OnUpdate`).

### 3.2 `Component_Economy` — the auto-job-allocator (already running!)

**This is the biggest finding.** `Object_System` includes `Component_Economy`
(`src/liblt/Game/Object/System.cpp:48`), and its `OnUpdate` calls
`Economy.Run(this, s)` **every frame** (`src/liblt/Component/Economy.h:28-30`).

What `Run()` does (per `src/liblt/Component/Economy.cpp`):

1. Scans all interior objects of the system for:
   - **Markets** (`Component_Market`) — stations with markets become trade nodes
   - **Zones with resources** (`Component_Resources`) — mineable areas
   - **Assembly chips / blueprints** — production and research sites
2. Builds **trade routes**: pairs of markets get bidirectional
   `Task_Transport` assignments (A→B and B→A), creating a trade network.
3. Assigns **mining + pirate tasks** to resource zones.
4. Does **profit accounting per route/zone** and allocates ships accordingly:
   profitable lines get more ships (via `Task_Spawn`), unprofitable ones get
   ships retired.

Implication: **the "who gets a job" decision system already exists and is
running.** It's profit-driven, central, and autonomous. The reason it does
nothing in `ltheory-main` is purely that the system contains no markets, no
resource zones, and no blueprints. Spawn a station with a market and a zone
with resources, and NPC ships will start trading and mining **without any new
C++ code**.

### 3.3 Stations, Zones, Resources

- **`Object_Station`** (`src/liblt/Game/Object/Station.cpp`) — full stations
  with `Component_Market`, `Component_MissionBoard`, `Component_Dockable`,
  cargo/storage. Docks give ships places to dock; markets give them things to
  buy/sell; the **mission board is the engine's built-in structured
  player↔NPC interaction point** (the closest thing to "communication" that
  exists today).
- **`Object_Zone`** (`src/liblt/Game/Object/Zone.cpp`) — volume objects with
  `Component_Resources`; resource elements are inherited from the parent's
  resource set and sampled by seed. Zones are what make areas *targetable* by
  `Task_Mine` / `Task_Pirate` / `Task_Patrol`.

### 3.4 Players & Ownership

- `Player_AI(...)` factory exists (`src/liblt/Game/Player.h:56`) alongside
  `Player_Human()` — AI ships can be owned by named AI players, which matters
  because `Task_Pirate` **skips human-owned cargo** (ownership is the engine's
  de-facto faction line today).
- Ships are assets of players (`player.AddAsset ship`); ownership drives
  targeting rules and would drive any future faction/reputation system.

### 3.5 What Does NOT Exist

Be explicit about the gaps, since they define the work:

1. **No decision layer.** Tasks are imperative ("do X"). Nothing watches world
   state and *chooses* what to do. No utility scoring, no threat assessment,
   no self-defense. If a player attacks an NPC miner, the miner keeps mining.
2. **No communication.** No message bus between agents, no hailing, no
   broadcast channel, no way for one NPC to tell another (or the player)
   anything beyond what's visible in the world. Widget messages (`SendUp`) are
   UI-internal only.
3. **No cooperation.** No faction/group abstraction, no shared goals, no task
   negotiation. The economy allocator coordinates *centrally* (top-down
   allocation), which is not the same as peers teaming up.
4. **No reputation/faction data.** Ownership is binary-ish (human vs AI);
   there's no faction ID, standing, or alliance structure on players/objects.

---

## 4. Enabling Plan (Phased)

### Phase 1 — Wire existing tasks into the app (script-only, immediate payoff)

The original integration plan (§5 below). No C++ changes:

1. Track stations/zones in lists in `SystemPopulate.lts`.
2. Spawn mining ships with `Task_Mine` on zoned asteroid areas.
3. Spawn cargo haulers with `Task_Transport` between POIs.
4. Replace static pirates with `Task_Pirate` zone hunters.
5. Add `Task_Patrol` ships around stations.

**Prerequisite within Phase 1:** wrap the asteroid belt (or part of it) in an
`Object_Zone` with resources, or `Task_Mine` has nothing to target. The belt
in `SystemPopulate.lts` is currently bare objects.

### Phase 2 — Give the economy something to run on (script + existing C++)

1. **Spawn stations** (already planned as Pass A in AGENTS.md §8.3: 1–3 per
   system, seeded positions). Stations bring markets, docks, mission boards.
2. Once markets + resource zones exist, **`Component_Economy` starts working
   by itself**: trade routes form, ships get allocated by profit, and the
   population grows/shrinks via `Task_Spawn`. Verify with F3 DebugScene that
   the allocator is assigning tasks (it should be visible as ships changing
   behavior over time).
3. Optionally retire the hand-rolled task pushing in Phase 1 in favor of the
   economy's allocation — per the "one way to do things" principle, the
   economy allocator should become *the* job system, with script only seeding
   stations/zones/initial ships.

**This is where "do they have a job?" flips from no to yes, for free.**

### Phase 3 — Reactive decision-making (new, start scripted)

The engine has no utility AI; we add a thin one. Two tiers:

- **Tier 1 (script, now):** an app-level **dispatcher** script — a
  per-frame (or every-N-seconds) LTSL loop in `ltheory-main` that reads world
  state via existing APIs (`GetInteriorObjects`, distances, cargo, hull) and
  re-pushes tasks. Examples: "if my hull < 30%, drop cargo task and `Task_Goto`
  to nearest station dock"; "if a ship within X range is firing at me, push
  `Task_Destroy` on it (self-defense)"; "if I'm full of ore, go sell." This
  gets us reactive decisions with zero C++ — the interpreter is fast enough
  for ~20-50 NPCs re-evaluating every few seconds.
- **Tier 2 (C++, later):** if the dispatcher proves out, promote it to a
  `Component_AI` (or extend the economy component) with per-ship goal stacks /
  utility scoring in C++ for performance and cleaner structure. Don't start
  here — validate behavior design in script first.

### Phase 4 — Communication & cooperation (genuinely new)

The only area requiring real greenfield work. Options, cheapest first:

1. **Scripted shared state (now):** NPCs "communicate" through app-level LTSL
   variables (e.g., a `threatLevel` or `marketPrices` map the dispatcher
   updates and all ships read). Cheap, centralized, no new engine surface —
   effectively a broadcast channel implemented in the interpreter.
2. **Faction/group component (C++):** add `factionID` + standing to
   `Player`/`Object`, plus a small message API (e.g., `Message_Send(from, to,
   code, payload)` with per-ship inbound queues readable from script). This
   enables hailing, team coordination, and reputation. Design it *after* the
   Phase 3 dispatcher shows which comms patterns actually get used — build
   the bus for the traffic we measured, not a general RPC system.
3. **Player-facing comms:** reuse the station **mission board** (already
   exists) as the structured player↔NPC channel; add an in-game "comms" HUD
   widget fed by the message API when it lands.

### Suggested order & dependencies

```
Phase 1 (script wiring, zoned belt)          ← start here, visible results
    └─> Phase 2 (stations → economy goes live) ← flips "have a job" to yes
        └─> Phase 3 Tier 1 (scripted dispatcher) ← reactive decisions
            └─> Phase 4.1 (scripted shared state)  ← cheap "comms"
                └─> Phase 4.2 (C++ message bus + factions) ← real comms/teamwork
```

---

## 5. Phase 1 Wiring Details (Original Integration Plan)

### 5.1 Tracking Arrays (Required Foundation)

At the top of `Init()` in `SystemPopulate.lts`, add tracking lists:

```lts
var stations List        # Track all stations for AI targeting
var spawnZones List      # Track zones for mining/patrol assignment
```

When creating stations, append to list:

```lts
self.AddInterior station
stations += station      # track station for AI
```

When creating zones, append to list:

```lts
spawnZones += zone       # track zone for mining/patrols
```

### 5.2 Mining Operations (Visible Resource Extraction)

Replace static spawning with active mining ships that use `Task_Mine`. Each
zone gets 2-4 mining ships, orbiting asteroids and firing:

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

      # Task: mine this zone:
      miner.PushTask (Task_Mine zone)

      self.AddInterior miner
```

**Visual Impact:** High — mining ships orbiting asteroids, firing
weapons/beams is immediately obvious resource extraction activity.

### 5.3 Trade Routes Between Stations (Visible Cargo Transport)

Create cargo haulers that use `Task_Transport` to fly between stations,
creating visible trade routes:

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

**Visual Impact:** High — cargo ships flying between stations shows active
trade routes and economic activity.

### 5.4 Pirate Raids on Trade Routes (Dynamic Combat)

Replace static `Task_Destroy` pirate fights with `Task_Pirate` so pirates
actively hunt cargo ships in zones:

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

**Visual Impact:** High — pirates chasing/attacking cargo ships creates
dynamic combat encounters instead of static fights. Note `Task_Pirate`
skips human-owned ships, so the player is safe from them by default.

### 5.5 Station Patrols (Defensive Ships)

Add patrol ships using `Task_Patrol` around each station:

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

**Visual Impact:** Medium — patrol ships moving around stations shows
defensive presence and security.

### 5.6 Complex AI Chains (Mining → Transport → Sell Loop)

Full economic loops with chained tasks: mine resources, dock at a station to
sell, repeat:

```lts
desc "Economic loop"
  var miningShipType (Item_ShipType 800000 rng.Int 1 1 1 1 1 1)

  for zoneIndex 0 zoneIndex < spawnZones.Size zoneIndex.++
    ref zone (cast Object (spawnZones.Get zoneIndex))

    var miner miningShipType.Instantiate
    miner.SetPos zone.GetPos

    var drillWeapon (Item_WeaponType rng.Int + 10)
    while (miner.Plug drillWeapon) 0

    # Task chain: mine, then dock at a station to sell:
    ref station (cast Object (stations.GetRandom rng))

    miner.PushTask (Task_Mine zone)
    miner.PushTask (Task_Dock station)
    miner.PushTask (Task_Sell station (Item_WeaponType 10) 5 Task_Mine(zone))

    self.AddInterior miner
```

**Visual Impact:** High — shows the complete economic cycle: mining →
transport → selling → repeat.

> **Note for Phase 2:** once stations exist and `Component_Economy` is live,
> prefer letting the economy allocator drive these loops (it already builds
> trade nodes with bidirectional `Task_Transport` and assigns miners to
> resource zones by profit). Keep hand-pushed chains only where the economy
> doesn't cover a desired behavior.

---

## 6. Visual Impact Priority

For maximum visible activity, implement in this order:

### Priority 1 (Immediate Visual Payoff)

1. **Mining ships** — orbiting asteroids, firing weapons → obvious resource extraction
2. **Cargo haulers on rails** — flying between stations → trade routes
3. **Pirate raids** — chasing/attacking cargo ships → dynamic combat

### Priority 2 (Supporting Activity)

4. **Station patrols** — defensive presence
5. **Economy-driven loops** — mining → transport → sell cycles (Phase 2, mostly free)

---

## 7. Performance Considerations

### Object Count Targets (Per System)

| Element | Target Count | Purpose |
|---------|-------------|---------|
| Mining ships | 6-12 total | Visible resource extraction |
| Cargo haulers | 3-5 total | Trade route visibility |
| Pirate raiders | 4-8 total | Dynamic combat encounters |
| Patrol ships | 4-9 total | Station defense presence |

### Task Update Overhead

Tasks update per frame via the object update loop. For higher counts:

- Use `Task_LOD(owner, task)` to skip distant AI work.
- Have the Phase 3 dispatcher re-evaluate every N seconds, not every frame
  (task *execution* stays per-frame; only *decisions* are throttled).
- The economy allocator already does its own bookkeeping each frame — watch
  CPU if ship counts grow past ~100.

---

## 8. Testing Checklist

### Rendering Stress Test

- [ ] All NPC ships rendering without frame drops (50+ total)
- [ ] Mining activity visible: ships orbiting asteroids, firing weapons
- [ ] Trade routes visible: cargo ships flying between stations
- [ ] Combat encounters visible: pirates chasing/attacking traders

### AI Behavior Test

- [ ] `Task_Mine` ships extract resources from zoned asteroids
- [ ] `Task_Pirate` hunts cargo ships (and demonstrably ignores the player's human-owned ship)
- [ ] `Task_Transport` routes complete dock→load→travel→unload cycles
- [ ] `Task_Patrol` ships hold their zones
- [ ] After stations spawn: economy allocator visibly assigns tasks and spawns/retires ships over time (watch via F3 DebugScene)

### Reactivity Test (Phase 3)

- [ ] Damaged NPC drops its current task and flees/docks
- [ ] Attacked NPC returns fire (`Task_Destroy` on aggressor)
- [ ] Full-cargo NPC seeks a market to sell

### Procedural Generation Test

- [ ] Each system has unique NPC distribution via seed variation
- [ ] Mining/trade/combat activity differs per system based on zone count
- [ ] Station patrols scale with station count

---

## 9. Open Questions

1. **Economy allocator tuning** — is its profit model sane for our ship/weapon
   costs? Needs a live observation pass (F3 + task logging) before we trust
   it as *the* job system.
2. **Zoning the belt** — one big zone around the whole 1000-rock belt, or
   several smaller zones (better target density for miners)? Smaller zones
   also let pirates and miners share space intentionally.
3. **Dispatcher cadence** — every frame vs every N seconds for Phase 3
   re-evaluation; measure interpreter cost at ~50 NPCs first.
4. **Message bus shape** (Phase 4) — design only after the dispatcher shows
   which comms patterns are actually needed. Don't build a general RPC layer
   speculatively.
