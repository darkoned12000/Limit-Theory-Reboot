# LIMIT THEORY REBOOT — Complete Documentation Index

**Last Updated:** 2026-07-30  
**Purpose:** Central hub for all Limit Theory Reboot documentation

---

## 📚 Complete Document Library

### Core Technical Guides

1. **[AGENTS.md](AGENTS.md)** — AI contributor guide, architecture reference  
   **Audience:** AI agents, contributors  
   **When to read:** Before making changes, understanding codebase  
   **Length:** 12,000+ words

2. **[GRAPHICS-TECH.md](GRAPHICS-TECH.md)** — Graphics capabilities + demo apps  
   **Audience:** Visual upgrade developers  
   **When to read:** Improving graphics, adding effects  
   **Length:** 6,000+ words  
   **Quick wins:** Enable SSAO, bloom, lens flare in 5 minutes

3. **[GRAPHICS-ROADMAP-SUMMARY.md](GRAPHICS-ROADMAP-SUMMARY.md)** — Quick reference cheat sheet  
   **Audience:** Developers wanting TL;DR  
   **When to read:** Before diving into GRAPHICS-TECH.md  
   **Length:** 2,000 words

4. **[AUDIO-SYSTEM-GUIDE.md](AUDIO-SYSTEM-GUIDE.md)** — Sound/music wiring  
   **Audience:** Audio implementers  
   **When to read:** Adding background music, sound effects, 3D audio  
   **Length:** 8,000+ words  
   **Quick wins:** Add music to war.lts in 5 minutes

5. **[PROCEDURAL-GENERATION-GUIDE.md](PROCEDURAL-GENERATION-GUIDE.md)** — SDFs, PlateMesh, shader generation  
   **Audience:** Procedural content creators  
   **When to read:** Understanding how ships/stations/planets are built  
   **Length:** 10,000+ words  
   **Key concept:** All assets = runtime-generated from seed

---

### Strategic Planning & Design

6. **[PRD-LIMIT-THEORY-REBOOT.md](PRD-LIMIT-THEORY-REBOOT.md)** — Product Requirements Document  
   **Audience:** Project planners, contributors, stakeholders  
   **When to read:** Before starting work, quarterly reviews  
   **Length:** 15,000+ words (comprehensive)  
   **Contents:**
   - Executive vision
   - 6-phase roadmap (18 months)
   - Feature specifications
   - Success metrics
   - Community strategy
   - Risk mitigation

7. **[VULKAN-AND-SPACE-PHENOMENA.md](VULKAN-AND-SPACE-PHENOMENA.md)** — Vulkan migration + living universe design  
   **Audience:** Graphics programmers, game designers  
   **When to read:** Considering Vulkan upgrade, designing space phenomena  
   **Length:** 12,000+ words  
   **Contents:**
   - OpenGL → Vulkan migration effort (6 months)
   - Verdict: Stay with OpenGL 4.6 for now
   - Freelancer-style sector system
   - Black holes, wormholes, nebula, cosmic storms
   - **Space whales!** 🐋
   - Complete implementation roadmap

