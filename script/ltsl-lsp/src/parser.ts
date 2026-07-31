// Copyright (C) 2025  darkoned12000
// SPDX-License-Identifier: GPL-3.0-or-later
// Part of the ltheory-old-test modernization effort (Revamp Work).
// See NOTICE and LICENSE.GPL. Original engine (c) Josh Parnell, public domain.

// Structural LTSL parser. LTSL is indentation-blocked; each line is a
// whitespace/paren-delimited token list; `(...)` groups; `#` comments. Call
// argument counting mirrors the engine's RewriteBinaryOp precedence collapse
// (src/liblt/LTE/LTSL.cpp) so counts agree with the runtime.

import { KEYWORDS, Token, TokenType, Line } from './lexer';

export interface Param {
  type: string;
  name: string;
}

export interface VarDeclInfo {
  name: string;
  line: number;
  column: number;
  start: number;
  end: number;
}

export interface CallSite {
  name: string;
  isMethod: boolean;
  argCount: number;
  argCountKnown: boolean;
  line: number;
  column: number;
  start: number;
  end: number;
}

export interface FunctionInfo {
  name: string;
  returnType: string;
  params: Param[];
  isMethod: boolean;
  typeOwner: string;
  line: number;
  column: number;
  start: number;
  end: number;
  vars: VarDeclInfo[];
  calls: CallSite[];
}

export interface FieldInfo {
  type: string;
  name: string;
  line: number;
  column: number;
}

export interface TypeInfo {
  name: string;
  line: number;
  column: number;
  start: number;
  end: number;
  fields: FieldInfo[];
  methods: FunctionInfo[];
}

export interface ParserProblem {
  message: string;
  line: number;
  column: number;
  start: number;
  end: number;
}

export interface DocumentInfo {
  functions: FunctionInfo[];
  types: TypeInfo[];
  topLevelVars: VarDeclInfo[];
  problems: ParserProblem[];
}

interface Stmt {
  line: Line;
  child: Stmt[];
}

// Operator precedence, low to high, identical to LTSL.cpp.
const PRECEDENCE: string[][] = [
  ['^'],
  ['*', '/'],
  ['+', '-'],
  ['<', '>', '<=', '>='],
  ['==', '!='],
  ['&&'],
  ['||'],
  ['=', '+=', '-=', '*=', '/='],
];

const ASSIGN_OPS = new Set(['=', '+=', '-=', '*=', '/=']);

interface ScanCollector {
  vars: VarDeclInfo[];
  calls: CallSite[];
  functions: FunctionInfo[];
}

const OPERATORS = new Set(PRECEDENCE.flat());

interface Unit {
  token: Token | null;
  group: boolean;
}

function isOperatorWord(token: Token | undefined): boolean {
  return !!token && token.type === TokenType.WORD && OPERATORS.has(token.value);
}

function findMatchingParen(tokens: Token[], open: number): number {
  let depth = 0;
  for (let i = open; i < tokens.length; ++i) {
    if (tokens[i].type === TokenType.LPAREN) {
      depth++;
    } else if (tokens[i].type === TokenType.RPAREN) {
      depth--;
      if (depth === 0) {
        return i;
      }
    }
  }
  return -1;
}

function findOpenParen(tokens: Token[], close: number): number {
  let depth = 0;
  for (let i = close; i >= 0; --i) {
    if (tokens[i].type === TokenType.RPAREN) {
      depth++;
    } else if (tokens[i].type === TokenType.LPAREN) {
      depth--;
      if (depth === 0) {
        return i;
      }
    }
  }
  return -1;
}

// Collapse `a op b` triples into single units, in engine precedence order.
function collapseUnits(units: Unit[]): Unit[] {
  const result = units.slice();
  for (const row of PRECEDENCE) {
    let i = 0;
    while (i + 2 < result.length) {
      const mid = result[i + 1];
      if (
        !mid.group &&
        mid.token &&
        mid.token.type === TokenType.WORD &&
        row.includes(mid.token.value)
      ) {
        result.splice(i, 3, { token: null, group: true });
        i = Math.max(0, i - 1);
      } else {
        i++;
      }
    }
  }
  return result;
}

