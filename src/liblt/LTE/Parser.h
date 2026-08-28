// Copyright (C) 2025  darkoned12000
// SPDX-License-Identifier: GPL-3.0-or-later
// Part of the ltheory-old-test modernization effort (Revamp Work).
// See NOTICE and LICENSE.GPL. Original engine (c) Josh Parnell, public domain.

// Phase 2: Pratt parser for LTSL.
// Input: Token stream from the Lexer (Phase 1).
// Output: Abstract Syntax Tree (AST.h).

#ifndef LTE_Parser_h__
#define LTE_Parser_h__

#include "AST.h"
#include "Lexer.h"
#include "Vector.h"
#include <vector>

namespace LTE {

struct ParseError {
  String message;
  int line;
  int column;

  ParseError(String const& message, int line, int column) :
    message(message), line(line), column(column) {}
};

class Parser {
public:
  Parser(std::vector<Token> const& tokens) : tokens(tokens), pos(0), bareCallDepth(0), prattCalls(0), curDepth(0), maxDepth(0) {}

  // Parse the full token stream into an AST module.
  ASTNode Parse();

  // Parse a single expression (for testing).
  ASTNode ParseExpression();

  Vector<ParseError> const& GetErrors() const;

  size_t GetPrattCalls() const { return prattCalls; }
  int GetMaxDepth() const { return maxDepth; }

private:
  // Token stream
  std::vector<Token> tokens;
  size_t pos;
  Vector<ParseError> errors;
  int bareCallDepth;  // Prevents recursive bare-call detection
  size_t prattCalls;
  int curDepth;
  int maxDepth;

  // --- Token access ---
  Token const& Peek() const;
  Token const& Peek2() const;
  Token const& Advance();
  bool AtEnd() const;
  bool Check(TokenKind kind) const;
  bool Match(TokenKind kind);
  bool MatchValue(TokenKind kind, String const& value);
  Token const& Expect(TokenKind kind, String const& message);
  Token const& ExpectValue(TokenKind kind, String const& value, String const& message);

  // Skip NEWLINE tokens
  void SkipNewlines();

  // --- Statement parsing ---
  ASTNode ParseStatement();
  ASTNode ParseVarDecl();
  ASTNode ParseRefDecl();
  ASTNode ParseStaticDecl();
  ASTNode ParseFuncDecl();
  ASTNode ParseTypeDecl();
  ASTNode ParseTypeMember();
  ASTNode ParseReturn();
  ASTNode ParseIf();
  ASTNode ParseWhile();
  ASTNode ParseFor();
  ASTNode ParseSwitch();
  ASTNode ParseAssignment(ASTNode const& target);
  ASTNode ParseExprStmt();

  // --- Block parsing ---
  ASTNode ParseBlock();     // INDENT statements DEDENT
  ASTNode ParseDesc();      // desc label block

  // --- Expression parsing (Pratt) ---
  ASTNode ParsePratt(int minPrec);
  ASTNode ParseUnary();
  ASTNode ParsePrimary();
  ASTNode ParsePostfix(ASTNode left);
  ASTNode ParseIdentifier(Token const& tok);

  // Pratt helpers
  static int GetPrefixPrec(TokenKind kind);
  static int GetInfixPrec(TokenKind kind);
  static bool IsInfixRightAssoc(TokenKind kind);
  static String TokenKindToOp(TokenKind kind);

  // --- Type name parsing ---
  // Parses: IDENTIFIER ['/' IDENTIFIER]*
  String ParseTypeName();
  // Parses a type that may be parenthesized '(Array T)' / '(Pointer T)'
  // or a plain identifier path. Consumes exactly one type token group.
  String ParseFieldType();

  // --- Parameter list parsing ---
  void ParseParamList(Vector<String>& paramTypes, Vector<String>& paramNames);

  // --- Argument list parsing ---
  Vector<ASTNode> ParseArgList();

  // --- Error reporting ---
  void ReportError(String const& message);
  void ReportError(Token const& tok, String const& message);

  // --- Source location ---
  SourceLocation CurrentLoc() const;
};

// Top-level parse function: tokenizes then parses.
ASTNode ParseLTSL(String const& source, std::vector<ParseError>* errors = nullptr);

} // namespace LTE

#endif
