<!-- Copyright (C) 2025  darkoned12000 -->
<!-- SPDX-License-Identifier: GPL-3.0-or-later -->
<!-- Part of the ltheory-old-test modernization effort (Revamp Work). -->
<!-- See NOTICE and LICENSE.GPL. Original engine (c) Josh Parnell, public domain. -->

# Universe Population — How the Galaxy Is Generated & How to Tune It

**Date:** 2026-08-06
**Purpose:** Reverse-engineered walkthrough of the engine's procedural
**world/space** generation pipeline (`Universe -> Region -> System -> System:Init`),
plus a practical tuning guide for shaping galaxy size, density, and content.
This is the knowledge base for giving our scripted app (`ltheory-main.lts`) the
rich, connected galaxy that the revived old client (`ltheory_app`) demonstrates.

> **Companion doc:** `PROCEDURAL-GENERATION-GUIDE.md` covers the *asset* level
> (how ships/stations/planets are *built* — SDFs, PlateMesh, shaders). This doc
> covers the *space* level (how worlds are *arranged and populated*). The two
> stack: this doc tells you how many systems exist and what sits inside them;
> the other tells you how each object is made.

---

## 1. TL;DR

One C++ call builds a complete, connected galaxy:

```cpp
Object universe = Object_Universe(seed, depth);
```

- `seed` -> the entire galaxy is deterministic from this one number.
- `depth` -> the region-recursion depth; the main lever for **how many systems**.

| depth | approx. systems in galaxy | feel |
|-------|---------------------------|------|
| 0     | ~6                          | one cluster |
| 1     | ~36                         | the demo's galaxy (seed 98080) |
| 2     | ~216                        | dense interstellar space |

> **Where the numbers come from:** the universe spawns exactly **1** top-level
> region (`Universe.cpp:81`, loop `i < 1`), and **every** region spawns
> `childCount = rng->GetInt(1, kMaxChildren) + 5.0f * rng->GetExp()` children
> (`Region.cpp:95-96`). `kMaxChildren = 1` (Region.cpp:21), so that's
> `1 + 5·Exp` -> **mean ~6** children per region (min 1).
>
> Regions only make *systems* at `level == 0` (`Region.cpp:147-154`); a region
> with `level > 0` instead spawns *sub-regions* at `level - 1`
> (`Region.cpp:139-146`). So each extra `depth` multiplies the count by ~6:
> `depth=1` -> ~6 sub-regions x ~6 systems ~ **36 systems**; `depth=2` -> ~216.
> Every system is populated by `System:Init` and wormhole-linked to its siblings.

The old client then **walks down** the containment tree to the first `System`
and drops the player in it (near a station). Our scripted app currently skips
all of this and builds one bare system (`Object_System`), which is why it has a
single system instead of dozens of connected ones.

---

## 2. The Call Chain

```
Object_Universe(seed, depth)                    Universe.cpp:42
+- Universe
   +- economy: currency basis + 8 ore types + colony types   Universe.cpp:49-78
   +- N regions (N = loop below, currently 1)                 Universe.cpp:81
      +- Object_Region(level=depth)                           Region.cpp:72
         +- ~childCount children (packed disks)               Region.cpp:95-124
         |   +- if level > 0 -> Object_Region(level-1)        Region.cpp:139-146
         |   +- if level == 0 -> Object_System + System:Init  Region.cpp:147-154
         |        +- star, nebula, starfield, dust            System.cpp:239-258
         |        +- planets, colonies, stations, pirates,
         |           AI players (scripted)                    System.lts
         +- wormhole MST network connecting all children      Region.cpp:163-188
```

Each object type stringifies to its `ObjectType_*` name via the generated
`ObjectType_String[]` table (visible through `o.GetType` in LTSL — e.g.
`"Universe"`, `"Region"`, `"System"`, `"Station"`, `"Wormhole"`, `"Planet"`).
That is how scripts classify interiors.

---

## 3. Level 0 — `Object_Universe` (`src/liblt/Game/Universe.cpp`)

Creates the top-level `Universe` object and the shared **economy**:

- **Currency basis** — one `Item_OreType` (seed 190) that anchors trade value
  (`Universe.cpp:53-56`).
- **Ore types** — 8 ore types, each `value = kValueMult * (5*Exp + 1)`
  (`Universe.cpp:58-64`). Rarer ore = higher value; quantities are `1/value`,
  so total value is roughly constant.
