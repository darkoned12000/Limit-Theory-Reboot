# LTSL Migration Plan: From Tree-Walker to Proper Compiler

## Why This Exists

The current LTSL interpreter has fundamental design flaws that block rapid
content creation:

1. **Ordering sensitivity** — declarations must come before use; no forward
   references. A typo in line 3 silently breaks line 50.
2. **Silent failures** — failed statements are dropped without error. You
   spend hours debugging invisible problems.
3. **Confusing syntax** — `a.b` rewrites to `(b a)` at parse time, causing
   constant confusion about method call order.
4. **No symbol table** — variable scoping is implicit; no shadow warnings,
   no forward references, no type checking.
5. **Terse error messages** — "Unused variable in Shader(...)" tells you
   nothing about what went wrong or where.

**Goal:** Replace the tree-walking interpreter with a proper two-pass
compiler that eliminates these problems while preserving the engine
integration (AutoClass reflection, FunctionBind, etc.).

**Non-goal:** Replacing LTSL with Lua or another language. The engine's
reflection system is deeply coupled to LTSL. A rewrite of the language
itself is more practical than a rewrite of 160+ script files plus the
engine bridge.

---

## Current Architecture

```
Source → Parse (single-pass) → AST → Evaluate (recursive walk)
                Phase 1                    Phase 2
```

**Parser:** `src/liblt/LTE/Expression.cpp` + 25 `Expression/*.cpp` files
**Evaluator:** `src/liblt/LTE/LTSL.cpp` (~2,854 LOC)
**Bindings:** `src/liblt/Game/ScriptAPI/` (1,881 functions)

### Known Problems

| Problem | Example | Root Cause |
|---------|---------|------------|
| Ordering sensitivity | `var x (f y)` fails if `f` defined later | Single-pass compilation |
| Silent statement drop | `this.DoSave self` silently does nothing | Arity check fails, statement dropped |
| `a.b` confusion | `this.Method args` rewrites to `(Method this args)` | Parse-time rewrite |
| No forward references | Can't call a function before it's declared | Symbol table doesn't exist |
| Terse errors | "Unused variable in Shader(...)" | No line/column tracking |
| `~` not negation | `(Vec3 (~ x) 0 0)` does nothing | Operator not implemented |

---

## Proposed Architecture

```
Source → Lexer → Tokens → Parser → AST → SymbolResolver → TypedAST → Evaluator
                Phase 1           Phase 2                Phase 3      Phase 4
```

### Phase 1: Lexer (Token Scanner)

**Input:** Source code string
**Output:** Token stream with line/column tracking

```cpp
enum TokenKind {
  // Literals
  TOK_INT, TOK_FLOAT, TOK_STRING, TOK_BOOL, TOK_NULL,

  // Identifiers
  TOK_IDENTIFIER, TOK_TYPE, TOK_MEMBER,

  // Operators
  TOK_PLUS, TOK_MINUS, TOK_STAR, TOK_SLASH, TOK_MOD,
  TOK_EQUALS, TOK_NOT_EQUALS, TOK_LESS, TOK_GREATER,
  TOK_LESS_EQUALS, TOK_GREATER_EQUALS,
  TOK_AND, TOK_OR, TOK_NOT, TOK_NEGATE,

  // Assignment
  TOK_ASSIGN, TOK_PLUS_ASSIGN, TOK_MINUS_ASSIGN,
  TOK_MULTIPLY_ASSIGN, TOK_DIVIDE_ASSIGN,

  // Delimiters
  TOK_LPAREN, TOK_RPAREN, TOK_LBRACKET, TOK_RBRACKET,
  TOK_LBRACE, TOK_RBRACE,
  TOK_COMMA, TOK_SEMICOLON, TOK_COLON, TOK_DOT, TOK_ARROW,

  // Keywords
  TOK_VAR, TOK_FUNCTION, TOK_RETURN, TOK_IF, TOK_ELSE,
  TOK_WHILE, TOK_FOR, TOK_SWITCH, TOK_CASE, TOK_OTHERWISE,
  TOK_BREAK, TOK_CONTINUE, TOK_TYPEOF, TOK_SIZEOF,
  TOK_TRUE, TOK_FALSE,

  // Special
  TOK_NEWLINE, TOK_INDENT, TOK_DEDENT, TOK_EOF
};

struct Token {
  TokenKind kind;
  String value;
  int line;
  int column;
  int length;
};
```

