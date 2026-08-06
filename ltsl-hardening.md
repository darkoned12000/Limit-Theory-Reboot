<!--
Copyright (C) 2025  darkoned12000
SPDX-License-Identifier: GPL-3.0-or-later
Part of the ltheory-old-test modernization effort (Revamp Work).
See NOTICE and LICENSE.GPL. Original engine (c) Josh Parnell, public domain.
-->

# ltsl-hardening.md — Making LTSL Easy to Develop & Troubleshoot

**Status:** Living analysis doc (feedback + roadmap). Complements `AGENTS.md`
(engine reference), `docs/ltsl-docs.md` (language), `ROADMAP.md` (work plan),
and `ltsl-binding-bridge-replacement.md` (the binding bridge that finished in
Step 10, 2026-08-05). It exists so that every DX failure we hit while doing the
bridge/While/LSP work is captured once, with a fix priority, instead of being
re-derived next time.

Two halves:

1. **§2–4 — Ordering & priority.** What runs before what so everything renders
   the way you want. This is the "function priority" question: render passes,
   widget lifecycle, interface z-order, frame timing, and the C++-side
   registration invariants (alias-after-source, binding registration).
2. **§5–9 — DX feedback & troubleshooting.** Every pain point observed in real
   sessions (spurious errors, silent failures, hang traps) plus a
   prioritized hardening roadmap.

---

## 1. TL;DR — The 30-second version

- **Render passes run strictly in list order.** `RenderPass_Clear` MUST be
  first, `RenderPass_PostFilter` MUST be last, `RenderPass_Interface` before
  the post-filter. See §2.1.
- **World simulation must be stepped before UI update before draw.**
  Prescribed app order: `system.Update dt` → `ui.Update` → `gameView.Update`
  → `gameView.Draw`. `gameView.Draw` (NOT `ui.Draw`) drives the whole 3D +
  UI pipeline. See §2.4.
- **Widget z-order = add order.** The last widget added to an interface draws
  on top (and updates first). No explicit z value exists. See §2.3.
- **Widget hooks have a strict order** (PreUpdate → children → PostUpdate;
  PreDraw → children → PostDraw; CreateChildren runs once on the rebuild
  path). See §2.2.
- **C++ alias order is a hard invariant:** `Function_Alias("Src", "Dst")`
  must come textually AFTER its source binding in the same file, or it
  registers an empty bucket silently. Gate: `script/check_binding_alias_order.py`
  → `OK: 509 / 1 known`. See §4.
- **The single biggest DX problem today:** `Expression_Compile`'s probe chain
  reports spurious "unknown variable '1'"-style errors for every literal atom
  before the Constant parser succeeds. Non-fatal, but noisy and misleading
  (ui/market/hud print "7 compilation error(s)" every load). Fix is small;
  see §6.1.
- **Hardening priority:** (P1) silent literal probes, script-visible logging,
  runtime error channel, startup watchdog. (P2) explicit-return enforcement,
  line-attribution for `#` comments, `ltsl_api_dump` docs. (P3) hot reload,
  data-driven UI, list methods. Full list in §8.

---

## 2. Ordering & Priority — What Goes Before What

### 2.1 Render pass pipeline (the top of the "priority" question)

Attach a render list with `gameView.Add (Widget_Rendered passes)` where
`passes` is `Vector<Reference<RenderPassT>>`. The passes execute **in list
order** inside `Widget_Rendered::PreDraw`
(`src/liblt/UI/Widget/Rendered.cpp:128-129`):

```
for (i = 0; i < passes.size(); ++i) passes[i]->Render(&state);
```

The canonical chain (war.lts:24-29, ltheory-main.lts:214-218, launcher.lts):

```
RenderPass_Clear (Vec4 0.0)          # MUST be first
RenderPass_Camera camera             # 3D scene (has its own hard-coded sub-chain)
RenderPass_SMAA                      # anti-aliasing
RenderPass_Interface ui              # UI composited on top of the 3D scene
RenderPass_PostFilter "post/dither.jsl"   # MUST be last (full-screen post FX)
```

Rules that keep rendering correct:

