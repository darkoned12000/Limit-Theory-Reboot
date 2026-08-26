// Copyright (C) 2025  darkoned12000
// SPDX-License-Identifier: GPL-3.0-or-later
// Part of the ltheory-old-test modernization effort (Revamp Work).
// See NOTICE and LICENSE.GPL. Original engine (c) Josh Parnell, public domain.

// Phase 2: AST node definitions for the new LTSL compiler.
// This is the intermediate representation produced by the Parser (Phase 2),
// consumed by the SymbolResolver (Phase 3), and eventually used to generate
// the existing Expression_T nodes (Phase 4).

#ifndef LTE_AST_h__
#define LTE_AST_h__

#include "Reference.h"
#include "String.h"
#include "Vector.h"

namespace LTE {

// Source location for error reporting on every node.
struct SourceLocation {
  int line;
  int column;

  SourceLocation() : line(0), column(0) {}
  SourceLocation(int line, int column) : line(line), column(column) {}
};

// ============================================================================
// AST node kind tags
// ============================================================================

enum ASTNodeKind {
  // --- Statements ---
  AST_VAR_DECL,
  AST_REF_DECL,
  AST_STATIC_DECL,
  AST_FUNC_DECL,
  AST_TYPE_DECL,
  AST_RETURN,
  AST_BREAK,
  AST_IF,
  AST_WHILE,
  AST_FOR,
  AST_SWITCH,
  AST_ASSIGN,
  AST_EXPR_STMT,

  // --- Expressions ---
  AST_INT_LITERAL,
  AST_FLOAT_LITERAL,
  AST_STRING_LITERAL,
  AST_BOOL_LITERAL,
  AST_NULL_LITERAL,
  AST_IDENTIFIER,
  AST_BINARY_OP,
  AST_UNARY_OP,
  AST_METHOD_CALL,
  AST_FUNC_CALL,
  AST_CAST,
  AST_ADDRESS,
  AST_DEREF,
  AST_ARRAY_LITERAL,
  AST_CONSTRUCTOR,
  AST_PRINT,

  // --- Block expressions ---
  AST_BLOCK,
  AST_DESC,

  // --- Special ---
  AST_MODULE,
  AST_NOOP
};

// ============================================================================
// Base AST node
// ============================================================================

struct ASTNodeT : public RefCounted {
  ASTNodeKind kind;
  SourceLocation loc;

  ASTNodeT() : kind(AST_NOOP) {}
  ASTNodeT(ASTNodeKind kind) : kind(kind) {}
  virtual ~ASTNodeT() {}
};

using ASTNode = Reference<ASTNodeT>;

// Helper to downcast. Returns nullptr on mismatch.
template <class T>
T* ASTNodeAs(ASTNode const& node) {
  return dynamic_cast<T*>(node.t);
}

// ============================================================================
// Statement nodes
// ============================================================================

// var name expr  /  ref name expr  /  static name expr
// All three share the same shape: a keyword, a name, and an initializer.
struct ASTDeclNodeT : public ASTNodeT {
  String name;
  ASTNode initializer;   // The expression after the name

  ASTDeclNodeT() : ASTNodeT(AST_VAR_DECL) {}
};

// function RetType Name (Type1 p1, Type2 p2, ...) body
struct ASTFuncDeclNodeT : public ASTNodeT {
  String returnType;     // Return type name (may be empty for void)
  String name;           // Function name
  Vector<String> paramTypes;  // Parameter type names
  Vector<String> paramNames;  // Parameter names
  ASTNode body;          // Block body

  ASTFuncDeclNodeT() : ASTNodeT(AST_FUNC_DECL) {}
};

// type Name
//   Type1 field1
//   Type2 field2
//   function RetType Method (params) body
struct ASTTypeDeclNodeT : public ASTNodeT {
  String name;
  Vector<ASTNode> members;  // ASTDeclNodeT (fields) and ASTFuncDeclNodeT (methods)

  ASTTypeDeclNodeT() : ASTNodeT(AST_TYPE_DECL) {}
};

// return [expr]
struct ASTReturnNodeT : public ASTNodeT {
  ASTNode value;  // May be nullptr for bare 'return'

  ASTReturnNodeT() : ASTNodeT(AST_RETURN) {}
};

// if expr block [else block / if]
struct ASTIfNodeT : public ASTNodeT {
  ASTNode condition;
  ASTNode thenBlock;
  ASTNode elseBlock;  // May be nullptr; can be another ASTIfNodeT for else-if

  ASTIfNodeT() : ASTNodeT(AST_IF) {}
};

// while expr block
struct ASTWhileNodeT : public ASTNodeT {
  ASTNode condition;
  ASTNode body;

  ASTWhileNodeT() : ASTNodeT(AST_WHILE) {}
};

// for name init pred step body
// (The old interpreter form: for name init pred step body...)
// Also supports: for i in range start end body (sugar, desugared later)
struct ASTForNodeT : public ASTNodeT {
  String iteratorName;
  ASTNode init;
  ASTNode condition;
  ASTNode step;
  ASTNode body;

  ASTForNodeT() : ASTNodeT(AST_FOR) {}
};

// switch (cases are inline pairs: condition body, condition body, ...)
// otherwise body
struct ASTSwitchCase {
  ASTNode condition;
  ASTNode body;
};

struct ASTSwitchNodeT : public ASTNodeT {
  Vector<ASTSwitchCase> cases;
  ASTNode otherwise;  // Default branch, may be nullptr

