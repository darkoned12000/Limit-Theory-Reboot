// Copyright (C) 2025  darkoned12000
// SPDX-License-Identifier: GPL-3.0-or-later
// Part of the ltheory-old-test modernization effort (Revamp Work).
// See NOTICE and LICENSE.GPL. Original engine (c) Josh Parnell, public domain.
//
// Unit tests for the new LTSL Lexer (Phase 1 of the compiler rewrite).

#include "Harness.h"
#include "LTE/Lexer.h"

using namespace LTE;

// ── Helpers ────────────────────────────────────────────────────────────

// Tokenize source and return tokens (ignoring errors for convenience).
static Vector<Token> Lex(String const& src) {
  Lexer lexer(src);
  return lexer.Tokenize();
}

// Count tokens of a given kind.
static int Count(Vector<Token> const& toks, TokenKind kind) {
  int n = 0;
  for (size_t i = 0; i < toks.size(); i++)
    if (toks[i].kind == kind)
      n++;
  return n;
}

// Find first token of a given kind; return nullptr if none.
static Token const* FindFirst(Vector<Token> const& toks, TokenKind kind) {
  for (size_t i = 0; i < toks.size(); i++)
    if (toks[i].kind == kind)
      return &toks[i];
  return nullptr;
}

// ── Basic token types ──────────────────────────────────────────────────

LTE_TEST(Lexer_IntLiteral) {
  auto toks = Lex("42");
  auto* t = FindFirst(toks, TOK_INT);
  LTE_CHECK(t != nullptr);
  LTE_CHECK_EQ(t->value, String("42"));
}

LTE_TEST(Lexer_FloatLiteral) {
  auto toks = Lex("3.14");
  auto* t = FindFirst(toks, TOK_FLOAT);
  LTE_CHECK(t != nullptr);
  LTE_CHECK_EQ(t->value, String("3.14"));
}

LTE_TEST(Lexer_StringLiteral) {
  auto toks = Lex("\"hello world\"");
  auto* t = FindFirst(toks, TOK_STRING);
  LTE_CHECK(t != nullptr);
  LTE_CHECK_EQ(t->value, String("hello world"));
}

LTE_TEST(Lexer_SingleQuotedString) {
  auto toks = Lex("'abc def'");
  auto* t = FindFirst(toks, TOK_STRING);
  LTE_CHECK(t != nullptr);
  LTE_CHECK_EQ(t->value, String("abc def"));
}

LTE_TEST(Lexer_BoolLiterals) {
  auto toks = Lex("true false");
  LTE_CHECK_EQ(Count(toks, TOK_TRUE), 1);
  LTE_CHECK_EQ(Count(toks, TOK_FALSE), 1);
}

LTE_TEST(Lexer_NullLiteral) {
  auto toks = Lex("null");
  // null is TOK_NULL if we support it, or TOK_IDENTIFIER if not yet
  LTE_CHECK(FindFirst(toks, TOK_NULL) != nullptr || FindFirst(toks, TOK_IDENTIFIER) != nullptr);
}

// ── Identifiers and keywords ───────────────────────────────────────────

LTE_TEST(Lexer_Identifier) {
  auto toks = Lex("myVarName");
  auto* t = FindFirst(toks, TOK_IDENTIFIER);
  LTE_CHECK(t != nullptr);
  LTE_CHECK_EQ(t->value, String("myVarName"));
}

