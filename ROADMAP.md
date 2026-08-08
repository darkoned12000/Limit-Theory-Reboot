// Copyright (C) 2025  darkoned12000
// SPDX-License-Identifier: GPL-3.0-or-later
// Part of the ltheory-old-test modernization effort (Revamp Work).
// See NOTICE and LICENSE.GPL. Original engine (c) Josh Parnell, public domain.

# ROADMAP.md — Engine Improvement Plan (single source of truth)

Forward-looking work plan for `ltheory-old-test`. This file **synthesizes**
the per-topic planning docs (listed in §8) into one prioritized queue and
tracks **actual engine state** (verified against `src/`), so it is the
authority on "what's next". For the current-state reference and the
completed-work log, see `AGENTS.md`.

> **Rule of thumb:** when a doc below contradicts this file, ROADMAP.md wins
> if it was updated after the doc. Keep this file current; the source docs
> are reference material.

---

## 1. Legend

| Priority | Meaning |
|----------|---------|
| **P0** | Blocks the core loop / next milestone |
| **P1** | High value, near-term |
| **P2** | Stretch / depends on earlier work |
| **Deferred** | Investigated, deliberately not planned (cost > value) |

Status: `todo` · `in-progress` · `done` · `blocked`. Effort is a rough
estimate. Each item lists its **source doc** (§8) so the design detail lives
there, not here.

---

## 2. Recommended next (the immediate queue)

Ordered by impact ÷ effort, respecting dependencies. **Nothing here requires
a big rewrite** — the engine core is healthy (§9 of AGENTS.md).

### 2.1 Save/Load game state — **P0, first** (`todo`)
- **Effort:** 1–2 weeks. **Sources:** `ENGINE-STABILITY-AND-MODDING.md` Part 1
  (Phase 1, CRITICAL); `SAVE-LOAD-AND-INVENTORY.md` Part 1/3; PRD Phase 1 P1.
- **Why first:** the only item flagged CRITICAL by two independent roadmaps,
  and the reflection/`Serializer` infrastructure already exists — this is
  mostly wiring `SaveGame.cpp` + 2 LTSL bindings + menu buttons.
- **Scope:** serialize player, universe, economy → disk; `SaveGame_Create` /
  `SaveGame_Load` LTSL bindings; Save/Load entries in the main menu;
  round-trip test (save → quit → load → verify state).

### 2.2 Crash logging polish — **P0, quick** (`in-progress`)
- **Effort:** ~2 days. **Source:** `ENGINE-STABILITY-AND-MODDING.md` Part 1.
- Current state: a `signal(SIGSEGV, SegHandler)` trap exists in
  `src/liblt/LTE/OS.cpp:66`. Extend it to write a stack trace + crash log to
  disk and surface a user-friendly message. Low risk, high debugging value.

### 2.3 Data-driven JSON layer — **P1** (`todo`)
- **Effort:** 3–4 weeks. **Source:** `ENGINE-STABILITY-AND-MODDING.md` Part 3
  + Part 7 Phase 2.
- **Why now:** unlocks ship/weapon/planet tuning without recompiles, and is
  the foundation for the modding system (§3.3). Needs `nlohmann/json`
  vendored into `include/` (not currently present).
- **Scope:** `ShipDatabase` / `WeaponDatabase` / `PlanetDatabase` loaders +
  LTSL bindings; `ships.json` / `weapons.json` / `planet_biomes.json`.

### 2.4 Asset hot-reload — **P1** (`todo`)
- **Effort:** 1–2 weeks. **Source:** `ENGINE-STABILITY-AND-MODDING.md`
  Part 7 Phase 2 (week 4).
- **Scope:** `AssetWatcher.cpp` (file watcher) → reload shaders / JSON /
  textures on change without restart. Pairs with the LSP + live `.lts`
  recompile story (§10.3 of AGENTS.md, "LTSL Hot-Reloading").

---

## 3. Work tracks

### 3.1 C++ engine core