// Split a token list into units (paren groups count as one), then collapse
// infix operators so `a op b` reads as a single argument.
function toUnits(tokens: Token[]): Unit[] {
  const units: Unit[] = [];
  let i = 0;
  while (i < tokens.length) {
    const t = tokens[i];
    if (t.type === TokenType.LPAREN) {
      const close = findMatchingParen(tokens, i);
      const end = close >= 0 ? close : tokens.length - 1;
      units.push({ token: null, group: true });
      i = end + 1;
    } else {
      units.push({ token: t, group: false });
      i++;
    }
  }
  return collapseUnits(units);
}

function lastDotPart(value: string): string {
  const idx = value.lastIndexOf('.');
  return idx < 0 ? value : value.slice(idx + 1);
}

export class LTSLParser {
  private lines: Line[] = [];
  private problems: ParserProblem[] = [];

  parse(lines: Line[]): DocumentInfo {
    this.lines = lines;
    this.problems = [];
    const info: DocumentInfo = {
      functions: [],
      types: [],
      topLevelVars: [],
      problems: this.problems,
    };

    const { stmts } = this.parseBlock(0, -1);
    for (const stmt of stmts) {
      const first = stmt.line.tokens[0];
      if (!first || first.type !== TokenType.WORD) {
        continue;
      }
      if (first.value === 'function') {
        const nested: FunctionInfo[] = [];
        const fn = this.parseFunction(stmt, false, '', nested);
        if (fn) {
          info.functions.push(fn);
          info.functions.push(...nested);
        }
      } else if (first.value === 'type') {
        const ty = this.parseType(stmt);
        info.types.push(ty);
        for (const m of ty.methods) {
          info.functions.push(m);
        }
      } else if (first.value === 'var' || first.value === 'ref' || first.value === 'static') {
        const v = this.parseVar(stmt);
        if (v) {
          info.topLevelVars.push(v);
        }
      }
    }

    return info;
  }

  private parseBlock(i: number, baseIndent: number): { stmts: Stmt[]; i: number } {
    const stmts: Stmt[] = [];
    while (i < this.lines.length) {
      const line = this.lines[i];
      if (line.indent <= baseIndent) {
        break;
      }
      const stmt: Stmt = { line, child: [] };
      if (i + 1 < this.lines.length && this.lines[i + 1].indent > line.indent) {
        const r = this.parseBlock(i + 1, line.indent);
        stmt.child = r.stmts;
        i = r.i;
      } else {
        i++;
      }
      stmts.push(stmt);
    }
    return { stmts, i };
  }

  private parseFunction(
    stmt: Stmt,
    isMethod: boolean,
    typeOwner: string,
    nestedOut?: FunctionInfo[]
  ): FunctionInfo | null {
    const tokens = stmt.line.tokens;

    // The parameter list is the last `(` in the line, and the function name is
    // the word immediately before it.
    let lp = -1;
    for (let i = tokens.length - 1; i >= 0; --i) {
      if (tokens[i].type === TokenType.LPAREN && i >= 1 && tokens[i - 1].type === TokenType.WORD) {
        lp = i;
        break;
      }
    }

    let nameToken: Token | null = null;
    if (lp >= 1 && tokens[lp - 1].type === TokenType.WORD) {
      nameToken = tokens[lp - 1];
    } else if (tokens[0].value === 'function' && tokens[1] && tokens[1].type === TokenType.WORD) {
      nameToken = tokens[1]; // `function Foo` with no parens
      lp = -1;
    }

    if (!nameToken) {
      return null;
    }

    let returnType = '';
    if (lp >= 2) {
      const prev = tokens[lp - 2];
      if (prev.type === TokenType.WORD && prev.value !== 'function') {
        returnType = prev.value;
      } else if (prev.type === TokenType.RPAREN) {
        // Grouped return type, e.g. `function (Array Widget) GatherWidgets (...)`
        const open = findOpenParen(tokens, lp - 2);
        if (open >= 0) {
          returnType = tokens
            .slice(open + 1, lp - 2)
            .filter(t => t.type === TokenType.WORD)
            .map(t => t.value)
            .join(' ');
        }
      }
    }

    const rp = lp >= 0 ? findMatchingParen(tokens, lp) : -1;
    const paramTokens = rp >= 0 ? tokens.slice(lp + 1, rp) : [];
    const params = this.parseParams(paramTokens);

    const last = tokens[tokens.length - 1];
    const collector: ScanCollector = {
      vars: [],
      calls: [],
      functions: nestedOut ? nestedOut : [],
    };
    this.scanBlock(stmt.child, collector);

    return {
      name: nameToken.value,
      returnType,
      params,
      isMethod,
      typeOwner,
      line: stmt.line.line,
      column: nameToken.column,
      start: nameToken.start,
      end: nameToken.end,
      vars: collector.vars,
      calls: collector.calls,
    };
  }