  ASTSwitchNodeT() : ASTNodeT(AST_SWITCH) {}
};

// lvalue op expr  where op is = += -= *= /=
struct ASTAssignNodeT : public ASTNodeT {
  ASTNode target;     // The lvalue (identifier or dot-access)
  String op;          // "=", "+=", "-=", "*=", "/="
  ASTNode value;

  ASTAssignNodeT() : ASTNodeT(AST_ASSIGN) {}
};

// Expression statement — an expression used for its side effects
struct ASTExprStmtNodeT : public ASTNodeT {
  ASTNode expression;

  ASTExprStmtNodeT() : ASTNodeT(AST_EXPR_STMT) {}
};

// ============================================================================
// Expression nodes
// ============================================================================

struct ASTIntLiteralNodeT : public ASTNodeT {
  long long value;
  ASTIntLiteralNodeT() : ASTNodeT(AST_INT_LITERAL), value(0) {}
};

struct ASTFloatLiteralNodeT : public ASTNodeT {
  double value;
  ASTFloatLiteralNodeT() : ASTNodeT(AST_FLOAT_LITERAL), value(0.0) {}
};

struct ASTStringLiteralNodeT : public ASTNodeT {
  String value;
  ASTStringLiteralNodeT() : ASTNodeT(AST_STRING_LITERAL) {}
};

struct ASTBoolLiteralNodeT : public ASTNodeT {
  bool value;
  ASTBoolLiteralNodeT() : ASTNodeT(AST_BOOL_LITERAL), value(false) {}
};

struct ASTNullLiteralNodeT : public ASTNodeT {
  ASTNullLiteralNodeT() : ASTNodeT(AST_NULL_LITERAL) {}
};

struct ASTIdentifierNodeT : public ASTNodeT {
  String name;
  ASTIdentifierNodeT() : ASTNodeT(AST_IDENTIFIER) {}
};

// Binary operator: left op right
struct ASTBinaryOpNodeT : public ASTNodeT {
  ASTNode left;
  String op;          // "+", "-", "*", "/", "%", "==", "!=", "<", ">",
                      // "<=", ">=", "&&", "||"
  ASTNode right;

  ASTBinaryOpNodeT() : ASTNodeT(AST_BINARY_OP) {}
};

// Unary operator: op operand
struct ASTUnaryOpNodeT : public ASTNodeT {
  String op;          // "-" (negate), "!" (not)
  ASTNode operand;

  ASTUnaryOpNodeT() : ASTNodeT(AST_UNARY_OP) {}
};

// Method call: object.methodName(args)
// Also handles chained method calls: a.b.c(args) — the parser builds
// nested ASTMethodCallNodeT where the object is itself a method call.
struct ASTMethodCallNodeT : public ASTNodeT {
  ASTNode object;       // Receiver expression
  String methodName;
  Vector<ASTNode> args;

  ASTMethodCallNodeT() : ASTNodeT(AST_METHOD_CALL) {}
};

// Function call: Name(args)  — standalone, not a method call
struct ASTFuncCallNodeT : public ASTNodeT {
  String name;
  Vector<ASTNode> args;

  ASTFuncCallNodeT() : ASTNodeT(AST_FUNC_CALL) {}
};

// cast TypeName expr
struct ASTCastNodeT : public ASTNodeT {
  String typeName;
  ASTNode operand;

  ASTCastNodeT() : ASTNodeT(AST_CAST) {}
};

// address expr
struct ASTAddressNodeT : public ASTNodeT {
  ASTNode operand;
  ASTAddressNodeT() : ASTNodeT(AST_ADDRESS) {}
};

// deref expr
struct ASTDerefNodeT : public ASTNodeT {
  ASTNode operand;
  ASTDerefNodeT() : ASTNodeT(AST_DEREF) {}
};

// [expr, expr, expr]
struct ASTArrayLiteralNodeT : public ASTNodeT {
  Vector<ASTNode> elements;
  ASTArrayLiteralNodeT() : ASTNodeT(AST_ARRAY_LITERAL) {}
};

// TypeName(args) — type constructor
struct ASTConstructorNodeT : public ASTNodeT {
  String typeName;
  Vector<ASTNode> args;

  ASTConstructorNodeT() : ASTNodeT(AST_CONSTRUCTOR) {}
};

// @expr — debug print prefix operator
struct ASTPrintNodeT : public ASTNodeT {
  ASTNode operand;
  ASTPrintNodeT() : ASTNodeT(AST_PRINT) {}
};

// ============================================================================
// Block expressions
// ============================================================================

// block (body) — single-arg block, skip count 1
// desc "label" (body) — two-arg block, skip count 2
// Both are represented as a list of statements with a flag.
struct ASTBlockNodeT : public ASTNodeT {
  Vector<ASTNode> statements;
  String label;        // Non-empty for 'desc' blocks
  bool isDesc;         // true = desc, false = block

  ASTBlockNodeT() : ASTNodeT(AST_BLOCK), isDesc(false) {}
};

// ============================================================================
// Module (top-level)
// ============================================================================

// A module is simply a list of top-level statements.
struct ASTModuleNodeT : public ASTNodeT {
  Vector<ASTNode> statements;

  ASTModuleNodeT() : ASTNodeT(AST_MODULE) {}
};

// ============================================================================
// Factory functions (cleaner than raw new)
// ============================================================================

inline ASTNode AST_MakeNoop() {
  ASTNode n = new ASTNodeT(AST_NOOP);
  return n;
}

} // namespace LTE

#endif
