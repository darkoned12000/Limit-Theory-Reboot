<!--
Copyright (C) 2025  darkoned12000
SPDX-License-Identifier: GPL-3.0-or-later
Part of the ltheory-old-test modernization effort (Revamp Work).
See NOTICE and LICENSE.GPL. Original engine (c) Josh Parnell, public domain.
-->

# Limit Theory Old

The (old) C++ implementation of **Limit Theory** — the Limit Theory Engine
(LTE) and the Limit Theory Scripting Language (LTSL) — written by Josh Parnell
from 2012 to 2015. While the code predates the newer C/Lua Limit Theory, it
carries substantially more gameplay implementation: ships, weapons, mining,
trade markets, docking, asteroid belts, procedural stellar systems.

This fork (`ltheory-old-test`) modernizes the engine for **Linux** on current
toolchains (GCC 15/16, CMake 3.31+, SFML 3.1, OpenGL 4.6), hardens the LTSL
scripting experience (better errors, editor tooling, watchdogged runtime), and
builds toward a data-driven, moddable space sandbox.

---

## Features

- **Real-time 3D space engine** — deferred G-buffer renderer (OpenGL 4.6,
  GLSL 4.60 core), SMAA post-processing, lens flares, bloom, nebula and
  star-field rendering, particles, dust clouds.
- **Procedural universe** — seeded spectral-class stars with pulsing
  brightness and auto-generated per-class lens flares, 5-biome planets
  generated from JSON data, asteroid belts, dust flecks, nebulae.
- **Flight & combat** — flyable ships with thruster visuals, turrets, weapon
  classes (beam / pulse / rail / missile), shield and armor simulation.
- **LTSL scripting** — the entire gameplay layer is script (`resource/script/`),
  backed by ~1,600 engine bindings. Good compile-time errors with line
  numbers and "did you mean?" suggestions; opt-in strict return checks; a
  runtime error channel and startup watchdog for fast diagnosis.
- **Proper tooling** — a language server (`script/ltsl-lsp/`) + ZED extension
  with highlighting, completion, hover signatures, and live diagnostics driven
  by a generated API database.
- **Economy & ships** — component-based ship system (hull, scanner, generator,
  power/boost, shields, thrusters), stations with docks and markets,
  JSON-driven ship archetypes and weapon balance.
- **Save/load** — timestamped quicksaves + named save slots (JSON), Esc-menu
  save/load managers, per-slot metadata.
- A **1,052-check headless test suite** (`python3 configure.py test`) covering the
  type system, containers, script compile semantics, JSON database, planet/star
  generation, save format, and engine bindings.

<!-- Screenshots to be added: gameplay shots of ltheory-main / war apps -->

---

## Building on Linux

### Debian / Ubuntu (PikaOS etc.)

```bash
sudo apt install build-essential cmake libopenal-dev libvorbis-dev \
                 libogg-dev libflac-dev flac libfreetype6-dev
git clone https://github.com/darkoned12000/ltheory-old-test.git
cd ltheory-old-test
python3 configure.py            # CMake configure
python3 configure.py build      # parallel build (~10s after first build)
python3 configure.py run ltheory-main
```

### Arch / Omarchy

```bash
sudo pacman -S base-devel cmake sfml freetype2 openal libvorbis libogg flac
git clone https://github.com/darkoned12000/ltheory-old-test.git
cd ltheory-old-test
python3 configure.py && python3 configure.py build
python3 configure.py run ltheory-main
```

> **Wayland note:** the engine window runs through **XWayland** (SFML 3.1 is
> X11-only). Most distros enable XWayland by default; on a
> pure-Wayland session launch under XWayland. A harmless known quirk: pressing
> ESC on some Wayland+XWayland setups can leave the CapsLock LED lit — tap
> CapsLock to clear it (compositor artifact, not an engine bug).
>
> **Audio note:** SFML's audio backend can abort on a cold-started PipeWire
> session (first-run race). Any audio client run once warms the session up if
> you hit a `pa_channel_map_init_extend` abort.

### Verified toolchain

GCC 16.2 / CMake 3.31 / SFML 3.1.0 — **zero build warnings** (`-Werror`
scoped to project code), `lte_tests` at 1052 checks / 0 failures. Older
toolchains (GCC 15, CMake 4) are fine too; see `AGENTS.md` §3 for details.

---

## Running an App

`bin/launch <name>` boots an LTSL script from `resource/script/App/`:

```bash
python3 configure.py run ltheory-main     # recommended starting point
python3 configure.py run war              # AI skirmish testing
```

**`ltheory-main`** is the showcase app: a seed-driven universe sandbox —
class-spectral star with pulsing brightness, nebula + 100k+ star-field,
a biome-generated planet with rings, ~1000-asteroid belt, player ship flying
among the rocks, and 12 seeded AI ships on a patrol shell outside the belt. Tweak it all from
`resource/script/gameConfig.txt` (`seed`, `loadTime`, `playerCredits`,
`shipHull`).

In-app keys (ltheory-main):

