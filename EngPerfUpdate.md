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

## Current Engine State (as of GLSL 4.60 / GLEW 2.3.1)

| Area | Status |
|------|--------|
| C++ standard | C++17, `-fno-exceptions` |
| GLSL version | 4.60 core (full range: 330→460) |
| OpenGL context | 4.6 (core profile bit off for Mesa compat) |
| SSBO / compute infra | Enum + wrappers present; no concrete use yet |
| SFML | 3.1.0 system-installed, C++17, miniaudio |
| GLEW | 2.3.1, built from source |
| Unit tests | 83 tests, 0 failures |
| Shader count | 170 `.jsl` files |
| Known perf bottleneck | No GPU instancing; 3000+ individual draw calls/frame |

---

## Phase 1: Quick Performance Wins (Est: 2-3 days)

Low-risk changes with immediate FPS impact. Zero architecture changes.
Each task is independently testable.

### 1.1 Re-enable Shader State Caching

**Impact: HIGH | Effort: 0.5 day | Risk: LOW**

`ShaderInstance::Begin()` has a state-caching check that is force-disabled
(`gSkippedState = false` with a `/* TODO : Fix state caching. */` comment).
Re-enabling this skips redundant `glUseProgram`, render state push/pop,
and uniform uploads for consecutive draws with the same shader.

- **File:** `src/liblt/LTE/ShaderInstance.cpp` (~line 185-187)
- **Fix:** Investigate why caching was disabled (likely a bug), fix the root
  cause, and re-enable `gSkippedState = true`.
- **Verify:** FPS counter before/after; draw call count should drop.
- **Test:** All apps run clean; visual output unchanged.

### 1.2 Conditional World Inverse-Transpose (worldIT)

**Impact: HIGH | Effort: 0.5 day | Risk: LOW**

`Renderer_SetWorldTransform()` (Renderer.cpp:895-902) unconditionally computes
`worldIT = world.Inverse().Transpose()` — a 48-multiply brute-force 4x4 matrix
inverse — for every visible object in every render pass. That's ~3000
inversions/frame with 1000 objects.

`worldIT` is only needed by shaders that do normal mapping. Most shaders
don't use it.

- **File:** `src/liblt/LTE/Renderer.cpp` (Renderer_SetWorldTransform)
- **Fix:** Check if the current shader has a `WORLDIT` uniform location ≥ 0
  before computing the inverse. If the shader doesn't use it, skip it.
  Also: for uniform-scale transforms (very common), `worldIT = world` (no
  inverse needed — just copy).
- **Verify:** FPS counter; `Renderer_GetDrawCallCount()` unchanged.
- **Test:** `ltheory-main`, `war` visually identical.

### 1.3 Replace GL_BufferData with glBufferSubData for Quads

**Impact: MEDIUM | Effort: 0.5 day | Risk: LOW**

`Renderer_DrawQuad()` (Renderer.cpp:517-552) re-uploads the same 48-byte
quad vertex buffer via `GL_BufferData(DynamicDraw)` 20-50 times per frame
(fullscreen post-process passes, lens flares, local lights). `glBufferData`
discards and re-allocates the entire driver-side buffer. `glBufferSubData`
does a partial update without allocation.

- **File:** `src/liblt/LTE/Renderer.cpp` (Renderer_DrawQuad)
- **Fix:** Create the quad VBO once at init with `GL_BufferData(DynamicDraw)`.
  On subsequent draws, use `glBufferSubData` to update the 48 bytes.
- **Verify:** Visual output identical; slight FPS improvement.

### 1.4 Sort Visible Objects by Material Before Draw Passes

**Impact: MEDIUM | Effort: 1 day | Risk: LOW**

Currently, the `visible[]` list is iterated in tree-traversal order. Each
object may use a different shader/mesh, causing GPU pipeline stalls on state
changes.

- **File:** `src/liblt/Game/Component/Drawable.cpp`, render pass files
  (`GBuffer.cpp`, `Blended.cpp`, `DepthPrepass.cpp`)
- **Fix:** Sort `visible[]` by a composite key: `(shader pointer, mesh pointer)`
  before iterating in each render pass. This minimizes shader switches and
  VBO binds.
- **Verify:** FPS counter; visual output identical.
- **Test:** All apps with 1000+ objects.

### 1.5 Early-Out in Particle System Draw

**Impact: LOW | Effort: 0.25 day | Risk: NONE**

`ParticleSystemImpl::Draw()` (ParticleSystem.cpp:75-142) rebuilds vertex
arrays per-shader-group even if all particles are culled. Add an early-out
after the visibility loop if `vertexCount == 0`.

- **File:** `src/liblt/Game/Graphics/ParticleSystem.cpp`
- **Fix:** After the frustum/LOD culling loop, check `if (vertexCount == 0)
  continue;` before the buffer upload.

---

## Phase 2: GPU Instancing (Est: 3-5 days)

The single biggest performance win. Replaces 3000+ individual draw calls
with ~10-20 instanced calls.

### 2.1 Add GL Draw Instanced Wrapper

