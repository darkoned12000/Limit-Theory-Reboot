// Copyright (C) 2025  darkoned12000
// SPDX-License-Identifier: GPL-3.0-or-later
// Part of the ltheory-old-test modernization effort (Revamp Work).
// See NOTICE and LICENSE.GPL. Original engine (c) Josh Parnell, public domain.

// Semantic analysis: duplicate declarations, unknown functions, argument-count
// mismatches and unbalanced-parenthesis problems. Deliberately conservative --
// LTSL is loose (cross-file script functions, namespaced refs, fields accessed
// bare inside methods), so anything we cannot resolve with confidence is left
// alone rather than reported.

import { APIDatabase } from './api-database';
import { CallSite, DocumentInfo, FunctionInfo } from './parser';
import { KEYWORDS } from './lexer';

export interface Range {
  start: { line: number; character: number };
  end: { line: number; character: number };
}

export interface Diagnostic {
  severity: number; // 1 Error, 2 Warning, 3 Information, 4 Hint
  range: Range;
  message: string;
  source: string;
}

export const Severity = {
  Error: 1,
  Warning: 2,
  Information: 3,
  Hint: 4,
} as const;

// Names that look like call targets but are really engine constants/values.
const SKIP_CONSTANTS = new Set([
  'true',
  'false',
  'null',
  'nil',
  'Pi',
  '2Pi',
  'Self',
  'This',
  // LTSL builtins / operators not registered as functions:
  'deref',
  'address',
  '?',
  '@',
  'else',
  'Array',
  'block',
  'desc',
]);

// Script-side type constructor aliases that also accept scalar conversions
// (e.g. `(Vec3 1.0)`, `(Vec4 0.0)`), so declared arities don't apply.
const TYPE_ALIAS_CONSTRUCTORS = new Set(['Vec2', 'Vec3', 'Vec4']);

export class LTSLAnalyzer {
  public api: APIDatabase;

  constructor(api: APIDatabase) {
    this.api = api;
  }

  analyze(info: DocumentInfo): Diagnostic[] {
    const diags: Diagnostic[] = [];

    for (const p of info.problems) {
      diags.push({
        severity: Severity.Warning,
        range: {
          start: { line: p.line, character: p.column },
          end: { line: p.line, character: p.column + 1 },
        },
        message: p.message,
        source: 'ltsl',
      });
    }

    // Same-file script functions and type methods (LTSL allows overloading).
    const docFunctions = new Map<string, FunctionInfo[]>();
    for (const fn of info.functions) {
      const arr = docFunctions.get(fn.name);
      if (arr) {
        arr.push(fn);
      } else {
        docFunctions.set(fn.name, [fn]);
      }
    }

    const typeNames = new Set(info.types.map(t => t.name));
    const seenTypes = new Set<string>();
    for (const ty of info.types) {
      if (seenTypes.has(ty.name)) {
        diags.push({
          severity: Severity.Error,
          range: {
            start: { line: ty.line, character: ty.column },
            end: { line: ty.line, character: ty.column + Math.max(1, ty.name.length) },
          },
          message: `Type '${ty.name}' is already defined`,
          source: 'ltsl',
        });
      }
      seenTypes.add(ty.name);
    }

    const fieldNamesByOwner = new Map<string, Set<string>>();
    const allFieldNames = new Set<string>();
    for (const ty of info.types) {
      const names = new Set(ty.fields.map(f => f.name));
      fieldNamesByOwner.set(ty.name, names);
      for (const n of names) {
        allFieldNames.add(n);
      }
    }

    const globalVarNames = new Set(info.topLevelVars.map(v => v.name));

    for (const fn of info.functions) {
      this.analyzeFunction(
        fn,
        docFunctions,
        typeNames,
        globalVarNames,
        allFieldNames,
        fieldNamesByOwner,
        diags
      );
    }

    return diags;
  }

  private analyzeFunction(
    fn: FunctionInfo,
    docFunctions: Map<string, FunctionInfo[]>,
    typeNames: Set<string>,
    globalVarNames: Set<string>,
    allFieldNames: Set<string>,
    fieldNamesByOwner: Map<string, Set<string>>,
    diags: Diagnostic[]
  ): void {
    const localNames = new Set<string>();
    for (const v of fn.vars) {
      localNames.add(v.name);
    }
    for (const p of fn.params) {
      localNames.add(p.name);
    }
    for (const g of globalVarNames) {
      localNames.add(g);
    }
    for (const f of allFieldNames) {
      localNames.add(f); // bare field access is valid anywhere in the file
    }
    const ownerFields = fn.typeOwner ? fieldNamesByOwner.get(fn.typeOwner) : undefined;
    if (ownerFields) {
      for (const f of ownerFields) {
        localNames.add(f);
      }
    }

    for (const call of fn.calls) {
      this.analyzeCall(call, localNames, docFunctions, typeNames, diags);
    }
  }

