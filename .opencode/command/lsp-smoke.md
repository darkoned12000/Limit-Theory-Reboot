---
description: Run the LTSL LSP diagnostics smoke over the full script corpus
---

Run the full-corpus LTSL LSP smoke test and report the results.

1. Build first if `script/ltsl-lsp/out/` is missing or stale (`npm run compile` in `script/ltsl-lsp/`).
2. From the repo root, run:
   ```
   node script/ltsl-lsp/out/smoke.js $(find resource/script -name '*.lts' | sort)
   ```
3. Expected state: **6 diagnostics total** (4 structural problems + 2 cross-file warnings).
   - The 4 structural problems are the known unbalanced-paren engine bugs: `resource/script/App/draw.lts:57`, `App/draw.lts:58`, `Widget/Slider.lts:42`, `Widget/Text.lts:26` (these are trusted fixtures — do not "fix" them).
   - The 2 warnings are legitimate cross-file symbols: `SelectItem` (`Widget/Market/MidPanel.lts:108`) and `WidgetSettings` (`Widget/Settings.lts:13`).
   Any count higher than 6 means a regression — investigate the newly introduced diagnostics before continuing.
