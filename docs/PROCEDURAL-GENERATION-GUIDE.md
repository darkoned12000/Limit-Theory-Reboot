# Procedural Generation Deep-Dive — How Ships, Stations, & Planets Are Built

**Date:** 2026-07-30  
**Purpose:** Complete technical breakdown of procedural mesh generation from first principles

---

## Executive Summary

Limit Theory uses **THREE different procedural generation systems**:

1. **SDFs (Signed Distance Functions)** → Asteroids, stations (implicit surfaces → mesh)
2. **PlateMesh** → Ships (plate-based hull construction)
3. **Shader-Based** → Planet surfaces, nebulae (GPU-generated textures)

**All generation is 100% engine-driven** — NO external asset pipeline. Every ship, asteroid, and planet is generated from a seed at runtime.

**This is Josh's secret sauce** — pure algorithmic art.

---

## Part 1: How SDFs Work (Asteroids & Stations)

### What is an SDF?

**SDF (Signed Distance Function):** A function that returns the **distance from any point in 3D space to the nearest surface**.

```
f(x, y, z) → distance
  positive = outside surface
  zero = on surface
  negative = inside surface
```

**Example: Sphere SDF**

```cpp
float sphere_sdf(Vec3 p, Vec3 center, float radius) {
  return Length(p - center) - radius;
}
```

**At point (2, 0, 0):**
- Center: (0, 0, 0)
- Radius: 1.0
- Distance: `sqrt(4) - 1 = 2 - 1 = 1.0` (outside)

---

### SDF Operations (Boolean)

**Union (OR):**
```cpp
SDF sphere1 = SDF_Sphere(Vec3(0), 1.0);
SDF sphere2 = SDF_Sphere(Vec3(2, 0, 0), 0.5);
SDF combined = sphere1->Union(sphere2);
// Result: Two spheres merged
```

**Subtraction (Difference):**
```cpp
SDF box = SDF_Box(Vec3(0), Vec3(2, 2, 2));
SDF sphere = SDF_Sphere(Vec3(0), 1.5);
SDF result = box->Subtract(sphere);
// Result: Box with sphere carved out (hollow)
```

**Intersection (AND):**
```cpp
SDF result = sphere1->Intersect(sphere2);
// Result: Only the overlapping region
```

---

### How Asteroids Use SDFs

**File:** `src/liblt/Game/Renderable/Asteroid.cpp`

```cpp
SDF d = SDF_Radial(
  SDF_FractalWorley(Rand(1, 1000), 6, 2.6f),  // 6 octaves, 2.6x frequency
  0.0f,  // Minimum radius
  2.0f   // Maximum radius
);

Renderable asteroid = Model_Create()
  ->Add(SDFMesh_Create(d), Material_Rock());
```

**Breakdown:**

1. **`SDF_FractalWorley`** — 3D Worley noise (cellular/voronoi pattern)
   - 6 octaves → Multiple levels of detail
   - 2.6x frequency multiplier → Sharp features
   - Seed → Deterministic randomness

2. **`SDF_Radial`** — Wraps noise in spherical domain
   - 0.0 min, 2.0 max → Sphere with bumpy surface

3. **`SDFMesh_Create`** — Converts SDF to triangle mesh
   - Marching cubes algorithm (samples SDF grid, finds isosurface)
   - Output: ~500-2000 triangles per asteroid

4. **`Material_Rock`** — Applies rock texture/shader

**Result:** Lumpy, irregular asteroid with seeded shape.

---

### How Stations Use SDFs

**File:** `src/liblt/Game/Item/StationType.cpp`

```cpp
SDF interior = SDF_Shell(0, 1, 0.1f)  // Hollow sphere
  ->Subtract(SDF_Cylinder(0, V3(0, 0, 1), 0.1f));  // Carve docking port

// Add spokes
for (int i = 0; i < 8; ++i) {
  float angle = i * (2.0f * PI / 8.0f);
  V3 direction(cos(angle), sin(angle), 0);
  SDF spoke = SDF_Box(direction * 2.0f, V3(0.2, 0.2, 1.0));
  interior = interior->Union(spoke);
}

Mesh mesh = SDFMesh_Create(interior);
```

