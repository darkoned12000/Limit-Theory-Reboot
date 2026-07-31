---
description: Rebuild the LTSL LSP server and the ZED extension wasm
---

Rebuild the LTSL LSP server and the ZED extension wasm bundle.

1. LSP server (TypeScript → JS):
   ```
   npm install
   npm run compile
   ```
   in `script/ltsl-lsp/`. Zero compile errors expected. Output goes to `script/ltsl-lsp/out/`.

2. ZED extension (Rust → wasm), requires `wasm32-wasip2` target:
   ```
   cargo build --release --target wasm32-wasip2
   ```
   in `extensions/ltsl/`. Produces `extensions/ltsl/target/wasm32-wasip2/release/ltsl.wasm` (~166 KB).

3. Optional: regenerate the API database from the engine when C++ APIs changed:
   ```
   cmake --build ./build --target ltsl_api_dump -j
   LD_LIBRARY_PATH=bin:extbin/linux64 ./bin/ltsl_api_dump > script/ltsl-lsp/api-database.json
   ```

Report the wasm file size and confirm both builds completed cleanly.
