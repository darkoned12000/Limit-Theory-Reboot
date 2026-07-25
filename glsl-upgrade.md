# GLSL Upgrade Plan

Current Version: `330 core` (via `src/liblt/LTE/Shader.cpp`)
Target Version: `460 core` (Staged)

## Benefits & Developer Experience

### Current Capabilities (GLSL 3.30)
- **Foundation**: Supports a complete programmable pipeline with custom vertex/fragment shaders.
- **Basic FX**: Capable of basic lighting, noise generation, and UI rendering.
- **Limitations**: High overhead for large object counts (no native instancing), limited global state management (individual uniforms), and restricted per-pixel complexity for heavy effects like volumetric fog.

### Future Capabilities (GLSL 4.6)
- **Scalability**: Geometry Instancing and Multi-Draw Indirect allow rendering thousands of asteroids/debris with minimal CPU overhead.
- **Complex Simulations**: Compute Shaders enable real-time procedural generation, fluid dynamics, and advanced volumetric effects that are currently too slow for the fragment stage.
- **Memory Efficiency**: SSBOs and Bindless Textures remove traditional uniform limits, allowing complex scenes to share massive amounts of data easily.

### Improved Developer Experience
The upgrade significantly lowers the friction for adding new visual features:
- **Easier State Management**: By using UBOs, developers no longer need to manually bind dozens of global uniforms; they are updated once per frame/pass.
- **Compute Pipelines**: Complex logic can be written in a more parallel-friendly "compute" mindset, which is often easier to debug and optimize than complex fragment shader loops.
- **Reduced Boilerplate**: Bindless textures and MDI reduce the amount of CPU-side "setup" code required per object, letting developers focus on the actual shader logic.

## Phase 1: Audit & Initial Jump (3.30 -> 4.0)

### Shader Audit
All `.jsl` files in `resource/shader/` will be scanned for:
- **Deprecated Functions**: Identification of any legacy functions that might change behavior or be removed in 4.x.
- **Precision Qualifiers**: Ensuring consistent precision across different hardware profiles.
- **Variable Limits**: Checking if complex shaders (e.g., `scattering.jsl`, `raytracing.jsl`) approach the max uniform/attribute limits of the current driver setup.

### Impact Analysis (Going to 4.0)
1. **Implicit Type Conversions**: GLSL 4.0 is stricter about types. We may need to explicitly cast floats where they were previously coerced.
2. **Uniform Layouts**: While not strictly required yet, we will prepare for `layout(binding = ...)` syntax to future-proof against bindless adoption.

### Engine Changes Needed
- Update `kVersionDirective` in `src/liblt/LTE/Shader.cpp`.
- Verify that the current SFML/OpenGL context initialization correctly requests a Core Profile 4.0+ context.

## Phase 2: Feature Adoption (4.0 -> 4.3)

### Key Upgrades
1. **Uniform Buffer Objects (UBOs)**:
   - Move global state (Time, Camera Matrices, Light Data) into UBOs.
   - Impact: Requires updating `Shader` class to handle buffer binding instead of individual uniform sets.
2. **Geometry Instancing**:
   - Implement for asteroid belts and starfields.
   - Impact: Modification of the renderer to use `glDrawElementsInstanced`.
3. **Shader Storage Buffer Objects (SSBOs)**:
   - Transition large data arrays (e.g., terrain heightmaps or complex particle state) to SSBOs.

## Phase 3: Advanced Optimization (4.3 -> 4.6)

### Key Upgrades
1. **Compute Shaders**:
   - Offload procedural noise generation and volumetric fog calculations to compute passes.
2. **Bindless Textures**:
   - Eliminate uniform limits for texture counts by using bindless handles.
3. **Multi-Draw Indirect (MDI)**:
   - Optimize the rendering of thousands of small objects (asteroids/debris) in a single draw call.

## Verification Strategy
- **Automated Compilation Tests**: Create a test runner that attempts to compile every `.jsl` file in `resource/shader/` using the engine's compilation pipeline. This will be run after every version jump to ensure no shader is broken by syntax changes or driver-specific strictness.
- **Visual Regression**: Compare renders of `ltheory-main` against a "Golden Master" set of screenshots for each GLSL version jump.
- **Error Logging**: Monitor `JSLPreprocess` and shader compilation logs for any warnings or "did you mean?" suggestions from the driver.

## Refactoring & Repair Milestones
Instead of jumping directly to 4.6, we will follow a "Bump, Repair, Refactor" cycle:

### Milestone A: Type Safety (After 4.0 Jump)
- **Task**: Identify and fix all implicit float/int conversion warnings.
- **Refactoring**: Update common utility shaders (`common/math.jsl`, `common/color.jsl`) to use explicit casting where necessary. Ensure consistent behavior across different GPU vendors.

### Milestone B: Data Pipeline (After 4.3 Jump)
- **Task**: Refactor the engine's Uniform management system.
- **Refactoring**: Replace individual uniform updates with **Uniform Buffer Objects (UBOs)** for global data (Time, View/Projection matrices). This is a major architectural change to `src/liblt/LTE/Shader.cpp` and the renderer.

### Milestone C: Compute Offloading (After 4.6 Jump)
- **Task**: Move heavy per-pixel calculations to Compute Shaders.
- **Refactoring**: Identify high-cost fragment shaders (e.g., `scattering.jsl`, `raytracing.jsl`) and refactor their logic into compute passes, using SSBOs to pass the results back to the fragment stage.
