// Copyright (C) 2025  darkoned12000
// SPDX-License-Identifier: GPL-3.0-or-later
// Part of the ltheory-old-test modernization effort (Revamp Work).
// See NOTICE and LICENSE.GPL. Original engine (c) Josh Parnell, public domain.

// Phase 2: Pratt parser for LTSL.
// Builds an AST from the token stream produced by the Lexer (Phase 1).

#include "Parser.h"
#include "Lexer.h"

#include <cstdlib>
#include <cstring>

namespace LTE {

// ============================================================================
// Token access
// ============================================================================

Token const& Parser::Peek() const {
  static const Token eof(TOK_EOF, "", 0, 0, 0);
  return pos < tokens.size() ? tokens[pos] : eof;
}

Token const& Parser::Peek2() const {
  static const Token eof(TOK_EOF, "", 0, 0, 0);
  return (pos + 1) < tokens.size() ? tokens[pos + 1] : eof;
}

Token const& Parser::Advance() {
  Token const& tok = Peek();
  if (pos < tokens.size())
    pos++;
  return tok;
}

bool Parser::AtEnd() const {
  return pos >= tokens.size() || Peek().kind == TOK_EOF;
}

bool Parser::Check(TokenKind kind) const {
  return Peek().kind == kind;
}

bool Parser::Match(TokenKind kind) {
  if (Check(kind)) {
    Advance();
    return true;
  }
  return false;
}

bool Parser::MatchValue(TokenKind kind, String const& value) {
  if (Check(kind) && Peek().value == value) {
    Advance();
    return true;
  }
  return false;
}

Token const& Parser::Expect(TokenKind kind, String const& message) {
  if (Check(kind))
    return Advance();
  ReportError(Peek(), message);
  // Return the current token (not advanced) so callers don't crash
  return Peek();
}

Token const& Parser::ExpectValue(
  TokenKind kind, String const& value, String const& message)
{
  if (Check(kind) && Peek().value == value)
    return Advance();
  ReportError(Peek(), message);
  return Peek();
}

void Parser::SkipNewlines() {
  while (Match(TOK_NEWLINE))
    ;
}

SourceLocation Parser::CurrentLoc() const {
  Token const& tok = Peek();
  return SourceLocation(tok.line, tok.column);
}

// ============================================================================
// Error reporting
// ============================================================================

void Parser::ReportError(String const& message) {
  Token const& tok = Peek();
  errors.push(ParseError(message, tok.line, tok.column));
}

void Parser::ReportError(Token const& tok, String const& message) {
  errors.push(ParseError(message, tok.line, tok.column));
}

// ============================================================================
// Pratt precedence helpers
// ============================================================================

int Parser::GetPrefixPrec(TokenKind kind) {
  switch (kind) {
    case TOK_MINUS:
    case TOK_NOT:
      return 8;
    default:
      return -1;
  }
}

int Parser::GetInfixPrec(TokenKind kind) {
  switch (kind) {
    case TOK_STAR:
    case TOK_SLASH:
    case TOK_MOD:
      return 7;
    case TOK_PLUS:
    case TOK_MINUS:
      return 6;
    case TOK_LESS:
    case TOK_GREATER:
    case TOK_LESS_EQUALS:
    case TOK_GREATER_EQUALS:
      return 5;
    case TOK_EQUALS:
    case TOK_NOT_EQUALS:
      return 4;
    case TOK_AND:
      return 3;
    case TOK_OR:
      return 2;
    case TOK_ASSIGN:
    case TOK_PLUS_ASSIGN:
    case TOK_MINUS_ASSIGN:
    case TOK_MULTIPLY_ASSIGN:
    case TOK_DIVIDE_ASSIGN:
      return 1;
    default:
      return -1;
  }
}

bool Parser::IsInfixRightAssoc(TokenKind kind) {
  // Assignment operators are right-associative; everything else is left
  return kind == TOK_ASSIGN || kind == TOK_PLUS_ASSIGN ||
         kind == TOK_MINUS_ASSIGN || kind == TOK_MULTIPLY_ASSIGN ||
         kind == TOK_DIVIDE_ASSIGN;
}

String Parser::TokenKindToOp(TokenKind kind) {
  switch (kind) {
    case TOK_PLUS:             return "+";
    case TOK_MINUS:            return "-";
    case TOK_STAR:             return "*";
    case TOK_SLASH:            return "/";
    case TOK_MOD:              return "%";
    case TOK_EQUALS:           return "==";
    case TOK_NOT_EQUALS:       return "!=";
    case TOK_LESS:             return "<";
    case TOK_GREATER:          return ">";
    case TOK_LESS_EQUALS:      return "<=";
    case TOK_GREATER_EQUALS:   return ">=";
    case TOK_AND:              return "&&";
    case TOK_OR:               return "||";
    case TOK_ASSIGN:           return "=";
    case TOK_PLUS_ASSIGN:      return "+=";
    case TOK_MINUS_ASSIGN:     return "-=";
    case TOK_MULTIPLY_ASSIGN:  return "*=";
    case TOK_DIVIDE_ASSIGN:    return "/=";
    default:                   return "?";
  }
}

// ============================================================================
// Type name parsing
// ============================================================================

// typeName = IDENTIFIER ['/' IDENTIFIER]*
// Note: generics (<T>) are NOT handled in the parser — they don't appear
// in LTSL function signatures (the engine's type system is monomorphic).
String Parser::ParseTypeName() {
  Token const& name = Expect(TOK_IDENTIFIER, "expected type name");
  String result = name.value;

  // Handle namespace paths: Vec3/Int → "Vec3/Int"
  while (Match(TOK_COLON)) {
    // COLON is used as path separator in LTSL: "Script:function"
    Token const& part = Expect(TOK_IDENTIFIER, "expected name after ':'");
    result += String(":") + part.value;
  }

  return result;
}

// ============================================================================
// Parameter list parsing
// ============================================================================

// paramList = [typeName IDENTIFIER (typeName IDENTIFIER)*]
// LTSL uses space-separated params — no commas in function signatures.
void Parser::ParseParamList(
  Vector<String>& paramTypes, Vector<String>& paramNames)
{
  while (!Check(TOK_RPAREN) && !AtEnd()) {
    // Type name
    Token const& typeName = Expect(TOK_IDENTIFIER, "expected parameter type");
    Token const& paramName = Expect(TOK_IDENTIFIER, "expected parameter name");
    paramTypes.push(typeName.value);
    paramNames.push(paramName.value);
  }
}

// ============================================================================
// Argument list parsing
// ============================================================================

// argList = [expr (',' expr)*]
Vector<ASTNode> Parser::ParseArgList() {
  Vector<ASTNode> args;
  if (!Check(TOK_RPAREN)) {
    do {
      args.push(ParseExpression());
    } while (Match(TOK_COMMA));
  }
  return args;
}

// ============================================================================
// Block parsing
// ============================================================================

// block = INDENT statement+ DEDENT
ASTNode Parser::ParseBlock() {
  if (!Match(TOK_INDENT)) {
    ReportError("expected indented block");
    // Error recovery: parse a single statement
    ASTNode stmt = ParseStatement();
    if (stmt)
      return stmt;
    return AST_MakeNoop();
  }

  ASTBlockNodeT* block = new ASTBlockNodeT();
  block->loc = CurrentLoc();
  block->isDesc = false;

  SkipNewlines();
  while (!AtEnd() && !Check(TOK_DEDENT)) {
    ASTNode stmt = ParseStatement();
    if (stmt)
      block->statements.push(stmt);
    SkipNewlines();
  }

  if (!Match(TOK_DEDENT)) {
    ReportError("unterminated block (missing dedent)");
  }

  return block;
}

// desc "label" block
ASTNode Parser::ParseDesc() {
  Token const& descTok = Advance();  // consume 'desc'

  ASTBlockNodeT* block = new ASTBlockNodeT();
  block->loc = SourceLocation(descTok.line, descTok.column);
  block->isDesc = true;

  // The label — typically a string literal, but could be any expression
  if (Check(TOK_STRING)) {
    block->label = Peek().value;
    Advance();
  } else if (Check(TOK_IDENTIFIER)) {
    block->label = Peek().value;
    Advance();
  }

  SkipNewlines();
  if (!Match(TOK_INDENT)) {
    ReportError("expected indented block after desc");
    ASTNode stmt = ParseStatement();
    if (stmt)
      block->statements.push(stmt);
    return block;
  }

  SkipNewlines();
  while (!AtEnd() && !Check(TOK_DEDENT)) {
    ASTNode stmt = ParseStatement();
    if (stmt)
      block->statements.push(stmt);
    SkipNewlines();
  }

  if (!Match(TOK_DEDENT)) {
    ReportError("unterminated desc block (missing dedent)");
  }

  return block;
}

// ============================================================================
// Statement parsing
// ============================================================================

ASTNode Parser::ParseStatement() {
  SkipNewlines();
  if (AtEnd() || Check(TOK_DEDENT))
    return nullptr;

  Token const& tok = Peek();

  // Declarations
  if (tok.kind == TOK_VAR)     return ParseVarDecl();
  if (tok.kind == TOK_REF)     return ParseRefDecl();
  if (tok.kind == TOK_STATIC)  return ParseStaticDecl();
  if (tok.kind == TOK_FUNCTION) return ParseFuncDecl();
  if (tok.kind == TOK_TYPE)    return ParseTypeDecl();

  // Control flow
  if (tok.kind == TOK_RETURN)  return ParseReturn();
  if (tok.kind == TOK_IF)      return ParseIf();
  if (tok.kind == TOK_WHILE)   return ParseWhile();
  if (tok.kind == TOK_FOR)     return ParseFor();
  if (tok.kind == TOK_SWITCH)  return ParseSwitch();

  // Break
  if (tok.kind == TOK_BREAK) {
    Advance();
    ASTNodeT* node = new ASTNodeT(AST_BREAK);
    node->loc = SourceLocation(tok.line, tok.column);
    return node;
  }

  // desc / block — block expressions used as statements
  if (tok.kind == TOK_DESC)    return ParseDesc();
  if (tok.kind == TOK_BLOCK) {
    Advance();
    SkipNewlines();
    return ParseBlock();
  }

  // @ debug print
  if (tok.kind == TOK_AT) {
    Advance();
    ASTPrintNodeT* node = new ASTPrintNodeT();
    node->loc = SourceLocation(tok.line, tok.column);
    node->operand = ParseExpression();
    return node;
  }

  // Try to parse as expression — might be assignment or expr statement
  ASTNode expr = ParseExpression();
  if (!expr) {
    // Error recovery: advance past the unrecognized token to prevent infinite loop
    ReportError(Peek(), "unexpected token");
    Advance();
    return nullptr;
  }

  // Check for assignment: expr = expr, expr += expr, etc.
  if (Check(TOK_ASSIGN) || Check(TOK_PLUS_ASSIGN) ||
      Check(TOK_MINUS_ASSIGN) || Check(TOK_MULTIPLY_ASSIGN) ||
      Check(TOK_DIVIDE_ASSIGN)) {
    return ParseAssignment(expr);
  }

  // Expression statement
  ASTExprStmtNodeT* stmt = new ASTExprStmtNodeT();
  stmt->loc = expr->loc;
  stmt->expression = expr;
  return stmt;
}

// var name expr
ASTNode Parser::ParseVarDecl() {
  Token const& varTok = Advance();  // consume 'var'

  ASTDeclNodeT* node = new ASTDeclNodeT();
  node->kind = AST_VAR_DECL;
  node->loc = SourceLocation(varTok.line, varTok.column);

  Token const& name = Expect(TOK_IDENTIFIER, "expected variable name");
  node->name = name.value;

  SkipNewlines();
  node->initializer = ParseExpression();
  if (!node->initializer) {
    ReportError("expected initializer expression");
  }

  return node;
}

// ref name expr
ASTNode Parser::ParseRefDecl() {
  Token const& refTok = Advance();  // consume 'ref'

  ASTDeclNodeT* node = new ASTDeclNodeT();
  node->kind = AST_REF_DECL;
  node->loc = SourceLocation(refTok.line, refTok.column);

  Token const& name = Expect(TOK_IDENTIFIER, "expected reference name");
  node->name = name.value;

  SkipNewlines();
  node->initializer = ParseExpression();
  if (!node->initializer) {
    ReportError("expected initializer expression");
  }

  return node;
}

// static name expr
ASTNode Parser::ParseStaticDecl() {
  Token const& staticTok = Advance();  // consume 'static'

  ASTDeclNodeT* node = new ASTDeclNodeT();
  node->kind = AST_STATIC_DECL;
  node->loc = SourceLocation(staticTok.line, staticTok.column);

  Token const& name = Expect(TOK_IDENTIFIER, "expected static variable name");
  node->name = name.value;

  SkipNewlines();
  node->initializer = ParseExpression();
  if (!node->initializer) {
    ReportError("expected initializer expression");
  }

  return node;
}

// function RetType Name (Type1 p1, Type2 p2, ...) body
ASTNode Parser::ParseFuncDecl() {
  Token const& funcTok = Advance();  // consume 'function'

  ASTFuncDeclNodeT* node = new ASTFuncDeclNodeT();
  node->loc = SourceLocation(funcTok.line, funcTok.column);

  // Return type
  node->returnType = ParseTypeName();

  // Function name
  Token const& name = Expect(TOK_IDENTIFIER, "expected function name");
  node->name = name.value;

  // Parameter list
  Expect(TOK_LPAREN, "expected '(' after function name");
  SkipNewlines();
  ParseParamList(node->paramTypes, node->paramNames);
  SkipNewlines();
  Expect(TOK_RPAREN, "expected ')' after parameters");

  // Body
  SkipNewlines();
  node->body = ParseBlock();

  return node;
}

// type Name
//   Type1 field1
//   Type2 field2
//   function RetType Method (params) body
ASTNode Parser::ParseTypeDecl() {
  Token const& typeTok = Advance();  // consume 'type'

  ASTTypeDeclNodeT* node = new ASTTypeDeclNodeT();
  node->loc = SourceLocation(typeTok.line, typeTok.column);

  Token const& name = Expect(TOK_IDENTIFIER, "expected type name");
  node->name = name.value;

  SkipNewlines();
  if (!Match(TOK_INDENT)) {
    ReportError("expected indented type body");
    return node;
  }

  SkipNewlines();
  while (!AtEnd() && !Check(TOK_DEDENT)) {
    ASTNode member = ParseStatement();
    if (member)
      node->members.push(member);
    SkipNewlines();
  }

  if (!Match(TOK_DEDENT)) {
    ReportError("unterminated type body (missing dedent)");
  }

  return node;
}

// return [expr]
ASTNode Parser::ParseReturn() {
  Token const& retTok = Advance();  // consume 'return'

  ASTReturnNodeT* node = new ASTReturnNodeT();
  node->loc = SourceLocation(retTok.line, retTok.column);

  // return can be bare (returns nothing) or followed by an expression
  if (!AtEnd() && !Check(TOK_NEWLINE) && !Check(TOK_DEDENT) && !Check(TOK_EOF)) {
    node->value = ParseExpression();
  }

  return node;
}

// if expr block ['else' (ifStmt | block)]
ASTNode Parser::ParseIf() {
  Token const& ifTok = Advance();  // consume 'if'

  ASTIfNodeT* node = new ASTIfNodeT();
  node->loc = SourceLocation(ifTok.line, ifTok.column);

  node->condition = ParseExpression();
  if (!node->condition) {
    ReportError("expected condition expression");
  }

  SkipNewlines();
  node->thenBlock = ParseBlock();

  // Optional else
  SkipNewlines();
  if (Match(TOK_ELSE)) {
    SkipNewlines();
    if (Check(TOK_IF)) {
      // else if — nest as the else branch
      node->elseBlock = ParseIf();
    } else {
      node->elseBlock = ParseBlock();
    }
  }

  return node;
}

// while expr block
ASTNode Parser::ParseWhile() {
  Token const& whileTok = Advance();  // consume 'while'

  ASTWhileNodeT* node = new ASTWhileNodeT();
  node->loc = SourceLocation(whileTok.line, whileTok.column);

  node->condition = ParseExpression();
  if (!node->condition) {
    ReportError("expected condition expression");
  }

  SkipNewlines();
  node->body = ParseBlock();

  return node;
}

// for name init pred step body
// The old interpreter form: (for name init pred step body...)
// The parser reads: for IDENTIFIER expr expr expr block
// Also handles: for i in range start end body (sugar)
ASTNode Parser::ParseFor() {
  Token const& forTok = Advance();  // consume 'for'

  ASTForNodeT* node = new ASTForNodeT();
  node->loc = SourceLocation(forTok.line, forTok.column);

  // Iterator variable name
  Token const& name = Expect(TOK_IDENTIFIER, "expected loop variable name");
  node->iteratorName = name.value;

  SkipNewlines();

  // Check for 'in' sugar form: for i in expr expr
  if (Match(TOK_IN)) {
    // Sugar form: for i in start end body
    // Desugar to: var i start; while i < end; body; i.++
    // For now, just parse as: init=none, condition=in-expr, step=none
    // The desugaring happens in the resolver or codegen phase.
    node->init = nullptr;
    node->condition = ParseExpression();
    node->step = nullptr;
    SkipNewlines();
    node->body = ParseBlock();
    return node;
  }

  // Standard form: for name init pred step body
  node->init = ParseExpression();
  SkipNewlines();
  node->condition = ParseExpression();
  SkipNewlines();
  node->step = ParseExpression();
  SkipNewlines();
  node->body = ParseBlock();

  return node;
}

// switch INDENT switchCase+ ('otherwise' block)? DEDENT
// switchCase = expr block
ASTNode Parser::ParseSwitch() {
  Token const& switchTok = Advance();  // consume 'switch'

  ASTSwitchNodeT* node = new ASTSwitchNodeT();
  node->loc = SourceLocation(switchTok.line, switchTok.column);

  SkipNewlines();
  node->expression = ParseExpression();
  SkipNewlines();
  if (!Match(TOK_INDENT)) {
    ReportError("expected indented switch body");
    return node;
  }

  SkipNewlines();
  while (!AtEnd() && !Check(TOK_DEDENT)) {
    // Check for 'otherwise' — the default case
    if (Match(TOK_OTHERWISE)) {
      SkipNewlines();
      node->otherwise = ParseBlock();
      SkipNewlines();
      continue;
    }

    // Regular case: expr block
    ASTSwitchCase sc;
    sc.condition = ParseExpression();
    SkipNewlines();
    sc.body = ParseBlock();
    node->cases.push(sc);
    SkipNewlines();
  }

  if (!Match(TOK_DEDENT)) {
    ReportError("unterminated switch body (missing dedent)");
  }

  return node;
}

// lvalue op expr  where op is = += -= *= /=
ASTNode Parser::ParseAssignment(ASTNode const& target) {
  Token const& opTok = Advance();  // consume the operator

  ASTAssignNodeT* node = new ASTAssignNodeT();
  node->loc = target->loc;
  node->target = target;
  node->op = opTok.value;

  SkipNewlines();
  node->value = ParseExpression();
  if (!node->value) {
    ReportError("expected expression on right side of assignment");
  }

  return node;
}

// ============================================================================
// Expression parsing (Pratt)
// ============================================================================

ASTNode Parser::ParseExpression() {
  return ParsePratt(0);
}

ASTNode Parser::ParsePratt(int minPrec) {
  ASTNode left = ParseUnary();
  if (!left)
    return nullptr;

  for (;;) {
    Token const& tok = Peek();
    int prec = GetInfixPrec(tok.kind);

    // Stop if not an infix operator or precedence too low
    if (prec < minPrec)
      break;

    // For right-associative operators, parse at one higher precedence
    int nextPrec = IsInfixRightAssoc(tok.kind) ? prec + 1 : prec + 1;

    // Method call / dot access: handle specially (not via Pratt)
    if (tok.kind == TOK_DOT) {
      left = ParsePostfix(left);
      continue;
    }

    // Assignment operators — handled by ParseStatement, not in expression Pratt
    if (tok.kind == TOK_ASSIGN || tok.kind == TOK_PLUS_ASSIGN ||
        tok.kind == TOK_MINUS_ASSIGN || tok.kind == TOK_MULTIPLY_ASSIGN ||
        tok.kind == TOK_DIVIDE_ASSIGN) {
      break;
    }

    // Binary operator
    String op = TokenKindToOp(tok.kind);
    SourceLocation opLoc(tok.line, tok.column);
    Advance();

    ASTNode right = ParsePratt(nextPrec);
    if (!right) {
      ReportError("expected expression after operator");
      return left;
    }

    ASTBinaryOpNodeT* binop = new ASTBinaryOpNodeT();
    binop->loc = left->loc;
    binop->left = left;
    binop->op = op;
    binop->right = right;
    left = binop;
  }

  return left;
}

ASTNode Parser::ParseUnary() {
  Token const& tok = Peek();

  // Prefix operators: - and !
  if (tok.kind == TOK_MINUS || tok.kind == TOK_NOT) {
    Advance();
    ASTNode operand = ParseUnary();
    if (!operand) {
      ReportError("expected expression after unary operator");
      return nullptr;
    }

    ASTUnaryOpNodeT* node = new ASTUnaryOpNodeT();
    node->loc = SourceLocation(tok.line, tok.column);
    node->op = (tok.kind == TOK_MINUS) ? "-" : "!";
    node->operand = operand;
    return node;
  }

  return ParsePrimary();
}

ASTNode Parser::ParsePrimary() {
  Token const& tok = Peek();

  // --- Literals ---
  if (tok.kind == TOK_INT) {
    Advance();
    ASTIntLiteralNodeT* node = new ASTIntLiteralNodeT();
    node->loc = SourceLocation(tok.line, tok.column);
    node->value = std::strtoll(tok.value.c_str(), nullptr, 10);
    return node;
  }

  if (tok.kind == TOK_FLOAT) {
    Advance();
    ASTFloatLiteralNodeT* node = new ASTFloatLiteralNodeT();
    node->loc = SourceLocation(tok.line, tok.column);
    node->value = std::strtod(tok.value.c_str(), nullptr);
    return node;
  }

  if (tok.kind == TOK_STRING) {
    Advance();
    ASTStringLiteralNodeT* node = new ASTStringLiteralNodeT();
    node->loc = SourceLocation(tok.line, tok.column);
    node->value = tok.value;
    return node;
  }

  if (tok.kind == TOK_TRUE || tok.kind == TOK_FALSE) {
    Advance();
    ASTBoolLiteralNodeT* node = new ASTBoolLiteralNodeT();
    node->loc = SourceLocation(tok.line, tok.column);
    node->value = (tok.kind == TOK_TRUE);
    return node;
  }

  if (tok.kind == TOK_NULL) {
    Advance();
    ASTNullLiteralNodeT* node = new ASTNullLiteralNodeT();
    node->loc = SourceLocation(tok.line, tok.column);
    return node;
  }

  // --- Parenthesized expression or function call ---
  if (tok.kind == TOK_LPAREN) {
    Advance();
    SkipNewlines();

    // Check for function call: (name arg1 arg2 ...)
    // If first expression is an identifier and more tokens follow before ')',
    // treat as space-separated function call.
    Token const& firstTok = Peek();
    if (firstTok.kind == TOK_IDENTIFIER) {
      Token saved = firstTok;
      Advance();
      SkipNewlines();

      if (!Check(TOK_RPAREN)) {
        // This is a function call — parse space-separated arguments
        ASTFuncCallNodeT* call = new ASTFuncCallNodeT();
        call->loc = SourceLocation(saved.line, saved.column);
        call->name = saved.value;

        do {
          SkipNewlines();
          call->args.push(ParseExpression());
          SkipNewlines();
        } while (!Check(TOK_RPAREN) && !Check(TOK_EOF));

        Expect(TOK_RPAREN, "expected ')' after function arguments");
        return call;
      }

      // No args — bare (name) is a function call with zero args
      ASTFuncCallNodeT* call = new ASTFuncCallNodeT();
      call->loc = SourceLocation(saved.line, saved.column);
      call->name = saved.value;
      Expect(TOK_RPAREN, "expected ')'");
      return call;
    }

    ASTNode expr = ParseExpression();
    SkipNewlines();
    Expect(TOK_RPAREN, "expected ')'");
    return expr;
  }

  // --- Array literal ---
  if (tok.kind == TOK_LBRACKET) {
    Advance();
    ASTArrayLiteralNodeT* node = new ASTArrayLiteralNodeT();
    node->loc = SourceLocation(tok.line, tok.column);

    SkipNewlines();
    if (!Check(TOK_RBRACKET)) {
      do {
        SkipNewlines();
        ASTNode elem = ParseExpression();
        if (elem)
          node->elements.push(elem);
        SkipNewlines();
      } while (Match(TOK_COMMA));
    }
    SkipNewlines();
    Expect(TOK_RBRACKET, "expected ']'");
    return node;
  }

  // --- Special forms ---
  if (tok.kind == TOK_CAST) {
    Advance();
    ASTCastNodeT* node = new ASTCastNodeT();
    node->loc = SourceLocation(tok.line, tok.column);
    node->typeName = ParseTypeName();
    SkipNewlines();
    node->operand = ParseExpression();
    return node;
  }

  if (tok.kind == TOK_ADDRESS) {
    Advance();
    ASTAddressNodeT* node = new ASTAddressNodeT();
    node->loc = SourceLocation(tok.line, tok.column);
    SkipNewlines();
    node->operand = ParseExpression();
    return node;
  }

  if (tok.kind == TOK_DEREF) {
    Advance();
    ASTDerefNodeT* node = new ASTDerefNodeT();
    node->loc = SourceLocation(tok.line, tok.column);
    SkipNewlines();
    node->operand = ParseExpression();
    return node;
  }

  // --- Identifier: variable, function call, constructor, or type name ---
  if (tok.kind == TOK_IDENTIFIER) {
    Advance();

    ASTNode ident = ParseIdentifier(tok);

    // Check for function/constructor call: name(args)
    if (Check(TOK_LPAREN)) {
      Advance();  // consume '('
      SkipNewlines();

      ASTFuncCallNodeT* call = new ASTFuncCallNodeT();
      call->loc = ident->loc;
      call->name = tok.value;

      if (!Check(TOK_RPAREN)) {
        do {
          SkipNewlines();
          call->args.push(ParseExpression());
          SkipNewlines();
        } while (Match(TOK_COMMA));
      }

      SkipNewlines();
      Expect(TOK_RPAREN, "expected ')' after function arguments");
      ASTNode result = ParsePostfix(call);
      return result;
    }

    // Check for dotted access / method call: identifier[.identifier...] [(args)]
    ASTNode result = ParsePostfix(ident);

    return result;
  }

  ReportError(Peek(), "unexpected token in expression");
  return nullptr;
}

// Parse postfix operators (dot access and method calls)
ASTNode Parser::ParsePostfix(ASTNode left) {
  if (!left)
    return nullptr;

  for (;;) {
    if (!Match(TOK_DOT))
      break;

    Token const& member = Expect(TOK_IDENTIFIER, "expected member name after '.'");

    // Check for method call: .methodName(args)
    if (Check(TOK_LPAREN)) {
      Advance();  // consume '('
      SkipNewlines();

      ASTMethodCallNodeT* call = new ASTMethodCallNodeT();
      call->loc = left->loc;
      call->object = left;
      call->methodName = member.value;

      if (!Check(TOK_RPAREN)) {
        do {
          SkipNewlines();
          call->args.push(ParseExpression());
          SkipNewlines();
        } while (Match(TOK_COMMA));
      }

      SkipNewlines();
      Expect(TOK_RPAREN, "expected ')' after method arguments");
      left = call;
    } else {
      // LTSL space-separated method call: .name arg1 arg2 ...
      // Collect space-separated identifier arguments.
      ASTMethodCallNodeT* call = new ASTMethodCallNodeT();
      call->loc = left->loc;
      call->object = left;
      call->methodName = member.value;

      // Collect simple identifier args until we hit a non-identifier token
      // (dot, operator, newline, EOF, etc.)
      for (;;) {
        if (!Check(TOK_IDENTIFIER))
          break;
        Token const& argTok = Peek();
        Advance();
        call->args.push(ParseIdentifier(argTok));
      }

      left = call;
    }
  }

  return left;
}

ASTNode Parser::ParseIdentifier(Token const& tok) {
  ASTIdentifierNodeT* node = new ASTIdentifierNodeT();
  node->loc = SourceLocation(tok.line, tok.column);
  node->name = tok.value;
  return node;
}

// ============================================================================
// Top-level parse
// ============================================================================

Parser::Parser(Vector<Token> const& tokens) : tokens(tokens), pos(0) {}

ASTNode Parser::Parse() {
  ASTModuleNodeT* module = new ASTModuleNodeT();
  module->loc = CurrentLoc();

  SkipNewlines();
  while (!AtEnd()) {
    ASTNode stmt = ParseStatement();
    if (stmt)
      module->statements.push(stmt);
    SkipNewlines();
  }

  return module;
}

Vector<ParseError> const& Parser::GetErrors() const {
  return errors;
}

// ============================================================================
// Top-level convenience function
// ============================================================================

ASTNode ParseLTSL(String const& source, Vector<ParseError>* errors) {
  // Lex
  Lexer lexer(source);
  Vector<Token> tokens = lexer.Tokenize();

  // Forward lexer errors as parse errors
  if (errors) {
    Vector<LexError> const& lexErrors = lexer.GetErrors();
    for (size_t i = 0; i < lexErrors.size(); ++i) {
      errors->push(ParseError(lexErrors[i].message, lexErrors[i].line, lexErrors[i].column));
    }
  }

  // Parse
  Parser parser(tokens);
  ASTNode result = parser.Parse();

  if (errors) {
    Vector<ParseError> const& parseErrors = parser.GetErrors();
    for (size_t i = 0; i < parseErrors.size(); ++i) {
      errors->push(parseErrors[i]);
    }
  }

  return result;
}

} // namespace LTE
