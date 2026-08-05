#!/usr/bin/env python3
# Copyright (C) 2025  darkoned12000
# SPDX-License-Identifier: GPL-3.0-or-later
# Part of the ltheory-old-test modernization effort (Revamp Work).
# See NOTICE and LICENSE.GPL. Original engine (c) Josh Parnell, public domain.

"""Migrate the DefineFunction macro family to the macro-free Function_Bind API.

Handles the three macro families used by a subsystem:
  DeclareFunction           -> LT_API per-param function declaration
  DeclareFunctionNoParams   -> LT_API zero-arg function declaration
  DeclareFunctionArgBind    -> AutoClass Name_Args bundle + per-param overload

Cpp `DefineFunction(Name) { ... }` bodies become real functions:
  - plain:   RT Name(T0 const& N0, ...) { ... X ... }   (args.X -> X)
  - noparams:RT Name() { ... }
  - argbind: RT Name(Name_Args const& args) { ... }     (body unchanged)

Each body is followed by a registration:
  static Function const Name_Registration = Function_Bind("Name", "None", ...);

Plain bodies that forward the whole `args` bundle (instead of reading
args.X fields) are migrated with the argbind transform (Object_Missile).

Usage:
  python3 script/migrate_definefunction.py            # report only
  python3 script/migrate_definefunction.py --apply    # write files
"""

import argparse
import glob
import os
import re
import sys

GAME_DIR = os.path.join("src", "liblt", "Game")


# ---------------------------------------------------------------------------
# Source scanning helpers
# ---------------------------------------------------------------------------

def mask_comments_strings(text):
    """Return a copy with comments and string literals replaced by spaces
    (preserving newlines), so regex scans never match inside them."""
    out = []
    i = 0
    n = len(text)
    while i < n:
        c = text[i]
        if c == '/' and i + 1 < n and text[i + 1] == '/':
            j = text.find('\n', i)
            if j == -1:
                j = n
            out.append(' ' * (j - i))
            i = j
        elif c == '/' and i + 1 < n and text[i + 1] == '*':
            j = text.find('*/', i + 2)
            if j == -1:
                j = n
            out.append(' ' * (j - i + 2))
            i = j + 2
        elif c == '"':
            j = i + 1
            while j < n:
                if text[j] == '\\':
                    j += 2
                    continue
                if text[j] == '"':
                    j += 1
                    break
                j += 1
            out.append(' ' * (j - i))
            i = j
        elif c == "'":
            j = i + 1
            while j < n:
                if text[j] == '\\':
                    j += 2
                    continue
                if text[j] == "'":
                    j += 1
                    break
                j += 1
            out.append(' ' * (j - i))
            i = j
        else:
            out.append(c)
            i += 1
    return ''.join(out)


def split_top_level(text):
    """Split on top-level commas (no nested parens in these arg lists)."""
    parts, depth, cur = [], 0, ''
    for c in text:
        if c == '(' or c == '<':
            depth += 1
        elif c == ')' or c == '>':
            depth -= 1
        if c == ',' and depth == 0:
            parts.append(cur)
            cur = ''
        else:
            cur += c
    parts.append(cur)
    return [p.strip() for p in parts]


# ---------------------------------------------------------------------------
# Header parsing
# ---------------------------------------------------------------------------

def parse_headers(header_paths):
    """Return {name: (header, kind, rt, [(type, name), ...])}."""
    fmap = {}
    for h in header_paths:
        src = open(h).read().splitlines()
        skip = 0
        for i, line in enumerate(src):
            m = re.match(r'\s*#\s*(if 0|ifdef|ifndef|if|else|elif|endif)\b', line)
            if m:
                d = m.group(1)
                if d == 'if 0':
                    skip += 1
                elif d == 'endif' and skip > 0:
                    skip -= 1
                continue
            if skip > 0:
                continue
            for kw in ('DeclareFunctionArgBind', 'DeclareFunctionNoParams',
                       'DeclareFunction'):
                if re.match(r'\s*' + kw + r'\s*\(', line):
                    text = line
                    j = i
                    while text.count('(') > text.count(')'):
                        j += 1
                        text += '\n' + src[j]
                    body = text[text.index('(') + 1:text.rindex(')')]
                    parts = split_top_level(body)
                    name, rt = parts[0], parts[1]
                    params = [(parts[k], parts[k + 1])
                              for k in range(2, len(parts) - 1, 2)]
                    kind = {'DeclareFunctionArgBind': 'argbind',
                            'DeclareFunctionNoParams': 'noparams',
                            'DeclareFunction': 'plain'}[kw]
                    fmap[name] = (h, kind, rt, params)
                    break
    return fmap


