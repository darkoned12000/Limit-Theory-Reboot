// Copyright (C) 2025  darkoned12000
// SPDX-License-Identifier: GPL-3.0-or-later
// Part of the ltheory-old-test modernization effort (Revamp Work).
// See NOTICE and LICENSE.GPL. Original engine (c) Josh Parnell, public domain.

# Engine & Universe Guide — Architecture Decisions + Maximum Test Scenario

Master reference for engine architecture decisions (graphics API, rendering path),
universe population tuning, NPC AI integration, atmospheric effects, and the
"maximum engine test scenario" that exercises all major systems simultaneously.
Read before making large changes to rendering, universe generation, or game logic.

---

## Part 1: Graphics Architecture — OpenGL 4.6 vs Vulkan

### Current State

- **OpenGL 4.6** via GLAD 2.0.8 (vendored), SFML 3.1.0 window/context
- Programmable pipeline: FBOs, MRT, VBOs, single global VAO
- All `.jsl` shaders forced to `#version 460 core`
- Core profile bit intentionally left off (SFML + Mesa GLX crash)

### Pros of Staying with OpenGL 4.6

| Factor | Detail |
|--------|--------|
| **Maturity** | Already working; all 169 shaders compile, zero GL errors under BUILD_STRICT |
| **Tooling** | Debuggers (RenderDoc, apitrace), profilers, shader analyzers are mature and widely available |
| **Codebase fit** | Engine is ~72k LOC C++ with custom reflection/serialization; OpenGL's implicit state model matches existing patterns |
| **SFML integration** | SFML provides GL context natively; no extra layer needed for windowing/input/audio |
| **Dev velocity** | No API migration needed — focus on gameplay, procedural generation, NPC AI instead of driver compatibility |

### Cons of Staying with OpenGL 4.6

| Factor | Detail |
|--------|--------|
| **State management overhead** | OpenGL's implicit state machine requires more CPU-side tracking than Vulkan's explicit model |
| **Driver variability** | Different vendors implement extensions/features differently; edge cases harder to debug across platforms |
| **Multi-threading limits** | OpenGL context is single-threaded by default; harder to parallelize rendering work compared to Vulkan's command buffers |

### Pros of Moving to Vulkan

| Factor | Detail |
|--------|--------|
| **Explicit control** | Full control over memory allocation, synchronization, pipeline state — eliminates hidden overhead |
| **Multi-threading** | Command buffer recording can be parallelized across CPU cores; better utilization on modern hardware |
| **Performance ceiling** | Lower driver overhead, more predictable performance on high-end GPUs |
| **Modern features** | Native support for ray tracing (via extensions), mesh shaders, subgroup operations |

### Cons of Moving to Vulkan

| Factor | Detail |
|--------|--------|
| **Massive effort** | Estimated 6-12 months full-time for equivalent functionality; requires rewriting Renderer.cpp, Shader.cpp, GL.h wrappers, all draw paths |
| **Boilerplate explosion** | Vulkan requires explicit management of pipelines, descriptors, memory heaps, synchronization — ~5x more code than OpenGL |
| **Debugging complexity** | Validation layers help but are verbose; driver bugs harder to isolate without OpenGL's implicit safety nets |
| **SFML incompatibility** | SFML doesn't provide Vulkan context natively; need separate windowing layer or custom integration |
| **Diminishing returns** | Engine is CPU-bound on procedural generation/AI updates, not GPU-bound — Vulkan won't fix the actual bottleneck |

### Files Affected by Vulkan Migration (Estimated)

| File/Module | Impact Level | Notes |
|-------------|-------------|-------|
| `src/liblt/LTE/GL.h` / `GL.cpp` | Complete rewrite | All OpenGL wrappers → Vulkan equivalents |
| `src/liblt/LTE/Renderer.cpp` | Complete rewrite | Context init, draw loop, FBO management |
| `src/liblt/LTE/Shader.cpp` | Major changes | Pipeline creation, descriptor sets vs uniforms |
| `resource/shader/*.jsl` (169 files) | Minor changes | GLSL → SPIR-V compilation; some syntax adjustments |
| `src/liblt/Game/GraphicsGenerator/*.cpp` | Moderate changes | VBO uploads, draw calls |
| `src/liblt/UI/Compositor.cpp` | Major changes | Render targets, blending |
| `configure.py` / `CMakeLists.txt` | Minor changes | Link Vulkan SDK instead of OpenGL/GLAD |