**Key improvement:** Every token carries line/column info. Currently LTSL
loses this information, making error messages useless.

**Negation fix:** `TOK_NEGATE` is a distinct token from `TOK_MINUS`.
The lexer resolves ambiguity by context (after operator/separator =
negate; after value = minus).

---

### Phase 2: Parser (AST Builder)

**Input:** Token stream
**Output:** Abstract Syntax Tree

```cpp
enum ASTNodeKind {
  // Expressions
  AST_LITERAL_INT, AST_LITERAL_FLOAT, AST_LITERAL_STRING,
  AST_LITERAL_BOOL, AST_LITERAL_NULL,
  AST_IDENTIFIER, AST_BINARY_OP, AST_UNARY_OP,
  AST_FUNCTION_CALL, AST_METHOD_CALL, AST_CONSTRUCTOR,
  AST_ARRAY_LITERAL, AST_CAST, AST_INDEX, AST_MEMBER_ACCESS,

  // Statements
  AST_VAR_DECL, AST_ASSIGN, AST_RETURN,
  AST_IF, AST_WHILE, AST_FOR, AST_SWITCH,
  AST_BLOCK, AST_EXPRESSION_STMT,

  // Declarations
  AST_FUNCTION_DECL, AST_TYPE_DECL,

  // Top-level
  AST_MODULE, AST_IMPORT
};

struct SourceLocation {
  int line;
  int column;
  int length;
  String file;  // For multi-file projects
};

struct ASTNode {
  ASTNodeKind kind;
  SourceLocation loc;
  Vector<Reference<ASTNode>> children;
  // Kind-specific data stored in union or subclasses
};
```

**Key improvements:**

1. **Operator precedence in grammar** — `a + b * c` parses as
   `a + (b * c)` naturally. No rewrite needed.

2. **Method call syntax as first-class** — `obj.Method(args)` is
   syntactic sugar for `Method(obj, args)`. Resolved in Phase 3
   (SymbolResolver), not at parse time.

3. **Array literals** — `[1, 2, 3]` is valid syntax.

4. **Block scoping in grammar** — `if x { ... }` creates a new scope.

5. **Negation operator** — `-x` works everywhere (unary minus vs
   binary minus resolved by context).

6. **Comments preserved** — `#` comments are attached to the next
   AST node for docstring extraction.

**Grammar (simplified EBNF):**

```
module       = statement*
statement    = varDecl | funcDecl | typeDecl | returnStmt
             | ifStmt | whileStmt | forStmt | switchStmt
             | exprStmt
varDecl      = 'var' IDENTIFIER expr
funcDecl     = 'function' type IDENTIFIER '(' params ')' block
typeDecl     = 'type' IDENTIFIER '{' fieldDecl* '}'
ifStmt       = 'if' expr block ('else' (ifStmt | block))?
whileStmt    = 'while' expr block
forStmt      = 'for' IDENTIFIER expr expr expr '.++' block
switchStmt   = 'switch' caseBranch+ ('otherwise' block)?
exprStmt     = expr
block        = '{' statement* '}' | INDENT statement* DEDENT
expr         = literal | IDENTIFIER | binaryOp | unaryOp
             | functionCall | methodCall | constructor
             | arrayLiteral | cast | index
binaryOp     = expr ('+' | '-' | '*' | '/' | '==' | '!=' | ...) expr
unaryOp      = ('-' | '!') expr
functionCall = IDENTIFIER '(' argList ')'
methodCall   = expr '.' IDENTIFIER '(' argList ')'
constructor  = '(' type argList ')'
arrayLiteral = '[' exprList ']'
```

