#!/usr/bin/env python3
# Copyright (C) 2025  darkoned12000
# SPDX-License-Identifier: GPL-3.0-or-later
# Part of the ltheory-old-test modernization effort (Revamp Work).
# See NOTICE and LICENSE.GPL. Original engine (c) Josh Parnell, public domain.
#
# Alias-ordering invariant checker for the LTSL binding bridge replacement.
# Function_AddAlias copies the *current* source bucket (Function.cpp:62), so an
# alias that appears before its source silently registers an empty bucket.
# This script fails any alias whose source name is not registered (or itself
# aliased) textually earlier in the same file. It understands both the old
# (FunctionAlias(A, B)) and new (Function_Alias("A", "B")) syntax so it stays
# usable through the mixed migration state (Steps 2-9). Alias chains (A->B,
# then B->C) are handled because aliases contribute both their names. It is
# intentionally lenient (permissive regexes) - its job is to catch ordering
# regressions, not to be a full parser.
#
# KNOWN_EXCEPTIONS: pre-existing broken aliases whose source is never
# registered. These are genuine bugs scheduled to be fixed during the
# migration (they change the API DB, so they are whitelisted in the gate-3
# byte-diff rather than fixed on the baseline). A NEW violation is a hard
# failure; a KNOWN exception is reported but tolerated.
#
#   src/liblt/LTE/ScriptAPI/V2.cpp  'Vec2_Distance'  - copy-paste bug (binds
#     Vec3_Distance with V2 params); alias of a never-registered name. See
#     ltsl-binding-bridge-replacement.md SS6.8 / SS11 (default: keep both,
#     whitelist +1 in gate 3; deferred).
#   (V4.cpp 'Vec4_Dot' was a known exception pre-migration: it registered
#     Vec4f_Dot but aliased the never-registered Vec4_Dot, so the V4F 'Dot'
#     overload was missing. FIXED during migration by pointing the alias at
#     Vec4f_Dot - adds one 'Dot' entry to the API DB, whitelisted in gate 3.)
#
# Usage:
#   python3 script/check_binding_alias_order.py $(git ls-files 'src/liblt/**/*.cpp' 'src/liblt/**/*.h')

import re, sys

ALIAS_NEW = re.compile(r'Function_Alias\(\s*"([^"]+)"\s*,\s*"([^"]+)"\s*\)')
ALIAS_OLD = re.compile(r'\bFunctionAlias\(([A-Za-z_]\w*)\s*,')
BIND_OPEN = re.compile(r'Function_(?:Bind|Bind_Member|Create)\(')
BIND_NAME = re.compile(r'Function_(?:Bind|Bind_Member|Create)\(\s*"([^"]+)"')
NAMES = [
  BIND_NAME,
  re.compile(r'Function_Alias\(\s*"([^"]+)"\s*,\s*"([^"]+)"\s*\)'),
  re.compile(r'\bFunctionAlias\(([A-Za-z_]\w*)\s*,\s*([A-Za-z_]\w*)'),
  re.compile(r'\bDefineFunction\(([A-Za-z_]\w*)'),
  re.compile(r'\bDeclareFunction(?:ArgBind)?\(([A-Za-z_]\w*)'),
  re.compile(r'\bVoidFreeFunction(?:NoParams)?\(([A-Za-z_]\w*)'),
  re.compile(r'\bFreeFunction(?:NoParams)?\([^,]+,\s*([A-Za-z_]\w*)'),
]

NAME_STR = re.compile(r'"([A-Za-z_]\w*)"')

KNOWN_EXCEPTIONS = {
  ('src/liblt/LTE/ScriptAPI/V2.cpp', 'Vec2_Distance'),
}

def names_on(line):
  out = set()
  for p in NAMES:
    for m in p.finditer(line):
      out.update(g for g in m.groups() if g)
  return out

def main(paths):
  bad, known, checked = [], [], 0
  for path in paths:
    lines = open(path, encoding='utf-8').read().splitlines()
    seen = set()
    pending_bind = False
    for i, line in enumerate(lines):
      if line.lstrip().startswith('#define'):
        continue
      # Multiline Function_Bind( / Function_Alias( whose name is on the next
      # line (the migration emits `Function_Bind(` then the name on its own
      # line). Capture the first string literal of the call as the name.
      if pending_bind:
        m2 = NAME_STR.search(line)
        if m2:
          seen.add(m2.group(1))
          pending_bind = False
      m = ALIAS_NEW.search(line) or ALIAS_OLD.search(line)
      if m:
        checked += 1
        if m.group(1) not in seen:
          if (path, m.group(1)) in KNOWN_EXCEPTIONS:
            known.append((path, i + 1, m.group(1), line.strip()))
          else:
            bad.append((path, i + 1, m.group(1), line.strip()))
      if BIND_OPEN.search(line) and not BIND_NAME.search(line):
        pending_bind = True
      seen |= names_on(line)
  for p, ln, src, line in known:
    print(f"KNOWN: {p}:{ln}: alias of {src!r} before its source (documented exception): {line}")
  if bad:
    for p, ln, src, line in bad:
      print(f"FAIL: {p}:{ln}: alias of {src!r} appears before its source: {line}")
    print(f"FAIL: {len(bad)} new violation(s) of {checked} alias sites ({len(known)} known exceptions)")
    sys.exit(1)
  print(f"OK: {checked} alias sites follow their source ({len(known)} known exceptions)")

if __name__ == '__main__':
  main(sys.argv[1:])
