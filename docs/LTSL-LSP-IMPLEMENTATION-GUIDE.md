# LTSL Language Server Protocol (LSP) Implementation Guide

**Purpose:** Complete guide to building a Language Server for LTSL with IntelliSense, autocomplete, diagnostics, and hover support.  
**Audience:** Developers wanting IDE support for `.lts` files  
**Status:** Shipped for ZED (Linux) — tree-sitter grammar + TypeScript LSP server  
**Last Updated:** 2026-07-31

---

## Table of Contents

1. [Overview & Architecture](#1-overview--architecture)
2. [LTSL Language Analysis](#2-ltsl-language-analysis)
3. [LSP Server Implementation (TypeScript)](#3-lsp-server-implementation-typescript)
4. [ZED Extension](#4-zed-extension-shipped)
5. [Advanced Features](#5-advanced-features)
6. [Deployment & Distribution](#6-deployment--distribution)
7. [Future Enhancements](#7-future-enhancements)

---

## 1. Overview & Architecture

### What is an LSP?

The **Language Server Protocol** (LSP) is a standard protocol between code editors and language servers that provide language-specific features:
- **Autocomplete** (IntelliSense)
- **Diagnostics** (errors, warnings)
- **Hover tooltips** (type info, docs)
- **Go to Definition**
- **Find References**
- **Code formatting**

### Why Build an LSP for LTSL?

**Before the LSP:**
- ❌ No syntax highlighting
- ❌ No autocomplete for engine APIs (`Object_Ship`, `Item_WeaponType`, etc.)
- ❌ No error checking until runtime
- ❌ No type hints or documentation on hover
- ❌ Manual API reference lookups in docs

**Shipped (ZED on Linux, via the LSP + tree-sitter extension):**
- ✅ Syntax highlighting for `.lts` files
- ✅ Autocomplete for all engine functions, types, and variables
- ✅ Real-time error detection (typos, wrong argument counts)
- ✅ Hover tooltips with function signatures and docs
- ✅ Signature help on function calls

**Planned (not yet implemented):**
- ⏳ Go to Definition for user-defined functions/types
- ⏳ Code actions / quick fixes
- ⏳ Snippet templates for common patterns

### Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                       ZED Editor                             │
│  (or any LSP-compatible editor: VS Code, JetBrains, Neovim) │
└──────────────────────────┬──────────────────────────────────┘
                           │ LSP Protocol (JSON-RPC)
                           │
┌──────────────────────────▼──────────────────────────────────┐
│                   LTSL Language Server                       │
│  - Tokenizer (lexical analysis)                             │
│  - Parser (syntax tree)                                      │
│  - Semantic analyzer (type checking, scoping)                │
│  - Completion provider (autocomplete)                        │
│  - Diagnostics provider (errors/warnings)                    │
│  - Hover provider (tooltips)                                 │
│  - Definition provider (go-to-definition)                    │
└──────────────────────────┬──────────────────────────────────┘
                           │
                           │ Reads API metadata
                           │
┌──────────────────────────▼──────────────────────────────────┐
│              LTSL API Database (JSON)                        │
│  - Engine functions (Object_Ship, Item_WeaponType, ...)     │
│  - Type definitions (Object, Player, Camera, ...)            │
│  - Built-in operators (+, -, *, /, ==, !=, ...)             │
│  - Stdlib functions (Vec3, RNG_MTG, Sound_Play, ...)         │
└──────────────────────────────────────────────────────────────┘
```

---

## 2. LTSL Language Analysis

### 2.1 Tokenization (Lexical Analysis)

LTSL's tokenizer (`src/liblt/LTE/Tokenizer.h`) is a simple character-by-character scanner:

**Token types:**
```typescript
enum TokenType {
  // Literals
  NUMBER,           // 123, 45.67, 0.5
  STRING,           // "hello world"
  IDENTIFIER,       // myVariable, Object_Ship
  
  // Keywords
  FUNCTION,         // function
  TYPE,             // type
  VAR,              // var
  IF,               // if
  SWITCH,           // switch
  FOR,              // for
  WHILE,            // while
  RETURN,           // return
  STATIC,           // static
  REF,              // ref
  CAST,             // cast
  OTHERWISE,        // otherwise
  
  // Operators
  PLUS,             // +
  MINUS,            // -
  STAR,             // *
  SLASH,            // /
  EQUALS,           // =
  EQUALITY,         // ==
  NOT_EQUALS,       // !=
  LESS_THAN,        // <
  GREATER_THAN,     // >
  LESS_EQUAL,       // <=
  GREATER_EQUAL,    // >=
  LOGICAL_AND,      // &&
  LOGICAL_OR,       // ||
  LOGICAL_NOT,      // !
  DOT,              // .
  COMMA,            // ,
  
  // Delimiters
  LPAREN,           // (
  RPAREN,           // )
  LBRACE,           // {
  RBRACE,           // }
  LBRACKET,         // [
  RBRACKET,         // ]
  
  // Special
  NEWLINE,          // \n
  EOF,              // End of file
  COMMENT,          // # comment
}

interface Token {
  type: TokenType;
  value: string;
  line: number;
  column: number;
  start: number;      // Byte offset in file
  end: number;
}
```

**Tokenizer implementation (TypeScript):**

```typescript
// ltsl-lsp/src/lexer.ts
export class LTSLLexer {
  private source: string;
  private cursor: number = 0;
  private line: number = 1;
  private column: number = 1;
  private tokens: Token[] = [];
  
  // LTSL keywords
  private keywords = new Set([
    'function', 'type', 'var', 'if', 'switch', 'case',
    'for', 'while', 'return', 'static', 'ref', 'cast',
    'otherwise', 'Array', 'List', 'Map', 'Vector'
  ]);
  
  constructor(source: string) {
    this.source = source;
  }
  
  tokenize(): Token[] {
    this.tokens = [];
    
    while (this.cursor < this.source.length) {
      this.skipWhitespace();
      if (this.cursor >= this.source.length) break;
      
      const char = this.peek();
      
      // Comments (# to end of line)
      if (char === '#') {
        this.skipComment();
        continue;
      }
      
      // String literals
      if (char === '"') {
        this.tokenizeString();
        continue;
      }
      
      // Numbers
      if (this.isDigit(char)) {
        this.tokenizeNumber();
        continue;
      }
      
      // Identifiers and keywords
      if (this.isIdentifierStart(char)) {
        this.tokenizeIdentifier();
        continue;
      }
      
      // Operators and delimiters
      this.tokenizeOperator();
    }
    
    this.tokens.push({
      type: TokenType.EOF,
      value: '',
      line: this.line,
      column: this.column,
      start: this.cursor,
      end: this.cursor
    });
    
    return this.tokens;
  }
  
  private peek(offset: number = 0): string {
    return this.source[this.cursor + offset] || '';
  }
  
  private advance(): string {
    const char = this.source[this.cursor++];
    if (char === '\n') {
      this.line++;
      this.column = 1;
    } else {
      this.column++;
    }
    return char;
  }
  
  private skipWhitespace(): void {
    while (/\s/.test(this.peek())) {
      this.advance();
    }
  }
  
  private skipComment(): void {
    // Skip until end of line
    while (this.peek() !== '\n' && this.cursor < this.source.length) {
      this.advance();
    }
  }
  
  private tokenizeString(): void {
    const start = this.cursor;
    const startLine = this.line;
    const startColumn = this.column;
    
    this.advance(); // Skip opening "
    let value = '';
    let escaped = false;
    
    while (this.cursor < this.source.length) {
      const char = this.peek();
      
      if (escaped) {
        value += char;
        escaped = false;
        this.advance();
        continue;
      }
      
      if (char === '\\') {
        escaped = true;
        this.advance();
        continue;
      }
      
      if (char === '"') {
        this.advance(); // Skip closing "
        break;
      }
      
      value += char;
      this.advance();
    }
    
    this.tokens.push({
      type: TokenType.STRING,
      value: value,
      line: startLine,
      column: startColumn,
      start: start,
      end: this.cursor
    });
  }
  
  private tokenizeNumber(): void {
    const start = this.cursor;
    const startLine = this.line;
    const startColumn = this.column;
    let value = '';
    
    // Integer part
    while (this.isDigit(this.peek())) {
      value += this.advance();
    }
    
    // Decimal part
    if (this.peek() === '.' && this.isDigit(this.peek(1))) {
      value += this.advance(); // .
      while (this.isDigit(this.peek())) {
        value += this.advance();
      }
    }
    
    // Scientific notation (optional)
    if (this.peek().toLowerCase() === 'e') {
      value += this.advance(); // e
      if (this.peek() === '+' || this.peek() === '-') {
        value += this.advance();
      }
      while (this.isDigit(this.peek())) {
        value += this.advance();
      }
    }
    
    this.tokens.push({
      type: TokenType.NUMBER,
      value: value,
      line: startLine,
      column: startColumn,
      start: start,
      end: this.cursor
    });
  }
  
  private tokenizeIdentifier(): void {
    const start = this.cursor;
    const startLine = this.line;
    const startColumn = this.column;
    let value = '';
    
    // LTSL allows alphanumeric, underscores, and '::' namespace separator
    while (this.isIdentifierChar(this.peek()) || 
           (this.peek() === ':' && this.peek(1) === ':')) {
      if (this.peek() === ':') {
        value += this.advance(); // :
        value += this.advance(); // :
      } else {
        value += this.advance();
      }
    }
    
    const type = this.keywords.has(value) 
      ? this.getKeywordType(value)
      : TokenType.IDENTIFIER;
    
    this.tokens.push({
      type: type,
      value: value,
      line: startLine,
      column: startColumn,
      start: start,
      end: this.cursor
    });
  }
  
  private tokenizeOperator(): void {
    const start = this.cursor;
    const startLine = this.line;
    const startColumn = this.column;
    const char = this.peek();
    const next = this.peek(1);
    
    let type: TokenType;
    let value: string;
    
    // Two-character operators
    if (char === '=' && next === '=') {
      type = TokenType.EQUALITY;
      value = '==';
      this.advance();
      this.advance();
    } else if (char === '!' && next === '=') {
      type = TokenType.NOT_EQUALS;
      value = '!=';
      this.advance();
      this.advance();
    } else if (char === '<' && next === '=') {
      type = TokenType.LESS_EQUAL;
      value = '<=';
      this.advance();
      this.advance();
    } else if (char === '>' && next === '=') {
      type = TokenType.GREATER_EQUAL;
      value = '>=';
      this.advance();
      this.advance();
    } else if (char === '&' && next === '&') {
      type = TokenType.LOGICAL_AND;
      value = '&&';
      this.advance();
      this.advance();
    } else if (char === '|' && next === '|') {
      type = TokenType.LOGICAL_OR;
      value = '||';
      this.advance();
      this.advance();
    } else {
      // Single-character operators
      value = this.advance();
      type = this.getOperatorType(char);
    }
    
    this.tokens.push({
      type: type,
      value: value,
      line: startLine,
      column: startColumn,
      start: start,
      end: this.cursor
    });
  }
  
  private isDigit(char: string): boolean {
    return /[0-9]/.test(char);
  }
  
  private isIdentifierStart(char: string): boolean {
    return /[a-zA-Z_]/.test(char);
  }
  
  private isIdentifierChar(char: string): boolean {
    return /[a-zA-Z0-9_]/.test(char);
  }
  
  private getKeywordType(keyword: string): TokenType {
    const map: { [key: string]: TokenType } = {
      'function': TokenType.FUNCTION,
      'type': TokenType.TYPE,
      'var': TokenType.VAR,
      'if': TokenType.IF,
      'switch': TokenType.SWITCH,
      'for': TokenType.FOR,
      'while': TokenType.WHILE,
      'return': TokenType.RETURN,
      'static': TokenType.STATIC,
      'ref': TokenType.REF,
      'cast': TokenType.CAST,
      'otherwise': TokenType.OTHERWISE,
    };
    return map[keyword] || TokenType.IDENTIFIER;
  }
  
  private getOperatorType(char: string): TokenType {
    const map: { [key: string]: TokenType } = {
      '+': TokenType.PLUS,
      '-': TokenType.MINUS,
      '*': TokenType.STAR,
      '/': TokenType.SLASH,
      '=': TokenType.EQUALS,
      '<': TokenType.LESS_THAN,
      '>': TokenType.GREATER_THAN,
      '!': TokenType.LOGICAL_NOT,
      '.': TokenType.DOT,
      ',': TokenType.COMMA,
      '(': TokenType.LPAREN,
      ')': TokenType.RPAREN,
      '{': TokenType.LBRACE,
      '}': TokenType.RBRACE,
      '[': TokenType.LBRACKET,
      ']': TokenType.RBRACKET,
    };
    return map[char] || TokenType.IDENTIFIER;
  }
}
```

### 2.2 Parsing (Syntax Analysis)

LTSL uses a **tree-walking parser** that builds an AST (Abstract Syntax Tree):

**AST Node types:**
```typescript
// ltsl-lsp/src/ast.ts
export enum ASTNodeType {
  Program,
  FunctionDecl,
  TypeDecl,
  VariableDecl,
  FunctionCall,
  BinaryOp,
  UnaryOp,
  Literal,
  Identifier,
  Block,
  IfStatement,
  SwitchStatement,
  ForLoop,
  WhileLoop,
  ReturnStatement,
}

export interface ASTNode {
  type: ASTNodeType;
  start: number;
  end: number;
  line: number;
  column: number;
}

export interface Program extends ASTNode {
  type: ASTNodeType.Program;
  declarations: Declaration[];
}

export interface FunctionDecl extends ASTNode {
  type: ASTNodeType.FunctionDecl;
  returnType: string;
  name: string;
  parameters: Parameter[];
  body: Block;
}

export interface Parameter {
  type: string;
  name: string;
}

export interface TypeDecl extends ASTNode {
  type: ASTNodeType.TypeDecl;
  name: string;
  fields: Field[];
  methods: FunctionDecl[];
}

export interface Field {
  type: string;
  name: string;
  defaultValue?: Expression;
}

export interface VariableDecl extends ASTNode {
  type: ASTNodeType.VariableDecl;
  name: string;
  value: Expression;
  isStatic?: boolean;
  isRef?: boolean;
}

export interface FunctionCall extends ASTNode {
  type: ASTNodeType.FunctionCall;
  callee: Expression;
  arguments: Expression[];
}
```

**Parser implementation:**
```typescript
// ltsl-lsp/src/parser.ts
export class LTSLParser {
  private tokens: Token[];
  private current: number = 0;
  
  constructor(tokens: Token[]) {
    this.tokens = tokens;
  }
  
  parse(): Program {
    const declarations: Declaration[] = [];
    
    while (!this.isAtEnd()) {
      try {
        declarations.push(this.declaration());
      } catch (error) {
        // Error recovery: skip to next declaration
        this.synchronize();
      }
    }
    
    return {
      type: ASTNodeType.Program,
      declarations: declarations,
      start: 0,
      end: this.tokens[this.tokens.length - 1].end,
      line: 1,
      column: 1
    };
  }
  
  private declaration(): Declaration {
    if (this.match(TokenType.FUNCTION)) {
      return this.functionDeclaration();
    }
    if (this.match(TokenType.TYPE)) {
      return this.typeDeclaration();
    }
    
    // Top-level variable declaration
    if (this.match(TokenType.VAR)) {
      return this.variableDeclaration();
    }
    
    throw this.error('Expected function, type, or variable declaration');
  }
  
  private functionDeclaration(): FunctionDecl {
    const start = this.previous().start;
    
    // Return type
    const returnType = this.consume(TokenType.IDENTIFIER, 'Expected return type').value;
    
    // Function name
    const name = this.consume(TokenType.IDENTIFIER, 'Expected function name').value;
    
    // Parameters
    this.consume(TokenType.LPAREN, 'Expected (');
    const parameters: Parameter[] = [];
    
    if (!this.check(TokenType.RPAREN)) {
      do {
        const paramType = this.consume(TokenType.IDENTIFIER, 'Expected parameter type').value;
        const paramName = this.consume(TokenType.IDENTIFIER, 'Expected parameter name').value;
        parameters.push({ type: paramType, name: paramName });
      } while (this.match(TokenType.COMMA));
    }
    
    this.consume(TokenType.RPAREN, 'Expected )');
    
    // Function body
    const body = this.block();
    
    return {
      type: ASTNodeType.FunctionDecl,
      returnType: returnType,
      name: name,
      parameters: parameters,
      body: body,
      start: start,
      end: this.previous().end,
      line: this.tokens[0].line,
      column: this.tokens[0].column
    };
  }
  
  private typeDeclaration(): TypeDecl {
    const start = this.previous().start;
    const name = this.consume(TokenType.IDENTIFIER, 'Expected type name').value;
    
    const fields: Field[] = [];
    const methods: FunctionDecl[] = [];
    
    // Parse type body (fields and methods)
    while (!this.isAtEnd() && !this.check(TokenType.TYPE) && !this.check(TokenType.FUNCTION)) {
      if (this.match(TokenType.FUNCTION)) {
        methods.push(this.functionDeclaration());
      } else if (this.check(TokenType.IDENTIFIER)) {
        // Field declaration: TypeName fieldName defaultValue?
        const fieldType = this.advance().value;
        const fieldName = this.consume(TokenType.IDENTIFIER, 'Expected field name').value;
        
        let defaultValue: Expression | undefined;
        if (!this.check(TokenType.IDENTIFIER) && !this.check(TokenType.FUNCTION)) {
          // Has default value
          defaultValue = this.expression();
        }
        
        fields.push({ type: fieldType, name: fieldName, defaultValue: defaultValue });
      } else {
        break;
      }
    }
    
    return {
      type: ASTNodeType.TypeDecl,
      name: name,
      fields: fields,
      methods: methods,
      start: start,
      end: this.previous().end,
      line: this.tokens[0].line,
      column: this.tokens[0].column
    };
  }
  
  private block(): Block {
    const start = this.peek().start;
    const statements: Statement[] = [];
    
    // LTSL blocks are indent-sensitive or brace-delimited
    if (this.match(TokenType.LBRACE)) {
      while (!this.check(TokenType.RBRACE) && !this.isAtEnd()) {
        statements.push(this.statement());
      }
      this.consume(TokenType.RBRACE, 'Expected }');
    } else {
      // Indent-based block (parse until dedent)
      // For simplicity, just parse one statement
      statements.push(this.statement());
    }
    
    return {
      type: ASTNodeType.Block,
      statements: statements,
      start: start,
      end: this.previous().end,
      line: this.tokens[0].line,
      column: this.tokens[0].column
    };
  }
  
  private statement(): Statement {
    if (this.match(TokenType.VAR)) {
      return this.variableDeclaration();
    }
    if (this.match(TokenType.IF)) {
      return this.ifStatement();
    }
    if (this.match(TokenType.SWITCH)) {
      return this.switchStatement();
    }
    if (this.match(TokenType.FOR)) {
      return this.forLoop();
    }
    if (this.match(TokenType.WHILE)) {
      return this.whileLoop();
    }
    if (this.match(TokenType.RETURN)) {
      return this.returnStatement();
    }
    
    // Expression statement
    return this.expressionStatement();
  }
  
  // ... more parsing methods
  
  private match(...types: TokenType[]): boolean {
    for (const type of types) {
      if (this.check(type)) {
        this.advance();
        return true;
      }
    }
    return false;
  }
  
  private check(type: TokenType): boolean {
    if (this.isAtEnd()) return false;
    return this.peek().type === type;
  }
  
  private advance(): Token {
    if (!this.isAtEnd()) this.current++;
    return this.previous();
  }
  
  private isAtEnd(): boolean {
    return this.peek().type === TokenType.EOF;
  }
  
  private peek(): Token {
    return this.tokens[this.current];
  }
  
  private previous(): Token {
    return this.tokens[this.current - 1];
  }
  
  private consume(type: TokenType, message: string): Token {
    if (this.check(type)) return this.advance();
    throw this.error(message);
  }
  
  private error(message: string): Error {
    const token = this.peek();
    return new Error(`Parse error at line ${token.line}:${token.column}: ${message}`);
  }
  
  private synchronize(): void {
    this.advance();
    
    while (!this.isAtEnd()) {
      // Skip to next statement
      if (this.previous().type === TokenType.NEWLINE) return;
      
      switch (this.peek().type) {
        case TokenType.FUNCTION:
        case TokenType.TYPE:
        case TokenType.VAR:
        case TokenType.IF:
        case TokenType.FOR:
        case TokenType.WHILE:
        case TokenType.RETURN:
          return;
      }
      
      this.advance();
    }
  }
}
```

### 2.3 Semantic Analysis

**Type checking and scope resolution:**

```typescript
// ltsl-lsp/src/analyzer.ts
export class LTSLAnalyzer {
  private symbols: Map<string, Symbol> = new Map();
  private scopes: Scope[] = [];
  private diagnostics: Diagnostic[] = [];
  private apiDatabase: APIDatabase;
  
  constructor(apiDatabase: APIDatabase) {
    this.apiDatabase = apiDatabase;
    this.pushScope(); // Global scope
  }
  
  analyze(ast: Program): Diagnostic[] {
    this.diagnostics = [];
    
    for (const decl of ast.declarations) {
      this.analyzeDeclaration(decl);
    }
    
    return this.diagnostics;
  }
  
  private analyzeDeclaration(decl: Declaration): void {
    switch (decl.type) {
      case ASTNodeType.FunctionDecl:
        this.analyzeFunctionDecl(decl as FunctionDecl);
        break;
      case ASTNodeType.TypeDecl:
        this.analyzeTypeDecl(decl as TypeDecl);
        break;
      case ASTNodeType.VariableDecl:
        this.analyzeVariableDecl(decl as VariableDecl);
        break;
    }
  }
  
  private analyzeFunctionDecl(decl: FunctionDecl): void {
    // Check if function already defined
    if (this.currentScope().has(decl.name)) {
      this.error(
        `Function '${decl.name}' already defined`,
        decl.line,
        decl.column
      );
    }
    
    // Register function in symbol table
    this.currentScope().define(decl.name, {
      kind: 'function',
      type: decl.returnType,
      params: decl.parameters,
      node: decl
    });
    
    // Analyze function body in new scope
    this.pushScope();
    
    // Add parameters to scope
    for (const param of decl.parameters) {
      this.currentScope().define(param.name, {
        kind: 'parameter',
        type: param.type,
        node: decl
      });
    }
    
    this.analyzeBlock(decl.body);
    this.popScope();
  }
  
  private analyzeFunctionCall(call: FunctionCall): string | null {
    // Get function symbol
    if (call.callee.type === ASTNodeType.Identifier) {
      const name = (call.callee as Identifier).name;
      
      // Check local symbols
      const symbol = this.resolve(name);
      if (symbol && symbol.kind === 'function') {
        // Check argument count
        if (call.arguments.length !== symbol.params.length) {
          this.error(
            `Function '${name}' expects ${symbol.params.length} arguments, got ${call.arguments.length}`,
            call.line,
            call.column
          );
        }
        
        // Type check arguments
        for (let i = 0; i < call.arguments.length; i++) {
          const argType = this.inferType(call.arguments[i]);
          const paramType = symbol.params[i].type;
          
          if (argType && argType !== paramType) {
            this.warning(
              `Argument ${i + 1} type mismatch: expected '${paramType}', got '${argType}'`,
              call.line,
              call.column
            );
          }
        }
        
        return symbol.type; // Return type
      }
      
      // Check engine API
      const apiFunc = this.apiDatabase.getFunction(name);
      if (apiFunc) {
        // Check argument count
        if (call.arguments.length !== apiFunc.parameters.length) {
          this.error(
            `Function '${name}' expects ${apiFunc.parameters.length} arguments, got ${call.arguments.length}`,
            call.line,
            call.column
          );
        }
        
        return apiFunc.returnType;
      }
      
      // Unknown function
      this.error(
        `Undefined function '${name}'`,
        call.line,
        call.column
      );
    }
    
    return null;
  }
  
  private inferType(expr: Expression): string | null {
    switch (expr.type) {
      case ASTNodeType.Literal:
        const lit = expr as Literal;
        if (lit.value.type === TokenType.NUMBER) {
          return lit.value.value.includes('.') ? 'Float' : 'Int';
        }
        if (lit.value.type === TokenType.STRING) {
          return 'String';
        }
        break;
        
      case ASTNodeType.Identifier:
        const ident = expr as Identifier;
        const symbol = this.resolve(ident.name);
        return symbol ? symbol.type : null;
        
      case ASTNodeType.FunctionCall:
        return this.analyzeFunctionCall(expr as FunctionCall);
        
      case ASTNodeType.BinaryOp:
        // Type inference for binary operators
        const binOp = expr as BinaryOp;
        const leftType = this.inferType(binOp.left);
        const rightType = this.inferType(binOp.right);
        
        if (leftType === 'Int' && rightType === 'Int') {
          return 'Int';
        }
        if ((leftType === 'Float' || leftType === 'Int') && 
            (rightType === 'Float' || rightType === 'Int')) {
          return 'Float';
        }
        
        return leftType; // Best guess
    }
    
    return null;
  }
  
  private pushScope(): void {
    this.scopes.push(new Scope());
  }
  
  private popScope(): void {
    this.scopes.pop();
  }
  
  private currentScope(): Scope {
    return this.scopes[this.scopes.length - 1];
  }
  
  private resolve(name: string): Symbol | null {
    // Search from innermost to outermost scope
    for (let i = this.scopes.length - 1; i >= 0; i--) {
      if (this.scopes[i].has(name)) {
        return this.scopes[i].get(name);
      }
    }
    return null;
  }
  
  private error(message: string, line: number, column: number): void {
    this.diagnostics.push({
      severity: DiagnosticSeverity.Error,
      range: {
        start: { line: line - 1, character: column - 1 },
        end: { line: line - 1, character: column + 10 }
      },
      message: message,
      source: 'ltsl'
    });
  }
  
  private warning(message: string, line: number, column: number): void {
    this.diagnostics.push({
      severity: DiagnosticSeverity.Warning,
      range: {
        start: { line: line - 1, character: column - 1 },
        end: { line: line - 1, character: column + 10 }
      },
      message: message,
      source: 'ltsl'
    });
  }
}

class Scope {
  private symbols: Map<string, Symbol> = new Map();
  
  define(name: string, symbol: Symbol): void {
    this.symbols.set(name, symbol);
  }
  
  get(name: string): Symbol | null {
    return this.symbols.get(name) || null;
  }
  
  has(name: string): boolean {
    return this.symbols.has(name);
  }
}

interface Symbol {
  kind: 'function' | 'type' | 'variable' | 'parameter';
  type: string;
  params?: Parameter[];
  node: ASTNode;
}
```

---

## 3. LSP Server Implementation (TypeScript)

### 3.1 Project Setup

```bash
# Create LSP project
mkdir ltsl-lsp
cd ltsl-lsp
npm init -y

# Install dependencies
npm install --save vscode-languageserver vscode-languageserver-textdocument
npm install --save-dev @types/node typescript

# Initialize TypeScript
npx tsc --init
```

**`tsconfig.json`:**
```json
{
  "compilerOptions": {
    "target": "ES2020",
    "module": "commonjs",
    "lib": ["ES2020"],
    "outDir": "./out",
    "rootDir": "./src",
    "sourceMap": true,
    "strict": true,
    "esModuleInterop": true,
    "skipLibCheck": true,
    "forceConsistentCasingInFileNames": true
  },
  "include": ["src/**/*"],
  "exclude": ["node_modules", "out"]
}
```

**`package.json`:**
```json
{
  "name": "ltsl-language-server",
  "version": "1.0.0",
  "description": "Language Server for LTSL (Limit Theory Scripting Language)",
  "main": "./out/server.js",
  "scripts": {
    "compile": "tsc -p .",
    "watch": "tsc -watch -p .",
    "test": "node ./out/test.js"
  },
  "dependencies": {
    "vscode-languageserver": "^8.0.0",
    "vscode-languageserver-textdocument": "^1.0.8"
  },
  "devDependencies": {
    "@types/node": "^20.0.0",
    "typescript": "^5.0.0"
  }
}
```

### 3.2 Main Server Implementation

```typescript
// ltsl-lsp/src/server.ts
import {
  createConnection,
  TextDocuments,
  ProposedFeatures,
  InitializeParams,
  DidChangeConfigurationNotification,
  CompletionItem,
  CompletionItemKind,
  TextDocumentPositionParams,
  TextDocumentSyncKind,
  InitializeResult,
  Hover,
  MarkupKind,
  Definition,
  Location
} from 'vscode-languageserver/node';

import { TextDocument } from 'vscode-languageserver-textdocument';
import { LTSLLexer } from './lexer';
import { LTSLParser } from './parser';
import { LTSLAnalyzer } from './analyzer';
import { APIDatabase } from './api-database';

// Create connection
const connection = createConnection(ProposedFeatures.all);

// Create document manager
const documents: TextDocuments<TextDocument> = new TextDocuments(TextDocument);

// Load API database
const apiDatabase = new APIDatabase();
apiDatabase.loadFromFile('./api-database.json');

let hasConfigurationCapability = false;
let hasWorkspaceFolderCapability = false;

connection.onInitialize((params: InitializeParams) => {
  const capabilities = params.capabilities;
  
  hasConfigurationCapability = !!(
    capabilities.workspace && !!capabilities.workspace.configuration
  );
  hasWorkspaceFolderCapability = !!(
    capabilities.workspace && !!capabilities.workspace.workspaceFolders
  );
  
  const result: InitializeResult = {
    capabilities: {
      textDocumentSync: TextDocumentSyncKind.Full,
      completionProvider: {
        resolveProvider: true,
        triggerCharacters: ['.', ':', '(']
      },
      hoverProvider: true,
      definitionProvider: true,
    }
  };
  
  if (hasWorkspaceFolderCapability) {
    result.capabilities.workspace = {
      workspaceFolders: {
        supported: true
      }
    };
  }
  
  return result;
});

connection.onInitialized(() => {
  if (hasConfigurationCapability) {
    connection.client.register(DidChangeConfigurationNotification.type, undefined);
  }
  
  connection.console.log('LTSL Language Server initialized');
});

// Document validation
documents.onDidChangeContent(change => {
  validateTextDocument(change.document);
});

async function validateTextDocument(textDocument: TextDocument): Promise<void> {
  const text = textDocument.getText();
  
  try {
    // Tokenize
    const lexer = new LTSLLexer(text);
    const tokens = lexer.tokenize();
    
    // Parse
    const parser = new LTSLParser(tokens);
    const ast = parser.parse();
    
    // Analyze
    const analyzer = new LTSLAnalyzer(apiDatabase);
    const diagnostics = analyzer.analyze(ast);
    
    // Send diagnostics to client
    connection.sendDiagnostics({ uri: textDocument.uri, diagnostics });
  } catch (error) {
    connection.console.error(`Validation error: ${error}`);
  }
}

// Completion provider
connection.onCompletion(
  (textDocumentPosition: TextDocumentPositionParams): CompletionItem[] => {
    const document = documents.get(textDocumentPosition.textDocument.uri);
    if (!document) {
      return [];
    }
    
    const text = document.getText();
    const offset = document.offsetAt(textDocumentPosition.position);
    
    // Get word at cursor
    const wordRange = getWordRangeAtPosition(text, offset);
    const word = text.substring(wordRange.start, wordRange.end);
    
    const completions: CompletionItem[] = [];
    
    // Add engine API functions
    for (const func of apiDatabase.getAllFunctions()) {
      if (func.name.startsWith(word)) {
        completions.push({
          label: func.name,
          kind: CompletionItemKind.Function,
          detail: func.signature,
          documentation: {
            kind: MarkupKind.Markdown,
            value: func.documentation
          },
          insertText: func.name,
          data: { type: 'function', name: func.name }
        });
      }
    }
    
    // Add engine types
    for (const type of apiDatabase.getAllTypes()) {
      if (type.name.startsWith(word)) {
        completions.push({
          label: type.name,
          kind: CompletionItemKind.Class,
          detail: type.description,
          documentation: {
            kind: MarkupKind.Markdown,
            value: type.documentation
          },
          insertText: type.name,
          data: { type: 'type', name: type.name }
        });
      }
    }
    
    // Add keywords
    const keywords = [
      'function', 'type', 'var', 'if', 'switch', 'case', 'for', 'while',
      'return', 'static', 'ref', 'cast', 'otherwise'
    ];
    
    for (const keyword of keywords) {
      if (keyword.startsWith(word.toLowerCase())) {
        completions.push({
          label: keyword,
          kind: CompletionItemKind.Keyword,
          insertText: keyword
        });
      }
    }
    
    return completions;
  }
);

// Completion resolve (additional details)
connection.onCompletionResolve(
  (item: CompletionItem): CompletionItem => {
    if (item.data && item.data.type === 'function') {
      const func = apiDatabase.getFunction(item.data.name);
      if (func) {
        item.detail = func.signature;
        item.documentation = {
          kind: MarkupKind.Markdown,
          value: func.documentation
        };
      }
    }
    return item;
  }
);

// Hover provider
connection.onHover(
  (textDocumentPosition: TextDocumentPositionParams): Hover | null => {
    const document = documents.get(textDocumentPosition.textDocument.uri);
    if (!document) {
      return null;
    }
    
    const text = document.getText();
    const offset = document.offsetAt(textDocumentPosition.position);
    
    // Get word at cursor
    const wordRange = getWordRangeAtPosition(text, offset);
    const word = text.substring(wordRange.start, wordRange.end);
    
    // Check if it's a function
    const func = apiDatabase.getFunction(word);
    if (func) {
      return {
        contents: {
          kind: MarkupKind.Markdown,
          value: [
            '```ltsl',
            func.signature,
            '```',
            '---',
            func.documentation
          ].join('\n')
        }
      };
    }
    
    // Check if it's a type
    const type = apiDatabase.getType(word);
    if (type) {
      return {
        contents: {
          kind: MarkupKind.Markdown,
          value: [
            '```ltsl',
            `type ${type.name}`,
            '```',
            '---',
            type.documentation
          ].join('\n')
        }
      };
    }
    
    return null;
  }
);

// Definition provider
connection.onDefinition(
  (textDocumentPosition: TextDocumentPositionParams): Definition | null => {
    // For now, return null (would need full workspace analysis)
    return null;
  }
);

// Helper function
function getWordRangeAtPosition(text: string, offset: number): { start: number; end: number } {
  let start = offset;
  let end = offset;
  
  // Move backwards to start of word
  while (start > 0 && /[a-zA-Z0-9_]/.test(text[start - 1])) {
    start--;
  }
  
  // Move forwards to end of word
  while (end < text.length && /[a-zA-Z0-9_]/.test(text[end])) {
    end++;
  }
  
  return { start, end };
}

// Make the text document manager listen on the connection
documents.listen(connection);

// Listen on the connection
connection.listen();
```

### 3.3 API Database

**Generate API database from engine source:**

```typescript
// ltsl-lsp/src/api-database.ts
import * as fs from 'fs';

export interface APIFunction {
  name: string;
  signature: string;
  returnType: string;
  parameters: { name: string; type: string }[];
  documentation: string;
  source: string; // C++ file where defined
}

export interface APIType {
  name: string;
  description: string;
  fields: { name: string; type: string }[];
  methods: APIFunction[];
  documentation: string;
}

export class APIDatabase {
  private functions: Map<string, APIFunction> = new Map();
  private types: Map<string, APIType> = new Map();
  
  loadFromFile(path: string): void {
    const data = JSON.parse(fs.readFileSync(path, 'utf-8'));
    
    // Load functions
    for (const func of data.functions) {
      this.functions.set(func.name, func);
    }
    
    // Load types
    for (const type of data.types) {
      this.types.set(type.name, type);
    }
  }
  
  getFunction(name: string): APIFunction | undefined {
    return this.functions.get(name);
  }
  
  getType(name: string): APIType | undefined {
    return this.types.get(name);
  }
  
  getAllFunctions(): APIFunction[] {
    return Array.from(this.functions.values());
  }
  
  getAllTypes(): APIType[] {
    return Array.from(this.types.values());
  }
}
```

**Example `api-database.json`:**
```json
{
  "functions": [
    {
      "name": "Object_Ship",
      "signature": "Object Object_Ship(Item_ShipType shipType)",
      "returnType": "Object",
      "parameters": [
        { "name": "shipType", "type": "Item_ShipType" }
      ],
      "documentation": "Creates a new ship object from a ship type blueprint. The ship will have all standard components (Drawable, Collidable, Motion, etc.) and sockets filled with default equipment.",
      "source": "src/liblt/Game/ScriptAPI/Object.cpp"
    },
    {
      "name": "Item_ShipType",
      "signature": "Item_ShipType Item_ShipType(Float value, Int seed, Float capacity, Float compactness, Float integrity, Float propulsion, Float systems, Float turrets)",
      "returnType": "Item_ShipType",
      "parameters": [
        { "name": "value", "type": "Float" },
        { "name": "seed", "type": "Int" },
        { "name": "capacity", "type": "Float" },
        { "name": "compactness", "type": "Float" },
        { "name": "integrity", "type": "Float" },
        { "name": "propulsion", "type": "Float" },
        { "name": "systems", "type": "Float" },
        { "name": "turrets", "type": "Float" }
      ],
      "documentation": "Creates a procedural ship type blueprint. Value determines size/tier (10000=fighter, 100000=corvette, etc.). Seed controls mesh generation. Tuning parameters (capacity, integrity, etc.) default to 1.0.",
      "source": "src/liblt/Game/Item/ShipType.cpp"
    },
    {
      "name": "Vec3",
      "signature": "Vec3 Vec3(Float x, Float y, Float z)",
      "returnType": "Vec3",
      "parameters": [
        { "name": "x", "type": "Float" },
        { "name": "y", "type": "Float" },
        { "name": "z", "type": "Float" }
      ],
      "documentation": "Creates a 3D vector. Used for positions, directions, colors, etc.",
      "source": "src/liblt/LTE/V3.cpp"
    }
  ],
  "types": [
    {
      "name": "Object",
      "description": "Game object with components (Drawable, Collidable, Motion, etc.)",
      "fields": [],
      "methods": [
        {
          "name": "SetPos",
          "signature": "Void SetPos(Vec3 position)",
          "returnType": "Void",
          "parameters": [{ "name": "position", "type": "Vec3" }],
          "documentation": "Sets the object's position in 3D space.",
          "source": "Component/Orientation.cpp"
        },
        {
          "name": "GetPos",
          "signature": "Vec3 GetPos()",
          "returnType": "Vec3",
          "parameters": [],
          "documentation": "Returns the object's current position.",
          "source": "Component/Orientation.cpp"
        }
      ],
      "documentation": "Base game object type. All entities (ships, planets, asteroids, stations) are Objects with various components attached."
    },
    {
      "name": "Player",
      "description": "Player controller (human or AI)",
      "fields": [],
      "methods": [
        {
          "name": "Pilot",
          "signature": "Void Pilot(Object ship)",
          "returnType": "Void",
          "parameters": [{ "name": "ship", "type": "Object" }],
          "documentation": "Take control of a ship.",
          "source": "Game/Player.cpp"
        }
      ],
      "documentation": "Player controller. Can be human (Player_Human) or AI (Player_AI)."
    }
  ]
}
```

---

## 4. ZED Extension (shipped)

The editor integration is a **ZED extension** (not VS Code). It ships as
`extensions/ltsl/` in the repo and provides:

- **Tree-sitter grammar** (`script/tree-sitter-ltsl/`) — line-oriented LTSL
  parsing. Handles the prefix/indentation syntax: `function`/`type`/
  `var`/`ref`/`static` declarations, control statements, `(paren)` groups,
  `word.path:parts` identifiers, `#` comments, strings, numbers, operators.
  The grammar is deliberately *line-flat* (no indentation-block structure);
  paren groups are the only nesting.
- **Queries** (`extensions/ltsl/languages/ltsl/*.scm`): `highlights.scm`
  (keywords, types, functions, strings, numbers, comments, operators,
  constants), `brackets.scm` (paren matching), `outline.scm` (functions +
  types).
- **LSP adapter** (`extensions/ltsl/src/lib.rs`): Rust → wasm extension that
  launches `node script/ltsl-lsp/out/server.js --stdio` with
  `LTSL_API_DATABASE` pointed at `script/ltsl-lsp/api-database.json`.

### 4.1 Installing the dev extension

1. Install prerequisites if needed:
   ```bash
   rustup target add wasm32-wasip2
   npm install          # in script/ltsl-lsp/
   npm run compile      # in script/ltsl-lsp/ (builds out/server.js)
   ```
2. In Zed, open the Extensions panel (`zed: extensions`), click
   **Install Dev Extension**, and select `extensions/ltsl/`.
3. Zed compiles the grammar to wasm automatically (it downloads `wasi-sdk` on
   first use). The extension then serves `.lts` files: highlighting, bracket
   matching, outline, and LSP features (diagnostics, completion, hover,
   signature help).
4. Debug output: launch Zed with `zed --foreground`; the LSP logs
   `window/logMessage` "loaded N functions, M types" on initialize.

### 4.2 Regenerating the API database

The LSP is only as good as `script/ltsl-lsp/api-database.json`. Rebuild it
from the engine when the C++ API changes:

```bash
cmake --build ./build --target ltsl_api_dump -j
LD_LIBRARY_PATH=bin:extbin/linux64 ./bin/ltsl_api_dump > script/ltsl-lsp/api-database.json
```

### 4.3 VS Code (legacy, superseded)

The material below documents a hypothetical VS Code extension and TextMate
grammar. It is retained for reference only — the shipped integration is the
ZED extension above, and the tree-sitter grammar replaces the TextMate
approach.

### 4.4 Extension Setup (legacy outline)

```bash
# Create extension project
mkdir ltsl-vscode
```

```bash
# Create extension project
mkdir ltsl-vscode
cd ltsl-vscode
npm init -y

# Install dependencies
npm install --save-dev @types/vscode @types/node typescript
npm install vscode-languageclient
```

**`package.json`:**
```json
{
  "name": "ltsl",
  "displayName": "LTSL Language Support",
  "description": "Language support for Limit Theory Scripting Language",
  "version": "1.0.0",
  "publisher": "limit-theory",
  "engines": {
    "vscode": "^1.75.0"
  },
  "categories": ["Programming Languages"],
  "activationEvents": ["onLanguage:ltsl"],
  "main": "./out/extension.js",
  "contributes": {
    "languages": [{
      "id": "ltsl",
      "aliases": ["LTSL", "ltsl"],
      "extensions": [".lts"],
      "configuration": "./language-configuration.json"
    }],
    "grammars": [{
      "language": "ltsl",
      "scopeName": "source.ltsl",
      "path": "./syntaxes/ltsl.tmLanguage.json"
    }],
    "snippets": [{
      "language": "ltsl",
      "path": "./snippets/ltsl.json"
    }]
  },
  "scripts": {
    "vscode:prepublish": "npm run compile",
    "compile": "tsc -p ./",
    "watch": "tsc -watch -p ./"
  },
  "devDependencies": {
    "@types/vscode": "^1.75.0",
    "@types/node": "^20.0.0",
    "typescript": "^5.0.0"
  },
  "dependencies": {
    "vscode-languageclient": "^8.0.0"
  }
}
```

**`language-configuration.json`:**
```json
{
  "comments": {
    "lineComment": "#"
  },
  "brackets": [
    ["{", "}"],
    ["[", "]"],
    ["(", ")"]
  ],
  "autoClosingPairs": [
    { "open": "{", "close": "}" },
    { "open": "[", "close": "]" },
    { "open": "(", "close": ")" },
    { "open": "\"", "close": "\"", "notIn": ["string"] }
  ],
  "surroundingPairs": [
    ["{", "}"],
    ["[", "]"],
    ["(", ")"],
    ["\"", "\""]
  ],
  "folding": {
    "markers": {
      "start": "^\\s*#\\s*region\\b",
      "end": "^\\s*#\\s*endregion\\b"
    }
  }
}
```

### 4.2 Extension Client

```typescript
// ltsl-vscode/src/extension.ts
import * as path from 'path';
import { workspace, ExtensionContext } from 'vscode';

import {
  LanguageClient,
  LanguageClientOptions,
  ServerOptions,
  TransportKind
} from 'vscode-languageclient/node';

let client: LanguageClient;

export function activate(context: ExtensionContext) {
  // Server module path
  const serverModule = context.asAbsolutePath(
    path.join('..', 'ltsl-lsp', 'out', 'server.js')
  );
  
  // Debug options for the server
  const debugOptions = { execArgv: ['--nolazy', '--inspect=6009'] };
  
  // Server options
  const serverOptions: ServerOptions = {
    run: { module: serverModule, transport: TransportKind.ipc },
    debug: {
      module: serverModule,
      transport: TransportKind.ipc,
      options: debugOptions
    }
  };
  
  // Client options
  const clientOptions: LanguageClientOptions = {
    documentSelector: [{ scheme: 'file', language: 'ltsl' }],
    synchronize: {
      fileEvents: workspace.createFileSystemWatcher('**/.lts')
    }
  };
  
  // Create language client
  client = new LanguageClient(
    'ltslLanguageServer',
    'LTSL Language Server',
    serverOptions,
    clientOptions
  );
  
  // Start the client (also starts the server)
  client.start();
}

export function deactivate(): Thenable<void> | undefined {
  if (!client) {
    return undefined;
  }
  return client.stop();
}
```

### 4.3 Syntax Highlighting (TextMate Grammar)

**`syntaxes/ltsl.tmLanguage.json`:**
```json
{
  "$schema": "https://raw.githubusercontent.com/martinring/tmlanguage/master/tmlanguage.json",
  "name": "LTSL",
  "patterns": [
    { "include": "#comments" },
    { "include": "#keywords" },
    { "include": "#strings" },
    { "include": "#numbers" },
    { "include": "#functions" },
    { "include": "#types" },
    { "include": "#operators" }
  ],
  "repository": {
    "comments": {
      "patterns": [{
        "name": "comment.line.number-sign.ltsl",
        "match": "#.*$"
      }]
    },
    "keywords": {
      "patterns": [{
        "name": "keyword.control.ltsl",
        "match": "\\b(function|type|var|if|switch|case|for|while|return|static|ref|cast|otherwise)\\b"
      }]
    },
    "strings": {
      "name": "string.quoted.double.ltsl",
      "begin": "\"",
      "end": "\"",
      "patterns": [{
        "name": "constant.character.escape.ltsl",
        "match": "\\\\."
      }]
    },
    "numbers": {
      "patterns": [{
        "name": "constant.numeric.ltsl",
        "match": "\\b\\d+(\\.\\d+)?([eE][+-]?\\d+)?\\b"
      }]
    },
    "functions": {
      "patterns": [{
        "name": "entity.name.function.ltsl",
        "match": "\\b([A-Z][a-zA-Z0-9_]*(?:::[A-Z][a-zA-Z0-9_]*)*)(?=\\s*\\()"
      }]
    },
    "types": {
      "patterns": [{
        "name": "entity.name.type.ltsl",
        "match": "\\b(Object|Player|Camera|Interface|Vec3|Vec2|Float|Int|String|Bool|List|Array|Map|Vector|Item_ShipType|Item_WeaponType)\\b"
      }]
    },
    "operators": {
      "patterns": [{
        "name": "keyword.operator.ltsl",
        "match": "(\\+|\\-|\\*|\\/|==|!=|<|>|<=|>=|&&|\\|\\||!|\\.)"
      }]
    }
  },
  "scopeName": "source.ltsl"
}
```

### 4.4 Code Snippets

**`snippets/ltsl.json`:**
```json
{
  "Function Declaration": {
    "prefix": "func",
    "body": [
      "function ${1:Void} ${2:FunctionName} (${3:Type param}) {",
      "\t$0",
      "}"
    ],
    "description": "Function declaration"
  },
  "Type Declaration": {
    "prefix": "type",
    "body": [
      "type ${1:TypeName}",
      "\t${2:Type field} ${3:defaultValue}",
      "\t",
      "\tfunction Void ${4:MethodName} (${5:Type self}) {",
      "\t\t$0",
      "\t}"
    ],
    "description": "Type declaration"
  },
  "Driven App": {
    "prefix": "app",
    "body": [
      "type App",
      "\tInterface ui",
      "\tInterface gameView",
      "\tCamera camera",
      "\t",
      "\tfunction Void Initialize () {",
      "\t\tcamera = Camera_Create",
      "\t\tcamera.Push",
      "\t\tui = (Interface_Create \"UI\")",
      "\t\tgameView = (Interface_Create \"Game View\")",
      "\t\t$0",
      "\t}",
      "\t",
      "\tfunction Void Update () {",
      "\t\tvar dt FrameTimer_Get",
      "\t\tui.Update",
      "\t\tgameView.Update",
      "\t\tgameView.Draw",
      "\t}",
      "",
      "function App Main () {",
      "\tvar self App",
      "\tself",
      "}"
    ],
    "description": "Driven app template"
  },
  "Create Ship": {
    "prefix": "ship",
    "body": [
      "var shipType (Item_ShipType ${1:10000} ${2:seed})",
      "var ship shipType.Instantiate",
      "ship.SetPos ${3:(Vec3 0)}"
    ],
    "description": "Create ship"
  },
  "For Loop": {
    "prefix": "for",
    "body": [
      "for ${1:i} 0 ${1:i} < ${2:count} ${1:i}.++ {",
      "\t$0",
      "}"
    ],
    "description": "For loop"
  }
}
```

---

## 5. Advanced Features

### 5.1 Error Recovery & "Did You Mean?" Suggestions

```typescript
// Add to analyzer.ts
private error(message: string, line: number, column: number): void {
  // Check for similar symbols (Levenshtein distance)
  const suggestions = this.findSimilarSymbols(message);
  
  if (suggestions.length > 0) {
    message += `\n\nDid you mean: ${suggestions.join(', ')}?`;
  }
  
  this.diagnostics.push({
    severity: DiagnosticSeverity.Error,
    range: {
      start: { line: line - 1, character: column - 1 },
      end: { line: line - 1, character: column + 10 }
    },
    message: message,
    source: 'ltsl'
  });
}

private findSimilarSymbols(search: string): string[] {
  const candidates: { name: string; distance: number }[] = [];
  
  // Search local symbols
  for (const scope of this.scopes) {
    for (const [name, _] of scope.symbols) {
      const distance = this.levenshteinDistance(search, name);
      if (distance <= 3) {
        candidates.push({ name, distance });
      }
    }
  }
  
  // Search API functions
  for (const func of this.apiDatabase.getAllFunctions()) {
    const distance = this.levenshteinDistance(search, func.name);
    if (distance <= 3) {
      candidates.push({ name: func.name, distance });
    }
  }
  
  // Sort by distance and return top 3
  return candidates
    .sort((a, b) => a.distance - b.distance)
    .slice(0, 3)
    .map(c => c.name);
}

private levenshteinDistance(a: string, b: string): number {
  const matrix = [];
  
  for (let i = 0; i <= b.length; i++) {
    matrix[i] = [i];
  }
  
  for (let j = 0; j <= a.length; j++) {
    matrix[0][j] = j;
  }
  
  for (let i = 1; i <= b.length; i++) {
    for (let j = 1; j <= a.length; j++) {
      if (b.charAt(i - 1) === a.charAt(j - 1)) {
        matrix[i][j] = matrix[i - 1][j - 1];
      } else {
        matrix[i][j] = Math.min(
          matrix[i - 1][j - 1] + 1, // substitution
          matrix[i][j - 1] + 1,     // insertion
          matrix[i - 1][j] + 1      // deletion
        );
      }
    }
  }
  
  return matrix[b.length][a.length];
}
```

### 5.2 Signature Help (Parameter Hints)

```typescript
// Add to server.ts
connection.onSignatureHelp(
  (textDocumentPosition: TextDocumentPositionParams): SignatureHelp | null => {
    const document = documents.get(textDocumentPosition.textDocument.uri);
    if (!document) {
      return null;
    }
    
    const text = document.getText();
    const offset = document.offsetAt(textDocumentPosition.position);
    
    // Find function call we're inside
    const callInfo = findFunctionCallAtPosition(text, offset);
    if (!callInfo) {
      return null;
    }
    
    // Get function signature
    const func = apiDatabase.getFunction(callInfo.functionName);
    if (!func) {
      return null;
    }
    
    // Build parameter info
    const parameters = func.parameters.map(p => ({
      label: `${p.type} ${p.name}`,
      documentation: ''
    }));
    
    return {
      signatures: [{
        label: func.signature,
        documentation: func.documentation,
        parameters: parameters
      }],
      activeSignature: 0,
      activeParameter: callInfo.parameterIndex
    };
  }
);
```

### 5.3 Code Actions (Quick Fixes)

> **Not yet implemented in the shipped server.** The snippet below is the
> proposed design, kept for when quick-fix support is added.

```typescript
// Add to server.ts
connection.onCodeAction(
  (params): CodeAction[] => {
    const document = documents.get(params.textDocument.uri);
    if (!document) {
      return [];
    }
    
    const actions: CodeAction[] = [];
    
    // Check diagnostics in range
    for (const diagnostic of params.context.diagnostics) {
      if (diagnostic.message.includes('Undefined function')) {
        // Extract function name
        const match = diagnostic.message.match(/'([^']+)'/);
        if (match) {
          const funcName = match[1];
          
          // Suggest creating function
          actions.push({
            title: `Create function '${funcName}'`,
            kind: CodeActionKind.QuickFix,
            edit: {
              changes: {
                [params.textDocument.uri]: [{
                  range: diagnostic.range,
                  newText: `function Void ${funcName} () {\n\t\n}\n\n`
                }]
              }
            }
          });
        }
      }
    }
    
    return actions;
  }
);
```

---

## 6. Deployment & Distribution

### 6.1 Build & Package

```bash
# Build LSP server
cd ltsl-lsp
npm install
npm run compile

# Build ZED extension (wasm)
cd ../extensions/ltsl
rustup target add wasm32-wasip2
cargo build --release --target wasm32-wasip2
```

### 6.2 Installation

**Local dev install (recommended):** In Zed, open the Extensions panel
(`zed: extensions`) → **Install Dev Extension** → select `extensions/ltsl/`.

**Publishing:** open a PR against `zed-industries/extensions` adding the
extension as a git submodule. Note the publishing rules: the extension ID must
not contain `zed`, the LSP must not be shipped inside the extension (the
adapter already launches the repo-local `out/server.js` instead), and the
grammar must be referenced by `file://` URL + `rev` while developing locally
and by an HTTPS repo URL when published.

### 6.3 Testing

```bash
# LSP smoke test over the whole script corpus
cd ltsl-lsp
npm run smoke -- $(find ../../resource/script -name '*.lts' | sort)

# End-to-end JSON-RPC check (initialize/hover/completion/signatureHelp/diagnostics)
node test-rpc.js

# Grammar check: parse the corpus, expect ERROR nodes only on the four known
# unbalanced-paren fixtures (App/draw.lts:57,58; Widget/Slider.lts:42; Widget/Text.lts:26)
cd ../tree-sitter-ltsl
tree-sitter parse --lib-path <built.so> --lang-name ltsl <files...>
```

In Zed, open a `.lts` file and test:
- Syntax highlighting, bracket matching, code outline
- Autocomplete, hover tooltips, signature help, error diagnostics

---

## 7. Future Enhancements

### 7.1 Advanced Type Inference

- **Flow-sensitive typing** (track type narrowing through if/switch)
- **Generic type inference** (List<T>, Array<T>, Map<K,V>)
- **Duck typing support** (structural type checking)

### 7.2 Refactoring Support

- **Rename symbol** (across all files)
- **Extract function** (selected code → new function)
- **Inline function** (replace call with body)
- **Convert var ↔ ref** (automatic conversion)

### 7.3 Debugging Integration

- **Breakpoints** (integrate with C++ debugger)
- **Variable inspection** (watch expressions)
- **Call stack** (LTSL call frames)
- **Step through** (line-by-line execution)

### 7.4 Documentation Generation

- **Auto-generate API docs** from C++ ScriptAPI bindings
- **LTSL→Markdown** (extract function docs from comments)
- **Interactive examples** (runnable code snippets)

### 7.5 Linting Rules

- **Style enforcement** (naming conventions, indentation)
- **Performance warnings** (inefficient patterns)
- **Best practices** (driven app pattern, widget composition)

---

## Appendix: Quick Reference

### Building from Scratch (Linux, ZED)

```bash
# 1. Clone repo (grammar is a separate git repo — see AGENTS.md §6.2)
git clone https://github.com/darkoned12000/ltheory-old-test.git
cd ltheory-old-test

# 2. Build the LSP server
cd script/ltsl-lsp
npm install
npm run compile

# 3. Install the ZED dev extension
cd ../..     # back to repo root
# Zed -> Extensions (zed: extensions) -> "Install Dev Extension" -> extensions/ltsl/

# 4. Verify
node script/ltsl-lsp/test-rpc.js
node script/ltsl-lsp/out/smoke.js $(find resource/script -name '*.lts' | sort)

# 5. Test in the editor
zed resource/script/App/ltheory-main.lts
# Try autocomplete ('.'), hover, and signature help!
```

### Key Files

| File | Purpose |
|------|---------|
| `ltsl-lsp/src/lexer.ts` | Tokenizer (syntax → tokens) |
| `ltsl-lsp/src/parser.ts` | Parser (tokens → AST) |
| `ltsl-lsp/src/analyzer.ts` | Semantic analysis (type checking, scoping) |
| `ltsl-lsp/src/api-database.ts` | Engine API metadata |
| `ltsl-lsp/src/server.ts` | Main LSP server |
| `ltsl-lsp/src/smoke.ts` | Corpus-wide diagnostics smoke runner |
| `ltsl-lsp/test-rpc.js` | End-to-end JSON-RPC check of the server |
| `ltsl-lsp/api-database.json` | Function/type definitions (engine-generated) |
| `tree-sitter-ltsl/grammar.js` | ZED/tree-sitter LTSL grammar |
| `tree-sitter-ltsl/queries/*.scm` | Highlights / brackets / outline queries |
| `extensions/ltsl/extension.toml` | ZED extension manifest |
| `extensions/ltsl/src/lib.rs` | ZED LSP adapter (launches `out/server.js`) |
| `extensions/ltsl/languages/ltsl/config.toml` | ZED language registration for `.lts` |

---

**Last Updated:** 2026-07-30  
**Contributors:** darkoned12000, AI agents  
**License:** GPL-3.0  
**See Also:** [SKILL.md](../SKILL.md), [AGENTS.md](../AGENTS.md), [ltsl-docs.md](./ltsl-docs.md)