  private analyzeCall(
    call: CallSite,
    localNames: Set<string>,
    docFunctions: Map<string, FunctionInfo[]>,
    typeNames: Set<string>,
    diags: Diagnostic[]
  ): void {
    const name = call.name;
    if (!name || /^[0-9]/.test(name) || name === '0') {
      return;
    }
    if (KEYWORDS.has(name)) {
      return; // control keyword used in a group, e.g. `(cast Int x)`
    }
    if (localNames.has(name)) {
      return; // variable reference, not a call
    }
    if (name.includes('/') || name.includes(':')) {
      return; // namespaced script reference (cross-file); cannot resolve
    }
    if (SKIP_CONSTANTS.has(name)) {
      return;
    }

    const range: Range = {
      start: { line: call.line, character: call.column },
      end: { line: call.line, character: call.column + name.length },
    };

    // Methods: the receiver type is unknown to us, so an unrecognized method
    // name is expected (fields, cross-file widget methods, engine members).
    // Only arg-count-check when the DB knows the method.
    if (call.isMethod) {
      const engineFns = this.api.getFunctions(name);
      if (engineFns.length === 0) {
        return;
      }
      if (call.argCountKnown) {
        // The receiver is the first DB parameter; add it back in.
        const want = call.argCount + 1;
        if (engineFns.every(f => f.parameters.length !== want)) {
          this.pushArgCountError(name, engineFns, call.argCount, 'Method', range, diags);
        }
      }
      return;
    }

    // Same-file script function or type method (overloads allowed).
    const docFns = docFunctions.get(name);
    if (docFns) {
      if (call.argCountKnown && !docFns.some(f => f.params.length === call.argCount)) {
        const counts = [...new Set(docFns.map(f => f.params.length))].sort((a, b) => a - b);
        const countsText = counts.length === 1
          ? `${counts[0]}`
          : `${counts.slice(0, -1).join(', ')} or ${counts[counts.length - 1]}`;
        diags.push({
          severity: Severity.Error,
          range,
          message: `Function '${name}' expects ${countsText} argument(s), got ${call.argCount}`,
          source: 'ltsl',
        });
      }
      return;
    }

    // Engine API function (possibly overloaded / aliased).
    const engineFns = this.api.getFunctions(name);
    if (engineFns.length === 0) {
      if (typeNames.has(name) || this.api.isTypeName(name)) {
        return; // constructor
      }
      // Warning, not error: LTSL scripts routinely call functions defined in
      // other (possibly not-yet-vendored) script files.
      diags.push({
        severity: Severity.Warning,
        range,
        message: `Unknown function '${name}' (not in the engine API or this file)`,
        source: 'ltsl',
      });
      return;
    }

    if (TYPE_ALIAS_CONSTRUCTORS.has(name) || this.isConstructorName(engineFns)) {
      return; // conversions like `(Vec3 1.0)` don't match a declared arity
    }

    if (call.argCountKnown && engineFns.every(f => f.parameters.length !== call.argCount)) {
      this.pushArgCountError(name, engineFns, call.argCount, 'Function', range, diags);
    }
  }

  private isConstructorName(engineFns: { signature: string }[]): boolean {
    return engineFns.some(f => {
      const sigName = f.signature.split('(')[0].trim().split(' ').pop();
      return !!sigName && sigName.endsWith('_Create');
    });
  }

  private pushArgCountError(
    name: string,
    engineFns: { parameters: unknown[] }[],
    got: number,
    kind: string,
    range: Range,
    diags: Diagnostic[]
  ): void {
    const counts = [...new Set(engineFns.map(f => f.parameters.length))].sort((a, b) => a - b);
    const countsText = counts.length === 1
      ? `${counts[0]}`
      : `${counts.slice(0, -1).join(', ')} or ${counts[counts.length - 1]}`;
    diags.push({
      severity: Severity.Error,
      range,
      message: `'${name}' expects ${countsText} argument(s), got ${got}`,
      source: 'ltsl',
    });
  }
}
