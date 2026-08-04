#!/usr/bin/env python3
# Copyright (C) 2025  darkoned12000
# SPDX-License-Identifier: GPL-3.0-or-later
# Part of the ltheory-old-test modernization effort (Revamp Work).
# See NOTICE and LICENSE.GPL. Original engine (c) Josh Parnell, public domain.
#
# Mechanical migrator for the LTSL binding-bridge replacement (Step 3).
# Converts the inline FreeFunction/VoidFreeFunction/FreeFunctionNoParams/
# VoidFreeFunctionNoParams macro sites (Function_Generated.h) into the new
# Function_Bind / Function_Alias form (§6.1 of ltsl-binding-bridge-replacement.md).
#
# Usage: python3 script/migrate_freefunction.py file1.cpp file2.cpp ...
#
# Guarantees:
#   - Only the four macro families are touched; everything else is preserved
#     byte-for-byte.
#   - The alias stays after its source (Function_AddAlias snapshots the bucket).
#   - Same-name overloads in namespace wrappers keep distinct static names.
#   - The API-DB byte-diff (gate 3) is the correctness net: run after each chunk.

import re
import sys

MACROS = [
    ("FreeFunctionNoParams", 3),      # RT, Name, Desc
    ("VoidFreeFunctionNoParams", 2),  # Name, Desc
    ("VoidFreeFunction", 0),          # Name, Desc, params...
    ("FreeFunction", 0),              # RT, Name, Desc, params...
]

ALIAS_RE = re.compile(r'FunctionAlias\(([A-Za-z_]\w*)\s*,\s*([^)]*)\)\s*;?')


class State:
    NORMAL = 0
    STRING = 1
    CHAR = 2
    LINE_COMMENT = 3
    BLOCK_COMMENT = 4


def _classify(text, i):
    c = text[i]
    if c == '"':
        return State.STRING
    if c == "'":
        return State.CHAR
    if c == '/' and i + 1 < len(text):
        n = text[i + 1]
        if n == '/':
            return State.LINE_COMMENT
        if n == '*':
            return State.BLOCK_COMMENT
    return State.NORMAL


def skip_ws_comments(text, i):
    """Advance i past whitespace and C/C++ comments. Returns new index."""
    n = len(text)
    while i < n:
        c = text[i]
        if c in ' \t\r\n':
            i += 1
        elif c == '/' and i + 1 < n and text[i + 1] == '/':
            j = text.find('\n', i)
            i = n if j < 0 else j + 1
        elif c == '/' and i + 1 < n and text[i + 1] == '*':
            j = text.find('*/', i + 2)
            i = n if j < 0 else j + 2
        else:
            break
    return i


def read_balanced(text, i, open_c, close_c):
    """text[i] == open_c. Return (content, index_after_close)."""
    n = len(text)
    depth = 0
    j = i
    state = State.NORMAL
    start = i + 1
    while j < n:
        c = text[j]
        if state == State.STRING:
            if c == '\\':
                j += 2
                continue
            if c == '"':
                state = State.NORMAL
        elif state == State.CHAR:
            if c == '\\':
                j += 2
                continue
            if c == "'":
                state = State.NORMAL
        elif state == State.LINE_COMMENT:
            if c == '\n':
                state = State.NORMAL
        elif state == State.BLOCK_COMMENT:
            if c == '*' and j + 1 < n and text[j + 1] == '/':
                j += 2
                state = State.NORMAL
                continue
        else:
            st = _classify(text, j)
            if st != State.NORMAL:
                state = st
            elif c == open_c:
                depth += 1
            elif c == close_c:
                depth -= 1
                if depth == 0:
                    return text[start:j], j + 1
        j += 1
    raise ValueError("unbalanced %s at offset %d" % (open_c, i))


