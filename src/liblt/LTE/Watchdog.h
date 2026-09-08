// Copyright (C) 2025  darkoned12000
// SPDX-License-Identifier: GPL-3.0-or-later
// Part of the ltheory-old-test modernization effort (Revamp Work).
// See NOTICE and LICENSE.GPL. Original engine (c) Josh Parnell, public domain.

#ifndef LTE_Watchdog_h__
#define LTE_Watchdog_h__

#include "Common.h"
#include "String.h"

namespace LTE {
  /* P1-4 startup watchdog (ltsl-hardening.md §8). Arms a deadline around a
     potentially-hanging section (app Initialize / per-frame Update). A
     background thread fires the trip action if the deadline passes while
     armed — turning silent hangs (while-loop class, 2026-08-04) into a
     logged, loud failure with a stack dump instead of a frozen window.

     Trip action per §9: log+abort (fail loud). A legitimately slow frame
     that RETURNS never trips the watchdog — only a section that never
     returns does. Thresholds are deliberately generous (see Program.cpp)
     so loading screens stay far inside budget. */

  LT_API void Watchdog_Arm(double seconds, String const& context);
  LT_API void Watchdog_Disarm();

  /* Test seam (also for alternate policies): install a custom trip action.
     Default: writes a crash-style report (signal 0) and fails loud via
     LTE_ASSERT(false). */
  LT_API void Watchdog_SetTripAction(
    void (*action)(String const& context, String const& stack));
}

#endif