| Item | Priority | Effort | Status | Source |
|------|----------|--------|--------|--------|
| Save/Load game state | P0 | 1–2 wk | todo | Stability P1, Save P1, PRD P1 |
| Crash logging + stack traces | P0 | 2 d | in-progress | Stability P1 |
| Asset hot-reload watcher | P1 | 1–2 wk | todo | Stability P2 |
| Memory profiling (alloc tracking / valgrind) | P1 | 2 d | todo | Stability P1 |
| InputManager + key rebinding (JSON-driven) | P1 | 2–3 wk | todo | Stability P3 |
| Background multithreading (asset load, physics) | P2 | 3–4 wk | todo | PRD §9.3 — profile first |
| LTSL hot-reloading (runtime `.lts` recompile) | P1 | — | todo | AGENTS §10.3 |
| UBO batching (5 MVP matrices) | P2 | — | todo | AGENTS §10.3 — only if profiling shows a bottleneck |
| Delete dead EasyGL wrappers (`GL_Begin`/`GL_End`/…) | P2 | 1 d | todo | AGENTS §10.3 |

### 3.2 Rendering & graphics

> **Done already (verified in `src/`):** GPU instancing infrastructure
> (Phase 2, `InstancedDraw.h`, `Mesh::DrawInstanced`), GPU-instanced particle
> rendering (Phase 2.4), Hi-Z occlusion culling + LOD fade-in
> (`e7f24ea`). So PRD Phase 1 "GPU instancing 30K asteroids @60 FPS" is
> **already delivered**; the PRD table is stale.

| Item | Priority | Effort | Status | Source |
|------|----------|--------|--------|--------|
| Enable existing post FX (SSAO `filter_ao.jsl`, lensflare, vignette, motion blur) | P0 | ~1 wk | todo | GRAPHICS-ROADMAP "Quick Wins" |
| PBR (Cook-Torrance BRDF + material switching) | P0 | 2–3 wk | todo | PRD P2, GRAPHICS-TECH Pt5 |
| HDR + bloom | P0 | 3–5 d | todo | PRD P2, GRAPHICS-ROADMAP |
| Volumetric nebula (compute 3D noise + raymarch) | P1 | 1 wk | todo | PRD P2, GRAPHICS-ROADMAP |
| Planet biomes (biome enum + palettes) | P1 | 3 d | todo | PRD P2, GRAPHICS-ROADMAP, PROCEDURAL-GENERATION |
| Shadow mapping (directional / terminator) | P2 | ~2 wk | todo | PRD P2 (stretch), AGENTS §8 |
| Normal maps for planets | P2 | — | todo | PRD P2 (stretch) |
| Star-field parallax layers | P2 | — | todo | AGENTS §10.6 |
| HDR + bloom, directional lighting, scattering | P2 | — | todo | AGENTS §10.6 |

### 3.3 Modding & data-driven

| Item | Priority | Effort | Status | Source |
|------|----------|--------|--------|--------|
| JSON content databases (ship/weapon/planet) | P1 | 3–4 wk | todo | Stability P2 |
| ModManager (scan `mods/`, parse `mod.json`, load JSON/scripts/shaders) | P1 | 3–4 wk | todo | Stability P4 |
| Mod hooks (`onGameStart`, `onSectorGenerate`) | P1 | — | todo | Stability P4 |
| JSON save-file schemas + versioned migration | P1 | — | todo | SAVE-LOAD Pt2/3 |
| Mod manager UI | P2 | — | todo | Stability P4 |

### 3.4 Universe generation & gameplay (Pass A→C)

| Item | Priority | Effort | Status | Source |
|------|----------|--------|--------|--------|
| **Pass A:** config-driven counts + stations + multi-planet | P0 | mostly script | todo | AGENTS §8.3 |
| **Pass B:** biomes + sun/nebula/fog/dust knobs threaded through `Object_System` | P1 | C++/GLSL | todo | AGENTS §8.3 |
| **Pass C:** live DevTool tweaking (depends on Pass A) | P2 | — | todo | AGENTS §8.3 |
| Multi-system universe (50 systems, warp-gate graph) | P1 | — | todo | PRD P1 |
| Seeded economy + station markets (basic trading) | P1 | — | todo | PRD P1 |
| Dynamic economy + production chains | P2 | — | todo | PRD P3 |
| 10 mission types | P2 | — | todo | PRD P3 |
| Faction reputation | P2 | — | todo | PRD P3 |
| Ship upgrades | P1 | — | todo | PRD P1 |
| Player-owned stations / fleet / capital ships / research | P2 | — | todo | PRD P4 |