---

### Phase 3: Symbol Resolver (Semantic Analysis)

**Input:** Raw AST
**Output:** Typed AST with resolved references

```cpp
struct Symbol {
  String name;
  Type* type;
  int scopeLevel;
  bool isMutable;
  bool isFunction;
  SourceLocation declLoc;  // For error messages
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

  // Pass 1: Collect all declarations (functions, types, globals)
  void CollectDeclarations(ASTNode* module);

  // Pass 2: Resolve all references (variables, functions, types)
  void ResolveReferences(ASTNode* module);

  // Type inference
  Type* InferType(ASTNode* expr);

  // Error recovery (continue after errors)
  void RecoverFromError();
};
```

**Key improvements:**

1. **Two-pass compilation** — First collect declarations, then resolve
   references. Forward references work. No ordering sensitivity.

2. **Proper scoping** — Block-level scopes, shadow warnings, variable
   lifetime tracking.

3. **Type inference** — Basic type checking at compile time. Catch
   `SetPos "hello"` before runtime.

4. **Error recovery** — Continue compiling after errors. Report all
   errors at once, not just the first one.

5. **"Did you mean?" suggestions** — Levenshtein distance for
   misspelled identifiers (already implemented in current LTSL,
   preserved and improved).

---

### Phase 4: Evaluator (Runtime)

**Input:** Typed AST
**Output:** Execution results

Keep the tree-walking evaluator for now. Bytecode VM is a future
optimization if profiling shows the interpreter is a bottleneck.

```cpp
class Evaluator {
  Vector<Reference<Scope>> scopeStack;

  Value Evaluate(ASTNode* node);
  Value EvaluateFunctionCall(ASTNode* call);
  Value EvaluateMethodCall(ASTNode* call);
  Value EvaluateBinaryOp(ASTNode* op);
  Value EvaluateUnaryOp(ASTNode* op);
  // ... etc
};
```

**Compatibility:** The evaluator calls the same C++ engine functions
via `FunctionBind`/`Function_Alias`. No changes needed to the engine
bridge.

---

## Error Message Quality

**Current (bad):**
```
[Warning] (Initialize.Initialize.Item_ShipType.Main) Unused variable in Shader(...)
```

**Proposed (good):**
```
Error: Undefined variable 'spawnR' at ltheory-main.lts:42:15
  42 | s3.SetPos (planetPos + (Vec3 (~ spawnR) 0 0))
     |                                    ^^^^^^^
     |
  Did you mean 'spawnDir'?
    Note: 'spawnDir' is declared at ltheory-main.lts:38:3
```

---

## Migration Strategy

### Step 1: Build New Compiler Alongside Old Interpreter

- New compiler lives in `src/liblt/LTE/Compiler/`
- Old interpreter stays in `src/liblt/LTE/Expression.cpp` + `LTSL.cpp`
- Both coexist; scripts use old interpreter by default
- Feature flag: `--compiler v2` enables new compiler

### Step 2: Migrate Scripts Gradually

- Start with `ltheory-main.lts` (our test app)
- Then `Widget/DevPanel.lts`, `Widget/HUD.lts` (UI scripts)
- Then `Object/SystemPopulate.lts` (procedural generation)
- Then remaining scripts

### Step 3: Remove Old Interpreter

- Once all scripts compile with new compiler
- Delete `Expression.cpp` + `Expression/*.cpp`
- Delete old `LTSL.cpp` evaluator
- Keep `FunctionBind.h`/`Function_Bind`/`Function_Alias` (engine bridge)

---

## File Layout (New)

