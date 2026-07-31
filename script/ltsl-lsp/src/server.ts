// Copyright (C) 2025  darkoned12000
// SPDX-License-Identifier: GPL-3.0-or-later
// Part of the ltheory-old-test modernization effort (Revamp Work).
// See NOTICE and LICENSE.GPL. Original engine (c) Josh Parnell, public domain.

// LTSL language server: diagnostics, completion, hover and signature help
// backed by the engine-generated API database (script/ltsl-lsp/api-database.json).

import {
  createConnection,
  ProposedFeatures,
  TextDocuments,
  TextDocumentSyncKind,
  CompletionItem,
  CompletionItemKind,
  Diagnostic as LSPDiagnostic,
  Hover,
  MarkupKind,
  Position,
  SignatureHelp,
  SignatureInformation,
  TextDocumentPositionParams,
} from 'vscode-languageserver/node';
import { TextDocument } from 'vscode-languageserver-textdocument';
import { APIDatabase, resolveDatabasePath } from './api-database';
import { LTSLAnalyzer, Diagnostic as AnalyzerDiagnostic } from './analyzer';
import { LTSLParser, DocumentInfo } from './parser';
import { LTSLLexer } from './lexer';

const connection = createConnection(ProposedFeatures.all);

let api: APIDatabase = new APIDatabase();
const analyzer = new LTSLAnalyzer(api);
const parser = new LTSLParser();
const lexer = new LTSLLexer();

// Document state keyed by URI: full text + last parse + last diagnostics.
interface DocState {
  text: string;
  info: DocumentInfo;
  diags: LSPDiagnostic[];
}
const states = new Map<string, DocState>();

function toLSPDiagnostic(d: AnalyzerDiagnostic): LSPDiagnostic {
  return {
    severity: d.severity as LSPDiagnostic['severity'],
    range: d.range,
    message: d.message,
    source: d.source,
  };
}

function analyzeText(text: string): { info: DocumentInfo; diags: LSPDiagnostic[] } {
  const info = parser.parse(lexer.tokenize(text));
  return { info, diags: analyzer.analyze(info).map(toLSPDiagnostic) };
}

function refresh(doc: TextDocument): void {
  const { info, diags } = analyzeText(doc.getText());
  states.set(doc.uri, { text: doc.getText(), info, diags });
}

function getState(uri: string): DocState | undefined {
  let state = states.get(uri);
  const doc = documents.get(uri);
  if (!state && doc) {
    refresh(doc);
    state = states.get(uri);
  }
  return state;
}

const documents: TextDocuments<TextDocument> = new TextDocuments<TextDocument>({
  create: (uri, languageId, version, content) => TextDocument.create(uri, languageId, version, content),
  update: (doc, changes, version) => TextDocument.update(doc, changes, version),
});

documents.onDidChangeContent((event: { document: TextDocument }) => {
  refresh(event.document);
  const state = states.get(event.document.uri);
  if (state) {
    connection.sendDiagnostics({ uri: event.document.uri, diagnostics: state.diags });
  }
});

documents.onDidSave((event: { document: TextDocument }) => {
  refresh(event.document);
  const state = states.get(event.document.uri);
  if (state) {
    connection.sendDiagnostics({ uri: event.document.uri, diagnostics: state.diags });
  }
});

function toPosition(offset: number, doc: TextDocument): Position {
  return doc.positionAt(offset);
}