# ---------------------------------------------------------------------------
# Header rewriting
# ---------------------------------------------------------------------------

def wrap_params(params, indent, width=90):
    """Render 'T0 const& N0, T1 const& N1, ...' wrapped at width."""
    parts = [f"{t} const& {n}" for t, n in params]
    lines, cur = [], indent
    line = ''
    for p in parts:
        if not line:
            line = indent + p
        elif len(line) + len(p) + 2 <= width:
            line += ', ' + p
        else:
            lines.append(line + ',')
            line = indent + p
    lines.append(line)
    return '\n'.join(lines)


def header_replacement(name, rt, params, kind):
    """Build the replacement text for one macro invocation."""
    if kind == 'noparams':
        return f"LT_API {rt} {name}();\n"
    if kind == 'plain':
        decl = f"LT_API {rt} {name}("
        body = wrap_params(params, '  ')
        return f"{decl}\n{body});\n"
    # argbind (and plain functions whose body forwards the bundle)
    fields = ',\n  '.join(f"{t}, {n}" for t, n in params)
    bundle = (f"AutoClass({name}_Args,\n"
              f"  {fields})\n"
              f"  {name}_Args() {{}}\n"
              f"}};\n")
    overload_params = wrap_params(params, '  ')
    forward = ', '.join(n for _, n in params)
    ctor = f"{name}_Args({forward})"
    overload = (f"inline {rt} {name}(\n{overload_params}) {{\n"
                f"  return {name}({ctor});\n"
                f"}}\n")
    return f"{bundle}\nLT_API {rt} {name}({name}_Args const& args);\n{overload}\n"


def rewrite_headers(fmap, header_paths, bundle_forward_names):
    """Rewrite DeclareFunction macro invocations; drop DeclareFunction.h include."""
    changes = {}
    for h in header_paths:
        src = open(h).read().splitlines()
        skip = 0
        out = []
        i = 0
        n = len(src)
        changed = False
        while i < n:
            line = src[i]
            m = re.match(r'\s*#\s*(if 0|ifdef|ifndef|if|else|elif|endif)\b', line)
            if m:
                d = m.group(1)
                if d == 'if 0':
                    skip += 1
                elif d == 'endif' and skip > 0:
                    skip -= 1
                out.append(line)
                i += 1
                continue
            if skip > 0:
                out.append(line)
                i += 1
                continue
            if re.match(r'\s*#include\s+["<]LTE/DeclareFunction\.h', line):
                changed = True
                i += 1
                continue
            macro = None
            for kw in ('DeclareFunctionArgBind', 'DeclareFunctionNoParams',
                       'DeclareFunction'):
                if re.match(r'\s*' + kw + r'\s*\(', line):
                    macro = kw
                    break
            if macro is None:
                out.append(line)
                i += 1
                continue
            text = line
            j = i
            while text.count('(') > text.count(')'):
                j += 1
                text += '\n' + src[j]
            body = text[text.index('(') + 1:text.rindex(')')]
            parts = split_top_level(body)
            name, rt = parts[0], parts[1]
            params = [(parts[k], parts[k + 1])
                      for k in range(2, len(parts) - 1, 2)]
            kind = {'DeclareFunctionArgBind': 'argbind',
                    'DeclareFunctionNoParams': 'noparams',
                    'DeclareFunction': 'plain'}[kw]
            if kind == 'plain' and name in bundle_forward_names:
                kind = 'argbind'  # bundle-forwarding plain function
            assert fmap.get(name) == (h, kind, rt, params), \
                f"header parse mismatch for {name} in {h}"
            out.append(header_replacement(name, rt, params, kind).rstrip('\n'))
            changed = True
            i = j + 1
        if changed:
            out = '\n'.join(out)
            # DeclareFunction.h previously pulled in AutoClass.h transitively;
            # add it back where the argbind/bundle transform now needs it.
            if 'AutoClass(' in out and \
                    not re.search(r'#\s*include\s+["<]LTE/AutoClass\.h',
                                  out):
                lines = out.split('\n')
                last_inc = max((i for i, l in enumerate(lines)
                                if l.startswith('#include ')), default=None)
                if last_inc is not None:
                    lines.insert(last_inc + 1, '#include "LTE/AutoClass.h"')
                    out = '\n'.join(lines)
            changes[h] = out + '\n'
    return changes