  private parseParams(tokens: Token[]): Param[] {
    // LTSL parameter lists are alternating `Type Name` pairs, e.g.
    // `(String label Int minValue Int maxValue)`. A `(...)` group is a single
    // type unit (`(Array Widget)`). A unit that starts with an uppercase
    // letter is a type; anything else is a parameter name.
    const units: string[] = [];
    let i = 0;
    while (i < tokens.length) {
      const t = tokens[i];
      if (t.type === TokenType.LPAREN) {
        const close = findMatchingParen(tokens, i);
        const end = close >= 0 ? close : tokens.length - 1;
        const inner = tokens
          .slice(i + 1, end)
          .filter(x => x.type === TokenType.WORD)
          .map(x => x.value)
          .join(' ');
        units.push(`(${inner})`);
        i = end + 1;
      } else if (t.type === TokenType.WORD) {
        units.push(t.value);
        i++;
      } else {
        i++;
      }
    }

    const params: Param[] = [];
    i = 0;
    while (i < units.length) {
      const typeUnit = units[i];
      i++;
      let name = '';
      if (i < units.length && !this.isParamTypeUnit(units[i])) {
        name = units[i];
        i++;
      }
      if (name) {
        params.push({ type: typeUnit, name });
      }
    }
    return params;
  }

  private isParamTypeUnit(u: string): boolean {
    if (u.startsWith('(')) {
      return true; // grouped type
    }
    if (/[<>/]/.test(u)) {
      return true; // generic or namespaced type
    }
    return /^[A-Z]/.test(u); // capitalized type-name convention
  }

  private parseVar(stmt: Stmt): VarDeclInfo | null {
    const tokens = stmt.line.tokens;
    const nameToken = tokens[1];
    if (!nameToken || nameToken.type !== TokenType.WORD) {
      return null;
    }
    return {
      name: nameToken.value,
      line: stmt.line.line,
      column: nameToken.column,
      start: nameToken.start,
      end: nameToken.end,
    };
  }

  private parseType(stmt: Stmt): TypeInfo {
    const tokens = stmt.line.tokens;
    const name = tokens[1] && tokens[1].type === TokenType.WORD ? tokens[1].value : '';
    const fields: FieldInfo[] = [];
    const methods: FunctionInfo[] = [];

    for (const member of stmt.child) {
      const mtokens = member.line.tokens;
      const first = mtokens[0];
      if (!first) {
        continue;
      }
      if (first.type === TokenType.WORD && first.value === 'function') {
        const nested: FunctionInfo[] = [];
        const fn = this.parseFunction(member, true, name, nested);
        if (fn) {
          methods.push(fn);
          methods.push(...nested);
        }
        continue;
      }
      const field = this.parseField(member);
      if (field) {
        fields.push(field);
      }
    }

    const last = tokens[tokens.length - 1];
    return {
      name,
      line: stmt.line.line,
      column: name ? tokens[1].column : 0,
      start: tokens[0].start,
      end: last.end,
      fields,
      methods,
    };
  }

  private parseField(stmt: Stmt): FieldInfo | null {
    const tokens = stmt.line.tokens;
    if (tokens[0].type === TokenType.LPAREN) {
      // (Type Name) fieldName (default)?
      const close = findMatchingParen(tokens, 0);
      if (close < 0) {
        return null;
      }
      const interior = tokens.slice(1, close).filter(t => t.type === TokenType.WORD);
      const nameToken = tokens[close + 1];
      if (!nameToken || nameToken.type !== TokenType.WORD) {
        return null;
      }
      return {
        type: interior.map(t => t.value).join(' '),
        name: nameToken.value,
        line: stmt.line.line,
        column: nameToken.column,
      };
    }
    if (tokens[0].type === TokenType.WORD) {
      const nameToken = tokens[1];
      if (!nameToken || nameToken.type !== TokenType.WORD) {
        return null;
      }
      return {
        type: tokens[0].value,
        name: nameToken.value,
        line: stmt.line.line,
        column: nameToken.column,
      };
    }
    return null;
  }