connection.onHover((params: TextDocumentPositionParams): Hover | null => {
  const doc = documents.get(params.textDocument.uri);
  const state = getState(params.textDocument.uri);
  if (!doc || !state) {
    return null;
  }
  const offset = doc.offsetAt(params.position);
  const candidates: { name: string; start: number; end: number }[] = [];
  for (const fn of state.info.functions) {
    candidates.push({ name: fn.name, start: fn.start, end: fn.end });
    for (const call of fn.calls) {
      candidates.push({ name: call.name, start: call.start, end: call.end });
    }
  }
  for (const ty of state.info.types) {
    candidates.push({ name: ty.name, start: ty.start, end: ty.end });
    for (const m of ty.methods) {
      for (const call of m.calls) {
        candidates.push({ name: call.name, start: call.start, end: call.end });
      }
    }
  }
  let best: { name: string; start: number; end: number } | null = null;
  for (const c of candidates) {
    if (offset >= c.start && offset <= c.end && (!best || c.end - c.start < best.end - best.start)) {
      best = c;
    }
  }
  if (!best) {
    return null;
  }
  const fn = api.getFunctions(best.name)[0];
  const type = api.getType(best.name);
  const content = fn
    ? `\`\`\`ltsl\n${fn.signature}\n\`\`\`\n\n${fn.documentation || ''}`
    : type
      ? `\`\`\`ltsl\ntype ${type.name}\n\`\`\`\n\n${type.description || ''}`
      : `\`\`\`ltsl\n${best.name}\n\`\`\``;
  return {
    contents: { kind: MarkupKind.Markdown, value: content },
    range: { start: toPosition(best.start, doc), end: toPosition(best.end, doc) },
  };
});

connection.onCompletion((params: TextDocumentPositionParams): CompletionItem[] | null => {
  const doc = documents.get(params.textDocument.uri);
  if (!doc) {
    return null;
  }
  const textBefore = doc.getText({
    start: { line: params.position.line, character: 0 },
    end: params.position,
  });
  const match = textBefore.match(/([A-Za-z_][A-Za-z0-9_.]*)$/);
  const prefix = match ? match[1] : '';
  const items: CompletionItem[] = [];
  const added = new Set<string>();
  const push = (label: string, kind: CompletionItemKind) => {
    if (!added.has(label)) {
      added.add(label);
      items.push({ label, kind });
    }
  };
  for (const name of api.functionNamesMatching(prefix)) {
    const fn = api.getFunctions(name)[0];
    push(name, fn && fn.parameters.length > 0 ? CompletionItemKind.Function : CompletionItemKind.Variable);
  }
  for (const name of api.typeNamesMatching(prefix)) {
    push(name, CompletionItemKind.Struct);
  }
  const state = states.get(doc.uri);
  if (state) {
    for (const fn of state.info.functions) {
      push(fn.name, CompletionItemKind.Function);
    }
    for (const v of state.info.topLevelVars) {
      push(v.name, CompletionItemKind.Variable);
    }
  }
  return items;
});

connection.onSignatureHelp((params: TextDocumentPositionParams): SignatureHelp | null => {
  const doc = documents.get(params.textDocument.uri);
  if (!doc) {
    return null;
  }
  const textBefore = doc.getText({
    start: { line: params.position.line, character: 0 },
    end: params.position,
  });
  const match = textBefore.match(/([A-Za-z_][A-Za-z0-9_.]*)\s*$/);
  if (!match) {
    return null;
  }
  const fns = api.getFunctions(match[1]);
  if (fns.length === 0) {
    return null;
  }
  const signatures: SignatureInformation[] = fns.map(f => ({
    label: f.signature,
    documentation: f.documentation || '',
    parameters: f.parameters.map(p => ({
      label: `${p.name}: ${p.type}`,
      documentation: p.type ? `type ${p.type}` : '',
    })),
  }));
  return {
    signatures,
    activeSignature: signatures.length === 1 ? 0 : 0,
    activeParameter: 0,
  };
});

connection.onInitialize(() => {
  const dbPath = resolveDatabasePath();
  if (!dbPath) {
    connection.console.error(
      'ltsl: could not locate api-database.json (set LTSL_API_DATABASE, or run the engine dump)'
    );
  } else {
    api.loadFromFile(dbPath);
    connection.console.info(
      `ltsl: loaded ${api.functionCount} functions, ${api.typeCount} types from ${dbPath}`
    );
  }
  analyzer.api = api;
  return {
    capabilities: {
      textDocumentSync: TextDocumentSyncKind.Incremental,
      hoverProvider: true,
      completionProvider: { triggerCharacters: ['.'] },
      signatureHelpProvider: { triggerCharacters: ['(', '.'] },
    },
  };
});

documents.listen(connection);
connection.listen();