# ---------------------------------------------------------------------------
# Cpp rewriting
# ---------------------------------------------------------------------------

def find_body_end(src, brace_pos):
    """Given index of '{', return index of matching '}' (comment/string aware)."""
    masked = mask_comments_strings(src)
    depth = 0
    i = brace_pos
    while i < len(masked):
        c = masked[i]
        if c == '{':
            depth += 1
        elif c == '}':
            depth -= 1
            if depth == 0:
                return i
        i += 1
    raise RuntimeError("unbalanced braces in DefineFunction body")


def strip_args(body, param_names, name, out_report):
    """Replace args.<param> with <param> for the given param names."""
    if not param_names:
        return body
    pat = re.compile(r'\bargs\.(' + '|'.join(re.escape(p) for p in param_names) + r')\b')
    # report shadowing hazards: local declarations matching a param name
    masked = mask_comments_strings(body)
    for p in param_names:
        if re.search(r'^\s*[\w:<>]+\s+' + re.escape(p) + r'\s*[=(;]',
                     masked, re.M):
            out_report.append(f"  shadow-hazard: {name}: local shadows param '{p}'")
    return pat.sub(lambda m: m.group(1), body)


def registration_block(name, rt, params, kind, alias=None):
    """Emit the static registration (and optional alias) block."""
    if kind in ('plain', 'noparams'):
        if params:
            target = f"&{name},\n  " + ", ".join(f'"{n}"' for _, n in params)
        else:
            target = f"&{name}"
    else:  # argbind / bundle-forwarding
        ptypes = ", ".join(f"{t} const& {n}" for t, n in params)
        fwd = ", ".join(n for _, n in params)
        names = ", ".join(f'"{n}"' for _, n in params)
        target = (f"[]({ptypes}) -> {rt} {{ return {name}({fwd}); }},\n"
                  f"  {names}")
    block = (f"static Function const {name}_Registration = Function_Bind(\n"
             f"  \"{name}\",\n"
             f"  \"None\",\n"
             f"  {target});")
    if alias:
        block += (f"\nstatic int const {name}_Alias = "
                  f"Function_Alias(\"{name}\", \"{alias}\");")
    return block