| Rule | Why |
|---|---|
| `RenderPass_Clear` first | A bare `gameView.Draw` with no clear leaves stale buffers. Clear also resets the renderer's per-frame counters and clears the `smallColor` buffers (`Game/RenderPass/Clear.cpp:23-34`). The "MUST be first" constraint is documented at `App/launcher.lts:24`. |
| `RenderPass_Camera` second (scene) | It pushes camera view/proj + fog and runs its own hard-coded sub-chain: Visibility → DepthPrepass → HiZOcclusion → GBuffer → GlobalLighting → LocalLighting → Blended → DustClouds → Particles → LensFlares → Bloom(128,64) → MotionBlur → PostFilter(`colorgrade1D.jsl`) (`Game/RenderPass/Camera.cpp:26-40`). You do not control that sub-chain from LTSL. |
| `RenderPass_Interface` after the scene, before post | It draws the interface into `state->tertiary`, composites via `ui/composite.jsl`, and flips (`UI/Interface.cpp:88-125`). UI must be drawn after the 3D scene so it sits on top. |
| `RenderPass_PostFilter` last | Post filters are full-screen passes that read `primary` and write/flip. Anything added after them would draw without the effect (or get blurred/dithered). SMAA itself is 3 edge/weight/blend FSQ passes ending in a swap (`Game/RenderPass/SMAA.cpp:59-97`). |
| One rendered widget per interface | `gameView.Clear` + re-`Add` swaps the whole pipeline (ltheory-main uses this to switch from the loading render list to the game render list). |

To inspect at runtime which passes a camera runs, or reorder, edit the
`passes` list in the app. To toggle whole passes, the engine gates each pass
on `Settings_Bool("Graphics/<name>", true)` (`LTE/RenderPass.cpp:9-16`).

### 2.2 Widget lifecycle order (per frame)

Widgets are the composable UI units. Each frame the engine drives them in a
fixed hook order (`src/liblt/UI/Widget.cpp`). If you use the script hooks
(`PreUpdate`/`PostUpdate`/`PreDraw`/`PostDraw`/`PrePosition`/`PostPosition`/
`CreateChildren` — wired in `UI/Widget/Custom.cpp:33-129`), this is the order
your code runs in:

