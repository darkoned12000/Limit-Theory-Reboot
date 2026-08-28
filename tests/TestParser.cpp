// Copyright (C) 2025  darkoned12000
// SPDX-License-Identifier: GPL-3.0-or-later
// Part of the ltheory-old-test modernization effort (Revamp Work).
// See NOTICE and LICENSE.GPL. Original engine (c) Josh Parnell, public domain.
//
// Unit tests for the new LTSL Parser (Phase 2 of the compiler rewrite).

#include "Harness.h"
#include "LTE/AST.h"
#include "LTE/Parser.h"

using namespace LTE;

// ── Helpers ────────────────────────────────────────────────────────────

// Parse source and return the AST module node (ignoring errors for convenience).
static ASTNode Parse(String const& src) {
  std::vector<ParseError> errors;
  return ParseLTSL(src, &errors);
}

// Count parse errors.
static int ErrorCount(String const& src) {
  std::vector<ParseError> errors;
  ParseLTSL(src, &errors);
  return (int)errors.size();
}

// Convenience casts
static ASTModuleNodeT const* AsModule(ASTNode const& n) {
  return dynamic_cast<ASTModuleNodeT const*>(n.t);
}

static ASTDeclNodeT const* AsDecl(ASTNode const& n) {
  return dynamic_cast<ASTDeclNodeT const*>(n.t);
}

static ASTFuncDeclNodeT const* AsFuncDecl(ASTNode const& n) {
  return dynamic_cast<ASTFuncDeclNodeT const*>(n.t);
}

static ASTBlockNodeT const* AsBlock(ASTNode const& n) {
  return dynamic_cast<ASTBlockNodeT const*>(n.t);
}

static ASTIfNodeT const* AsIf(ASTNode const& n) {
  return dynamic_cast<ASTIfNodeT const*>(n.t);
}

static ASTWhileNodeT const* AsWhile(ASTNode const& n) {
  return dynamic_cast<ASTWhileNodeT const*>(n.t);
}

static ASTForNodeT const* AsFor(ASTNode const& n) {
  return dynamic_cast<ASTForNodeT const*>(n.t);
}

static ASTSwitchNodeT const* AsSwitch(ASTNode const& n) {
  return dynamic_cast<ASTSwitchNodeT const*>(n.t);
}

static ASTIntLiteralNodeT const* AsIntLit(ASTNode const& n) {
  return dynamic_cast<ASTIntLiteralNodeT const*>(n.t);
}

static ASTFloatLiteralNodeT const* AsFloatLit(ASTNode const& n) {
  return dynamic_cast<ASTFloatLiteralNodeT const*>(n.t);
}

static ASTStringLiteralNodeT const* AsStringLit(ASTNode const& n) {
  return dynamic_cast<ASTStringLiteralNodeT const*>(n.t);
}

static ASTBoolLiteralNodeT const* AsBoolLit(ASTNode const& n) {
  return dynamic_cast<ASTBoolLiteralNodeT const*>(n.t);
}

static ASTIdentifierNodeT const* AsIdent(ASTNode const& n) {
  return dynamic_cast<ASTIdentifierNodeT const*>(n.t);
}

static ASTBinaryOpNodeT const* AsBinOp(ASTNode const& n) {
  return dynamic_cast<ASTBinaryOpNodeT const*>(n.t);
}

static ASTUnaryOpNodeT const* AsUnaryOp(ASTNode const& n) {
  return dynamic_cast<ASTUnaryOpNodeT const*>(n.t);
}

static ASTMethodCallNodeT const* AsMethodCall(ASTNode const& n) {
  return dynamic_cast<ASTMethodCallNodeT const*>(n.t);
}

static ASTFuncCallNodeT const* AsFuncCall(ASTNode const& n) {
  return dynamic_cast<ASTFuncCallNodeT const*>(n.t);
}

static ASTAssignNodeT const* AsAssign(ASTNode const& n) {
  return dynamic_cast<ASTAssignNodeT const*>(n.t);
}

static ASTReturnNodeT const* AsReturn(ASTNode const& n) {
  return dynamic_cast<ASTReturnNodeT const*>(n.t);
}

static ASTCastNodeT const* AsCast(ASTNode const& n) {
  return dynamic_cast<ASTCastNodeT const*>(n.t);
}

static ASTArrayLiteralNodeT const* AsArrayLit(ASTNode const& n) {
  return dynamic_cast<ASTArrayLiteralNodeT const*>(n.t);
}

static ASTConstructorNodeT const* AsConstructor(ASTNode const& n) {
  return dynamic_cast<ASTConstructorNodeT const*>(n.t);
}

static ASTPrintNodeT const* AsPrint(ASTNode const& n) {
  return dynamic_cast<ASTPrintNodeT const*>(n.t);
}

static ASTExprStmtNodeT const* AsExprStmt(ASTNode const& n) {
  return dynamic_cast<ASTExprStmtNodeT const*>(n.t);
}

// Get the nth statement from a module (nullptr if out of range).
static ASTNode ModuleStmt(ASTNode const& mod, int n) {
  ASTModuleNodeT const* m = AsModule(mod);
  if (!m || n >= (int)m->statements.size())
    return nullptr;
  return m->statements[n];
}

// Get the nth statement from a block (nullptr if out of range).
static ASTNode BlockStmt(ASTNode const& blk, int n) {
  ASTBlockNodeT const* b = AsBlock(blk);
  if (!b || n >= (int)b->statements.size())
    return nullptr;
  return b->statements[n];
}

// ── Literals ───────────────────────────────────────────────────────────

