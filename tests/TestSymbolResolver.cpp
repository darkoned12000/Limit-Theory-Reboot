// Copyright (C) 2025  darkoned12000
// SPDX-License-Identifier: GPL-3.0-or-later
// Part of the ltheory-old-test modernization effort (Revamp Work).
// See NOTICE and LICENSE.GPL. Original engine (c) Josh Parnell, public domain.
//
// Unit tests for the new LTSL Symbol Resolver (Phase 3 of the compiler rewrite).

#include "Harness.h"
#include "LTE/AST.h"
#include "LTE/Parser.h"
#include "LTE/SymbolResolver.h"

using namespace LTE;

// ── Helpers ────────────────────────────────────────────────────────────

// Parse source, resolve, and return error count.
static int Resolve(String const& src) {
  Vector<ParseError> parseErrors;
  ASTNode ast = ParseLTSL(src, &parseErrors);
  SymbolResolver resolver;
  resolver.Resolve(ast);
  return (int)resolver.GetErrors().size();
}

// Parse, resolve, and return the first error message (empty if no errors).
static String FirstError(String const& src) {
  Vector<ParseError> parseErrors;
  ASTNode ast = ParseLTSL(src, &parseErrors);
  SymbolResolver resolver;
  resolver.Resolve(ast);
  if (resolver.GetErrors().isEmpty()) return "";
  return resolver.GetErrors()[0].message;
}

// Parse, resolve, and return all error messages.
static Vector<String> AllErrors(String const& src) {
  Vector<ParseError> parseErrors;
  ASTNode ast = ParseLTSL(src, &parseErrors);
  SymbolResolver resolver;
  resolver.Resolve(ast);
  Vector<String> msgs;
  for (size_t i = 0; i < resolver.GetErrors().size(); ++i)
    msgs.push(resolver.GetErrors()[i].message);
  return msgs;
}

// Parse, resolve, and return a specific error's line number.
static int ErrorLine(String const& src, size_t index) {
  Vector<ParseError> parseErrors;
  ASTNode ast = ParseLTSL(src, &parseErrors);
  SymbolResolver resolver;
  resolver.Resolve(ast);
  if (index >= resolver.GetErrors().size()) return -1;
  return resolver.GetErrors()[index].line;
}

// Check if a string contains a substring.
static bool Contains(String const& haystack, String const& needle) {
  return haystack.contains(needle);
}

// ── Tests ──────────────────────────────────────────────────────────────

// --- Basic resolution ---

LTE_TEST(Resolver_EmptyModule) {
  ASTNode mod = new ASTModuleNodeT;
  SymbolResolver resolver;
  bool ok = resolver.Resolve(mod);
  LTE_CHECK(ok);
  LTE_CHECK_EQ(resolver.GetErrors().size(), (size_t)0);
}

LTE_TEST(Resolver_NullInput) {
  SymbolResolver resolver;
  bool ok = resolver.Resolve(ASTNode());
  LTE_CHECK(ok);
}

LTE_TEST(Resolver_VarUse) {
  LTE_CHECK_EQ(Resolve("var x 1\nx"), 0);
}

LTE_TEST(Resolver_VarUseUndefined) {
  LTE_CHECK_EQ(Resolve("x"), 1);
  String err = FirstError("x");
  LTE_CHECK(Contains(err, "undefined variable"));
  LTE_CHECK(Contains(err, "'x'"));
}

LTE_TEST(Resolver_DidYouMean) {
  String err = FirstError("var spawnR 1\nspwanR");
  LTE_CHECK(Contains(err, "did you mean"));
  LTE_CHECK(Contains(err, "'spawnR'"));
}

// --- Scoping ---

LTE_TEST(Resolver_InnerSeesOuter) {
  // var x 1 in outer scope, used in inner block
  LTE_CHECK_EQ(Resolve("var x 1\n"
                        "if true\n"
                        "  x"), 0);
}

LTE_TEST(Resolver_InnerVarInvisibleOutside) {
  // var y declared inside block, used outside
  LTE_CHECK_EQ(Resolve("if true\n"
                        "  var y 1\n"
                        "y"), 1);
  String err = FirstError("if true\n  var y 1\ny");
  LTE_CHECK(Contains(err, "undefined variable"));
}