**Result:** Hollow sphere with 8 spokes and docking cylinder (classic space station).

---

## Part 2: How Ships Use PlateMesh (Plate-Based Hulls)

### What is PlateMesh?

**PlateMesh:** Additive system where **rectangular plates** are snapped together to form hull.

**Algorithm:**
1. Start with 2 core boxes (cockpit + engine)
2. Randomly attach plates to sides of existing boxes
3. Repeat N times (N = complexity budget from seed)
4. Apply **warps** (deformations like vertical compress, pinch, expand)
5. Generate triangle mesh from plate structure

**File:** `resource/script/Item/ShipType/Generate.lts`

---

### Step-by-Step Ship Generation

**Step 1: Create PlateMesh**

```lts
var self (PlateMesh_Create 12)  # Quality: 12 (triangle density)
```

**Step 2: Add Initial Core Boxes**

```lts
var boxes List
boxes += (Box (Vec3 0 0 0) (Vec3 2.0 0.5 0.5))  # Cockpit (flat, wide)
boxes += (Box (Vec3 0 0 0) (Vec3 0.5 1.0 2.0))  # Engine block (tall, long)
```

**Step 3: Grow Random Plates**

```lts
var plates 2 + (Int (Sqrt scale))  # More plates for bigger ships

for i 0 plates
  var box (boxes.Get (rng.Int boxes.Size))  # Pick random existing box
  var dir (rng.GetAxis)  # Random cardinal direction (-X, +X, -Y, +Y, -Z, +Z)
  var newSize (box.size * (rng.Vec3 0.9 0.99))  # Slightly smaller
  var newCenter (box.center + dir * (box.size + newSize))  # Snap to side
  
  boxes += (Box newCenter newSize)  # Add new box
```

**Visual Result (after 10 iterations):**

```
        [====]  ← New plate
[====]  [====]  ← Core boxes
  [====]        ← New plate
```

**Step 4: Add Beveled Plates to Mesh**

```lts
var kBevel 0.25  # Rounded corners

for i 0 boxes.Size
  var box (boxes.Get i)
  self.Add box.center box.size 0 kBevel  # Add beveled box
```

**Step 5: Apply Deformations (Warps)**

```lts
# Vertical compress (flatten hull near center)
self.Add (Warp_Custom (VerticalCompress 1.0 0.0 1.25 1.0 1.0))

# Horizontal expand (widen hull at rear)
self.Add (Warp_Custom (HExpand 0.5 1.0 0.0 2.0 0.25))
```

**Visual Result:**

```
Before:    [====]
           [====]
           [====]

After:     [=======]  ← Wider
           [=====]    ← Flatter
           [=======]  ← Wider
```

**Step 6: Convert to Triangle Mesh**

```lts
var mesh (self.GetMesh)
mesh.Center  # Center at origin
mesh.Mesh_ComputeOcclusion  # Bake ambient occlusion
```

**Output:** ~1000-5000 triangles depending on plate count.

---

### Why PlateMesh vs SDF?

| System | Pros | Cons | Best For |
|--------|------|------|----------|
| **SDF** | Organic shapes, smooth blending, boolean ops | Expensive meshing (marching cubes), hard to control | Asteroids, stations (irregular shapes) |
| **PlateMesh** | Fast, geometric hulls, easy to control | Blocky (not organic), limited to boxes | Ships (hard-surface sci-fi) |

---

## Part 3: Shader-Based Generation (Planets)

### Planet Surface Generation

**File:** `resource/shader/fragment/gen/planet.jsl`

**Process:**

1. **GPU compute shader** generates 4-channel texture (height, color, clouds, spare)
2. Render to cubemap (6 faces)
3. Apply to sphere mesh in planet shader

**Algorithm (GLSL):**