LTE_TEST(Parser_IntLiteral) {
  ASTNode mod = Parse("42");
  ASTNode stmt = ModuleStmt(mod, 0);
  LTE_CHECK(stmt != nullptr);
  ASTExprStmtNodeT const* es = AsExprStmt(stmt);
  LTE_CHECK(es != nullptr);
  ASTIntLiteralNodeT const* lit = AsIntLit(es->expression);
  LTE_CHECK(lit != nullptr);
  LTE_CHECK_EQ(lit->value, (long long)42);
}

LTE_TEST(Parser_FloatLiteral) {
  ASTNode mod = Parse("3.14");
  ASTNode stmt = ModuleStmt(mod, 0);
  ASTExprStmtNodeT const* es = AsExprStmt(stmt);
  LTE_CHECK(es != nullptr);
  ASTFloatLiteralNodeT const* lit = AsFloatLit(es->expression);
  LTE_CHECK(lit != nullptr);
  LTE_CHECK(lit->value > 3.13 && lit->value < 3.15);
}

LTE_TEST(Parser_StringLiteral) {
  ASTNode mod = Parse("\"hello world\"");
  ASTNode stmt = ModuleStmt(mod, 0);
  ASTExprStmtNodeT const* es = AsExprStmt(stmt);
  ASTStringLiteralNodeT const* lit = AsStringLit(es->expression);
  LTE_CHECK(lit != nullptr);
  LTE_CHECK_EQ(lit->value, String("hello world"));
}

LTE_TEST(Parser_SingleQuotedString) {
  ASTNode mod = Parse("'hello'");
  ASTNode stmt = ModuleStmt(mod, 0);
  ASTExprStmtNodeT const* es = AsExprStmt(stmt);
  ASTStringLiteralNodeT const* lit = AsStringLit(es->expression);
  LTE_CHECK(lit != nullptr);
  LTE_CHECK_EQ(lit->value, String("hello"));
}

LTE_TEST(Parser_BoolLiterals) {
  {
    ASTNode mod = Parse("true");
    ASTExprStmtNodeT const* es = AsExprStmt(ModuleStmt(mod, 0));
    ASTBoolLiteralNodeT const* lit = AsBoolLit(es->expression);
    LTE_CHECK(lit != nullptr);
    LTE_CHECK(lit->value == true);
  }
  {
    ASTNode mod = Parse("false");
    ASTExprStmtNodeT const* es = AsExprStmt(ModuleStmt(mod, 0));
    ASTBoolLiteralNodeT const* lit = AsBoolLit(es->expression);
    LTE_CHECK(lit != nullptr);
    LTE_CHECK(lit->value == false);
  }
}

LTE_TEST(Parser_NullLiteral) {
  ASTNode mod = Parse("null");
  ASTExprStmtNodeT const* es = AsExprStmt(ModuleStmt(mod, 0));
  LTE_CHECK(es->expression->kind == AST_NULL_LITERAL);
}

// ── Identifiers ────────────────────────────────────────────────────────

LTE_TEST(Parser_Identifier) {
  ASTNode mod = Parse("myVar");
  ASTExprStmtNodeT const* es = AsExprStmt(ModuleStmt(mod, 0));
  ASTIdentifierNodeT const* id = AsIdent(es->expression);
  LTE_CHECK(id != nullptr);
  LTE_CHECK_EQ(id->name, String("myVar"));
}

// ── Binary expressions ─────────────────────────────────────────────────

LTE_TEST(Parser_BinaryAdd) {
  ASTNode mod = Parse("1 + 2");
  ASTExprStmtNodeT const* es = AsExprStmt(ModuleStmt(mod, 0));
  ASTBinaryOpNodeT const* op = AsBinOp(es->expression);
  LTE_CHECK(op != nullptr);
  LTE_CHECK_EQ(op->op, String("+"));
  LTE_CHECK(op->left != nullptr);
  LTE_CHECK(op->right != nullptr);
  ASTIntLiteralNodeT const* l = AsIntLit(op->left);
  ASTIntLiteralNodeT const* r = AsIntLit(op->right);
  LTE_CHECK(l != nullptr);
  LTE_CHECK(r != nullptr);
  LTE_CHECK_EQ(l->value, (long long)1);
  LTE_CHECK_EQ(r->value, (long long)2);
}

LTE_TEST(Parser_Precedence) {
  // 1 + 2 * 3 should parse as 1 + (2 * 3)
  ASTNode mod = Parse("1 + 2 * 3");
  ASTExprStmtNodeT const* es = AsExprStmt(ModuleStmt(mod, 0));
  ASTBinaryOpNodeT const* add = AsBinOp(es->expression);
  LTE_CHECK(add != nullptr);
  LTE_CHECK_EQ(add->op, String("+"));
  ASTIntLiteralNodeT const* l = AsIntLit(add->left);
  LTE_CHECK(l != nullptr);
  LTE_CHECK_EQ(l->value, (long long)1);
  ASTBinaryOpNodeT const* mul = AsBinOp(add->right);
  LTE_CHECK(mul != nullptr);
  LTE_CHECK_EQ(mul->op, String("*"));
}

LTE_TEST(Parser_ParenOverride) {
  // (1 + 2) * 3 should parse as (1 + 2) * 3
  ASTNode mod = Parse("(1 + 2) * 3");
  ASTExprStmtNodeT const* es = AsExprStmt(ModuleStmt(mod, 0));
  ASTBinaryOpNodeT const* mul = AsBinOp(es->expression);
  LTE_CHECK(mul != nullptr);
  LTE_CHECK_EQ(mul->op, String("*"));
  ASTBinaryOpNodeT const* add = AsBinOp(mul->left);
  LTE_CHECK(add != nullptr);
  LTE_CHECK_EQ(add->op, String("+"));
}

// ── Unary expressions ──────────────────────────────────────────────────