- **File:** `src/liblt/LTE/GL.h`, `src/liblt/LTE/GLEnum.h`
- **Add:** `GL_DrawElementsInstanced(mode, count, type, indices, instanceCount)`
  wrapper. Add `GL_Instanced` to `GL_BufferTarget` if needed.
- **Add:** `GL_InstanceDivisor(index, divisor)` wrapper for setting per-instance
  attribute divisor.

### 2.2 Add Instanced Draw Path to Renderer

- **File:** `src/liblt/LTE/Renderer.cpp`, `src/liblt/LTE/Renderer.h`
- **Add:** `Renderer_DrawMeshInstanced(mesh, instanceCount, instanceBuffer)`
  that binds the per-instance data buffer and calls `GL_DrawElementsInstanced`.
- **Add:** Per-instance transform buffer management (one SSBO or VBO holding
  all instance model matrices or position+scale).

### 2.3 Batch Identical Objects in Visible List

- **File:** `src/liblt/Game/Component/Drawable.cpp`, render passes
- **Fix:** Group visible objects by `(mesh, shader)`. For each group, build
  a per-instance transform buffer and issue one instanced draw call.
- **Priority groups:**
  1. Asteroids (1000+ objects, same mesh) — biggest win
  2. Dust flecks (1024 billboards)
  3. Ships (if multiple share a hull mesh)

### 2.4 Update Particle System to Use GPU Instancing

- **File:** `src/liblt/Game/Graphics/ParticleSystem.cpp`
- **Fix:** Replace the CPU-side billboard vertex generation with a single
  billboard mesh drawn instanced, with per-instance position/size/age in
  a buffer. This eliminates the per-particle CPU vertex copy + full buffer
  re-upload.

---

## Phase 3: Object Lifecycle & Memory (Est: 2-3 days)

Reduce allocation churn and move expensive work out of the render path.

### 3.1 Move Zone Object Generation Out of Render Pass

**Impact: HIGH | Effort: 0.5 day | Risk: LOW**

`Zone::OnDraw()` (Zone.cpp:119-123) calls `field[i]->Update()` which
destroys/recreates ~3800 objects synchronously during the draw pass. This
must move to the update loop.

- **File:** `src/liblt/Game/Zone.cpp`
- **Fix:** Move `field[i]->Update()` calls from `OnDraw()` to `OnUpdate()`.
  If update timing is critical, defer creation across multiple frames
  (amortize: create N objects per frame instead of all at once).

### 3.2 Object Pooling for Frequent Create/Destroy

- **File:** New file: `src/liblt/LTE/ObjectPool.h`
- **Add:** A simple slab allocator for game objects. Pre-allocate pools
  for common types (asteroids, particles, projectiles). Avoids `new`/`delete`
  per-object churn.
- **Integrate:** Replace raw `new`/`delete` in hot-path object creation
  (Zone cells, particle systems) with pool allocation.

### 3.3 Pre-reserve Mesh Vectors

- **File:** `src/liblt/LTE/Mesh.cpp` (MeshT::AddMesh)
- **Fix:** Uncomment the `reserve()` calls at lines ~117 and ~125 to avoid
  vector reallocations during mesh construction.

### 3.4 Conditional Frustum Culling Improvements

- **File:** `src/liblt/Game/Component/Visibility.cpp`
- **Audit:** Check if hierarchical frustum culling (parent bounding volume
  culls entire subtree) is implemented. If not, add it — eliminates
  per-object frustum tests for children of off-screen parents.

---

## Phase 4: Dependency Modernization (Est: 1-2 days)

Low-risk library upgrades and replacements.

### 4.1 GLEW → GLAD

**Impact: LOW (code cleanliness) | Effort: 1 day | Risk: LOW**

Replace GLEW with GLAD for cleaner init (no `glewExperimental` quirk),
smaller binary (generate only GL 4.6 core functions), and self-contained
headers (no system dependency).

