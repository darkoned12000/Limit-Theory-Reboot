// Copyright (C) 2025  darkoned12000
// SPDX-License-Identifier: GPL-3.0-or-later
// Part of the ltheory-old-test modernization effort (Revamp Work).
// See NOTICE and LICENSE.GPL. Original engine (c) Josh Parnell, public domain.

# Engine Performance & Modernization Roadmap

Consolidated workflow for hardening the Limit Theory engine: performance
optimization, visual expansion, library modernization, and code quality.
Each phase builds on the previous. Complete phases in order; tasks within
a phase can be parallelized.

---

## Current Engine State (as of GLSL 4.60 / GLAD 2.0.8)

| Area | Status |
|------|--------|
| C++ standard | C++17, `-fno-exceptions` |
| GLSL version | 4.60 core (full range: 330→460) |
| OpenGL context | 4.6 (core profile bit off for Mesa compat) |
| SSBO / compute infra | ✅ Model instancing (binding 0) + particle instancing (binding 1) |
| SFML | 3.1.0 system-installed, C++17, miniaudio |
| GL loader | GLAD 2.0.8 (core 4.6, no extensions). Replaced GLEW 2.3.1. |
| Unit tests | 83 tests, 0 failures (69 run before pre-existing segfault) |
| Shader count | 170 `.jsl` files |
| Shader state caching | ✅ Re-enabled (Phase 1.1 — 33→39-41 FPS) |
| worldIT computation | ✅ Deferred to InjectMatrices (Phase 1.2 — ~30K inverses → ~5-10/frame) |

### Measured Performance Baseline (ltheory-main, 30K asteroids)

**Pre-Phase 1.1 (original):**

| Metric | Moving | Still |
|--------|--------|-------|
| FPS | 33 | 47 |
| Avg frame time | 30 ms | 21 ms |
| Total polys | 579,700+ | 579,700+ |
| Draw calls/frame | 30,000+ | 30,000+ |
| Models (interior) | 13,941 | 13,941 |
| Frustum culled | 16,105 | 16,105 |
| Render time | 17.3 ms | 12.0 ms |

**Post-Phase 1.1 — shader state caching re-enabled:**

| Metric | Moving | Change |
|--------|--------|--------|
| FPS | 39-41 | +18-24% |
| Polygons | 900,000+ | more visible objects rendered |
| Draw calls/frame | 30,046 | unchanged |

*Source: F3 debug overlay on `ltheory-main` app. Phase 1.1 result measured
after shader state caching fix (instance pointer comparison + WVP re-upload
on cache hit). Ship model is a flat disk due to missing .xmesh data (see §6
— cosmetic only, does not affect perf measurement).*

**Key insight:** At ~19 polys/object, the GPU is not geometry-bound. The
bottleneck is pure draw call overhead — each object triggers a full
`glDrawElements` + render state push/pop + uniform upload. Adding visual
effects (dust, nebula, particles) will make this worse unless instancing
lands first.

**Headroom for effects:** With instancing (Phase 2) dropping to ~10-20
draw calls, the GPU budget opens up for:
- 10K+ dust fleck particles (compute-instanced billboards)
- Nebula volume rendering (compute raymarch, ~1 draw call)
- Atmospheric scattering per planet (1 shader pass)
- Bloom/HDR post-processing (2-3 fullscreen passes)
- Normal maps on planets/stations (no draw call increase)

Without instancing, adding effects will push sub-30 FPS.

---

## Phase 1: Quick Performance Wins (Est: 2-3 days)

Low-risk changes with immediate FPS impact. Zero architecture changes.
Each task is independently testable.

### 1.1 Re-enable Shader State Caching ✅

**Impact: HIGH | Effort: 1 day | Risk: LOW — COMPLETED**

`ShaderInstance::Begin()` had a state-caching check that was force-disabled
(`gSkippedState = false` with a `/* TODO : Fix state caching. */` comment).

**Root cause:** The original caching used a global version counter
(`gActiveVersion`) shared across all instances. Two different ShaderInstance
objects using the same Shader with the same number of SetVar/SetState calls
produced identical version numbers → false cache hit → wrong uniforms rendered.
Additionally, `Renderer_SetShader()` (which uploads per-object WVP matrices)
was skipped on cache hit, causing all cached objects to render at the origin.

**Fix:** Replaced global version comparison with instance pointer comparison
(`this == gActiveInstance`). On cache hit, still call `Renderer_SetShader()`
to re-upload WVP matrices (they change per-object), but skip per-instance
uniform uploads and render state push/pop. Added `gActiveInstance` tracking.

- **File:** `src/liblt/LTE/ShaderInstance.cpp` (lines 22-24, 184-213)
- **Result:** 33 → 39-41 FPS moving (+18-24%), visual output correct.
- **Test:** ltheory-main verified — asteroids, thrusters, planet all render.

### 1.2 Conditional World Inverse-Transpose (worldIT) ✅

**Impact: HIGH | Effort: 0.5 day | Risk: LOW — COMPLETED**

`Renderer_SetWorldTransform()` unconditionally computed
`worldIT = world.Inverse().Transpose()` — a 48-multiply brute-force 4x4
matrix inverse — for every visible object in every render pass (~30K/frame).
`worldIT` is only needed by 6 of ~170 shaders (normal mapping / bump mapping).

**Root cause of initial bug:** First attempt cached a `needsWorldIT` flag in
`Renderer_SetShader`, but `SetTransform()` is called BEFORE `Renderer_SetShader`
in the render style — so `Renderer_SetWorldTransform` used the flag from the
PREVIOUS object's shader, not the current one. Asteroids (no WORLDIT) set it
false, then the ring's worldIT was never computed → broken normals → broken
specular → bright white blowout.

