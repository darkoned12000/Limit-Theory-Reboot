// Copyright (C) 2025  darkoned12000
// SPDX-License-Identifier: GPL-3.0-or-later
// Part of the ltheory-old-test modernization effort (Revamp Work).
// See NOTICE and LICENSE.GPL. Original engine (c) Josh Parnell, public domain.
//
// Self-test harness support bindings: Program_Exit (clean shutdown with an
// exit code so `launch` can gate on failures) and Mouse_SetPos (deterministic
// cursor placement for focus-mouse hit-tests). These are exercised by
// App/selftest.lts; here we only verify the bindings registered (calling them
// in lte_tests would warp the real cursor / terminate a non-existent program).

#include "Harness.h"
#include "LTE/Function.h"
#include "LTE/String.h"
#include "LTE/Vector.h"

using namespace LTE;

LTE_TEST(Selftest_ProgramExitBindingRegistered) {
  Vector<Function> funcs = Function_Find("Program_Exit");
  LTE_CHECK_EQ(funcs.size(), size_t(1));
  if (funcs.size() == 1)
    LTE_CHECK_EQ(funcs[0]->paramCount, uint(1));
}

LTE_TEST(Selftest_MouseSetPosBindingRegistered) {
  Vector<Function> funcs = Function_Find("Mouse_SetPos");
  LTE_CHECK_EQ(funcs.size(), size_t(1));
  if (funcs.size() == 1)
    LTE_CHECK_EQ(funcs[0]->paramCount, uint(1));
}