8. **[ENGINE-STABILITY-AND-MODDING.md](ENGINE-STABILITY-AND-MODDING.md)** — Production-ready engine + modding system  
   **Audience:** Engine developers, modders, game designers  
   **When to read:** Stabilizing engine, adding save/load, creating modding support  
   **Length:** 15,000+ words (comprehensive)  
   **Contents:**
   - Engine stability assessment (what's missing)
   - C vs C++ verdict (no conversion needed)
   - Data-driven JSON architecture (ships, weapons, planets)
   - UI & input system (Freelancer-style + gamepad)
   - Complete modding architecture (Skyrim-level)
   - Scanner & atmospheric effects (dust, clouds, nebula)
   - 5-phase implementation roadmap (13-18 weeks)

9. **[SAVE-LOAD-AND-INVENTORY.md](SAVE-LOAD-AND-INVENTORY.md)** — Save system + JSON schemas + inventory  
   **Audience:** Gameplay programmers, save system implementers  
   **When to read:** Implementing save/load, exposing inventory, adding mining  
   **Length:** 12,000+ words (comprehensive)  
   **Contents:**
   - Discovery: Cargo system EXISTS but hidden!
   - Discovery: Mining system EXISTS but not wired!
   - JSON save file schemas (v1.0 + minimal)
   - Binary vs JSON serialization comparison
   - Complete inventory UI (LTSL code)
   - Mining mechanics (wire existing `Action_Mine`)
   - Loot drop system (NPC cargo pods)
   - 7-phase implementation roadmap (5-6 weeks)

10. **[LTSL-ARCHITECTURE-AND-IMPROVEMENTS.md](LTSL-ARCHITECTURE-AND-IMPROVEMENTS.md)** — LTSL deep-dive + user-friendliness  
   **Audience:** Language designers, LTSL developers, scripters  
   **When to read:** Understanding LTSL internals, improving scripting ergonomics  
   **Length:** 15,000+ words (comprehensive)  
 1. **[assessment.md](assessment.md)** — Technical assessment + Top 5 priorities  
   **Audience:** Refactoring planners  
   **When to read:** Understanding modernization work, priority decisions  
   **Length:** 12,000+ words

12. **[docs/ltsl-docs.md](docs/ltsl-docs.md)** — LTSL scripting language reference  
   **Audience:** Gameplay scripters  
   **When to read:** Writing LTSL code, debugging scripts  
   **Length:** 8,000+ wordg, optional chaining)
   - Bytecode VM (optional 3-5x speedup)
   - 3-phase implementation roadmap (2-4 weeks → 2-3 months)

---

### Secondary Docs

10. **[assessment.md](assessment.md)** — Technical assessment + Top 5 priorities  
   **Audience:** Refactoring planners  
   **When to read:** Understanding modernization work, priority decisions  
   **Length:** 12,000+ words

11. **[docs/ltsl-docs.md](docs/ltsl-docs.md)** — LTSL scripting language reference  
   **Audience:** Gameplay scripters  
   **When to read:** Writing LTSL code, debugging scripts

12. **[docs/LTSL-LSP-IMPLEMENTATION-GUIDE.md](docs/LTSL-LSP-IMPLEMENTATION-GUIDE.md)** — Build Language Server for LTSL IntelliSense  
   **Audience:** IDE tooling developers, LTSL scripters wanting autocomplete  
   **When to read:** Want IntelliSense/autocomplete/hover tooltips for `.lts` files  
   **Length:** 25,000+ words (comprehensive with complete working code)  
   **Contents:**
   - Full tokenizer implementation (TypeScript)
   - Parser & AST builder (handle 26 LTSL expression types)
   - Semantic analyzer (type checking, scoping, diagnostics)
   - LSP server (autocomplete, hover, errors, go-to-definition)
   - VS Code extension (syntax highlighting, snippets)
   - API database (auto-generate from C++ bindings)
   - Advanced features (error recovery, "did you mean?", signature help)
   - Complete deployment guide
   - **20-minute speedrun** setup guide

13. **[docs/ship-info.md](docs/ship-info.md)** — Ships, weapons, combat, AI  
   **Audience:** Gameplay designers, combat system implementers  
   **When to read:** Creating ships, balancing weapons, implementing AI behaviors  
   **Length:** 15,000+ words  
   **Contents:**
   - Complete ship creation pipeline
   - Weapon system (Pulse/Missile/Beam/Rail + proposed Torpedo/Flak)
   - JSON data file integration proposal
   - AI task system (Destroy, Goto, Patrol, Mine, Play)
   - Combat mechanics (intercept calculation, damage model)
   - Ship physics & SDF-based steering
   - Custom ship archetypes (Interceptor, Tank, Carrier, etc.)

---

## 🎯 Quick Navigation by Use Case

### "I want to improve graphics"

**Path:**
1. Read [GRAPHICS-ROADMAP-SUMMARY.md](GRAPHICS-ROADMAP-SUMMARY.md) (10 min)
2. Try "Quick Wins" — uncomment 4 lines in war.lts (5 min)
3. Read [GRAPHICS-TECH.md](GRAPHICS-TECH.md) Section 2.1 (PBR shader) (30 min)
4. Implement PBR following code examples (2-3 weeks)