### 3.5 LTSL language (compiler/interpreter)

> See `LTSL-ARCHITECTURE-AND-IMPROVEMENTS.md` Parts 4–5 for full designs.

| Item | Priority | Effort | Status | Source |
|------|----------|--------|--------|--------|
| List methods `.Filter()`/`.Map()`/`.Reduce()` | P0 | 3 d | todo | LTSL Pt4 |
| `foreach` loop sugar | P0 | 2 d | todo | LTSL Pt4 |
| String interpolation `$"{expr}"` | P0 | 3 d | todo | LTSL Pt4 |
| Lambda syntax `=> { }` | P0 | 2 d | todo | LTSL Pt4 |
| Range syntax `for i in 0..10 step 5` | P1 | 2 d | todo | LTSL Pt4 |
| Array indexing `list[5]` | P1 | 2 d | todo | LTSL Pt4 |
| Pattern matching / destructuring / optional chaining | P2 | 1–2 wk | todo | LTSL Pt5 |
| LTSL-level `try/catch` | P2 | 1 wk | todo | LTSL Pt5 |
| Bytecode VM | Deferred | 2–3 mo | todo | LTSL Pt5 — only if profiling shows scripting as a bottleneck |

### 3.5a LTSL DX & tooling — making the language easy to work on

> The living feedback + prioritized queue lives in `ltsl-hardening.md` (§5–9);
> this table is the ROADMAP-side view. The **big DX wins already landed**
> (2026-08-08): literal-probe silencing (spurious error flood), script-visible
> `Log`/`Log_Warn`/`Log_Error`, `#`-comment parse strip, and **function-body
> error propagation** — scripts that fail inside a function body now report
> their errors instead of silently swallowing them (exposed `SettingsPanel`,
> `DebugScene`, `ltheory-unitest`; see `ltsl-hardening.md` §6).

| Item | Priority | Effort | Status | Source |
|------|----------|--------|--------|--------|
| Runtime error channel (LTSL stack dump via `StackFrame_Print` + F3 overlay) | P1 | 2–3 d | todo | hardening P1-3 |
| Startup watchdog (hang trip + stack dump) | P1 | 1–2 d | todo | hardening P1-4 |
| Explicit-return strict mode | P2 | 2 d | todo | hardening P2-5 |
| `StringList_Create` single-line no-double-wrap | P2 | 1 d | todo | hardening P2-7 |
| Bind `String_Split` 2-arg | P2 | 1 d | todo | hardening P2-8 |
| Selftest app (extend `App/selftest.lts` assert set) | P2 | — | done | hardening §6 |
| API-DB refresh on C++ API change (gate: 0 added/0 removed) | — | 5 min | done | hardening §3.3 |

### 3.6 Content wiring & audio

> The cargo/mining/serialization **systems already exist** in the engine;
> these items wire them into the UI/gameplay. `AUDIO-SYSTEM-GUIDE.md` notes
> the audio engine is fully working but apps don't use it yet.

| Item | Priority | Effort | Status | Source |
|------|----------|--------|--------|--------|
| Inventory UI + hotkey | P1 | 3–4 d | todo | SAVE-LOAD Pt4 |
| Mining hotkey + beam + asteroid targeting | P1 | 1 wk | todo | SAVE-LOAD Pt5 |
| NPC loot pods on death | P1 | 3–4 d | todo | SAVE-LOAD Pt5 |
| Auto-save | P1 | 2–3 d | todo | SAVE-LOAD Pt6 |
| Wire BGM / engine / weapon / explosion sounds into apps | P1 | 1 d | todo | AUDIO Pt1 |
| Dynamic music manager (intensity cross-fade) | P2 | 2 d | todo | AUDIO Pt2 |
| Environmental ambiance | P2 | 1 d | todo | AUDIO Pt3 |
| Radar scanner widget | P2 | 1 wk | todo | Stability P5 |
| Volumetric fog / dust streams / cloud droplets | P2 | 3–4 d | todo | Stability P5 |
| Procedural audio (DSP synthesis) | P2 | 1–2 wk | todo | AUDIO Pt13 |

