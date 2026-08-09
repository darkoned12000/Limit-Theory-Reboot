// Copyright (C) 2025  darkoned12000
// SPDX-License-Identifier: GPL-3.0-or-later
// Part of the ltheory-old-test modernization effort (Revamp Work).
// See NOTICE and LICENSE.GPL. Original engine (c) Josh Parnell, public domain.

#ifndef LTE_CrashHandler_h__
#define LTE_CrashHandler_h__

#include "Common.h"
#include "String.h"

/* Install signal handlers that write a crash log to disk (native stack
   trace + engine frame annotations) and exit with a non-zero code. */
LT_API void CrashHandler_Install();

/* Build the crash-log path for a crash that happens "now":
   <userdata>crash_YYYY-MM-DD_HH-MM-SS.log */
LT_API String CrashHandler_GetLogPath();

/* Write a crash report to `path`. Used by the signal handler; also exposed
   so the unit tests can verify the log format without crashing. */
LT_API bool CrashHandler_WriteLog(String const& path, int signal);

#endif