def split_top_level(s):
    """Split on commas at paren/brace/angle depth 0, string/comment aware."""
    parts = []
    depth_p = depth_b = depth_a = 0
    state = State.NORMAL
    cur = []
    n = len(s)
    j = 0
    while j < n:
        c = s[j]
        if state == State.STRING:
            cur.append(c)
            if c == '\\':
                if j + 1 < n:
                    cur.append(s[j + 1])
                    j += 1
            elif c == '"':
                state = State.NORMAL
        elif state == State.CHAR:
            cur.append(c)
            if c == '\\':
                if j + 1 < n:
                    cur.append(s[j + 1])
                    j += 1
            elif c == "'":
                state = State.NORMAL
        elif state == State.LINE_COMMENT:
            cur.append(c)
            if c == '\n':
                state = State.NORMAL
        elif state == State.BLOCK_COMMENT:
            cur.append(c)
            if c == '*' and j + 1 < n and s[j + 1] == '/':
                cur.append('/')
                j += 1
                state = State.NORMAL
        else:
            st = _classify(s, j)
            if st == State.STRING:
                state = State.STRING
                cur.append(c)
            elif st == State.CHAR:
                state = State.CHAR
                cur.append(c)
            elif st == State.LINE_COMMENT:
                state = State.LINE_COMMENT
                cur.append(c)
            elif st == State.BLOCK_COMMENT:
                state = State.BLOCK_COMMENT
                cur.append(c)
                if j + 1 < n:
                    cur.append(s[j + 1])
                    j += 1
            elif c == '(':
                depth_p += 1
                cur.append(c)
            elif c == ')':
                depth_p -= 1
                cur.append(c)
            elif c == '{':
                depth_b += 1
                cur.append(c)
            elif c == '}':
                depth_b -= 1
                cur.append(c)
            elif c == '<':
                depth_a += 1
                cur.append(c)
            elif c == '>':
                if depth_a > 0:
                    depth_a -= 1
                cur.append(c)
            elif c == ',' and depth_p == 0 and depth_b == 0 and depth_a == 0:
                parts.append(''.join(cur))
                cur = []
            else:
                cur.append(c)
        j += 1
    parts.append(''.join(cur))
    return parts


def parse_params(args, start):
    """args[start:...] are alternating type/name tokens. Return (pairs, name_list)."""
    pairs = []
    names = []
    rest = args[start:]
    assert len(rest) % 2 == 0, "odd param tokens: %r" % (rest,)
    for k in range(0, len(rest), 2):
        t = rest[k].strip()
        nm = rest[k + 1].strip()
        pairs.append((t, nm))
        names.append(nm)
    return pairs, names


def make_registration(macro, args, body):
    if macro == "FreeFunction":
        rt, name, desc = args[0].strip(), args[1].strip(), args[2]
        pairs, names = parse_params(args, 3)
        ret = " -> %s" % rt
    elif macro == "VoidFreeFunction":
        name, desc = args[0].strip(), args[1]
        pairs, names = parse_params(args, 2)
        ret = ""
    elif macro == "FreeFunctionNoParams":
        rt, name, desc = args[0].strip(), args[1].strip(), args[2]
        pairs, names = [], []
        ret = " -> %s" % rt
    else:  # VoidFreeFunctionNoParams
        name, desc = args[0].strip(), args[1]
        pairs, names = [], []
        ret = ""

    params = ", ".join("%s const& %s" % (t, nm) for t, nm in pairs)

    # body is the text after '{'; strip only the immediate leading newline so
    # the first statement keeps its original indentation.
    body_open = body[1:] if body.startswith('\n') else body
    body_lines = body_open.split('\n')
    if body_lines and body_lines[0].strip() == '':
        body_lines = body_lines[1:]
    inner = "\n".join(body_lines).rstrip('\n')

    close = ", ".join('"%s"' % nm for nm in names) if names else ""
    open_block = "  {"
    close_block = "  });" if not close else "  },\n  %s);" % close

    text = (
        "static Function const %s_Registration = Function_Bind(\n"
        '  "%s",\n'
        "  %s,\n"
        "  [](%s)%s\n"
        "%s\n"
        "%s\n"
        "%s"
    ) % (name, name, desc.strip(), params, ret, open_block,
         inner if inner else "", close_block)
    return text