LTE_TEST(Parser_UnaryNegate) {
  ASTNode mod = Parse("-5");
  ASTExprStmtNodeT const* es = AsExprStmt(ModuleStmt(mod, 0));
  ASTUnaryOpNodeT const* op = AsUnaryOp(es->expression);
  LTE_CHECK(op != nullptr);
  LTE_CHECK_EQ(op->op, String("-"));
  ASTIntLiteralNodeT const* lit = AsIntLit(op->operand);
  LTE_CHECK(lit != nullptr);
  LTE_CHECK_EQ(lit->value, (long long)5);
}

LTE_TEST(Parser_UnaryNot) {
  ASTNode mod = Parse("!true");
  ASTExprStmtNodeT const* es = AsExprStmt(ModuleStmt(mod, 0));
  ASTUnaryOpNodeT const* op = AsUnaryOp(es->expression);
  LTE_CHECK(op != nullptr);
  LTE_CHECK_EQ(op->op, String("!"));
  ASTBoolLiteralNodeT const* lit = AsBoolLit(op->operand);
  LTE_CHECK(lit != nullptr);
  LTE_CHECK(lit->value == true);
}

// ── Method calls / dot access ──────────────────────────────────────────

LTE_TEST(Parser_DotAccess) {
  ASTNode mod = Parse("self.x");
  ASTExprStmtNodeT const* es = AsExprStmt(ModuleStmt(mod, 0));
  ASTMethodCallNodeT const* mc = AsMethodCall(es->expression);
  LTE_CHECK(mc != nullptr);
  LTE_CHECK_EQ(mc->methodName, String("x"));
  LTE_CHECK(mc->args.size() == 0);
  ASTIdentifierNodeT const* obj = AsIdent(mc->object);
  LTE_CHECK(obj != nullptr);
  LTE_CHECK_EQ(obj->name, String("self"));
}

LTE_TEST(Parser_MethodCall) {
  ASTNode mod = Parse("self.SetPos pos");
  ASTExprStmtNodeT const* es = AsExprStmt(ModuleStmt(mod, 0));
  ASTMethodCallNodeT const* mc = AsMethodCall(es->expression);
  LTE_CHECK(mc != nullptr);
  LTE_CHECK_EQ(mc->methodName, String("SetPos"));
  LTE_CHECK(mc->args.size() == 1);
}

LTE_TEST(Parser_ChainedMethodCall) {
  ASTNode mod = Parse("self.GetPos.x");
  ASTExprStmtNodeT const* es = AsExprStmt(ModuleStmt(mod, 0));
  ASTMethodCallNodeT const* outer = AsMethodCall(es->expression);
  LTE_CHECK(outer != nullptr);
  LTE_CHECK_EQ(outer->methodName, String("x"));
  ASTMethodCallNodeT const* inner = AsMethodCall(outer->object);
  LTE_CHECK(inner != nullptr);
  LTE_CHECK_EQ(inner->methodName, String("GetPos"));
}

LTE_TEST(Parser_MethodCallWithArgs) {
  ASTNode mod = Parse("a.Add b c");
  ASTExprStmtNodeT const* es = AsExprStmt(ModuleStmt(mod, 0));
  ASTMethodCallNodeT const* mc = AsMethodCall(es->expression);
  LTE_CHECK(mc != nullptr);
  LTE_CHECK_EQ(mc->methodName, String("Add"));
  LTE_CHECK(mc->args.size() == 2);
}

// ── Statements ─────────────────────────────────────────────────────────

LTE_TEST(Parser_VarDecl) {
  ASTNode mod = Parse("var x 42");
  ASTDeclNodeT const* d = AsDecl(ModuleStmt(mod, 0));
  LTE_CHECK(d != nullptr);
  LTE_CHECK_EQ(d->kind, AST_VAR_DECL);
  LTE_CHECK_EQ(d->name, String("x"));
  ASTIntLiteralNodeT const* lit = AsIntLit(d->initializer);
  LTE_CHECK(lit != nullptr);
  LTE_CHECK_EQ(lit->value, (long long)42);
}

LTE_TEST(Parser_RefDecl) {
  ASTNode mod = Parse("ref y x");
  ASTDeclNodeT const* d = AsDecl(ModuleStmt(mod, 0));
  LTE_CHECK(d != nullptr);
  LTE_CHECK_EQ(d->kind, AST_REF_DECL);
  LTE_CHECK_EQ(d->name, String("y"));
}

LTE_TEST(Parser_StaticDecl) {
  ASTNode mod = Parse("static count 0");
  ASTDeclNodeT const* d = AsDecl(ModuleStmt(mod, 0));
  LTE_CHECK(d != nullptr);
  LTE_CHECK_EQ(d->kind, AST_STATIC_DECL);
  LTE_CHECK_EQ(d->name, String("count"));
}

LTE_TEST(Parser_Assignment) {
  ASTNode mod = Parse("x = 10");
  ASTAssignNodeT const* a = AsAssign(ModuleStmt(mod, 0));
  LTE_CHECK(a != nullptr);
  LTE_CHECK_EQ(a->op, String("="));
  ASTIdentifierNodeT const* target = AsIdent(a->target);
  LTE_CHECK(target != nullptr);
  LTE_CHECK_EQ(target->name, String("x"));
  ASTIntLiteralNodeT const* val = AsIntLit(a->value);
  LTE_CHECK(val != nullptr);
  LTE_CHECK_EQ(val->value, (long long)10);
}

LTE_TEST(Parser_PlusAssign) {
  ASTNode mod = Parse("x += 5");
  ASTAssignNodeT const* a = AsAssign(ModuleStmt(mod, 0));
  LTE_CHECK(a != nullptr);
  LTE_CHECK_EQ(a->op, String("+="));
}

LTE_TEST(Parser_Break) {
  ASTNode mod = Parse("break");
  ASTNode stmt = ModuleStmt(mod, 0);
  LTE_CHECK(stmt->kind == AST_BREAK);
}

