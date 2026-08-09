// Copyright (C) 2025  darkoned12000
// SPDX-License-Identifier: GPL-3.0-or-later
// Part of the ltheory-old-test modernization effort (Revamp Work).
// See NOTICE and LICENSE.GPL. Original engine (c) Josh Parnell, public domain.
//
// Crash-log writing (ROADMAP 2.2). The signal handler path can't be
// exercised in-process (it exits), so these tests verify the pieces it uses:
// the report-writer produces a file with the expected header fields, and the
// log-path builder yields a unique, dated path in the user-data dir.

#include "Harness.h"
#include "LTE/CrashHandler.h"
#include "LTE/OS.h"
#include "LTE/String.h"

#include <csignal>
#include <fstream>

using namespace LTE;

static String ReadFile(String const& path) {
  std::ifstream in(path.c_str());
  String contents;
  String line;
  while (getline(in, line))
    contents += line + "\n";
  return contents;
}

LTE_TEST(CrashLog_WriteCreatesFile) {
  String path = OS_GetUserDataPath() + "crash_test.log";
  LTE_CHECK(CrashHandler_WriteLog(path, SIGSEGV));
  LTE_CHECK(OS_IsFile(path));

  String contents = ReadFile(path);
  LTE_CHECK(contents.contains("Limit Theory crash report"));
  LTE_CHECK(contents.contains("SIGSEGV"));
  LTE_CHECK(contents.contains("Timestamp"));
  LTE_CHECK(contents.contains("Native call stack"));
  LTE_CHECK(contents.contains("Engine frames"));
  remove(path.c_str());
}

LTE_TEST(CrashLog_WriteAppendsSecondReport) {
  String path = OS_GetUserDataPath() + "crash_test.log";
  CrashHandler_WriteLog(path, SIGSEGV);
  CrashHandler_WriteLog(path, SIGABRT);
  LTE_CHECK(OS_IsFile(path));

  String contents = ReadFile(path);
  LTE_CHECK(contents.contains("SIGSEGV"));
  LTE_CHECK(contents.contains("SIGABRT"));
  remove(path.c_str());
}

LTE_TEST(CrashLog_GetPathIsDated) {
  String path = CrashHandler_GetLogPath();
  LTE_CHECK(path.contains("crash_"));
  LTE_CHECK(path.contains(".log"));
  LTE_CHECK(path.contains("-"));  // YYYY-MM-DD_HH-MM-SS separators
  LTE_CHECK(path.contains(OS_GetUserDataPath()));
}
