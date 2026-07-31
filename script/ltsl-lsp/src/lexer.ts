// Copyright (C) 2025  darkoned12000
// SPDX-License-Identifier: GPL-3.0-or-later
// Part of the ltheory-old-test modernization effort (Revamp Work).
// See NOTICE and LICENSE.GPL. Original engine (c) Josh Parnell, public domain.

// LTSL tokenizer. The engine splits source on whitespace, treats `(`/`)` as
// grouping scope (which may span lines), `"..."` as string literals and `#` as
// a line comment. Everything else -- including operators like `=`, `==`, `.`,
// `/`, `:` and mixed atoms like `2Pi`, `i.++`, `Object/SystemPopulate:Init`,
// `Vector<Reference<RenderPassT>>` -- is a single whitespace-delimited atom.
// This lexer mirrors that exactly so line/block structure and call-argument
// boundaries agree with the real runtime.

export enum TokenType {
  WORD, // identifiers, numbers, operators, mixed atoms
  STRING,
  LPAREN,
  RPAREN,
}

export interface Token {
  type: TokenType;
  value: string;
  line: number; // 0-based
  column: number; // 0-based character offset within the line
  start: number; // absolute byte offset (inclusive)
  end: number; // absolute byte offset (exclusive)
}

export interface Line {
  indent: number; // leading whitespace (spaces + tabs)
  tokens: Token[];
  line: number; // 0-based
  start: number; // absolute offset of first token (or end of line if blank)
  end: number; // absolute offset past the last token
}

export const KEYWORDS = new Set([
  'function',
  'type',
  'var',
  'ref',
  'static',
  'if',
  'switch',
  'case',
  'otherwise',
  'for',
  'while',
  'return',
  'cast',
]);

export class LTSLLexer {
  private source = '';
  private cursor = 0;
  private line = 0;
  private lineStart = 0;

  tokenize(source: string): Line[] {
    this.source = source;
    this.cursor = 0;
    this.line = 0;
    this.lineStart = 0;
    const lines: Line[] = [];

    while (this.cursor < this.source.length) {
      const indent = this.countIndent();

      // Blank line (only whitespace before the newline): skip.
      if (this.cursor >= this.source.length) {
        break;
      }
      if (this.source[this.cursor] === '\n') {
        this.advanceLine();
        continue;
      }

      const start = this.cursor;
      const tokens: Token[] = [];

      while (this.cursor < this.source.length) {
        const ch = this.source[this.cursor];

        if (ch === '\n') {
          break;
        }

        if (ch === ' ' || ch === '\t') {
          this.cursor++;
          continue;
        }

        // Line comment: everything after `#` to end of line.
        if (ch === '#') {
          this.consumeLine();
          break;
        }

        if (ch === '"') {
          tokens.push(this.tokenizeString());
          continue;
        }

        if (ch === '(') {
          tokens.push(this.singleChar(TokenType.LPAREN));
          continue;
        }

        if (ch === ')') {
          tokens.push(this.singleChar(TokenType.RPAREN));
          continue;
        }

        tokens.push(this.tokenizeAtom());
      }

      const end = this.cursor;
      if (tokens.length > 0) {
        lines.push({ indent, tokens, line: this.line, start, end });
      }
      if (this.cursor < this.source.length && this.source[this.cursor] === '\n') {
        this.advanceLine();
      }
    }
    return lines;
  }

  private singleChar(type: TokenType): Token {
    const start = this.cursor;
    const column = this.cursor - this.lineStart;
    this.cursor++;
    return {
      type,
      value: this.source.slice(start, this.cursor),
      line: this.line,
      column,
      start,
      end: this.cursor,
    };
  }

  private tokenizeString(): Token {
    const start = this.cursor;
    const column = this.cursor - this.lineStart;
    this.cursor++; // opening quote
    let value = '';
    let escaped = false;

    while (this.cursor < this.source.length) {
      const ch = this.source[this.cursor];
      if (escaped) {
        value += ch;
        escaped = false;
        this.cursor++;
        continue;
      }
      if (ch === '\\') {
        escaped = true;
        this.cursor++;
        continue;
      }
      if (ch === '"') {
        this.cursor++;
        break;
      }
      if (ch === '\n') {
        // Unterminated string: stop at end of line like the engine does.
        break;
      }
      value += ch;
      this.cursor++;
    }

    return {
      type: TokenType.STRING,
      value,
      line: this.line,
      column,
      start,
      end: this.cursor,
    };
  }

  private tokenizeAtom(): Token {
    const start = this.cursor;
    const column = this.cursor - this.lineStart;
    while (this.cursor < this.source.length) {
      const ch = this.source[this.cursor];
      if (ch === ' ' || ch === '\t' || ch === '\n') {
        break;
      }
      if (ch === '(' || ch === ')' || ch === '"' || ch === '#') {
        break;
      }
      this.cursor++;
    }
    return {
      type: TokenType.WORD,
      value: this.source.slice(start, this.cursor),
      line: this.line,
      column,
      start,
      end: this.cursor,
    };
  }

  private countIndent(): number {
    let indent = 0;
    while (this.cursor < this.source.length) {
      const ch = this.source[this.cursor];
      if (ch === ' ' || ch === '\t') {
        indent++;
        this.cursor++;
      } else {
        break;
      }
    }
    return indent;
  }

  private consumeLine(): void {
    while (this.cursor < this.source.length && this.source[this.cursor] !== '\n') {
      this.cursor++;
    }
  }

  private advanceLine(): void {
    this.cursor++;
    this.line++;
    this.lineStart = this.cursor;
  }
}