LTE_TEST(Parser_Return) {
  ASTNode mod = Parse("return 42");
  ASTReturnNodeT const* r = AsReturn(ModuleStmt(mod, 0));
  LTE_CHECK(r != nullptr);
  ASTIntLiteralNodeT const* lit = AsIntLit(r->value);
  LTE_CHECK(lit != nullptr);
  LTE_CHECK_EQ(lit->value, (long long)42);
}

LTE_TEST(Parser_ReturnBare) {
  ASTNode mod = Parse("return");
  ASTReturnNodeT const* r = AsReturn(ModuleStmt(mod, 0));
  LTE_CHECK(r != nullptr);
  LTE_CHECK(r->value == nullptr);
}

// ── Blocks ─────────────────────────────────────────────────────────────

LTE_TEST(Parser_SimpleBlock) {
  ASTNode mod = Parse("if true\n  1\n  2");
  ASTIfNodeT const* ifn = AsIf(ModuleStmt(mod, 0));
  LTE_CHECK(ifn != nullptr);
  ASTBlockNodeT const* blk = AsBlock(ifn->thenBlock);
  LTE_CHECK(blk != nullptr);
  LTE_CHECK(blk->statements.size() == 2);
}

LTE_TEST(Parser_NestedBlock) {
  String src =
    "if true\n"
    "  if false\n"
    "    1\n"
    "  2";
  ASTNode mod = Parse(src);
  ASTIfNodeT const* outer = AsIf(ModuleStmt(mod, 0));
  LTE_CHECK(outer != nullptr);
  ASTBlockNodeT const* outerBlk = AsBlock(outer->thenBlock);
  LTE_CHECK(outerBlk != nullptr);
  LTE_CHECK(outerBlk->statements.size() == 2);
  ASTIfNodeT const* inner = AsIf(outerBlk->statements[0]);
  LTE_CHECK(inner != nullptr);
}

// ── Control flow ───────────────────────────────────────────────────────

LTE_TEST(Parser_IfElse) {
  String src =
    "if true\n"
    "  1\n"
    "else\n"
    "  2";
  ASTNode mod = Parse(src);
  ASTIfNodeT const* ifn = AsIf(ModuleStmt(mod, 0));
  LTE_CHECK(ifn != nullptr);
  LTE_CHECK(ifn->thenBlock != nullptr);
  LTE_CHECK(ifn->elseBlock != nullptr);
}

LTE_TEST(Parser_WhileLoop) {
  String src =
    "while true\n"
    "  1";
  ASTNode mod = Parse(src);
  ASTWhileNodeT const* w = AsWhile(ModuleStmt(mod, 0));
  LTE_CHECK(w != nullptr);
  LTE_CHECK(w->condition != nullptr);
  LTE_CHECK(w->body != nullptr);
}

LTE_TEST(Parser_ForLoop) {
  String src =
    "for i 0 10 1\n"
    "  i";
  ASTNode mod = Parse(src);
  ASTForNodeT const* f = AsFor(ModuleStmt(mod, 0));
  LTE_CHECK(f != nullptr);
  LTE_CHECK_EQ(f->iteratorName, String("i"));
  LTE_CHECK(f->init != nullptr);
  LTE_CHECK(f->condition != nullptr);
  LTE_CHECK(f->step != nullptr);
  LTE_CHECK(f->body != nullptr);
}

LTE_TEST(Parser_SwitchDefault) {
  String src =
    "switch\n"
    "  x == 1\n"
    "    10\n"
    "  otherwise\n"
    "    20";
  ASTNode mod = Parse(src);
  ASTSwitchNodeT const* sw = AsSwitch(ModuleStmt(mod, 0));
  LTE_CHECK(sw != nullptr);
  LTE_CHECK(sw->cases.size() == 1);
  LTE_CHECK(sw->otherwise != nullptr);
}

// ── Function declarations ──────────────────────────────────────────────

LTE_TEST(Parser_FuncDecl) {
  String src =
    "function Int Add (Int a Int b)\n"
    "  a + b";
  ASTNode mod = Parse(src);
  ASTFuncDeclNodeT const* fn = AsFuncDecl(ModuleStmt(mod, 0));
  LTE_CHECK(fn != nullptr);
  LTE_CHECK_EQ(fn->returnType, String("Int"));
  LTE_CHECK_EQ(fn->name, String("Add"));
  LTE_CHECK(fn->paramTypes.size() == 2);
  LTE_CHECK_EQ(fn->paramTypes[0], String("Int"));
  LTE_CHECK_EQ(fn->paramTypes[1], String("Int"));
  LTE_CHECK_EQ(fn->paramNames[0], String("a"));
  LTE_CHECK_EQ(fn->paramNames[1], String("b"));
  LTE_CHECK(fn->body != nullptr);
}

// ── Cast ───────────────────────────────────────────────────────────────

LTE_TEST(Parser_Cast) {
  ASTNode mod = Parse("cast Int x");
  ASTExprStmtNodeT const* es = AsExprStmt(ModuleStmt(mod, 0));
  ASTCastNodeT const* c = AsCast(es->expression);
  LTE_CHECK(c != nullptr);
  LTE_CHECK_EQ(c->typeName, String("Int"));
  LTE_CHECK(c->operand != nullptr);
}

// ── Array literal ──────────────────────────────────────────────────────

LTE_TEST(Parser_ArrayLiteral) {
  ASTNode mod = Parse("[1, 2, 3]");
  ASTExprStmtNodeT const* es = AsExprStmt(ModuleStmt(mod, 0));
  ASTArrayLiteralNodeT const* arr = AsArrayLit(es->expression);
  LTE_CHECK(arr != nullptr);
  LTE_CHECK(arr->elements.size() == 3);
}

