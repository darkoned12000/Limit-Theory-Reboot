// Copyright (C) 2025  darkoned12000
// SPDX-License-Identifier: GPL-3.0-or-later
// Part of the ltheory-old-test modernization effort (Revamp Work).
// See NOTICE and LICENSE.GPL. Original engine (c) Josh Parnell, public domain.

# Review: planets.json — first JSON content database

Review of `resource/gamedata/planets.json` + the C++ consumption path
(`Item_PlanetType`, `DatabaseManager`/`JsonDatabase`) as the base for all
future gamedata files. Verdict up front: **the foundation is solid** — typed,
defaulted, clamped, validated, deterministic. The issues below are polish and
a few structural things worth fixing *now* while planets is still the only
consumer, because every later database (ships, weapons, stations) will copy
these patterns.

## 1. What's already good (keep as the house pattern)

- **Typed reader with defaults + clamps** — `JGet/JRange/JColor3/JBiome` in
  `PlanetType.cpp` means a missing field falls back to a sane value instead of
  crashing or going NaN. This is exactly the "default fallbacks" risk item from
  the design doc (§7) done right. Every future loader should do this, not raw
  `json["field"]`.
- **Path-based error reporting** — `"planets.json: biomes.lava.colorPalette[2]
  expected array"` is the format all loaders should use (file + JSON path).
- **Determinism preserved** — biome pick = `seed % numBiomes`, everything else
  from one `RNG_MTG(seed)`. Same seed → same planet, and the diagnostic line
  (`Item_PlanetType(seed=...) biome=...`) makes it verifiable. Keep this
  invariant for all databases: JSON adds *content*, never new nondeterminism.
- **`_docs` block in the file** — self-documenting data is great for modders;
  keep the convention (underscore prefix = ignored by loaders).
- **Single source of truth** — `PlanetType.cpp` no longer has a second
  hardcoded palette next to the JSON one. Good "one way to do things."

## 2. The RGB question (0–1 vs 0–255)

Short answer: **keep 0..1 floats as the engine-internal representation, but
make authoring comfortable by accepting color strings.** Don't switch the file
format to raw 0–255 ints.

Reasons:

1. Every consumer of these values is GLSL (`V3F` uniforms), where 0..1 *is*
   the native format. Storing 0–255 means every load does `/255.0f` — trivial,
   but it also invites precision questions (256 steps vs float) and makes the
   JSON a different scale than every other number in the file.
2. More importantly, **0..1 decimals are genuinely hard to eyeball.**
   `[0.569, 0.404, 0.247]` (the gas giant tint) is unreadable without
   converting. That's the real pain, and the fix isn't "use ints" — it's
   **hex strings**.

Recommended color format for all gamedata (add to the JSON layer core, not
just planets):

```json
"surfaceTint": "#FF8C00"              // hex → (1.0, 0.549, 0.0)
"colorPalette": ["#D9A05B", "#BF8C3F", "#B38033", "#A67326"]
"wavelengthBase": [0.66, 0.53, 0.4]   // NOT a color — keep as float array
```

- `JColor3` accepts **both** `"#RRGGBB"` / `"#RRGGBBAA"` strings and a
  `[r,g,b]` float array (existing files keep working). Parse once at load.
- This is what Unity/Unreal mod formats do: hex in data, floats in the engine.
- Optional later nicety: support `hsl(h,s,l)` strings for tints where hue
  matters more than exact RGB.

Implementation sketch (add to the shared JSON layer, e.g. next to
`JsonDatabase`):

```cpp
// Parse "#RRGGBB" or [r,g,b] → V3F (0..1). Reports path on bad input.
bool JColor(json const* v, String const& path, V3F& out);
```

Then `JColor3` in `PlanetType.cpp` becomes a thin wrapper over it, and the
same helper is reused by ships/weapons/graphics (nebula colors, star color,
ocean color — all of §3.6 has this exact problem).