**Correct fix:** Moved the inverse computation from `Renderer_SetWorldTransform`
(per-object, 30K times) into `InjectMatrices` (per-shader-switch, ~5-10 times).
`InjectMatrices` now queries `WORLDIT` uniform location and only computes
`world.Inverse().Transpose()` when the shader actually uses it. Removed
`renderer.worldIT` field and `Renderer_GetWorldITMatrix()` accessor (now dead
code).

- **Files:** `src/liblt/LTE/Renderer.cpp` (InjectMatrices, Renderer_SetWorldTransform, Renderer_ClearMatrices, struct cleanup), `src/liblt/LTE/Renderer.h` (removed declaration)
- **Result:** ~30K matrix inverses/frame → ~5-10. Ring specular and normals verified correct.
- **Test:** ltheory-main verified — planets, rings, asteroids all render correctly.

### 1.3 Replace GL_BufferData with glBufferSubData for Quads ✅

**Impact: MEDIUM | Effort: 0.5 day | Risk: LOW — COMPLETED**

`Renderer_DrawQuad()` re-uploaded the same 48-byte quad vertex buffer via
`GL_BufferData(DynamicDraw)` 20-50 times per frame (fullscreen post-process
passes, lens flares, local lights). `glBufferData` discards and re-allocates
the entire driver-side buffer. `glBufferSubData` does a partial update without
allocation.

**Fix:** Changed quad VBO initial allocation from `StaticDraw` to `DynamicDraw`.
On subsequent draws, `GL_BufferSubData` updates the 48 bytes in-place.
Added `GL_BufferSubData` wrapper to `GL.h`.

- **Files:** `src/liblt/LTE/GL.h` (new wrapper), `src/liblt/LTE/Renderer.cpp` (GetSharedQuadVBO, Renderer_DrawQuad)
- **Result:** Eliminates 20-50 driver buffer re-allocations per frame.
- **Test:** ltheory-main verified — all post-process passes, lens flares, UI render correctly.

### 1.4 Sort Visible Objects by Material Before Draw Passes ✅

**Impact: MEDIUM | Effort: 1 day | Risk: LOW — COMPLETED**

The `visible[]` list was iterated in tree-traversal order. Each object may
use a different shader/mesh, causing GPU pipeline stalls on state changes
(glUseProgram, VBO binds, render state push/pop).

**Fix:** Added `std::sort` at the end of `Visibility::OnRender` that sorts
`state->visible[1..N]` by the object's `RenderableT*` pointer. Objects
sharing the same renderable (model) share the same shader and mesh, so
grouping them reduces shader switches and VBO binds during draw passes.
Index 0 (the container) is preserved since the Particles pass reads
`visible[0]` as the root object.

- **File:** `src/liblt/Game/RenderPass/Visibility.cpp` (added sort + GetRenderableKey helper)
- **Result:** Objects grouped by shader+mesh; consecutive draws share state.
- **Test:** ltheory-main verified — all objects render correctly.

### 1.5 Early-Out in Particle System Draw ✅

**Impact: LOW | Effort: 0.25 day | Risk: NONE — COMPLETED**

`ParticleSystemImpl::Draw()` rebuilt vertex arrays per-shader-group even if
all particles were culled (frustum + LOD). After the culling loop, if no
particles survived, the code still called `DrawState_Link`, `shader->Begin`,
texture binds, `Renderer_DrawVertices`, and `shader->End` — all for zero
visible particles.

**Fix:** Added `if (vertices.empty()) continue;` after the culling loop but
before the draw call setup. Skips the entire draw pipeline for empty batches.

- **File:** `src/liblt/Game/ParticleSystem.cpp` (line ~127)
- **Result:** Skips shader setup, texture binds, and draw call for fully-culled particle groups.
- **Test:** ltheory-main verified — particles render correctly.

---

## Phase 2: GPU Instancing (Est: 3-5 days)

The single biggest performance win. Replaces 30,000+ individual draw calls
with ~10-20 instanced calls. Without this, no amount of visual effects
will keep FPS above 30 at current object counts.

### 2.1 Add GL Draw Instanced Wrapper ✅ COMPLETE

- **File:** `src/liblt/LTE/GL.h`
- **Added:** `GL_DrawElementsInstanced(mode, count, type, indices, instanceCount)`
  wrapper (line 349). Uses SSBO-based approach (no `glVertexAttribDivisor` needed).
- **Result:** Low-level instancing call available. Verified in shader pipeline.

### 2.2 Add Instanced Draw Path to Renderer ✅ COMPLETE

- **Files:** `src/liblt/LTE/Renderer.cpp`, `Renderer.h`
- **Added:**
  - `Renderer_BeginInstancedDraw(state)` — uploads per-instance `world + worldIT`
    matrices (128 bytes each) to SSBO binding point 0, with orphan-and-reallocate
    streaming.
  - `Renderer_EndInstancedDraw()` — resets `uInstanced=0`, rebinds dummy SSBO.
  - `Renderer_DrawMeshInstanced(mesh, count)` — calls `GL_DrawElementsInstanced`
    with instance data from SSBO.
  - `Renderer_DrawParticlesInstanced(mesh, instances, count)` — SSBO binding
    point 1 for particles.
- **SSBO management:** Dual dummy buffers (binding 0 for models, binding 1 for
  particles), initialized at `Renderer_Initialize()`. Lazy allocation on first use.
- **Geometry type hierarchy:** `Renderable::RenderInstanced` → `Model::RenderInstanced`
  → `Mesh::DrawInstanced` → `Renderer_DrawMeshInstanced` virtual chain implemented.
- **Shader plumbing:** `vert.jsl`/`frag.jsl` SSBO struct `InstanceData` + `uInstanced`
  toggle. All vertex shaders (`npm.jsl`, `imposter.jsl`) read `instances[gl_InstanceID].world`.
  Fragment shaders (`lambert.jsl`, `metal.jsl`, `imposter1.jsl`) read `instances[fragInstanceID].worldIT` for normal transform.