### Architecture Changes Required for Vulkan

1. **Explicit pipeline management** — Replace implicit GL state with explicit VkPipeline objects per shader variant
2. **Descriptor set layout** — Replace glUniform calls with descriptor sets/buffers for MVP matrices, textures
3. **Command buffer recording** — Replace immediate draw calls with recorded command buffers submitted to queues
4. **Memory allocation** — Replace glBufferData with explicit VkDeviceMemory management (linear vs coherent)
5. **Synchronization** — Replace implicit GL barriers with explicit VkSemaphore/VkFence/VkBarrier objects
6. **Swap chain** — Replace SFML window surface with Vulkan swap chain for presentation

### Verdict: Is Vulkan Needed?

**No.** For this engine's current scope and goals, OpenGL 4.6 is sufficient and the right choice. Reasons:

1. **CPU-bound bottleneck** — Procedural generation, AI updates, LTSL interpretation are the actual performance limits, not GPU rendering
2. **Dev velocity priority** — Focus on gameplay features (NPC AI, universe population, atmospheric effects) instead of API migration
3. **Working foundation** — OpenGL 4.6 is already working with zero issues; Vulkan would introduce months of risk for marginal gains
4. **Future-proofing option** — If GPU becomes a bottleneck later (e.g., ray tracing, massive draw counts), Vulkan can be considered then

**Recommendation:** Stay with OpenGL 4.6. Invest effort in:
- NPC AI integration (`docs/npc-ai-integration.md`)
- Universe population tuning (`docs/universe-population.md`)
- Atmospheric effects (dust/nebula enhancements)
- LTSL tooling and gameplay features

---

## Part 2: Maximum Engine Test Scenario — "The Nexus Corridor"

### Concept

A connected cluster of 4-6 star systems linked by wormholes, each with distinct
features, populated with NPC ships on warp rails, dense asteroid fields, stations,
and atmospheric effects — all running simultaneously to test rendering, physics,
AI, and procedural generation.

This scenario exercises:
- Multi-system universe via `Object_Universe(seed, depth)`
- NPC AI tasks (`Task_Mine`, `Task_Transport`, `Task_Pirate`, etc.)
- Atmospheric effects (dust clouds, nebula backdrops)
- Warp rails connecting key points
- Wormholes for system traversal

### System Layout (4 Systems)

```
System Alpha (Hub) ←wormhole→ System Beta (Industrial)
     ↓ wormhole                    ↓ wormhole
System Gamma (Combat) ←wormhole→ System Delta (Exploration)
```

#### System Alpha: "The Nexus" (Central Hub)

- **Features:** Mega-station with multiple docking rings, 2 planets, dense traffic
- **NPC Ships:** 30+ ships on warp rails between station and planets
- **Effects:** Heavy dust clouds, bright star, nebula backdrop
- **Purpose:** Test high-density rendering + AI pathfinding

#### System Beta: "Forge" (Industrial)

- **Features:** Mining stations, asteroid belt with 5000+ rocks, gas giant
- **NPC Ships:** Mining drones on patrol routes, cargo haulers between stations
- **Effects:** Dust storms near asteroids, dimmer star
- **Purpose:** Test massive object counts + SDF rendering

#### System Gamma: "Crossfire" (Combat Zone)

- **Features:** 3 planets with orbital stations, pirate bases, combat zones
- **NPC Ships:** Patrol fighters, capital ships engaging in scripted battles
- **Effects:** Explosions, debris fields, atmospheric scattering on planets
- **Purpose:** Test physics + particle systems + AI combat

#### System Delta: "The Void" (Exploration)

