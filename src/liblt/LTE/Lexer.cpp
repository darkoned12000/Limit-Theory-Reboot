#include "Lexer.h"

namespace LTE {

// ============================================================================
// Helpers
// ============================================================================

bool Lexer::IsDigit(char c) {
  return c >= '0' && c <= '9';
}

bool Lexer::IsAlpha(char c) {
  return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}

bool Lexer::IsAlphaNumeric(char c) {
  return IsAlpha(c) || IsDigit(c);
}

bool Lexer::IsIdentStart(char c) {
  return IsAlpha(c);
}

bool Lexer::IsIdentChar(char c) {
  return IsAlphaNumeric(c);
}

bool Lexer::IsOpChar(char c) {
  return c == '+' || c == '-' || c == '*' || c == '/' || c == '%' ||
         c == '=' || c == '!' || c == '<' || c == '>' || c == '&' || c == '|';
}

TokenKind Lexer::LookupKeyword(String const& value) {
  if (value == "var")       return TOK_VAR;
  if (value == "ref")       return TOK_REF;
  if (value == "static")    return TOK_STATIC;
  if (value == "function")  return TOK_FUNCTION;
  if (value == "return")    return TOK_RETURN;
  if (value == "if")        return TOK_IF;
  if (value == "else")      return TOK_ELSE;
  if (value == "while")     return TOK_WHILE;
  if (value == "for")       return TOK_FOR;
  if (value == "in")        return TOK_IN;
  if (value == "switch")    return TOK_SWITCH;
  if (value == "otherwise") return TOK_OTHERWISE;
  if (value == "break")     return TOK_BREAK;
  if (value == "continue")  return TOK_CONTINUE;
  if (value == "true")      return TOK_TRUE;
  if (value == "false")     return TOK_FALSE;
  if (value == "type")      return TOK_TYPE;
  if (value == "cast")      return TOK_CAST;
  if (value == "block")     return TOK_BLOCK;
  if (value == "desc")      return TOK_DESC;
  if (value == "address")   return TOK_ADDRESS;
  if (value == "deref")     return TOK_DEREF;
  return TOK_IDENTIFIER;
}

// ============================================================================
// Character access
// ============================================================================

char Lexer::Peek() const {
  if (pos >= source.size())
    return '\0';
  return source[pos];
}

char Lexer::Peek2() const {
  if (pos + 1 >= source.size())
    return '\0';
  return source[pos + 1];
}

char Lexer::Advance() {
  char c = source[pos++];
  if (c == '\n') {
    line++;
    column = 1;
  } else {
    column++;
  }
  return c;
}

bool Lexer::AtEnd() const {
  return pos >= source.size();
}

// ============================================================================
// Whitespace / comments
// ============================================================================

void Lexer::SkipHorizontalWhitespace() {
  while (!AtEnd()) {
    char c = Peek();
    if (c == ' ' || c == '\t' || c == '\r')
      Advance();
    else
      break;
  }
}

void Lexer::SkipSingleLineComment() {
  while (!AtEnd() && Peek() != '\n')
    Advance();
}

// ============================================================================
// Indent handling
// ============================================================================

int Lexer::MeasureIndent() {
  int indent = 0;
  int saved = pos;
  int savedCol = column;

  while (!AtEnd()) {
    char c = Peek();
    if (c == ' ') {
      indent++;
      Advance();
    } else if (c == '\t') {
      indent += 4;
      Advance();
    } else {
      break;
    }
  }

  // If the line is blank (only whitespace), return -1 to signal "blank line"
  if (AtEnd() || Peek() == '\n')
    return -1;

  pos = saved;
  column = savedCol;
  return indent;
}

void Lexer::EmitIndentTokens(int newIndent) {
  if (indentStack.empty())
    indentStack.push(0);

  int current = indentStack.back();

  if (newIndent > current) {
    indentStack.push(newIndent);
    pending.push(Token(TOK_INDENT, "", line, 1, 0));
  } else if (newIndent < current) {
    while (indentStack.back() > newIndent) {
      indentStack.pop();
      pending.push(Token(TOK_DEDENT, "", line, 1, 0));
    }
    if (indentStack.back() != newIndent) {
      ReportError("unindent does not match any outer indentation level");
    }
  }
}

void Lexer::EmitPendingTokens() {
  for (size_t i = 0; i < pending.size(); i++)
    tokens.push(pending[i]);
  pending.clear();
}

// ============================================================================
// Error reporting
// ============================================================================

void Lexer::ReportError(String const& message) {
  errors.push(LexError(message, line, column));
}

void Lexer::Emit(Token const& tok) {
  tokens.push(tok);
  if (tok.kind != TOK_NEWLINE && tok.kind != TOK_INDENT &&
      tok.kind != TOK_DEDENT && tok.kind != TOK_EOF)
    hasTokensOnLine = true;
}

// ============================================================================
// Token readers
// ============================================================================

Token Lexer::ReadNumber() {
  int startLine = line;
  int startCol = column;
  int start = pos;
  bool isFloat = false;

  while (!AtEnd() && IsDigit(Peek()))
    Advance();

  if (!AtEnd() && Peek() == '.' && Peek2() != '.' && Peek2() != ' ') {
    isFloat = true;
    Advance();
    while (!AtEnd() && IsDigit(Peek()))
      Advance();
  }

  String value = source.substr(start, pos - start);
  return Token(isFloat ? TOK_FLOAT : TOK_INT, value, startLine, startCol, pos - start);
}

Token Lexer::ReadString() {
  int startLine = line;
  int startCol = column;
  char quote = Advance(); // consume opening quote
  int start = pos;

  while (!AtEnd() && Peek() != quote) {
    if (Peek() == '\\')
      Advance(); // skip escaped char
    if (!AtEnd())
      Advance();
  }

  if (AtEnd()) {
    ReportError("unterminated string literal");
    return Token(TOK_STRING, source.substr(start, pos - start), startLine, startCol, pos - start);
  }

  Advance(); // consume closing quote
  String value = source.substr(start, pos - start - 1); // exclude quotes
  return Token(TOK_STRING, value, startLine, startCol, pos - start);
}

Token Lexer::ReadIdentifier() {
  int startLine = line;
  int startCol = column;
  int start = pos;

  // Handle leading dots: `.!`, `++`, `--` postfix operators
  // These are part of identifiers in LTSL (e.g., `i.++`, `paused.!`)
  if (!AtEnd() && Peek() == '.' && !IsDigit(Peek2())) {
    // Check if next char is an operator or ident char
    char next = Peek2();
    if (next == '!' || next == '+' || next == '-') {
      Advance(); // consume '.'
      Advance(); // consume operator char
      String value = source.substr(start, pos - start);
      return Token(TOK_IDENTIFIER, value, startLine, startCol, pos - start);
    }
  }

  while (!AtEnd() && IsIdentChar(Peek()))
    Advance();

  String value = source.substr(start, pos - start);
  TokenKind kind = LookupKeyword(value);
  return Token(kind, value, startLine, startCol, pos - start);
}

Token Lexer::ReadOperator() {
  int startLine = line;
  int startCol = column;
  char c = Advance();
  char next = Peek();

  switch (c) {
    case '+':
      if (next == '=') { Advance(); return Token(TOK_PLUS_ASSIGN, "+=", startLine, startCol, 2); }
      return Token(TOK_PLUS, "+", startLine, startCol, 1);
    case '-':
      if (next == '=') { Advance(); return Token(TOK_MINUS_ASSIGN, "-=", startLine, startCol, 2); }
      return Token(TOK_MINUS, "-", startLine, startCol, 1);
    case '*':
      if (next == '=') { Advance(); return Token(TOK_MULTIPLY_ASSIGN, "*=", startLine, startCol, 2); }
      return Token(TOK_STAR, "*", startLine, startCol, 1);
    case '/':
      if (next == '=') { Advance(); return Token(TOK_DIVIDE_ASSIGN, "/=", startLine, startCol, 2); }
      return Token(TOK_SLASH, "/", startLine, startCol, 1);
    case '%':
      return Token(TOK_MOD, "%", startLine, startCol, 1);
    case '=':
      if (next == '=') { Advance(); return Token(TOK_EQUALS, "==", startLine, startCol, 2); }
      return Token(TOK_ASSIGN, "=", startLine, startCol, 1);
    case '!':
      if (next == '=') { Advance(); return Token(TOK_NOT_EQUALS, "!=", startLine, startCol, 2); }
      return Token(TOK_NOT, "!", startLine, startCol, 1);
    case '<':
      if (next == '=') { Advance(); return Token(TOK_LESS_EQUALS, "<=", startLine, startCol, 2); }
      return Token(TOK_LESS, "<", startLine, startCol, 1);
    case '>':
      if (next == '=') { Advance(); return Token(TOK_GREATER_EQUALS, ">=", startLine, startCol, 2); }
      return Token(TOK_GREATER, ">", startLine, startCol, 1);
    case '&':
      if (next == '&') { Advance(); return Token(TOK_AND, "&&", startLine, startCol, 2); }
      ReportError("single '&' is not a valid operator; did you mean '&&'?");
      return Token(TOK_UNKNOWN, "&", startLine, startCol, 1);
    case '|':
      if (next == '|') { Advance(); return Token(TOK_OR, "||", startLine, startCol, 2); }
      ReportError("single '|' is not a valid operator; did you mean '||'?");
      return Token(TOK_UNKNOWN, "|", startLine, startCol, 1);
    default:
      return Token(TOK_UNKNOWN, std::string(1, c), startLine, startCol, 1);
  }
}

// ============================================================================
// Constructor / public interface
// ============================================================================

Lexer::Lexer(String const& source) :
  source(source),
  pos(0),
  line(1),
  column(1),
  atLineStart(true),
  hasTokensOnLine(false),
  parenDepth(0) {
  indentStack.push(0);
}

Vector<LexError> const& Lexer::GetErrors() const {
  return errors;
}

// ============================================================================
// Main tokenize loop
// ============================================================================

Vector<Token> Lexer::Tokenize() {
  while (!AtEnd()) {
    // === Line-start indent processing ===
    if (atLineStart && parenDepth == 0) {
      // Measure indent BEFORE consuming whitespace
      int indent = MeasureIndent();

      // Blank line: skip entirely (don't emit NEWLINE)
      if (indent < 0) {
        // MeasureIndent restored pos; skip to end of blank line
        while (!AtEnd() && Peek() != '\n')
          Advance();
        if (!AtEnd())
          Advance(); // consume \n
        continue;
      }

      EmitIndentTokens(indent);
      atLineStart = false;

      // Now skip the horizontal whitespace that MeasureIndent measured
      SkipHorizontalWhitespace();
    }

    // === Skip horizontal whitespace (inside parens or mid-line) ===
    SkipHorizontalWhitespace();

    if (AtEnd())
      break;

    // === Newline handling ===
    if (Peek() == '\n') {
      Advance();
      if (parenDepth == 0) {
        // Only emit NEWLINE if real tokens were on this line
        if (hasTokensOnLine) {
          tokens.push(Token(TOK_NEWLINE, "\\n", line - 1, 0, 1));
          hasTokensOnLine = false;
        }
        atLineStart = true;
      }
      // Inside parens: just advance, don't emit NEWLINE
      continue;
    }

    // === Carriage return (skip) ===
    if (Peek() == '\r') {
      Advance();
      continue;
    }

    // === Comment: # to end of line (single-line only) ===
    if (Peek() == '#') {
      SkipSingleLineComment();
      continue;
    }

    // === Track paren/bracket depth ===
    if (Peek() == '(' || Peek() == '[') {
      parenDepth++;
      char c = Advance();
      TokenKind kind = (c == '(') ? TOK_LPAREN : TOK_LBRACKET;
      std::string val(1, c);
      Emit(Token(kind, val, line, column - 1, 1));
      continue;
    }

    if (Peek() == ')' || Peek() == ']') {
      char c = Advance();
      TokenKind kind = (c == ')') ? TOK_RPAREN : TOK_RBRACKET;
      std::string val(1, c);
      Emit(Token(kind, val, line, column - 1, 1));
      if (parenDepth > 0)
        parenDepth--;

      // If we just closed the outermost paren/bracket group,
      // the next newline should trigger indent re-evaluation
      if (parenDepth == 0)
        atLineStart = true;
      continue;
    }

    // === Dot (standalone — not part of identifiers) ===
    if (Peek() == '.') {
      Advance();
      Emit(Token(TOK_DOT, ".", line, column - 1, 1));
      continue;
    }

    // === Comma ===
    if (Peek() == ',') {
      Advance();
      Emit(Token(TOK_COMMA, ",", line, column - 1, 1));
      continue;
    }

    // === Colon ===
    if (Peek() == ':') {
      Advance();
      Emit(Token(TOK_COLON, ":", line, column - 1, 1));
      continue;
    }

    // === @ debug print ===
    if (Peek() == '@') {
      Advance();
      Emit(Token(TOK_AT, "@", line, column - 1, 1));
      continue;
    }

    // === String literal ===
    if (Peek() == '"' || Peek() == '\'') {
      Emit(ReadString());
      continue;
    }

    // === Number ===
    if (IsDigit(Peek())) {
      Emit(ReadNumber());
      continue;
    }

    // === Identifier or keyword ===
    if (IsIdentStart(Peek())) {
      Emit(ReadIdentifier());
      continue;
    }

    // === Operators ===
    if (IsOpChar(Peek())) {
      Emit(ReadOperator());
      continue;
    }

    // === Unknown character ===
    ReportError(String("unexpected character: ") + Peek());
    Advance();
  }

  // === Finalize: emit remaining pending indent tokens ===
  EmitPendingTokens();

  // Flush any remaining indent stack (implicit dedents at EOF)
  while (indentStack.size() > 1) {
    indentStack.pop();
    tokens.push(Token(TOK_DEDENT, "", line, 1, 0));
  }

  // Emit EOF
  tokens.push(Token(TOK_EOF, "", line, column, 0));

  return tokens;
}

} // namespace LTE