def migrate(text):
    out = []
    n = len(text)
    pos = 0
    while pos < n:
        # Find next macro invocation in code state (not in string/comment).
        best = None
        j = pos
        state = State.NORMAL
        while j < n:
            c = text[j]
            if state == State.STRING:
                if c == '\\':
                    j += 2
                    continue
                if c == '"':
                    state = State.NORMAL
            elif state == State.CHAR:
                if c == '\\':
                    j += 2
                    continue
                if c == "'":
                    state = State.NORMAL
            elif state == State.LINE_COMMENT:
                if c == '\n':
                    state = State.NORMAL
            elif state == State.BLOCK_COMMENT:
                if c == '*' and j + 1 < n and text[j + 1] == '/':
                    j += 2
                    state = State.NORMAL
                    continue
            else:
                st = _classify(text, j)
                if st != State.NORMAL:
                    state = st
                else:
                    for mname, _arity in MACROS:
                        if text.startswith(mname, j) and j + len(mname) < n and text[j + len(mname)] == '(':
                            best = (j, mname)
                            break
                    if best:
                        break
            j += 1
        if best is None:
            out.append(text[pos:])
            break
        start, mname = best
        out.append(text[pos:start])
        # read balanced parens
        open_i = start + len(mname)
        content, after_paren = read_balanced(text, open_i, '(', ')')
        args = split_top_level(content)
        # skip ws/comments to the body '{'
        b = skip_ws_comments(text, after_paren)
        if b >= n or text[b] != '{':
            # Not followed by a body — leave untouched (defensive).
            out.append(text[start:after_paren])
            pos = after_paren
            continue
        body, after_brace = read_balanced(text, b, '{', '}')
        # trailing ';' defensively
        k = after_brace
        if k < n and text[k] == ';':
            k += 1
        out.append(make_registration(mname, args, body))

        # Convert every immediately-following alias line (possibly after a
        # namespace-closing brace), preserving intervening whitespace/braces.
        # Keep k unchanged when no alias is found so the boundary
        # whitespace/newlines stay with the following segment.
        while True:
            probe = skip_ws_comments(text, k)
            if probe < n and text[probe] == '}':
                probe = skip_ws_comments(text, probe + 1)
            m = ALIAS_RE.match(text[probe:])
            if not m:
                break
            out.append(text[k:probe].rstrip(' \t') + "\n")
            src, als = m.group(1), m.group(2).strip()
            out.append("static int const %s_Alias = Function_Alias(\"%s\", \"%s\");"
                       % (src, src, als))
            k = probe + m.end()
        pos = k
    return "".join(out)


def ensure_include(text):
    if '#include "LTE/FunctionBind.h"' in text:
        return text
    lines = text.split('\n')
    out = []
    inserted = False
    for ln in lines:
        out.append(ln)
        if not inserted and ln.startswith('#include "LTE/Function.h"'):
            out.append('#include "LTE/FunctionBind.h"')
            inserted = True
    if not inserted:
        # fall back: insert after the last #include line of any kind
        last = None
        for idx, ln in enumerate(lines):
            if ln.startswith('#include'):
                last = idx
        if last is not None:
            out.insert(last + 1, '#include "LTE/FunctionBind.h"')
    return "\n".join(out)


def main(paths):
    for path in paths:
        with open(path, 'r', encoding='utf-8') as f:
            text = f.read()
        if 'FreeFunction' not in text and 'VoidFreeFunction' not in text:
            continue
        new = migrate(text)
        new = ensure_include(new)
        with open(path, 'w', encoding='utf-8') as f:
            f.write(new)
        print("migrated %s" % path)


if __name__ == '__main__':
    main(sys.argv[1:])
