# LTSL Migration Plan: From Tree-Walker to Proper Compiler

## Why This Exists

The current LTSL interpreter has fundamental design flaws that block rapid
content creation. Two bugs from this project specifically validate the
proposed solution:

**Bug 1: Indent-level mismatch silently accepted.** In `Widget/HUD.lts` and
`Object/SystemPopulate.lts`, a block dedented to a level that didn't match
any enclosing scope. LTSL silently accepted it as valid code with different
meaning instead of erroring. A proper indent-stack lexer — the way Python
actually works — maintains an indent stack and throws a hard error the
moment a dedent doesn't match a level on the stack. This would have caught
the bug at compile time instead of requiring manual diagnosis.

**Bug 2: Type mismatch silently accepted.** `Widgets:Text ... 0.7` (a bare
Float in a color slot) and `RenderPass_Clear 0` (a bare Int in a Vec4
slot) compiled without error. These were two of the harder bugs to track
down precisely because LTSL fails silently on type mismatches. Compile-time
type checking closes this entire bug class.

Additional pain points (each grounded against the ~160-file corpus and engine
source; see §Corpus Findings):
- **Silent statement drops** — failed arity checks silently discard code.
  `Expression_Block` drops failed expressions (`Function.cpp:173-175`), so a
  typo'd call like `this.DoSave` compiles to nothing and the app "works" with
  zero diagnostics. The single biggest debugging time-waster.
- **`a.b` rewrite confusion** — method call order is unintuitive: `this.DoSave`
  rewrites to `(DoSave this)`, and a type method declared `function Void
  DoSave (Widget self)` expects the receiver as its *first* param, so calling
  it with one arg fails an arity check. This trap silently drops the statement;
  a Phase-3 arity checker would catch it automatically.
- **`~` not negation** — `(Vec3 (~ x) 0 0)` does nothing. **Corpus grep (all
  ~160 files):** `~` appears *only* in comments meaning "approximately"; zero
  operator uses. No pain to fix; QW1 is therefore optional (see §Corpus Findings).
- **Terse error messages** — "Unused variable in Shader(...)" with no location
  info. A.8 improved name-resolution errors but left type/arity mismatches open
  (QW3 covers the rest).

**Goal:** Replace the tree-walking interpreter with a proper two-pass
compiler that eliminates these problems while preserving the engine
integration (AutoClass reflection, FunctionBind, Function_Alias).

**Non-goal:** Replacing LTSL with Lua or another language. The engine's
reflection system is deeply coupled to LTSL. A rewrite of the language
itself is more practical than a rewrite of 160+ script files plus the
engine bridge. Both the compiler migration and the binding-bridge plan
preserve the FunctionBind/Function_Alias engine bridge untouched.

---

## Strategy: Quick Wins First, Then Full Rewrite

Claude Code's review identified that the Quick Wins section covers most of the
user-reported pain points and can ship in ~1 week vs ~9 weeks for the full rewrite.
The recommended sequence:

1. **Ship Quick Wins first** (Phase 0, ~1 week) — QW2/QW3/QW5 mandatory; QW1/QW4
   optional/deferred per §Corpus Findings
2. **Use them for real content work** (2-4 weeks)
3. **Decide whether the full rewrite is still justified** based on whether
   remaining pain points justify ~9 weeks of work (see §Corpus Findings — the
   answer for "ordering sensitivity" is narrower than assumed)
4. **If yes: full corpus audit** (Phase 0.5, ~1 week) — inventory every LTSL
   construct in actual use, determine breaking-change scope, scope the real
   parser work

This is the right call because:
- Several Quick Wins are literal subsets of Phase 1 (unary-minus token,
  line/column tracking). They're not wasted work if the full rewrite happens —
  they're forced familiarity with `Expression.cpp` before replacing it.
- Landing them gives a real signal: does the pain actually go away?
- **Ordering sensitivity** requires a symbol table *only* for intra-scope top-level
  forward references (call `foo()` before `function foo()`). Cross-file forward
  refs already work via lazy runtime load + dependency tracking; methods work
  via type pre-registration (`Function.cpp:90-93`). No corpus script calls a
  same-scope top-level function before declaring it. So this is a real but
  **corpus-inert** gap — it should not alone justify the rewrite (see §Corpus
  Findings). The `a.b` ordering confusion, by contrast, *is* worth fixing and
  Phase 3's arity checker catches it for free.


---

## Corpus Findings (verified against all ~160 `.lts` files + engine source)

These findings replace earlier estimates/assumptions with data gathered before the
Phase-0.5 audit would have produced them:

### Verified — keep as written
- **`#` block-comment is a real, shipped mechanism** (`StringList.cpp:96-117`). A `#`
  line comments to end of line *and* every deeper-indented line beneath it (e.g.
  `# desc "X"` dead-blocks). Switching to single-line `#` would silently compile
  disabled blocks — the plan's Phase 0.5 concern is valid and confirmed. Keep the
  independent `##`-disambiguation migration track.
- **Special forms are used:** `deref`=16, `address`=11, `desc`=11, `static`=11,
  `call`=4 files (all confirmed present → all must be handled in the grammar).
- **`ref`/`static` are local-scope-bound** (`Expression.cpp:170`, `Declare.cpp`) — the
  resolver's Scope model must replicate this or ref aliasing breaks.
- **`desc` vs `block` arity trap** is real: both route through `Expression_Block(list, env, N)`
  but block=1 vs desc=2 (`Function.cpp:165`). Two-pass landmine — keep explicit handling.

### Corrected / downgraded (were over-stated)
- **Pain point #1 (`~` negation): no such bug.** `~` appears *only* in comments meaning
  "approximately"; zero operator uses across all files. QW1 has nothing to fix — make it
  optional for future-proofing only.
- **QW4 (`for i in range a b` sugar): no corpus demand and no target.** No `range`
  construct is used (every `in` match is prose) and no integer-range iterator function is
  bound, so there is nothing to desugar into. Defer until the audit confirms real use.
- **Pain point #4 / "order-dependent compilation":** real but corpus-inert (see Strategy).
  It should not alone justify ~9 weeks of parser work — especially since the most expensive
  phase fixes a case no existing script relies on, while introducing side-effect-reordering
  risk under `-fno-exceptions`.

### Confirmed unused / to drop from audit scope
- `TOK_SIZEOF`, `TOK_TYPEOF` are declared but never used in the corpus — confirm whether to
  remove them before writing the lexer.

---

## Phase 0: Quick Wins (~1 week)

These fix the worst pain points in the current interpreter. Each is
1-3 days, low-risk, and can ship independently.

### QW1: Negation operator (optional — 1 day)

Add `-x` (unary minus) as an inline prefix in contexts like `(Vec3 (- x) 0 0)`.
Currently `-x` only works wrapped as a function call (`(- x)`); the bare form is
not recognized in nested argument slots.

> **Optional, not required.** Corpus grep of all ~160 files shows `~` (the token
> that would signal an inline-prefix negation) appearing *only* in comments — zero
> operator uses. No content author hit this bug, so there is no pain to fix. Add
> it for completeness / future-proofing only; see §Corpus Findings. If the full
> rewrite happens, the unary-minus token lands there anyway (Phase 2 precedence
> table, highest level).

**Changes:**
- `Expression.cpp`: recognize `-` as a unary prefix when followed by an
  identifier/literal/operator
- `Expression_UnaryMinus` new node (or reuse the existing `(- x)` path)

### QW2: Silent statement drop warning (1-2 days)

> **DONE (A.15, 2026-08-25) — verified already hardened.** Block.cpp aborts
> blocks on failed statements with recorded errors (no silent drops), and
> `ReportError` includes line numbers. The compile gate + regression tests
> pin both behaviors; no code change was needed.

When an arity check fails in `Expression_Block`, log a warning with
line number instead of silently dropping the statement. This is the
single biggest debugging time-waster.

**Changes:**
- `Block.cpp`: after failed statement compilation, emit
  `env.ReportError` with the line number and "arity mismatch" message
- `FunctionCall.cpp` / `ExpressionCall.cpp`: on arity failure, log
  which argument count was expected vs received

### QW3: Type/arity error messages with line/column (2-3 days)

> **DONE (A.15, 2026-08-25) — trimmed per the scope guard below.** The one
> real remaining defect was in `ExpressionCall.cpp`: argument-type-mismatch
> diagnostics read `arguments.back()` (previous argument's type; UB on empty
> vector when argument 1 failed). Fixed to capture the source type before the
> conversion attempt and report at the failing sub-expression's line.
> Arity messages and line-number plumbing were already shipped by A.8 +
> ltsl-hardening work.

