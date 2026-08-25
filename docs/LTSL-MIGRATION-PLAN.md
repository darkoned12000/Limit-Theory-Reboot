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

## Phase 1: Lexer (Token Scanner)

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

4. **Tab/space indents are equivalent.** A char-based lexer that counts only
   spaces will fire spurious dedent errors on any file mixing tabs and spaces,
   or mis-measure every tab-containing block. Decide up front: treat a tab as 8
   (or N) spaces for measurement, OR require consistent indentation and report a
   clear error otherwise. Either way it must be deterministic — not "whatever the
   editor emitted."

5. **Multi-line paren groups span newlines.** `(` / `)` may open on one line and
   close on another (every multi-line expression in the corpus relies on this).
   The lexer must defer INDENT/DEDENT measurement until a balanced-paren boundary,
   or it will split expressions at every newline inside parens. Same for `[` / `]`.

```cpp
enum TokenKind {
  // Literals
  TOK_INT, TOK_FLOAT, TOK_STRING, TOK_BOOL, TOK_NULL,

  // Identifiers
  TOK_IDENTIFIER, TOK_MEMBER,

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
  TOK_COMMA, TOK_SEMICOLON, TOK_COLON, TOK_DOT, TOK_ARROW,

  // Keywords
  TOK_VAR, TOK_REF, TOK_STATIC, TOK_FUNCTION, TOK_RETURN,
  TOK_IF, TOK_ELSE, TOK_WHILE, TOK_FOR, TOK_IN,
  TOK_SWITCH, TOK_OTHERWISE,
  TOK_BREAK, TOK_CONTINUE, TOK_TRUE, TOK_FALSE,
  TOK_TYPE, TOK_CAST, TOK_BLOCK, TOK_DESC,
  TOK_ADDRESS, TOK_DEREF, TOK_AT,

  // Special
  TOK_NEWLINE, TOK_INDENT, TOK_DEDENT, TOK_EOF
};

struct Token {
  TokenKind kind;
  String value;      // The raw text of the token
  int line;          // 1-based line number
  int column;        // 1-based column number
  int length;        // Character length of the token
};
```

### Lexer Architecture

```
class Lexer {
  String source;
  int pos;            // Current position in source
  int line;           // Current line (1-based)
  int column;         // Current column (1-based)
  Vector<int> indentStack;  // Indent levels (starts with [0])
  Vector<Token> pending;    // Buffered tokens (for INDENT/DEDENT)
  bool atLineStart;         // True if next char starts a new line

  // Core
  Token NextToken();
  Vector<Token> Tokenize();

  // Character helpers
  char Peek();
  char Peek2();
  char Advance();
  bool AtEnd();
  void SkipWhitespace();  // Spaces and tabs (NOT newlines)
  void SkipComment();     # to end of line

  // Token readers
  Token ReadNumber();
  Token ReadString();
  Token ReadIdentifier();
  Token ReadOperator();

  // Indent handling
  int MeasureIndent();    // Count leading spaces at line start
  void EmitIndentTokens(int newIndent);  // Push INDENT/DEDENT tokens
};
```

### Indent-Stack Rules (Python-style)

```
indentStack = [0]  # Initial indent level

On each NEWLINE + leading whitespace:
  newIndent = MeasureIndent()

  if newIndent > indentStack[-1]:
    indentStack.push(newIndent)
    emit TOK_INDENT

  else if newIndent < indentStack[-1]:
    while indentStack[-1] > newIndent:
      indentStack.pop()
      emit TOK_DEDENT
    if indentStack[-1] != newIndent:
      ERROR("unindent does not match any outer indentation level")

  # newIndent == indentStack[-1]: no indent/dedent tokens
```

**Hard error on mismatched dedent** — this is the key improvement.
The current interpreter silently accepts bad indentation; the new
lexer rejects it.

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

## Phase 2: Parser (AST Builder)

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

## Phase 3: Symbol Resolver + Type Checker

**Input:** Raw AST from Phase 2
**Output:** Typed AST with resolved references

```cpp
struct Symbol {
  String name;
  Type* type;
  int scopeLevel;
  bool isMutable;
  SourceLocation declLoc;
};

struct Scope {
  Reference<Scope> parent;
  Map<String, Symbol> symbols;
  int level;
};

class SymbolResolver {
  Vector<Reference<Scope>> scopeStack;
  Map<String, Type*> typeTable;
  Vector<CompileError> errors;

  // Pass 1: Collect all declarations
  void CollectDeclarations(ASTNode* module);

  // Pass 2: Resolve all references
  void ResolveReferences(ASTNode* module);

  // Type inference + checking
  Type* InferType(ASTNode* expr);
  bool CheckType(Type* expected, Type* actual, SourceLocation loc);

  // Error recovery
  void RecoverFromError();
};
```

### What This Catches

| Bug class | Example | How caught |
|---|---|---|
| Wrong type in slot | `Widgets:Text ... 0.7` (Float where Vec4 expected) | Type mismatch error |
| Wrong arg count | `this.DoSave` (1 arg vs 2 expected) | Arity check |
| Undefined variable | `spawnR` misspelled | "Did you mean?" |
| Forward reference | Function called before declaration | Two-pass collection |
| Duplicate declaration | Two `var x` in same scope | Shadow warning |

