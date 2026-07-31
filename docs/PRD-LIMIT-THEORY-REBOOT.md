# LIMIT THEORY REBOOT — Product Requirements Document (PRD)

**Project:** Limit Theory Reboot  
**Version:** 1.0  
**Date:** 2026-07-30  
**Status:** PLANNING → DEVELOPMENT  
**Owner:** darkoned12000  
**Purpose:** Transform Josh Parnell's 2012-2015 prototype into a complete,modern procedural space game

---

## Table of Contents

1. [Executive Vision](#1-executive-vision)
2. [Product Goals & Success Metrics](#2-product-goals--success-metrics)
3. [Target Audience](#3-target-audience)
4. [Core Pillars](#4-core-pillars)
5. [Feature Roadmap (Phases 1-6)](#5-feature-roadmap-phases-1-6)
6. [Technical Requirements](#6-technical-requirements)
7. [Content Pipeline](#7-content-pipeline)
8. [User Experience (UX) Requirements](#8-user-experience-ux-requirements)
9. [Performance Targets](#9-performance-targets)
10. [Quality Assurance](#10-quality-assurance)
11. [Community & Open-Source Strategy](#11-community--open-source-strategy)
12. [Risks & Mitigation](#12-risks--mitigation)
13. [Timeline & Milestones](#13-timeline--milestones)
14. [Success Criteria](#14-success-criteria)

---

## 1. Executive Vision

### 1.1 The Dream

**"A limitless procedural universe where every star, planet, ship, and story is generated from a seed — and every player's journey is unique."**

Limit Theory Reboot is the continuation of Josh Parnell's Kickstarter vision: a **single-player, open-ended space simulation** where you start with one ship and build an empire through trade, combat, mining, exploration, and strategy. The game world is **100% procedural**, generated at runtime from mathematical algorithms, creating infinite variety without pre-made assets.

### 1.2 Why This Matters (2026)

**Modern Context:**
- **No Man's Sky** proved procedural universes work (18 quintillion planets)
- **Starfield** showed demand for deep space RPGs
- **Elite Dangerous** validated realistic flight + economy simulation
- **X4: Foundations** demonstrated complex empire-building

**Our Unique Angle:**
- **Smallest game file in the genre** — ~500MB (vs. 150GB for Starfield)
- **Pure algorithmic art** — Every asset generated from seed, no art team needed
- **Open-source engine** — Community can mod/extend everything
- **Learning platform** — Teach procedural generation, game AI, C++/GLSL

### 1.3 Core Philosophy

1. **Procedural Everything** — Ships, stations, planets, factions, economy, missions
2. **Player-Driven Narrative** — No scripted story, only emergent gameplay
3. **Sandbox Freedom** — Trade, fight, explore, build, or do nothing
4. **Technical Excellence** — Modern C++17, OpenGL 4.6, 60 FPS, zero crashes
5. **Educational Value** — Fully documented, open-source, teach-by-example

---

## 2. Product Goals & Success Metrics

### 2.1 Primary Goals (What Success Looks Like)

**Goal 1: Complete Working Game (12 months)**
- 50-system procedural universe
- 20 hours of core gameplay loop (trade → upgrade → explore)
- 10 mission types (delivery, combat, mining, escort, etc.)
- Functional economy (supply/demand, market prices, trading)
- Ship customization (weapons, engines, cargo, shields)

**Goal 2: Modern Visual Quality (6 months)**
- PBR materials (realistic lighting)
- HDR + bloom (glowing stars, bright engines)
- Volumetric nebulae (3D fog)
- Planet biomes (5 types: desert, ice, lava, ocean, forest)
- 60 FPS with 30K asteroids, 100 ships, HDR post-processing

**Goal 3: Developer-Friendly (9 months)**
- Comprehensive documentation (10,000+ words)
- Live-edit dev tools (F2 panel, shader hot-reload)
- Modding API (LTSL scripting, custom ships/stations)
- Video tutorials (YouTube series)

**Goal 4: Community Engagement (Ongoing)**
- GitHub open-source release
- 100+ stars on GitHub
- 10+ community contributors
- Active Discord server (50+ members)

### 2.2 Key Performance Indicators (KPIs)

| Metric | Target (6 months) | Target (12 months) |
|--------|-------------------|-------------------|
| **Playable Content** | 10 hours | 20+ hours |
| **Systems Generated** | 20 | 50-100 |
| **Unique Ships** | 50 | 500+ (procedural) |
| **Mission Types** | 5 | 10-15 |
| **FPS (30K asteroids)** | 45 FPS | 60 FPS |
| **Build Time** | 30s | <15s |
| **GitHub Stars** | 50 | 100+ |
| **Community PRs** | 5 | 20+ |
| **Bug Count** | <50 | <20 |

---

## 3. Target Audience

### 3.1 Primary Audience

**Profile: "The Indie Space Sim Fan"**
- Age: 25-45
- Loves: No Man's Sky, Elite Dangerous, X4, Kerbal Space Program
- Seeks: Deep systems, emergent gameplay, infinite replayability
- Tolerates: Rough edges if core systems are solid
- Budget: Free (open-source) or $5-15 if standalone release

### 3.2 Secondary Audience

**Profile: "The Procedural Generation Student"**
- Age: 18-35
- Loves: Game development, algorithms, graphics programming
- Seeks: Learning resources, well-documented code, case studies
- Tolerates: Incomplete features if educational value is high
- Budget: Free (learning tool)

### 3.3 Tertiary Audience

**Profile: "The Modder/Contributor"**
- Age: 20-50
- Loves: Open-source projects, community collaboration, extending games
- Seeks: Clean codebase, modding API, active maintainers
- Tolerates: Breaking changes if clearly communicated
- Budget: Free (contribution-driven)

---

## 4. Core Pillars

### Pillar 1: **INFINITE UNIVERSE**

**What:** 50-100 procedural star systems, each with 2-5 planets, stations, asteroid belts, warp gates.

**Why:** Replayability. Every seed generates a unique universe.

**How:**
- `Object_Galaxy` container (50 systems)
- `SystemPopulate:MultiPlanet` (2-5 planets per system)
- `SystemPopulate:SpawnStations` (1-3 stations per system)
- `WarpGate` graph (travel between systems)
- Seeded economy (prices, goods, faction ownership)

**Success Metric:** Player can explore 50 systems without encountering identical planets/stations.

---

### Pillar 2: **EMERGENT ECONOMY**

**What:** Dynamic supply/demand simulation. Prices change based on trade, production, and consumption.

**Why:** Creates trading opportunities, risk/reward, player-driven market manipulation.

**How:**
- `Market` system per station (buy/sell goods)
- `Production` chains (ore → metal → ships)
- `Consumption` (stations consume food, fuel, ammo)
- `Price Algorithm` (low supply = high price)
- `Trade Routes` (AI traders move goods between stations)

**Example Flow:**
1. Station A produces food, consumes metal
2. Station B produces metal, consumes food
3. Player buys food at A (low price), sells at B (high price)
4. AI traders copy player's route → prices equalize
5. Player must find new opportunity

**Success Metric:** Player can earn 1M credits via pure trading (no combat) in 2-3 hours.

---

### Pillar 3: **TACTICAL COMBAT**

**What:** Newtonian physics dogfighting (6DOF: pitch, yaw, roll, thrust, strafe, brake).

**Why:** Skill-based combat differentiates from auto-targeting MMOs.

**How:**
- `ComponentMotion` (velocity, acceleration, angular velocity)
- `Weapon` types (projectile, beam, missile, turret)
- `Shield` system (absorbs damage, recharges)
- `Armor` system (permanent HP, requires repair)
- `AI Combat` (lead targeting, evasive maneuvers, formation flying)

**Success Metric:** Player can defeat 3 AI fighters via skill (not just stats) in <2 minutes.

---

### Pillar 4: **SHIP PROGRESSION**

**What:** Start with tiny scout ship, upgrade to cargo hauler, combat frigate, or mining barge.

**Why:** Sense of progression, goal-driven gameplay.

**How:**
- **Credits** (currency from trade, combat, missions)
- **Ship Types** (scout, fighter, cargo, miner, capital)
- **Upgrades** (weapons, engines, shields, cargo capacity)
- **Modular Design** (swap weapons, install turrets, upgrade thrusters)

**Progression Curve:**
- **Hour 0-2:** Scout ship (10K credits, 2 weapon slots, 100 cargo)
- **Hour 3-5:** Fighter (50K credits, 4 weapon slots, shields)
- **Hour 6-10:** Cargo hauler (200K credits, 1000 cargo, slow but profitable)
- **Hour 11-15:** Mining barge (500K credits, mining laser, 2000 ore capacity)
- **Hour 16-20:** Capital ship (2M credits, 8 weapon slots, docking bay for fighters)

**Success Metric:** Player feels "I can't go back to the starter ship" by hour 5.

---

### Pillar 5: **PROCEDURAL CONTENT**

**What:** All ships, stations, asteroids, planets generated from seed (no pre-made 3D assets).

**Why:** Infinite variety, tiny game size, educational showcase.

**How:**
- **Ships:** PlateMesh (plate-based hulls)
- **Asteroids:** SDF fractal noise → marching cubes
- **Stations:** SDF boolean ops (shell, spokes, docking ports)
- **Planets:** GPU shader fractals (height, color, clouds)
- **Factions:** Seeded names, colors, relationships

**Success Metric:** Player can generate 1000 unique ships in <1 minute (no repeats).

---

## 5. Feature Roadmap (Phases 1-6)

### Phase 1: **CORE SYSTEMS (Months 1-3)**

**Focus:** Performance + universe generation + basic gameplay.

| Feature | Owner | Priority | Status |
|---------|-------|----------|--------|
| GPU instancing (30K asteroids @ 60 FPS) | You | P0 | Planned |
| Multi-system universe (50 systems) | You | P0 | Planned |
| Station spawning + markets | You | P0 | Planned |
| Basic trading (buy/sell) | You | P1 | Planned |
| Ship upgrades (weapons, engines) | You | P1 | Planned |
| Save/load game state | You | P1 | Planned |

**Deliverable:** Playable demo with 20 systems, trading, combat, 60 FPS.

---

### Phase 2: **VISUAL UPGRADE (Months 4-6)**

**Focus:** AAA-quality graphics.

| Feature | Owner | Priority | Status |
|---------|-------|----------|--------|
| PBR shader (albedo, normal, roughness) | You | P0 | Planned |
| HDR + bloom | You | P0 | Planned |
| Volumetric nebula (compute shader) | You | P1 | Planned |
| Planet biomes (5 types) | You | P1 | Planned |
| Shadow mapping (directional light) | You | P2 | Stretch |
| Normal maps for planets | You | P2 | Stretch |

**Deliverable:** Screenshots that rival modern indie space sims.

---

### Phase 3: **ECONOMY & MISSIONS (Months 7-9)**

**Focus:** Emergent gameplay loops.

| Feature | Owner | Priority | Status |
|---------|-------|----------|--------|
| Dynamic economy (supply/demand) | You | P0 | Planned |
| Production chains (ore → metal → ships) | You | P0 | Planned |
| 10 mission types (delivery, combat, escort, mine, scan) | You | P0 | Planned |
| Faction reputation system | You | P1 | Planned |
| AI trade ships (move goods between stations) | You | P1 | Planned |
| Contracts/bulletin board UI | You | P1 | Planned |

**Deliverable:** Player can earn money 3 ways (trade, missions, combat).

---

### Phase 4: **EMPIRE BUILDING (Months 10-12)**

**Focus:** Long-term goals.

| Feature | Owner | Priority | Status |
|---------|-------|----------|--------|
| Player-owned stations (buy, upgrade, manage) | You | P0 | Planned |
| Fleet management (hire AI pilots, form squads) | You | P0 | Planned |
| Capital ships (carriers, battleships) | You | P1 | Planned |
| Research tree (unlock tech, ship blueprints) | You | P1 | Planned |
| Territory control (claim systems, collect tax) | You | P2 | Stretch |

**Deliverable:** Player can build a trading empire (10+ owned ships/stations).

---

### Phase 5: **POLISH & CONTENT (Months 13-15)**

**Focus:** Juice, audio, variety.

| Feature | Owner | Priority | Status |
|---------|-------|----------|--------|
| Background music (exploration, combat) | You | P0 | Planned |
| 3D positional audio (engines, weapons) | You | P0 | Planned |
| 50 unique ship hulls (procedural variants) | You | P1 | Planned |
| 20 weapon types (lasers, missiles, railguns) | You | P1 | Planned |
| Particle effects (explosions, warp trails) | You | P1 | Planned |
| Story scenarios (optional narrative arcs) | Community | P2 | Future |

**Deliverable:** Game feels "complete" (audio, VFX, variety).

---

### Phase 6: **COMMUNITY & MODDING (Months 16-18)**

**Focus:** Open-source release, modding tools.

| Feature | Owner | Priority | Status |
|---------|-------|----------|--------|
| GitHub public release (GPL-3.0) | You | P0 | Planned |
| Modding guide (LTSL scripting tutorial) | You | P0 | Planned |
| Ship editor (PlateMesh UI tool) | Community | P1 | Future |
| Mission scripting API (custom quests) | Community | P1 | Future |
| Steam Workshop integration (if standalone) | You | P2 | Future |

**Deliverable:** Active community, 20+ mods released.

---

## 6. Technical Requirements

### 6.1 Minimum System Requirements

| Component | Minimum | Recommended |
|-----------|---------|-------------|
| **OS** | Windows 10 64-bit, Linux (Ubuntu 20.04+) | Windows 11, Arch Linux |
| **CPU** | Intel i5-6600 / AMD Ryzen 3 1200 | Intel i7-9700 / AMD Ryzen 7 3700X |
| **GPU** | Nvidia GTX 960 / AMD RX 560 (OpenGL 4.6) | Nvidia RTX 3060 / AMD RX 6700 |
| **RAM** | 8 GB | 16 GB |
| **Storage** | 1 GB | 2 GB |
| **Resolution** | 1280x720 | 1920x1080 or 2560x1440 |

**Performance Target:** 60 FPS @ 1080p on recommended hardware with 30K asteroids, 100 ships, HDR/bloom enabled.

---

### 6.2 Build System

| Tool | Version | Purpose |
|------|---------|---------|
| **CMake** | 3.20+ | Cross-platform build |
| **Compiler** | GCC 13+, Clang 16+, MSVC 2022+ | C++17 support |
| **Python** | 3.8+ | `configure.py` helper script |
| **Git** | 2.30+ | Version control |
| **Git LFS** | 3.0+ | (Future) Large resource files |

**Build Time Target:** <30s incremental, <5min full rebuild (parallel compilation).

---

### 6.3 Dependencies

| Library | Version | Purpose |
|---------|---------|---------|
| **SFML** | 3.1.0+ | Window, input, image I/O, audio |
| **GLEW** | 2.3.1+ | OpenGL extension loading |
| **FreeType** | 2.12+ | Font rasterization |
| **OpenAL** | 1.21+ | SFML audio backend dependency |
| **Vorbis/FLAC/Ogg** | Latest | Audio codec support |

**Dependency Management:** System packages (Linux), vcpkg (Windows future).

---

## 7. Content Pipeline

### 7.1 Procedural Assets (Runtime Generation)

**All content generated at runtime from seeds:**

| Asset Type | Generator | Output | Performance |
|------------|-----------|--------|-------------|
| **Ships** | PlateMesh (LTSL) | 1K-5K tris | 2-10ms |
| **Asteroids** | SDF + marching cubes (C++) | 500-2K tris | 5-20ms |
| **Stations** | SDF boolean ops (C++) | 2K-10K tris | 10-50ms |
| **Planet Surfaces** | Shader fractals (GPU) | 4K cubemap | 20-100ms |
| **Faction Names** | Grammar rules (LTSL) | String | <1ms |

**Cache Strategy:** First generation → cache by seed. Subsequent spawns = 0ms (cache hit).

---

### 7.2 Static Assets (Artist-Created, Optional)

**Hybrid approach (future):**

| Asset Type | Format | Source | Use Case |
|------------|--------|--------|----------|
| **Cockpit Meshes** | .obj | Blender/Maya | Kitbashing (attach to procedural hulls) |
| **Decal Textures** | .png | Photoshop/GIMP | Faction logos, hull markings |
| **Detail Normal Maps** | .png | Substance Painter | Planet/asteroid surface detail |
| **UI Icons** | .svg | Inkscape | HUD elements, market items |

**Asset Budget (if added):**
- 50 cockpit variants × 50KB = 2.5MB
- 100 decal textures × 256KB = 25MB
- 20 normal maps × 2MB = 40MB
- **Total:** ~70MB (still tiny compared to AAA games)

---

## 8. User Experience (UX) Requirements

### 8.1 First-Time User Experience (FTUE)

**Goal:** Player understands core loop in <5 minutes.

**Tutorial Flow:**

1. **Launch Screen** (0:00)
   - "NEW GAME" → Select seed (random or custom)
   - "CONTINUE" → Load saved game
   - "OPTIONS" → Graphics, audio, controls

2. **Universe Generation** (0:10)
   - Loading screen shows seed, progress bar
   - "Generating 50 star systems..."
   - Estimated time: 10-30 seconds

3. **Tutorial Mission** (0:30)
   - Spawn in starter system near station
   - **Objective 1:** "Fly to waypoint (WASD, mouse look)"
   - **Objective 2:** "Dock at station (F key when close)"
   - **Objective 3:** "Buy 10 units of food from market"
   - **Objective 4:** "Sell food at next station for profit"
   - **Reward:** 5K credits, unlock "Free Roam" mode

4. **Free Roam** (5:00)
   - Tutorial complete, sandbox unlocked
   - Player decides: trade, fight, explore, or follow missions

**Success Metric:** 80% of players complete tutorial, 60% play >1 hour after tutorial.

---

### 8.2 HUD & UI Design

**HUD Elements:**

| Element | Position | Purpose |
|---------|----------|---------|
| **Radar** | Top-right | Show nearby ships, stations, warp gates (2D projection) |
| **Target Info** | Top-center | Selected object (name, distance, faction, threat level) |
| **Ship Status** | Bottom-left | Hull HP, shield %, energy %, cargo space |
| **Weapon Status** | Bottom-center | Active weapon, ammo/charge, firing mode |
| **Speed/Throttle** | Bottom-right | Current speed, throttle %, thruster indicators |
| **Minimap** | Top-left | Local system map (click to open galaxy map) |

**UI Panels:**

| Panel | Hotkey | Purpose |
|-------|--------|---------|
| **Market** | M | Buy/sell goods at station |
| **Shipyard** | Y | Buy/sell ships, upgrade weapons/engines |
| **Missions** | J | Accept contracts, track objectives |
| **Inventory** | I | View cargo, equipment, credits |
| **Map** | Tab | Galaxy map (warp between systems) |
| **DevPanel** | F2 | (Dev mode) Tweak parameters, regenerate universe |

---

### 8.3 Control Scheme

**Keyboard + Mouse (Default):**

| Action | Key/Mouse | Notes |
|--------|-----------|-------|
| **Thrust Forward** | W | Accelerate |
| **Thrust Backward** | S | Brake/reverse |
| **Strafe Left/Right** | A/D | Lateral movement |
| **Strafe Up/Down** | Space/Ctrl | Vertical movement |
| **Pitch/Yaw** | Mouse | Look around (free-look mode) |
| **Roll** | Q/E | Rotate ship on Z-axis |
| **Fire Weapon** | Left Click | Primary weapon |
| **Cycle Weapons** | Tab | Switch between equipped weapons |
| **Target Nearest** | T | Auto-target closest hostile |
| **Dock** | F | (Near station) Initiate docking |
| **Warp** | W (hold near gate) | Jump to linked system |
| **Throttle 0%** | Backspace | Full stop |
| **Throttle 50%/100%** | R / Shift+R | Quick speed presets |

**Gamepad Support (Future):**
- Left stick: Pitch/yaw
- Right stick: Roll/throttle
- Triggers: Fire/brake
- D-pad: Power management (shields/weapons/engines)

---

## 9. Performance Targets

### 9.1 Frame Rate (FPS)

| Scenario | Min FPS | Target FPS | Max FPS |
|----------|---------|------------|---------|
| **Empty space** (just stars) | 120 | 240 | Uncapped |
| **Single system** (1 planet, 1000 asteroids, 10 ships) | 60 | 90 | 120 |
| **Dense combat** (30K asteroids, 100 ships, effects) | 45 | 60 | 90 |
| **Station interior** (UI heavy, 3D preview) | 60 | 90 | 120 |

**Optimization Strategy:**
- GPU instancing (Priority 1) → 2-3x FPS boost
- LOD system (Priority 1) → Cull distant objects
- Frustum culling (already implemented)
- Occlusion culling (future, Priority 6)

---

### 9.2 Memory Usage

| Component | Budget (Min) | Budget (Rec) |
|-----------|--------------|--------------|
| **Engine + DLLs** | 50 MB | 80 MB |
| **GPU Textures** | 200 MB | 500 MB |
| **Meshes (cached)** | 100 MB | 300 MB |
| **Audio Buffers** | 20 MB | 50 MB |
| **Universe State** (50 systems) | 50 MB | 100 MB |
| **Total** | **420 MB** | **1030 MB** |

**Optimization:**
- Stream system data (unload distant systems)
- Compress textures (BC7 for albedo, BC5 for normals)
- Limit mesh cache size (1000 ships max, LRU eviction)

---

### 9.3 Loading Times

| Action | Target Time | Notes |
|--------|-------------|-------|
| **Launch to Main Menu** | <3s | Engine init, load UI |
| **New Game** (generate 50 systems) | <30s | Parallelized generation |
| **Load Saved Game** | <5s | Deserialize universe state |
| **Warp to New System** | <1s | Generate on-demand or cache |
| **Dock at Station** | <0.5s | Instant (UI transition) |

---

## 10. Quality Assurance

### 10.1 Testing Strategy

**Unit Tests:**
- `tests/TestString.cpp` — String operations
- `tests/TestVectorArray.cpp` — Container ops
- `tests/TestType.cpp` — Reflection system
- `tests/TestScriptCompile.cpp` — LTSL error reporting
- **Target:** 80% code coverage (LTE core)

**Integration Tests:**
- `tests/TestShaderAudit.cpp` — All 170 shaders compile
- `tests/TestTexture2D.cpp` — Texture load/save
- `tests/TestSound.cpp` — Audio playback (2D/3D)
- **Target:** All apps launch without crashes

**Manual QA Checklist:**
- [ ] Generate 50 systems in <30s
- [ ] Trade goods at 5 stations, earn 100K credits
- [ ] Destroy 10 enemy ships, no crashes
- [ ] Warp through 10 systems, no memory leaks
- [ ] Play for 2 hours continuous, no FPS drops
- [ ] Save/load game state, verify persistence

---

### 10.2 Performance Profiling

**Tools:**
- `gprof` (CPU profiling)
- `valgrind` (memory leaks)
- `perf` (Linux performance counters)
- `RenderDoc` (GPU profiling)

**Monthly Profiling Sessions:**
- Identify hot functions (>5% CPU time)
- Check for memory leaks (valgrind)
- Measure draw call count (RenderDoc)
- Verify VRAM usage (GPU memory viewer)

---

## 11. Community & Open-Source Strategy

### 11.1 GitHub Release

**Timeline:** Month 6 (after Phase 2 complete)

**License:** GPL-3.0-or-later (engine code) + Unlicense (Josh's original public domain work)

**Repository Structure:**
```
darkoned12000/ltheory-reboot
  ├─ src/            C++ engine code
  ├─ resource/       Scripts, shaders, sounds
  ├─ docs/           Documentation, guides
  ├─ tests/          Unit + integration tests
  ├─ CMakeLists.txt  Build system
  └─ README.md       Quick start, build instructions
```

**Documentation (Deliverables):**
- `README.md` — Build instructions, quick start
- `AGENTS.md` — AI contributor guide (architecture, conventions)
- `GRAPHICS-TECH.md` — Visual upgrade guide
- `AUDIO-SYSTEM-GUIDE.md` — Sound implementation
- `PROCEDURAL-GENERATION-GUIDE.md` — SDF/PlateMesh internals
- `docs/ltsl-docs.md` — LTSL scripting reference
- `docs/content-creation-guide.md` — How to make ships/stations

---

### 11.2 Community Engagement

**Discord Server:**
- Channels: #general, #development, #modding, #bug-reports, #showcase
- Voice: Weekly dev streams (Sunday 2pm PST)
- Roles: Contributor, Tester, Modder, Artist

**YouTube Channel:**
- "Limit Theory Reboot Devlog" series (10 episodes, bi-weekly)
- Episode 1: "What is Limit Theory?" (history, vision)
- Episode 2: "Building Your First Ship" (PlateMesh tutorial)
- Episode 3: "Trading for Profit" (economy guide)
- ... (8 more episodes)

**Reddit/Forums:**
- r/proceduralgeneration — Share techniques
- r/gamedev — Dev updates
- r/spacesimgames — Player announcements

---

### 11.3 Contribution Guidelines

**How to Contribute:**
1. Fork repository
2. Create feature branch (`feature/your-feature-name`)
3. Write code + tests
4. Run `python3 configure.py test` (all tests pass)
5. Submit PR with description
6. Maintainer reviews + merges

**Code Style:**
- `.clang-format` (2-space indent, K&R braces)
- Add Revamp Work header to new files (GPL-3.0)
- Comment non-obvious algorithms
- No `#include` in headers unless necessary

**Maintainer Responsibilities:**
- Review PRs within 7 days
- Provide constructive feedback
- Merge when tests pass + code quality OK
- Tag releases (v0.1, v0.2, ..., v1.0)

---

## 12. Risks & Mitigation

| Risk | Probability | Impact | Mitigation |
|------|-------------|--------|------------|
| **Scope Creep** | HIGH | HIGH | Stick to PRD phases, defer features to Phase 6+ |
| **Performance Regression** | MEDIUM | HIGH | Monthly profiling, automated FPS tests |
| **LTSL Syntax Fragility** | MEDIUM | MEDIUM | Unit tests for scripts, error reporting improvements |
| **Contributor Burnout** | MEDIUM | MEDIUM | Clear milestones, celebrate small wins |
| **Community Disinterest** | LOW | HIGH | Engage early (Discord, devlogs), showcase progress |
| **Hardware Incompatibility** | LOW | MEDIUM | Test on 5+ GPU models (Nvidia/AMD), fallback to lower settings |

---

## 13. Timeline & Milestones

### Overview (18-Month Plan)

```
Month 1-3:   Phase 1 (Core Systems)
Month 4-6:   Phase 2 (Visual Upgrade)
Month 7-9:   Phase 3 (Economy & Missions)
Month 10-12: Phase 4 (Empire Building)
Month 13-15: Phase 5 (Polish & Content)
Month 16-18: Phase 6 (Community & Modding)
```

### Detailed Milestones

**Milestone 1: "Playable Demo" (Month 3)**
- **Deliverable:** 20 systems, trading, combat, 60 FPS
- **Demo:** 30-minute gameplay video (trade route + dogfight)
- **Release:** Private alpha to 10 testers

**Milestone 2: "Visual Showcase" (Month 6)**
- **Deliverable:** PBR + HDR + bloom, planet biomes
- **Demo:** Screenshot comparison (before/after)
- **Release:** GitHub public repo (open-source)

**Milestone 3: "Economy Simulation" (Month 9)**
- **Deliverable:** Dynamic prices, 10 mission types
- **Demo:** 2-hour stream (earn 1M credits via trading)
- **Release:** Beta v0.5 (Steam page if standalone)

**Milestone 4: "Empire Mode" (Month 12)**
- **Deliverable:** Player-owned stations, fleet management
- **Demo:** "From Zero to Hero" 20-hour playthrough
- **Release:** v1.0 Release Candidate

**Milestone 5: "Content Complete" (Month 15)**
- **Deliverable:** Audio, VFX, 50 ship variants
- **Demo:** "Full Gameplay Loop" video (0→empire)
- **Release:** v1.0 Gold Master

**Milestone 6: "Community Launch" (Month 18)**
- **Deliverable:** Modding guide, ship editor tool
- **Demo:** Community mod showcase (10+ mods)
- **Release:** Steam launch (if standalone), modding API docs

---

## 14. Success Criteria

### What "Success" Looks Like (12 Months)

**Technical:**
- ✅ 148+ unit tests passing, 0 crashes in 10-hour playthrough
- ✅ 60 FPS @ 1080p with 30K asteroids, 100 ships, HDR/bloom
- ✅ 50-system procedural universe, fully seeded

**Gameplay:**
- ✅ 20+ hours of content (trade, combat, exploration, missions)
- ✅ Player progression: scout → cargo → capital ship (5-10 hour curve)
- ✅ Emergent stories ("I became a pirate lord by accident")

**Community:**
- ✅ 100+ GitHub stars
- ✅ 20+ community PRs merged
- ✅ 50+ Discord members, active daily
- ✅ 10+ YouTube videos (devlogs + tutorials)

**Educational:**
- ✅ 10,000+ words of documentation
- ✅ 5+ case study articles (SDF generation, PBR shaders, LTSL)
- ✅ Referenced in 3+ academic papers / blog posts

---

### What "Exceptional Success" Looks Like (18 Months)

- 🏆 **1,000+ GitHub stars** (top 1% of game engine repos)
- 🏆 **100+ community mods** (Steam Workshop if standalone)
- 🏆 **$50K+ revenue** (if standalone on Steam/Itch.io)
- 🏆 **Featured on /r/gamedev** (front page post)
- 🏆 **Academic citation** (procedural generation research paper)
- 🏆 **Port to VR** (Oculus/SteamVR support, community-driven)

---

## Appendix A: Inspiration & References

**Games:**
- **No Man's Sky** — Procedural universe scale
- **Elite Dangerous** — Realistic flight + economy
- **X4: Foundations** — Empire building, fleet management
- **Kerbal Space Program** — Physics-based flight, moddability
- **EVE Online** — Player-driven economy, emergent warfare

**Technical:**
- **Inigo Quilez** — SDF techniques (iquilezles.org)
- **Sebastian Lague** — Procedural planets (YouTube series)
- **GPU Gems** — Marching cubes, PBR shaders
- **Physically Based Rendering (Pharr et al.)** — PBR theory

---

## Appendix B: FAQ for AI Video Question

**Q: "Is there AI that can watch and interpret videos like Josh's devlogs?"**

**A: Partial YES** (as of 2026)

**Video-Understanding AI (Available Now):**
- **GPT-4 Vision / Claude Sonnet (Image Mode)** — Can analyze screenshots/images, not full video
- **Google Gemini Pro Vision** — Can process video frames, extract text, describe scenes
- **Microsoft Azure Video Indexer** — Extracts metadata (faces, objects, text) from video
- **OpenAI Whisper** — Transcribes audio (what Josh says in devlogs)

**What They Can Do:**
- ✅ Transcribe audio → Get full text of Josh's explanations
- ✅ Extract keyframes → Analyze UI screenshots, code snippets shown
- ✅ Detect objects → Identify ships, planets, UI elements
- ✅ Read text → OCR any on-screen code/docs

**What They CAN'T Do (Yet):**
- ❌ Understand complex 3D spatial relationships (how ship generator works)
- ❌ Infer algorithmic intent (why Josh chose that SDF operation)
- ❌ Generate working code from visual demos alone

**Recommended Workflow:**
1. **Transcribe audio** (Whisper) → Get Josh's verbal explanations
2. **Extract keyframes** (every 5 seconds) → Get visual examples
3. **Analyze frames** (GPT-4V) → Describe UI, code, shapes
4. **Combine** → Text transcript + frame descriptions → Full understanding

**Practical Example:**
```
Input: Josh's devlog video "Procedural Ship Generation"
Output:
  - Transcript: "...so I'm using signed distance functions to carve out the hull..."
  - Frame 42: [Shows code snippet: SDF_Subtract(box, sphere)]
  - Frame 89: [Shows 3D preview of ship with carved hull]
  → AI infers: Josh uses boolean subtraction to create hollow interiors
```

**Your Use Case:**
- Upload Josh's devlog videos to YouTube → Auto-generate transcripts
- Extract keyframes with `ffmpeg` (every 5s)
- Feed frames + transcript to Claude/GPT-4V
- Ask: "What technique is Josh demonstrating here?"
- Result: 70-80% accurate understanding (enough to guide implementation)

---

## Final Note

**This PRD is a living document.** As you build Limit Theory Reboot, revisit these sections, update priorities, and celebrate milestones. The journey from prototype to complete game is long, but with clear goals and systematic execution, it's absolutely achievable.

**Your dream is valid. This PRD makes it real.**

Now let's build something extraordinary. 🚀✨

---

**END OF PRD v1.0**

**Next Steps:**
1. Review this PRD (highlight uncertainties)
2. Pick Phase 1 first task (GPU instancing or multi-system universe)
3. Create GitHub project board (Kanban: TODO, IN PROGRESS, DONE)
4. Start coding! 💻