def rewrite_cpps(fmap, cpp_paths, bundle_forward_names, out_report):
    changes = {}
    for cpp in cpp_paths:
        src = open(cpp).read()
        masked = mask_comments_strings(src)
        out = []
        pos = 0
        changed = False
        for m in re.finditer(r'DefineFunction\((\w+)\)\s*\{', masked):
            name = m.group(1)
            if name not in fmap:
                out_report.append(f"  ERROR: no header decl for {name} in {cpp}")
                continue
            h, kind, rt, params = fmap[name]
            brace_pos = m.end() - 1
            end = find_body_end(src, brace_pos)
            # append untouched source up to the DefineFunction token; the token
            # itself is replaced by the new signature below
            out.append(src[pos:m.start()])
            body = src[brace_pos + 1:end]
            if kind == 'plain' and name in bundle_forward_names:
                kind = 'argbind'  # bundle-forwarding plain function
            if kind == 'plain':
                sig = (f"{rt} {name}(" +
                       ", ".join(f"{t} const& {n}" for t, n in params) + ") {")
                body = strip_args(body, [n for _, n in params], name, out_report)
            elif kind == 'noparams':
                sig = f"{rt} {name}() {{"
            else:  # argbind
                sig = f"{rt} {name}({name}_Args const& args) {{"
            # validate: no stray `args` references in plain/noparams bodies
            if kind in ('plain', 'noparams') and \
                    re.search(r'\bargs\b', mask_comments_strings(body)):
                out_report.append(f"  ERROR: {name} in {cpp} still references "
                                  f"'args' after strip (expected "
                                  f"{' '.join(n for _, n in params)})")
            out.append(sig)
            out.append(body)
            # closing brace + optional FunctionAlias on the same line
            tail = src[end:]
            alias = None
            am = re.match(r'\}\s*FunctionAlias\((\w+),\s*(\w+)\);', tail)
            if am:
                alias = am.group(2)
                tail_end = am.end()
            else:
                tail_end = 1
            out.append('}')
            out.append('\n' + registration_block(name, rt, params, kind, alias))
            out.append('\n\n')
            pos = end + tail_end
            changed = True
        out.append(src[pos:])
        if changed:
            text = ''.join(out)
            if '#include "LTE/FunctionBind.h"' not in text:
                lines = text.split('\n')
                last_inc = max((i for i, l in enumerate(lines)
                                if l.startswith('#include ')), default=None)
                if last_inc is not None:
                    lines.insert(last_inc + 1, '#include "LTE/FunctionBind.h"')
                    text = '\n'.join(lines)
            changes[cpp] = text
    return changes


# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--apply', action='store_true',
                    help='write files (default: report only)')
    ap.add_argument('--subsystem', default=GAME_DIR,
                    help='subsystem directory (default: src/liblt/Game)')
    args = ap.parse_args()

    root = args.subsystem
    header_paths = sorted(glob.glob(os.path.join(root, '**', '*.h'),
                                    recursive=True))
    header_paths = [h for h in header_paths
                    if 'DeclareFunction' in open(h).read()]
    cpp_paths = sorted(glob.glob(os.path.join(root, '**', '*.cpp'),
                                 recursive=True))
    cpp_paths = [c for c in cpp_paths if 'DefineFunction' in open(c).read()]

    report = []
    fmap = parse_headers(header_paths)
    report.append(f"Parsed {len(fmap)} DeclareFunction decls from "
                  f"{len(header_paths)} headers.")

    # plain bodies that forward the whole bundle (no args.X access)
    bundle_forward_names = set()
    for cpp in cpp_paths:
        src = open(cpp).read()
        masked = mask_comments_strings(src)
        for m in re.finditer(r'DefineFunction\((\w+)\)\s*\{', masked):
            name = m.group(1)
            if fmap.get(name, (None, None, None, None))[1] != 'plain':
                continue
            end = find_body_end(src, m.end() - 1)
            body = src[m.end():end]
            if re.search(r'\bargs\b(?!\.)', mask_comments_strings(body)):
                bundle_forward_names.add(name)
    if bundle_forward_names:
        report.append(f"Bundle-forwarding plain functions (migrated as argbind): "
                      f"{', '.join(sorted(bundle_forward_names))}")
    for name in bundle_forward_names:
        h, kind, rt, params = fmap[name]
        fmap[name] = (h, 'argbind', rt, params)

    header_changes = rewrite_headers(fmap, header_paths, bundle_forward_names)
    report.append(f"Headers to change: {len(header_changes)}")
    cpp_changes = rewrite_cpps(fmap, cpp_paths, bundle_forward_names, report)
    report.append(f"Cpps to change: {len(cpp_changes)} "
                  f"({sum(len(open(c).read().split('DefineFunction')) - 1
                        for c in cpp_changes)} DefineFunction sites)")

    print('\n'.join(report))
    print()

    if not args.apply:
        for h in sorted(header_changes):
            print(f"  header: {h}")
        for c in sorted(cpp_changes):
            print(f"  cpp:    {c}")
        print("\n(dry run — rerun with --apply to write files)")
        return

    for path in sorted(header_changes):
        with open(path, 'w') as f:
            f.write(header_changes[path])
    for path in sorted(cpp_changes):
        with open(path, 'w') as f:
            f.write(cpp_changes[path])
    print(f"Applied {len(header_changes)} header + {len(cpp_changes)} cpp changes.")


if __name__ == '__main__':
    main()