---

### "I want to add sound/music"

**Path:**
1. Read [AUDIO-SYSTEM-GUIDE.md](AUDIO-SYSTEM-GUIDE.md) Part 4 (10 min)
2. Add background music to war.lts (copy-paste from Part 12) (5 min)
3. Add engine hum + weapon sounds (Part 5) (30 min)
4. Implement dynamic music system (Part 4.2) (2 days)

---

### "I want IntelliSense/autocomplete for LTSL scripts"

**Path:**
1. Read [docs/LTSL-LSP-IMPLEMENTATION-GUIDE.md](docs/LTSL-LSP-IMPLEMENTATION-GUIDE.md) Appendix (10 min)
2. Follow "20-minute speedrun" setup guide
3. Install VS Code extension: `code --install-extension ltsl-1.0.0.vsix`
4. Open any `.lts` file — try Ctrl+Space for autocomplete!
5. **Result:** Full IntelliSense (autocomplete, hover tooltips, error checking)

---

### "I want to understand procedural generation"

**Path:**
1. Read [PROCEDURAL-GENERATION-GUIDE.md](PROCEDURAL-GENERATION-GUIDE.md) Part 1 (SDFs) (20 min)
2. Read Part 2 (PlateMesh) (20 min)
3. Experiment with `Item/ShipType/Generate.lts` — change seed params (1 hour)
4. Implement custom procedural type (Part 5) (1 week)

---

### "I want to plan the project roadmap"

**Path:**
1. Read [PRD-LIMIT-THEORY-REBOOT.md](PRD-LIMIT-THEORY-REBOOT.md) Executive Vision (10 min)
2. Read Section 5 (Feature Roadmap Phases 1-6) (30 min)
3. Read Section 13 (Timeline & Milestones) (15 min)
4. Create GitHub project board from PRD tasks (1 hour)

---

### "Should I migrate to Vulkan?"

