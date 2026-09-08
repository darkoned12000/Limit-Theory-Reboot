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

static Function FindLogBinding(String const& name, uint arity = 1) {
  Vector<Function> const& funcs = Function_Find(name);
  for (size_t i = 0; i < funcs.size(); ++i)
    if (funcs[i]->name == name && funcs[i]->paramCount == arity)
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

LTE_TEST(LogBinding_FailureAccessors) {
  // P1-3: scalar failure accessors feed the F3 overlay. Logging via the
  // Log binding (a plain message) must NOT count as a failure; the
  // error channel must only include [Error]/[CRITICAL] entries.
  Function tailFn = FindLogBinding("Log_GetErrorCount", 0);
  LTE_CHECK(tailFn);
  if (!tailFn) return;

  size_t before = Log_GetEntries();
  String plain = "log-count-no-failure-marker";
  void* argsPlain[] = { &plain };
  Function logfn = FindLogBinding("Log");
  logfn->call(logfn->binding, argsPlain, nullptr);
  LTE_CHECK(Log_GetEntries() > before);

  int countBefore = 0;
  tailFn->call(tailFn->binding, nullptr, &countBefore);
  LTE_CHECK(countBefore >= 0);

  String err1 = "log-failure-counter-check-alpha Error";
  Function errFn = FindLogBinding("Log_Error");
  void* argsErr[] = { &err1 };
  errFn->call(errFn->binding, argsErr, nullptr);

  int countAfter = 0;
  tailFn->call(tailFn->binding, nullptr, &countAfter);
  LTE_CHECK_EQ(countAfter, countBefore + 1);

  /* Out-of-range index returns the empty string safely. */
  Function getFn = FindLogBinding("Log_GetError");
  LTE_CHECK(getFn);
  if (!getFn) return;
  int bogus = countAfter + 100;
  void* argsBogus[] = { &bogus };
  String outOfRange;
  getFn->call(getFn->binding, argsBogus, &outOfRange);
  LTE_CHECK(outOfRange.size() == 0);

  /* In-range index returns the failure entry text. */
  int last = countAfter - 1;
  void* argsLast[] = { &last };
  String entry;
  getFn->call(getFn->binding, argsLast, &entry);
  LTE_CHECK(entry.find("log-failure-counter-check-alpha") != String::npos);
}
