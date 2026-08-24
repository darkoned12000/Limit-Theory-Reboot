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

Additional pain points:
- **Ordering sensitivity** — declarations must come before use
- **Silent statement drops** — failed arity checks silently discard code
- **`a.b` rewrite confusion** — method call order is unintuitive
- **`~` not negation** — `(Vec3 (~ x) 0 0)` does nothing (corpus grep
  confirms `~` only appears in comments as "approximately")
- **Terse error messages** — "Unused variable in Shader(...)" with no
  location info

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

Claude Code's review correctly identified that the Quick Wins section
covers 8 of 10 user-reported pain points and can ship in ~2 weeks
vs ~7 weeks for the full rewrite. The recommended sequence:

1. **Ship Quick Wins first** (Phase 0, ~2 weeks)
2. **Use them for real content work** (2-4 weeks)
3. **Decide whether the full rewrite is still justified** based on
   whether ordering sensitivity and no forward references still hurt
   after the other 8 problems are gone
4. **If yes: full corpus audit** (Phase 0.5, ~1 week) — inventory
   every LTSL construct in actual use, determine breaking-change
   scope, scope the real parser work

This is the right call because:
- Several Quick Wins are literal subsets of Phase 1 (negation token,
  line/column tracking). They're not wasted work if the full rewrite
  happens — they're forced familiarity with Expression.cpp before
  replacing it.
- Landing them gives a real signal: does the pain actually go away?
- The two remaining pain points (ordering sensitivity, no forward
  references) genuinely require a symbol table — but we should verify
  they still hurt in practice before spending 7 weeks on the solution.

---

## Phase 0: Quick Wins (~2 weeks)

These fix the worst pain points in the current interpreter. Each is
1-3 days, low-risk, and can ship independently.

### QW1: Negation operator (1 day)

Add `-x` (unary minus) to the current parser. Currently `-x` works as
a function call (`(- x)`) but not as an inline prefix in contexts like
`(Vec3 (- x) 0 0)`.

**Corpus grep result:** `~` only appears in comments (meaning
"approximately"). Safe to leave unmapped; no need to repurpose.

**Changes:**
- `Expression.cpp`: recognize `-` as unary prefix when followed by
  identifier/literal/operator
- `Expression_UnaryMinus` new node (or reuse existing `(- x)` path)

### QW2: Silent statement drop warning (1-2 days)

When an arity check fails in `Expression_Block`, log a warning with
line number instead of silently dropping the statement. This is the
single biggest debugging time-waster.

**Changes:**
- `Block.cpp`: after failed statement compilation, emit
  `env.ReportError` with the line number and "arity mismatch" message
- `FunctionCall.cpp` / `ExpressionCall.cpp`: on arity failure, log
  which argument count was expected vs received

### QW3: Better error messages with line/column (2-3 days)