LTE_TEST(Lexer_Keywords) {
  auto toks = Lex("var ref static function return if else while for in switch otherwise break continue type cast block desc address deref");
  LTE_CHECK_EQ(Count(toks, TOK_VAR), 1);
  LTE_CHECK_EQ(Count(toks, TOK_REF), 1);
  LTE_CHECK_EQ(Count(toks, TOK_STATIC), 1);
  LTE_CHECK_EQ(Count(toks, TOK_FUNCTION), 1);
  LTE_CHECK_EQ(Count(toks, TOK_RETURN), 1);
  LTE_CHECK_EQ(Count(toks, TOK_IF), 1);
  LTE_CHECK_EQ(Count(toks, TOK_ELSE), 1);
  LTE_CHECK_EQ(Count(toks, TOK_WHILE), 1);
  LTE_CHECK_EQ(Count(toks, TOK_FOR), 1);
  LTE_CHECK_EQ(Count(toks, TOK_IN), 1);
  LTE_CHECK_EQ(Count(toks, TOK_SWITCH), 1);
  LTE_CHECK_EQ(Count(toks, TOK_OTHERWISE), 1);
  LTE_CHECK_EQ(Count(toks, TOK_BREAK), 1);
  LTE_CHECK_EQ(Count(toks, TOK_CONTINUE), 1);
  LTE_CHECK_EQ(Count(toks, TOK_TYPE), 1);
  LTE_CHECK_EQ(Count(toks, TOK_CAST), 1);
  LTE_CHECK_EQ(Count(toks, TOK_BLOCK), 1);
  LTE_CHECK_EQ(Count(toks, TOK_DESC), 1);
  LTE_CHECK_EQ(Count(toks, TOK_ADDRESS), 1);
  LTE_CHECK_EQ(Count(toks, TOK_DEREF), 1);
}

// ── Operators ──────────────────────────────────────────────────────────

LTE_TEST(Lexer_ArithmeticOperators) {
  auto toks = Lex("+ - * / %");
  LTE_CHECK_EQ(Count(toks, TOK_PLUS), 1);
  LTE_CHECK_EQ(Count(toks, TOK_MINUS), 1);
  LTE_CHECK_EQ(Count(toks, TOK_STAR), 1);
  LTE_CHECK_EQ(Count(toks, TOK_SLASH), 1);
  LTE_CHECK_EQ(Count(toks, TOK_MOD), 1);
}

LTE_TEST(Lexer_ComparisonOperators) {
  auto toks = Lex("== != < > <= >=");
  LTE_CHECK_EQ(Count(toks, TOK_EQUALS), 1);
  LTE_CHECK_EQ(Count(toks, TOK_NOT_EQUALS), 1);
  LTE_CHECK_EQ(Count(toks, TOK_LESS), 1);
  LTE_CHECK_EQ(Count(toks, TOK_GREATER), 1);
  LTE_CHECK_EQ(Count(toks, TOK_LESS_EQUALS), 1);
  LTE_CHECK_EQ(Count(toks, TOK_GREATER_EQUALS), 1);
}

LTE_TEST(Lexer_LogicalOperators) {
  auto toks = Lex("&& || !");
  LTE_CHECK_EQ(Count(toks, TOK_AND), 1);
  LTE_CHECK_EQ(Count(toks, TOK_OR), 1);
  LTE_CHECK_EQ(Count(toks, TOK_NOT), 1);
}

LTE_TEST(Lexer_AssignmentOperators) {
  auto toks = Lex("= += -= *= /=");
  LTE_CHECK_EQ(Count(toks, TOK_ASSIGN), 1);
  LTE_CHECK_EQ(Count(toks, TOK_PLUS_ASSIGN), 1);
  LTE_CHECK_EQ(Count(toks, TOK_MINUS_ASSIGN), 1);
  LTE_CHECK_EQ(Count(toks, TOK_MULTIPLY_ASSIGN), 1);
  LTE_CHECK_EQ(Count(toks, TOK_DIVIDE_ASSIGN), 1);
}

// ── Delimiters ─────────────────────────────────────────────────────────

LTE_TEST(Lexer_Delimiters) {
  auto toks = Lex("( ) [ ] , : .");
  LTE_CHECK_EQ(Count(toks, TOK_LPAREN), 1);
  LTE_CHECK_EQ(Count(toks, TOK_RPAREN), 1);
  LTE_CHECK_EQ(Count(toks, TOK_LBRACKET), 1);
  LTE_CHECK_EQ(Count(toks, TOK_RBRACKET), 1);
  LTE_CHECK_EQ(Count(toks, TOK_COMMA), 1);
  LTE_CHECK_EQ(Count(toks, TOK_COLON), 1);
  LTE_CHECK_EQ(Count(toks, TOK_DOT), 1);
}

