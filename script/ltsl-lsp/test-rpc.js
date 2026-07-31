// End-to-end JSON-RPC test of the LTSL language server over stdio.
// Spawns out/server.js, performs initialize / didOpen / hover / completion /
// signatureHelp / didChange / shutdown, and prints results.
const { spawn } = require('child_process');
const path = require('path');

const LSP_DIR = __dirname;
const SERVER = path.join(LSP_DIR, 'out', 'server.js');
const DB = path.join(LSP_DIR, 'api-database.json');

const child = spawn('node', [SERVER, '--stdio'], {
  env: { ...process.env, LTSL_API_DATABASE: DB },
  stdio: ['pipe', 'pipe', 'pipe'],
});

let buf = '';
const pending = new Map();
const published = [];
let nextId = 1;

function send(msg) {
  const body = JSON.stringify(msg);
  const header = `Content-Length: ${Buffer.byteLength(body)}\r\n\r\n`;
  child.stdin.write(header + body);
}

function request(method, params) {
  const id = nextId++;
  return new Promise((resolve, reject) => {
    pending.set(id, { resolve, reject });
    send({ jsonrpc: '2.0', id, method, params });
    setTimeout(() => {
      if (pending.has(id)) {
        pending.delete(id);
        reject(new Error(`timeout: ${method}`));
      }
    }, 5000);
  });
}

function notify(method, params) {
  send({ jsonrpc: '2.0', method, params });
}

child.stdout.on('data', (chunk) => {
  buf += chunk.toString();
  let idx;
  while ((idx = buf.indexOf('\r\n\r\n')) >= 0) {
    const head = buf.slice(0, idx).toString();
    const m = /Content-Length: (\d+)/i.exec(head);
    if (!m) {
      buf = buf.slice(idx + 4);
      continue;
    }
    const len = parseInt(m[1], 10);
    const start = idx + 4;
    if (buf.length < start + len) break;
    const body = buf.slice(start, start + len).toString();
    buf = buf.slice(start + len);
    let msg;
    try { msg = JSON.parse(body); } catch { continue; }
    if (msg.method === 'textDocument/publishDiagnostics') {
      published.push(msg.params);
      continue;
    }
    if (msg.id !== undefined && pending.has(msg.id)) {
      const p = pending.get(msg.id);
      pending.delete(msg.id);
      if (msg.error) p.reject(new Error(JSON.stringify(msg.error)));
      else p.resolve(msg.result);
    }
  }
});

child.stderr.on('data', (d) => process.stderr.write('[server] ' + d));

function lspDiagCount(diags) {
  return diags.map((d) => `${d.severity}:${d.range.start.line}:${d.message.slice(0, 60)}`);
}

async function main() {
  const uri = 'file:///repo/resource/script/App/ltheory-main.lts';
  const text = `function Object Main (Int seed)
  var rng (RNG_MTG seed)
  var system (Object_System (Vec3 15.012) seed)
  system
`;

  const init = await request('initialize', {
    processId: process.pid,
    rootUri: 'file:///repo',
    capabilities: {},
  });
  notify('initialized', {});

  notify('textDocument/didOpen', {
    textDocument: { uri, languageId: 'ltsl', version: 1, text },
  });

  // diagnostics arrive as notifications
  const diagPromise = new Promise((resolve) => {
    const handler = (data) => {
      child.stdout.removeListener('data', handler);
      resolve(data);
    };
  });
  // We'll instead poll: send hover first, then check diagnostics were delivered.
  await new Promise((r) => setTimeout(r, 300));

  const hover = await request('textDocument/hover', {
    textDocument: { uri },
    position: { line: 2, character: 20 },
  });

  const completion = await request('textDocument/completion', {
    textDocument: { uri },
    position: { line: 3, character: 2 },
  });

  const sig = await request('textDocument/signatureHelp', {
    textDocument: { uri },
    position: { line: 2, character: 27 },
  });

  notify('textDocument/didChange', {
    textDocument: { uri, version: 2 },
    contentChanges: [{ text: text + '  var broken (\n' }],
  });
  await new Promise((r) => setTimeout(r, 200));

  const shutdown = await request('shutdown', null);
  notify('exit', {});

  console.log('initialize ok, capabilities loaded');
  console.log('hover:', JSON.stringify(hover && hover.contents && hover.contents.value).slice(0, 120));
  console.log('completion count:', completion ? completion.length : 0);
  console.log('signatureHelp:', JSON.stringify(sig && sig.signatures && sig.signatures[0] && sig.signatures[0].label));
  const lastPub = published[published.length - 1];
  const brokenParenDiag = lastPub && lastPub.diagnostics.find((d) => /Unbalanced/.test(d.message));
  console.log('diagnostics:', published.length, 'publish(es), broken-paren diagnostic:', brokenParenDiag ? brokenParenDiag.message : 'NOT FOUND');
  console.log('didChange + shutdown ok');
  child.kill();
}

main().catch((e) => {
  console.error('FAIL:', e.message);
  child.kill();
  process.exit(1);
});