Extend A.8's "did you mean?" work to all expression types. Currently
suggestions exist only for name-resolution errors (Variable, Reference,
Constructor, FunctionCall, Conversion). Extend to:
- Type mismatches ("expected Vec3d, got Float at line N")
- Arity mismatches ("SetPos expects 2 args, got 3 at line N")
- Unknown member access ("Object has no field 'positon' — did you
  mean 'position'?")

**Note:** The existing "did you mean?" suggestions use Levenshtein
distance ≤ 3 via `BestMatch()` in `Environment.h:67-108`. This
already works for name resolution; we're extending coverage.

### QW4: `for` loop syntax improvement (1 day)

Current `for` requires 6 elements: `(for name init pred step body...)`.
The `i.++` step syntax is confusing. Add sugar **while keeping the
existing form** (it's more general — arbitrary predicates and custom
steps):
```
for i in range 0 512
  body
```
This is sugar only, not a language change — it desugars to the existing
`for` form. The old form stays for loops that need non-linear iteration
(arbitrary predicate, custom step, or non-integer ranges).

### QW5: Array literal syntax (1 day)

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

---

## Phase 0 Gate: Ship, Test, Decide

After shipping Quick Wins:
1. Run all ~160 `.lts` files through the improved interpreter
2. Verify no regressions (same behavior as before for working scripts)
3. Use the improved scripts for 2-4 weeks of real content work
4. **Decision point:** Do ordering sensitivity and no forward references
   still hurt enough to justify the full rewrite?

If yes → proceed to Phase 0.5. If no → stay on improved interpreter.

---

## Phase 0.5: Full Corpus Audit (~1 week)

**Before the full rewrite is scoped**, do a complete inventory of every
distinct LTSL construct actually in use across all 160 scripts. This
feeds three separate parts of the plan:

1. **Breaking-change scope** — especially the `#` block-comment change:
   if any script uses `#` to disable a whole block (line + deeper-indented
   lines below it), switching to single-line `#` will cause that block
   to silently compile and run — the opposite failure mode of everything
   this project is trying to fix. The audit finds every such occurrence
   before the lexer is written.

2. **Quirks completeness** — the Appendix B list is a starting point
   based on a handful of files. The corpus has meaningfully more surface
   area (every file we haven't looked at directly is a source of unknown
   behavior). "100% backward compatible" does a lot of unweighted work
   in the estimate until there's a real inventory.

3. **Realistic Phase 2 estimate** — every undocumented quirk is a
   compatibility landmine that needs explicit discovery. The audit
   replaces guesswork with a concrete count.

### Audit deliverables

| Deliverable | What | Feeds |
|---|---|---|
| Construct inventory | Every distinct LTSL construct (keywords, operators, special forms) used in any `.lts` file | Appendix B, Phase 2 grammar |
| `#` block-comment audit | Every `#` occurrence classified: single-line (safe) vs block-disabling (breaking change) | `#` migration plan below |
| `@` / `desc` / `block` / `call` / `static` / `ref` / `deref` / `address` usage | Which files use which, how they're used | Phase 1 token list, Phase 2 grammar |
| Cross-file dependencies | Which scripts reference types/functions defined in other scripts | Phase 3 symbol resolver scope |

### `#` comment migration strategy

The audit determines the migration path:

- **If block-comment `#` usage is rare** (my guess, based on files
  reviewed): mechanical one-time rewrite of the handful of offending
  files, then `#` = single-line comment going forward.

- **If block-comment `#` usage is common**: give the block-comment
  behavior a syntactically distinct marker (`##`, say) for a transition
  period, mechanically migrate every real occurrence to `##`, then drop
  the ambiguity.

Either way, `#` becomes unambiguous going forward. An opt-in mode where
`#` means two different things depending on invisible context is a
smaller version of the exact ambiguity problem (`self.x` vs bare `x`,
`i.++` vs `(++ i)`) that's already caused real bugs in this project.

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
  TOK_TYPE, TOK_CAST, TOK_CALL, TOK_BLOCK, TOK_DESC,
  TOK_SIZEOF, TOK_TYPEOF, TOK_ADDRESS, TOK_DEREF,

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
              | typeConstructor | castExpr
              | funcCall

typeConstructor = typeName '(' argList ')'
castExpr      = 'cast' typeName expr
arrayLiteral  = '[' [exprList] ']'
funcCall      = IDENTIFIER '(' argList ')'

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
| `tests/TestLexer.cpp` | Token stream correctness | 50+ tests |
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

| Item | Time | Dependencies |
|------|------|--------------|
| QW1: Negation operator | 1 day | None |
| QW2: Silent-drop warnings | 1-2 days | None |
| QW3: Better error messages | 2-3 days | None |
| QW4: `for` sugar | 1 day | None |
| QW5: Array literals | 1 day | None |
| QW6: `switch` improvements | 1-2 days | None |
| **Phase 0 total** | **~2 weeks** | |

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

### Phase 0 (Quick Wins)
- [ ] `-x` negation works everywhere
- [ ] Failed statements log warnings with line numbers
- [ ] All compile errors include line/column info
- [ ] `[1, 2, 3]` array literal syntax works
- [ ] `for` with multiple cases works reliably

### Full Migration (If Decided)
- [ ] All 160 `.lts` files compile with new compiler
- [ ] Behavioral equivalence confirmed for critical scripts
- [ ] Forward references work (call function before declaration)
- [ ] Type mismatches caught at compile time
- [ ] "Did you mean?" for misspelled identifiers
- [ ] LSP works in ZED with new compiler backend

---

## Appendix A: Current LTSL Pain Points (User-Reported)

| # | Pain point | Fixed by Phase 0? | Fixed by full rewrite? |
|---|---|---|---|
| 1 | `~ spawnR` silently does nothing | Yes (QW1: negation) | Yes |
| 2 | Method call order confusion (`a.b` → `(b a)`) | No | Yes (parser handles directly) |
| 3 | Silent statement drops when arity check fails | Yes (QW2: warnings) | Yes (error recovery) |
| 4 | No forward references | No | Yes (two-pass symbol table) |
| 5 | Hard to understand Josh's original code | No (code style issue) | No |
| 6 | LSP not working in ZED | Separate task | Yes (new AST backend) |
| 7 | Hard to prototype UI widgets | Partially (QW2-3) | Yes |
| 8 | Hard to add in-game dev tools | Partially (QW2-3) | Yes |
| 9 | Hard to create maps/worlds/levels quickly | Partially (QW2-3) | Yes |
| 10 | Order-dependent compilation | No | Yes (two-pass) |

Phase 0 addresses 6/10 directly, partially addresses 2 more.
Full rewrite addresses all 10 (except #5, which is code style).

---

## Appendix B: Undocumented Quirks to Discover

The Phase 0.5 corpus audit is the authoritative source. This list is
the starting point based on files reviewed so far. Items marked
**[audit-required]** need usage data from the full 160-script inventory
before they can be scoped.

### Known quirks (from files reviewed)

| Quirk | Status | Risk |
|---|---|---|
| `.` postfix negation (`debugVisible.!`) | Known | Dot-rewrite in parser |
| `i.++` increment syntax | Known | Dot-rewrite in parser |
| `#` comments out block below (not just line) | Known | Breaking change — see Phase 0.5 audit |
| `self.x` vs bare-`x` field access | Known | Method-call parsing |
| Single-quoted strings (`'a b'`) | Known | Post-hoc recognition |
| `<...>` inside atoms (generics) | Known | Token must handle |
| `:` colon paths (`Script:function`) | Known | Single-token identifiers |
| Multi-line paren groups | Known | Cross-line continuation |
| `switch` inline pairs vs indented body | Known | Both forms needed |
| Unmatched `)` swallowing rest of line | Known | Error recovery |

### Constructs needing audit **[audit-required]**

| Construct | What to determine | Risk |
|---|---|---|
| `@` debug print | Syntax, scope, interaction with indentation | Token + parser handling |
| `desc` blocks | Syntax, nesting rules, relationship to `block` | Parser special form |
| `block` blocks | Syntax, how they differ from `desc` | Parser special form |
| `call` dynamic dispatch | Syntax, argument passing, return type | Parser + type checker |
| `static` local variables | Scope rules, initialization timing | Symbol resolver |
| `ref` aliases | Syntax, mutability rules, interaction with `var` | Symbol resolver |
| `deref` / `address` pointers | Syntax, safety constraints, engine integration | Parser + type checker |
| Any other special forms | Full inventory from Phase 0.5 | Unknown until audited |

This list will grow during the Phase 0.5 audit. Budget 1 week for
it — it feeds the grammar (Phase 2), the token list (Phase 1), and
the estimate confidence.