  private scanBlock(stmts: Stmt[], collector: ScanCollector): void {
    for (const stmt of stmts) {
      this.scanStatement(stmt, collector);
    }
  }

  private scanStatement(stmt: Stmt, collector: ScanCollector): void {
    const tokens = stmt.line.tokens;
    const first = tokens[0];

    if (first && first.type === TokenType.WORD) {
      if (first.value === 'function') {
        // Nested function declaration: record it and scan its body. It is
        // visible throughout the file, so it must appear in docFunctions.
        const nested: FunctionInfo[] = [];
        const fn = this.parseFunction(stmt, false, '', nested);
        if (fn) {
          collector.functions.push(fn, ...nested);
        }
        return;
      }
      if (first.value === 'var' || first.value === 'ref' || first.value === 'static') {
        const v = this.parseVar(stmt);
        if (v) {
          collector.vars.push(v);
        }
        this.scanStatementBody(stmt, collector);
        return;
      }
      if (KEYWORDS.has(first.value)) {
        // control statement (if/switch/for/while/return/cast/case/otherwise)
        if (first.value === 'for' && tokens[1] && tokens[1].type === TokenType.WORD) {
          collector.vars.push({
            name: tokens[1].value,
            line: stmt.line.line,
            column: tokens[1].column,
            start: tokens[1].start,
            end: tokens[1].end,
          });
        }
        this.scanGroups(tokens, collector);
        this.scanBlock(stmt.child, collector);
        return;
      }
      if (
        tokens.length > 1 &&
        tokens[1].type === TokenType.WORD &&
        ASSIGN_OPS.has(tokens[1].value)
      ) {
        // Assignment to a plain variable — treat the target as a known name
        // (e.g. `time += FrameTimer_Get` where `time` is a field/global).
        if (!tokens[0].value.includes('.')) {
          collector.vars.push({
            name: tokens[0].value,
            line: stmt.line.line,
            column: tokens[0].column,
            start: tokens[0].start,
            end: tokens[0].end,
          });
        }
        this.scanStatementBody(stmt, collector);
        return;
      }
    }

    this.classifyExpression(stmt, tokens, collector);
    this.scanStatementBody(stmt, collector);
  }

  // Children of a var/assignment/call statement are either a control block
  // (e.g. `var x` followed by a `switch`) or continuation expression lines of
  // a multiline call. Expressions must not be treated as statements.
  private scanStatementBody(stmt: Stmt, collector: ScanCollector): void {
    this.scanGroups(stmt.line.tokens, collector);
    if (stmt.child.length === 0) {
      return;
    }
    const head = stmt.child[0].line.tokens[0];
    if (head && head.type === TokenType.WORD && KEYWORDS.has(head.value)) {
      this.scanBlock(stmt.child, collector);
    } else {
      for (const child of stmt.child) {
        this.scanExpressionLine(child, collector);
      }
    }
  }

  private classifyExpression(
    stmt: Stmt,
    tokens: Token[],
    collector: ScanCollector
  ): void {
    if (tokens.length === 0) {
      return;
    }
    const head = tokens[0];
    if (head.type !== TokenType.WORD) {
      return;
    }

    if (head.value.includes('.')) {
      // `obj.Method arg...` — method call when it has arguments. But a pair
      // of dotted reads on one line is a sibling-expression pair (switch
      // cases, `else` clauses, etc.), not a method call.
      const rest = tokens.slice(1);
      const firstRest = rest[0];
      if (tokens.length > 1 && !(firstRest.type === TokenType.WORD && firstRest.value.includes('.'))) {
        collector.calls.push({
          name: lastDotPart(head.value),
          isMethod: true,
          argCount: toUnits(rest).length,
          argCountKnown: stmt.child.length === 0,
          line: stmt.line.line,
          column: head.column,
          start: head.start,
          end: tokens[tokens.length - 1].end,
        });
      }
      return;
    }

    if (tokens.length > 1) {
      // Bare prefix call: f arg1 arg2 ... A deeper-indented child block adds
      // more arguments we cannot count reliably, so mark it unknown.
      collector.calls.push({
        name: head.value,
        isMethod: false,
        argCount: toUnits(tokens.slice(1)).length,
        argCountKnown: stmt.child.length === 0,
        line: stmt.line.line,
        column: head.column,
        start: head.start,
        end: tokens[tokens.length - 1].end,
      });
      return;
    }

    // Single atom with a deeper-indented block is a multiline call
    // (e.g. `DrawText` with arguments on the next lines).
    if (stmt.child.length > 0) {
      collector.calls.push({
        name: head.value,
        isMethod: false,
        argCount: 0,
        argCountKnown: false,
        line: stmt.line.line,
        column: head.column,
        start: head.start,
        end: head.end,
      });
    }
  }

