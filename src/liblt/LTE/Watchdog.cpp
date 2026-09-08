// Copyright (C) 2025  darkoned12000
// SPDX-License-Identifier: GPL-3.0-or-later
// Part of the ltheory-old-test modernization effort (Revamp Work).
// See NOTICE and LICENSE.GPL. Original engine (c) Josh Parnell, public domain.

#include "Watchdog.h"

#include "CrashHandler.h"
#include "ProgramLog.h"
#include "StackFrame.h"
#include "Vector.h"

#include <atomic>
#include <chrono>
#include <thread>

namespace LTE {
  namespace {
    std::atomic<bool> running(false);      /* armed state, mutable between frames */
    std::atomic<bool> armed(false);        /* window is open while true */
    std::atomic<double> deadlineMs(0.0);   /* monotonic ms deadline; 0 = disarmed */
    std::atomic<bool> threadStarted(false);
    void (*tripAction)(String const& context, String const& stack) = nullptr;

    /* The armed context: an append-only pool of distinct literal contexts.
       Each Arm pushes a copy and the armed index picks it — the trip
       thread reads by index, never dangling. Contexts are few (2 or 3
       distinct call sites + tests), so the pool stays tiny. */
    Vector<String> contextPool;
    std::atomic<size_t> armedCtxIndex(0);

    double NowMs() {
      using namespace std::chrono;
      return duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count();
    }

    void DefaultTripAction(String const& context, String const& stack) {
      Log_Critical(Stringize()
        | "Watchdog trip: '" | context
        | "' did not return within its deadline (hung?); stack="
        | stack);
      /* Keep the builder-style crash report on disk alongside the
         message; then fail loud per ltsl-hardening.md §9 (log+abort). */
      CrashHandler_WriteLog(CrashHandler_GetLogPath(), 0);
      LTE_ASSERT(false);
    }

    void EnsureThread() {
      bool expected = false;
      if (!threadStarted.compare_exchange_strong(expected, true))
        return;
      std::thread([]() {
        for (;;) {
          if (armed.load()) {
            double deadline = deadlineMs.load();
            if (deadline > 0.0 && NowMs() > deadline) {
              /* One-shot: disarm before delivering so a trip fires at
                 most once per arm and re-entry is safe. */
              armed.store(false);
              bool wasRunning = running.exchange(false);
              deadlineMs.store(0.0);
              if (wasRunning) {
                size_t nIndex = armedCtxIndex.load();
                String context = nIndex < contextPool.size()
                  ? contextPool[nIndex] : String("");
                void (*action)(String const&, String const&) =
                  tripAction ? tripAction : &DefaultTripAction;
                action(context, StackFrame_Get());
              }
            }
          }
          std::this_thread::sleep_for(std::chrono::milliseconds(20));
        }
      }).detach();
    }
  }

  void Watchdog_Arm(double seconds, String const& context) {
    EnsureThread();
    /* Publish order: context first, then armed state, then the
       deadline — so the trip thread never reads a stale context. */
    size_t n;
    {
      /* Append-only; no lock needed — pushes come from the single
         arming thread, the trip thread only *reads* by index. */
      n = contextPool.size();
      contextPool.push(context);
    }
    armedCtxIndex.store(n);
    deadlineMs.store(NowMs() + seconds * 1000.0);
    running.store(true);
    armed.store(true);
  }

  void Watchdog_Disarm() {
    armed.store(false);
    running.store(false);
    deadlineMs.store(0.0);
  }

  void Watchdog_SetTripAction(
    void (*action)(String const& context, String const& stack))
  {
    tripAction = action;
  }
}