- **Colony types** — one `"Farming"` colony type with a `Task_Spawn` production
  task (`Universe.cpp:67-78`).
- **Regions** — one region per loop iteration: `level = depth`, random position
  +-10,000, **radius 100** (`Universe.cpp:81-99`). The loop bound (`i < 1`) is
  the knob for **multiple top-level regions** (each one a separate galaxy
  cluster on the map).

### Knobs (Universe.cpp)

| Knob | Line | Default | Effect |
|------|------|---------|--------|
| `kValueMult` | 13 | 10.0 | scales all ore values / economy |
| ore RNG seed | 50 | 190 | the actual ore economy layout |
| `oreTypeCount` | 59 | 8 | ore variety in the galaxy |
| region loop bound | 81 | `i < 1` | **# of top-level regions** |
| `type.radius` | 85 | 100 | region radius -> child scale basis |
| `type.pos` | 84 | +-10,000 | region offset from universe origin |

---

## 4. Level 1+ — `Object_Region` (`src/liblt/Game/Object/Region.cpp`)

The recursion engine. A region:

1. **Picks a name** from the `$system` grammar (`Region.cpp:81`).
2. **Packs children** into non-overlapping disks (`Region.cpp:88-124`):
   - `childCount = rng.GetInt(1, kMaxChildren) + 5.0f * rng.GetExp()`
     -> deterministic 1 child + ~5 exponential-boosted -> **mean ~ 6** children.
   - Each child: random circle position within region radius, radius between
     `minRadius = 0.3*r` and `maxRadius = 0.7*r`, shrinking to avoid overlap
     (`maxOverlap = 0.1*r`). Up to `kMaxAttempts = 1000` tries.
3. **Recurses or terminates** (`Region.cpp:139-154`):
   - `level > 0` -> child is another `Region` at `level-1`.
   - `level == 0` -> child is an `Object_System` **plus** the script
     `Object/System:Init` is executed on it (`Region.cpp:153`). This is the
     bridge from C++ scaffolding to LTSL population.
4. **Varies resources** per child — each child's ore distribution jitters by
   +-0.1 (`Region.cpp:135-137`).
5. **Connects children with wormholes** (`Region.cpp:163-188`):
   - Builds `kConnectivity = 2` spanning trees (MST) over the children, with
     edge weights `distance * (1 + 0.5*Exp)`.
   - Each unique MST edge calls `Connect(a, b)` -> `Object_Wormholes(e1, e2)`
     which links the *nearest systems* of the two subtrees (`Region.cpp:46-70`).
     Result: every system is reachable via wormholes, and high-connectivity
     clusters have redundant routes.

### Knobs (Region.cpp)

| Knob | Line | Default | Effect |
|------|------|---------|--------|
| `kMaxChildren` | 21 | 1 | deterministic base child count per region |
| `childCount += 5.0f * GetExp()` | 96 | +5 | avg extra children (exp. tail) |
| `maxRadius` / `minRadius` | 91-92 | 0.7r / 0.3r | child radius spread |
| `maxOverlap` | 93 | 0.1r | packing overlap allowance |
| `kMaxAttempts` | 88 | 1000 | packing tries per child |
| `kPadding` | 89 | 0 | gap between child disks |
| `kConnectivity` | 165 | 2 | # spanning trees -> **wormhole density** |
| resource jitter | 137 | +-0.1 | per-child economy variation |
| `kPlanetCount` | 22 | 1 | (declared, **unused** — dead constant) |

---

## 5. Level 2 — `Object_System` (`src/liblt/Game/Object/System.cpp`)

Each leaf system (built by C++, `System.cpp:239-258`) gets:

- **Central star** — `Object_Star(GenerateStarColor(rng))` at
  `Spherical(60,000,000, ...)` — i.e. 60M units out; a directional light
  source, not a nearby body (`System.cpp:247-249`).
- **Nebula** — a seeded `Generator_Nebula` cube-map (envMap) tinted from the
  star color (`System.cpp:85-100`), disk-cached per seed.
- **Starfield** — `Renderable_Starfield(seed, kBaseStarCount + 2000*Gaussian)`,
  i.e. ~100,000 stars (`System.cpp:33, 102-106`).
- **Dust** — `Object_DustFlecks()`: 1024 thin additive billboard streaks
  oriented along camera velocity (`System.cpp:253-255`, `DustFlecks.cpp`).
- Color curves / color-grading tables (`System.cpp:114-183`).

