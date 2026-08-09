// Copyright (C) 2025  darkoned12000
// SPDX-License-Identifier: GPL-3.0-or-later
// Part of the ltheory-old-test modernization effort (Revamp Work).
// See NOTICE and LICENSE.GPL. Original engine (c) Josh Parnell, public domain.

#include "LTE/Function.h"
#include "LTE/FunctionBind.h"
#include "LTE/ProgramLog.h"

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
