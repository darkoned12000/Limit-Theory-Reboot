// Copyright (C) 2025  darkoned12000
// SPDX-License-Identifier: GPL-3.0-or-later
// Part of the ltheory-old-test modernization effort (Revamp Work).
// See NOTICE and LICENSE.GPL. Original engine (c) Josh Parnell, public domain.
//
// LTSL compile gate — compiles every .lts script under a corpus root with the
// REAL engine (Expression_Compile via Script_CompileCheck) and fails if any
// file produces compile errors that are not explicitly allowlisted.
//
// This catches the class of bug the LSP smoke test cannot: the LSP analyzer
// models script-type constructors and arity rules approximately, while this
// tool runs the exact compiler that ships (see AGENTS.md A.14 #14 — the
// CreateButton breakage compiled clean under the analyzer but dead-ended
// every SAVE/LOAD button at runtime).
//
//   Build:  cmake --build ./build --target ltsl_compile_gate
//   Run:    LD_LIBRARY_PATH=bin:extbin/linux64 bin/ltsl_compile_gate \
//             [corpusRoot] [allowlistFile]
//           corpusRoot     default "resource/script"
//           allowlistFile  default "tools/compile_gate_allowlist.txt"
//
// Allowlist format (one per line):
//   <scriptName>#<line>
// e.g.  App/draw#57
// Blank lines and lines starting with '#' are ignored. An allowlist entry
// that matches no actual failure is reported as STALE and fails the run, so
// fixtures cannot rot silently in either direction.
//
// Exit code: 0 = all files compile clean or are fully allowlisted with no
// stale entries; 1 = otherwise.

#include "LTE/OS.h"
#include "LTE/Script.h"
#include "LTE/String.h"

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <set>
#include <string>
#include <vector>

using namespace LTE;

static std::string Std(String const& s) {
  return std::string(s.c_str());
}

/* Error strings are formatted by CompileEnvironment::ReportError as
   "  line N: message"; errors without a known line have no such prefix.
   Normalize both to a line number (0 = unknown). */
static unsigned ErrorLine(String const& err) {
  char const* p = err.c_str();
  while (*p == ' ')
    ++p;
  unsigned line = 0;
  if (sscanf(p, "line %u:", &line) != 1)
    return 0;
  return line;
}

static void CollectScripts(
  String const& root,
  Vector<String>& names)
{
  Vector<String> entries = OS_ListDir(root);
  for (size_t i = 0; i < entries.size(); ++i) {
    String const& entry = entries[i];
    if (entry == "." || entry == "..")
      continue;
    String full = root + "/" + entry;
    if (OS_IsDir(full)) {
      CollectScripts(full, names);
    } else if (entry.size() > 4 &&
               Std(entry).rfind(".lts") == entry.size() - 4) {
      names.push(full);
    }
  }
}

/* resource/script/App/Foo.lts -> "App/Foo" */
static String ToScriptName(String const& fullPath, String const& root) {
  String rel = fullPath;
  if (rel.size() > root.size() && strncmp(rel.c_str(), root.c_str(), root.size()) == 0)
    rel = rel.substr(root.size() + 1);
  if (rel.size() > 4)
    rel = rel.substr(0, rel.size() - 4);
  return rel;
}

static std::string MakeKey(String const& name, unsigned line) {
  return Std(name) + "#" + std::to_string(line);
}

static bool LoadAllowlist(
  char const* path,
  std::set<std::string>& out)
{
  FILE* f = fopen(path, "r");
  if (!f)
    return false;
  char buf[512];
  while (fgets(buf, sizeof(buf), f)) {
    std::string line(buf);
    while (!line.empty() && (line.back() == '\n' || line.back() == '\r'))
      line.pop_back();
    if (line.empty() || line[0] == '#')
      continue;
    out.insert(line);
  }
  fclose(f);
  return true;
}

int main(int argc, char** argv) {
  String corpusRoot = argc > 1 ? String(argv[1]) : String("resource/script");
  char const* allowlistPath =
    argc > 2 ? argv[2] : "tools/compile_gate_allowlist.txt";

  Vector<String> paths;
  CollectScripts(corpusRoot, paths);
  if (paths.empty()) {
    std::printf("compile-gate: no .lts files found under '%s'\n",
                corpusRoot.c_str());
    return 1;
  }

  std::sort(paths.begin(), paths.end(),
            [](String const& a, String const& b) {
              return strcmp(a.c_str(), b.c_str()) < 0;
            });

  std::set<std::string> allow;
  if (!LoadAllowlist(allowlistPath, allow)) {
    std::printf("compile-gate: cannot read allowlist file '%s'\n",
                allowlistPath);
    return 1;
  }
  std::set<std::string> unused = allow;

  size_t passed = 0;
  size_t allowlisted = 0;
  size_t failed = 0;
  Vector<String> failures;

  for (size_t i = 0; i < paths.size(); ++i) {
    String name = ToScriptName(paths[i], corpusRoot);

    Vector<String> errors;
    bool ok = Script_CompileCheck(name, errors);
    if (ok) {
      ++passed;
      continue;
    }

    /* Every error line in the file must be individually allowlisted. */
    bool covered = true;
    for (size_t e = 0; e < errors.size(); ++e) {
      std::string k = MakeKey(name, ErrorLine(errors[e]));
      auto it = unused.find(k);
      if (it != unused.end())
        unused.erase(it);
      else
        covered = false;
    }

    if (covered) {
      ++allowlisted;
    } else {
      ++failed;
      failures.push(name);
      std::printf("FAIL %s (%zu error(s))\n", name.c_str(), errors.size());
      for (size_t e = 0; e < errors.size(); ++e)
        std::printf("    %s\n", errors[e].c_str());
    }
  }

  std::printf("\ncompile-gate: %zu file(s): %zu clean, %zu allowlisted, "
              "%zu FAILED\n",
              paths.size(), passed, allowlisted, failed);

  if (!unused.empty()) {
    std::printf("compile-gate: %zu STALE allowlist entr%s "
                "(no matching failure):\n",
                unused.size(), unused.size() == 1 ? "y" : "ies");
    for (auto const& k : unused)
      std::printf("    %s\n", k.c_str());
  }

  if (failed == 0 && unused.empty()) {
    std::printf("compile-gate: PASS\n");
    return 0;
  }
  std::printf("compile-gate: FAIL\n");
  return 1;
}
