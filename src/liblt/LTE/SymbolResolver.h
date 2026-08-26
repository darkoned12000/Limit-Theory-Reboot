// Copyright (C) 2025  darkoned12000
// SPDX-License-Identifier: GPL-3.0-or-later
// Part of the ltheory-old-test modernization effort (Revamp Work).
// See NOTICE and LICENSE.GPL. Original engine (c) Josh Parnell, public domain.

// Phase 3: Symbol Resolver + Type Checker for the new LTSL compiler.
// Input: Raw AST from the Parser (Phase 2).
// Output: Validated AST — all identifiers bound to declarations,
//         arities checked, basic types inferred. No AST mutation;
//         errors reported via CompileError vector.
//
// Architecture: a quick pre-scan registers all function/type declarations
// (enabling forward references), then a single combined pass declares and
// resolves variables in one walk. This avoids the scope-mismatch problem
// of a strict two-pass approach.
//
// Scope model:
//   - Nested Scope tree with parent pointers.
//   - var = local mutable, ref = local immutable, static = local const.
//   - Functions and types are file-level (visible throughout file).
//   - for-loop iterators and function parameters are local to their block.

#ifndef LTE_SymbolResolver_h__
#define LTE_SymbolResolver_h__

#include "AST.h"
#include "Map.h"
#include "String.h"
#include "Vector.h"

namespace LTE {

// ============================================================================
// Symbol — a declared name in a scope
// ============================================================================

struct Symbol {
  enum Kind {
    Sym_Var,       // var name expr
    Sym_Ref,       // ref name expr
    Sym_Static,    // static name expr
    Sym_Function,  // function RetType Name (params) body
    Sym_Type,      // type Name members
    Sym_Param,     // function parameter (for arity checking)
    Sym_Iterator   // for-loop iterator variable
  };

  String name;
  String typeName;      // Declared or inferred type name (may be empty)
  Kind kind;
  SourceLocation declLoc;
  int scopeLevel;
  bool isMutable;

  Symbol() :
    kind(Sym_Var),
    scopeLevel(0),
    isMutable(true) {}

  Symbol(String const& name, String const& typeName, Kind kind,
         SourceLocation loc, int scopeLevel, bool isMutable) :
    name(name),
    typeName(typeName),
    kind(kind),
    declLoc(loc),
    scopeLevel(scopeLevel),
    isMutable(isMutable) {}
};

// ============================================================================
// Scope — a lexical scope with parent chain
// ============================================================================

struct Scope : public RefCounted {
  Reference<Scope> parent;
  Map<String, Symbol> symbols;
  int level;

  Scope() : level(0) {}
  explicit Scope(int level) : level(level) {}
};

// ============================================================================
// CompileError — a diagnostic produced during resolution
// ============================================================================

struct CompileError {
  String message;
  int line;
  int column;

  CompileError() : line(0), column(0) {}
  CompileError(String const& message, int line, int column) :
    message(message), line(line), column(column) {}
};

// ============================================================================
// SymbolResolver — two-pass name resolution and type checking
// ============================================================================

class SymbolResolver {
public:
  SymbolResolver();

  // Main entry: resolve all references in an AST module.
  // Returns true if zero errors.
  bool Resolve(ASTNode const& module);

  Vector<CompileError> const& GetErrors() const;

  // --- Query helpers (for Phase 4 evaluator) ---

  // Look up a symbol by name, walking the scope chain from currentScope.
  Symbol* LookupSymbol(String const& name);

  // Collect all symbol names in scope chain (for "did you mean?" suggestions).
  Vector<String> AllSymbolNames();

  // Access the root scope (file-level).
  Scope& GetRootScope();

private:
  Reference<Scope> rootScope;
  Reference<Scope> currentScope;
  Vector<CompileError> errors;

  // File-level registries (functions and types are file-scoped).
  Map<String, ASTFuncDeclNodeT*> functions;
  Map<String, ASTTypeDeclNodeT*> types;

  // --- Pre-scan: register file-level functions/types ---
  void PreScanDeclarations(ASTNode const& node);

  // --- Combined pass: declare + resolve ---
  void ResolveAndDeclare(ASTNode const& node);

  // --- Function call arity checking ---
  void ResolveFuncCall(ASTFuncCallNodeT const* call);

  // --- Scope management ---
  void PushScope();
  void PopScope();

  // --- Declaration ---
  bool DeclareSymbol(String const& name, String const& typeName,
                     Symbol::Kind kind, bool isMutable,
                     SourceLocation loc);

  // --- Type inference (best-effort) ---
  String InferType(ASTNode const& node);
  String InferTypeFromBinaryOp(String const& leftType, String const& op,
                               String const& rightType);
  String InferTypeFromUnaryOp(String const& op, String const& operandType);

  // --- Error reporting ---
  void ReportError(String const& message, int line, int column);

  // --- Helpers ---
  static size_t EditDistance(String const& a, String const& b);
  static String BestMatch(String const& target,
                          Vector<String> const& candidates,
                          size_t maxDistance = 3);
};

} // namespace LTE

#endif