**Do NOT** do "if value > 1.0 assume 0–255" auto-detection — `1.5` becomes
ambiguous and it silently changes meaning when someone edits a file.

## 3. Schema structure issues (fix while only one consumer exists)

### 3.1 Biomes are keyed by id, but the picker is positional

`seed % numBiomes` indexes the biome *map in iteration order*. nlohmann::json
objects preserve insertion order, so this works today — but it means **adding
a new biome at the top of the file silently re-assigns every seed's biome.**
A save created with 5 biomes loads a different planet type after a content
edit.

Fix options (pick one, document it):

- **Explicit order array**: `"biomeOrder": ["desert","terran","ice","lava","gas_giant"]`
  — picker indexes this. Adding a biome appends; existing seeds are stable.
  (Recommended — cheap and obvious.)
- Or hash the key: `hash(biomeKey) % something` — stable but opaque and
  clumps unevenly.

Either way, write it into `_docs.biome_selection` so modders know the rule.

### 3.2 Two layers of "range" semantics are confusing

Right now a biome can have either:
- `cloudLevel: -0.1` (fixed) **and/or**
- `cloudLevelRange: [min,max]` (random, overrides)

…while `atmoDensityRange` is *always* a range even for fixed-feeling biomes.
Two different field names encoding "how much randomness" is a trap for the
next person editing this. Suggest converging on **one** convention:

```json
"atmoDensity":  [0.2, 0.8]     // always [min,max]; use [x,x] for fixed
"cloudLevel":   [-0.2, 0.15]
```

i.e. drop the singular/plural pair; a single value or equal-range means
fixed. (If you want to keep the current names for backward compat, at minimum
document that `*Range` overrides the fixed field — and note it's planets-only
until ships/weapons force a decision.)

### 3.3 `wavelengthJitter` is a range, not a jitter

`"wavelengthJitter": [-0.1, 0.1]` reads as "±0.1", but it's stored/consumed as
a `[min,max]` range. If the convention from §3.2 lands, this becomes
`"wavelengthRange"` or stays a single `jitter: 0.1` scalar (symmetric ±).
A scalar is simpler for jitter specifically — pick one.

### 3.4 `oceanLevel` / `rotationSpeed` / `cloudWindSpeed` are dead fields

The `_docs` honestly says "informational only (shader wiring pending)" and the
code confirms it (`PlanetType.cpp:288` `(void)oceanVal; // TODO`). Two problems:

1. **Docs lie to modders.** Someone sets `oceanLevel: 0.9`, relaunches, sees
   nothing, files a bug. Mark dead fields explicitly in the file itself:
   move them under a top-level `"pending": { ... }` or prefix with `_`
   (`_rotationSpeed`) so it's obvious they're not active. Underscore-prefix is
   already your "ignored" convention from `_docs`.
2. **The design doc (§3.11/§3.12) says rotation/cloud drift is Phase B** — a
   shader change with a clear plan. That's fine, but the JSON should not
   advertise knobs that do nothing. Either wire `oceanLevel` first (it's the
   cheapest: it already exists as a planet field, `Planet.h:44`, and the TODO
   at `PlanetType.cpp:288` is literally the wiring point) or mark it pending.

**Wiring `oceanLevel` now is the highest-value small win on this list.** The
field exists, the JSON read exists, the only missing piece is passing it to the
surface generator / shader threshold. It makes one of the five biomes (terran)
actually *do* something different from what it does today, which validates the
whole "edit JSON → see change" loop for the flagship biome.

### 3.5 `defaults` vs biome fields: document the merge rule

The code merges defaults ← biome (biome wins), but the file doesn't say so.
Add one line to `_docs`: *"Any field present on a biome overrides defaults;
fields absent everywhere use the C++ built-in fallback."* Also consider moving
the C++-side final fallbacks into `defaults` so **one** place holds every
default (right now the real defaults live in `PlanetType.cpp`'s reader calls,
and `defaults` in JSON is a second copy — they can drift).

