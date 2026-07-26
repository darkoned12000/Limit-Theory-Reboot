#!/usr/bin/env python3
"""Add layout(location=N) qualifiers to VERT_OUT/FRAG_IN in .jsl shader files.

Builds a global varying name → location map starting after the 5 standard
varyings (0-4 from vert.jsl/frag.jsl). Ensures vertex 'out' and matching
fragment 'in' declarations get the same location.
"""

import os
import re
import sys

SHADER_ROOT = "resource/shader"

# Standard varyings already assigned in vert.jsl/frag.jsl (0-4)
LOCATION_START = 5

# Global map: varying_name → assigned location
varying_locations = {}
next_location = LOCATION_START

# Array varying pattern: "vec4 offset[3]"
ARRAY_RE = re.compile(
    r'^(layout\(location=\d+\)\s+)?(VERT_OUT|FRAG_IN)\s+'
    r'(\w+)\s+(\w+)\s*\[(\d+)\]\s*;'
)
# Scalar varying pattern
SCALAR_RE = re.compile(
    r'^(layout\(location=\d+\)\s+)?(VERT_OUT|FRAG_IN)\s+'
    r'(\w+)\s+(\w+)\s*;'
)

def get_location(name, array_size=0):
    """Get or assign a location for a varying name."""
    global next_location
    if name in varying_locations:
        return varying_locations[name]
    loc = next_location
    varying_locations[name] = loc
    next_location += max(1, array_size)
    return loc

def process_file(filepath, is_vertex):
    """Add layout(location=N) to VERT_OUT (vertex) or FRAG_IN (fragment) declarations."""
    with open(filepath, 'r') as f:
        content = f.read()

    lines = content.split('\n')
    changed = False
    new_lines = []

    for line in lines:
        # Skip if already has layout qualifier
        if 'layout(location=' in line and ('VERT_OUT' in line or 'FRAG_IN' in line):
            new_lines.append(line)
            continue

        # Try array pattern first
        m = ARRAY_RE.match(line)
        if m:
            qualifier = m.group(1) or ''
            direction = m.group(2)
            vtype = m.group(3)
            vname = m.group(4)
            arr_size = int(m.group(5))
            loc = get_location(vname, arr_size)
            new_lines.append(
                f'layout(location={loc}) {direction} {vtype} {vname}[{arr_size}];'
            )
            changed = True
            continue

        # Try scalar pattern
        m = SCALAR_RE.match(line)
        if m:
            qualifier = m.group(1) or ''
            direction = m.group(2)
            vtype = m.group(3)
            vname = m.group(4)
            loc = get_location(vname)
            new_lines.append(
                f'layout(location={loc}) {direction} {vtype} {vname};'
            )
            changed = True
            continue

        new_lines.append(line)

    if changed:
        with open(filepath, 'w') as f:
            f.write('\n'.join(new_lines))
        return True
    return False


def collect_files():
    """Collect all vertex and fragment shader files."""
    vertex_files = []
    fragment_files = []

    # Vertex shaders
    vert_dir = os.path.join(SHADER_ROOT, "vertex")
    if os.path.isdir(vert_dir):
        for name in sorted(os.listdir(vert_dir)):
            if name.endswith('.jsl'):
                vertex_files.append(os.path.join(vert_dir, name))

    # Fragment shaders (including subdirectories)
    for root, dirs, files in os.walk(os.path.join(SHADER_ROOT, "fragment")):
        for name in sorted(files):
            if name.endswith('.jsl'):
                fragment_files.append(os.path.join(root, name))

    # Common headers with FRAG_IN/VERT_OUT
    common_dir = os.path.join(SHADER_ROOT, "common")
    common_files_with_io = []
    for name in ['scattering.jsl', 'softparticle.jsl', 'deferred.jsl', 'ui.jsl']:
        path = os.path.join(common_dir, name)
        if os.path.isfile(path):
            common_files_with_io.append(path)

    return vertex_files, fragment_files, common_files_with_io


def main():
    global next_location

    vertex_files, fragment_files, common_files = collect_files()

    # PHASE 1: Scan all files to build the global varying location map
    # Process common headers first (they define shared varyings)
    print("Phase 1: Building global varying location map...")
    for path in common_files:
        with open(path, 'r') as f:
            content = f.read()
        for m in ARRAY_RE.finditer(content):
            vname = m.group(4)
            arr_size = int(m.group(5))
            get_location(vname, arr_size)
        for m in SCALAR_RE.finditer(content):
            vname = m.group(4)
            get_location(vname)

    # Process vertex shaders
    for path in vertex_files:
        with open(path, 'r') as f:
            content = f.read()
        for m in ARRAY_RE.finditer(content):
            vname = m.group(4)
            arr_size = int(m.group(5))
            get_location(vname, arr_size)
        for m in SCALAR_RE.finditer(content):
            vname = m.group(4)
            get_location(vname)

    # Process fragment shaders
    for path in fragment_files:
        with open(path, 'r') as f:
            content = f.read()
        for m in ARRAY_RE.finditer(content):
            vname = m.group(4)
            arr_size = int(m.group(5))
            get_location(vname, arr_size)
        for m in SCALAR_RE.finditer(content):
            vname = m.group(4)
            get_location(vname)

    print(f"  Found {len(varying_locations)} unique varying names, "
          f"locations {LOCATION_START}-{next_location - 1}")

    # PHASE 2: Apply layout qualifiers
    print("\nPhase 2: Applying layout qualifiers...")
    changed_count = 0

    # Common headers
    for path in common_files:
        if process_file(path, is_vertex=False):
            changed_count += 1
            print(f"  Updated: {path}")

    # Vertex shaders
    for path in vertex_files:
        if process_file(path, is_vertex=True):
            changed_count += 1
            print(f"  Updated: {path}")

    # Fragment shaders
    for path in fragment_files:
        if process_file(path, is_vertex=False):
            changed_count += 1
            print(f"  Updated: {path}")

    print(f"\nDone. Updated {changed_count} files.")
    print(f"Varying location map:")
    for name, loc in sorted(varying_locations.items(), key=lambda x: x[1]):
        print(f"  {name} → location {loc}")


if __name__ == '__main__':
    os.chdir(os.path.join(os.path.dirname(os.path.abspath(__file__)), '..'))
    main()
