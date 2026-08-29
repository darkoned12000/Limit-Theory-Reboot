// Copyright (C) 2025  darkoned12000
// SPDX-License-Identifier: GPL-3.0-or-later
// Part of the ltheory-old-test modernization effort (Revamp Work).
// See NOTICE and LICENSE.GPL. Original engine (c) Josh Parnell, public domain.

// Phase 3: Symbol Resolver + Type Checker implementation.
// Architecture: a quick pre-scan registers all function/type declarations,
// then a single combined pass declares and resolves in one walk.
// This avoids the scope-mismatch problem of a two-pass approach where
// Pass-1 scopes are destroyed before Pass 2 can use them.

#include "SymbolResolver.h"
#include "LTE/Function.h"
#include "LTE/Type.h"
#include "LTE/Script.h"
#include <algorithm>
#include <string>

namespace LTE {

// ============================================================================
// Constructor
// ============================================================================

SymbolResolver::SymbolResolver() {
  rootScope = new Scope(0);
  currentScope = rootScope;
}

// ============================================================================
// Main entry
// ============================================================================

bool SymbolResolver::Resolve(ASTNode const& module) {
  if (!module) return true;
  errors.clear();
  functions.clear();
  types.clear();
  rootScope->symbols.clear();
  currentScope = rootScope;

  // Pre-scan: register all function and type declarations (file-level only).
  // This enables forward references and doesn't need scope management.
  PreScanDeclarations(module);

  // Combined pass: declare + resolve in a single walk.
  ResolveAndDeclare(module);

  return errors.isEmpty();
}

Vector<CompileError> const& SymbolResolver::GetErrors() const {
  return errors;
}

// ============================================================================
// Pre-scan: register file-level functions and types (no scope management)
// ============================================================================

void SymbolResolver::PreScanDeclarations(ASTNode const& node) {
  if (!node) return;
  if (node->kind != AST_MODULE) return;

  auto mod = ASTNodeAs<ASTModuleNodeT>(node);
  for (size_t i = 0; i < mod->statements.size(); ++i) {
    auto stmt = mod->statements[i];
    if (!stmt) continue;
    if (stmt->kind == AST_FUNC_DECL) {
      auto func = ASTNodeAs<ASTFuncDeclNodeT>(stmt);
      functions[func->name] = func;
    } else if (stmt->kind == AST_TYPE_DECL) {
      auto td = ASTNodeAs<ASTTypeDeclNodeT>(stmt);
      types[td->name] = td;
      // Methods declared inside a type body are top-level script functions in
      // LTSL (resolved as `Script:Method`, e.g. Widget/Window:CreateChildren).
      for (size_t m = 0; m < td->members.size(); ++m)
        PreScanNode(td->members[m]);
    } else {
      PreScanNode(stmt);
    }
  }
}

// Recursive pre-scan. Old LTSL hoists nested declarations into script scope:
// a `function` nested inside another function body (no type context) lands in
// script->functions, and type methods are surfaced as `Script:Method`. Walk
// nested constructs so the new pipeline exposes the same declaration set.
void SymbolResolver::PreScanNode(ASTNode const& node) {
  if (!node) return;

  switch (node->kind) {
    case AST_FUNC_DECL: {
      auto func = ASTNodeAs<ASTFuncDeclNodeT>(node);
      functions[func->name] = func;
      PreScanNode(func->body);
      break;
    }
    case AST_TYPE_DECL: {
      auto td = ASTNodeAs<ASTTypeDeclNodeT>(node);
      types[td->name] = td;
      for (size_t m = 0; m < td->members.size(); ++m)
        PreScanNode(td->members[m]);
      break;
    }
    case AST_BLOCK: {
      auto blk = ASTNodeAs<ASTBlockNodeT>(node);
      for (size_t i = 0; i < blk->statements.size(); ++i)
        PreScanNode(blk->statements[i]);
      break;
    }
    case AST_IF: {
      auto ifNode = ASTNodeAs<ASTIfNodeT>(node);
      PreScanNode(ifNode->thenBlock);
      PreScanNode(ifNode->elseBlock);
      break;
    }
    case AST_WHILE: {
      auto w = ASTNodeAs<ASTWhileNodeT>(node);
      PreScanNode(w->body);
      break;
    }
    case AST_FOR: {
      auto f = ASTNodeAs<ASTForNodeT>(node);
      PreScanNode(f->body);
      break;
    }
    default:
      break;
  }
}

// ============================================================================
// Combined pass: declare variables + resolve references in one walk
// ============================================================================

void SymbolResolver::ResolveAndDeclare(ASTNode const& node) {
  if (!node) return;

  switch (node->kind) {
    case AST_MODULE: {
      auto mod = ASTNodeAs<ASTModuleNodeT>(node);
      for (size_t i = 0; i < mod->statements.size(); ++i)
        ResolveAndDeclare(mod->statements[i]);
      break;
    }

    case AST_FUNC_DECL: {
      auto func = ASTNodeAs<ASTFuncDeclNodeT>(node);
      PushScope();
      for (size_t i = 0; i < func->paramNames.size(); ++i)
        DeclareSymbol(func->paramNames[i], func->paramTypes[i],
                      Symbol::Sym_Param, false, func->loc);
      ResolveAndDeclare(func->body);
      PopScope();
      break;
    }

    case AST_TYPE_DECL: {
      // Type members are type-name + field-name pairs (e.g. "Int x"),
      // not expressions — skip resolving them.
      break;
    }

    case AST_VAR_DECL:
    case AST_REF_DECL:
    case AST_STATIC_DECL: {
      auto decl = ASTNodeAs<ASTDeclNodeT>(node);
      bool isMutable = (node->kind == AST_VAR_DECL);
      Symbol::Kind kind = (node->kind == AST_VAR_DECL) ? Symbol::Sym_Var :
                          (node->kind == AST_REF_DECL) ? Symbol::Sym_Ref :
                          Symbol::Sym_Static;
      DeclareSymbol(decl->name, "", kind, isMutable, decl->loc);
      ResolveAndDeclare(decl->initializer);
      break;
    }

    case AST_BLOCK: {
      auto block = ASTNodeAs<ASTBlockNodeT>(node);
      PushScope();
      for (size_t i = 0; i < block->statements.size(); ++i)
        ResolveAndDeclare(block->statements[i]);
      PopScope();
      break;
    }

    case AST_IF: {
      auto ifNode = ASTNodeAs<ASTIfNodeT>(node);
      ResolveAndDeclare(ifNode->condition);
      ResolveAndDeclare(ifNode->thenBlock);
      ResolveAndDeclare(ifNode->elseBlock);
      break;
    }

    case AST_WHILE: {
      auto whileNode = ASTNodeAs<ASTWhileNodeT>(node);
      ResolveAndDeclare(whileNode->condition);
      PushScope();
      ResolveAndDeclare(whileNode->body);
      PopScope();
      break;
    }

    case AST_FOR: {
      auto forNode = ASTNodeAs<ASTForNodeT>(node);
      PushScope();
      DeclareSymbol(forNode->iteratorName, "", Symbol::Sym_Iterator, true,
                    forNode->loc);
      ResolveAndDeclare(forNode->init);
      ResolveAndDeclare(forNode->condition);
      ResolveAndDeclare(forNode->step);
      ResolveAndDeclare(forNode->body);
      PopScope();
      break;
    }

    case AST_SWITCH: {
      auto sw = ASTNodeAs<ASTSwitchNodeT>(node);
      ResolveAndDeclare(sw->expression);
      for (size_t i = 0; i < sw->cases.size(); ++i) {
        ResolveAndDeclare(sw->cases[i].condition);
        ResolveAndDeclare(sw->cases[i].body);
      }
      ResolveAndDeclare(sw->otherwise);
      break;
    }

    case AST_ASSIGN: {
      auto assign = ASTNodeAs<ASTAssignNodeT>(node);
      ResolveAndDeclare(assign->target);
      ResolveAndDeclare(assign->value);
      break;
    }

    case AST_RETURN: {
      auto ret = ASTNodeAs<ASTReturnNodeT>(node);
      ResolveAndDeclare(ret->value);
      break;
    }

    case AST_EXPR_STMT: {
      auto es = ASTNodeAs<ASTExprStmtNodeT>(node);
      ResolveAndDeclare(es->expression);
      break;
    }

    case AST_FUNC_CALL: {
      auto fc = ASTNodeAs<ASTFuncCallNodeT>(node);
      for (size_t i = 0; i < fc->args.size(); ++i)
        ResolveAndDeclare(fc->args[i]);
      ResolveFuncCall(fc);
      break;
    }

    case AST_METHOD_CALL: {
      auto mc = ASTNodeAs<ASTMethodCallNodeT>(node);
      ResolveAndDeclare(mc->object);
      for (size_t i = 0; i < mc->args.size(); ++i)
        ResolveAndDeclare(mc->args[i]);
      break;
    }

    case AST_BINARY_OP: {
      auto op = ASTNodeAs<ASTBinaryOpNodeT>(node);
      ResolveAndDeclare(op->left);
      ResolveAndDeclare(op->right);
      break;
    }

    case AST_UNARY_OP: {
      auto op = ASTNodeAs<ASTUnaryOpNodeT>(node);
      ResolveAndDeclare(op->operand);
      break;
    }

    case AST_CAST: {
      auto cast = ASTNodeAs<ASTCastNodeT>(node);
      ResolveAndDeclare(cast->operand);
      break;
    }

    case AST_CONSTRUCTOR: {
      auto ctor = ASTNodeAs<ASTConstructorNodeT>(node);
      for (size_t i = 0; i < ctor->args.size(); ++i)
        ResolveAndDeclare(ctor->args[i]);
      break;
    }

    case AST_ARRAY_LITERAL: {
      auto al = ASTNodeAs<ASTArrayLiteralNodeT>(node);
      for (size_t i = 0; i < al->elements.size(); ++i)
        ResolveAndDeclare(al->elements[i]);
      break;
    }

    case AST_PRINT: {
      auto pr = ASTNodeAs<ASTPrintNodeT>(node);
      ResolveAndDeclare(pr->operand);
      break;
    }

    case AST_ADDRESS: {
      auto addr = ASTNodeAs<ASTAddressNodeT>(node);
      ResolveAndDeclare(addr->operand);
      break;
    }

    case AST_DEREF: {
      auto deref = ASTNodeAs<ASTDerefNodeT>(node);
      ResolveAndDeclare(deref->operand);
      break;
    }

    case AST_IDENTIFIER: {
      auto ident = ASTNodeAs<ASTIdentifierNodeT>(node);
      Symbol* sym = LookupSymbol(ident->name);
      if (sym) break;
      // An identifier may also name an engine function (used as a value,
      // e.g. a callback), a type (e.g. a constructor name), or a cross-file
      // script function. LTSL resolves these dynamically at runtime, so an
      // unrecognized name is not a compile error — match the old
      // interpreter's runtime-dynamic behavior.
      break;
    }

    case AST_INT_LITERAL:
    case AST_FLOAT_LITERAL:
    case AST_STRING_LITERAL:
    case AST_BOOL_LITERAL:
    case AST_NULL_LITERAL:
    case AST_NOOP:
      break;

    default:
      break;
  }
}

// ============================================================================
// Function call arity checking
// ============================================================================

void SymbolResolver::ResolveFuncCall(ASTFuncCallNodeT const* call) {
  ASTFuncDeclNodeT** funcPtr = functions.get(call->name);
  if (funcPtr) {
    // Script-defined function — enforce arity strictly.
    ASTFuncDeclNodeT* func = *funcPtr;
    size_t expected = func->paramNames.size();
    size_t got = call->args.size();
    if (got != expected) {
      String msg = "function '" + call->name + "' expects " +
                   String(std::to_string(expected)) + " argument" +
                   (expected != 1 ? "s" : "") + " but got " +
                   String(std::to_string(got));
      ReportError(msg, call->loc.line, call->loc.column);
    }
    return;
  }

  // Not a script function: check the engine's native registry. Engine
  // functions are resolved dynamically by arity at runtime, so we only
  // verify that the name is known (constructors like Vec3 are bound as
  // functions too).
  if (Function_Exists(call->name))
    return;

  // Cross-file script function (e.g. "Config:Get" defined in Config.lts).
  // The engine resolves these lazily by loading the defining script, so
  // probe it the same way.
  if (ScriptFunction_Load(call->name))
    return;

  // LTSL is dynamically typed: names are resolved at runtime (the engine
  // lazily loads the defining script and falls back to Function_Find). An
  // unknown name here is not a compile error — it matches the old
  // interpreter's runtime-dynamic behavior. Defer to runtime.
  return;
}

// ============================================================================
// Scope management
// ============================================================================

void SymbolResolver::PushScope() {
  Reference<Scope> newScope = new Scope(currentScope->level + 1);
  newScope->parent = currentScope;
  currentScope = newScope;
}

void SymbolResolver::PopScope() {
  Reference<Scope> parent = currentScope->parent;
  currentScope = parent;
}

// ============================================================================
// Declaration
// ============================================================================

bool SymbolResolver::DeclareSymbol(String const& name, String const& typeName,
                                   Symbol::Kind kind, bool isMutable,
                                   SourceLocation loc) {
  if (currentScope->symbols.contains(name)) {
    ReportError("duplicate declaration of '" + name + "'",
                loc.line, loc.column);
    return false;
  }
  currentScope->symbols[name] = Symbol(name, typeName, kind, loc,
                                        currentScope->level, isMutable);
  return true;
}

// ============================================================================
// Lookup
// ============================================================================

Symbol* SymbolResolver::LookupSymbol(String const& name) {
  Reference<Scope> scope = currentScope;
  while (scope) {
    Symbol* sym = scope->symbols.get(name);
    if (sym) return sym;
    Reference<Scope> parent = scope->parent;
    scope = parent;
  }
  return nullptr;
}

Vector<String> SymbolResolver::AllSymbolNames() {
  Vector<String> names;
  Reference<Scope> scope = currentScope;
  while (scope) {
    for (Map<String, Symbol>::const_iterator it = scope->symbols.begin();
         it != scope->symbols.end(); ++it) {
      if (!names.contains(it->first))
        names.push(it->first);
    }
    Reference<Scope> parent = scope->parent;
    scope = parent;
  }
  // Also include file-level functions
  for (Map<String, ASTFuncDeclNodeT*>::const_iterator it = functions.begin();
       it != functions.end(); ++it) {
    if (!names.contains(it->first))
      names.push(it->first);
  }
  return names;
}

Scope& SymbolResolver::GetRootScope() {
  return *rootScope;
}

// ============================================================================
// Type inference (best-effort)
// ============================================================================

String SymbolResolver::InferType(ASTNode const& node) {
  if (!node) return "";

  switch (node->kind) {
    case AST_INT_LITERAL:    return "Int";
    case AST_FLOAT_LITERAL:  return "Float";
    case AST_STRING_LITERAL: return "String";
    case AST_BOOL_LITERAL:   return "Bool";
    case AST_NULL_LITERAL:   return "Null";

    case AST_IDENTIFIER: {
      auto ident = ASTNodeAs<ASTIdentifierNodeT>(node);
      Symbol* sym = LookupSymbol(ident->name);
      if (sym) return sym->typeName;
      return "";
    }

    case AST_BINARY_OP: {
      auto op = ASTNodeAs<ASTBinaryOpNodeT>(node);
      String leftType = InferType(op->left);
      String rightType = InferType(op->right);
      return InferTypeFromBinaryOp(leftType, op->op, rightType);
    }

    case AST_UNARY_OP: {
      auto op = ASTNodeAs<ASTUnaryOpNodeT>(node);
      String operandType = InferType(op->operand);
      return InferTypeFromUnaryOp(op->op, operandType);
    }

    case AST_FUNC_CALL: {
      auto call = ASTNodeAs<ASTFuncCallNodeT>(node);
      ASTFuncDeclNodeT** funcPtr = functions.get(call->name);
      if (funcPtr) return (*funcPtr)->returnType;
      return "";
    }

    case AST_CONSTRUCTOR: {
      auto ctor = ASTNodeAs<ASTConstructorNodeT>(node);
      return ctor->typeName;
    }

    case AST_CAST: {
      auto cast = ASTNodeAs<ASTCastNodeT>(node);
      return cast->typeName;
    }

    case AST_METHOD_CALL: {
      // TODO: Full method return-type lookup when we have type info
      return "";
    }

    default:
      return "";
  }
}

String SymbolResolver::InferTypeFromBinaryOp(String const& leftType,
                                              String const& op,
                                              String const& rightType) {
  // Comparison and logical operators always return Bool
  if (op == "==" || op == "!=" || op == "<" || op == ">" ||
      op == "<=" || op == ">=" || op == "&&" || op == "||")
    return "Bool";

  // Arithmetic: return left operand type (assuming matching types)
  if (op == "+" || op == "-" || op == "*" || op == "/" || op == "%")
    return leftType;

  return "";
}

String SymbolResolver::InferTypeFromUnaryOp(String const& op,
                                             String const& operandType) {
  if (op == "!") return "Bool";
  if (op == "-") return operandType;
  return "";
}

// ============================================================================
// Error reporting
// ============================================================================

void SymbolResolver::ReportError(String const& message, int line, int column) {
  errors.push(CompileError(message, line, column));
}

// ============================================================================
// Levenshtein edit distance — "did you mean?" suggestions
// ============================================================================

size_t SymbolResolver::EditDistance(String const& a, String const& b) {
  size_t m = a.size();
  size_t n = b.size();
  std::vector<size_t> prev(n + 1, 0);
  std::vector<size_t> curr(n + 1, 0);

  for (size_t j = 0; j <= n; ++j)
    prev[j] = j;

  for (size_t i = 1; i <= m; ++i) {
    curr[0] = i;
    for (size_t j = 1; j <= n; ++j) {
      size_t cost = (a[i - 1] == b[j - 1]) ? 0 : 1;
      curr[j] = std::min({
        prev[j] + 1,
        curr[j - 1] + 1,
        prev[j - 1] + cost
      });
    }
    std::swap(prev, curr);
  }
  return prev[n];
}

String SymbolResolver::BestMatch(String const& target,
                                  Vector<String> const& candidates,
                                  size_t maxDistance) {
  String best;
  size_t bestDist = maxDistance + 1;
  for (size_t i = 0; i < candidates.size(); ++i) {
    size_t d = EditDistance(target, candidates[i]);
    if (d < bestDist) {
      bestDist = d;
      best = candidates[i];
    }
  }
  return best;
}

} // namespace LTE