- **Generate:** Use GLAD web generator (https://glad.dav1d.de/) for
  GL 4.6 Core profile, C/C++ language.
- **Files to change:**
  - `src/liblt/LTE/GL.h` — `#include <glad/gl.h>` instead of `<GL/glew.h>`
  - `src/liblt/LTE/GLEnum.h` — same
  - `tests/TestTexture2D.cpp` — same
  - `src/launch/launch.cpp` — call `gladLoadGLLoader()` after context creation
    instead of `glewInit()`
  - `CMakeLists.txt` — add `glad.c` to sources, remove GLEW link
  - Remove `#define GLEW_STATIC` from all files
- **Add to build:** `thirdparty/glad/src/glad.c`, `thirdparty/glad/include/glad/gl.h`

### 4.2 UTF8-CPP Upgrade

**Impact: LOW | Effort: 0.5 day | Risk: LOW**

Update vendored `include/UTF8` from the current ancient release to a current
release. Library is 12+ years old.

- **Source:** https://github.com/nemtrif/utfcpp/releases
- **Files:** Replace `include/UTF8/*` with latest headers.
- **Verify:** UTF-8 string operations still work (font rendering, UI text).

### 4.3 Git LFS for Large Resources

- **Fix:** Add `.gitattributes` tracking large binary files (textures,
  fonts, FMOD binaries) via LFS.

---

## Phase 5: Code Cleanup & Modernization (Est: 3-5 days)

Reduce technical debt, improve maintainability.

### 5.1 Delete Dead EasyGL Wrappers

- **File:** `src/liblt/LTE/GL.h`
- **Remove:** `GL_Begin`, `GL_End`, `GL_Vertex`, `GL_VertexPointer`,
  `GL_NormalPointer`, `GL_TexCoordPointer`, `GL_Color`, `GL_LoadIdentity`,
  `GL_LoadMatrix`, `GL_MatrixMode`, `GL_MultMatrix`, `GL_PushMatrix`,
  `GL_PopMatrix`, `GL_Ortho`, `GL_TexCoord`, `GL_TexBaseLevel`
- **Verify:** Full build with `-Werror`; grep for any remaining references.
- **Note:** Do NOT mark as Revamp Work (original code removal).

### 5.2 C++ Cast Cleanup

- **Scope:** ~1094 C-style casts across `src/liblt/`
- **Fix:** Replace with `static_cast`/`reinterpret_cast` where safe.
  Exclude intentional `*(T const*)0` idioms (centralized in `Type_Ref<T>()`).
- **Approach:** Subsystem-by-subsystem. Apply clang-tidy
  `cppcoreguidelines-pro-type-cstyle-cast` per-dir, review diffs.

### 5.3 NULL → nullptr Review

- **Scope:** ~41 remaining `NULL` usages
- **Fix:** Manual review and replacement. Cannot auto-fix because `0` is
  overloaded as both null-pointer and integer-zero in this codebase.

### 5.4 typedef → using Cleanup

- **Scope:** ~539 `typedef` vs 7 `using`
- **Fix:** Mechanical conversion where safe. Low priority.

### 5.5 `-Wmaybe-uninitialized` / Deprecation Audit

- **Scope:** GCC 15 strict warnings
- **Fix:** Fix at source, not via suppression. Audit one subsystem at a time.

### 5.6 Delete Vendored ext/SFML/ Directory

- **Scope:** `ext/SFML/` is legacy (2.6.2, no longer built). System SFML 3.1
  is used via `find_package`.
- **Fix:** Remove the directory to reduce repo size. Update `.gitignore` if
  needed.

---

## Phase 6: Visual Expansion (Est: 2-3 weeks)

Rendering improvements. Requires Phase 1-2 for CPU headroom.

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
| GLEW | 2.3.1 | GLAD (latest) | 4.1 | 1 day |
| UTF8-CPP | vendored ~12yr old | Latest release | 4.2 | 0.5 day |
| ext/SFML/ | 2.6.2 (vendored, unused) | Delete directory | 5.6 | 0.25 day |

---

## Task Priority Matrix

| Priority | Tasks | Estimated Impact |
|----------|-------|-----------------|
| **P0 — Do First** | 1.1 State caching, 1.2 Conditional worldIT, 1.3 glBufferSubData | Immediate FPS gain, zero risk |
| **P1 — High Value** | 2.1-2.4 GPU instancing, 3.1 Zone fix | 3-10x FPS with many objects |
| **P2 — Medium Value** | 1.4 Sort by material, 3.2 Object pooling, 3.4 Frustum culling | Steady improvement |
| **P3 — Foundation** | 4.1 GLAD, 4.2 UTF8-CPP, 5.1 EasyGL cleanup, 5.2 Cast cleanup | Code health |
| **P4 — Visuals** | 6.1 PBR, 6.2 Directional light, 6.3 Atmosphere | Visual quality leap |
| **P5 — Tooling** | 7.1 LTSL HOF, 7.2 Hot-reload, 7.4 DevPanel | Developer productivity |
| **P6 — Quality** | 8.1 CI, 8.2 Visual regression, 8.3 Benchmarking | Long-term stability |

---

## 60 FPS Target Strategy

The engine currently runs at ~30 FPS with 1000 asteroids. To hit a
consistent 60 FPS regardless of object count:

1. **Phase 1** (quick wins): Expected ~10-20% FPS improvement from state
   caching, conditional worldIT, and buffer optimization.
2. **Phase 2** (instancing): Expected 3-10x FPS improvement. This is the
   critical path — drops 3000+ draw calls to ~10-20.
3. **Phase 3** (object lifecycle): Eliminates frame-time spikes from Zone
   cell changes. Smooths out hitches.
4. **Phase 6** (visuals): PBR and atmospheric scattering add GPU work but
   benefit from the CPU headroom gained in Phases 1-3.

**Minimum viable path to 60 FPS:** Phases 1 + 2 = ~5 days of work.
**Full 60 FPS with visuals:** Phases 1-3 + 6.1-6.4 = ~3-4 weeks.

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
- `cleanup: remove dead EasyGL wrappers`
- `render: add PBR metallic-roughness pipeline`
- `script: add .Filter()/.Map() to LTSL arrays`

---

## Revision History

| Date | Change | Author |
|------|--------|--------|
| 2025-07-26 | Initial creation — consolidated from AGENTS.md + engine analysis | AI-assisted |