LTE_TEST(Parser_EmptyArrayLiteral) {
  ASTNode mod = Parse("[]");
  ASTExprStmtNodeT const* es = AsExprStmt(ModuleStmt(mod, 0));
  ASTArrayLiteralNodeT const* arr = AsArrayLit(es->expression);
  LTE_CHECK(arr != nullptr);
  LTE_CHECK(arr->elements.size() == 0);
}

// ── Debug print ────────────────────────────────────────────────────────

LTE_TEST(Parser_DebugPrint) {
  ASTNode mod = Parse("@ x");
  ASTPrintNodeT const* p = AsPrint(ModuleStmt(mod, 0));
  LTE_CHECK(p != nullptr);
  ASTIdentifierNodeT const* id = AsIdent(p->operand);
  LTE_CHECK(id != nullptr);
  LTE_CHECK_EQ(id->name, String("x"));
}

// ── Special forms ──────────────────────────────────────────────────────

LTE_TEST(Parser_AddressDeref) {
  ASTNode mod = Parse("address x");
  ASTExprStmtNodeT const* es = AsExprStmt(ModuleStmt(mod, 0));
  LTE_CHECK(es->expression->kind == AST_ADDRESS);
}

LTE_TEST(Parser_Deref) {
  ASTNode mod = Parse("deref p");
  ASTExprStmtNodeT const* es = AsExprStmt(ModuleStmt(mod, 0));
  LTE_CHECK(es->expression->kind == AST_DEREF);
}

// ── Multiple statements ────────────────────────────────────────────────

LTE_TEST(Parser_MultipleStatements) {
  String src =
    "var x 1\n"
    "var y 2\n"
    "x + y";
  ASTNode mod = Parse(src);
  ASTModuleNodeT const* m = AsModule(mod);
  LTE_CHECK(m != nullptr);
  LTE_CHECK(m->statements.size() == 3);
}

// ── Source location tracking ───────────────────────────────────────────

LTE_TEST(Parser_SourceLocations) {
  ASTNode mod = Parse("42");
  ASTExprStmtNodeT const* es = AsExprStmt(ModuleStmt(mod, 0));
  ASTIntLiteralNodeT const* lit = AsIntLit(es->expression);
  LTE_CHECK(lit != nullptr);
  LTE_CHECK(lit->loc.line > 0);
  LTE_CHECK(lit->loc.column > 0);
}

// ── Error cases ────────────────────────────────────────────────────────

LTE_TEST(Parser_MissingExpression) {
  LTE_CHECK(ErrorCount("var x") > 0);
}

LTE_TEST(Parser_UnterminatedBlock) {
  // "if true\n  1" — block terminated by EOF DEDENT auto-flush (valid)
  LTE_CHECK(ErrorCount("if true\n  1") == 0);
  // "if true\n  1\nbad" — dedent to column 0 without matching indent level
  // is valid (dedents back to 0). But a missing block after if is an error.
  LTE_CHECK(ErrorCount("if") > 0);
  // Incomplete expression
  LTE_CHECK(ErrorCount("var x") > 0);
}

LTE_TEST(Parser_UnexpectedToken) {
  LTE_CHECK(ErrorCount("1 +") > 0);
}

// ── Real-world-ish code ────────────────────────────────────────────────

LTE_TEST(Parser_FuncWithBody) {
  String src =
    "function Int Add (Int a Int b)\n"
    "  var result 0\n"
    "  result = a + b\n"
    "  return result";
  ASTNode mod = Parse(src);
  ASTFuncDeclNodeT const* fn = AsFuncDecl(ModuleStmt(mod, 0));
  LTE_CHECK(fn != nullptr);
  ASTBlockNodeT const* body = AsBlock(fn->body);
  LTE_CHECK(body != nullptr);
  LTE_CHECK(body->statements.size() == 3);
}

LTE_TEST(Parser_MethodCallChain) {
  ASTNode mod = Parse("self.GetPos.x");
  ASTExprStmtNodeT const* es = AsExprStmt(ModuleStmt(mod, 0));
  ASTMethodCallNodeT const* mc = AsMethodCall(es->expression);
  LTE_CHECK(mc != nullptr);
  LTE_CHECK_EQ(mc->methodName, String("x"));
}

LTE_TEST(Parser_MultilineIf) {
  String src =
    "if x > 0\n"
    "  var y 1\n"
    "  y = x + 1\n"
    "  y";
  ASTNode mod = Parse(src);
  ASTIfNodeT const* ifn = AsIf(ModuleStmt(mod, 0));
  LTE_CHECK(ifn != nullptr);
  ASTBlockNodeT const* body = AsBlock(ifn->thenBlock);
  LTE_CHECK(body != nullptr);
  LTE_CHECK(body->statements.size() == 3);
}

// ── Comparison operators ───────────────────────────────────────────────

LTE_TEST(Parser_Comparison) {
  ASTNode mod = Parse("1 < 2");
  ASTExprStmtNodeT const* es = AsExprStmt(ModuleStmt(mod, 0));
  ASTBinaryOpNodeT const* op = AsBinOp(es->expression);
  LTE_CHECK(op != nullptr);
  LTE_CHECK_EQ(op->op, String("<"));
  LTE_CHECK(op->left != nullptr);
  LTE_CHECK(op->right != nullptr);
}

LTE_TEST(Parser_ComparisonEqual) {
  ASTNode mod = Parse("x == 5");
  ASTExprStmtNodeT const* es = AsExprStmt(ModuleStmt(mod, 0));
  ASTBinaryOpNodeT const* op = AsBinOp(es->expression);
  LTE_CHECK(op != nullptr);
  LTE_CHECK_EQ(op->op, String("=="));
}

