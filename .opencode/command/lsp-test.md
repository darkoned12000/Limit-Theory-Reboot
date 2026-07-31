---
description: Run the LTSL LSP end-to-end JSON-RPC protocol test
---

Run the LTSL LSP server's end-to-end JSON-RPC test and confirm every stage passes.

1. Ensure the server is compiled: `npm run compile` in `script/ltsl-lsp/`.
2. From the repo root, run:
   ```
   node script/ltsl-lsp/test-rpc.js
   ```
3. The test drives the real `out/server.js` over stdio and asserts: initialize capabilities (textDocumentSync 2, hover, completion trigger `.`, signatureHelp trigger `(` and `.`), hover output (`Object Object_System(Vec3d position, Uint32 seed)`), completion count (1946), signatureHelp label, diagnostics publish on didOpen/didChange (must include the broken-paren diagnostic), and a clean shutdown.
   Report any failing stage.
