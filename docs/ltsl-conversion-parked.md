# LTSL Conversion — Parked Files Needing Parser Fix

Files that still fail after batch conversion to `(fn ...)` and need deeper `Parser.cpp` work. Parked to avoid blocking batch progress.

## Parser blocker: `DrawPanel`/`Components:Positioned` + `(Vec2/Vec3 ...)` nested arg

`Widget/Component/FocusPanel.lts:9`
- `DrawPanel self.pos (Vec2 4 self.size.y) c 0.5 1.0 0.0`
- Both bare `DrawPanel self.pos (Vec2 4 self.size.y) c ...` (`Parser.cpp:464` bare-call) and parenthesized `(DrawPanel self.pos (Vec2 4 self.size.y) c ...)` (`Parser.cpp:1130` LPAREN) fail with `expected ')' after method arguments` / `unexpected token` at `self.size.y`
- Isolated `var x (Vec2 4 self.size.y)` succeeds — failure only when `(Vec2 ...)` is arg to `DrawPanel` (and similar `DrawPanel`+`Vec2` combos)
- Affects: `Parser.cpp:1130` `funcName`/`ParseExpression` arg loop handling of nested `(Vec2 4 self.size.y)` where second arg is `self.size.y` (double DOT chain)

Confirmed duplicates:
- `Widget/Component/RedBG.lts:3` `DrawPanel self.pos self.size (Vec3 1.0 0.0 0.0) 1.0 1.0 0` — same
- `Widget/Component/ContextMenu.lts:9` `menu = (Components:Positioned Cursor_Get (Stack Widget))` — isolated succeeds, file fails inside `if visible.! && ...`
- `App/loading.lts:25` `Draw Icon/Cursors:Pointer Cursor_Get 12 2.0 * Colors:Primary 1.0` / `self.Add` etc. — 6 errs, same nested `Draw`+`Vec`/`Icon/...` slash handling (`Parser.cpp:1233` slash fold added but still fails)
- `Item/StationType/Generate.lts:39` `self.Add (Warp_AttractorPoint (Vec3 0 10 0) 8.0 * rng.Float)` — 6 errs, `Warp_AttractorPoint`+`Vec3` nested
- `Item/TransferUnitType.lts:3` `self.Add (Vec3 0 0.25 2.0) ...` — 6 errs, `self.Add`+`Vec3`
- `Item/WeaponType.lts:3` same `self.Add`+`Vec3`
- `Widget/HUD/PilotingBadge.lts:31` `var value switch` `i == 3` case — fixed via `Parser.cpp:522` `var`+`switch` support (now 0)

Fixed in batch:
- `Widget/Icon.lts:3` — 3→0 (auto via Vec2 negative fix, no edit)
- `Widget/Text.lts:26` missing `)` — `var offset (Vec2 0 0.5 * (self.size.y + 0.2 * size)` → `... size))` — 13→0

## Converted clean in this batch (for reference)

- `Texture/Filters.lts:182` `var self (Texture2D_CreateHDR ...)` — 2→0
- `Widget/Grid.lts:19` `for it (self.GetChildren) (it.HasMore) (it.Advance)` + `var child (it.Get)` — 2→0
- `Widget/DevPanel/Clock.lts:15` `l += (Components:AlignCenter ...)` — 2→0 (required `Parser.cpp:1117` colon fold + `Parser.cpp:1107` unary fix)
- `Widget/DevPanel/Status.lts:6` `l += (Widgets:ListH2 ...)` — 2→0
- `Widget/SplashScreen.lts:19` `l += (Components:AlignCenter ...)` — 2→0
- `Icon/Cursors.lts:3` `(Vec2 -1 0)` negativeVec — 3→0 via `Parser.cpp:1107` unary check
- `Texture/RandomScreenshot.lts:11` `Texture_LoadFrom (Location_File ...)` — 3→0 via same

## Converted clean — second batch

