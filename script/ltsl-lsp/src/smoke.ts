// Copyright (C) 2025  darkoned12000
// SPDX-License-Identifier: GPL-3.0-or-later
// Part of the ltheory-old-test modernization effort (Revamp Work).
// See NOTICE and LICENSE.GPL. Original engine (c) Josh Parnell, public domain.

// Standalone smoke test: run lexer + parser + analyzer over real .lts files
// and print diagnostics. Exit code is 0 when analysis completes without a
// crash; the known-unbalanced-paren fixtures must surface warnings.

import * as fs from 'fs';
import * as path from 'path';
import { APIDatabase, resolveDatabasePath } from './api-database';
import { LTSLAnalyzer } from './analyzer';
import { LTSLParser } from './parser';
import { LTSLLexer } from './lexer';

const dbPath = resolveDatabasePath();
if (!dbPath) {
  console.error('smoke: no api-database.json found');
  process.exit(1);
}
const api = new APIDatabase();
api.loadFromFile(dbPath);

const lexer = new LTSLLexer();
const parser = new LTSLParser();
const analyzer = new LTSLAnalyzer(api);

const files = process.argv.slice(2);
if (files.length === 0) {
  console.error('usage: node out/smoke.js <file.lts> [...]');
  process.exit(1);
}

let totalDiags = 0;
let totalProblems = 0;
for (const file of files) {
  const text = fs.readFileSync(file, 'utf-8');
  const info = parser.parse(lexer.tokenize(text));
  const diags = analyzer.analyze(info);
  totalDiags += diags.length;
  totalProblems += info.problems.length;
  console.log(`${file}: ${diags.length} diagnostics (${info.problems.length} structural problems), ` +
    `${info.functions.length} functions, ${info.types.length} types, ` +
    `${info.topLevelVars.length} top-level vars`);
  for (const d of diags) {
    const sev = d.severity === 1 ? 'ERROR  ' : d.severity === 2 ? 'WARNING' : 'INFO   ';
    console.log(`  ${sev} ${d.range.start.line + 1}:${d.range.start.character + 1} ${d.message}`);
  }
}

console.log(`\ntotal: ${totalDiags} diagnostics, ${totalProblems} structural problems across ${files.length} file(s)`);