LTE_TEST(Parser_ComparisonNotEqual) {
  ASTNode mod = Parse("x != 5");
  ASTExprStmtNodeT const* es = AsExprStmt(ModuleStmt(mod, 0));
  ASTBinaryOpNodeT const* op = AsBinOp(es->expression);
  LTE_CHECK(op != nullptr);
  LTE_CHECK_EQ(op->op, String("!="));
}

LTE_TEST(Parser_ComparisonGreaterEqual) {
  ASTNode mod = Parse("x >= 5");
  ASTExprStmtNodeT const* es = AsExprStmt(ModuleStmt(mod, 0));
  ASTBinaryOpNodeT const* op = AsBinOp(es->expression);
  LTE_CHECK(op != nullptr);
  LTE_CHECK_EQ(op->op, String(">="));
}

LTE_TEST(Parser_ComparisonLessEqual) {
  ASTNode mod = Parse("x <= 5");
  ASTExprStmtNodeT const* es = AsExprStmt(ModuleStmt(mod, 0));
  ASTBinaryOpNodeT const* op = AsBinOp(es->expression);
  LTE_CHECK(op != nullptr);
  LTE_CHECK_EQ(op->op, String("<="));
}

// ── Logical operators ──────────────────────────────────────────────────

LTE_TEST(Parser_LogicalAnd) {
  ASTNode mod = Parse("true && false");
  ASTExprStmtNodeT const* es = AsExprStmt(ModuleStmt(mod, 0));
  ASTBinaryOpNodeT const* op = AsBinOp(es->expression);
  LTE_CHECK(op != nullptr);
  LTE_CHECK_EQ(op->op, String("&&"));
  ASTBoolLiteralNodeT const* l = AsBoolLit(op->left);
  ASTBoolLiteralNodeT const* r = AsBoolLit(op->right);
  LTE_CHECK(l != nullptr);
  LTE_CHECK(r != nullptr);
}

LTE_TEST(Parser_LogicalOr) {
  ASTNode mod = Parse("true || false");
  ASTExprStmtNodeT const* es = AsExprStmt(ModuleStmt(mod, 0));
  ASTBinaryOpNodeT const* op = AsBinOp(es->expression);
  LTE_CHECK(op != nullptr);
  LTE_CHECK_EQ(op->op, String("||"));
}

LTE_TEST(Parser_LogicalPrecedence) {
  // && binds tighter than ||: a || b && c  ==  a || (b && c)
  ASTNode mod = Parse("a || b && c");
  ASTExprStmtNodeT const* es = AsExprStmt(ModuleStmt(mod, 0));
  ASTBinaryOpNodeT const* orOp = AsBinOp(es->expression);
  LTE_CHECK(orOp != nullptr);
  LTE_CHECK_EQ(orOp->op, String("||"));
  ASTBinaryOpNodeT const* andOp = AsBinOp(orOp->right);
  LTE_CHECK(andOp != nullptr);
  LTE_CHECK_EQ(andOp->op, String("&&"));
}

// ── Modulo operator ────────────────────────────────────────────────────

LTE_TEST(Parser_Modulo) {
  ASTNode mod = Parse("5 % 3");
  ASTExprStmtNodeT const* es = AsExprStmt(ModuleStmt(mod, 0));
  ASTBinaryOpNodeT const* op = AsBinOp(es->expression);
  LTE_CHECK(op != nullptr);
  LTE_CHECK_EQ(op->op, String("%"));
  ASTIntLiteralNodeT const* l = AsIntLit(op->left);
  ASTIntLiteralNodeT const* r = AsIntLit(op->right);
  LTE_CHECK(l != nullptr);
  LTE_CHECK_EQ(l->value, (long long)5);
  LTE_CHECK(r != nullptr);
  LTE_CHECK_EQ(r->value, (long long)3);
}

// ── Compound assignments ───────────────────────────────────────────────

LTE_TEST(Parser_MinusAssign) {
  ASTNode mod = Parse("x -= 1");
  ASTAssignNodeT const* a = AsAssign(ModuleStmt(mod, 0));
  LTE_CHECK(a != nullptr);
  LTE_CHECK_EQ(a->op, String("-="));
}

LTE_TEST(Parser_MultiplyAssign) {
  ASTNode mod = Parse("x *= 2");
  ASTAssignNodeT const* a = AsAssign(ModuleStmt(mod, 0));
  LTE_CHECK(a != nullptr);
  LTE_CHECK_EQ(a->op, String("*="));
}

LTE_TEST(Parser_DivideAssign) {
  ASTNode mod = Parse("x /= 3");
  ASTAssignNodeT const* a = AsAssign(ModuleStmt(mod, 0));
  LTE_CHECK(a != nullptr);
  LTE_CHECK_EQ(a->op, String("/="));
}

// ── Method call with paren args ────────────────────────────────────────

LTE_TEST(MethodCallParenArgs) {
  ASTNode mod = Parse("obj.Method(1, 2)");
  ASTExprStmtNodeT const* es = AsExprStmt(ModuleStmt(mod, 0));
  ASTMethodCallNodeT const* mc = AsMethodCall(es->expression);
  LTE_CHECK(mc != nullptr);
  LTE_CHECK_EQ(mc->methodName, String("Method"));
  LTE_CHECK(mc->args.size() == 2);
}

LTE_TEST(MethodCallParenSingleArg) {
  ASTNode mod = Parse("obj.Run(x)");
  ASTExprStmtNodeT const* es = AsExprStmt(ModuleStmt(mod, 0));
  ASTMethodCallNodeT const* mc = AsMethodCall(es->expression);
  LTE_CHECK(mc != nullptr);
  LTE_CHECK_EQ(mc->methodName, String("Run"));
  LTE_CHECK(mc->args.size() == 1);
}

// ── Type declarations ──────────────────────────────────────────────────