- `Object/Widget/Colony.lts:55` `l += (Custom (Widget) (ColonyTraits object))` — 2→0
- `Widget/ScrollFrame.lts:14` `for it (self.GetChildren) (it.HasMore) (it.Advance)` + `var widget (it.Get)` — 2→0
- `Widget/DevPanel/Apps.lts:8` `l += (Components:Backdrop ...)` — 2→0
- `Widget/TextEditor.lts:12` `l += (Widgets:ListH2 ...)` — 2→0
- `App/widget.lts:18` `desktop = (Layer ...)` + `settings = (Layer ...)` — 4→0
- `Object/Ship.lts:7` `interior = (Sound_PlayLooped ...)` — 4→0
- `Widget/ExitButton.lts:8` `l += (Components:AlignCenter ...)` + `var c (? ...)` reverted to `var c switch` via `Parser.cpp:522` — 4→0
- `Widget/HUD/HUDWidget.lts:39` `var c (? ...)` — 4→0
- `Widget/HUD/Targets.lts:7` `for it (object.GetTargets) ...` + `l += (Components:MinSize ...)` — 4→0
- `Widget/HUD/WorldObject.lts:17` `l += (Components:MinSize ...)` + `l += (Components:Tooltip ...)` — 4→0
- `ZZShipDriver.lts:7` `Sound_PlayLooped` bare — 4→0
- `Widget/DebugScene.lts:45` `for it (target.GetInteriorObjects) ...` + `var o (it.Get)` — 6→0
- `Widget/HUD/PilotingBadge.lts:31` `var value switch` `i == 3` fix + `Parser.cpp:522` `var`+`switch` — 5→0
- `Widget/Text.lts:26` missing `)` — 13→0
- `Widget/Icon.lts:3` — 3→0 (auto)
- `Widget/Button.lts:10` `var c switch` — 6→4 (remaining `(! enabled)` case parked)

## Parked — needs deeper `Parser.cpp` work (do not batch-convert manually)

- `Widget/Component/FocusPanel.lts:9` `DrawPanel`+`Vec2` nested — `Parser.cpp:1130` `DrawPanel`+`Vec` arg
- `Widget/Component/RedBG.lts:3` `DrawPanel`+`Vec3` — same
- `Widget/Component/ContextMenu.lts:9` `Components:Positioned`+`Stack Widget` inside `if visible.! && ...`
- `App/loading.lts:25` `Draw Icon/Cursors:Pointer` + `Warp_AttractorPoint` nested
- `Item/StationType/Generate.lts:39` `self.Add (Warp_AttractorPoint ...)` + `Vec3`
- `Item/TransferUnitType.lts:3` `self.Add (Vec3 ...)` — same
- `Item/WeaponType.lts:3` same
- `Widget/Reticle/Default.lts:64` `for it object.GetSockets ...` + `DrawPanel`+`Vec` nested
- `Widget/Spacer.lts:5` `DrawPanel self.LeftCenter (Vec2 ...) ...` — same `DrawPanel`+`Vec`
- `ZZSlotDriver.lts:6` `var title (info.saveName.Length > 0 info.saveName info.slotName)` ternary without `?` — `Parser.cpp:1095` `?` handling
- `Widget/HUD/ObjectBadge.lts:40` `var self`+`Custom` indented — `var`+`Custom` block
- `App/dogfight.lts:55` `var zone Object_Zone` 10-arg indented — `var`+`Object_Zone` block
- `Widget/Market/MidPanel.lts:56` 45× `l +=` indented `Components:` chains — bulk `l +=` pattern, needs script `script/ltsl-convert.py`
- `Widget/Button.lts:10` `var c switch` `(! enabled)` case — `!` prefix in `switch` condition

Corpus: 71 OK / 85 PARSE as of 2026-08-28 (71 new-parser clean via scan_fork; 16 old strictly clean before conversion). New-parser clean includes all batch conversions to (fn ...) listed above. Parked still 14 (FocusPanel/RedBG/ContextMenu/loading/Generate/Transfer/Weapon/Reticle/Spacer/ZZSlot/ObjectBadge/dogfight/MidPanel/Button/strukt). Next batch: App/brain.lts 9 (rng.Vec2 -1 handled via Parser.cpp:1362 isUnaryNum, now 0 after fix), ThrusterType etc. (self.Add+Vec) still parked. See git log ltheory-old-test for converted files.