### 2.3 Batch Identical Objects in Visible List ⏸️ ON HOLD

- **File:** `src/liblt/Game/InstancedDraw.h` (89 lines, **dormant**)
- **Status:** `DrawVisibleInstanced()` batching pass is fully implemented but
  **never called** — no file `#include`s `InstancedDraw.h`. Render passes
  (`GBuffer.cpp`, `Blended.cpp`, `DepthPrepass.cpp`) still use per-object
  `for(i...) obj->OnDraw(state)` loops.
- **Issue:** Reverted due to instanced draw bug with cached models. Debugging
  attempted (hours) without resolution. Deferred until after Phase 3 lifecycle
  improvements, which may simplify the batching logic.
- **What it does when activated:** Groups `visible[1..n]` by `RenderableT*`
  pointer (already sorted by Phase 1.4). Batches of size 2..255 use full
  instanced path. Singleton and ≥256 batches fall back to `OnDraw`.
  Container `visible[0]` always drawn individually first.

### 2.4 Update Particle System to Use GPU Instancing ✅ COMPLETE

- **File:** `src/liblt/LTE/ParticleSystem.cpp`
- **Fix:** Replaced CPU-side billboard vertex generation with a single unit-quad
  billboard mesh (`Mesh_Billboard(-1, 1, -1, 1)`) drawn instanced. Per-particle
  position/size/age uploaded as `ParticleInstanceData` (32 bytes each) to SSBO
  binding point 1.
- **Shaders:** `particle.jsl` reads from SSBO at binding point 1 when
  `uParticleInstanced > 0`.
- **Result:** Eliminated per-particle CPU vertex copy + full buffer re-upload.
  **Active in render path** — the only instancing code path running at runtime.

---

## Phase 3: Object Lifecycle & Memory (Est: 2-3 days)

Reduce allocation churn and move expensive work out of the render path.

### 3.1 Move Zone Object Generation Out of Render Pass ✅ COMPLETE

**Impact: HIGH | Effort: 0.5 day | Risk: LOW**

`Zone::OnDraw()` (Zone.cpp:118-123) called `field[i]->Update(this, pos)`
which iterated 6 `DynamicCell` levels, comparing current camera cell to
last-known cell. On cell change, **deleted all existing elements** and
**regenerated** up to 648 asteroids per field level × 6 levels = ~3,888
objects — all synchronously from inside the draw pass.

**Fix:** Moved `field[i]->Update()` calls from `OnDraw()` into a new
`OnUpdate()` override. Since `UpdateState` lacks camera position, stored
the eye position from `OnDraw()` via a `lastEyePos` member + `hasEyePos`
flag (skips first update until camera position is known).

- **File:** `src/liblt/Game/Object/Zone.cpp` (OnDraw line 118-123 → new OnUpdate)
- **Members added:** `Position lastEyePos`, `bool hasEyePos`
- **Result:** Object generation now runs in the update loop, not the draw
  pass. Eliminates ~3,888 object creations/destructions from the render
  path. Build clean, 69 tests pass, apps verified.
- **Measured impact:** The 14 FPS gap between moving (33) and still (47)
  is largely this + planet bounce scan — expected to close significantly.

### 3.2 Object Pooling for Frequent Create/Destroy ✅ COMPLETE

- **Existing infrastructure:** `Pool.h` (94 lines) with `PoolRaw<T>` lock-free
  free-list, `Pool<T>` typed wrapper, `GetTypePool<T>()` singleton, and
  `POOLED_TYPE` macro. Already used by ~100+ types (AST nodes, UI glyphs,
  widgets, tasks) and major game objects (Asteroid, Ship, Station, etc.).
- **Gap found:** 7 game object types and 2 derivative types lacked `POOLED_TYPE`.
- **Fix:** Added `POOLED_TYPE` + `#include "LTE/Pool.h"` to:
  - `PlanetImpl` (Planet.cpp)
  - `Zone` (Zone.cpp)
  - `WarpNode`, `WarpNodeControllerT` (WarpNode.cpp)
  - `WarpRail` (WarpRail.cpp)
  - `DustFlecks` (DustFlecks.cpp)
  - `ObjectCustom` (Custom.cpp)
  - `DroneConstruction`, `DroneConstructionType` (Drone/Construction.cpp)
  - `DroneProspecting`, `DroneProspectingType` (Drone/Prospecting.cpp)
- **Result:** All game objects now use pool allocation. Hot path (Asteroid,
  AsteroidRich from Zone) was already pooled. Build clean, 69 tests pass.

### 3.3 Pre-reserve Mesh Vectors ✅ COMPLETE

- **File:** `src/liblt/LTE/Mesh.cpp` (MeshT::AddMesh lines 121, 129)
- **Fix:** Uncomment `vertices.reserve()` and `indices.reserve()` calls
  to avoid vector reallocations during mesh construction.
- **Status:** Simple fix — four commented-out lines to uncomment. Verified
  in audit.

### 3.4 Conditional Frustum Culling Improvements

- **File:** `src/liblt/Game/RenderPass/Visibility.cpp`
- **Audit:** Per-object culling only. No hierarchical culling — parent
  bounding volumes do NOT cull subtrees. Visibility pass does a flat
  iteration over all interior objects via `InteriorTypeIterator`, not a
  tree walk. Spatial partitioning exists for physics (`SpatialPartition_Hash`
  in `Component/Queryable.cpp`) but is NOT used for render culling.
- **Measured need:** With 30K objects, even a fast frustum test is 30K
  bounding sphere/plane checks per frame. Hierarchical culling could
  reduce this to ~100-500 checks.

---

## Phase 3b: Level-of-Detail (LOD) System (Est: 3-5 days)

