#ifndef LTE_Lexer_h__
#define LTE_Lexer_h__

#include "String.h"
#include <string>
#include "Vector.h"
#include <vector>

namespace LTE {

enum TokenKind {
  // Literals
  TOK_INT,
  TOK_FLOAT,
  TOK_STRING,
  TOK_BOOL,
  TOK_NULL,

  // Identifiers
  TOK_IDENTIFIER,

  // Operators
  TOK_PLUS,
  TOK_MINUS,
  TOK_STAR,
  TOK_SLASH,
  TOK_MOD,
  TOK_EQUALS,
  TOK_NOT_EQUALS,
  TOK_LESS,
  TOK_GREATER,
  TOK_LESS_EQUALS,
  TOK_GREATER_EQUALS,
  TOK_AND,
  TOK_OR,
  TOK_NOT,
  TOK_CARET,

  // Assignment
  TOK_ASSIGN,
  TOK_PLUS_ASSIGN,
  TOK_MINUS_ASSIGN,
  TOK_MULTIPLY_ASSIGN,
  TOK_DIVIDE_ASSIGN,

  // Delimiters
  TOK_LPAREN,
  TOK_RPAREN,
  TOK_LBRACKET,
  TOK_RBRACKET,
  TOK_COMMA,
  TOK_COLON,
  TOK_DOT,

  // Keywords
  TOK_VAR,
  TOK_REF,
  TOK_STATIC,
  TOK_FUNCTION,
  TOK_RETURN,
  TOK_IF,
  TOK_ELSE,
  TOK_WHILE,
  TOK_FOR,
  TOK_IN,
  TOK_SWITCH,
  TOK_OTHERWISE,
  TOK_BREAK,
  TOK_CONTINUE,
  TOK_TRUE,
  TOK_FALSE,
  TOK_TYPE,
  TOK_CAST,
  TOK_BLOCK,
  TOK_DESC,
  TOK_ADDRESS,
  TOK_DEREF,
  TOK_AT,
  TOK_QUESTION,

  // Special
  TOK_NEWLINE,
  TOK_INDENT,
  TOK_DEDENT,
  TOK_EOF,

  TOK_UNKNOWN
};

struct Token {
  TokenKind kind;
  std::string value;
  int line;
  int column;
  int length;

  Token() :
    kind(TOK_UNKNOWN),
    line(0),
    column(0),
    length(0) {}

  Token(TokenKind kind, std::string const& value, int line, int column, int length) :
    kind(kind),
    value(value),
    line(line),
    column(column),
    length(length) {}

  Token(Token const& other) :
    kind(other.kind),
    value(other.value),
    line(other.line),
    column(other.column),
    length(other.length) {}

  Token& operator=(Token const& other) {
    kind = other.kind;
    value = other.value;
    line = other.line;
    column = other.column;
    length = other.length;
    return *this;
  }
};

struct LexError {
  std::string message;
  int line;
  int column;

  LexError(std::string const& message, int line, int column) :
    message(message),
    line(line),
    column(column) {}

  LexError(String const& message, int line, int column) :
    message(std::string(message.c_str())),
    line(line),
    column(column) {}

  LexError(LexError const& other) :
    message(other.message),
    line(other.line),
    column(other.column) {}

  LexError& operator=(LexError const& other) {
    message = other.message;
    line = other.line;
    column = other.column;
    return *this;
  }
};

class Lexer {
public:
  Lexer(String const& source);

  std::vector<Token> Tokenize();
  std::vector<LexError> const& GetErrors() const;

private:
  // Source state
  String source;
  size_t pos;
  int line;
  int column;

  // Indent stack
  Vector<int> indentStack;
  bool atLineStart;
  bool hasTokensOnLine;
  int parenDepth;
  // Open-group kinds ('(' or '[') — used to auto-close unbalanced groups at
  // end of line, matching the old line parser's per-line paren balance.
  std::vector<char> groupStack;

  // Output
  std::vector<Token> tokens;
  std::vector<LexError> errors;

  // Buffered indent tokens
  std::vector<Token> pending;

  // Character access
  char Peek() const;
  char Peek2() const;
  char Advance();
  bool AtEnd() const;

  // Whitespace / comments
  void SkipHorizontalWhitespace();
  void SkipSingleLineComment();

  // Indent handling
  int MeasureIndent();
  void EmitIndentTokens(int newIndent);
  void EmitPendingTokens();

  // Token readers
  Token ReadNumber();
  Token ReadString();
  Token ReadIdentifier();
  Token ReadOperator();

  // Error reporting
  void ReportError(String const& message);

  // Emit a token (marks that real content exists on the current line)
  void Emit(Token const& tok);

  // Helpers
  static bool IsDigit(char c);
  static bool IsAlpha(char c);
  static bool IsAlphaNumeric(char c);
  static bool IsIdentStart(char c);
  static bool IsIdentChar(char c);
  static bool IsOpChar(char c);
  static TokenKind LookupKeyword(String const& value);
};

} // namespace LTE

#endif
