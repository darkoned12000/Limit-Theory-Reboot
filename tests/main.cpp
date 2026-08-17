// Copyright (C) 2025  darkoned12000
// SPDX-License-Identifier: GPL-3.0-or-later
// Part of the ltheory-old-test modernization effort (Revamp Work).
// See NOTICE and LICENSE.GPL. Original engine (c) Josh Parnell, public domain.

// Test runner for the LTE core unit tests. Each LTE_TEST registers itself
// through the Harness; this main() executes them all and reports a summary.
// Exit code is non-zero if any check failed, so CI can gate on it.

#include "Harness.h"

#include <cstdio>
#include <cstring>

int main(int argc, char** argv) {
  using namespace LTE::Tests;

  setvbuf(stdout, nullptr, _IONBF, 0);

  char const* filter = argc > 1 ? argv[1] : nullptr;
  std::printf("Running %zu LTE core test(s)...\n", Registry::entries().size());
  for (auto const& e : Registry::entries()) {
    if (filter && !std::strstr(e.name, filter))
      continue;
    std::printf("  -> %s (%s)\n", e.name, e.file);
    e.fn();
  }

  std::printf("\n%d check(s), %d failure(s).\n",
    Registry::checks(), Registry::failures());
  return Registry::failures() == 0 ? 0 : 1;
}
