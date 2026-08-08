---
description: Run the LTSL LSP diagnostics smoke over the full script corpus
---

Run the full-corpus LTSL LSP smoke test and report the results.

1. Build first if `script/ltsl-lsp/out/` is missing or stale (`npm run compile` in `script/ltsl-lsp/`).
2. From the repo root, run:
   ```
   node script/ltsl-lsp/out/smoke.js $(find resource/script -name '*.lts' | sort)
   ```
3. Expected state: **8 diagnostics total** (4 structural problems + 4 warnings).
   - The 4 structural problems are the known unbalanced-paren engine bugs: `resource/script/App/draw.lts:57`, `App/draw.lts:58`, `Widget/Slider.lts:42`, `Widget/Text.lts:26` (these are trusted fixtures — do not "fix" them).
   - The 4 warnings are known/accepted: `SelectItem` (`Widget/Market/MidPanel.lts:108`), `WidgetSettings` (`Widget/Settings.lts:13`), `break` (`App/ltheory-unitest.lts:201`, WIP app uses a non-LTSL `break`), and `RenderPass_Bloom` (`App/ltheory-main.lts:217`, comma-paren call the analyzer counts as 1 arg but the engine compiles as 2 — valid at runtime).
   Any count higher than 8 means a regression — investigate the newly introduced diagnostics before continuing.
