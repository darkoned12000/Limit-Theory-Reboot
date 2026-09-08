// Copyright (C) 2025  darkoned12000
// SPDX-License-Identifier: GPL-3.0-or-later
// Part of the ltheory-old-test modernization effort (Revamp Work).
// See NOTICE and LICENSE.GPL. Original engine (c) Josh Parnell, public domain.

#include "LTE/Function.h"
#include "LTE/FunctionBind.h"
#include "LTE/ProgramLog.h"
#include "LTE/Vector.h"

/* Script-visible logging (ltsl-hardening.md §8 P1-2): lets apps trace to the
   console + log file without touching engine C++. Level mapping mirrors
   ProgramLog's severities: Log (info) -> Log_Message, Log_Warn -> Log_Warning,
   Log_Error -> Log_Error. */
static Function const Log_Registration = Function_Bind(
  "Log",
  "Write a plain message to the console and log file",
  [](String const& entry)
  {
  ::LTE::Log_Message(entry);
  },
  "entry");

static Function const Log_Warn_Registration = Function_Bind(
  "Log_Warn",
  "Write a warning message to the console and log file",
  [](String const& entry)
  {
  ::LTE::Log_Warning(entry);
  },
  "entry");

static Function const Log_Error_Registration = Function_Bind(
  "Log_Error",
  "Write an error message to the console and log file",
  [](String const& entry)
  {
  ::LTE::Log_Error(entry);
  },
  "entry");

/* P1-3 runtime error channel: scalar failure-entry access for the F3
   debug overlay. Deliberately Vector-free — exposing Vector<String>
   through the script type system hit the static-init type-resolution
   hazard (AGENTS.md A.7 class) in app loads; String/int params are safe. */
static Function const Log_GetErrorCount_Registration = Function_Bind(
  "Log_GetErrorCount",
  "Return the number of failure entries ([Error]/[CRITICAL]) in the engine log",
  []() -> int
  {
    return (int)::LTE::Log_GetFailureCount();
  });
static int const Log_GetErrorCount_Alias =
  Function_Alias("Log_GetErrorCount", "GetErrorCount");

static Function const Log_GetError_Registration = Function_Bind(
  "Log_GetError",
  "Return the i-th failure log entry, 0 = oldest failure, empty string "
  "when the index is out of range",
  [](int const& index) -> String
  {
    return ::LTE::Log_GetFailure(index);
  },
  "index");
static int const Log_GetError_Alias = Function_Alias("Log_GetError", "GetError");