---

## 4. Settings (WIP) — unverified, do not build on yet

`resource/script/Widget/SettingsPanel.lts` + Window fullscreen/VSync APIs
were committed as **WIP** (`1549f45`) — **not runtime-verified**. Known
issues: **confirmed not to compile** — 97 compile errors (unbound
`Components:Margin`/`ToggleButton:Create`, missing `fullscreen`/`vsync`/
`masterVolume` widget fields; surfaced by the function-body error propagation,
see `ltsl-hardening.md` §6). Toggling fullscreen recreates the GL context and
drops shader/texture state (note `Shader_RecompileAll`); panel focus/delete
path unverified. Re-test before relying on any of it. `createSettings.md`
holds the original creation plan.

---

## 5. Deliberately NOT doing (with reasons)

| Feature | Reason | Alternative |
|---------|--------|-------------|
| Ray tracing | OpenGL 4.6 has no RT API | Screen-space reflections (SSR) |
| Tessellation | Needs hull/domain shaders | CPU high-poly meshes (planets already) |
| Global illumination | Needs baked light probes / RT | Baked cubemap reflections per system |
| Vulkan migration | High effort, low payoff for this engine | Stay on OpenGL 4.6 (see `VULKAN-AND-SPACE-PHENOMENA.md`) |
| LTSL bytecode VM | Only worth it if profiling shows need | Keep tree-walker; see §3.5 |
| Full C-style cast cleanup | `0` is overloaded (null vs int) | Manual, low-value sweep — see AGENTS §10.7 |

---

## 6. How to use this file

1. **Start a task:** pick the top `todo` item in §2, read its source doc, do it.
2. **Update as you work:** flip `todo → in-progress → done` and re-check the
   source doc's table if it tracks the same item.
3. **Don't duplicate:** this file is the plan; `AGENTS.md` is current state +
   completed work; the per-topic docs are the designs.

---

## 7. Verification

After any engine/C++/GLSL change:
- `python3 configure.py build`
- `python3 configure.py test` (unit tests: expect **399 checks, 0 failures**)
- LSP (if LTSL/API changed): `node script/ltsl-lsp/test-rpc.js` and
  `node script/ltsl-lsp/out/smoke.js` (expect **8 diagnostics**)

---

## 8. Source docs index

| Doc | Covers |
|-----|--------|
| `docs/PRD-LIMIT-THEORY-REBOOT.md` | Product vision, 6 phases, priorities P0–P2 |
| `docs/ENGINE-STABILITY-AND-MODDING.md` | Stability, JSON data-driven, UI/input, modding, scanners/FX |
| `docs/GRAPHICS-ROADMAP-SUMMARY.md` | Graphics quick wins + major upgrades, timeline |
| `docs/GRAPHICS-TECH.md` | Full GLSL implementation guides (PBR, HDR, nebula, biomes) |
| `docs/LTSL-ARCHITECTURE-AND-IMPROVEMENTS.md` | LTSL language extensions, bytecode VM |
| `docs/SAVE-LOAD-AND-INVENTORY.md` | Save schemas, serialization, inventory/mining wiring |
| `docs/PROCEDURAL-GENERATION-GUIDE.md` | SDFs, PlateMesh, shader generation, hybrid pipeline |
| `docs/AUDIO-SYSTEM-GUIDE.md` | Audio wiring, music manager, procedural audio |
| `docs/VULKAN-AND-SPACE-PHENOMENA.md` | Vulkan assessment (rejected), space phenomena |
| `ltsl-hardening.md` (repo root) | LTSL DX feedback, ordering/priority rules, hardening queue (P1–P3) |
| `AGENTS.md` | Current state, subsystem map, completed-work log, §8 universe gen |
| `SKILL.md` (`.opencode/skills/ltheory/`) | Master AI reference for engine + LTSL |