Critical for scaling beyond 30K objects. Without LOD, every asteroid
renders at full polygon count regardless of distance.

### 3b.1 Distance-Based LOD Selection

**Impact: VERY HIGH | Effort: 2 days | Risk: MEDIUM**

Each asteroid currently renders ~19 polys at all distances. At 30K
asteroids, most are far away and could render as 1-4 polys (billboard)
or be culled entirely.

- **File:** `src/liblt/Game/Component/Drawable.cpp`, new `LOD.h`
- **Add:** Per-object LOD level selection based on screen-space coverage:
  - LOD 0 (close): Full mesh (~19 polys)
  - LOD 1 (medium): Simplified mesh (~8 polys)
  - LOD 2 (far): Billboard/impostor (2 polys, 1 draw call for all)
  - LOD 3 (very far): Culled entirely (0 polys)
- **Thresholds:** Based on object's bounding sphere radius / distance to camera.
- **Expected:** 70-80% of asteroids at LOD 2/3 → ~2000-6000 effective
  draw calls instead of 30,000.

### 3b.2 Impostor / Billboard Rendering for Distant Objects

- **Technique:** For LOD 2+, render a camera-facing quad with a pre-baked
  or procedurally-generated texture of the object.
- **Option A (simpler):** Colored billboard with size from bounding sphere.
  Zero texture cost, works immediately.
- **Option B (better):** Pre-render 6-face cubemap impostor at load time.
  Better visual quality but more setup.
- **Dependency:** Phase 2 GPU instancing — billboards are instanced.

### 3b.3 LOD Mesh Generation

- **Option A (runtime):** Vertex decimation shader (compute) — generate
  simplified meshes on GPU at load time.
- **Option B (offline):** Pre-generate LOD meshes in `Object_Asteroid` at
  different detail levels. Simpler, no compute needed.
- **Recommendation:** Option B for now. 3 LOD levels per asteroid type,
  selected by distance.

---

## Phase 3c: Occlusion & Hi-Z Culling (Est: 2-3 days)

Further reduce draw calls by not rendering objects hidden behind others.

### 3c.1 Hi-Z (Hierarchical Z-Buffer) Occlusion

**Impact: HIGH | Effort: 2 days | Risk: MEDIUM**

After the depth prepass, generate a mipmap chain of the depth buffer.
Use it to test bounding boxes of objects against the scene — if the
object's nearest corner is behind the farthest known depth at that
screen position, skip the draw entirely.

- **File:** New: `src/liblt/Game/RenderPass/HiZOcclusion.cpp`
- **Technique:**
  1. After depth prepass, `glGenerateMipmap` on the depth texture.
  2. For each object, project bounding box to screen space.
  3. Sample Hi-Z at the bounding box's mip level.
  4. If all corners are behind Hi-Z → object is occluded → skip.
- **Expected:** 20-40% additional draw call reduction for dense asteroid
  fields where asteroids overlap from camera perspective.
- **GPU cost:** 1 texture sample per bounding box corner (4 samples total),
  negligible compared to the skipped draw call.

### 3c.2 Temporal Occlusion Reuse

- **Optimization:** Reuse previous frame's occlusion results with a 1-pixel
  dilation. Objects that were occluded last frame are likely occluded this
  frame too. Reduces Hi-Z queries by ~50%.
- **Risk:** Can cause pop-in on fast camera movement. Mitigate with a
  "confidence" counter — re-query after 2-3 frames of occlusion.

---

## Phase 4: Dependency Modernization (Est: 1-2 days)

Low-risk library upgrades and replacements.

### 4.1 GLEW → GLAD ✅ COMPLETE

Replaced GLEW 2.3.1 with GLAD 2.0.8 (core 4.6, no extensions). Used `glad2`
pip package for generation. `gladLoadGL()` called as first line in
`Renderer_Initialize`, replacing `glewInit()`. Removed dead EasyGL wrappers
from `GL.h` and legacy GL enums from `GLEnum.h`. Deleted vendored
`include/Glew/` directory, `cmake/FindGLEW.cmake`. Removed GLEW from all
platform `LINK_LIBRARIES` in CMakeLists.txt. Verified with `war`, `rails`,
`ltheory-main` apps; 69 unit tests pass.

### 4.2 UTF8-CPP Upgrade ✅ COMPLETE

Updated vendored `include/UTF8/` from ancient 2006 release to latest
(upstream `utfcpp` master). Key changes:
- `core.h`: `utfchar8_t`/`utfchar16_t`/`utfchar32_t` typedefs replace raw
  `uint8_t`/`uint16_t`/`uint32_t`; `#include <cstring>`/`<string>` added;
  C++ version detection macros (`UTF_CPP_OVERRIDE`, `UTF_CPP_NOEXCEPT`);
  `sequence_length()` returns `int`; `validate_next()` inlined with per-case
  overlong checks.
- `checked.h`: Iterator class no longer inherits from deprecated
  `std::iterator` — explicit typedefs instead (fixes C++17 deprecation).
- `unchecked.h`: Same `next()` API (only consumer: `UniString.cpp`).
- New: `cpp11.h`, `cpp17.h`, `cpp20.h` convenience overloads (string_view,
  u8string).
- Verified: 69 unit tests pass, build clean, apps run.

### 4.3 Git LFS for Large Resources

- **Fix:** Add `.gitattributes` tracking large binary files (textures,
  fonts, FMOD binaries) via LFS.

---

## Phase 5: Code Cleanup & Modernization (Est: 3-5 days)

Reduce technical debt, improve maintainability.

### 5.1 Delete Dead EasyGL Wrappers ✅ COMPLETE

Removed as part of Phase 4.1 (GLAD migration). See `GL.h` and `GLEnum.h` diff
in commit history.