### 3.6 `moons.moonScaleFormula` is a formula *string*

`"moonScaleFormula": "Pow(mass / 1000, 0.5)"` — a string expression in a data
file. This starts an eval-engine scope hole (the design doc's `countFormula:
"2 * (logScale / log10)"` for ships does the same). For moons it's simpler to
store the parameters and let C++ own the formula:

```json
"moons": { "sizeExponent": 0.5, "sizeDivisor": 1000 }
```

If you later genuinely need data-driven formulas, make it one deliberate,
versioned mini-language — don't let it sneak in field by field.

## 4. Implementation / engine improvements

Ordered by value-to-effort:

1. **`JColor` hex support** (§2) — add to the shared JSON layer now; every
   later database benefits. ~30 lines + tests.
2. **Wire `oceanLevel`** (§3.4) — one TODO already marked at the call site.
   Unblocks the terran biome visually and proves the loop end-to-end.
3. **Biome order stability** (§3.1) — `biomeOrder` array, or document + test
   the insertion-order behavior. Add a unit test: "adding a biome does not
   change existing seeds' picks."
4. **Schema validation pass on load** — the design doc's Phase 4 item. With
   planets as the only DB today, it's cheap to build the habit: walk the file,
   check types of known fields (`colorPalette` = array of 4 arrays of 3
   numbers), report `file + path`. nlohmann gives you `is_array()`/
   `is_number()` for free. This is what makes modding safe (a typo'd
   `"surfaceTint": "red"` should say so at load, not silently fall back to
   the default).
5. **Move final fallbacks into JSON `defaults`** (§3.5) — C++ keeps only the
   "file missing/corrupt" emergency defaults (the design doc's §7 backward-
   compat item already assumes this shape).
6. **Unit tests for the planets loader specifically** — `TestJsonDatabase.cpp`
   covers the generic layer; add cases that load the *real*
   `resource/gamedata/planets.json`: all biomes parse, every color in 0..1,
   ranges have min<=max, biome count matches `biomeOrder`. This is the
   regression guard for "schema drift" (§7 risk table) and it's what makes a
   modder's broken file fail loudly.
7. **Hot-reload note** — design doc defers it to 2.4; fine. But structure the
   loader as `LoadPlanets(json const&) → PlanetBiomes` (pure parse into a
   struct) + a cached copy, so `Reload()` later is just re-parse + swap. If
   the parse is entangled with `Item_PlanetType`'s constructor path from day
   one, hot-reload gets expensive to bolt on.

## 5. Small nits

- `_docs.field_reference.colorPalette` has a typo: `[R,G.B]` (dot instead of
  comma) in the format description.
- `gas_giant.surfaceTint` = `[0.569, 0.404, 0.247]` and palette entry 1 is the
  same value — with hex strings this becomes `"#91673F"` twice; fine, but it's
  exactly the kind of line that motivated §2.
- `dockCapacity: -1` in defaults — document what `-1` means ("no docks") next
  to it; magic negative sentinel in a data file is confusing.
- Consider a top-level `"biomeWeights"` later if you want non-uniform biome
  frequency (e.g. deserts common, lava rare) — `seed % N` gives every biome
  exactly 1/N. Not needed now, but the `biomeOrder` array from §3.1 is the
  natural place to add weights without another schema change.

## 6. Suggested next steps (priority order)

1. Add `JColor` hex-string support + convert `planets.json` colors to hex.
2. Wire `oceanLevel` (the marked TODO) so terran oceans actually render.
3. Add `biomeOrder` for seed stability; update `_docs.biome_selection`.
4. Add real-file validation tests (`TestPlanetsData.cpp`).
5. Mark dead fields (`rotationSpeed`, `cloudWindSpeed`) as pending until
   2.3b Phase B lands.

None of these block starting `ships.json`/`weapons.json` — but doing #1–#3
first means those files inherit the final color format and merge rules instead
of forcing a migration later.