// ── @ debug print ─────────────────────────────────────────────────────

LTE_TEST(Lexer_AtOperator) {
  auto toks = Lex("@ myVar");
  LTE_CHECK_EQ(Count(toks, TOK_AT), 1);
  LTE_CHECK_EQ(Count(toks, TOK_IDENTIFIER), 1);
}

// ── Comments ───────────────────────────────────────────────────────────

LTE_TEST(Lexer_SingleLineComment) {
  auto toks = Lex("# this is a comment\n42");
  LTE_CHECK_EQ(Count(toks, TOK_INT), 1);
  // No comment token should appear
  LTE_CHECK_EQ(Count(toks, TOK_UNKNOWN), 0);
}

LTE_TEST(Lexer_TrailingComment) {
  auto toks = Lex("42 # comment");
  LTE_CHECK_EQ(Count(toks, TOK_INT), 1);
}

// ── Newlines ───────────────────────────────────────────────────────────

LTE_TEST(Lexer_Newlines) {
  auto toks = Lex("a\nb\nc");
  LTE_CHECK_EQ(Count(toks, TOK_NEWLINE), 2);
}

LTE_TEST(Lexer_BlankLinesSkipped) {
  auto toks = Lex("a\n\n\nb");
  // Blank lines should not produce extra newlines
  LTE_CHECK_EQ(Count(toks, TOK_NEWLINE), 1);
}

// ── Indentation ────────────────────────────────────────────────────────

LTE_TEST(Lexer_SimpleIndent) {
  auto toks = Lex("if true\n  stmt1\n  stmt2");
  LTE_CHECK(FindFirst(toks, TOK_INDENT) != nullptr);
  LTE_CHECK(FindFirst(toks, TOK_DEDENT) != nullptr);
}

LTE_TEST(Lexer_NestedIndent) {
  auto toks = Lex("if true\n  if false\n    inner\n  outer");
  int indents = Count(toks, TOK_INDENT);
  int dedents = Count(toks, TOK_DEDENT);
  LTE_CHECK(indents >= 2);
  LTE_CHECK(dedents >= 2);
}

LTE_TEST(Lexer_MismatchedDedent) {
  Lexer lexer("if true\n  stmt1\n stmt2");
  auto toks = lexer.Tokenize();
  // Should produce an error for unindent not matching
  LTE_CHECK(lexer.GetErrors().size() > 0);
}

LTE_TEST(Lexer_TabsAsIndent) {
  auto toks = Lex("if true\n\tstmt1");
  LTE_CHECK(FindFirst(toks, TOK_INDENT) != nullptr);
}

// ── Multi-line paren groups ────────────────────────────────────────────

LTE_TEST(Lexer_MultiLineParens) {
  auto toks = Lex("var x (a +\n      b)");
  // Inside parens, no NEWLINE should be emitted
  LTE_CHECK_EQ(Count(toks, TOK_NEWLINE), 0);
  // All tokens should be present
  LTE_CHECK(FindFirst(toks, TOK_VAR) != nullptr);
  LTE_CHECK(FindFirst(toks, TOK_LPAREN) != nullptr);
  LTE_CHECK(FindFirst(toks, TOK_RPAREN) != nullptr);
}

LTE_TEST(Lexer_NestedParens) {
  auto toks = Lex("(a (b +\n       c))");
  // Still no newlines inside parens
  LTE_CHECK_EQ(Count(toks, TOK_NEWLINE), 0);
  LTE_CHECK_EQ(Count(toks, TOK_LPAREN), 2);
  LTE_CHECK_EQ(Count(toks, TOK_RPAREN), 2);
}

// ── Postfix operators (.! .++ .--) ────────────────────────────────────

LTE_TEST(Lexer_PostfixNot) {
  auto toks = Lex("paused.!");
  // In the new lexer, postfix .! is separate tokens: ident + dot + not
  // The parser will handle the rewriting
  LTE_CHECK_EQ(Count(toks, TOK_IDENTIFIER), 1);
  LTE_CHECK_EQ(Count(toks, TOK_DOT), 1);
  LTE_CHECK_EQ(Count(toks, TOK_NOT), 1);
}