- **Features:** Sparse systems, hidden wormholes, rare resources
- **NPC Ships:** Scout ships, lone traders, mysterious entities
- **Effects:** Dense nebula clouds, starfields with parallax layers
- **Purpose:** Test atmospheric effects + discovery mechanics

---

## Part 3: Implementation Plan (Building on Existing Code)

### 3.1 Universe Structure (`Object_Universe`)

**File:** `resource/script/App/ltheory-unitest.lts` (already implemented)

Current implementation uses `Object_Universe(seed, depth)` which creates the full
galaxy in one call via containment tree: Universe → Region → System. Each system
is populated by `Object/System:Init` script and linked to neighbors by wormholes.

**Tuning knobs:**

| Parameter | Where | Effect |
|-----------|-------|--------|
| `depth=1` | `ltheory-unitest.lts:176` | ~36 systems (demo scale) |
| `depth=2` | Same line | ~216 systems (full galaxy) |
| Region children | `Region.cpp:96` | Raise boost from 5.0f → 8.0f for more systems per region |
| Wormhole density | `Region.cpp:165` | Raise kConnectivity from 2 → 3-4 for more links |

### 3.2 NPC Ships with AI (`System.lts` modifications)

**File:** `resource/script/Object/System.lts` (discussion in `docs/npc-ai-integration.md`)

Current implementation creates static pirates/patrols via `Task_Destroy`. Change to
use active tasks for visible behaviors:

| Task | Behavior | Visual Result |
|------|----------|---------------|
| `Task_Mine(zone)` | Fires weapons at asteroids, extracts resources | Mining ships orbiting rocks, firing beams/lasers |
| `Task_Transport(source, dest, item)` | Ships dock → load → travel → unload | Cargo haulers flying between stations on rails |
| `Task_Pirate(zone)` | Attacks cargo-carrying ships in zone | Pirate ships chasing/engaging traders |
| `Task_Dock(target)` | Ship flies to and docks at station | Ships entering station docking bays |
| `Task_Patrol(zone)` | Ship patrols zone boundaries | Patrol ships moving in patterns around zones |

**Required changes:**

1. Add tracking arrays: `var stations List`, `var spawnZones List` (~line 7)
2. Append to lists when creating stations/zones (~lines 150, 169)
3. Replace static pirate section with active tasks (~lines 171-190)

See `docs/npc-ai-integration.md` for concrete examples and visual impact priority.

### 3.3 Warp Rails (`WarpNode:BuildRail`)

**File:** `resource/script/Object/System.lts` (already implemented, lines 192-198)

Current implementation creates warp rails between POIs using `WarpNode:BuildRail`.
This connects key points (planets, stations, zones) with navigable paths for ships.

**Enhancement discussion:** Add visual effects to warp rails via custom shaders:

```lts
# Conceptual enhancement (requires C++ support):
var rail (Object_WarpRail)
rail.SetVisualEffect "warp_rail.jsl"  # Glowing path shader
```

### 3.4 Atmospheric Effects (`Object_System` enhancements)

**File:** `resource/script/Object/System.lts` (discussion below)

Current implementation creates dust/nebula via engine-side code in `Object_System`.
Enhance with per-system variation for distinct atmospheres:

#### Dust Clouds

```lts
# Conceptual enhancement:
var dustLevel (rng.Float 0.5 2.0)   # Per system variation

for i 0 (dustLevel * 10) {
  var cloud (Object_DustCloud rng.Seed)
  cloud.SetRadius (rng.Float 200 500)
  cloud.SetDensity (rng.Float 0.3 0.8)
  cloud.SetColor (Vec3 rng.Float 0.8 1.0 rng.Float 0.6 0.9)
}
```

#### Nebula Backdrop

```lts
# Conceptual enhancement:
var nebulaIntensity (rng.Float 0.3 1.5)

var nebula (Object_Nebula rng.Seed)
nebula.SetIntensity nebulaIntensity
nebula.SetColorPalette (rng.Choose "blue" "purple" "red")
```