**Path:**
1. Read [VULKAN-AND-SPACE-PHENOMENA.md](VULKAN-AND-SPACE-PHENOMENA.md) Part 1 (20 min)
2. **Answer: NO** — Stay with OpenGL 4.6 (6 months work, 0% player benefit)
3. **Alternative:** Implement GPU instancing instead (2-3 weeks, 95% of Vulkan's benefit)

---

### "I want to design a Freelancer-style living universe"

**Path:**
1. Read [VULKAN-AND-SPACE-PHENOMENA.md](VULKAN-AND-SPACE-PHENOMENA.md) Part 2 (40 min)
|----------|-------|-------|----------------|
| AGENTS.md | 12,000+ | Architecture, LTSL, rendering, AI reference | 2-3 hours |
| GRAPHICS-TECH.md | 6,000+ | Visual upgrade, demo apps, shader code | 1-2 hours |
| GRAPHICS-ROADMAP-SUMMARY.md | 2,000 | Quick wins, TL;DR | 15 min |
| AUDIO-SYSTEM-GUIDE.md | 8,000+ | Sound/music wiring, 3D audio | 1-2 hours |
| PROCEDURAL-GENERATION-GUIDE.md | 10,000+ | SDFs, PlateMesh, shader generation | 2-3 hours |
| PRD-LIMIT-THEORY-REBOOT.md | 15,000+ | Vision, roadmap, strategy | 3-4 hours |
| VULKAN-AND-SPACE-PHENOMENA.md | 12,000+ | Vulkan assessment, space phenomena | 2-3 hours |
| ENGINE-STABILITY-AND-MODDING.md | 15,000+ | Save/load, JSON, modding, UI, scanners | 3-4 hours |
| LTSL-ARCHITECTURE-AND-IMPROVEMENTS.md | 15,000+ | Tree-walking interpreter, language improvements | 3-4 hours |
| AI-ASSISTED-DEVELOPMENT-GUIDE.md | 12,000+ | Creating AI-friendly docs, skill files | 2-3 hours |
| **SKILL.md** (`.opencode/skills/ltheory/`) | **15,000+** | **⭐ Master AI reference (load first!)** | **1-2 hours** |
| **ROADMAP.md** (repo root) | ~4,000 | **⭐ Consolidated forward work plan (single source of truth)** | 30 min |
| assessment.md | 12,000+ | Modernization priorities | 2-3 hours |
| docs/ltsl-docs.md | 8,000+ | LTSL syntax reference | 1-2 hours |
| docs/LTSL-LSP-IMPLEMENTATION-GUIDE.md | 25,000+ | LSP server + VS Code extension (complete code) | 2-3 hours |
| docs/ship-info.md | 15,000+ | Ships, weapons, JSON data, AI, combat | 1-2 hours |
| **TOTAL** | **194,000+** | **Complete reference library** | **36-42 hours** |
| **TOTAL** | **104,000+** | **Complete reference library** | **20-24 hours** |
**Path:**
1. Read [ENGINE-STABILITY-AND-MODDING.md](ENGINE-STABILITY-AND-MODDING.md) Part 5 (30 min)
2. Implement mod folder system (1 week)
3. Add mod JSON loader (1 week)
4. Create script hooks (1 week)
5. Test with example mod (space whales expanded)

---

### "I want Freelancer-style UI + gamepad support"

**Path:**
1. Read [ENGINE-STABILITY-AND-MODDING.md](ENGINE-STABILITY-AND-MODDING.md) Part 4 (20 min)
2. **Good news:** Gamepad support ALREADY EXISTS! (verified in code)
3. Add input rebinding UI (1 week)
4. Create radial menu (2-3 days)
5. Add context menus (2 days)

---

### "I want flying-through-dust/nebula effects + scanners"

**Path:**
1. Read [ENGINE-STABILITY-AND-MODDING.md](ENGINE-STABILITY-AND-MODDING.md) Part 6 (25 min)
2. Implement radar scanner widget (1 week)
3. Add volumetric fog shader (3-4 days)
4. Create dust particle system (2-3 days)
5. Add cloud droplets effect (2 days)

---

### "How do I implement save/load system?"

**Path:**
1. Read [SAVE-LOAD-AND-INVENTORY.md](SAVE-LOAD-AND-INVENTORY.md) Part 2-3 (40 min)
2. **Key discovery:** Serialization infrastructure EXISTS!
3. Add `nlohmann/json` library (10 minutes)
4. Implement `SaveGame.cpp` with JSON writer (2-3 days)
5. Add main menu "Save/Load" buttons (1-2 days)
6. Test: Save → quit → load → verify state

---

### "I can't mine asteroids or see my inventory!"

**Path:**
1. Read [SAVE-LOAD-AND-INVENTORY.md](SAVE-LOAD-AND-INVENTORY.md) Part 1 + 4-5 (30 min)
2. **HUGE discovery:** Cargo system EXISTS, just needs UI!
3. **HUGE discovery:** Mining system EXISTS, just needs hotkey!
4. Add inventory UI (press 'I') (3-4 days)
5. Wire mining hotkey (press 'M') (1 week)
6. Add targeting system (press 'T') (2-3 days)

---

### "How do I add loot drops from NPCs?"

**Path:**
1. Read [SAVE-LOAD-AND-INVENTORY.md](SAVE-LOAD-AND-INVENTORY.md) Part 5 (15 min)
2. Create `Object/Pod.lts` (loot container) (2 days)
3. 

### "How does LTSL work? Can I make it more user-friendly?"

**Path:**
1. Read [LTSL-ARCHITECTURE-AND-IMPROVEMENTS.md](LTSL-ARCHITECTURE-AND-IMPROVEMENTS.md) Part 1-2 (40 min)
2. **Key discovery:** Tree-walking interpreter with 26 expression types
3. Read Part 3-4 for improvements (list methods, foreach, string interpolation)
4. Implement Phase 1: List methods (`.Filter`, `.Map`, `.Reduce`) (3 days)
5. Implement `foreach` loop (2 days)
6. Implement string interpolation (`$"..."`) (3 days)
7. Implement lambda syntax (`arg => body`) (2 days)

---

### "I want to understand LTSL internals / modify the language"

**Path:**
1. Read [LTSL-ARCHITECTURE-AND-IMPROVEMENTS.md](LTSL-ARCHITECTURE-AND-IMPROVEMENTS.md) Part 2 (1 hour)
2. Study `src/liblt/LTE/Expression.cpp` (compilation dispatcher)
3. Study `src/liblt/LTE/Expression/If.cpp` (example expression node)
4. Pick an improvement from Part 4-5 (list methods, pattern matching, etc.)
5. Implement + test

---

### "I'm an AI agent, how can I help most effectively?"

**Path:**
1. **Load [SKILL.md](../.opencode/skills/ltheory/SKILL.md) FIRST** (15 min read, then instant lookups)
2. Read [AI-ASSISTED-DEVELOPMENT-GUIDE.md](AI-ASSISTED-DEVELOPMENT-GUIDE.md) (30 min)
3. **Key facts to remember:**
   - Cargo/mining/serialization systems EXIST (don't re-implement!)
   - LTSL string concat uses juxtaposition, not `+`
   - Position ≠ Vec3 (double vs float precision)
   - Stay with OpenGL 4.6 (Vulkan not recommended)
   - Self-widget apps broken (use driven pattern)
4. When user asks question:
   - Check SKILL.md quick reference first
   - Provide exact file paths
   - Give working code (copy-paste ready)
   - Reference detailed docs for deep-dives
   - Warn about gotchas before user hits them

---Modify ship death to drop cargo (1 day)
4. Add pickup collision (player flies through pods) (1 day)

---

### "I'm an AI agent contributing code"

**Path:**
1. Read [AGENTS.md](AGENTS.md) Sections 1-6 (architecture, LTSL, rendering) (1 hour)
2. Read Section 11 (licensing, contributor headers) (10 min)
3. Check [assessment.md](assessment.md) Section 9 (Top 5 Priorities) for current tasks (20 min)
4. Read relevant technical guide (GRAPHICS-TECH.md, AUDIO-SYSTEM-GUIDE.md, etc.) (30 min)

---

## 📊 Document Statistics

| Document | Words | Focus | Effort to Read |

---

### "Should I migrate to Vulkan?"

**Path:**
1. Read [VULKAN-AND-SPACE-PHENOMENA.md](VULKAN-AND-SPACE-PHENOMENA.md) Part 1 (20 min)
2.VULKAN-AND-SPACE-PHENOMENA.md | 12,000+ | Vulkan assessment, space phenomena design | 2-3 hours |
| assessment.md | 12,000+ | Modernization, priorities, content guide | 2-3 hours |
| **TOTAL** | **77,000+** | **Complete reference library** | **15-18 of Vulkan's benefit)

---

### "I want to design a Freelancer-style living universe"

**Path:**
1. Read [VULKAN-AND-SPACE-PHENOMENA.md](VULKAN-AND-SPACE-PHENOMENA.md) Part 2 (40 min)
2. Prototype sector + warp gate system (1 day)
3. Add black hole (3 days)
4. Add space whales 🐋 (1 week)
5. Expand to 50 sectors (seeded generation)
|----------|-------|-------|----------------|
| AGENTS.md | 12,000+ | Architecture, LTSL, rendering, AI reference | 2-3 hours |
| GRAPHICS-TECH.md | 6,000+ | Visual upgrade, demo apps, shader code | 1-2 hours |
| GRAPHICS-ROADMAP-SUMMARY.md | 2,000 | Quick wins, TL;DR | 15 min |
| AUDIO-SYSTEM-GUIDE.md | 8,000+ | Sound/music wiring, 3D audio | 1-2 hours |
| PROCEDURAL-GENERATION-GUIDE.md | 10,000+ | SDFs, PlateMesh, shader generation | 2-3 hours |
| PRD-LIMIT-THEORY-REBOOT.md | 15,000+ | Vision, roadmap, strategy | 3-4 hours |
| assessment.md | 12,000+ | Modernization, priorities, content guide | 2-3 hours |
| **TOTAL** | **65,000+** | **Complete reference library** | **12-15 hours** |

---

## 🚀 Getting Started Paths

### Path A: "I just want to try the engine"

```bash
git clone <your-repo-url>
cd ltheory-reboot
python3 configure.py
python3 configure.py build
python3 configure.py run war
```

**Next:** Try other apps (`launcher`, `ltheory-main`, `dogfight`)

---

### Path B: "I want to contribute (visual upgrade)"

1. Read GRAPHICS-ROADMAP-SUMMARY.md (15 min)
2. Try Quick Wins (5 min)
3. Pick Priority 1 (GPU instancing) or Priority 4 (PBR) from assessment.md
4. Follow step-by-step implementation guide
5. Submit PR

---

### Path C: "I want to learn procedural generation"

1. Read PROCEDURAL-GENERATION-GUIDE.md Part 1-3 (1 hour)
2. Run `python3 configure.py run planet_viewer` (demo app)
3. Modify `Item/ShipType/Generate.lts` — change plate count, bevels
4. Generate 100 unique ships, screenshot gallery

---

### Path D: "I want to build the full project (long-term)"

1. Read PRD-LIMIT-THEORY-REBOOT.md in full (3-4 hours)
2. Create GitHub project board (Kanban)
3. Pick Phase 1 tasks (GPU instancing, multi-system universe, trading)
4. Work through 18-month roadmap systematically
5. Release v1.0

---

## 🎓 Learning Resources (External)

**SDFs (Signed Distance Functions):**
- [Inigo Quilez - SDF Functions](https://iquilezles.org/articles/distfunctions/)
- [GPU Gems: Volume Rendering](https://developer.nvidia.com/gpugems/gpugems3/part-i-geometry/chapter-1-generating-complex-procedural-terrains-using-gpu)

**PBR (Physically-Based Rendering):**
- [Learn OpenGL - PBR Theory](https://learnopengl.com/PBR/Theory)
- [Filament PBR Guide](https://google.github.io/filament/Filament.html)

**Procedural Generation:**
- [Sebastian Lague - Procedural Planets](https://www.youtube.com/watch?v=QN39W020LqU)
- [Procedural Generation Wiki](http://pcg.wikidot.com/)

**GLSL Shaders:**
- [Book of Shaders](https://thebookofshaders.com/)
- [Shadertoy](https://www.shadertoy.com/) — Community shader examples

---

## 🛠️ Quick Reference: Common Tasks

### Add Background Music

**File:** `resource/script/App/war.lts`  
**Add in `Initialize()`:**
```lts
Sound_PlayLooped "system/ambiance/089.wav" 0.05
```

### Enable SSAO

**File:** `resource/script/App/war.lts`  
**Add after line ~28:**
```lts
passes.Append (RenderPass_SSAO)
```

### Generate New Ship Type

**File:** `resource/script/Item/ShipType/Generate.lts`  
**Modify line ~10:**
```lts
var plates 2 + (Int (Sqrt scale))  # Change to: 5 + (Int (Sqrt scale))
```

### Add New Planet to System

**File:** `resource/script/Object/SystemPopulate.lts`  
**In `Init()`, add after existing planet:**
```lts
var planet2 (Object_Planet (Item_PlanetType (rng.Int + 99)))
planet2.SetPos (Vec3 300000 0 0)  # 300km away
system.AddInterior planet2
```

---

## 📞 Support & Community

**GitHub Issues:** [Report bugs, request features](https://github.com/darkoned12000/ltheory-reboot/issues)  
**Discord:** [Join dev community](#) (coming soon)  
**YouTube:** [Devlog series](#) (coming soon)  
**Reddit:** [r/limittheory](https://reddit.com/r/limittheory)

---

## 📝 License

- **Original code** (Josh Parnell, 2012-2015): Public domain, Unlicense
- **Revamp Work** (darkoned12000, 2025+): GPL-3.0-or-later
- **Documentation** (this file + all .md guides): CC-BY-4.0 (attribution required)

See [NOTICE](NOTICE), [LICENSE.UNLICENSE](LICENSE.UNLICENSE), [LICENSE.GPL](LICENSE.GPL) for details.

---

## 🎉 You're Ready!

**Pick a path above and start building.** Every great journey begins with a single commit. 🚀

**Last tip:** Start small (Quick Wins), build confidence, then tackle bigger features. Josh's engine is a treasure trove waiting to be unlocked. You've got this!

---

**Happy coding! 💻✨**