**Update phase** (`Widget.cpp:154-172`) — parent-first pre, child-first post:
1. `PreUpdate` components — forward vector order
2. Children `Update` — **reverse** add order (deepest/last-added child's full
   update, including its PostUpdate, finishes before the parent's PostUpdate)
3. `CaptureFocus` + focus-state merge
4. `PostUpdate` components — reverse order

**Draw phase** (`Widget.cpp:40-57`) — parent-first throughout:
1. `PreDraw` components — reverse order
2. Children `Draw` — **forward** add order (last child drawn on top);
   `child->alpha *= alpha` cascades transparency down the tree
3. `PostDraw` components — forward order

**Position/layout phase** (`Widget.cpp:91-133`):
- `PrePosition`: applies the `rebuild` flag (drops children, resets
  `initialized`), then `CreateChildren` is called **once** when `!initialized`,
  then children `PrePosition` forward, then components `PrePosition` forward.
- `PostPosition` (`Widget.cpp:135-144`): components reverse → children forward.
- `Rebuild()` (set the `rebuild` flag) is consumed at the next `PrePosition`;
  it does NOT take effect mid-frame.

Practical consequences:
- **Put per-frame state changes in `PostUpdate`** (after children have run) and
  **read/position children in `PostPosition`** — that is the pattern the
  map app uses (`Widget/Map.lts:PostPosition`).
- **The visual z-order of sibling widgets inside a `Stack` = add order** — the
  last child added is drawn on top (`Stack` layers in Button.lts rely on this).
- **Draw-once graphics (panels, glyphs) go in `PreDraw`/`PostDraw`, never in
  `Update`** — `Update` has no guarantees about draw state.

### 2.3 Interface z-order & update order

`Interface` (`src/liblt/UI/Interface.cpp`) keeps widgets in a plain vector:
- **Draw**: forward order (`Interface.cpp:45-56`) — **later-added widgets draw
  on top. This is the entire z-ordering mechanism** (there is no z value).
- **Update**: reverse order (`Interface.cpp:58-85`) — later-added (on-top)
  widgets update first, and the pointer-following/modal layer stays correct.
  Per widget: `PrePosition → pos/size assignment → PostPosition → Update`.

Consequences:
- To render X above Y: `Add` X after Y. To bring a widget to front:
  re-add it (remove + add at the end).
- Hotkey overlays (F2/F3 dev panels) added after the HUD naturally sit on top.
- If a widget never shows, it is almost always an add-order / containment
  problem, not a shader problem.

### 2.4 Frame timing & the app update order

Main loop (`src/liblt/LTE/Program.cpp:41-71`):
1. input update → window update
2. **`OnUpdate()` — your LTSL app `Update` runs here**
3. `Module_UpdateGlobal()` (FrameTimer + Scheduler refresh) — **after** the app
   Update
4. `window->Display()`

`FrameTimer_Get` therefore returns the delta from the previous frame (which
included the previous app Update), not "time since last frame start". Clamp it
or the first frame after a stall can run a huge step: `dt = (Min dt 0.1)` —
both war.lts and ltheory-main.lts do this.

The launcher calls the scripted `Update` **before** `physicsEngine->Update()`
and `soundEngine->Update()` (`src/launch/launch.cpp:90-111`).

**Prescribed in-app order** (war.lts:81-97, mirrored in ltheory-main.lts):

```
var dt FrameTimer_Get          # 1. grab + clamp dt
dt = (Min dt 0.1)
...
system.Update dt               # 2. STEP THE WORLD FIRST
ui.Update                      # 3. update UI layer
gameView.Update                # 4. update game view layer
gameView.Draw                  # 5. DRAW (drives the entire pass pipeline)
```

Three subtleties that bite people:
- **Call `gameView.Draw`, not `ui.Draw`.** `gameView.Draw` triggers
  `Widget_Rendered::PreDraw`, which runs the pass list (including
  `RenderPass_Interface`, which draws `ui` into the pipeline). A bare
  `ui.Draw` renders the interface outside the pipeline (no post effects).
- **Step the world before drawing.** The 3D render happens inside
  `gameView.Draw`; if you draw before stepping, everything lags one frame and
  camera-follow looks broken.
- **A `ui.Update` (or `gameView.Update`) missing** leaves the interface never
  progressing (hover/focus/animations frozen) even though it draws.

### 2.5 `desc`/`block` body evaluation order

- Bodies evaluate **left-to-right**; only the **last** expression becomes the
  return value (`Expression/Block.cpp:50-85`).
- A `return expr` sets `env.returnSignal` and **stops** the block loop
  (`Block.cpp:70-71`); this is the mechanism the while/return fix relies on
  (see §6.6).
- `while` checks the `returnSignal` **before re-evaluating its predicate and
  after its body** — a `return` inside a loop terminates it immediately
  (`Expression/While.cpp`, fixed 2026-08-04; regression test
  `Expression_While_ReturnInLoopBreaks`).

### 2.6 LTSL expression precedence & rewriting

Engine rewrites before compile (`LTE/LTSL.cpp`):
- `a.b` → `(b a)`; `a.b.c` → `(c (b a))` (bottom-up).
- Operator precedence, highest → lowest: `^` → `* /` → `+ -` →
  `< > <= >=` → `== !=` → `&&` → `||` → `= += -= *= /=`, **left-associative**
  (`LTSL.cpp:50-64`).

When in doubt, parenthesize. `(a + b * c)` and `(a + (b * c))` agree, but any
chain of mixed operators is easier to misread than to mistype.

---

## 3. C++-side registration order (affects what LTSL can see)

### 3.1 Alias AFTER source — hard invariant

`Function_AddAlias(source, alias)` **copies** the source function bucket by
value at the moment the alias statement runs (`Function.cpp:65-67`,
`Vector.h:81-87`). If the source binding hasn't executed yet (wrong static-init
order), the alias registers an **empty bucket permanently** — later source
registrations never retro-fill it. The alias also snapshots the bucket, so the
alias keeps only what was registered up to that point.

- **Rule:** in a `.cpp` file, put `Function_Bind(...)` (or the registration
  expression) before the `Function_Alias(...)` line for the same function.
- **Gate:** `python3 script/check_binding_alias_order.py $(git ls-files 'src/liblt/**/*.cpp' 'src/liblt/**/*.h')`
  → `OK: 509 alias sites follow their source (1 known exceptions)`. The 1
  known exception is the pre-existing `Vec2_Distance` copy-paste bug in
  `V2.cpp:34` (aliases a never-registered name; documented in
  `ltsl-binding-bridge-replacement.md`).
- **Note:** `Function_AddAlias` copies the *current* bucket, so overloads
  registered after the alias line won't appear under the alias.

### 3.2 Bindings are registered at static-init (per TU)