> **Note:** `Object_System` does **not** add planets, stations, or asteroids.
> That is entirely the `System:Init` script's job (next section). The C++
> factory only makes the skybox + star.

### Knobs (System.cpp)

| Knob | Line | Default | Effect |
|------|------|---------|--------|
| `kBaseStarCount` | 33 | 100,000 | starfield star count |
| star distance | 248 | 60,000,000 | star orbit / light distance |
| nebula seed/roughness | 87-89 | rng | per-system nebula variation |

---

## 6. Level 3 — `System:Init` (`resource/script/Object/System.lts`)

The script that makes a system *alive*. Every level-0 region's system runs this
(`Region.cpp:153`). It builds, per system:

- **1 planet** "Melian Prime" — radius `systemScale * (1 + 0.1*Exp)`, at
  `0.5 * systemScale` from the system origin (`System.lts:20-31`).
- **1-3 colonies** per planet, each with a `Player_AI` governor, weapons,
  random traits, `Task_Spawn` production (`System.lts:33-66`).
- **Orbital belt** — an `Object_Zone` with a `SDF_Torus` belt ring
  (`System.lts:68-83`).
- **Orbital warp rails** — 4x `WarpNode:BuildRailRadial` rings around the
  planet (`System.lts:85-112`).
- **2 zones** ("fields") at `3.5 * systemScale`, each containing:
  - An **outpost station** (`Item_StationType 1e9 ...`) with a storage locker,
    32 market sell-orders, and an AI manager (`System.lts:146-164`).
  - **2 patrol ships + 3 pirates** each, with `Task_Destroy` feud pairs
    (`System.lts:171-189`).
- **POI rails** — warp rails from the planet to each zone (`System.lts:191-198`).
- **2 AI players** with boss ships, transfer units, and `Task_Play`
  (`System.lts:200-221`).

### Knobs (System.lts)

| Knob | Line | Default | Effect |
|------|------|---------|--------|
| `systemScale` | 10 | 100,000 | unit of in-system distance (positions!) |
| `planetCount` | 11 | 1 | planets per system |
| `playerCount` | 12 | 2 | AI players per system |
| `zoneCount` | 13 | 2 | zones (and thus stations) per system |
| `colonyCount` | 34 | `rng.Int 1 3` | colonies per planet |
| station value | 147 | 1,000,000,000 | station tier / capacity |
| market listings | 159 | 32 | market order slots per station |
| patrol/pirate counts | 172, 180 | 2 / 3 | combat spawns per zone |

> **Units gotcha:** the system object's `Scale` (from Region packing, ~30-70)
> governs bounds/culling, but the *scripted content* positions use absolute
> coordinates in units of `systemScale` (planet at 50,000, zones at 350,000).
> Don't try to make content fit inside the object's radius — the skybox and
> queryable handle visibility, and content is placed on absolute coordinates.

---

## 7. Connectivity & Travel

### 7.1 Wormholes (inter-system)

- `Object_Wormholes(o1, o2)` creates a **pair** of wormhole objects
  (`Wormhole.cpp:90-107`): one at `+400,000 * dir` from o1's origin, one at
  `-400,000 * dir` (near o2), each `SetScale(1000)`, named after the far
  system, and `Navigable` — targetable and shown in navigation/map.
- **Interacting** — a wormhole is *dockable*. `Wormhole::Dock(docker)`
  (`Wormhole.cpp:58-63`) moves the ship into the linked system:
  `dest->GetContainer()->AddInterior(docker)` and repositions it
  `-1500 * Normalize(dest->GetPos())` from the destination wormhole.
- **How the player triggers it** — `Task_Dock(target)` (`Task/Dock.cpp`):
  the ship flies to within `kDockDistance = 100` (`Task/Dock.cpp:12`), then
  calls `target->Dock(self)`. In LTSL:

  ```lts
  ship.PushTask (Task_Dock wormholeObject)   # warp to the linked system
  ```

- **Script accessibility gap:** `Object_Wormhole` (single, unlinked) is bound;
  `Object_Wormholes(o1, o2)` (the pair-builder) is **not** script-bound yet.
  The universe generator creates all pairs today. Binding it is a one-liner
  (`Function_Bind("Object_Wormholes", ...)`) if we want script-built warps.

### 7.2 Warp nodes / rails (in-system)