```glsl
uniform float seed;
uniform vec4 coef;   // Fractal coefficients
uniform float freq;  // Frequency
uniform float power; // Power curve

float genHeight(vec3 p) {
  vec4 z = vec4(p / 4.0 + 0.75, 0.3);
  float a = 0.0, l = 0.0, w = 1.0;
  
  // 32 iterations of domain-warped fractal
  for (int i = 0; i < 32; ++i) {
    float m = dot(z, z);
    z = abs(z) / m - vec4(0.4, 0.5, 0.6, 0.3);  // Mandelbox-like
    z += 0.1 * log(1.e-10 + noise4(float(i) + seed));  // Noise warp
    z *= 1.0 + 0.25 * noise(float(i) + seed * 2.0);
    z = z.yzwx;  // Swizzle (rotate components)
    
    m = coef.x*z.x*z.x + coef.y*z.y*z.y + 
        coef.z*z.z*z.z + coef.w*z.w*z.w;
    a += w * exp(-abs(m - l));  // Accumulate
    w *= 0.8;  // Decay weight
    l = m;
  }
  
  return gain(pow(0.5 + 0.5 * sin(freq * a), power), 4.0);
}
```

**What This Does:**

- **Domain warping** — Distorts noise coordinates → Sharp features
- **Mandelbox-like iteration** — `abs(z) / m` → Creates ridges
- **32 octaves** — Multiple scales of detail
- **Seeded** — `noise4(float(i) + seed)` → Deterministic

**Result:** Alien fractal terrain (height map).

---

### Why Shader-Based for Planets?

| Pros | Cons |
|------|------|
| ✅ GPU-accelerated (10-50ms) | ❌ Hard to control specific features |
| ✅ Infinite detail (no mesh limit) | ❌ Requires GLSL knowledge |
| ✅ Seamless (cubemap wraps perfectly) | ❌ Can't export to traditional 3D tools |

---

## Part 4: From Assets vs All-Engine (Your Question)

### Current System: 100% Engine-Generated

**NO external assets are used** for procedural content:

- **Ships:** PlateMesh algorithm (plates + warps + seed) → Mesh at runtime
- **Asteroids:** SDF fractal noise → Marching cubes → Mesh at runtime
- **Planets:** Shader fractal noise → Cubemap texture at runtime
- **Stations:** SDF boolean ops (shell, cylinders, spokes) → Mesh at runtime

**Advantages:**
- ✅ Zero asset storage (1 seed = infinite ships)
- ✅ Consistent art style (all from same algorithm)
- ✅ Fast iteration (change seed → new ship instantly)