LTE_TEST(Parser_TypeDecl) {
  String src =
    "type Vec3\n"
    "  Float x\n"
    "  Float y\n"
    "  Float z";
  ASTNode mod = Parse(src);
  ASTModuleNodeT const* m = AsModule(mod);
  LTE_CHECK(m != nullptr);
  LTE_CHECK(m->statements.size() == 1);
  LTE_CHECK(m->statements[0]->kind == AST_TYPE_DECL);
}

LTE_TEST(Parser_TypeDeclEmpty) {
  ASTNode mod = Parse("type Empty");
  ASTModuleNodeT const* m = AsModule(mod);
  LTE_CHECK(m != nullptr);
  LTE_CHECK(m->statements.size() == 1);
  LTE_CHECK(m->statements[0]->kind == AST_TYPE_DECL);
}

// ── desc block ─────────────────────────────────────────────────────────

LTE_TEST(Parser_DescBlock) {
  String src =
    "desc \"label\"\n"
    "  1\n"
    "  2";
  ASTNode mod = Parse(src);
  ASTBlockNodeT const* blk = AsBlock(ModuleStmt(mod, 0));
  LTE_CHECK(blk != nullptr);
  LTE_CHECK(blk->isDesc == true);
  LTE_CHECK_EQ(blk->label, String("label"));
  LTE_CHECK(blk->statements.size() == 2);
}

LTE_TEST(Parser_DescNoLabel) {
  String src =
    "desc\n"
    "  1";
  ASTNode mod = Parse(src);
  ASTBlockNodeT const* blk = AsBlock(ModuleStmt(mod, 0));
  LTE_CHECK(blk != nullptr);
  LTE_CHECK(blk->isDesc == true);
  LTE_CHECK(blk->label.size() == 0);
}

// ── block keyword ──────────────────────────────────────────────────────

LTE_TEST(Parser_BlockKeyword) {
  String src =
    "block\n"
    "  1\n"
    "  2";
  ASTNode mod = Parse(src);
  ASTBlockNodeT const* blk = AsBlock(ModuleStmt(mod, 0));
  LTE_CHECK(blk != nullptr);
  LTE_CHECK(blk->isDesc == false);
  LTE_CHECK(blk->statements.size() == 2);
}

// ── Switch with multiple cases ─────────────────────────────────────────

LTE_TEST(Parser_SwitchMultiCase) {
  String src =
    "switch\n"
    "  x == 1\n"
    "    10\n"
    "  x == 2\n"
    "    20\n"
    "  otherwise\n"
    "    30";
  ASTNode mod = Parse(src);
  ASTSwitchNodeT const* sw = AsSwitch(ModuleStmt(mod, 0));
  LTE_CHECK(sw != nullptr);
  LTE_CHECK(sw->cases.size() == 2);
  LTE_CHECK(sw->otherwise != nullptr);
  ASTBlockNodeT const* blk = AsBlock(sw->cases[0].body);
  LTE_CHECK(blk != nullptr);
  LTE_CHECK(blk->statements.size() == 1);
}

// ── else-if chains ─────────────────────────────────────────────────────

LTE_TEST(Parser_ElseIfChain) {
  String src =
    "if true\n"
    "  1\n"
    "else if false\n"
    "  2\n"
    "else\n"
    "  3";
  ASTNode mod = Parse(src);
  ASTIfNodeT const* ifn = AsIf(ModuleStmt(mod, 0));
  LTE_CHECK(ifn != nullptr);
  LTE_CHECK(ifn->thenBlock != nullptr);
  LTE_CHECK(ifn->elseBlock != nullptr);
  ASTIfNodeT const* elseIf = AsIf(ifn->elseBlock);
  LTE_CHECK(elseIf != nullptr);
  LTE_CHECK(elseIf->thenBlock != nullptr);
  LTE_CHECK(elseIf->elseBlock != nullptr);
}

// ── Nested parentheses ────────────────────────────────────────────────

LTE_TEST(Parser_NestedParens) {
  ASTNode mod = Parse("(1 + (2 * 3))");
  ASTExprStmtNodeT const* es = AsExprStmt(ModuleStmt(mod, 0));
  ASTBinaryOpNodeT const* add = AsBinOp(es->expression);
  LTE_CHECK(add != nullptr);
  LTE_CHECK_EQ(add->op, String("+"));
  ASTIntLiteralNodeT const* l = AsIntLit(add->left);
  LTE_CHECK(l != nullptr);
  LTE_CHECK_EQ(l->value, (long long)1);
  ASTBinaryOpNodeT const* mul = AsBinOp(add->right);
  LTE_CHECK(mul != nullptr);
  LTE_CHECK_EQ(mul->op, String("*"));
}

// ── Method chain with args ─────────────────────────────────────────────

LTE_TEST(Parser_MethodChainWithArgs) {
  ASTNode mod = Parse("obj.Filter(x).Map(y)");
  ASTExprStmtNodeT const* es = AsExprStmt(ModuleStmt(mod, 0));
  ASTMethodCallNodeT const* outer = AsMethodCall(es->expression);
  LTE_CHECK(outer != nullptr);
  LTE_CHECK_EQ(outer->methodName, String("Map"));
  LTE_CHECK(outer->args.size() == 1);
  ASTMethodCallNodeT const* inner = AsMethodCall(outer->object);
  LTE_CHECK(inner != nullptr);
  LTE_CHECK_EQ(inner->methodName, String("Filter"));
  LTE_CHECK(inner->args.size() == 1);
}

// ── Declarations without initializer ───────────────────────────────────

LTE_TEST(Parser_VarDeclNoInit) {
  ASTNode mod = Parse("var x");
  ASTDeclNodeT const* d = AsDecl(ModuleStmt(mod, 0));
  LTE_CHECK(d != nullptr);
  LTE_CHECK_EQ(d->kind, AST_VAR_DECL);
  LTE_CHECK_EQ(d->name, String("x"));
  LTE_CHECK(d->initializer == nullptr);
}