- `WarpNode:BuildRail(self, p1, p2)` and `WarpNode:BuildRailRadial(...)`
  (`WarpNode.lts:4-40`) lay chains of `Object_WarpNode` + `WarpRail` at
  `WarpNodeSpacing = 15,000` spacing.
- Travel is message-driven: sending `MessageStartUsing(self, target)` to a
  warp node starts warp travel along its rail; `MessageStopUsing` stops it
  (`WarpNode.cpp:404-419`, `Task/Goto.cpp:264`). This is what the Y/U keys in
  `ltheory-main.lts` already do (`ltheory-main.lts:240-252`).

---

## 8. Determinism & Seeds

- Every level uses `RNG_MTG(seed)`:
  - Universe: `RNG_MTG(seed)` -> region pos + region seed (`Universe.cpp:44,86`).
  - Region: `RNG_MTG(args.seed)` -> child positions, seeds (`Region.cpp:80`).
  - System: `RNG_MTG(args.seed)` -> nebula, colors, starfield (`System.cpp:82`).
  - System:Init script: `RNG_MTG(self.GetSeed)` -> all content (`System.lts:9`).
- Each child's `seed` comes from its parent's RNG sequence, so **the entire
  galaxy is fully deterministic from the single `Object_Universe` seed**.
  Same seed = same galaxy, every run. New seed = entirely new galaxy.
- The demo uses `kUniverseSeed = 98080` (`ltheory-app.cpp:42`).

---

## 9. The Walk-Down — Where the Player Spawns

The old client doesn't pick a system specially; it descends the containment
tree taking the **first interior child** at each level until it hits a
`System` (`ltheory-app.cpp:149-156`):

```cpp
Object base = universe;
while (base && base->GetType() != ObjectType_System) {
  InteriorIterator it = Object_GetInteriorObjects(base);
  if (it.HasMore()) base = it.Get();
  else break;
}
```

Then it spawns the player **near the first station** found in that system
(`ltheory-app.cpp:167-179`), with a fallback position, plus a second ship
"Voyager" (`ltheory-app.cpp:203-213`).

> **Bias:** the walk-down always lands in the *first* branch of the first
> region — the "starting system" is effectively the top of the RNG stream.
> To spawn in a different system, choose a different depth/child, or walk down
> a different branch.

---

## 10. Tuning Guide — Recipes for Specific Results

### 10.1 "I want dozens of connected systems" (the demo feel)

| What | Where | Change |
|------|-------|--------|
| Region depth | `Object_Universe(seed, depth)` | `depth = 1` -> ~36 systems; `2` -> ~216 |
| Children per region | `Region.cpp:96` | raise `5.0f` boost (e.g. `8.0f`) |
| Base children | `Region.cpp:21` | raise `kMaxChildren` (e.g. 2-4) |
| Wormhole density | `Region.cpp:165` | raise `kConnectivity` (e.g. 3-4) |

With `depth=1` you get ~6 sub-regions x ~6 systems ~ 36 systems, every one
populated by `System:Init`, all wormhole-linked. That's the demo.

### 10.2 "I want multiple galaxy clusters"

| What | Where | Change |
|------|-------|--------|
| Top-level regions | `Universe.cpp:81` | loop `i < 1` -> `i < N` |
| Cluster separation | `Universe.cpp:84` | `type.pos` range +-10,000 |

### 10.3 "I want busier systems"

| What | Where | Change |
|------|-------|--------|
| Stations per system | `System.lts:13` (`zoneCount`) | 2 -> 3-5 |
| Planets per system | `System.lts:11` | 1 -> 2-3 (loop planets) |
| AI players per system | `System.lts:12` | 2 -> N |
| Colonies per planet | `System.lts:34` | widen `rng.Int 1 3` |
| Pirates/patrols | `System.lts:172,180` | bump counts |

### 10.4 "I want a richer economy"

| What | Where | Change |
|------|-------|--------|
| Ore values | `Universe.cpp:13` (`kValueMult`) | 10 -> N |
| Ore variety | `Universe.cpp:59` | 8 -> N |
| Resource jitter | `Region.cpp:137` | +-0.1 -> +-N |

### 10.5 "I want denser dust / brighter nebula"

The geometry is identical across apps; what reads as "missing" dust/clouds is
actually the **extra bloom pass** the old client appends (see section 12). Add

```lts
passes.Append (RenderPass_Bloom 64 32)
```

to the render pass list to make the additive dust streaks and nebula glow.

---