**Visual impact:** High — volumetric clouds with color variation create distinct atmosphere per system.

---

## Part 4: Tuning for Performance + Visual Impact

### Rendering Optimization

| Technique | Where | Effect |
|-----------|-------|--------|
| LOD System | Distance-based geometry simplification | Reduce triangles beyond 1000 units |
| Frustum Culling | Only render visible objects per frame | Eliminate off-screen draw calls |
| Batch Updates | Group NPC AI updates every N frames | Reduce per-frame task overhead |

### Object Count Targets (Per System)

| Element | Target Count | Purpose |
|---------|-------------|---------|
| Planets | 2-3 total | Test shader variety + atmospheric effects |
| Stations | 1-3 total | Test SDF rendering + docking mechanics |
| NPC Ships | 20-50 total | Test AI pathfinding + physics simulation |
| Asteroids | 1000-5000 total | Test massive object counts + collision detection |
| Dust Clouds | 5-10 total | Test atmospheric effects + transparency rendering |

### Visual Effect Priorities

1. **Wormholes** — Swirling shader with particle trails (high visibility)
2. **Warp Rails** — Glowing paths connecting key points (navigation aid)
3. **Dust/Nebula** — Volumetric clouds with color variation (atmosphere)
4. **Ship Effects** — Engine trails, explosions, weapon fire (action feedback)

---

## Part 5: Testing Checklist

### Rendering Stress Test

- [ ] All 4 systems visible simultaneously via wormhole network
- [ ] 100+ NPC ships rendering without frame drops
- [ ] Atmospheric effects active across all systems
- [ ] Shader variety demonstrated (planets, stations, asteroids)

### AI/Physics Stress Test

- [ ] Ships following warp rail routes correctly via `Task_Transport`
- [ ] Mining activity visible: ships orbiting asteroids, firing weapons via `Task_Mine`
- [ ] Combat encounters happening between NPCs via `Task_Pirate`
- [ ] Collision detection working with dense asteroid fields
- [ ] Pathfinding around obstacles functioning

### Procedural Generation Test

- [ ] Each system has unique layout via seed variation
- [ ] Stations/asteroids generated with SDF variety
- [ ] Atmospheric effects differ per system (dust/nebula intensity)
- [ ] NPC ship types distributed appropriately by zone type

---

## Part 6: Next Steps (Concrete Implementation Order)

### Phase 1: Foundation (Script-only, immediate visual payoff)

1. **Add tracking arrays** — `stations` and `spawnZones` lists in `System.lts:Init()`
2. **Implement mining ships** — use `Task_Mine` for visible resource extraction
3. **Implement trade routes** — use `Task_Transport` between stations on rails
4. **Replace static pirates** — use `Task_Pirate` for dynamic hunting behavior

### Phase 2: Enhancement (Atmospheric effects, visual polish)

5. **Add station patrols** — use `Task_Patrol` around each station zone
6. **Enhance dust clouds** — per-system variation in density/color
7. **Enhance nebula backdrops** — distinct color palettes per system type
8. **Test performance** — monitor frame rates with 100+ NPC ships active

### Phase 3: Advanced (Complex AI, economic loops)

9. **Implement complex chains** — mining → transport → sell loops via chained tasks
10. **Add wormhole visual effects** — custom shaders for swirling portal appearance
11. **Scale to full galaxy** — test with `depth=2` (~216 systems) for stress testing

All changes are script-only; no C++ modifications required beyond existing task implementations and atmospheric effect support.

---

## Cross-References

| Document | Purpose |
|----------|---------|
| `AGENTS.md` | Master project reference, build system, third-party libraries |
| `docs/npc-ai-integration.md` | Detailed NPC AI wiring discussion for System.lts |
| `docs/universe-population.md` | Universe generation tuning, region/system knobs |
| `docs/PROCEDURAL-GENERATION-GUIDE.md` | SDF-based asset generation (asteroids, stations, ships) |
| `docs/ltsl-docs.md` | LTSL scripting language reference |