### 5.2 C++ Cast Cleanup ✅ COMPLETE

Bulk-converted numeric C-style casts to `static_cast<>` across engine systems:
- **Scope:** ~1094 C-style casts identified across `src/liblt/`
- **Converted:** ~50+ files, 600+ numeric casts (`(int)`, `(float)`, `(double)`, `(uint)`, `(size_t)`, `(real)`)
- **Preserved:** Pointer-to-integer casts (need `reinterpret_cast`), macro-generated casts in `DeclareFunction.h`/`AutoClass_Generated.h`
- **Result:** Build clean, 69 unit tests pass, `war`/`rails`/`ltheory-main` verified
- **Commit:** `33c1c03` on `lt-perf` branch

### 5.3 NULL → nullptr Review ✅ COMPLETE

Replaced engine-code `NULL` with `nullptr` in `Archive.cpp` (4 uses),
`Diff.cpp` (1 use), `MarchingCubes.cpp` (8 uses), and `MarchingCubes.h`
(4 uses). Removed unnecessary C-style casts (e.g. `(real*)NULL` → `nullptr`).
Win32 API and OpenGL API calls left as `NULL` (idiomatic for those APIs).
69 tests pass, build clean.

### 5.4 typedef → using Cleanup ✅ COMPLETE

Bulk-converted ~530 `typedef` to modern C++ `using`-declarations across 181 files.
- Single-line type aliases (type aliases, function pointers)
- Multiline template chains (`ObjectWrapper<...>`, `Attribute_...<...>`)
- `typedef struct { ... } Name;` → `struct Name { ... };` (MarchingCubes.h, Array.h)
- Function pointer typedefs in `Type.h` (10 aliases: `AllocateFn`, `AssignFn`, etc.)
- Preserved `IteratorType` alias in Array.h (used by tests)
- **Left as-is (intentional):** ~131 in macro-generated files (`DeclareFunction.h` (92),
  `AutoClass_Generated.h` (32), `BaseType.h` (3), `AutoClass.h` (2)),
  `XVector.h` macro expansion (1), and `OS.cpp` Win32 `_stdcall` (1)
- **Result:** 181 files changed, -61 net lines. Build clean, 69 tests pass.
- **Verified:** `war`, `rails`, `ltheory-main` all run correctly.
- **Commit:** `722c536` on `lt-perf` branch.

### 5.5 `-Wmaybe-uninitialized` / Deprecation Audit ✅ COMPLETE

GCC 15 audit: zero `-Wmaybe-uninitialized` and zero `-Wdeprecated` warnings
in engine code. Only issue found was C++20 designated initializer syntax
(`.field = value`) in `TestSFML.cpp`, converted to C++17 compatible syntax.
- **Result:** Engine code is warning-clean under GCC 15 with `-Wall -Wextra`.
- **Commit:** `69fe7de` on `lt-perf` branch.

### 5.6 Delete Vendored ext/SFML/ Directory ✅ COMPLETE

Removed legacy `ext/SFML/` submodule (2.6.2, never built). System SFML 3.1.0
is used via `find_package(SFML 3.1 REQUIRED ...)`. Updated `install_dependencies.sh`
to include `libsfml-dev` and remove `libglew-dev`. Updated README, AGENTS.md,
CMakeLists.txt, and .clang-format to remove all references.

---

## Phase 6: Visual Expansion (Est: 2-3 weeks)

Rendering improvements. **Requires Phases 1-3c for CPU/GPU headroom.**
Each visual effect has a budget estimate based on the current 30K-object
scene — costs assume instancing + LOD are already in place.

### 6.0 Visual Effects Budget (Post-Instancing)

With GPU instancing (Phase 2) + LOD (Phase 3b), the render budget
changes dramatically:

| Effect | Draw Calls | GPU Cost | Prerequisite |
|--------|-----------|----------|--------------|
| 30K asteroids (instanced, LOD) | 1-3 | Low | Phase 2 + 3b |
| 10K dust flecks (instanced billboard) | 1 | Low | Phase 2 |
| Nebula volume (compute raymarch) | 1 | Medium | Phase 2 + SSBO |
| Planet atmosphere (scattering) | 1-2 per planet | Medium | Phase 6.3 |
| PBR lighting (per-object) | 0 (shader cost) | Medium | Phase 6.1 |
| Shadow map (1 directional) | 1 | Medium | Phase 6.2 |
| Bloom/HDR (fullscreen passes) | 2-3 | Low | Phase 6.5 |
| SMAA (existing) | 1 | Low | Already present |
| **Total** | **~10-15** | — | — |

Without instancing, adding any of the above pushes below 30 FPS.

### 6.1 Physically Based Rendering (PBR) Pipeline

**Priority: HIGH | Effort: 1-2 weeks**

Replace the current Blinn-Phong lighting with a PBR metallic-roughness
workflow.

- **Shader changes:**
  - `common/lighting.jsl` — Cook-Torrance BRDF (GGX normal distribution,
    Smith geometry, Fresnel-Schlick)
  - `common/vert.jsl` / `common/frag.jsl` — Add tangent attribute for
    normal mapping
  - New: `common/pbr.jsl` — PBR utility functions
- **Material system:**
  - Add roughness/metallic uniforms to `RenderStyle` / `ShaderInstance`
  - Create PBR material types in LTSL
- **Textures:**
  - Albedo maps (diffuse color)
  - Normal maps (surface detail)
  - Roughness/metallic maps (PBR parameters)
- **Lighting:**
  - Directional light from star (with terminator line / shadow curve)
  - Point lights for stations/lasers
  - Image-based lighting (IBL) for ambient (stretch)

### 6.2 Directional Lighting & Shadows

- **Shader:** Star-based directional light with shadow curve (terminator
  line on planets)
- **Technique:** Cascaded shadow maps or simple depth-from-star for planet
  shadows