**Disadvantages:**
- ❌ Hard to make specific shapes (e.g., "X-wing fighter")
- ❌ All ships look "algorithmic" (same geometric style)
- ❌ Limited artist control (can't hand-paint textures)

---

### Hybrid Approach: Mix Procedural + Assets

**Idea:** Use procedural base + artist-authored details.

**Example 1: Procedural Hull + Asset Decals**

```cpp
// Generate base ship hull (PlateMesh)
Mesh hull = GenerateHullFromSeed(12345);

// Apply artist-made decal texture
Texture2D decal = Texture_Load("decals/faction_logo.png");
Material material = Material_Metal();
material->SetTexture("decal", decal);

// Result: Procedural hull with hand-painted logo
```

**Example 2: Procedural Asteroids + Asset Craters**

```cpp
// Generate asteroid base shape (SDF)
SDF asteroid = SDF_FractalWorley(seed, 6, 2.6f);

// Subtract pre-made crater SDF
SDF crater = SDF_LoadFromMesh("assets/crater_01.obj");
asteroid = asteroid->Subtract(crater->Translate(craterPos));

// Result: Procedural asteroid with realistic crater detail
```

**Example 3: Procedural Planets + Asset Normal Maps**

```glsl
// Planet shader
vec3 baseColor = generateProceduralColor(uv, seed);  // Procedural
vec3 normal = texture(normalMap, uv).rgb;  // Artist-made normal map
vec3 detail = texture(detailMap, uv).rgb;  // Artist-made detail (cracks, etc.)

vec3 finalColor = baseColor * detail;  // Blend
```

---

### How to Add Asset Support

**Step 1: Import Mesh as SDF Template**

```cpp
// Load .obj file
Mesh mesh = Mesh_LoadOBJ("assets/fighter_cockpit.obj");

// Convert to SDF (voxelize, then distance field)
SDF cockpitTemplate = SDF_FromMesh(mesh, resolution=64);

// Use in ship generation
SDF hull = GenerateProceduralHull(seed);
SDF finalShip = hull->Union(cockpitTemplate->Translate(Vec3(0, 0, 5)));
```

**Step 2: Use Assets as "Kits"**

```lts
# Ship generation with asset kits
function Renderable Generate (Int seed)
  var rng (RNG_MTG seed)
  var hull (GenerateHullFromSeed seed)
  
  # Attach artist-made cockpit (3 variants)
  var cockpitVariant (rng.Int 0 2)
  var cockpit (Mesh_Load ("assets/cockpit_" + (String cockpitVariant) + ".obj"))
  hull.Attach cockpit (Vec3 0 0 5)
  
  # Attach wings (6 variants)
  var wingVariant (rng.Int 0 5)
  var wings (Mesh_Load ("assets/wings_" + (String wingVariant) + ".obj"))
  hull.Attach wings (Vec3 0)
  
  # Procedural color/texture
  var material (GenerateProceduralMaterial rng)
  hull.SetMaterial material
  
  Renderable hull
```

**Result:** Procedural variation + artist-quality details.

---

## Part 5: Creating New Procedural Types

### Example: Procedural Space Debris

**Goal:** Generate broken ship parts floating in space.

**Step 1: Create SDF Fragment**

```cpp
// In src/liblt/Game/Renderable/Debris.cpp
SDF GenerateDebris(int seed) {
  RNG rng(seed);
  
  // Start with random box
  V3 size = rng.GetVec3(1.0f, 5.0f);
  SDF base = SDF_Box(V3(0), size);
  
  // Carve random chunks (5-10 holes)
  for (int i = 0; i < rng.GetInt(5, 10); ++i) {
    V3 holePos = rng.GetVec3(-size, size);
    float holeRadius = rng.GetFloat(0.5f, 2.0f);
    SDF hole = SDF_Sphere(holePos, holeRadius);
    base = base->Subtract(hole);
  }
  
  // Add damage (cracks)
  SDF crack = SDF_Cylinder(V3(0), rng.GetDirection(), 0.1f);
  base = base->Subtract(crack);
  
  return base;
}
```

**Step 2: Wire into LTSL**

```cpp
// In Module/ScriptAPI/API_Object.cpp
FreeFunctionNoPtr(Object_Debris,
  "Object", "Debris",
  "seed:int")

ObjectT* Object_Debris(int seed) {
  SDF debrisSDF = GenerateDebris(seed);
  Mesh mesh = SDFMesh_Create(debrisSDF);
  Renderable model = Model_Create()->Add(mesh, Material_Metal());
  
  ObjectT* debris = new ObjectT();
  debris->SetRenderable(model);
  return debris;
}
```

**Step 3: Use in LTSL**

```lts
# Spawn debris field
for i 0 100
  var debris (Object_Debris (rng.Int))
  debris.SetPos (rng.GetUniform 10000.0 50000.0) * rng.Direction
  debris.SetRotation (rng.Direction)
  system.AddInterior debris
```

**Result:** 100 unique debris pieces, all seeded.

---

## Part 6: Advanced: Mesh Kitbashing System

**Goal:** Mix procedural + asset parts like Lego blocks.

**Architecture:**

```
MeshKit (collection of parts)
  ├─ Cockpit (10 variants)
  ├─ Wings (20 variants)
  ├─ Engines (15 variants)
  └─ Weapons (30 variants)

Ship Generator:
  1. Generate base hull (PlateMesh)
  2. Attach random cockpit from kit
  3. Attach random wings (symmetry)
  4. Attach random engines (rear)
  5. Attach weapon hardpoints
```

**Implementation:**

```cpp
// MeshKit.h
struct MeshKit {
  Vector<Mesh> cockpits;
  Vector<Mesh> wings;
  Vector<Mesh> engines;
  
  void LoadFromDirectory(String path) {
    cockpits = Mesh_LoadAll(path + "/cockpits/");
    wings = Mesh_LoadAll(path + "/wings/");
    engines = Mesh_LoadAll(path + "/engines/");
  }
};

// ShipGenerator.cpp
Mesh GenerateShip(int seed, MeshKit const& kit) {
  RNG rng(seed);
  Mesh hull = GenerateProceduralHull(seed);
  
  // Attach parts
  Mesh cockpit = kit.cockpits[rng.GetInt(0, kit.cockpits.size())];
  hull.Attach(cockpit, V3(0, 0, 5), Quat::Identity);
  
  Mesh wingLeft = kit.wings[rng.GetInt(0, kit.wings.size())];
  hull.Attach(wingLeft, V3(-3, 0, 0), Quat::Identity);
  hull.Attach(wingLeft->Mirror(V3(1, 0, 0)), V3(3, 0, 0), Quat::Identity);
  
  // ... engines, weapons, etc.
  
  return hull;
}
```

**Result:** Infinite ship variety from 50-100 artist-made parts.

---

## Part 7: Performance Considerations

### Mesh Generation Cost

| Type | Algorithm | Triangles | Generation Time |
|------|-----------|-----------|-----------------|
| Asteroid | SDF + Marching Cubes | 500-2000 | 5-20ms |
| Ship | PlateMesh | 1000-5000 | 2-10ms |
| Station | SDF + Marching Cubes | 2000-10000 | 10-50ms |
| Planet Surface | Shader (GPU) | N/A (texture) | 20-100ms |

**Best Practice:** Generate meshes on **loading screen**, not mid-gameplay.

---

### Caching Strategy

```cpp
// Cache generated meshes by seed
std::unordered_map<int, Mesh> shipCache;

Mesh GetShipMesh(int seed) {
  auto it = shipCache.find(seed);
  if (it != shipCache.end())
    return it->second;  // Cache hit
  
  Mesh mesh = GenerateShipFromSeed(seed);  // Cache miss
  shipCache[seed] = mesh;
  return mesh;
}
```

**Result:** First asteroid of seed 12345 takes 10ms, subsequent ones take 0ms (cache hit).

---

## Part 8: Recommended Learning Path

**Week 1: Understand SDFs**
- Read [Inigo Quilez's SDF articles](https://iquilezles.org/articles/distfunctions/)
- Experiment with `SDF_Sphere`, `SDF_Box`, `SDF_Union`, `SDF_Subtract` in C++
- Generate 10 asteroid variants, screenshot them

**Week 2: Master PlateMesh**
- Read `resource/script/Item/ShipType/Generate.lts` line-by-line
- Modify plate count, bevel amount, warp parameters
- Generate 20 ship hulls, find aesthetic patterns

**Week 3: Shader Generation**
- Read `resource/shader/fragment/gen/planet.jsl`
- Tweak `freq`, `power`, `coef` parameters
- Generate 10 unique planet surfaces

**Week 4: Hybrid System**
- Create 5 artist-made cockpit meshes (.obj files)
- Write kitbashing system (attach cockpits to procedural hulls)
- Generate 50 ships from 5 cockpits × 10 hull seeds

---

## Summary

**What Josh Built:**
- ✅ SDF-based asteroid/station generator (marching cubes)
- ✅ PlateMesh ship hull generator (plate snapping + warps)
- ✅ Shader-based planet surface generator (GPU fractals)
- ✅ 100% engine-driven (zero asset pipeline)

**What You Can Add:**
- 🎨 Artist-made detail assets (cockpits, wings, decals)
- 🔧 Kitbashing system (mix procedural + assets)
- 🎲 More SDF primitives (torus, pyramid, etc.)
- 🌈 Procedural textures (wear, paint schemes, faction logos)

**Recommended Path:**
- **Short-term:** Master existing systems (tweakseed params)
- **Medium-term:** Add asset support (hybrid procedural + kits)
- **Long-term:** Extend SDF library (new shapes, operations)

**The engine is a blank canvas.** Josh gave you the brushes (SDFs, PlateMesh, shaders). Now paint your universe! 🎨🚀