// ── Expression precedence deep ─────────────────────────────────────────

LTE_TEST(Parser_OperatorPrecedenceDeep) {
  ASTNode mod = Parse("a + b * c - d / e");
  ASTExprStmtNodeT const* es = AsExprStmt(ModuleStmt(mod, 0));
  ASTBinaryOpNodeT const* sub = AsBinOp(es->expression);
  LTE_CHECK(sub != nullptr);
  LTE_CHECK_EQ(sub->op, String("-"));
  ASTBinaryOpNodeT const* add = AsBinOp(sub->left);
  LTE_CHECK(add != nullptr);
  LTE_CHECK_EQ(add->op, String("+"));
  ASTBinaryOpNodeT const* div = AsBinOp(sub->right);
  LTE_CHECK(div != nullptr);
  LTE_CHECK_EQ(div->op, String("/"));
}

// ── Comment lines ──────────────────────────────────────────────────────

LTE_TEST(Parser_CommentOnly) {
  ASTNode mod = Parse("# this is a comment");
  ASTModuleNodeT const* m = AsModule(mod);
  LTE_CHECK(m != nullptr);
  LTE_CHECK(m->statements.size() == 0);
}

LTE_TEST(Parser_CommentBeforeStatement) {
  ASTNode mod = Parse("# comment\n42");
  ASTModuleNodeT const* m = AsModule(mod);
  LTE_CHECK(m != nullptr);
  LTE_CHECK(m->statements.size() == 1);
  ASTExprStmtNodeT const* es = AsExprStmt(m->statements[0]);
  ASTIntLiteralNodeT const* lit = AsIntLit(es->expression);
  LTE_CHECK(lit != nullptr);
  LTE_CHECK_EQ(lit->value, (long long)42);
}

// ── Large block ────────────────────────────────────────────────────────

LTE_TEST(Parser_LargeBlock) {
  String src =
    "if true\n"
    "  1\n"
    "  2\n"
    "  3\n"
    "  4\n"
    "  5";
  ASTNode mod = Parse(src);
  ASTIfNodeT const* ifn = AsIf(ModuleStmt(mod, 0));
  LTE_CHECK(ifn != nullptr);
  ASTBlockNodeT const* blk = AsBlock(ifn->thenBlock);
  LTE_CHECK(blk != nullptr);
  LTE_CHECK(blk->statements.size() == 5);
}

// ── While with complex condition ───────────────────────────────────────

LTE_TEST(Parser_WhileComplexCond) {
  String src =
    "while x > 0 && y < 10\n"
    "  x = x - 1";
  ASTNode mod = Parse(src);
  ASTWhileNodeT const* w = AsWhile(ModuleStmt(mod, 0));
  LTE_CHECK(w != nullptr);
  ASTBinaryOpNodeT const* andOp = AsBinOp(w->condition);
  LTE_CHECK(andOp != nullptr);
  LTE_CHECK_EQ(andOp->op, String("&&"));
}

// ── Bare function call bridge (temporary migration aid) ─────────────────

LTE_TEST(Parser_BareCallSimple) {
  // `foo bar` should parse as function call `foo(bar)`
  ASTNode mod = Parse("foo bar");
  ASTFuncCallNodeT const* call = AsFuncCall(ModuleStmt(mod, 0));
  LTE_CHECK(call != nullptr);
  LTE_CHECK_EQ(call->name, String("foo"));
  LTE_CHECK(call->args.size() == 1);
  ASTIdentifierNodeT const* arg0 = AsIdent(call->args[0]);
  LTE_CHECK(arg0 != nullptr);
  LTE_CHECK_EQ(arg0->name, String("bar"));
}

LTE_TEST(Parser_BareCallMultipleArgs) {
  // `foo a b c` should parse as function call `foo(a, b, c)`
  ASTNode mod = Parse("foo a b c");
  ASTFuncCallNodeT const* call = AsFuncCall(ModuleStmt(mod, 0));
  LTE_CHECK(call != nullptr);
  LTE_CHECK_EQ(call->name, String("foo"));
  LTE_CHECK(call->args.size() == 3);
}

LTE_TEST(Parser_BareCallWithLiterals) {
  // `Vec3 1.0 2.0 3.0` should parse as function call with float args
  ASTNode mod = Parse("Vec3 1.0 2.0 3.0");
  ASTFuncCallNodeT const* call = AsFuncCall(ModuleStmt(mod, 0));
  LTE_CHECK(call != nullptr);
  LTE_CHECK_EQ(call->name, String("Vec3"));
  LTE_CHECK(call->args.size() == 3);
}

LTE_TEST(Parser_BareCallAssignment) {
  // `x = 10` should parse as assignment, NOT bare call
  ASTNode mod = Parse("x = 10");
  ASTAssignNodeT const* a = AsAssign(ModuleStmt(mod, 0));
  LTE_CHECK(a != nullptr);
  ASTIdentifierNodeT const* target = AsIdent(a->target);
  LTE_CHECK(target != nullptr);
  LTE_CHECK_EQ(target->name, String("x"));
}

LTE_TEST(Parser_BareCallInsideVarDecl) {
  // `var x foo` — bare-call bridge only works at statement level.
  // Inside var initializers, ParseExpression is used directly.
  // This test verifies the initializer is just the identifier `foo`.
  ASTNode mod = Parse("var x foo");
  ASTDeclNodeT const* d = AsDecl(ModuleStmt(mod, 0));
  LTE_CHECK(d != nullptr);
  ASTIdentifierNodeT const* init = AsIdent(d->initializer);
  LTE_CHECK(init != nullptr);
  LTE_CHECK_EQ(init->name, String("foo"));
}