LTE_TEST(Lexer_PostfixIncrement) {
  auto toks = Lex("i.++");
  // Postfix .++ is separate tokens: ident + dot + plus + plus
  LTE_CHECK_EQ(Count(toks, TOK_IDENTIFIER), 1);
  LTE_CHECK_EQ(Count(toks, TOK_DOT), 1);
  LTE_CHECK_EQ(Count(toks, TOK_PLUS), 2);
}

// ── Complex expressions ────────────────────────────────────────────────

LTE_TEST(Lexer_VarDecl) {
  auto toks = Lex("var x 42");
  LTE_CHECK_EQ(Count(toks, TOK_VAR), 1);
  LTE_CHECK_EQ(Count(toks, TOK_IDENTIFIER), 1);
  LTE_CHECK_EQ(Count(toks, TOK_INT), 1);
}

LTE_TEST(Lexer_FunctionDecl) {
  auto toks = Lex("function Int Add (Int a Int b)");
  LTE_CHECK_EQ(Count(toks, TOK_FUNCTION), 1);
  // "Int" is a type name (TOK_IDENTIFIER), not a keyword
  // function, Int, Add, (, Int, a, Int, b, ) → 6 identifiers
  LTE_CHECK_EQ(Count(toks, TOK_IDENTIFIER), 6);
  LTE_CHECK(FindFirst(toks, TOK_LPAREN) != nullptr);
  LTE_CHECK(FindFirst(toks, TOK_RPAREN) != nullptr);
}

LTE_TEST(Lexer_ColonPath) {
  auto toks = Lex("Widget/Components:AlignCenter");
  // Colon is a standalone delimiter
  LTE_CHECK_EQ(Count(toks, TOK_COLON), 1);
  LTE_CHECK(FindFirst(toks, TOK_SLASH) != nullptr);
}

// ── Line/column tracking ───────────────────────────────────────────────

LTE_TEST(Lexer_LineTracking) {
  auto toks = Lex("a\nb\n  c");
  // First identifier 'a' should be line 1
  auto* t = FindFirst(toks, TOK_IDENTIFIER);
  LTE_CHECK(t != nullptr);
  LTE_CHECK_EQ(t->line, 1);
}

LTE_TEST(Lexer_ColumnTracking) {
  auto toks = Lex("  var x 10");
  auto* t = FindFirst(toks, TOK_VAR);
  LTE_CHECK(t != nullptr);
  LTE_CHECK_EQ(t->column, 3); // 1-based, after 2 spaces
}

// ── Error cases ────────────────────────────────────────────────────────

LTE_TEST(Lexer_UnterminatedString) {
  Lexer lexer("var x \"hello");
  auto toks = lexer.Tokenize();
  LTE_CHECK(lexer.GetErrors().size() > 0);
}

LTE_TEST(Lexer_UnknownCharacter) {
  Lexer lexer("var x @");
  auto toks = lexer.Tokenize();
  // @ is valid, so no error
  LTE_CHECK_EQ(lexer.GetErrors().size(), 0);
}

// ── EOF handling ───────────────────────────────────────────────────────

LTE_TEST(Lexer_EOF) {
  auto toks = Lex("42");
  LTE_CHECK_EQ(toks.back().kind, TOK_EOF);
}

LTE_TEST(Lexer_EmptySource) {
  auto toks = Lex("");
  LTE_CHECK_EQ(toks.size(), 1); // just EOF
  LTE_CHECK_EQ(toks[0].kind, TOK_EOF);
}

LTE_TEST(Lexer_OnlyWhitespace) {
  auto toks = Lex("   \t  ");
  LTE_CHECK_EQ(toks.size(), 1); // just EOF
  LTE_CHECK_EQ(toks[0].kind, TOK_EOF);
}

LTE_TEST(Lexer_OnlyComments) {
  auto toks = Lex("# comment 1\n# comment 2\n# comment 3");
  LTE_CHECK_EQ(toks.size(), 1); // just EOF
  LTE_CHECK_EQ(toks[0].kind, TOK_EOF);
}
