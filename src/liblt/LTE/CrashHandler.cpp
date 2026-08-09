// Copyright (C) 2025  darkoned12000
// SPDX-License-Identifier: GPL-3.0-or-later
// Part of the ltheory-old-test modernization effort (Revamp Work).
// See NOTICE and LICENSE.GPL. Original engine (c) Josh Parnell, public domain.

#include "CrashHandler.h"

#include "BuildMode.h"
#include "OS.h"
#include "ProgramLog.h"
#include "StackFrame.h"
#include "Time.h"

#include <csignal>
#include <ctime>
#include <fstream>
#include <sstream>

#ifdef LIBLT_LINUX
  #include <execinfo.h>
#endif

#ifdef LIBLT_WINDOWS
  #include <windows.h>
  #undef MessageBox
#endif

namespace {
  char const* GetSignalName(int signal) {
    switch (signal) {
      case SIGSEGV: return "SIGSEGV (segmentation fault)";
      case SIGABRT: return "SIGABRT (abort / assertion failure)";
      case SIGFPE:  return "SIGFPE (floating point exception)";
      case SIGILL:  return "SIGILL (illegal instruction)";
      case SIGINT:  return "SIGINT (interrupt)";
      default:      return "unknown signal";
    }
  }

  /* Zero-padded component (e.g. 3 -> "03") so log names/timestamps sort. */
  void WritePadded(std::ostream& stream, int value, int width) {
    stream.fill('0');
    stream.width(width);
    stream << value;
    stream.fill(' ');
  }

  String FormatTimestamp(Time const& now) {
    std::stringstream stream;
    WritePadded(stream, now.year, 4);
    stream << "-";
    WritePadded(stream, now.month, 2);
    stream << "-";
    WritePadded(stream, now.day, 2);
    stream << " ";
    WritePadded(stream, now.hour, 2);
    stream << ":";
    WritePadded(stream, now.minute, 2);
    stream << ":";
    WritePadded(stream, now.second, 2);
    return stream.str();
  }

  String FormatTimestampForPath(Time const& now) {
    std::stringstream stream;
    WritePadded(stream, now.year, 4);
    stream << "-";
    WritePadded(stream, now.month, 2);
    stream << "-";
    WritePadded(stream, now.day, 2);
    stream << "_";
    WritePadded(stream, now.hour, 2);
    stream << "-";
    WritePadded(stream, now.minute, 2);
    stream << "-";
    WritePadded(stream, now.second, 2);
    return stream.str();
  }

  /* Native C++ call-stack via libc backtrace(). Best-effort: if symbols
     can't be resolved the lines still carry raw addresses. */
  void WriteNativeBacktrace(std::ostream& stream) {
#ifdef LIBLT_LINUX
    void* frames[64];
    int size = backtrace(frames, 64);
    if (size > 0) {
      char** symbols = backtrace_symbols(frames, size);
      for (int i = 0; i < size; ++i) {
        stream << "    " << (symbols ? symbols[i] : "(symbols unavailable)")
               << '\n';
      }
      if (symbols)
        free(symbols);
    }
#else
    stream << "    (native backtrace only available on Linux)\n";
#endif
  }
}

/* Write a crash report to `path` and return whether it was written. */
bool CrashHandler_WriteLog(String const& path, int signal) {
  std::ofstream log(path.c_str(), std::ios::app);
  if (!log)
    return false;

  Time now = Time_Current();
  log << "Limit Theory crash report\n";
  log << "  Signal        : " << signal << "  " << GetSignalName(signal) << "\n";
  log << "  Timestamp     : " << FormatTimestamp(now) << "\n";
#ifdef BUILD_DEBUG
  log << "  Build         : debug\n";
#else
  log << "  Build         : release\n";
#endif
  log << "  Engine frames : " << StackFrame_Get() << "\n\n";
  log << "Native call stack:\n";
  WriteNativeBacktrace(log);
  log << "\n";

  /* Also surface to the in-memory log ring so a windowed player gets the
     message in the console/log file too. */
  Log_Error(Stringize()
    | "Crash: " | signal | " " | GetSignalName(signal)
    | " -- log written to " | path);
  return true;
}

String CrashHandler_GetLogPath() {
  return OS_GetUserDataPath() + "crash_" + FormatTimestampForPath(Time_Current()) + ".log";
}

/* Signal handler entry. Not async-signal-safe in the strictest sense
   (allocates, opens files) but this is a best-effort last resort: the default
   disposition is restored first so a second fault during handling aborts
   instead of recursing, and nothing after the write can crash harder than the
   crash being reported. */
static void CrashHandler(int sig) {
  signal(sig, SIG_DFL);

  String path = CrashHandler_GetLogPath();
  CrashHandler_WriteLog(path, sig);

  String message = "Limit Theory has crashed.\n\n";
  message += "A crash log has been written to:\n  " + path + "\n";
  OS_MessageBox("Limit Theory Engine Error", message);
  exit(128 + sig);
}

/* Graceful exit for user-initiated interrupts (SIGINT). */
static void IntHandler(int) {
  StackFrame_Print();
  exit(0);
}

void CrashHandler_Install() {
#ifdef LIBLT_WINDOWS
  signal(SIGABRT, CrashHandler);
  signal(SIGSEGV, CrashHandler);
#else
  signal(SIGABRT, CrashHandler);
  signal(SIGSEGV, CrashHandler);
  signal(SIGFPE, CrashHandler);
  signal(SIGILL, CrashHandler);
  signal(SIGINT, IntHandler);
#endif
}
