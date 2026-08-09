// Copyright (C) 2025  darkoned12000
// SPDX-License-Identifier: GPL-3.0-or-later
// Part of the ltheory-old-test modernization effort (Revamp Work).
// See NOTICE and LICENSE.GPL. Original engine (c) Josh Parnell, public domain.

#include "LTE/Function.h"
#include "LTE/FunctionBind.h"
#include "LTE/Program.h"

static Function const Program_Delete_Registration = Function_Bind(
  "Program_Delete",
  "Request that the running application exit (clean shutdown)",
  []()
  {
  Program* p = Program_GetCurrent();
  if (p)
    p->Delete();
  });
static int const Program_Delete_Alias = Function_Alias("Program_Delete", "Exit");

static Function const Program_Exit_Registration = Function_Bind(
  "Program_Exit",
  "Request clean shutdown with the given exit code (used by the self-test harness to gate on failures)",
  [](int const& code)
  {
  Program_SetExitCode(code);
  Program* p = Program_GetCurrent();
  if (p)
    p->Delete();
  },
  "code");