A.8 already shipped "did you mean?" suggestions and `env.ReportError`
for **name-resolution** errors (Variable, Reference, Constructor,
FunctionCall, Conversion). This is the *remaining* coverage — type and
arity mismatches, which A.8 did not address:
- Type mismatch ("expected Vec3d, got Float at line N")
- Arity mismatch ("SetPos expects 2 args, got 3 at line N")
- Unknown member access ("Object has no field 'positon' — did you
  mean 'position'?")

**Note:** The existing "did you mean?" suggestions use Levenshtein
distance ≤ 3 via `BestMatch()` in `Environment.h:67-108`. This
already works for name resolution; we're extending coverage to the two
mismatch classes A.8 left open.

> **Scope guard:** Do not re-implement A.8's name-resolution work — it
> is complete (21 tests in `TestScriptCompile.cpp`). If QW3 feels like
> re-doing shipped code, trim to just the arity messages; they're the
> cheapest and highest-signal of the three.

### QW4: `for` loop syntax improvement (1 day)

Current `for` requires 6 elements: `(for name init pred step body...)`.
The `i.++` step syntax is confusing. Add sugar **while keeping the
existing form** (it's more general — arbitrary predicates and custom
steps):
```
for i in range 0 512
  body
```

> **Deferred pending Phase-0.5 audit.** Two blockers found against the corpus:
> (1) there is *no* `range` construct used anywhere — every `in` occurrence is a
> prose comment, not code; and (2) no integer-range iterator function is bound, so
> this sugar has no natural target to desugar into. It would require either inventing
> an iterator type or hand-rolling the loop body in the parser. Because nothing in
> the ~160-file corpus exercises it, defer until the audit confirms real demand (it
> currently does not).

This is sugar only, not a language change — it desugars to the existing
`for` form. The old form stays for loops that need non-linear iteration
(arbitrary predicate, custom step, or non-integer ranges).

### QW5: Array literal syntax (1 day)

> **DONE (A.15, 2026-08-25).** Implemented as `[a, b, c]` with type inferred
> from the first element (NOT as `(Array 1 2 3)` sugar — that form stays the
> explicit-typed empty-array constructor used by `var x (Array T)`).
> Tokenizer marks bracket groups `(__bracket ...)`; new
> `Expression/ArrayLiteral.cpp` node; helpers `Type_ArrayAlloc/Append/Size/Get`
> in `Type/Array.cpp`. Commas split elements inside brackets; parens inside
> keep space separation (`[(Vec2 1 2)]`).

Add `[1, 2, 3]` as sugar for `(Array 1 2 3)`. The bracket syntax
is standard across every other language.

### QW6: `switch` improvements (1-2 days)

Current switch is limited to 2 cases + otherwise. Add multi-case
support:
```
switch
  choice == 1 body1
  choice == 2 body2
  choice == 3 body3
  otherwise body4
```
This already works in LTSL (cases are inline pairs), but the error
recovery is fragile. Improve it.

> **Overlap guard.** A.8/A.9 already hardened `switch` error recovery —
> previously-silenced `switch -- case` warnings now always report, and switch
> is covered by existing tests in `TestScriptCompile.cpp`. Before spending time on
> QW6, confirm it isn't re-doing shipped work; if so, trim to the multi-case
> ergonomics only and point at the appendix regression suite.

---

## Phase 0 Gate: Ship, Test, Decide

After shipping Quick Wins:
1. Run all ~160 `.lts` files through the improved interpreter
2. Verify no regressions (same behavior as before for working scripts)
3. Use the improved scripts for 2-4 weeks of real content work
4. **Decision point:** Do ordering sensitivity still hurt enough to justify the
   full rewrite? (Per §Corpus Findings, this now means only *intra-scope top-level*
   forward references — cross-file/method refs already work and no corpus script relied
   on it. If real content authors start hitting that case in volume, proceed; otherwise
   stay on the improved interpreter.)

If yes → proceed to Phase 0.5. If no → stay on improved interpreter.

---

## Phase 0.5: Full Corpus Audit — COMPLETE (2026-08-25)

> **Done.** All deliverables below are complete. Feeds Phase 1 (lexer token
> list), Phase 2 (grammar — especially special forms), Phase 3 (symbol
> resolver scope), and the `#` migration path.

### Corpus stats

- **157 `.lts` files** (after removing 3 dead files in Phase 0)
- 155 files define functions; 127 files use `var`; 135 files define types

---

### A. `#` block-comment audit

**Engine behavior** (`StringList.cpp:96-117`): a `#` at the start of a line
(in possibly preceded by whitespace) comments out that line AND all subsequent
non-empty lines indented MORE than the `#` line. Stops at the first line with
indent ≤ the `#` line.

**Result:** 495 total `#` occurrences across 157 files.

| Classification | Count | % |
|---|---|---|
| **SINGLE-LINE** (safe — trailing comment, or followed by same/lesser indent) | 478 | 96.6% |
| **BLOCK-DISABLING** (breaks if `#` becomes single-line) | 16 | 3.2% |
| **EMPTY** (`#` alone, no code after, no indented block below) | 1 | 0.2% |

**Verdict:** Block-comment `#` usage is **rare** (16 occurrences). The
migration path is: mechanically rewrite the 16 offending files, then
`#` = single-line only going forward. No need for `##` transition period.

#### BLOCK-DISABLING occurrences (complete list)

| # | File:Line | `#` line text | Lines killed | Risk | What's disabled |
|---|---|---|---|---|---|
| 1 | `App/widget.lts:31` | `#` | 13 | **HIGH** | F4 settings toggle — entire `if Key_F4.Pressed` block |
| 2 | `App/brain.lts:96` | `# Add synch node` | 4 | **HIGH** | Right-click synapse handler (`if Mouse_RightPressed`) |
| 3 | `Widget/Handling.lts:91` | `#` | 5 | **HIGH** | Camera follow-offset math (position decay) |
| 4 | `Widget/HUD.lts:54` | `# l +=` | 4 | MED | PilotingBadge widget tree |
| 5 | `Widget/GridList.lts:41` | `#` | 16 | MED | Grid-layout path (cell wrapping, child placement) |
| 6 | `Widget/GridList.lts:77` | `# l +=` | 6 | MED | GreedyY widget tree |
| 7 | `Widget/Object/Overview.lts:290` | `# l +=` | 6 | MED | SignatureWidget |
| 8 | `Widget/DevPanel/Clock.lts:45` | `# l +=` | 3 | MED | Clock icon |
| 9 | `Widget/DevPanel/Clock.lts:50` | `# Components:Expand` | 4 | MED | Month/year text |
| 10 | `Widget/Reticle/Default.lts:16` | `# desc "Center Reticle"` | 4 | MED | 2-arc center reticle |
| 11 | `App/observatory.lts:57` | `# desc "Miners"` | 9 | LOW | Miner-ship spawn block |
| 12 | `Widget/Spacer.lts:6` | `#` | 3 | LOW | Debug corner dots (SpacerH) |
| 13 | `Widget/Spacer.lts:19` | `#` | 3 | LOW | Debug corner dots (SpacerV) |
| 14 | `Widget/ImageEditor.lts:51` | `# image =` | 2 | LOW | Median filter (inside `if false`) |
| 15 | `Widget/RadialList.lts:9` | `# function List CreateChildren` | 1 | LOW | Old function signature |
| 16 | `Widget/RadialList.lts:43` | `# function List CreateChildren` | 9 | LOW | Full CreateChildren body |

**HIGH risk** = disables live behavioral logic (3 files). **MED** = disables
UI/layout (6 files). **LOW** = disables dead/debug code (5 files).

#### Migration path

1. **Rewrite the 16 occurrences** — either uncomment the dead code (if it
   should live), or convert to explicit deletion / `##` form. Each file is
   a case-by-case decision.
2. After rewriting: `#` = single-line comment only, going forward.
3. No `##` double-hash exists in the corpus today — zero occurrences.
   If the engine keeps `##` as block-comment syntax, only the 16 rewritten
   lines need it.

---

### B. Special-forms inventory

**Source of truth:** `Expression.cpp:144-189` — the engine's keyword dispatch
table. These are NOT ordinary functions; they are prefix keywords parsed
specially by the interpreter.

| Keyword | Occurrences | Files | Status |
|---|---|---|---|
| `var` | 1,567 | 127 | Ubiquitous — most-used keyword |
| `function` | 902 | 155 | Nearly universal |
| `=` (assignment) | ~628 | ~101 | Inline assignment form |
| `if` | 522 | 92 | Heavily used |
| `for` | 250 | 72 | Heavily used |
| `switch` | 96 | 54 | Heavily used |
| `type` | 313 | 135 | Nearly universal |
| `cast` | 96 | 44 | Heavily used |
| `otherwise` | 72 | 42 | Used with `switch` |
| `static` | 70 | 11 | Moderate (mostly Icons.lts + Texture/Filters.lts) |
| `ref` | 147 | 49 | Heavy — mutable parent→child state |
| `desc` | 47 | 11 | Moderate — code organization blocks |
| `deref` | 36 | 16 | Moderate — always paired with `address`/`ref` |
| `block` | 39 | 3 | Rare (mostly Icons.lts) |
| `address` | 26 | 11 | Moderate — widget mutable-state pattern |
| `?` (switch expr) | 29 | 14 | Inline switch expression form |
| `while` | 15 | 11 | Rare |
| `else` | 15 | 6 | Rare |
| `return` | 13 | 7 | Light (revamp-work addition) |
| `@` (debug print) | 7 | 6 | Debug only |
| `break` | 3 | 1 | Only ltheory-unitest.lts |
| **`call`** | **0** | **0** | **UNUSED — can drop from grammar** |
| **`list`** | **0** | **0** | **UNUSED — can drop from grammar** |
| **`set`** | **0** | **0** | **UNUSED — only `=` form used** |
| **`sizeof`** | **0** | **0** | **Does not exist in engine** (no handler in Expression.cpp) |
| **`typeof`** | **0** | **0** | **Does not exist in engine** (no handler in Expression.cpp) |

**Grammar implications:**
- 5 keywords can be dropped from the new grammar: `call`, `list`, `set`,
  `sizeof`, `typeof` (last two don't even have engine handlers).
- `address`/`deref`/`ref` form a tight cluster (mutable state) — grammar
  must handle them as a unit.
- `desc` and `block` both dispatch to `Expression_Block` with different
  skip counts (2 vs 1) — the parser must model this.
- `@` is a prefix operator (not a function call) — the lexer must recognize
  it as `TOK_AT` or similar.

---

### C. Cross-file dependency map

**Hub files** (most-referenced — cutting these cascades across the corpus):

| Rank | Deps | File | Role |
|---|---|---|---|
| 1 | 118 | `App/strukt.lts` | App shell framework — every app inherits from it |
| 2 | 113 | `App/brain.lts` | Neural-net background animation |
| 3 | 90 | `Widget/Components.lts` | Widget component library (40 facades) |
| 4 | 77 | `Colors.lts` | Color palette |
| 5 | 64 | `Config.lts` | Config reader (gameConfig.txt) |
| 6 | 63 | `App/handling.lts` | Ship-handling physics |
| 7 | 59 | `Fonts.lts` | Font registry |
| 8 | 59 | `Widget/Dev/FrameInfo.lts` | Dev overlay (FPS, draw calls) |
| 9 | 56 | `Widgets.lts` | Widget barrel file (re-exports) |
| 10 | 45 | `App/draw.lts` | Drawing utilities |

**Architecture layers:**

```
  App/strukt.lts + App/brain.lts    ← gravity wells (App type base)
  ═══════════════════════════════════
  Widget/Components.lts + Colors/Config/Fonts  ← shared toolkit
  ═══════════════════════════════════
  App/draw + App/handling + App/map  ← utilities
  ═══════════════════════════════════
  Individual widgets + objects + apps ← leaf consumers
```

**Circular dependencies:** 22 pairs found, all safe. LTSL compiles each
file independently via lazy-load; circular load-order is resolved by the
engine's file cache. The `Widgets.lts ↔ Widget/*.lts` circles are the
classic barrel-file pattern.

**`ltheory-main.lts` depends on 19 files:** App/brain, colony, draw,
launcher, model, strukt, Colors, Config, Fonts, Icon/Cursors,
Object/Colony, Object/Ship, Object/SystemPopulate, Texture/Filters,
Widget/DebugScene, Widget/Dev/FrameInfo, Widget/HUD, Widget/Pause,
Widget/Toast.

---

### D. Construct inventory summary

All distinct LTSL constructs found in the corpus (feeds Phase 1 token list):

**Keywords:** `var`, `ref`, `static`, `function`, `return`, `if`, `else`,
`while`, `for`, `switch`, `otherwise`, `type`, `cast`, `break`, `desc`,
`block`, `address`, `deref`, `call` (unused), `set` (unused), `list` (unused)

**Operators:** `+` `-` `*` `/` `%` `==` `!=` `<` `>` `<=` `>=` `&&` `||`
`!` `.` `=` `+=` `-=` `*=` `/=`

**Delimiters:** `(` `)` `[` `]` `,` `:` `#`

**Prefix operators:** `@` (debug print)

**Literals:** integers, floats, strings (single-quoted `'a b'`), booleans
(`true`/`false`), null

**Patterns:** `a.b` dot-chain (rewritten to `(b a)`), `i.++` postfix
increment (rewritten to `(++ i)`), `debugVisible.!` postfix boolean negate
(rewritten to `(! debugVisible)`), `a ? b : c` inline switch expression,
`[a, b, c]` array literals, `(address refvar)` mutable pointer pattern

---

## Phase 1: Lexer (Token Scanner) — COMPLETE (2026-08-25)

**Input:** Source code string
**Output:** Token stream with line/column tracking + indent stack

### Design Decisions

1. **Indentation-only blocks** — No brace syntax. Indentation is the
   existing convention and the whole ecosystem writes it that way.
   Braces are NOT offered as an alternative. The grammar has one way
   to do blocks: INDENT/DEDENT.

2. **Hard indent-stack enforcement** — The lexer maintains an indent
   stack (like Python). A dedent that doesn't match a level on the
   stack is a HARD ERROR, not a warning. This catches the
   `HUD.lts`/`SystemPopulate.lts` class of bugs at lex time.

3. **Every token carries line/column** — Enables precise error messages.

4. **Tab/space indents are equivalent.** Tab treated as 4 spaces for
   measurement. No spurious dedent errors on mixed indentation.

5. **Multi-line paren groups span newlines.** `(` / `)` may open on one line and
   close on another (every multi-line expression in the corpus relies on this).
   `parenDepth` counter defers INDENT/DEDENT until balanced-paren boundary.
   Same for `[` / `]`.

6. **`#` = single-line comment only.** Block-comment behavior (16 occurrences)
   must be mechanically rewritten in Phase 0.5 migration before the new lexer
   can be wired in as the sole tokenization path.

7. **Postfix operators split.** `i.++` lexes as `IDENT DOT PLUS PLUS`;
   `debugVisible.!` as `IDENT DOT NOT`. The parser handles rewriting
   to `(++ i)` / `(! debugVisible)` — the lexer stays simple.

```cpp
enum TokenKind {
  // Literals
  TOK_INT, TOK_FLOAT, TOK_STRING, TOK_BOOL, TOK_NULL,

  // Identifiers
  TOK_IDENTIFIER,

  // Operators
  TOK_PLUS, TOK_MINUS, TOK_STAR, TOK_SLASH, TOK_MOD,
  TOK_EQUALS, TOK_NOT_EQUALS, TOK_LESS, TOK_GREATER,
  TOK_LESS_EQUALS, TOK_GREATER_EQUALS,
  TOK_AND, TOK_OR, TOK_NOT,

  // Assignment
  TOK_ASSIGN, TOK_PLUS_ASSIGN, TOK_MINUS_ASSIGN,
  TOK_MULTIPLY_ASSIGN, TOK_DIVIDE_ASSIGN,

  // Delimiters
  TOK_LPAREN, TOK_RPAREN, TOK_LBRACKET, TOK_RBRACKET,
  TOK_COMMA, TOK_COLON, TOK_DOT,

  // Keywords
  TOK_VAR, TOK_REF, TOK_STATIC, TOK_FUNCTION, TOK_RETURN,
  TOK_IF, TOK_ELSE, TOK_WHILE, TOK_FOR,
  TOK_SWITCH, TOK_OTHERWISE,
  TOK_BREAK, TOK_TRUE, TOK_FALSE,
  TOK_TYPE, TOK_CAST, TOK_BLOCK, TOK_DESC,
  TOK_ADDRESS, TOK_DEREF, TOK_AT,

  // Special
  TOK_NEWLINE, TOK_INDENT, TOK_DEDENT, TOK_EOF, TOK_UNKNOWN
};
```

### Lexer Architecture

```
class Lexer {
  String const& source;
  size_t pos;
  int line;
  int column;
  Vector<int> indentStack;   // Indent levels (starts with [0])
  Vector<Token> pending;     // Buffered indent tokens
  bool atLineStart;
  bool hasTokensOnLine;      // Suppresses NEWLINE on comment-only lines
  int parenDepth;            // Defers INDENT/DEDENT inside ()/[]

  // Core
  Vector<Token> Tokenize();

  // Character helpers
  char Peek(); char Peek2(); char Advance(); bool AtEnd();
  void SkipHorizontalWhitespace();
  void SkipSingleLineComment();

  // Token readers
  Token ReadNumber();
  Token ReadString();
  Token ReadIdentifier();
  Token ReadOperator();

  // Indent handling
  int MeasureIndent();
  void EmitIndentTokens(int newIndent);
  void EmitPendingTokens();
  void Emit(Token const& tok);

  // Error reporting
  void ReportError(String const& message);
};
```

### Completed (Phase 1)

- [x] `src/liblt/LTE/Lexer.h` — TokenKind enum, Token/LexError/Lexer class declarations
- [x] `src/liblt/LTE/Lexer.cpp` — Full implementation: character scanning, indent stack,
      `#` single-line comments, multi-line paren/bracket span, all token readers
- [x] `tests/TestLexer.cpp` — 35 tests, 1161 checks, 0 failures
- [x] Builds clean via `python3 configure.py build`; existing tests pass

### Key implementation details

- `hasTokensOnLine` flag: NEWLINE tokens only emitted when real tokens have
  appeared on the current line. Comment-only and blank lines produce no NEWLINE.
- `MeasureIndent()` saves/restores `pos`/`column` — returns indent level without
  consuming characters; `SkipHorizontalWhitespace()` called after
  `EmitIndentTokens()` to actually advance past the measured whitespace.
- `Emit()` helper marks `hasTokensOnLine = true` for non-structural tokens
  (everything except NEWLINE/INDENT/DEDENT/EOF).

### What Changes vs Current Parser

| Current behavior | New lexer behavior |
|---|---|
| Line-based + indent comparison → StringList tree | Character-based → flat token stream |
| No INDENT/DEDENT tokens (indent is implicit) | Explicit TOK_INDENT/TOK_DEDENT |
| `#` comments out entire block below | `#` = single-line comment (standard); block-comment uses `##` if needed (see Phase 0.5) |
| `a.b` rewrite at parse time | `.` is TOK_DOT, rewrite in parser phase |
| No `~` operator | `~` is TOK_UNKNOWN (error) |
| `-` always binary | `-` is TOK_MINUS; context resolves unary |
| Atoms classified by probe-chain | Tokens classified by lexer |

---

## Phase 2: Parser (AST Builder) ✅ COMPLETE

**Input:** Token stream from Phase 1
**Output:** Abstract Syntax Tree

### Grammar (EBNF)

```
module        = statement*

statement     = varDecl | refDecl | funcDecl | typeDecl
              | returnStmt | breakStmt | continueStmt
              | ifStmt | whileStmt | forStmt | switchStmt
              | assignStmt | exprStmt

varDecl       = 'var' IDENTIFIER expr
refDecl       = 'ref' IDENTIFIER expr
funcDecl      = 'function' typeName IDENTIFIER '(' paramList ')' block
typeDecl      = 'type' IDENTIFIER typeBody
typeBody      = INDENT (fieldDecl | funcDecl)+ DEDENT
fieldDecl     = typeName IDENTIFIER [expr]

ifStmt        = 'if' expr block ('else' (ifStmt | block))?
whileStmt     = 'while' expr block
forStmt       = 'for' IDENTIFIER ('in' expr | expr expr expr '.++') block
switchStmt    = 'switch' INDENT switchCase+ ('otherwise' block)? DEDENT
switchCase    = expr block

assignStmt    = lvalue assignOp expr
exprStmt      = expr

block         = INDENT statement+ DEDENT
expr          = binaryExpr
binaryExpr    = unaryExpr (binOp unaryExpr)*
unaryExpr     = ('-' | '!') unaryExpr | postfixExpr
postfixExpr   = primaryExpr ('.' IDENTIFIER ['(' argList ')'])*
primaryExpr   = INT | FLOAT | STRING | BOOL | NULL
              | IDENTIFIER | '(' expr ')' | arrayLiteral
              | typeConstructor | castExpr | addressExpr | derefExpr
              | funcCall

typeConstructor = typeName '(' argList ')'
castExpr      = 'cast' typeName expr
addressExpr   = 'address' expr
derefExpr     = 'deref' expr
arrayLiteral  = '[' [exprList] ']'
funcCall      = IDENTIFIER '(' argList ')'

> **Special-form atoms** — `@`, `block`, `desc`, `call`, `static` are also
> primary expressions (they produce expression nodes, like `cast`). They are
> NOT in the grammar above because they share the identifier-dispatch path;
> list them explicitly so the parser does not silently drop them:
>   specialFormAtom = '@' | 'block' | 'desc' | 'call' | 'static'

### Special Forms Inventory (reference behind the Phase 0.5 deliverable)

Before grammar completion is claimed, enumerate **every** engine special form
and its corpus usage count. A script using an unlisted form cannot parse at all,
so the Phase 2 checkpoint is meaningless until this is done:

| Form | Node (`Expression.cpp`) | Corpus files (pre-audit grep) | Grammar status |
|---|---|---|---|
| `cast` | `Expression_Cast` | ~10+ | Covered above |
| `address` | `Expression_Address` | **11** | Added above (was missing) — confirmed used |
| `deref` | `Expression_DereferencePointer` | **16** | Added above (was missing) — confirmed used |
| `@` print | `Expression_Print` | unverified — audit required | Optional; drop if unused. Grep is noisy (`@` in strings/emails); confirm real uses before deciding dead. |
| `block` | `Expression_Block(...,1)` | many | Covered by INDENT/DEDENT blocks |
| `desc` | `Expression_Block(...,2)` | **11** (confirmed) | **Arity trap** — see below |
| `call` | `Expression_DynamicDispatch` | **4** (confirmed) | Used in 4 files — not dead; must be handled, audit for exact form |
| `static` | `Expression_DeclareStatic` | **11** (confirmed) | Covered by symbol resolver |
| `ref` | `Expression_DeclareReference` | many | Covered by symbol resolver |

> **Corpus grounding.** The counts above are file-level greps across all ~160
> `.lts` files, run before the Phase-0.5 audit. They replace earlier estimates:
> `address`=11 and `deref`=16 (both "was missing" from the grammar),
> `desc`=11, `static`=11, `call`=4 are **confirmed present**, so all must be
> handled — not treated as optional. Tokens declared but unused in corpus:
> `sizeof`, `typeof` (`TOK_SIZEOF`, `TOK_TYPEOF`) — audit should confirm whether to
> drop them from the lexer.


**`desc` vs `block` arity trap:** both route through `Expression_Block(list, env, N)`
but with **different arg counts — block=1, desc=2**. If the new parser treats them
as identical INDENT/DEDENT blocks and passes the wrong arity, it silently breaks one.
The resolver must preserve this distinction explicitly (it is not a "quirk" to be
discovered; it is a two-pass rewrite landmine).

### Evaluation ordering model (Phase 3 design rule)

Pin down the current evaluation semantics before building the symbol table:

- **`ref` / `static` are local-scope-bound**, not global symbols — both take a
  `locals` param (`Expression.cpp:170`, `DeclareStatic`/`DeclareReference` in
  `Declare.cpp`). The resolver's `Scope` struct must replicate this local-binding
  model, or ref aliasing (heavily used in `ltheory-main.lts`) breaks.
- **Eager vs lazy init** — state the current var/ref/static initialization timing;
  two-pass compilation changes declaration collection order and can reorder side
  effects even for scripts that compile cleanly. Document any intentional change.

> **Hard rule:** The engine builds with `-fno-exceptions` (zero `try`/`catch`/`throw`
> anywhere). All error recovery in the resolver/parser is via struct + vector only —
> never exceptions.

binOp         = '+' | '-' | '*' | '/' | '==' | '!='
              | '<' | '>' | '<=' | '>=' | '&&' | '||'
              | '=' | '+=' | '-=' | '*=' | '/='
assignOp      = '=' | '+=' | '-=' | '*=' | '/='

typeName      = IDENTIFIER ['/' IDENTIFIER]* ['<' genericArg '>']
genericArg    = typeName (',' typeName)*
argList       = [expr (',' expr)*]
paramList     = [typeName IDENTIFIER (',' typeName IDENTIFIER)*]
```

### Key Design Decisions

1. **Method calls parsed directly** — `obj.Method(args)` is parsed as
   a method call, not rewritten. The parser understands `.`-chain syntax.

2. **Operator precedence via Pratt parsing** — not rewrite passes.
   Precedence table:

   | Precedence | Operators | Associativity |
   |---|---|---|
   | 8 (highest) | `-` (unary), `!` | right |
   | 7 | `*`, `/` | left |
   | 6 | `+`, `-` (binary) | left |
   | 5 | `<`, `>`, `<=`, `>=` | left |
   | 4 | `==`, `!=` | left |
   | 3 | `&&` | left |
   | 2 | `\|\|` | left |
   | 1 (lowest) | `=`, `+=`, `-=`, `*=`, `/=` | right |

3. **Indentation-only blocks** — `block = INDENT statement+ DEDENT`.
   No brace syntax.

4. **`for` loop sugar** — `for i in expr expr` desugars to existing
   `for` form. Backward-compatible.

---

## Phase 3: Symbol Resolver + Type Checker ✅ COMPLETE (2026-08-26)

**Input:** Raw AST from Phase 2
**Output:** Typed AST with resolved references
**Implementation:** `src/liblt/LTE/SymbolResolver.h` + `src/liblt/LTE/SymbolResolver.cpp`

### Architecture

The resolver uses a **single-pass declare+resolve** approach (not the originally planned two-pass design):

1. **PreScanDeclarations** — registers file-level functions and types only (no scope management, no nested functions), enabling forward references.
2. **ResolveAndDeclare** — combined single pass that declares and resolves in one walk, so the same scope objects hold symbols and are queried.

```cpp
struct Symbol {
  String name;
  SymbolKind kind;  // Var, Func, Type, Param
  String declaredType;
  int arity;
};

struct Scope : public RefCounted {
  Reference<Scope> parent;
  Map<String, Symbol> symbols;
};

class SymbolResolver {
  Reference<Scope> currentScope;
  Vector<CompileError> errors;
  
  // Pre-scan for forward references
  void PreScanDeclarations(ASTModuleNodeT* module);
  
  // Single-pass declare+resolve
  void ResolveAndDeclare(ASTNode* node);
  
  // Helper methods
  void PushScope();
  void PopScope();
  void DeclareSymbol(String const& name, SymbolKind kind, ...);
  Symbol* LookupSymbol(String const& name);
  void ResolveAndDeclareFunction(ASTFuncDeclNodeT* node);
  void ResolveAndDeclareType(ASTTypeDeclNodeT* node);
  // ... etc
};
```

### Key Implementation Details

- **Single-pass design:** The original two-pass design (CollectDeclarations + ResolveReferences) was fundamentally broken — Pass 1 created scopes, pushed/popped them, declared symbols, then destroyed them; Pass 2 created NEW empty scopes with no symbols. Fixed by restructuring to PreScanDeclarations (register only) + single-pass ResolveAndDeclare.
- **Use-after-free fix:** `PopScope()`, `LookupSymbol()`, and `AllSymbolNames()` all had `scope = scope->parent` which released the current scope before reading `ref.t` from the destroyed object. Fixed by caching parent in a local `Reference<Scope>` first.
- **ASTSwitchNodeT expression field:** Added `ASTNode expression` member to hold the switched-on value; resolver resolves it.
- **TypeDecl resolver fix:** Type members (`Int x`) are type-name + field-name pairs, not expressions. Resolver skips resolving them.
- **Dot-chain rewriting:** Method calls `a.b(args)` are rewritten to call `b` with `a` as first arg.
- **"Did you mean?" suggestions:** Levenshtein distance ≤3 via `EditDistance`/`BestMatch` static methods (copied from old interpreter's `Environment.h`).

### What This Catches

| Bug class | Example | How caught |
|---|---|---|
| Wrong type in slot | `Widgets:Text ... 0.7` (Float where Vec4 expected) | Type mismatch error |
| Wrong arg count | `this.DoSave` (1 arg vs 2 expected) | Arity check |
| Undefined variable | `spawnR` misspelled | "Did you mean?" |
| Forward reference | Function called before declaration | PreScanDeclarations |
| Duplicate declaration | Two `var x` in same scope | Shadow warning |
| Switch missing expression | `switch` without value | ASTSwitchNodeT.expression resolved |

---

## Migration Bridge: Bare Function Calls (2026-08-26)

**Problem:** The old interpreter treats `fn arg1 arg2` as a function call by splitting on spaces. The new parser requires parenthesized calls: `(fn arg1 arg2)`. Corpus has ~1,155 bare function calls across 157 files.

**Strategy:** Add bare-call support to the parser as a **temporary bridge**, not a permanent feature.

### Steps (pre-Phase 4)

1. **Add bare-call detection to `ParseStatement`** (~15 lines) — ✅ **DONE (2026-08-26)**: after `ParseExpression` returns an identifier, peek ahead: if more tokens exist on the same line (before NEWLINE/DEDENT), parse them as function arguments → `ASTFuncCallNodeT`. This makes all 157 scripts work under the new parser with zero script changes. Added `bareCallDepth` flag to prevent recursive bare calls, nullptr guard to prevent infinite loops, and assignment operator exclusion.

2. **Rewrite 16 block-comment `#` occurrences** — the new lexer treats `#` as single-line only. The 16 block-disabling occurrences (Phase 0.5 §A) must be mechanically rewritten: uncomment dead code, delete it, or convert to explicit form. Each file is a case-by-case decision. **✅ DONE (2026-08-26)**: All 16 rewrites completed. Decision summary:
   - **Uncommented (functional code):** `App/widget.lts:31` (F4 settings toggle), `Widget/RadialList.lts:9` and `:43` (function signatures)
   - **Deleted (incomplete/disabled code):** `App/brain.lts:96` (incomplete synapse handler), `Widget/Handling.lts:91` (camera offset), `Widget/HUD.lts:54` (PilotingBadge), `Widget/GridList.lts:41` + `:77` (grid layout + GreedyY), `Widget/Object/Overview.lts:290` (SignatureWidget), `Widget/DevPanel/Clock.lts:45` + `:50` (clock icon + date), `Widget/Reticle/Default.lts:16` (2-arc reticle), `App/observatory.lts:57` (miner ships), `Widget/Spacer.lts:6` + `:19` (debug dots)
   - **Simplified (already disabled):** `Widget/ImageEditor.lts:51` (removed `#`, kept `if false`)

### Steps (post-Phase 4 verification)

3. **One-shot corpus conversion** — script wraps all bare calls in parens across 157 files. Mechanical, ~1,155 lines changed.

4. **Remove bare-call support from parser** — delete the ~15 bridge lines from `ParseStatement`. All scripts now use parenthesized form only.

5. **Delete old interpreter** — remove `Expression.cpp`'s 25 node types, `LTSL.cpp` tree-walker, and `StringList.cpp` line parser.

**Rationale:** The bare-call bridge exists only during the migration window. It avoids a 1,155-line script conversion blocking Phase 4, while the "one way to do things" principle (§1.1) is preserved by converting and removing the bridge after verification.

---

## Phase 4: Evaluator (Runtime) — ✅ COMPLETE (2026-08-26)

Keep the tree-walking evaluator. The same C++ engine functions are
called via `FunctionBind`/`Function_Alias`. No changes to the engine
bridge.

### Prerequisites (before Phase 4)
- [x] Bare-call bridge in parser (~15 lines in `ParseStatement`)
- [x] 16 block-comment `#` rewrites in corpus scripts

### Implementation — ✅ DONE

**Files created:**
- `src/liblt/LTE/Evaluator.h` — Value type (tagged union), Evaluator class declaration
- `src/liblt/LTE/Evaluator.cpp` — Full implementation (~500 LOC)

**Value type** — discriminated union for all LTSL values:
- Primitives: INT, FLOAT, BOOL (stored in union, no heap)
- Strings: STRING (heap-allocated `String*`, owned)
- Engine types: CUSTOM (void* + Type tag, owned)
- References: PTR (void* + Type tag, not owned)
- Special: NONE, TYPE_REF, FUNC_REF, ARRAY

**Evaluator class** — walks the AST and produces runtime values:
- Scope chain via `Reference<Scope>` (parent pointers, `Map<String, Value>`)
- Control flow via `FlowSignal` enum (FLOW_NONE, FLOW_RETURN, FLOW_BREAK)
- Engine dispatch via `Function_Find(name)` → try overloads by arity
- Script functions via `ASTFuncDeclNodeT*` stored in scope as `FUNC_REF`

**Supported operations:**
- All literals (int, float, string, bool, null)
- Variable declarations (var, ref, static)
- Function and type declarations
- Control flow: if/else, while, for, switch/otherwise, return, break
- Assignments (=, +=, -=, *=, /=)
- Binary ops (+, -, *, /, %, ==, !=, <, >, <=, >=, &&, ||)
- Unary ops (-, !)
- Engine function calls (via Function_Find)
- Script function calls (recursive evaluation)
- Method calls (receiver as first arg)
- Casts (int/float/bool conversions)
- Address/deref (pointer operations)
- Debug print (@)
- Constructors (type name as function)

### Implementation

```cpp
// Value type - tagged union or std::variant
struct Value {
  enum Type { INT, FLOAT, STRING, BOOL, VEC2, VEC3, VEC4, OBJECT, REFERENCE, NONE };
  Type type;
  union {
    int intVal;
    float floatVal;
    bool boolVal;
    // ... etc
  };
  String stringVal;
  Reference<Object> objectVal;
  // ... etc
};

class Evaluator {
  Reference<Scope> currentScope;
  Vector<Value> stack;
  
  // Main evaluation
  Value Evaluate(ASTNode* node);
  Value EvaluateBlock(ASTBlockNodeT* block);
  Value EvaluateFunctionCall(ASTFuncCallNodeT* call);
  Value EvaluateMethodCall(ASTMethodCallNodeT* call);
  
  // Scope management
  void PushScope();
  void PopScope();
  void SetVariable(String const& name, Value const& val);
  Value GetVariable(String const& name);
  
  // Engine integration
  Value CallEngineFunction(String const& name, Vector<Value> const& args);
  Value CallScriptFunction(ASTFuncDeclNodeT* func, Vector<Value> const& args);
  
  // Error handling
  void RuntimeError(String const& message, SourceLocation loc);
};
```

### Key Design Decisions

1. **Value representation:** Use a tagged union or `std::variant` for runtime values. Engine types (Vec3, Object, etc.) are wrapped in `Reference<T>` for garbage collection.

2. **Scope chain:** Each scope has a parent pointer and a map of names to Values. Variable lookup walks up the scope chain.

3. **Function dispatch:** Script functions are looked up by name in the current scope. Engine functions are dispatched via `FunctionBind`/`Function_Alias` — the same mechanism the old interpreter uses.

4. **Method calls:** `obj.Method(args)` becomes `CallEngineFunction("Method", {obj, args...})` — receiver is first arg, matching the engine's binding convention.

5. **Error handling:** Runtime errors include source location from AST nodes. No exceptions (engine builds with `-fno-exceptions`) — use error state + early return.

### Post-verification cleanup
- [ ] One-shot corpus conversion (bare calls → parens)
- [ ] Remove bare-call bridge from parser
- [ ] Delete old interpreter (`Expression.cpp`, `LTSL.cpp`, `StringList.cpp`)

---

## Testing Strategy

### Test Suites (Actual Results)

| Suite | What | Actual Count |
|---|---|---|
| `tests/TestLexer.cpp` | Token stream + indent stack: hard-error on mismatched dedent, tab/space equivalence, multi-line paren/bracket groups spanning newlines, single-line `#` comments | 35 tests, 1161 checks, 0 failures |
| `tests/TestParser.cpp` | AST shape for known inputs: all literal types, binary/unary ops, method calls, declarations, blocks, functions, switch, array literals | 77 tests, 263+ checks, 0 failures |
| `tests/TestSymbolResolver.cpp` | Type checking, scoping, forward references, arity checking, "did you mean?" suggestions | 41 tests, 53 checks, 0 failures |
| `tests/TestScriptCompile.cpp` | Existing engine compile checks (must pass) | 21 tests, 95+ checks |
| `ltsl_compile_gate` | All 157 `.lts` files compile with real engine | PASS, 157 files, empty allowlist |

**Total:** 174 new tests, 1,572+ checks, 0 failures

### Regression Gate (Mandatory)

**Before any phase ships**, run all ~160 `.lts` files through both
old interpreter and new compiler. For each script:
1. "Compiles successfully" under both
2. "Produces the same top-level declaration set" (same functions,
   same types, same global variables)
3. Spot-check behavioral equivalence for critical scripts
   (`ltheory-main.lts`, `Widget/HUD.lts`, `Object/SystemPopulate.lts`)

This is the binding-bridge plan's best idea applied here: diff the
observable output of old vs new across the entire real corpus, not
just a curated test suite.

---

## Behavioral Equivalence Gate

"Compiles under the new compiler" ≠ "behaves the same as old
interpreter." Two-pass compilation is a real semantic change: any
script with top-level side effects whose order matters could now
execute in a different sequence even though it compiles cleanly.

**Gate:** For critical scripts, run both old and new, diff the
observable side effects (console output, spawned objects, widget
tree shape). If behavioral diffs appear, either fix the compiler
or document the intentional change.

---

## File Layout

```
src/liblt/LTE/
  AST.h                        — AST node definitions (~300 LOC)
  Lexer.h / Lexer.cpp          — Tokenizer (~800 LOC)
  Parser.h / Parser.cpp        — AST builder (~1,000 LOC)
  SymbolResolver.h / .cpp      — Semantic analysis (~700 LOC)
  Evaluator.h / .cpp           — Runtime evaluation (~500 LOC) ✅ DONE
  Script.h / Script.cpp        — Compile gate integration (existing)

tests/
  TestLexer.cpp                — 35 tests, 1161 checks
  TestParser.cpp               — 77 tests, 263+ checks
  TestSymbolResolver.cpp       — 41 tests, 53 checks
```

**Actual LOC (completed phases):**
- Lexer: ~800 LOC
- Parser: ~1,000 LOC
- SymbolResolver: ~700 LOC
- Evaluator: ~500 LOC
- **Total completed:** ~3,000 LOC (new compiler infrastructure)

**Remaining:**
- Old interpreter deletion: ~2,854 LOC removed (`Expression.cpp`, `LTSL.cpp`, `StringList.cpp`)

**Net change after full migration:** ~+3,000 LOC added, ~2,854 LOC removed

---

## Estimated Effort

### Phase 0: Quick Wins (Sequential)

| Item | Time | Dependencies | Required? |
|------|------|--------------|-----------|
| QW1: Negation operator | 1 day | None | Optional (see §Corpus Findings) |
| QW2: Silent-drop warnings | 1-2 days | None | ✅ Done (A.15 — already hardened) |
| QW3: Better error messages | 2-3 days | None | ✅ Done (A.15 — arguments.back() fix) |
| QW4: `for` sugar | 1 day | None | Deferred (see §Corpus Findings) |
| QW5: Array literals | 1 day | None | ✅ Done (A.15) |
| C.1a: Compile gate | 1 day | None | ✅ Done (A.15) |
| QW6: `switch` improvements | 1-2 days | None | Optional (overlaps A.8/A.9) |
| **Phase 0 total** | **~1 week** | | QW2/QW3/QW5 mandatory; others as time allows |


### Phase 1-4: Full Compiler (Actual Progress)

| Phase | LOC | Time | Status |
|-------|-----|------|--------|
| Phase 0.5: Corpus audit | — | 1 week | ✅ COMPLETE (2026-08-25) |
| Phase 1: Lexer | ~800 | 1 week | ✅ COMPLETE (2026-08-25) |
| Phase 2: Parser | ~1,000 | 1 week | ✅ COMPLETE (2026-08-25) |
| Phase 3: SymbolResolver | ~700 | 1 week | ✅ COMPLETE (2026-08-26) |
| Phase 4: Evaluator | ~500 | 1 week | ✅ COMPLETE (2026-08-26) |
| Post-Phase 4 strict 157 (Option B) | — | 1 week | IN PROGRESS (16/157 new clean, 7 slow-clean, 134 erroring; 157/157 old) |
| Phase 5: DX & Modding | ~400 | 1–2 weeks | NEXT |
| **Total completed** | **~3,000** | **5 weeks** | |
| **Remaining** | **—** | **1 week** | |

> **Phase 1 completed 2026-08-25.** Lexer tokenizes all single-line constructs
> with hard indent-stack enforcement.
>
> **Phase 2 completed 2026-08-25.** Pratt parser handles all LTSL grammar forms.
>
> **Phase 3 completed 2026-08-26.** Symbol resolver with single-pass declare+resolve.
> 41 tests, 53 checks, 0 failures. Fixed use-after-free and two-pass scope mismatch.
>
> **Phase 4 completed 2026-08-26.** Tree-walking evaluator with Value type, scope chains,
> control flow signals, engine function dispatch via Function_Find, and script function
> recursion. ~500 LOC.

### Post-Phase 4 Migration Steps (7 steps)

1. **Add bare-call bridge to parser** (~15 lines in `ParseStatement`) — ✅ DONE (2026-08-26)
2. **Rewrite 16 block-comment `#` occurrences** in corpus scripts — ✅ DONE (2026-08-26)
3. **Phase 4 evaluator implementation** (~500 LOC) — ✅ DONE (2026-08-26)
4. **Corpus regression diff** — run both old and new on all 157 scripts, diff observable behavior — IN PROGRESS (new parser at 16/157 clean + 7 slow-clean via `scan_fork`/`test_errors`, old `ltsl_compile_gate` at 157/157 clean; `ltheory-main.lts` now 0 errors, was hang)
5. **One-shot script conversion — Option B (strict subset via normalization)** — normalize the remaining 79 files' two loose patterns (`var x` with `switch` on next line → `var x switch` on same line; `if <cond> <body>` on same line → `if <cond>` + body on next line indented) — IN PROGRESS (19 files `var->switch`, 7 files `if->body` done via `sed`, `ltsl_compile_gate` now 157/157 clean)
6. **Remove bare-call support from parser** (~15 lines deleted) — NEXT (after 5)
7. **Delete old interpreter** — remove `Expression.cpp`, `LTSL.cpp`, `StringList.cpp` (~2,854 LOC) — NEXT

**Update 2026-08-28 (evening):** Strict 157/157 via the *new* parser — batch conversion in progress.
**Current: 71/157 new-parser clean (0 errors) via `scan_fork` (was 16/157), 85 parse with recoverable errors, 14 parked for deeper `Parser.cpp` work (see `docs/ltsl-conversion-parked.md:5`).** Batch converted ~44 files from bare `fn arg` / indented `l +=` / `for it a b c` / `var x switch` to `(fn ...)` / `for it (a) (b) (c)` / `var x (fn ...)` + `Parser.cpp` fixes: `isUnaryNum` for `Vec2 -1` (`Parser.cpp:1107`), `:`/`/` fold for `Icon/Cursors:Pointer`/`Widget/Components:` (`Parser.cpp:1233`), `var`+`switch` (`Parser.cpp:522`), `Draw` immediate `(` check (`Parser.cpp:1238`), `for` postfix guard (`Parser.cpp:1372`). Old `ltsl_compile_gate` still 157/157 clean on old interpreter; new parser now drives `scan_fork`. Remaining 85 are `l +=` indented `Components:`/`Widgets:` chains + `self.Add`+`Vec` nested (`TransferUnitType` etc.) — same patterns, batch continues a few files at a time to avoid timeouts. Parked 14 need `Parser.cpp:1130` `self.Method (Vec ...)` nested and `var`+`Custom` block fixes.

**Note:** The Phase 0.5 audit moves discovery cost from "implicit and
unbounded inside every phase" to a single upfront week with a concrete
deliverable. Phases 1-4 can proceed with confidence that no major
quirk will surface as a surprise — the inventory is already done. The
total stays at ~9 weeks because the audit was already implicit in the
old estimate; making it explicit doesn't add time, it just makes the
risk visible.

---

## Open Blocker #1: Declaration-hoisting mismatch (MUST resolve before wiring)

**Status:** OPEN. Discovered 2026-08-29. **Blocks** step 7 (delete old
interpreter) and the `Script.cpp` wiring, because a mismatched
declaration set means a function that exists at parse time but is
unreachable at runtime (or vice versa).

**Symptom.** `ltsl_regression_diff` reports `Declaration diffs: YES` —
8 functions that the old interpreter exposes as top-level script
functions but the new pipeline does not:

| File | Function |
|---|---|
| `Item/ShipType/Generate` | `GetAxis` |
| `Item/StationType/Generate` | `AddColumn`, `AddRing`, `ClampExp` |
| `Widget/Window` | `CaptureFocus`, `CreateChildren`, `PostUpdate`, `PreDraw` |

**Root cause.** Old-LTSL registers a `function` declaration into one of
two different maps, depending on `env.context`:

```cpp
// src/liblt/LTE/Expression/Function.cpp:181-184
if (env.context.size()) env.context.back()->functions[name] = fn;  // type scope
else                    env.script->functions[name] = fn;          // script scope
```

`env.context` is a `Vector<ScriptType>` pushed only while compiling a
`type` body (`Expression/Type.cpp:212`, popped at `:236`). So:

- **Functions nested inside another function** (e.g. `GetAxis` indented
  under `function Main` in `Item/ShipType/Generate.lts:30`) compile with
  an *empty* context → they land in **script scope**.
- **Functions inside a `type` body** (e.g. `WidgetWindow`'s
  `CreateChildren`) compile with a context → they land in **type scope**.

Yet old reports *both* classes as script functions, so the exact rule is
not simply "type-scoped functions are excluded". This is the unresolved
part.

**What was tried (do not repeat blindly).**
- Flat comparison (file-level statements only) → 8 diffs, all
  "old has X missing in new" (under-collects).
- Blanket recursive walk collecting every nested `AST_FUNC_DECL`
  (including type methods and function bodies) → **317** diffs, all
  "new has X missing in old" (over-collects; e.g. `App/colony`'s
  `Create`/`Initialize`/`Update` are type methods that old does *not*
  expose as script functions).
- The truth is between the two and depends on old's `env.context`
  semantics, which were not fully reconciled.

**Also note (verified bug in the checker).** The declaration-diff
detail was never printed: the prefixes are **13** characters
(`old_has_func:`) but the comparison used length **12**
(`tools/regression_diff.cpp:470-477`), so every branch silently failed
while `hasDeclMismatch` was still set. Fixed by comparing/substringing
13. Without this fix the diffs were invisible — re-check if the count
ever looks suspicious.

**Why it matters for wiring.** When the new pipeline drives
`ScriptT::Reload`, the declaration set determines which script functions
are callable. If new hoists a function old didn't (or misses one old
had), calls resolve differently at runtime — the failure mode is a null
reference, exactly the `Reference.h:125 "Attempt to access null
reference"` class of crash.

**Resolution plan.**
1. Read `Expression/Type.cpp:200-240` to determine exactly when
   `env.context` is pushed/popped relative to member compilation, and
   whether nested (non-type) functions are *always* script-scoped.
2. Make the new pipeline's declaration set match old exactly:
   script scope = file-level functions + functions nested inside
   function bodies (NOT type methods, per the `App/colony` evidence).
3. Re-run `ltsl_regression_diff` and require
   `Declaration diffs: none`.
4. Only then proceed to wire the new pipeline into `Script.cpp` and
   delete the old interpreter.

**Non-blocking note:** none of the 8 are in `ltheory-main`,
`ltheory-unitest`, `rails`, or `war`, so this does not block those four
apps from parsing.

---

## Migration Log — 2026-08-29 (parser hardening + switchover)

Living record of the parser-hardening and `Script.cpp` switchover session.
Read the **Gotchas** section before touching `Parser.cpp` — most of the traps
below cost a regression to discover, and two of them made things *worse*
before they made things better.

### Where things stand (measured, not estimated)

| Metric | Start of session | Now |
|---|---|---|
| `ltsl_regression_diff` Both PASS | 33 | **129** |
| New compiler `ltsl_compile_gate` | (not wired) | **139 / 157** |
| Old interpreter `ltsl_compile_gate` | 145 / 157 | 145 / 157 (unchanged) |
| Unit tests | 1477 checks | **1477 checks, 0 failures** |
| `ltheory-main`, `ltheory-unitest`, `rails`, `war` | broken / failing | **all compile under the new compiler** |

The new pipeline is wired in but **opt-in**: `LTSL_NEW_COMPILER=1`. The old
path is still the default until the new one is verified end-to-end at runtime.

### What was done

1. **Resolver made dynamic** (matches old-interpreter semantics). LTSL
   resolves names at *runtime*, not compile time. The resolver previously
   errored on every engine/cross-file name (`Vec3`, `Config:Get`,
   `Components:AlignCenter`). Added `Function_Exists`/`Type_Exists` (in
   `Function.h`/`Type.h`) and made unknown names **defer to runtime** instead
   of failing. 15 resolver unit tests had to be updated — they asserted strict
   compile-time semantics that LTSL does not have.
2. **Lexer name rules** (`ReadIdentifier`, `Lexer.cpp:249`): `/`, `:`, `<`, `>`
   are name characters when directly adjacent to identifier characters, so
   `Widget/Pause:Pause_State` and `Vector<Reference<RenderPassT>>` lex as one
   token. Spaced `a / b` is still division. `>` must follow an identifier char
   or another `>` so `>=` and `->` stay operators.
3. **Parser hardening** — the bulk of the work (see Gotchas): indented blocks
   as expression groups, parenthesized callee vs binary-expression
   disambiguation, `suppressSpaceArgs` for `for`/`if`/`while`/switch-case,
   `switch` as an expression, operator-as-function (`(++ i)`), generic types in
   members, inline `if cond body`, multi-line operator continuation.
4. **Switchover plumbing** — `Evaluator::CallFunction()` public entry point,
   `Evaluator::ValueFromSlot`/`ValueToSlot` marshalling (`void**`+`Type` ↔
   `Value`), `ScriptFunctionT` AST body + `astImplicitThis`, `ScriptT` gains
   `astModule`, `Reload()`/`Script_CompileCheck()` honor the switch,
   `BuildFunction`/`BuildType` populate the script from the AST.
5. **Types** — `ASTDeclNodeT::typeName` (was parsed and thrown away),
   `ScriptTypeT::astInitializers`, and the `ScriptType_*` hooks extracted into
   a shared `src/liblt/LTE/ScriptType.cpp` behind
   `ScriptType_CreateEngineType()`.
6. **Tooling** — fixed the declaration-diff reporting bug (prefix length 13,
   compared as 12), restored the `ltsl_regression_diff` target, added
   new-pipeline error dumping (`[NEW-PARSER]` lines on stderr).

Commits: `822ed31` (parser/resolver + groundwork), `8fe58e0` (switchover).

### Gotchas (read before touching Parser.cpp)

**1. The corpus was already migrated, and that is what breaks the app.**
`var x switch` + indented cases is valid for the *new* parser but invalid for
the *old* interpreter (`var` reads `switch`/case/body as extra args →
`expects 2 arguments, but got 4`). The app runs the old path, so ~12 scripts
fail to compile, their functions are null, and `InterfaceUpdate` dereferences
one → `Reference.h:125 "Attempt to access null reference"` → SIGABRT.
**Do not patch the runtime for this.** The only fix is to finish the
switchover. (9 of those 12 already compile under the new compiler.)

**2. Greedy space-separated method-arg collection — the single biggest trap.**
`ParsePostfix` collects following space-separated values as arguments of a
`.member`. That silently swallows the *next* token when it belongs to the
enclosing construct:
- `for it root.GetInteriorObjects it.HasMore it.Advance` → `it.HasMore` becomes an arg of `GetInteriorObjects`
- `if Key_P.Pressed paused = paused.!` → `paused` (the assignment target) becomes an arg of `Pressed`
- switch case `self.focusMouse Colors:Secondary` → the case body becomes an arg, so the case has no body

Fix: a `suppressSpaceArgs` flag set while parsing `for` headers, `if`/`while`
conditions, and switch-case **predicates**. Do NOT suppress it globally — that
breaks legitimate `.Method arg` calls.

**3. `(obj.Method args)` vs `(a.b - c.d)` — cost two regressions, get it right.**
Both start with an identifier + `.` + something. Two failed attempts, recorded
so they are not repeated:
- ❌ Allowing `-`/`+` as value-starts in the *space-separated* loop →
  `foo.Bar - 1` silently became `Bar(-1)` (gate 138 → **126**).
- ❌ Parsing the callee with `ParseUnary` unconditionally → broke
  `(ship.GetPos - o.GetPos)` (gate 138 → **109**).
- ✅ Correct: parse the callee with `ParseUnary` (Pratt would read
  `(rng.Vec2 -1 1)` as `rng.Vec2 - 1`), then **MINUS directly followed by a
  number** = negative-literal argument; any *other* infix operator = rewind and
  re-parse the whole group as a binary expression. `+` is deliberately
  excluded: `(rng.Int + 8)` is **addition**, not a positive literal.

**4. Method-vs-property + a parenthesized argument is context-dependent.**
`self.LeftCenter (Vec2 1 2)` (bare call) = property + sibling arg, but
`nodes.Get (Mod i + 1 nodes.Size)` = method call with that arg. Identical
spacing, opposite meaning, and it cannot be decided without runtime type
info. Resolved by context: **inside parens the member chain is the callee**
(args attach); in a bare/space-separated context, a space before `(` means
property + sibling.

**5. `switch` is a keyword but is also a value.** Needed for
`offset.x = <indented switch>`. `ParsePrimary` must dispatch `TOK_SWITCH`
(or it fails with "unexpected token in expression").

**6. Multi-line operator continuation.** LTSL puts the operator at end of
line: `a ||` / `  b`. After consuming a binary operator, skip `NEWLINE` and
`INDENT` to find the operand (this fixed ~42 `expected expression after
operator` errors corpus-wide). Related: `if` alone on a line with the
condition on following indented lines — the condition and body share one
indented block, so after the condition you must **unwind the continuation
INDENTs (consume pending DEDENTs)** before parsing the body.

**7. Type field types were parsed and discarded.** `ParseTypeMember` computed
`fieldType` but never stored it, so the runtime could not lay out `type`
members. Added `ASTDeclNodeT::typeName` and set it.

**8. Type field initializers are legacy IR.** `ScriptTypeT::initializers` is
`Vector<Expression>`, which the new pipeline does not produce. Added a
parallel `ScriptTypeT::astInitializers` (index-aligned with `fields`) and
wired it into `ScriptType_Construct`. **Without this, `Bool enabled true`
silently becomes `false`** — a quiet behavior change, not a crash.

**9. Don't probe `ScriptFunction_Load` from the resolver.** It forces script
loads as a compile-time side effect and logs "Failed to load script" for
names that are simply resolved lazily at runtime. Removed (spurious errors
went to 0).

**10. `-Werror` is on for project targets.** A `size_t` vs `int` comparison
(`pos > start` in the lexer) fails the build. Cast explicitly.

**11. Some scripts are genuinely malformed at HEAD.** Example fixed:
`Widget/Slider.lts:41` — `var value (cast Int (Mix ... t)` opened 4 parens
and closed 3.

### Remaining work (in priority order)

1. **Fix the 18 files that still fail under the new compiler** — this is the
   blocker for actually launching the game, because `ltheory-main` pulls in
   the HUD widgets. Work queue (all currently `FAIL` under
   `LTSL_NEW_COMPILER=1`):

   `App/draw`, `Icons`, `Object/Firework`, `Object/System`, `Texture/Filters`,
   `Widget/Browser`, `Widget/FontPreview`, `Widget/HUD/Minimap`,
   `Widget/HUD/PilotingBadge`, `Widget/HUD/WorldObjects`, `Widget/Map`,
   `Widget/Market/RightPanel`, `Widget/Market/Transaction`,
   `Widget/Object/Assets`, `Widget/RadialList`, `Widget/Text`,
   `Widget/TextField`, `ZZSlotDriver`

   These are the same classes as the gotchas above (paren/binary ambiguity,
   switch/operator forms). Fix a file, re-run the gate, repeat — the gate is
   the loop.

2. **Resolve Open Blocker #1** (declaration-hoisting mismatch) before
   deleting the old interpreter.

3. **Verify the Evaluator at runtime.** It has never executed real scripts.
   Expect a debug loop once the app boots: `python3 configure.py run war`
   with `LTSL_NEW_COMPILER=1`.

4. **Flip the default** to the new compiler, then do post-verification
   cleanup: remove the bare-call bridge, delete `Expression.cpp`/`LTSL.cpp`/
   `StringList.cpp` (~2,854 LOC).

**Commands for the loop:**
```bash
python3 configure.py build
LD_LIBRARY_PATH=bin:extbin/linux64 ./bin/lte_tests          # must stay 0 failures
LD_LIBRARY_PATH=bin:extbin/linux64 LTSL_NEW_COMPILER=1 ./bin/ltsl_compile_gate
LD_LIBRARY_PATH=bin:extbin/linux64 ./bin/ltsl_regression_diff --timeout 5
```

**Housekeeping:** `script/ltsl-convert.py` is still untracked and deliberately
uncommitted — it is the converter that corrupted 14 files with unbalanced
parens. The user intends to delete it once the migration is complete.

---

## Migration Log — 2026-08-31 (runtime bring-up: rails boots, black screen remains)

Living record of the first *runtime* debugging session. Phase 1-4 compiler work
got the corpus compiling; this session drove `rails` through the new Evaluator
end-to-end for the first time and fixed three engine-side runtime bugs that are
only reachable when scripts actually execute. These are **new finding classes** —
not locally scoped parser fixes — and several were masked by earlier bugs
(`boxes += box` used to be a silent no-op, so every loop over `boxes` saw
`boxes.Size == 0` and never ran).

### Where things stand (measured, not estimated)

| Metric | Start of session | Now |
|---|---|---|
| `rails` boots (process stays alive, no crash) | crashed (SIGFPE → `RNG.h:65` modulo) | **runs to timeout, no crash** |
| `lte_tests` | (green) | 1473 checks, 0 failures (unchanged) |
| `ltsl_compile_gate` | 157/157 PASS | 157/157 PASS (unchanged) |
| `ltsl_regression_diff` | PASS | PASS (unchanged) |
| Rendering | — | **still black across all apps** (see Open Blocker #2) |

### Fix #1 — `j.++` / `(++ i)` never incremented the loop variable (infinite loops)

**Symptom.** `rails.Initialize` never returned; it hung in
`for j 0 j < 8 j.++` (Generate.lts:115) doing 300k+ iterations. The whole app
blocked inside `Initialize`, so the render loop was never reached (this is the
*same* "black screen" from the launcher's perspective — no frames, no
`Widget_Rendered::PreDraw`).

**Root cause.** `Int_Increment` (`Int.cpp:171`, aliased to `++`) mutates its
argument **by reference**: `((int&)i)++`. The new Evaluator passes the receiver
as a *by-value copy* (`j` Value) into `CallEngineFunction`, so the binding
incremented a discarded copy and `j` never changed → `i < 8` stayed true.

**Fix (Evaluator).** `++`/`--` on a *named variable* must write back to the
scope slot. Added `IncDecSlot`/`IncDecOperand` helpers and route:
- `EvalMethodCall`: `j.++` (postfix, empty args, receiver is an identifier)
  → `IncDecSlot` mutates the scope variable in place.
- `EvalFuncCall`: `(++ j)` (prefix, single arg) → `IncDecOperand` mutates.

`IncDecOperand` also handles bare member names resolving to a field of `this`,
and a dotted field of a receiver (`obj.field.++`), writing back via `FieldSet`.

> **Not yet fixed:** `EvalUnaryOp` still raises "unsupported unary op: ++" for
> the *statement-level* unary form `i.++` when the parser emits
> `AST_UNARY_OP` rather than `AST_METHOD_CALL` (Generate.lts:67 — the
> `boxesPassive` step). It does not crash but the loop step silently no-ops;
> this is step 1 of "Immediate next steps" below.

### Fix #2 — compound-op `+=` on non-primitives was a silent no-op (empty lists)

**Symptom.** `boxes += (Box ...)` (Generate.lts:47) did nothing; `boxes.Size`
stayed 0, so `rng.Int boxes.Size` → `GetInt(0,-1)` → modulo by 0 → SIGFPE. This
was the *first* crash seen at the start of the session.

**Root cause.** `EvalAssign`'s compound-op handler only did inline math for
int/float/string primitives; everything else fell through to nothing (a `List`
append was silently dropped).

**Fix.** `EvalAssign` + `ApplyBinaryOp` now dispatch non-primitive compound ops
to the engine operator (`List_Append` aliased `+=`, `Vec3d_AddInPlace`, etc.),
assigning the result back only when it isn't NONE (void-returning mutators like
`List_Append` keep the target unchanged).

> **Gotcha exposed by Fix #2:** the earlier `RNG_Int(0,-1)` SIGFPE and the
> `VectorNP:84` type assert were both *masked by* the empty-list bug. Fixing the
> append is what surfaced Fix #3.

### Fix #3 — script-type boxing inconsistency (cast vs constructor)

**Symptom.** `VectorNP.h:84` assert `t.type == type` in `List_Append`, then
(once that was fixed) `Reference.h:19 refCount > 0` / SIGSEGV in
`Type::operator=` (Type.h:286) deep-copying a cast `Box`.

**Root cause (two-part).**
1. `EvalCast` returned a **bare** script value (`Value::MakePtr(st->type, inst,
   false)`) while `EvalConstructor` returned a **Data-wrapped** value
   (`MakeScriptTypeValue` → `Value::MakePtr(GetDataValueType(), d, true)` with
   `d->type = st->type`). Feeding a bare value to `List_Append` made `ValueArgPtr`
   hand the raw Box buffer where a `Data` struct was expected → mismatched
   `elem.type` → `VectorNP:84`.
2. The first fix attempt *borrowed* `inst` (`d->data = inst` pointing into a
   live List element). That left a lifetime hazard: the Box `Type`'s refcount got
   unbalanced by the borrow + deep-copy interaction → premature `delete` of the
   Box `Type` while a `Reference` still pointed at it → `refCount > 0` at
   destruction.

**Fix.** `EvalCast` now boxs the cast result exactly like the constructor,
**deep-owning a copy**: `d->type = st->type; d->data = st->type->Allocate();
st->type->Assign(inst, d->data);` and `MakePtr(Data, d, true)` — matching
`MakeScriptTypeValue`'s ownership and lifetime model, so `List_Append` reads a
consistent `Box` element type and no borrowed pointer outlives its source.

**Rule going forward.** In the Evaluator, a script-type instance is **always**
a `Data`-wrapped value (`Value.type == GetDataValueType()`, `data` = `Data*`
with `Data.type` = the script `Type`). Never construct a *bare* script value
(`MakePtr(st->type, ...)`) — it breaks `Data`-param engine bindings
(`List_Append`) and the Data deep-copy path in `Value::operator=`. `EvalCast`
must deep-own (copy) rather than borrow, matching `EvalConstructor`.

### Open Blocker #2 — black screen (no graphics) across all apps

`rails` (and `ltheory-main`, `war`, `ltheory-unitest`) now run to timeout with
**no crash**, but render a solid black window. **The apps DO reach the main
loop now** (Fix #1 unblocked `Initialize`), but `Widget_Rendered::PreDraw` is
still never called — verified by instrumenting `Rendered.cpp` (no `[dbg PreDraw]`
frames printed after `Initialize` returns). This is the next step.

**Hypotheses to pursue (in order), none yet proven:**
1. **The app's `Update` / `gameView.Draw` is not reached each frame.** The
   launcher drives `update->VoidCall(0, instance)` in `OnUpdate`
   (launch.cpp:105). If the script `Update` function (rails.lts:141) errors early
   or the const `gameView` widget was never added to the interface, `Draw` never
   fires. Now that `Initialize` returns, instrument `Launcher::OnUpdate` and the
   rails `Update` to confirm the draw call runs.
2. **`Widget_Rendered` / render passes aren't created.** `rails.Initialize`
   builds `Widget_Rendered([...passes])` + `gameView.Add(...)`. If any pass or
   the widget fails to materialize, nothing renders.
3. **GL/present quad issue.** Passes render to FBOs then present via a quad
   (`Rendered.cpp:143-155`). Independent of script — if the present shader or
   buffer chain is broken, even a correct scene is black.
4. **`rails.lts` setup not yet validated** — `var zone` `Object_Zone` block args
   (rails lines 50-62), `Item_StationType` block args. These compile gate-clean
   but may mis-execute.

### Immediate next steps

1. `EvalUnaryOp`: fix `i.++` (unary `++`/`--`) which still raises
   "unsupported unary op: ++" at Generate.lts:67 — route through
   `IncDecOperand`. (This is the remaining runtime error in `rails`; it does
   not crash, but the loop step silently no-ops.)
2. Instrument `Launcher::OnUpdate` + `Widget_Rendered::PreDraw` to confirm the
   draw path; then chase whichever of Open Blocker #2's hypotheses is confirmed.
3. Re-run the loop: build, `lte_tests`, `ltsl_compile_gate`, `ltsl_regression_diff`.

### Verification commands (this session)

```bash
python3 configure.py build
LD_LIBRARY_PATH=bin:extbin/linux64 timeout 25 ./bin/launch rails   # exit=124 = still running = no crash
LD_LIBRARY_PATH=bin:extbin/linux64 ./bin/lte_tests
LD_LIBRARY_PATH=bin:extbin/linux64 ./bin/ltsl_compile_gate
LD_LIBRARY_PATH=bin:extbin/linux64 ./bin/ltsl_regression_diff
```

> **Note:** `LTSL_OLD_COMPILER=1` (legacy interpreter) no longer runs `rails`
> cleanly — `rails` uses new-syntax constructs (WarpRail, Dict) the old
> interpreter can't parse (SIGSEGV). The new compiler is the only path forward
> for these apps; do not A/B the old interpreter against them.

---

## Migration Log — 2026-09-01 (runtime root-cause: arg→param type conversion + RNGT double-release)

This session produced the first *definitive* root cause of the 3-day-old `rails`
startup SIGSEGV. An AddressSanitizer build isolated the crash to a
**Value-marshalling layout bug** — not ship generation and not the type registry.
The fix unblocks the infamous `system = Object_System(Vec3(15.012), 1340)`
assignment (`App/rails.lts:30`); the run now advances into `Item_ShipType.Main`
ship generation, where a `Reference<RNGT>` double-release is the next target.

### Where things stand (measured, not estimated)

| Metric | Start of session | Now |
|---|---|---|
| `rails` crash site | SIGSEGV at `rails.lts:30` (the 3-day blocker) | `system =` succeeds → advances to `Item_ShipType.Main` → crashes on a `Reference<RNGT>` Release (refCount already 0) |
| Root cause of the old crash | unknown (value-ownership hypotheses) | **confirmed via ASAN + fixed** (see below) |
| `lte_tests` | 1473 checks, 0 failures | unchanged |
| `ltsl_compile_gate` (old interpreter) | 157/157 PASS | unchanged |
| AddressSanitizer | not configured | `build-asan/` configured + built clean; first real ASAN report captured |

### The fix — arg→param type conversion in `CallEngineFunction`

**Symptom.** `Object_System`'s C++ ran fine and `retBuf`/`MakeEngine` produced a
valid `Reference<ObjectT>` (verified via `[dbg-os]`), yet the `system` field write
received garbage. SIGSEGV at `Reference.h:85` inside `Reference<ObjectT>::operator=`
(assert `refCount > 0` at `Reference.h:19`, then a second crash).

**ASAN root cause.** First report in the whole project:
`heap-buffer-overflow: READ of size 24` at
`Object_System_Args::Object_System_Args(V3T<double> const&, uint const&)`
(`Objects.h:118`). Script `Vec3` is aliased to **V3F** — the *float* vec, 16 bytes
(`TypeAlias(V3F, Vec3)`, `ScriptAPI/V3.cpp:7`) — while the engine's `Position`
param is `V3D` = `V3T<double>`, 24 bytes. The new Evaluator passed the argument
value's **raw V3F buffer** into the binding, so `Object_System` read 24 bytes from
a 16-byte block: an 8-byte heap overread that poisoned adjacent heap, and the
returned Object `Reference` ended up slotted inside/heap-reused against the freed
vec block → garbage pointee → the refcount SIGSEGV.

The OLD interpreter never hit this because it **converted** mismatched args —
`V3F→V3D` is registered (`Conversion_Bind<&V3F_to_V3D_Impl>()`, `V3.cpp:11-14`),
and `EvalParamMatch` already treats it as a match via the conversion branch
(`v.type->GetConversions()[i].other == pt`). Only the marshalling skipped the
conversion step.

**Fix (Evaluator.cpp, marshalling loop in `CallEngineFunction`).** For
non-primitive params, search the arg Value's `type->GetConversions()`; on
`cv.other == paramType`, allocate a param-typed slot, `construct`, run `cv.fn`,
staged like primitives (freed after the call), and pass **the slot** instead of the
raw value storage. This is the general fix for every cross-type binding mismatch
(any float-vs-double, etc.) — not a one-off for `Object_System`.

**Verification.** `[dbg-field]` now logs `field=system` with a **valid pointee**
(identical to the returned Object); `Object_System` generates star/starfield
(`Unused variable texture ...` shader warnings from `GenerateStar` appear).

### Next crash — `Reference<RNGT>` double-release in `Item_ShipType.Main`

After the `system` assign, `rails` advances into ship generation. `var rng
(RNG_MTG seed + 102)` (`Item/ShipType/Generate.lts:41`) creates
`Reference<RNGT>`; the crash is a Release against refCount==0 (abort
`Reference.h:19`) during `EvalMethodCall` argument teardown inside the first
`for` loop (`var index (rng.Int boxes.Size)`, `Generate.lts:54`).

Debug status:
- Refcount instrumentation added to Release + acquire prints
  (`[dbg-rel] ... rc=N` / `[debug-ref] ... rc=N`), gated by `LTSL_DEBUG=1`.
- **Trap:** for polymorphic `RefCounted` types (RNGT is abstract → has a vptr),
  `refCount` lives at **offset 8**, not 0 — `*(uint*)pointee` reads the vptr
  (garbage constant). Reads must go to `*(uint*)((char*)pointee + 8)` (or avoid
  reads entirely and tally printed acquires vs releases per pointee).
- Pointee tally comes out **positive** (net-leak-looking) for every live pointee,
  because unprinted events (the C++ `retBuf` return slot / teardown Reference
  ownership) skew the count. The true imbalance is between a printed and an
  unprinted event — do not trust a raw tally.

**Next step.** Read the real refCount at offset 8 and find the release that hits
0 (or run the corrected binary under ASAN and let it name the first invalid write).

### Gotchas

1. **V3F vs V3D is a marshalling contract, not a script bug.** LTSL `Vec3` *is*
   the float vec by design; engine bindings take doubles where they want doubles
   (`Position`, `Vec3d`, ...). The fix belongs in the evaluator's arg staging
   (one conversion step for all bindings) — do **not** re-alias `Vec3`→V3D or edit
   scripts/bindings to dodge it.
2. **Conversions are registered on the SOURCE type** (`FunctionBind.h:141`:
   `Type_Get<SourceType>()->AddConversion({DestType, fn})`). Search
   `valType->GetConversions()` for `cv.other == paramType` — the direction used by
   `EvalParamMatch` — not the target type's list.
3. **ASAN shares `bin/`.** `RUNTIME_OUTPUT_DIRECTORY`/`LIBRARY_OUTPUT_DIRECTORY`
   point at repo `bin/` (`CMakeLists.txt:21-29`), so `cmake --build build-asan`
   **overwrites `bin/launch` and `bin/liblt.so`** with the instrumented build.
   Rebuild the normal config (`python3 configure.py build`) to restore the working
   binaries. (The first ASAN run's `exit=127` was a wrong path —
   `build-asan/lib/launch` does not exist; the output lands in `bin/`.)
4. Keep `build-asan/` around until this bug class is closed — it is generated/
   gitignored, but the configuration is reusable for the next sanitizer run.

### Temporary instrumentation (remove before the final commit)

`[dbg-field]`/`[dbg-os]`/`[dbg-aa]` in `Evaluator.cpp`, the `[debug-ref]`/
`[dbg-cpy]`/`[dbg-mkptr]`/`[dbg-rel]` Value traces, plus the pre-existing
`[dbg-ship]` prints (`ShipType.cpp`, `PlateMesh.cpp`, `launch.cpp` `OnUpdate`,
`Rendered.cpp` `PreDraw`) — all gated behind `LTSL_DEBUG=1`.

---

## Migration Log — 2026-09-01 (runtime root-cause: dangling `prevNode` in the warp-node loop)

Continuation of the `rails` runtime bring-up. The `Object_System` marshalling
crash from the previous entry is fixed and long behind us; the run now advances
~1.9M lines into `Initialize` (ship generation + warp-node loop + both stations)
before faulting on `rails.lts:88`.

### Where things stand (measured, not estimated)

| Metric | Start of session | Now |
|---|---|---|
| `rails` crash site | `Reference<RNGT>` double-release in ship generation | `rails.lts:88` `if prevNode.IsNotNull` — **dangling receiver** (root-caused, see below) |
| `system = Object_System(...)` | SIGSEGV (fixed prev. session) | ✅ succeeds; star/nebula/starfield generate |
| `Object_IsNull`/`IsNotNull` | unbound for `Object` receivers | ✅ dedicated bindings added |
| `lte_tests` | 1473 checks, 0 failures | 1473 checks, 0 failures (unchanged) |
| `ltsl_compile_gate` (old path) | 157/157 PASS | 157/157 PASS (unchanged) |

### Fix #1 — `Object_IsNull`/`Object_IsNotNull`: dedicated Object bindings

`prevNode.IsNotNull` (`rails.lts:88`) previously resolved to `Data_IsNotNull`
(`ScriptAPI/Data.cpp:40`), which takes a `Data`-boxed script value — the
receiver type never matched, so the call was the startup SIGSEGV candidate.
Added real bindings in `src/liblt/Game/ScriptAPI/Object.cpp`:

```cpp
// IsNull / IsNotNull for an Object receiver (Reference<ObjectT>).
// return ((Reference<ObjectT> const&)object).t == nullptr
```

Plus `Object_IsNull`. Aliases registered via `Function_Alias`, so both `IsNull`
and `IsNotNull` now have Data and Object candidates; overload resolution picks
the receiver-typed one. **Dispatch now selects the Object binding** (verified in
gdb: `FunctionBinding<bool, std::tuple<const Reference<ObjectT>&>, ...>`,
`name="IsNotNull"`, `loc.line=88`).

### Fix #2 — `EvalParamMatch` hardening

`EvalParamMatch` returned "match" whenever `v.type` was null — which included a
fully-destroyed `Value` (`data == nullptr`, `owned == false`, kind reset). A
null-typed-but-payloaded value is genuinely untyped, but a null-typed **and**
null-payloaded value must NOT silently match an arbitrary param slot. Hardened
the guard to `if (!v.type && !v.data) return true;`.

### Root cause — shallow assignment produces a dangling `prevNode`

**Symptom.** Crash inside `Object_IsNotNull`'s lambda at
`Object.cpp:66 *...; return object.t != nullptr;` — reading `object.t` from a
`Reference<ObjectT>` whose block was already freed. Backtrace:
`EvalFor -> EvalBlock -> EvalIf -> EvalMethodCall -> CallEngineFunction(+0x9ca)`
— i.e. the `if prevNode.IsNotNull` condition (rails.lts:88) in the warp loop.

**Why.** `rails.lts:92` does `prevNode = node`, where `node` is a per-iteration
local declared inside the for-body scope (`var node Object_WarpNode`,
rails.lts:82). The Evaluator `Value` copy semantics (`Evaluator.cpp` Value ctor /
`operator=`) are:
- `owned == true` source → **deep copy** the payload (`Allocate`+`construct`+
  `Assign`) — self-sufficient, refcounted for `Reference<T>`.
- `owned == false` source → **shallow borrow**: `data = other.data`, `owned =
  false`.

Lookup of `node` returns a copy of the slot value; if that copy is a borrow (or
if the assignment itself copies the borrow), `prevNode` aliases the loop-local
`node` block. When the for-body scope dies at DEDENT, `node`'s block is
released; iteration N+1's `prevNode.IsNotNull` reads freed memory. The `Add`
overload worst-match noise seen in the same window (`PlateMesh_AddWarp`
`pc=2` bestScore 0, `Object/WarpRail.lts:46 myModel.Add myMesh Material_Ice`)
is a *separate* latent overload-resolution issue, queued behind this crash.

### Fix #3 — `Evaluator::OwnedCopy`: deep-own borrowed values at storage sites

Added a deep-owning helper used **only where a value is stored into a
persistent variable slot** (so `address`/`deref`/`ref` borrow semantics are
untouched):

```cpp
Value Evaluator::OwnedCopy(Value const& other) {
  if (other.owned || other.kind != Value::CUSTOM || !other.data || !other.type)
    return other;
  Value out; /* deep copy via Allocate + construct + Assign, owned = true */
  return out;
}
```

Applied at three storage points:
- `EvalVarDecl` — `Declare(decl->name, OwnedCopy(init), ...)`
- `EvalAssign` — `*target = OwnedCopy(rhs)` for `=`
- `EvalAssign` dynamic-local fallback — `Declare(targetName, OwnedCopy(rhs))`

**Status: NOT yet verified to fix the crash.** Built and run: SIGSEGV persists
at the same `Object.cpp:66` site (~818th warp iteration, line ≈1.913M). The
deep-copy is correct in principle (refcounted copy of the Reference block), so
either the assignment isn't traversing the fixed path (e.g. `node` is a
`Reference<ObjectT>` raw-borrow whose `type->Assign` still shares the block
with unbalanced refcount), or the freed block predates `prevNode` (a shared
`node`-type instance released earlier). **Next step:** verify the stored
`prevNode` slot's `owned` flag and block refcount at the crash, then either
confirm OwnedCopy is doing its job and the fault lies in `type->Assign` for
`Reference<ObjectT>`, or chase the block's last release. ASAN loop
(`cmake --build build-asan -j 8`; note it clobbers `bin/`) is the next hammer.

### Parser fix (real, unrelated) — method args parse as full expressions

`ParsePostfix` collected each `.member` space-separated argument with
`ParseUnary`, so an argument containing a binary operator mangled the call into
a binary expression (e.g. `self.Add box.center * (Vec3 1 1 1) box.size *
(Vec3 1 1 1) 0 kBevel`). Now parses each argument with `ParseExpression`,
honoring `a.b expr1 expr2` → `(b a expr1 expr2)` grouping while stopping at the
next bare value/operator/EOL.

### Instrumentation

All `[dbg-*]` prints from this session were stripped before this commit
(Evaluator.cpp Value/assign/call foo traces; `[dbg-ship]` etc. in ShipType.cpp,
PlateMesh.cpp, launch.cpp, Rendered.cpp; parser diag cases in TestParser.cpp).
The Diagnostic/Parser `[S]/[P]/[M]` unexpected-token lines are pre-existing
HEAD code and were left alone.

---

## Phase 5: DX & Modding — Making LTSL Easy to Grasp (Next)

**Goal:** Files are hard to understand and future modding must be easy. Keep LTSL **strict but assisted** (hard errors + auto-fix), not permissive. The 79-file normalization (Option B) already lands the strict subset; Phase 5 adds the tooling so authors never have to think about it.

**Why strict + assisted:** Indentation sensitivity is the #1 complaint, but making the parser tolerant hides bugs (Bug 1: `Widget/HUD.lts`/`SystemPopulate.lts` dedent mismatch). Better: hard `DEDENT` mismatch at `Lexer.cpp:150` + `bin/ltsl fmt` that rewrites indent from the `AST` (`Parser.cpp:238` `ParseBlock`) on save. Wrong indent never commits.

**Deliverables (in priority order):**

1. **`bin/ltsl fmt` (1–2 days)** — re-emits `resource/script/**/*.lts` with correct 2-space indent from `AST` (`Lexer.cpp:150` `INDENT`/`DEDENT` stack as source of truth). Modders run `fmt` on save; `ltsl_compile_gate` (`tools/compile_gate.cpp:157` clean) enforces it in CI. Fixes the two loose patterns (`var x` with `switch` on next line, `if` with body on same line) mechanically.

2. **`bin/ltsl check <file>` (1 day)** — single-file `Script_CompileCheck` (`Script.h` `Script_CompileCheck`) for fast editor feedback, without forking the full corpus. LSP already does this, but a CLI is needed for CI and for modders without Zed.

3. **LSP as thin adapter (1 week)** — keep `script/ltsl-lsp/` + `extensions/ltsl/` + `tree-sitter-ltsl` but make `Lexer.cpp`/`Parser.cpp` the single source of truth: spawn `Parser` as a persistent worker over stdio, emit `AST` + symbol table as JSON, and make the TS layer a thin adapter (drop its own grammar). Single source of truth, no drift. Already complete for Zed per `AGENTS.md:6.2`, but needs the `fmt`/`check` integration.

4. **One-page `docs/ltsl-style.md` (1 day)** — canonical `var`/`if`/`switch`/`for` with `DOT` and `:`/`/` path and `Vector<...>` template examples (`Colors:Primary`, `Widget/Components:Expand`, `Vector<Reference<RenderPassT>>` via `Lexer.cpp:204` `ReadIdentifier`), and the `a.b` → `(b a)` rewrite (`LTSL.cpp`) with `this.Method` arity trap (`Parser.cpp:578` `ParseVarDecl` handles `switch` on next line). Cuts the `a.b`/`this.DoSave` bug class.

5. **Data-driven configs (ongoing)** — keep `Config.lts` (`Config:Get` at `resource/script/Config.lts`) + `gameConfig.txt` (`key:value`) for `seed`/`shipHull` etc., and expand to `PlanetType.cpp:12` `atmoDensity`/`biome` knobs via `nlohmann/json` (`include/json/json.hpp:3.11.3`). Scripts stay as glue, JSON holds balance — easier for modders than LTSL arithmetic.

**Non-goals for Phase 5:** `continue`/`in`/`;`/`:` as general syntax — corpus has 0 uses (`Lexer.h:62` `TOK_IN`/`TOK_CONTINUE` dead). Keep the strict subset (`Lexer.cpp:204` `ReadIdentifier` for `:`/`/`/`<...>` is enough). No `range` sugar (`Parser.cpp:200` `(Array String)` stays).

**Gate for Phase 5:** `bin/ltsl fmt --check` + `ltsl_compile_gate` + `ltsl_regression_diff --timeout 2` all 157 clean, plus `python3 configure.py run war` manual launch.

---

## Risk Mitigation

1. **Backward compatibility** — New compiler must accept 100% of
   existing LTSL syntax. The "quirks discovery" phase exists
   specifically to catalog every undocumented behavior before
   building the parser.

2. **Engine bridge unchanged** — `FunctionBind.h`, `Function_Bind`,
   `Function_Alias`, `DefineConversion` are not touched.

3. **Test suite** — Existing 21 tests must pass. New test suites
   for lexer/parser/resolver. Corpus-wide regression diff.

4. **LSP integration** — Cross-language, not a duplicate parser.

   > **Reframe (2026):** AGENTS.md §6.2 states the TypeScript LSP + ZED
   > integration is already **complete** (highlighting, completion, hover,
   > signature help, live diagnostics). Do **not** replace it with a C++ JSON-emitting
   > server — that risks breaking a working tool for enormous cost and adds a second
   > parser to maintain. Instead: have the existing TS LSP consume the new compiler's
   > serialized AST + symbol table (spawn `ltsl_api_dump`/compiler as a persistent
   > worker over stdio). Tokenization/parsing stay in C++; the TS layer drops its own
   > grammar and becomes a thin adapter. Single source of truth, simpler LSP, no rewrite.

   The LSP spawns the C++ compiler as a persistent worker process
   (stdin/stdout, not per-request). The compiler emits a serialized
   AST + symbol table as JSON. The LSP reads that and serves
   hover/completion/diagnostics. Single source of truth — no
   independently-maintained TypeScript grammar that can silently
   disagree with the real parser. The LSP becomes much simpler:
   tokenization and parsing are no longer its job.

---

## Rollback Criteria

Hard checkpoints, not "we'll know it when we see it." If a checkpoint
isn't met, fall back to Quick Wins only.

| Checkpoint | Metric | Threshold | Action if missed |
|---|---|---|---|
| End of Phase 0.5 | Corpus audit complete | 100% of 160 scripts inventoried | Stop — can't scope Phase 2 without it |
| End of Phase 1 (Lexer) | Token coverage of real corpus | 95%+ of 160 scripts tokenize cleanly | Stop — lexer can't handle the real grammar |
| End of Phase 2 (Parser) | Parse coverage of real corpus | 95%+ of 160 scripts parse to valid AST | Stop — fall back to Quick Wins + targeted fixes |

The 95% threshold leaves room for genuinely broken scripts (the 4 known
engine unbalanced-paren bugs, scripts that were always invalid) while
catching systematic gaps. If a Phase fails its checkpoint, the Quick
Wins already shipped — they're the fallback, not nothing.

---

## Success Criteria

### Phase 0 (Quick Wins) — ✅ COMPLETE (2026-08-25)
- [x] Failed statements log warnings with line numbers (QW2 — already hardened)
- [x] All compile errors include line/column info (QW3 — arguments.back() fix)
- [x] `[1, 2, 3]` array literal syntax works (QW5)
- [x] `-x` inline-prefix negation works — **optional** (QW1 — `~` is comment-only in corpus)
- [x] `for i in range a b` sugar works — **deferred** (QW4 — no corpus demand)

### Phase 4 (Evaluator) — ✅ DONE (2026-08-26)
- [x] `Value` type with tagged union for all LTSL values
- [x] Scope chain management (Push/Pop/Declare/Lookup)
- [x] Control flow signals (return, break)
- [x] Engine function dispatch via `Function_Find`
- [x] Script function recursion (store `ASTFuncDeclNodeT*` in scope)
- [x] All literals, binary/unary ops, assignments
- [x] if/else, while, for, switch/otherwise
- [x] Method calls (receiver as first arg)
- [x] Cast, address/deref, constructors, debug print
- [x] `src/liblt/LTE/Evaluator.h` + `Evaluator.cpp` (~500 LOC)
- [x] Builds clean, existing tests pass (1477 checks, 0 failures)

### `#` comment disambiguation — ✅ COMPLETE (2026-08-26)
- [x] Phase 0.5 audit classifies every `#` occurrence — **DONE (2026-08-25)**: 495 total, 16 block-disabling, 478 single-line
- [x] Mechanical rewrite of the 16 offending files — **DONE (2026-08-26)**: 3 uncommented, 12 deleted, 1 simplified
- [x] Drop block-comment behavior — `#` = single-line only, going forward

### Phase 1 (Lexer) — ✅ DONE (2026-08-25)
- [x] All token kinds from Phase 0.5 audit implemented
- [x] Hard indent-stack enforcement with hard error on mismatched dedent
- [x] `#` = single-line comment (block-comment rewrite still pending)
- [x] Multi-line paren/bracket groups span newlines (`parenDepth`)
- [x] Tab treated as 4 spaces; mixed indent is deterministic
- [x] `@` debug-print keyword lexed as `TOK_AT`
- [x] Postfix `.!` `.++` `.--` lexed as separate tokens
- [x] Every token carries line/column
- [x] `tests/TestLexer.cpp`: 35 tests, 1161 checks, 0 failures
- [x] `ltsl_compile_gate`: PASS, 157 files, empty allowlist

### Phase 2 (Parser) — ✅ DONE (2026-08-25)
- [x] Full Pratt parser: ~1,000 LOC, all LTSL grammar forms
- [x] `tests/TestParser.cpp`: 72 tests, 246 checks, 0 failures
- [x] INDENT/DEDENT-based block structure
- [x] LTSL space-separated function params and method args
- [x] Function call parsing `(name arg1 arg2)` via paren detection in `ParsePrimary`

### Phase 3 (Symbol Resolver) — ✅ DONE (2026-08-26)
- [x] `PreScanDeclarations` — file-level forward references (functions + types)
- [x] `ResolveAndDeclare` — single-pass declare+resolve (eliminated broken two-pass design)
- [x] `tests/TestSymbolResolver.cpp`: 41 tests, 53 checks, 0 failures
- [x] `ltsl_compile_gate`: PASS, 157 files, empty allowlist
- [x] Fixed use-after-free in `PopScope`/`LookupSymbol`/`AllSymbolNames`
- [x] Type inference: int/float/string literals, binary/comparison/logical ops
- [x] Dot-chain rewriting, arity checking, scope isolation, "did you mean?"
- [x] ASTSwitchNodeT expression field added and resolved
- [x] AST_TYPE_DECL resolver fix for type members

### Phase 4 (Evaluator) — NEXT
- [ ] Bare-call bridge in parser (~15 lines)
- [ ] 16 block-comment `#` rewrites in corpus scripts
- [ ] Value type implementation (tagged union or std::variant)
- [ ] Scope chain for variable lookup
- [ ] Function/method dispatch via FunctionBind/Function_Alias
- [ ] Runtime error handling with AST line/column info
- [ ] `tests/TestEvaluator.cpp`: target 30+ tests
- [ ] Corpus regression: behavioral equivalence with old interpreter

### Full Migration (Post-Phase 4)
- [ ] All 157 `.lts` files compile with new compiler
- [ ] Behavioral equivalence confirmed for critical scripts
- [ ] One-shot corpus conversion (bare calls → parens)
- [ ] Remove bare-call bridge from parser
- [ ] Delete old interpreter (`Expression.cpp`, `LTSL.cpp`, `StringList.cpp`)
- [ ] LSP works in ZED with new compiler backend

---

## Appendix A: Current LTSL Pain Points (User-Reported)

| # | Pain point | Fixed by Phase 0? | Fixed by full rewrite? |
|---|---|---|---|
| 1 | `~ spawnR` silently does nothing — **CORRECTED: no such bug.** Corpus grep shows `~` is comment-only ("approximately"); zero operator uses. No pain to fix; QW1 is optional (see §Corpus Findings). | Optional (QW1) | Yes |
| 2 | Method call order confusion (`a.b` → `(b a)` — receiver fills implicit `this`, remaining args fill declared params; a one-arg `this.DoSave` fails arity and silently drops). **Real.** Phase-3 arity checker catches it automatically. | No (trap is silent) | Yes (arity check, free for all method calls) |
| 3 | Silent statement drops when arity check fails (`Expression_Block` drops failed expressions; `Function.cpp:173-175`). **Real — biggest time-waster.** | Yes (QW2: warnings) | Yes (error recovery) |
| 4 | No forward references to intra-scope top-level functions. **Real but corpus-inert** — cross-file refs already work via lazy load; methods via type pre-registration (`Function.cpp:90-93`); no corpus script calls a same-scope top-level fn before declaring it. Not enough alone to justify the rewrite (see §Corpus Findings). | No | Yes (two-pass symbol table) |
| 5 | Hard to understand Josh's original code | No (code style issue) | No |
| 6 | LSP not working in ZED | Separate task | Resolved — TS LSP already complete; new compiler feeds it as AST backend (§Risk Mitigation). The "new AST backend" row is now a stretch goal, not the driver. |
| 7 | Hard to prototype UI widgets | Partially (QW2-3) | Yes |
| 8 | Hard to add in-game dev tools | Partially (QW2-3) | Yes |
| 9 | Hard to create maps/worlds/levels quickly | Partially (QW2-3) | Yes |
| 10 | Order-dependent compilation | No | Depends — only intra-scope top-level ordering is missing; method/type-ordering already handled by pre-registration. |

Phase 0 addresses #3 directly, partially #2/#7/#8/#9 (QW2-3), and leaves #4 as a real-but-inert capability gap. `~` (#1) has no pain to fix. Full rewrite adds compile-time type/arity checking (#3/#2 for free) plus forward references — but the ordering win is narrower than originally assumed, which should temper the ~9-week estimate (see §Corpus Findings).

---

## Appendix B: Undocumented Quirks — Audit Results (Phase 0.5, 2026-08-25)

All items below are now **confirmed** from the full 157-file corpus audit.
No remaining `[audit-required]` items.

### Confirmed quirks

| Quirk | Status | Risk |
|---|---|---|
| `.` postfix negation (`debugVisible.!`) | Confirmed | Dot-rewrite in parser |
| `i.++` increment syntax | Confirmed | Dot-rewrite in parser |
| `#` comments out block below (not just line) | Confirmed — 16 occurrences, all intentional code-deactivation | Breaking change — 16 files need rewrite |
| `self.x` vs bare-`x` field access | Confirmed | Method-call parsing |
| Single-quoted strings (`'a b'`) | Confirmed | Post-hoc recognition |
| `<...>` inside atoms (generics) | Confirmed | Token must handle |
| `:` colon paths (`Script:function`) | Confirmed | Single-token identifiers |
| Multi-line paren groups | Confirmed | Cross-line continuation |
| `switch` inline pairs vs indented body | Confirmed | Both forms needed |
| Unmatched `)` swallowing rest of line | Confirmed | Error recovery |

### Confirmed special forms (from Phase 0.5 §B)

| Form | Corpus usage | Confirmed behavior |
|---|---|---|
| `@` | 7 occurrences, 6 files | Prefix debug-print operator; must be `TOK_AT` |
| `desc` | 47 occurrences, 11 files | `Expression_Block(list, env, 2)` — label + body block |
| `block` | 39 occurrences, 3 files | `Expression_Block(list, env, 1)` — single-arg block |
| `call` | **0 occurrences** | **UNUSED — drop from grammar** |
| `static` | 70 occurrences, 11 files | Static local variable; mostly Icons.lts + Texture/Filters.lts |
| `ref` | 147 occurrences, 49 files | Reference alias; heavy — mutable parent→child state pattern |
| `deref` | 36 occurrences, 16 files | Pointer dereference; always paired with `address`/`ref` |
| `address` | 26 occurrences, 11 files | Address-of operator; widget mutable-state communication |

### Unused / non-existent keywords (confirmed)

| Keyword | Status |
|---|---|
| `call` | **UNUSED** — 0 occurrences in corpus |
| `list` | **UNUSED** — 0 occurrences in corpus |
| `set` | **UNUSED** — 0 occurrences (only `=` form used) |
| `sizeof` | **Does not exist** in engine (no handler in Expression.cpp) |
| `typeof` | **Does not exist** in engine (no handler in Expression.cpp) |

---

## Appendix C: What This Plan Omits + Ease-of-Use / Troubleshooting Opportunities

This appendix answers two questions: what should be added to this plan, and what
can be done *now* — regardless of whether Phases 1–4 happen — to make the engine
and LTSL easier to use and troubleshoot. Findings are grounded in the corpus and
engine source reviewed for this update.

### C.1 Things missing from the plan that should be covered

**C.1a — A real compile gate (catches bugs the LSP smoke test cannot).**
> **DONE (A.15, 2026-08-25)** — `tools/compile_gate.cpp` +
> `Script_CompileCheck`; see AGENTS.md §6.2 "Compile gate" for commands.
> First run found 5 broken files invisible to the LSP smoke test.

AGENTS.md §A.14 #14 documents a class of bug the analyzer missed: `Button.lts`'s
`CreateButton` passed an `enabled` arg positionally to an AutoClass-generated
constructor that only accepts 4 args, so every SAVE/LOAD button silently failed to
render — and the LSP smoke test caught nothing (its constructor-arity model differs
from what the engine generates). A CLI that compiles *every* `.lts` with the real
engine (`Expression_Compile` over all `resource/script/*.lts`, wired as a CI gate)
would catch these at authoring time. This is the single highest-value "catch real
bugs" tool and should be added to Phase 0 — it's cheap, independent of the rewrite,
and directly addresses a known gap in the existing DX.

**C.1b — The `this.Method` arity trap deserves an automatic check.**
Documented in AGENTS.md §A.14: `a.b` rewrites to `(b a)`, so `this.DoSave` (no
declared params) fills implicit `this`; but `this.DoSave self` with one declared
param fails the arity check and silently drops the statement, surfacing only a
console error mid-compile. Phase 3's resolver should treat method-call arity as an
*error*, not a silent drop — this is the trap #C.2 fixes for free.

**C.1c — The `#` block-comment migration is confirmed real but under-specified.**
Verified in `StringList.cpp:96-117`. The plan's Phase 0.5 audit must specifically
classify every `#` occurrence as single-line vs block-disabling (the latter = a line
+ all deeper-indented lines below it). This is the highest-risk latent bug and should
be tracked independently of Phases 1–4.

**C.1d — Special-form completeness.** The plan lists 9 forms; `@`/`call` were marked
"audit to confirm." Corpus grep shows both are used (`@`=print, `call`=dynamic
dispatch in 4 files) but their exact corpus form is unverified — the audit must pin
down syntax/scope for each before grammar completion is claimed.

**C.1e — Unused tokens.** `TOK_SIZEOF`/`TOK_TYPEOF` are declared but unused in the
corpus; the lexer should drop them (audit to confirm no future-facing use).

### C.2 Additional things we could do *now* (independent of the rewrite)

These are DX/troubleshooting wins that need no two-pass compiler and can ship as
Quick Wins or standalone tooling:

1. **A headless LTSL REPL/runner.** A CLI (`ltsl <file>.lts --run Fn args...`) that
   loads a script, invokes a function, and dumps its resulting state (variables,
   widget tree shape, spawned objects). This is the fastest path to "what does this
   script actually do?" — the most common troubleshooting question. Nothing in the
   plan addresses it; high value for content authors debugging off-screen.

2. **Compile every app at CI time** (see C.1a) — reuses `ltsl_api_dump`'s existing
   byte-diff philosophy already trusted in AGENTS.md §6.2, applied to script source.
   Turn "it works on my machine" into a red pipeline.

3. **Document the calling conventions explicitly** (the two that bite hardest):
   - **Receiver-first method calls:** `obj.Method(a b)` → `(Method obj a b)`; every
     declared param follows implicit `this`. A one-page "Writing LTSL functions" note
     covering this would cut the #C.1b class of bugs dramatically.
   - **Widget factory pattern** (`Button:Create msg "TEXT" 20`) — already proven in
     AGENTS.md §A.14; codify it as the canonical way to make widgets so field-order /
     defaulted-field constructor-arity bugs can't recur.

4. **`desc` vs `block` arity distinction** (already flagged in Phase 2) — worth a short
   note now since it's a shipped landmine, not a theoretical one: an indented block is
   `block(...)` with the *receiver* as arg 1; `desc(...)` adds a second arg.

5. **Consistent-indentation enforcement** (Phase-1 lexer design decision #4) — bake
   tab/space policy into the editor config now so mixed indentation can't produce
   spurious dedent errors later.

6. **`range`/iteration sugar — defer, but note it.** No corpus demand and no bound
   iterator (see §Corpus Findings); revisit only if content authors ask for it.