| Key | Action |
|-----|--------|
| Mouse | Camera rotation |
| W / S | Thrust Forward / Backward |
| A / D | Strafe Left / Right |
| Q / E | Roll Left / Right |
| Tab | Boost (thruster color ramp) |
| Right Mouse | Fire Weapons |
| Escape | Game menu (SAVE / LOAD / SETTINGS) |
| F2 / F3 | DevPanel / Scene inspector & engine log |
| F6 / F7 | Quicksave / Quickload |

Other runnable apps: `war`, `dogfight`, `launcher`, `threads`, `colony`,
`hnn`, `ui`, `platemesh`, `hud`, `objectinfo`, `map`, `market`, `rails`,
`selftest`.

### Unit tests

```bash
python3 configure.py test   # lte_tests: 1052 checks, headless
```

---

## LTSL Editor Tooling (LSP + ZED)

Editing `.lts` scripts is supported in **ZED** via a language server +
tree-sitter extension: highlighting, completion (`.` trigger), hover
signatures, signature help, live diagnostics — driven by the engine's real
API database.

Prerequisites: **Node.js + npm** (LSP build), **ZED**, plus an engine build
if you regenerate the API database. Setup:

```bash
cd script/ltsl-lsp && npm install && npm run compile && cd ../..
# Zed → Extensions → "Install Dev Extension" → select extensions/ltsl/
```

Verify at any time:

```bash
node script/ltsl-lsp/test-rpc.js
node script/ltsl-lsp/out/smoke.js $(find resource/script -name '*.lts' | sort)
```

Details live in `AGENTS.md` §6.2 (API database regeneration, expected
diagnostic baseline, analyzer invariants).

---

## Tech Stack

| Layer | Technology |
|-------|------------|
| Language | C++17 (`-fno-exceptions`), GCC/Clang |
| Build | CMake ≥ 3.10 via `configure.py` (CMakePresets) |
| Window / Input / Audio | SFML 3.1.0 (miniaudio audio backend) |
| Graphics | OpenGL 4.6 core + GLSL 4.60, GLAD loader, SMAA |
| Fonts | FreeType |
| Data | nlohmann/json (vendored, exception-free consumption) |
| Scripting | LTSL — prefix/indentation syntax interpreter (tree-walking) |
| Reflection | Custom macro system (`AutoClass`, `FIELDS`, `MapFields`) |
| Tooling | TypeScript LSP + tree-sitter grammar + ZED extension |
| Tests | Headless `lte_tests` suite (1052 checks) |

> The engine keeps its own reflection/serialization spine — `Reference<T>`
> (intrusive refcounting) is load-bearing, and the type/function registry
> registers at static-init per translation unit. Read `AGENTS.md` before
> engine surgery.

---

## Architecture Overview

- **`src/liblt/`** — the engine library (`liblt.so`). Subsystems: `LTE`
  (core, type system, serializer, LTSL interpreter, reflection, watchdog),
  `Game` (objects, items, generators, render passes), `Component`
  (Drawable, Collidable, Pilotable, Account, …), `UI` (widgets, glyphs,
  interface), `Module` (SoundEngine—SFML, Physics, Scheduler), `Audio`,
  `Volume`.
- **`src/launch/`** — the `launch` executable entry point.
- **`resource/`** — game data: 169 `.jsl` shaders, LTSL script apps and
  widgets, `gamedata/*.json` (stars, planets, ship archetypes), textures,
  fonts, `gameConfig.txt`.
- **`script/ltsl-lsp/`** + **`extensions/ltsl/`** + **`script/tree-sitter-ltsl/`**
  — the LTSL editing stack (see above).
- **`tests/`** — the headless `lte_tests` suite.
- **`script/ltsl-lsp/api-database.json`** — generated binding database
  (~1,600 fn entries / ~450 types) feeding the editor and the `/smoke` gate.

---

## Roadmap

See [`ROADMAP.md`](ROADMAP.md) for the full plan; highlights:

- **PBR transition** — Albedo/Normal/Roughness/Metallic shaders, directional
  shadows, atmospheric scattering.
- **Data-driven everything (2.3)** — ships/weapons/stations/config/graphics/
  NPC JSON databases (stars/planets/ships already loaded from
  `resource/gamedata/`), hot-reload via `AssetWatcher`.
- **Universe generation Pass A/B/C** — config-driven planet counts,
  station placement, biome-bound planet rotation and cloud drift, moons,
  and live DevTool tuning (F2) of all generator parameters.
- **LTSL language upgrades** — lambdas, arrays, string interpolation, ranges,
  pattern matching (see ROADMAP §3.5). Gameplay itself moves away from
  hardcoded C++ toward JSON-driven configuration.
- **Main menu + modding** — ModManager over the JSON databases, mod hooks,
  input rebinding.

---

## Licensing

- Original engine/languages (Josh Parnell, 2012–2015): **public domain**
  (`LICENSE.UNLICENSE`).
- Modernization / Revamp Work (this fork's new code and substantial
  modifications): **GPL-3.0-or-later** (`LICENSE.GPL`) — see `NOTICE`.

`AGENTS.md` is the technical reference for contributors and AI agents:
build system, subsystem map, engine traps, verification gates, and the
completed-work log.