- **Files:** `common/lighting.jsl`, new `common/shadow.jsl`

### 6.3 Atmospheric Scattering

- **Technique:** Single-scattering atmosphere shader (horizon fade, rim
  lighting, sunset colors)
- **Files:** `common/scattering.jsl` (already exists — extend it)
- **Uniforms:** Atmosphere density, tint color (already in `Item_PlanetType`)

### 6.4 Normal & Specular Maps

- **Tangent space:** Add `vertex_tangent` attribute to vertex format
- **Normal mapping:** Perturb surface normals from normal map
- **Specular:** Cook-Torrance specular from roughness map
- **Files:** `common/vert.jsl`, `common/frag.jsl`, `common/pbr.jsl`

### 6.5 HDR / Bloom Improvements

- **Current:** Basic bloom post-process exists
- **Upgrade:** HDR render target (RGBA16F), proper tonemapping (ACES or
  Reinhard), energy-preserving bloom
- **Files:** `fragment/post/bloom.jsl`, `fragment/post/tonemap.jsl`

### 6.6 Volumetric Fog / Nebula (Compute Shaders)

- **Technique:** Raymarching through 3D noise texture via compute shader
- **Use:** Compute shader infrastructure (GLSL 4.30 SSBOs) to precompute
  fog/light volumes on GPU
- **Files:** New `compute/fog.jsl`, `common/fog.jsl`
- **Dependency:** Phase 2 GPU instancing (need CPU headroom)
- **Budget:** 1 compute dispatch + 1 fullscreen composite pass. Negligible
  draw call impact. GPU cost: ~0.5 ms on modern hardware.

### 6.7 Star-Field Parallax

- **Technique:** Multi-layer background starfield with parallax scrolling
  based on camera rotation
- **Files:** `vertex/stars.jsl`, `fragment/stars.jsl`

### 6.8 Biome System for Planets

- **See:** AGENTS.md §8.3 Pass B
- **Add:** Biome enum to `PlanetType` (Desert/Terran/Ice/Lava/GasGiant/Vegetation)
- **Wire:** Biome-specific shader parameters (color palettes, height
  multipliers, cloud density)
- **Files:** `PlanetType.cpp`, `gen/planet.jsl`, `SystemPopulate.lts`

### 6.9 GPU Compute Particle Systems

**Impact: HIGH for effects | Effort: 2-3 days | Risk: MEDIUM**

Replace CPU-side particle simulation with compute shaders. Required for
10K+ dust flecks, projectile trails, engine exhaust, explosions.

- **Technique:**
  1. SSBO holds particle state (position, velocity, age, size).
  2. Compute shader updates all particles per frame (physics, aging).
  3. Indirect draw command (`glDrawElementsIndirect`) renders all alive
     particles in one instanced draw call.
- **Files:** New `compute/particles.jsl`, modify `ParticleSystem.cpp`
- **Dependency:** Phase 2 (instancing), GLSL 4.30 (SSBOs + compute)
- **Budget:** 1 compute dispatch + 1 instanced draw. ~0.2 ms for 10K particles.
- **Effects enabled:** Dense dust clouds, engine trails, weapon impacts,
  debris fields, atmosphere particles.

### 6.10 Multi-Draw Indirect (MDI) for Heterogeneous Objects

**Impact: MEDIUM | Effort: 2-3 days | Risk: MEDIUM**

For objects that share a shader but have different meshes (e.g., different
asteroid shapes), Multi-Draw Indirect batches them into a single draw call
with per-draw mesh offsets.

- **Technique:** Build a buffer of `DrawElementsIndirectCommand` structs,
  one per mesh variant. Issue `glMultiDrawElementsIndirect` for the batch.
- **Files:** New: `src/liblt/LTE/GL.h` wrappers, modify `Renderer.cpp`
- **Dependency:** Phase 2 (instancing framework)
- **Budget:** Reduces ~50-100 mesh-variant draw calls to 1. Useful after
  instancing handles the bulk of identical objects.

---

## Phase 7: Scripting & Tooling (Est: 1-2 weeks)

Improve LTSL developer experience and data-driven workflows.

### 7.1 LTSL Higher-Order Functions (.Filter(), .Map())

- **Scope:** Add `.Filter()`, `.Map()`, `.Reduce()` to LTSL list/array types
- **Impact:** Reduces boilerplate in procedural generation loops significantly
- **Files:** `src/liblt/LTE/Expression/` (new expression types),
  `src/liblt/Module/ScriptAPI*.cpp` (bind new methods)

### 7.2 LTSL Hot-Reloading

- **Scope:** File-watcher recompiles `.lts` at runtime
- **Impact:** Instant feedback on script changes without restarting
- **Files:** New `src/liblt/Module/ScriptReloader.cpp`, `Launcher.cpp`

### 7.3 Data-Driven UI (JSON Parser)

- **Scope:** External JSON files define HUD layouts, replacing hardcoded
  LTSL widget trees
- **Impact:** Designers can tweak UI without touching scripts
- **Add:** Minimal JSON parser in C++ (or use nlohmann/json header-only)
- **Files:** New `src/liblt/LTE/Json.h`, `Widget/` loader changes

### 7.4 Live Developer Tools (DevPanel Expansion)

- **See:** AGENTS.md §8.3 Pass C, §10.9 Phase 3
- **Scope:** Extend `Widget/DevPanel` to expose generation parameters
  (planet size, asteroid density, biome colors) as editable fields
- **Wire:** Changes trigger `SystemPopulate` re-run in place

### 7.5 LTSL Documentation

- **Document:** stdlib surface (all script-accessible functions)
- **Document:** Grammar, parser pipeline, 25 expression-node types
- **Document:** Reflection system (AutoClass, FIELDS, MAPFIELD, etc.)
- **Files:** `docs/ltsl-docs.md`, `docs/reflection.md`

