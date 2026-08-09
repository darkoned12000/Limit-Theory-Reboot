// Copyright (C) 2025  darkoned12000
// SPDX-License-Identifier: GPL-3.0-or-later
// Part of the ltheory-old-test modernization effort (Revamp Work).
// See NOTICE and LICENSE.GPL. Original engine (c) Josh Parnell, public domain.
//
// Script-visible logging bindings (ltsl-hardening.md §8 P1-2). Before this,
// apps had no way to trace to the console without touching engine C++ — every
// debug print meant an engine printf. Log / Log_Warn / Log_Error map to
// ProgramLog's severities and are exercised here via Function_Find/call.

#include "Harness.h"
#include "LTE/Function.h"
#include "LTE/ProgramLog.h"
#include "LTE/String.h"
#include "LTE/Type.h"
#include "LTE/Vector.h"

using namespace LTE;

static Function FindLogBinding(String const& name) {
  Vector<Function> const& funcs = Function_Find(name);
  for (size_t i = 0; i < funcs.size(); ++i)
    if (funcs[i]->name == name && funcs[i]->paramCount == 1)
      return funcs[i];
  return nullptr;
}

LTE_TEST(LogBindings_Registered) {
  LTE_CHECK(FindLogBinding("Log"));
  LTE_CHECK(FindLogBinding("Log_Warn"));
  LTE_CHECK(FindLogBinding("Log_Error"));
}

LTE_TEST(LogBinding_EmitsEntry) {
  // Log_Message appends to the in-memory entry ring; calling the script
  // binding must produce a log entry (this is the channel the UI self-test
  // harness reports its PASS/FAIL lines through).
  Function fn = FindLogBinding("Log");
  LTE_CHECK(fn);
  if (!fn)
    return;

  size_t before = Log_GetEntries();
  String msg = "selftest-log-binding-check";
  void* args[] = { &msg };
  fn->call(fn->binding, args, nullptr);
  LTE_CHECK(Log_GetEntries() > before);
}