  // A single line inside a multiline call argument list. Uses the same
  // collapsed-unit counting as groups: `f a b` is a call with 2 args, while
  // `FrameTimer_GetEMA10 * 1000.0 + " ms"` collapses to one expression whose
  // head is a 0-arg call.
  private scanExpressionLine(stmt: Stmt, collector: ScanCollector): void {
    const tokens = stmt.line.tokens;
    this.scanGroups(tokens, collector);
    if (tokens.length === 0) {
      return;
    }
    const head = tokens[0];
    if (head.type !== TokenType.WORD || isOperatorWord(head)) {
      return;
    }
    if (head.value.includes('.')) {
      return; // member read within an expression
    }
    const units = toUnits(tokens);
    collector.calls.push({
      name: head.value,
      isMethod: false,
      argCount: units.length > 1 ? units.length - 1 : 0,
      argCountKnown: units.length > 1 && stmt.child.length === 0,
      line: stmt.line.line,
      column: head.column,
      start: head.start,
      end: tokens[tokens.length - 1].end,
    });
  }

  private scanGroups(tokens: Token[], collector: ScanCollector): void {
    let i = 0;
    while (i < tokens.length) {
      if (tokens[i].type !== TokenType.LPAREN) {
        i++;
        continue;
      }
      const close = findMatchingParen(tokens, i);
      const end = close >= 0 ? close : tokens.length - 1;

      if (close < 0) {
        // Unbalanced group.
        this.problems.push({
          message: "Unbalanced '(' -- missing ')'",
          line: tokens[i].line,
          column: tokens[i].column,
          start: tokens[i].start,
          end: tokens[end].end,
        });
        const head = this.groupHead(tokens.slice(i + 1));
        if (head) {
          collector.calls.push({
            name: head.name,
            isMethod: head.isMethod,
            argCount: 0,
            argCountKnown: false,
            line: head.line,
            column: head.column,
            start: head.start,
            end: tokens[end].end,
          });
        }
      }

      if (close >= 0) {
        const interior = tokens.slice(i + 1, close);
        const head = this.groupHead(interior);
        // A dotted head means an infix expression / member read (e.g.
        // `(ship.Plug weapon)` is a call, but `(a.b ^ k + 1.0)` is math) --
        // skip those; the engine would treat them as `(b a)` calls of the
        // member, which we cannot arg-count reliably.
        if (head && !head.isMethod) {
          collector.calls.push({
            name: head.name,
            isMethod: false,
            argCount: Math.max(0, toUnits(interior).length - 1),
            argCountKnown: true,
            line: head.line,
            column: head.column,
            start: head.start,
            end: head.end,
          });
        }
        this.scanGroups(interior, collector);
      }

      i = end + 1;
    }
  }

  private groupHead(
    interior: Token[]
  ): { name: string; isMethod: boolean; index: number; line: number; column: number; start: number; end: number } | null {
    let index = 0;
    while (index < interior.length) {
      const t = interior[index];
      if (t.type === TokenType.WORD) {
        if (isOperatorWord(t)) {
          return null;
        }
        const isMethod = t.value.includes('.');
        return {
          name: lastDotPart(t.value),
          isMethod,
          index,
          line: t.line,
          column: t.column,
          start: t.start,
          end: t.end,
        };
      }
      if (t.type === TokenType.LPAREN) {
        return null;
      }
      index++;
    }
    return null;
  }
}
