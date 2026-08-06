#!/usr/bin/env python3
# Copyright (C) 2025  darkoned12000
# SPDX-License-Identifier: GPL-3.0-or-later
# Part of the ltheory-old-test modernization effort (Revamp Work).
# See NOTICE and LICENSE.GPL. Original engine (c) Josh Parnell, public domain.
#
# Migrate the DefineConversion macro family (Function.h) to the macro-free
# Conversion_Bind API (FunctionBind.h). See ltsl-binding-bridge-replacement.md
# §6.5: `DefineConversion(Name, Source, Dest) { body }` becomes
#   static void Name_Impl(Source const& src, Dest& dest) { body }
#   static int const Name_Registration = Conversion_Bind<&Name_Impl>();
#
# Usage:
#   python3 script/migrate_defineconversion.py [--apply] [--root <dir>]
# Default: report only; rerun with --apply to write files.

import argparse
import glob
import os
import re

CONV_RE = re.compile(
    r'DefineConversion\(\s*(\w+)\s*,\s*([^,()]+?)\s*,\s*([^,()]+?)\s*\)\s*\{')


def find_body_end(src, open_brace):
    depth = 1
    i = open_brace + 1
    n = len(src)
    while i < n and depth:
        c = src[i]
        if c == '{':
            depth += 1
        elif c == '}':
            depth -= 1
            if depth == 0:
                return i
        elif c == '/':
            if i + 1 < n and src[i + 1] == '/':
                j = src.find('\n', i)
                i = n - 1 if j < 0 else j
                continue
            if i + 1 < n and src[i + 1] == '*':
                j = src.find('*/', i + 2)
                i = n - 1 if j < 0 else j + 1
                continue
        elif c == '"':
            j = i + 1
            while j < n and src[j] != '"':
                if src[j] == '\\':
                    j += 1
                j += 1
            i = j
        i += 1
    return i


def rewrite_cpp(src):
    out = []
    pos = 0
    sites = []
    for m in CONV_RE.finditer(src):
        name, source_t, dest_t = m.group(1), m.group(2).strip(), m.group(3).strip()
        end = find_body_end(src, m.end() - 1)
        body = src[m.end():end]
        out.append(src[pos:m.start()])
        out.append(
            f'static void {name}_Impl({source_t} const& src, {dest_t}& dest) '
            f'{{{body}}}')
        out.append(
            f'\nstatic int const {name}_Registration = '
            f'Conversion_Bind<&{name}_Impl>();\n')
        pos = end + 1
        sites.append(name)
    out.append(src[pos:])
    return ''.join(out), sites


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--apply', action='store_true',
                    help='write files (default: report only)')
    ap.add_argument('--root', default='src/liblt',
                    help='root directory (default: src/liblt)')
    args = ap.parse_args()

    cpp_paths = sorted(glob.glob(os.path.join(args.root, '**', '*.cpp'),
                                 recursive=True))
    cpp_paths = [c for c in cpp_paths if 'DefineConversion' in open(c).read()]

    changes = {}
    total = 0
    for cpp in cpp_paths:
        src = open(cpp).read()
        if 'Conversion_Bind' in src and 'DefineConversion' not in src:
            continue
        new_src, sites = rewrite_cpp(src)
        if sites:
            changes[cpp] = new_src
            total += len(sites)
            print(f'  {cpp}: {len(sites)} sites')

    print(f'\n{len(changes)} cpp files, {total} DefineConversion sites')
    if not args.apply:
        print('\n(dry run — rerun with --apply to write files)')
        return

    for path, new_src in changes.items():
        with open(path, 'w') as f:
            f.write(new_src)
    print(f'Applied {len(changes)} cpp changes.')


if __name__ == '__main__':
    main()