All `Function_Bind`/`Function_Alias`/`TypeAlias`/`Conversion_Bind` run at
static-init in each translation unit. There is no central registration pass —
the API database is dumped from the live type/function registry
(`ltsl_api_dump`). Consequences:
- New functions appear to scripts **as soon as the rebuilt `.so` is loaded**
  (no DB regeneration needed for the engine; the LSP DB is only for editing).
- The alias-order gate is the only automated check on registration order; keep
  it in CI/commit flow.

### 3.3 The API DB and the LSP (for editing only)

`script/ltsl-lsp/api-database.json` (1852 functions / 445 types) feeds the
editor language server — it does NOT affect the engine at runtime. Regenerate
when the C++ API changes:

```bash
cmake --build ./build --target ltsl_api_dump -j
LD_LIBRARY_PATH=bin:extbin/linux64 ./bin/ltsl_api_dump script/ltsl-lsp/api-database.json
```

> **Trap (fixed in this doc's source, 2026-08-05):** `ltsl_api_dump` takes an
> optional output path and **defaults to `./api-database.json` in the current
> working directory**. The old AGENTS.md command used a `>` redirect, which
> overwrote the DB with the tool's status message. Always pass the target path
> explicitly (or run from `script/ltsl-lsp/`). Compare against
> `build/api-baseline.json` after changes: expect **0 added / 0 removed**.

---

## 4. Rendered output checklist (debug "it doesn't look right")

When something renders wrong, walk this list in order — it mirrors the engine's
own evaluation order:

1. **Is the world stepped before the draw?** `system.Update dt` before
   `gameView.Draw`. A frozen/laggy scene is usually a missing `Update`.
2. **Is `gameView.Draw` called (not `ui.Draw`)?** Without it no pass pipeline
   runs — no post effects, no camera.
3. **Is the render pass list in order?** Clear first, Camera, SMAA,
   Interface, PostFilter last (§2.1). Add/remove passes here, not by hacking
   a sub-chain you don't control.
4. **Is the interface widget added to the right layer?** `ui` widgets show
   after `RenderPass_Interface`; `gameView` is the 3D layer. Wrong layer =
   widget invisible or under the scene.
5. **Is the widget added after the thing it should cover?** z-order = add
   order (§2.3). Re-add to bring to front.
6. **Are hooks in the right phase?** `PreDraw` for backgrounds, `PostDraw`
   for overlays, `PostUpdate` for state, `PostPosition` for layout reads
   (§2.2).
7. **Per-frame allocations?** A tree-walking interpreter means hot `Update`
   paths churn. Cache per-frame state in the widget type instead of
   re-`Create` in `Update`.

---

## 5. DX feedback from real sessions (2026-08 bridge + While fix)

These are the pain points observed while finishing the binding-bridge Step 10
and debugging the while/return hang. Each is a real failure mode we hit.

### 5.1 Spurious "unknown variable '1'" errors (the biggest noise)

`Expression_Compile` probes an atom in order: Variable → Reference →
FunctionCall → ExpressionCall → Constructor → Constant. **Each failing probe
now calls `env.ReportError`** (the A.8 first-pass reporting change), so a bare
literal like `1` or `"x"` accumulates:

```
unknown variable '1'
unknown reference '1'
no native function named '1'
no function named '1' found in this script or imported scripts
cannot resolve type '1'
```

before `Constant` succeeds. This is **non-fatal** (`Script.cpp:57` only prints
when `env.hasErrors`), but:
- Every script using bare literals in expression positions prints this noise.
- `App/ui.lts`, `App/market.lts`, `App/hud.lts` print
  `'App/ui' -- 7 compilation error(s):` on startup (line 7, a `#` comment
  line — the same probe chain on the `#` token).
- It makes real errors impossible to spot in a flood of fake ones.

**Verified** pre-existing (identical output on a clean HEAD checkout,
2026-08-05) — unrelated to the bridge. Fix candidate §6.1.

### 5.2 Inline-script test trap: `StringList_Create` double-wraps

`StringList_Create("(while 1 (return 42))")` from a single line produces an
extra list level, so compilation fails with bewildering errors like
`cannot resolve type '(((while 1 (return 42))))'` and
`'.' field access expects 1 argument`. For inline script compiles in the unit
test harness, build the `StringList` programmatically
(`new StringListAtom("while")`, `new StringListList(...)`) instead. The
`Expression_While_ReturnInLoopBreaks` test shows the working pattern
(`tests/TestScriptCompile.cpp`).

### 5.3 No runtime error channel

- Script blocks return their **last expression** silently; a function that
  errors mid-body returns whatever fell out last with no diagnostic.
- Compile errors go to stdout + log (`Environment.cpp:176`, `ProgramLog.cpp:35`)
  but nothing is routed to the UI.
- `StackFrame_Print` exists but nothing calls it on script failure — a crash in
  a script leaves no stack trace.

### 5.4 Hangs are silent (no watchdog)

The while/return bug (2026-08-04) was an **infinite loop that never
terminated** — `while` had no `returnSignal` check, so `(while 1 (return 42))`
looped forever. The engine just froze: no timeout, no frame counter trip, no
stack dump. Root cause was a missing returnSignal check in `While.cpp`; the
regression test now covers it, but the engine still has **no startup watchdog**
and no per-frame script budget.

### 5.5 Debugging technique that worked: targeted printf + a test

The While fix was found by:
1. Adding temporary `[init]`/`[str]`/`[script]` `printf` markers in the
   engine compile/execute paths.
2. Isolating to a minimal script and reproducing in `lte_tests` instead of the
   full app.
3. Fixing the root cause (returnSignal in `While.cpp`) and encoding it as a
   unit test, then removing every printf.

Lesson for DX: the pain was **where to put the printf** — there was no
script-visible logging API, so every trace meant touching engine C++.
Recommendation §6.2 (bind `Log`/`Print`) removes that friction permanently.

### 5.6 Misc traps catalogued this session

- `String_Split` 2-arg overload is **not bindable** in this build — the
  config parser in `ltheory-main.lts` parses with `Substring`/`Length`
  instead. Bind it, or document the limitation.
- `Config_Get` re-parses the config file on **every call**; call once in
  `Initialize()` and cache (the app already does this).
- `switch -- case ... did not compile` lines now always warn (deliberate,
  Revamp Work) — a flood of "warning" lines with no context. Tolerate or fix
  at the macro sites.
- Old `FunctionAlias(A, B)` two-arg style and new `Function_Alias("A","B")`
  both exist in migration tools; new code must use the new style and keep
  alias-after-source (§3.1).
- `ltsl_api_dump` writes to CWD by default (§3.3).

---

## 6. DX improvements already done (don't re-do)

- [x] **Descriptive compile errors (A.8).** Line numbers, arg counts,
      expected types, "did you mean?" (Levenshtein ≤3), first-pass reporting.
      21 tests in `tests/TestScriptCompile.cpp`.
- [x] **`return` keyword.** `ExpressionReturn` + `returnSignal`; functions
      still return last expression for backward compat.
- [x] **While/return termination fix.** `While.cpp` checks `returnSignal`
      before predicate re-eval and after body; regression test added.
- [x] **Unit-test harness for scripts.** `lte_tests` (369 checks) can compile
      and evaluate script expressions headlessly — the fastest loop for LTSL
      engine bugs.
- [x] **Binding-bridge Step 10 complete.** All `Function_Generated.h` /
      `DeclareFunction.h` macros deleted; `Function_Bind`/`Conversion_Bind`/
      `Function_Alias` are the only mechanism. Alias-order gate, API-DB
      byte-diff, and LSP smoke (6 diagnostics) are the automated gates.

---

## 7. Troubleshooting workflow (recommended, standard)

For any LTSL bug, in order:

1. **Reproduce in `lte_tests` first.** Write a
   `tests/TestScriptCompile.cpp`-style test that compiles + evaluates the
   smallest failing script. Headless, fast, assertable. (This is how the
   while/return hang became a 10-minute fix instead of a long session.)
2. **Check the app's stdout for compile noise** and distinguish spurious
   literal-probe errors (§5.1) from real ones — read the *first* error on a
   line, not the probe flood.
3. **Verify render/update ordering against §4** before touching rendering code.
4. **Add script-visible logs** once §6.2 lands; until then, temporary
   printf in the engine with a `[tag]` prefix, and remove all of it before
   commit (grep for the tag).
5. **Run the gates** before committing:
   - `python3 configure.py build` (green, `-Werror` on project code)
   - `python3 configure.py test` (369 checks, 0 failures)
   - `python3 script/check_binding_alias_order.py $(git ls-files 'src/liblt/**/*.cpp' 'src/liblt/**/*.h')` → `OK: 509 / 1 known`
   - API-DB diff vs `build/api-baseline.json` → 0 added / 0 removed
   - `node script/ltsl-lsp/out/smoke.js $(find resource/script -name '*.lts' | sort)` → exactly 6 diagnostics (4 known unbalanced-paren fixtures + 2 cross-file symbols)
   - `timeout 8 python3 configure.py run <app>` for the affected apps

---

## 8. Hardening roadmap (prioritized)

Chosen direction (2026-08-05): **harden LTSL + rewrite `ltheory-updated.lts`**.
Order below is "biggest DX win per unit of effort".

### P1 — high value, low effort

| # | Item | What / why | Where |
|---|------|------------|-------|
| 1 | **Silent literal probes** | Make the Variable/Reference/FunctionCall/ExpressionCall/Constructor probes return `nullptr` **without** `ReportError` when the atom parses as a literal (number/string/bool) — the Constant factory is the last probe and always wins for literals. Kills the §5.1 noise at its source. | `Expression.cpp` atom path, `Variable.cpp`, `FunctionCall.cpp`, `Constructor.cpp` |
| 2 | **Script-visible logging** | Bind `Log`/`Print` (level + tag + string) to LTSL so apps can trace without touching engine C++ (replaces the §5.5 printf technique). | `ScriptAPI` (e.g. `String.cpp`), `ProgramLog.cpp` |
| 3 | **Runtime error channel** | On script exception/failure, print the LTSL stack via `StackFrame_Print` and route a message to the debug overlay (F3). Turns silent last-expression returns into diagnosable failures. | `StackFrame.cpp`, `Widget/DebugScene.lts` |
| 4 | **Startup watchdog** | A watchdog (frame counter or wall-clock) that trips if the app's `Update` runs too long / never returns — with a stack dump. Would have caught the while/return hang in seconds. | `Program.cpp`, `launch.cpp` |

### P2 — medium value/effort

| # | Item | What / why |
|---|------|------------|
| 5 | **Explicit-return strict mode** | Optional warning when a non-`Void` function body has no `return` (relies on last-expression fallback). Catches silent-wrong-value bugs. |
| 6 | **`#` comment line attribution** | `#` comment lines currently feed tokens into the probe chain (§5.1 observed as "line 7" errors); strip comments before compile or fix `LineNo` attribution so errors point at real code. |
| 7 | **`StringList_Create` single-line** | Make single-line input not double-wrap (§5.2) so inline script tests stop producing `cannot resolve type '(...)'` noise. |
| 8 | **Bind `String_Split` 2-arg** | Remove the `Substring`/`Length` workaround in config parsing. |
| 9 | **Fix `ltsl_api_dump` docs** | AGENTS.md `>` redirect trap (§3.3); document the output-path arg. |

### P3 — roadmap items already tracked elsewhere

| # | Item | Where |
|---|------|-------|
| 10 | LTSL hot-reload (file watcher) | AGENTS.md §10.3, ROADMAP |
| 11 | Data-driven UI / JSON layouts | AGENTS.md §6.1, §10.4, ROADMAP |
| 12 | List methods (`.Filter`, `.Map`) + foreach | AGENTS.md §6.1, LTSL roadmap |
| 13 | DevPanel live tweaking of universe gen params | AGENTS.md §8.3 Pass C, ROADMAP |

---

## 9. Open questions (to resolve before the P1/P2 work)

- Should the probe-silencing fix (§6.1/§8.1) treat **all** atoms that a literal
  factory accepts as literals, or only numbers/strings/bools? (Vectors like
  `Vec3` are constructors, not literals — out of scope.)
- Explicit-return strict mode: default **off** with a config flag, or on with a
  suppress comment? (Backward compat favors default off.)
- Watchdog trip action: log+abort (fail loud) vs log+resume (skip the hung
  frame)? Recommend log+abort for now — silent hangs are worse than a crash.
- Which log level mapping? Proposal: `Log` (info), `Log_Warn`, `Log_Error`
  mirroring `ProgramLog`'s existing severities.

---

## 10. Change log

- **2026-08-05** — Created. Captures the binding-bridge Step 10 session (Phase
  A–D gates), the while/return fix, the literal-probe noise finding, and the
  ordering/priority rules (§2–4) verified against source. Hardening priorities
  P1–P3 set.