---

## Phase 4: Evaluator (Runtime)

Keep the tree-walking evaluator. The same C++ engine functions are
called via `FunctionBind`/`Function_Alias`. No changes to the engine
bridge.

```cpp
class Evaluator {
  Vector<Reference<Scope>> scopeStack;
  Value Evaluate(ASTNode* node);
  Value EvaluateFunctionCall(ASTNode* call);
  Value EvaluateMethodCall(ASTNode* call);
  // ... etc
};
```

---

## Testing Strategy

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

### Test Suites

| Suite | What | Count target |
|---|---|---|
| `tests/TestLexer.cpp` | Token stream + indent stack: hard-error on mismatched dedent, tab/space equivalence, multi-line paren/bracket groups spanning newlines, single-line `#` comments | 50+ tests |
| `tests/TestParser.cpp` | AST shape for known inputs | 30+ tests |
| `tests/TestSymbolResolver.cpp` | Type checking, scoping | 30+ tests |
| `tests/TestScriptCompile.cpp` | Existing 21 tests (must pass) | 21+ tests |
| Corpus diff | All 160 .lts files compile + match | 160 scripts |

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
src/liblt/LTE/Compiler/
  Lexer.h / Lexer.cpp          — Tokenizer (~800 LOC)
  Parser.h / Parser.cpp        — AST builder (~1,200 LOC)
  AST.h                        — AST node definitions (~400 LOC)
  SymbolResolver.h / .cpp      — Semantic analysis (~1,000 LOC)
  CompileError.h               — Error types (~100 LOC)
  Compiler.h / Compiler.cpp    — Top-level API (~200 LOC)
```

**Total new code:** ~3,700 LOC (compiler only)
**Evaluator:** adapted from existing ~2,854 LOC (not new code)
**Net change:** ~+3,700 LOC added, ~2,854 LOC eventually removed

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


### Phase 1-4: Full Compiler (If Decided)

| Phase | LOC | Time | Dependencies |
|-------|-----|------|--------------|
| Phase 0.5: Corpus audit | — | 1 week | Phase 0 shipped |
| Phase 1: Lexer | ~800 | 1 week | Audit complete |
| Phase 2: Parser | ~1,200 | 1.5 weeks | Phase 1 |
| Rollback checkpoint | — | — | 95% parse coverage |
| Phase 3: SymbolResolver | ~1,000 | 2 weeks | Phase 2 passes |
| Phase 4: Evaluator adaptation | — | 1 week | Phase 3 |
| Corpus testing + migration | — | 2 weeks | Phase 4 |
| LSP integration | — | 1 week | Phase 4 |
| **Phase 1-4 total** | **~3,000** | **~9 weeks** | |

**Note:** The Phase 0.5 audit moves discovery cost from "implicit and
unbounded inside every phase" to a single upfront week with a concrete
deliverable. Phases 1-4 can proceed with confidence that no major
quirk will surface as a surprise — the inventory is already done. The
total stays at ~9 weeks because the audit was already implicit in the
old estimate; making it explicit doesn't add time, it just makes the
risk visible.

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

### Phase 0 (Quick Wins) — mandatory items only; QW1/QW4 are optional (see §Corpus Findings)
- [ ] Failed statements log warnings with line numbers (QW2)
- [ ] All compile errors include line/column info (QW3)
- [ ] `[1, 2, 3]` array literal syntax works (QW5)
- [x] `-x` inline-prefix negation works — **optional** (`~` is comment-only in the corpus; add only for completeness / future-proofing)
- [ ] `for i in range a b` sugar works — **deferred pending audit** (no `range` construct or iterator function bound; nothing in corpus exercises it)

### `#` comment disambiguation (standalone — track regardless of rewrite)

The `#` block-comment ambiguity is the highest-risk latent bug: switching
to single-line `#` would silently compile any block currently disabled by
it. Confirmed shipped as a real mechanism (`StringList.cpp:96-117`). Migrate
this **independently** of Phases 1-4, as soon as it's in scope:

- [x] Phase 0.5 audit classifies every `#` occurrence — **DONE (2026-08-25)**: 495
      total, 16 block-disabling, 478 single-line. See Phase 0.5 §A.
- [ ] Mechanical rewrite of the 16 offending files (pre-Phase 1, or fold into
      Phase 1 lexer work — decision pending)
- [ ] Drop block-comment behavior — `#` = single-line only, going forward

### Full Migration (If Decided)
- [ ] All 160 `.lts` files compile with new compiler
- [ ] Behavioral equivalence confirmed for critical scripts
- [ ] Forward references work (call function before declaration) — note: this is a real but **corpus-inert** capability; cross-file refs already work, only intra-scope top-level ordering is missing
- [ ] Type mismatches caught at compile time
- [ ] "Did you mean?" for misspelled identifiers
- [ ] `this.Method` arity trap caught automatically (receiver fills implicit `this`, remaining args fill declared params)
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