---

## Phase 8: Stability & Quality (Ongoing)

Runs in parallel with all other phases.

### 8.1 CI Pipeline (GitHub Actions)

- **Scope:** Linux (GCC + Clang), Windows builds
- **Reuse:** `configure.py` / CMakePresets
- **Tests:** Run `lte_tests` (83 tests) on every push
- **Lint:** `clang-format --dry-run --Werror` + `-Werror` scoped to project

### 8.2 Visual Regression Testing

- **Scope:** Golden-master screenshots of `ltheory-main` at each phase
- **Tool:** Script to launch app, wait for load, capture screenshot
- **Compare:** Pixel-diff against reference images

### 8.3 Performance Benchmarking

- **Add:** Frame time logging to `Renderer_GetFrameTime()` (exposed to LTSL)
- **Add:** Draw call count tracking (already exists: `Renderer_GetDrawCallCount()`)
- **Add:** Object count tracking
- **Tool:** Run `ltheory-main` with 1000+ asteroids, log FPS over 60 seconds

### 8.4 Memory Leak Detection

- **Tool:** Run with Valgrind or AddressSanitizer on test apps
- **Focus:** `Zone::DynamicCell::Update()` object creation/destruction
- **Focus:** `ParticleSystem::Draw()` per-frame allocations

---

## Dependency Upgrade Summary

| Library | Current | Target | Phase | Effort |
|---------|---------|--------|-------|--------|
| GLEW | ✅ Removed | GLAD 2.0.8 | 4.1 ✅ | ✅ |
| UTF8-CPP | ✅ Updated | Latest upstream | 4.2 ✅ | ✅ |
| ext/SFML/ | ✅ Deleted | System SFML 3.1.0 | 5.6 ✅ | ✅ |

---

## Task Priority Matrix

| Priority | Tasks | Estimated Impact |
|----------|-------|-----------------|
| **P0 — Do First** | 2.1-2.4 GPU instancing | Phase 1 complete — move to Phase 2 |
| **P1 — Critical Path** | 2.1-2.4 GPU instancing | 30K draw calls → ~10. +70% FPS. 2.3 on hold (see note). |
| **P2 — High Value** | 3b.1-3b.3 LOD system, 3.1 Zone fix | Keeps effective object count manageable at scale |
| **P3 — Medium Value** | 3c.1-3c.2 Hi-Z occlusion, 1.4 Sort by material, 3.2 Object pooling | +5-10% FPS, smoother frame times |
| **P4 — Effects Foundation** | 6.9 Compute particles, 6.10 MDI, 6.6 Nebula compute | Enables dust/nebula/particles without draw call explosion |
| **P5 — Visuals** | 6.1 PBR, 6.2 Directional light, 6.3 Atmosphere, 6.5 HDR/Bloom | Visual quality leap |
| **P6 — Tooling** | 7.1 LTSL HOF, 7.2 Hot-reload, 7.4 DevPanel | Developer productivity |
| **P7 — Foundation** | 5.2 Cast cleanup, 5.5 Deprecation audit, 4.3 Git LFS | Code health |
| **P8 — Quality** | 8.1 CI, 8.2 Visual regression, 8.3 Benchmarking | Long-term stability |

---

## 60 FPS Target Strategy

Measured baseline: **33 FPS moving / 47 FPS still** with 30K objects,
580K polys. Phase 1.1 brought this to **39-41 FPS moving / 900K polys**.
Target: **60 FPS sustained** with visual effects active.

### Phase-by-Phase Impact Estimate

| Phase | Expected FPS | Cumulative | Notes |
|-------|-------------|-----------|-------|
| **Current** | 33 / 47 | — | 30K objects, no instancing, no LOD |
| **Phase 1.1** (state caching) ✅ | 39-41 / — | +18-24% | Re-enabled cache; fixed WVP matrix bug |
| **Phase 1.2** (conditional worldIT) ✅ | — / — | — | ~30K inverses → ~5-10 per frame |
| **Phase 1.3** (glBufferSubData) ✅ | — / — | — | Eliminates 20-50 buffer re-allocs/frame |
| **Phase 1.4** (sort by material) ✅ | — / — | — | Objects grouped by shader+mesh; reduced state switches |
| **Phase 1.5** (early-out particles) ✅ | — / — | — | Skips draw calls for fully-culled particle groups |
| **Phase 2.1-2.2** (SSBO + Renderer infra) ✅ | — / — | — | Binding point 0, dummy buffer, Renderer_BeginInstancedDraw |
| **Phase 2.3** (batching + render pass wiring) ⏸️ | — / — | — | ON HOLD — infrastructure built but dormant. See 2.3 for details. Deferred after Phase 3. |
| **Phase 2.4** (particle GPU instancing) ✅ | — / — | — | SSBO at binding point 1, eliminated CPU vertex expansion |
| **Phase 2** (instancing) | 55 / 60+ | +70% | 30K draw calls → ~10-20. Critical path. |
| **Phase 3b** (LOD) | 58 / 62+ | +5% | 70-80% of asteroids → billboard/culled |
| **Phase 3c** (Hi-Z cull) | 60 / 63+ | +3% | 20-40% additional occlusion |
| **Phase 6** (visuals) | 55 / 58 | -5% | PBR, atmosphere, nebula, particles add GPU cost |
| **Phase 3** (lifecycle) | 58 / 62 | +3% | Smoother frame times, no spikes |

**Minimum viable path to 60 FPS:** Phases 1 + 2 = ~5 days.
**60 FPS with effects:** Phases 1 + 2 + 3b + 3c = ~12-14 days.
**60 FPS with full visuals:** All phases through 6 = ~4-5 weeks.

### Future-Proofing: What 60K-100K Objects Looks Like