## 11. Why `ltheory-main.lts` Currently Has Only One System

`ltheory-main.lts` line 156 does:

```lts
root = (Object_System (Vec3 15.012) universeSeed)
```

That creates a **bare system** (star + skybox + dust) with *no* regions, no
wormholes, no stations, no colonies — then `SystemPopulate:Init` manually adds
one planet + asteroids. It bypasses `Object_Universe` -> `Object_Region`
entirely.

The demo (`ltheory_app`) calls `Object_Universe(seed, 1)` and walks down, so it
gets the full ~36-system galaxy with every system populated by `System:Init`.

### The scaffold plan (bringing the galaxy to the scripted app)

1. **Build the galaxy** — replace the bare `Object_System` with:

   ```lts
   root = (Object_Universe universeSeed 1)
   while (root.GetType != "System")
     var found false
     for it root.GetInteriorObjects it.HasMore it.Advance
       if (! found)
         root = it.Get
         found = true
     if (! found)
       break
   ```

2. **Spawn near a station** — scan the system's interiors for `"Station"`
   (the demo's `ltheory-app.cpp:167-179` logic), fallback to the planet.
3. **Skip `SystemPopulate:Init`** — the universe's systems are already fully
   populated; don't double-add a planet.
4. **Add the extra bloom** — `(RenderPass_Bloom 64 32)` for the dust/nebula
   glow (per section 10.5).
5. **(Optional) Bind `Object_Wormholes`** so warps can be script-built
   (`Wormhole.cpp:90`, currently unbound).

---

## 12. Rendering & Presentation Learnings

- `RenderPass_Camera` already contains the full lighting pipeline:
  Visibility -> DepthPrepass -> HiZ -> GBuffer -> GlobalLighting ->
  LocalLighting -> Blended -> **DustClouds** -> Particles -> LensFlares ->
  **Bloom(128, 64)** -> MotionBlur -> colorgrade
  (`src/liblt/Game/RenderPass/Camera.cpp:26-40`).
- The old client appends a **second** `RenderPass_Bloom(64, 32)`
  (`ltheory-app.cpp:126`) after the HUD pass. The lower threshold (32) is what
  makes the faint additive dust/nebula actually visible. With only the internal
  bloom (threshold 64) they read as black.
- Dust: `Object_DustFlecks` = 1024 additive billboards, size `4 x 0.06*|v|`,
  oriented along velocity (`DustFlecks.cpp:14-15, 45-48`). Pure additive ->
  needs bloom to register.

---

## 13. File Reference Map

| File | Role |
|------|------|
| `src/liblt/Game/Universe.cpp` | top-level universe, economy, region creation |
| `src/liblt/Game/Object/Region.cpp` | region recursion, child packing, wormhole MST |
| `src/liblt/Game/Object/System.cpp` | system factory: star, nebula, starfield, dust |
| `src/liblt/Game/Object/Wormhole.cpp` | wormhole pair + `Dock` warp |
| `src/liblt/Game/Task/Dock.cpp` | `Task_Dock` — the warp trigger |
| `src/liblt/Game/Task/Goto.cpp` | navigation along warp nodes/rails |
| `src/liblt/Game/Object/WarpNode.cpp` | warp node/rail travel logic |
| `resource/script/Object/System.lts` | per-system population (planets/stations/...) |
| `resource/script/Object/WarpNode.lts` | rail-building script API |
| `resource/script/App/ltheory-main.lts` | our single-system app (to be scaffolded) |
| `src/old/ltheory/ltheory-app.cpp` | the revived old client: `Object_Universe(98080, 1)` |
| `src/liblt/Game/RenderPass/Camera.cpp` | the full camera/render pipeline |
| `src/liblt/Game/RenderPass/Visibility.cpp` | `visible[0]` = camera's container |
| `src/liblt/Game/Camera.cpp` | camera reparenting into target's system |
| `src/liblt/LTE/RNG.h` | the seeded RNG used at every level |

---

## 14. Next Steps

1. **Scaffold `ltheory-main.lts`** per section 11 to adopt the full galaxy.
2. **Bind `Object_Wormholes`** for script-built warps (section 7.1).
3. **Tune via section 10** and record the chosen values as config keys
   (`gameConfig.txt`): `depth`, `regionCount`, `connectivity`, `zoneCount`.
4. Consider a **system chooser** to replace the fixed first-branch walk-down
   (section 9) — e.g. pick a random region/system at a given depth.