```
src/liblt/LTE/Compiler/
  Lexer.h / Lexer.cpp          — Tokenizer (~800 LOC)
  Parser.h / Parser.cpp        — AST builder (~1,200 LOC)
  AST.h                        — AST node definitions (~400 LOC)
  SymbolResolver.h / .cpp      — Semantic analysis (~1,000 LOC)
  CompileError.h               — Error types (~100 LOC)
  Compiler.h / Compiler.cpp    — Top-level API (~200 LOC)

src/liblt/LTE/Evaluator.h / .cpp  — New tree-walker (~1,500 LOC, adapted)
```

**Total new code:** ~5,200 LOC
**Removed code:** ~2,854 LOC (old Expression + LTSL)
**Net change:** ~+2,350 LOC

---

## Estimated Effort

| Phase | LOC | Time | Dependencies |
|-------|-----|------|--------------|
| Phase 1: Lexer | ~800 | 1 week | None |
| Phase 2: Parser | ~1,200 | 1.5 weeks | Phase 1 |
| Phase 3: SymbolResolver | ~1,000 | 1.5 weeks | Phase 2 |
| Phase 4: Evaluator adaptation | ~1,500 | 1 week | Phase 3 |
| Script migration | — | 2 weeks | Phase 4 |
| Old interpreter removal | — | 1 day | All scripts migrated |
| **Total** | **~5,200** | **~7 weeks** | |

---

## Risk Mitigation

1. **Backwards compatibility** — New compiler accepts old LTSL syntax.
   All existing scripts must compile without changes.

2. **Engine bridge unchanged** — `FunctionBind.h`, `Function_Bind`,
   `Function_Alias`, `DefineConversion` are not touched.

3. **Test suite** — `tests/TestScriptCompile.cpp` (21 tests) must pass
   with new compiler. Add more tests for new features.

4. **LSP integration** — Update `script/ltsl-lsp/` to use new compiler's
   AST and symbol table. The LSP becomes much simpler (no duplicate
   parser).

---

## Priority: Quick Wins Before Full Migration

If the full 7-week migration is too long, these quick fixes (1-2 days
each) address the worst pain points in the current interpreter:

1. **Negation operator** — Add `TOK_NEGATE` to current parser.
   Fixes `(Vec3 (~ x) 0 0)` silently doing nothing.

2. **Silent statement drop warning** — When an arity check fails,
   log a warning with line number instead of silently dropping.

3. **Better error messages** — Add line/column tracking to all
   compile errors. Already partially done (A.8), extend to all.

4. **`switch` statement improvements** — Allow more than 2 cases.
   Current LTSL switch is limited.

5. **Array literal syntax** — Add `[1, 2, 3]` as sugar for
   `(Array 1 2 3)`.

These quick wins can be done NOW while the full migration is planned.

---

## Success Criteria

After migration, the following should be easy:

- [ ] Create a new ship archetype by adding a JSON entry + LTSL script
- [ ] Spawn all7 archetypes in a test scene without silent failures
- [ ] Get clear error messages when a function call has wrong args
- [ ] Use forward references (call a function before it's declared)
- [ ] Use array literals `[1, 2, 3]` instead of `(Array 1 2 3)`
- [ ] Negate variables with `-x` everywhere
- [ ] See hover docs for all engine functions in ZED
- [ ] Prototype a new UI widget in under 30 minutes
- [ ] Live-edit game parameters and see changes without restart

---

## Appendix: Current LTSL Pain Points (User-Reported)

1. `~ spawnR` silently does nothing (not a negation operator)
2. Method call order confusion (`a.b` → `(b a)`)
3. Silent statement drops when arity check fails
4. No forward references
5. Hard to understand Josh's original code (single-char variables)
6. LSP not working in ZED
7. Hard to prototype UI widgets
8. Hard to add in-game dev tools
9. Hard to create maps/worlds/levels quickly
10. Order-dependent compilation breaks simple scripts

This migration plan addresses all 10 items.