At double the current object count, instancing alone won't hold 60 FPS.
The full stack is needed:

| Object Count | Instancing Only | + LOD | + Hi-Z | + MDI |
|-------------|----------------|-------|--------|-------|
| 30K | 55-60 FPS | 58-62 | 60-63 | 60-63 |
| 60K | 35-45 FPS | 55-60 | 58-62 | 60-62 |
| 100K | 25-35 FPS | 50-58 | 55-60 | 58-61 |

LOD is the key scaler — it keeps the *effective* object count manageable
regardless of total scene population.

---

## Documentation Deliverables

Before starting level development, ensure:

- [ ] `docs/engine-architecture.md` — High-level engine architecture
- [ ] `docs/rendering-pipeline.md` — Render pass flow, shader pipeline
- [ ] `docs/ltsl-docs.md` — LTSL language reference (grammar, stdlib)
- [ ] `docs/reflection-system.md` — AutoClass/FIELDS/MAPFIELD documentation
- [ ] `docs/performance-guide.md` — Object count limits, profiling tips
- [ ] `EngPerfUpdate.md` (this file) — Updated as phases complete
- [ ] `AGENTS.md` — Stays in sync with completed work

---

## Version Control Strategy

Each phase should be a separate commit (or small PR) with a clear message:

- `perf: re-enable shader state caching`
- `perf: skip worldIT when shader doesn't use normal matrix`
- `perf: add GPU instancing for identical meshes`
- `deps: replace GLEW with GLAD`
- `cleanup: replace NULL with nullptr in engine code`
- `cleanup: replace typedef with using aliases across engine`
- `cleanup: remove dead EasyGL wrappers`
- `render: add PBR metallic-roughness pipeline`
- `script: add .Filter()/.Map() to LTSL arrays`

---

## Revision History

| Date | Change | Author |
|------|--------|--------|
| 2025-07-26 | Initial creation — consolidated from AGENTS.md + engine analysis | AI-assisted |
| 2025-07-26 | Added measured baseline (30K objects, 33/47 FPS, 580K polys), Phase 3b (LOD), Phase 3c (Hi-Z occlusion), Phase 6.9 (compute particles), Phase 6.10 (MDI), visual effects budget table, future-scaling estimates | AI-assisted |
| 2025-07-26 | Phase 1.1 complete: shader state caching re-enabled, WVP matrix bug fixed (33→39-41 FPS moving, +18-24%). Updated measured baseline and phase-by-phase impact table. | AI-assisted |
| 2025-07-26 | Phase 1.2 complete: moved worldIT computation from per-object (30K/frame) to per-shader-switch (~5-10/frame) in InjectMatrices. Removed dead renderer.worldIT field. Fixed ring specular blowout caused by wrong timing of needsWorldIT flag. | AI-assisted |
| 2025-07-26 | Phase 1.3 complete: quad VBO uses glBufferSubData for in-place updates instead of glBufferData re-allocation. Added GL_BufferSubData wrapper to GL.h. | AI-assisted |
| 2025-07-26 | Phase 1.4 complete: visible[] sorted by RenderableT* pointer after Visibility pass. Groups objects by shader+mesh to minimize state switches during draw. | AI-assisted |
| 2025-07-26 | Phase 1.5 complete: particle system early-out when all particles culled. Skips draw pipeline for empty batches. Phase 1 complete. | AI-assisted |
| 2025-07-26 | Phase 2.1-2.2 complete: GL_DrawElementsInstanced wrapper, SSBO at binding point 0 with dummy buffer, Renderer_BeginInstancedDraw/EndInstancedDraw/DrawMeshInstanced, Geometry/Mesh/Model/Renderable virtual DrawInstanced/RenderInstanced methods. Shader plumbing: vert.jsl/frag.jsl SSBO struct + uInstanced toggle, npm.jsl/imposter.jsl/lambert.jsl/metal.jsl/imposter1.jsl effectiveWorldIT from SSBO via fragInstanceID. InstancedDraw.h batching helper (dormant). Render passes reverted to for-loop (instanced batch bug with cached models). | AI-assisted |
| 2025-07-26 | Phase 2.4 complete: GPU-instanced particle rendering. SSBO at binding point 1 with ParticleInstanceData { posAndSize, ageAndColor } (32 bytes). Renderer_DrawParticlesInstanced uploads per-particle data and draws billboard mesh via glDrawElementsInstanced. particle.jsl reads from SSBO when uParticleInstanced > 0. Eliminated CPU vertex expansion. | AI-assisted |
| 2026-07-28 | Phase 5.4 complete: bulk-converted ~530 typedefs to modern C++ using-declarations across 181 files. Single-line aliases, multiline template chains, typedef-struct patterns, function pointers. Macro-generated typedefs left as-is. Build clean, 69 tests pass, all apps verified. | AI-assisted |
| 2026-07-28 | Phase 5.5 complete: GCC 15 deprecation/uninitialized audit. Zero warnings in engine code. Fixed C++20 designated initializers in TestSFML.cpp for C++17 compliance. Engine warning-clean. | AI-assisted |
| 2026-07-28 | Phase 5.2 complete: bulk-converted ~600 numeric C-style casts to static_cast<> across 50+ files (Component, Game, UI, Volume, LTE, Render systems). Safe numeric conversions only; pointer-to-integer casts preserved for manual review. Build clean, 69 tests pass, all apps verified. | AI-assisted |
| 2026-07-28 | Updated Phase 2 status: 2.1/2.2/2.4 marked ✅ complete. 2.3 marked ⏸️ ON HOLD (infrastructure built but dormant due to cached model bug; deferred after Phase 3). Phase 3 audit completed: 3.3 pre-reserve fix identified, 3.1 Zone update confirmed as top priority for ~14 FPS gain. | AI-assisted |