LTE_TEST(Resolver_NestedScopes) {
  // Three levels of nesting, innermost uses outermost var
  LTE_CHECK_EQ(Resolve("var a 1\n"
                        "if true\n"
                        "  var b 2\n"
                        "  if true\n"
                        "    var c 3\n"
                        "    a"), 0);
}

LTE_TEST(Resolver_SiblingScopesIsolated) {
  // var x in one block, used in sibling block
  LTE_CHECK_EQ(Resolve("if true\n"
                        "  var x 1\n"
                        "if true\n"
                        "  x"), 1);
}

// --- Function declarations ---

LTE_TEST(Resolver_FuncDecl) {
  LTE_CHECK_EQ(Resolve("function Int add (Int a Int b)\n"
                        "  a + b"), 0);
}

LTE_TEST(Resolver_FuncCallArityOk) {
  LTE_CHECK_EQ(Resolve("function Int add (Int a Int b)\n"
                        "  a + b\n"
                        "(add 1 2)"), 0);
}

LTE_TEST(Resolver_FuncCallTooFewArgs) {
  LTE_CHECK_EQ(Resolve("function Int add (Int a Int b)\n"
                        "  a + b\n"
                        "(add 1)"), 1);
  String err = FirstError("function Int add (Int a Int b)\n  a + b\n(add 1)");
  LTE_CHECK(Contains(err, "expects 2 arguments"));
  LTE_CHECK(Contains(err, "got 1"));
}

LTE_TEST(Resolver_FuncCallTooManyArgs) {
  LTE_CHECK_EQ(Resolve("function Int add (Int a Int b)\n"
                        "  a + b\n"
                        "(add 1 2 3)"), 1);
  String err = FirstError("function Int add (Int a Int b)\n  a + b\n(add 1 2 3)");
  LTE_CHECK(Contains(err, "expects 2 arguments"));
  LTE_CHECK(Contains(err, "got 3"));
}

LTE_TEST(Resolver_FuncCallUndefined) {
  LTE_CHECK_EQ(Resolve("(foo 1 2)"), 1);
  String err = FirstError("(foo 1 2)");
  LTE_CHECK(Contains(err, "undefined function"));
  LTE_CHECK(Contains(err, "'foo'"));
}

LTE_TEST(Resolver_ForwardFunctionRef) {
  // Call 'bar' before it's declared — two-pass should handle this
  LTE_CHECK_EQ(Resolve("(bar)\n"
                        "function Void bar ()\n"
                        "  1"), 0);
}

LTE_TEST(Resolver_FuncParamScope) {
  // Function parameters visible in body
  LTE_CHECK_EQ(Resolve("function Int double (Int x)\n"
                        "  x + x"), 0);
}

LTE_TEST(Resolver_FuncParamInvisibleOutside) {
  // Function parameter not visible outside function
  LTE_CHECK_EQ(Resolve("function Int f (Int x)\n"
                        "  x\n"
                        "x"), 1);
}

LTE_TEST(Resolver_FuncParamShadow) {
  // Function parameter shadows outer var
  LTE_CHECK_EQ(Resolve("var x 1\n"
                        "function Int f (Int x)\n"
                        "  x + x"), 0);
}

LTE_TEST(Resolver_DuplicateDecl) {
  LTE_CHECK_EQ(Resolve("var x 1\nvar x 2"), 1);
  String err = FirstError("var x 1\nvar x 2");
  LTE_CHECK(Contains(err, "duplicate declaration"));
}

// --- For-loop scope ---

LTE_TEST(Resolver_ForIteratorScope) {
  // Iterator visible inside for body
  LTE_CHECK_EQ(Resolve("for i 0 10\n"
                        "  i"), 0);
}

LTE_TEST(Resolver_ForIteratorInvisibleOutside) {
  // Iterator not visible outside for body
  LTE_CHECK_EQ(Resolve("for i 0 10\n"
                        "  i\n"
                        "i"), 1);
}

// --- Type inference ---

LTE_TEST(Resolver_InferIntLiteral) {
  SymbolResolver resolver;
  Vector<ParseError> parseErrors;
  ASTNode ast = ParseLTSL("1", &parseErrors);
  resolver.Resolve(ast);
  auto mod = ASTNodeAs<ASTModuleNodeT>(ast);
  LTE_CHECK(mod != nullptr);
  // The module should have one statement (the literal as expr stmt or standalone)
  // For a bare literal, the parser wraps it in an expression statement
}

LTE_TEST(Resolver_InferBoolFromComparison) {
  LTE_CHECK_EQ(Resolve("var x (== 1 2)"), 0);
}

LTE_TEST(Resolver_InferBoolFromLogical) {
  LTE_CHECK_EQ(Resolve("var x (&& true false)"), 0);
}

LTE_TEST(Resolver_BinaryOpTypeInference) {
  SymbolResolver resolver;
  Vector<ParseError> parseErrors;
  ASTNode ast = ParseLTSL("1 + 2", &parseErrors);
  resolver.Resolve(ast);
  LTE_CHECK_EQ(resolver.GetErrors().size(), (size_t)0);
}

// --- While / If / Switch ---

LTE_TEST(Resolver_WhileBodyScope) {
  LTE_CHECK_EQ(Resolve("var x 1\n"
                        "while x\n"
                        "  var y 2\n"
                        "  y"), 0);
}

LTE_TEST(Resolver_WhileVarInvisibleOutside) {
  LTE_CHECK_EQ(Resolve("while true\n"
                        "  var y 1\n"
                        "y"), 1);
}

LTE_TEST(Resolver_IfThenScope) {
  LTE_CHECK_EQ(Resolve("var x 1\n"
                        "if x\n"
                        "  var y 2\n"
                        "  y"), 0);
}

LTE_TEST(Resolver_SwitchCases) {
  LTE_CHECK_EQ(Resolve("switch 1\n"
                        "  1\n"
                        "    1\n"
                        "  otherwise\n"
                        "    2"), 0);
}

// --- Multiple functions ---

LTE_TEST(Resolver_MultipleFunctions) {
  LTE_CHECK_EQ(Resolve("function Int a ()\n"
                        "  1\n"
                        "function Int b ()\n"
                        "  2\n"
                        "(a)\n(b)"), 0);
}

LTE_TEST(Resolver_FunctionCallsFunction) {
  LTE_CHECK_EQ(Resolve("function Int double (Int x)\n"
                        "  x + x\n"
                        "function Int quad (Int x)\n"
                        "  (double x)\n"
                        "(quad 5)"), 0);
}

// --- Block / Desc ---

LTE_TEST(Resolver_BlockScope) {
  LTE_CHECK_EQ(Resolve("var x 1\n"
                        "block\n"
                        "  var y 2\n"
                        "  x + y"), 0);
}

LTE_TEST(Resolver_DescScope) {
  LTE_CHECK_EQ(Resolve("var x 1\n"
                        "desc \"label\"\n"
                        "  x + 1"), 0);
}

// --- Edge cases ---

LTE_TEST(Resolver_NoArgFunction) {
  LTE_CHECK_EQ(Resolve("function Int pi ()\n"
                        "  314\n"
                        "(pi)"), 0);
}

LTE_TEST(Resolver_ZeroArgCallArity) {
  LTE_CHECK_EQ(Resolve("function Int pi ()\n"
                        "  314\n"
                        "(pi 1)"), 1);
  String err = FirstError("function Int pi ()\n  314\n(pi 1)");
  LTE_CHECK(Contains(err, "expects 0 arguments"));
  LTE_CHECK(Contains(err, "got 1"));
}

LTE_TEST(Resolver_ReturnInFunction) {
  LTE_CHECK_EQ(Resolve("function Int f (Int x)\n"
                        "  return x\n"
                        "  x + 1"), 0);
}

// --- Type declarations ---

LTE_TEST(Resolver_TypeDecl) {
  LTE_CHECK_EQ(Resolve("type Foo\n"
                        "  Int x\n"
                        "  Int y"), 0);
}

// --- Assignments ---

LTE_TEST(Resolver_AssignToVar) {
  LTE_CHECK_EQ(Resolve("var x 1\n"
                        "x = 2"), 0);
}

LTE_TEST(Resolver_AssignToUndefined) {
  LTE_CHECK_EQ(Resolve("x = 1"), 1);
}
