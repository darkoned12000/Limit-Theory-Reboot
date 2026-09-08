// Copyright (C) 2025  darkoned12000
// SPDX-License-Identifier: GPL-3.0-or-later
// Part of the ltheory-old-test modernization effort (Revamp Work).
// See NOTICE and LICENSE.GPL. Original engine (c) Josh Parnell, public domain.

// Watchdog (ltsl-hardening.md §8 P1-4). The default trip action writes a
// crash log and fails loud via assert — exit-heavy, so these tests install
// a custom trip action (the documented test seam), verify the state
// machine (trip on timeout, no trip on disarm/return), and restore the
// default action afterwards.

#include "Harness.h"
#include "LTE/Watchdog.h"

#include <atomic>
#include <chrono>
#include <thread>

using namespace LTE;

namespace {
  std::atomic<int> tripCount(0);
  String lastContext;

  void CountingTripAction(String const& context, String const&) {
    lastContext = context;
    tripCount++;
  }
}

static void InstallCountingAction() {
  tripCount.store(0);
  Watchdog_SetTripAction(&CountingTripAction);
}

static void RestoreDefaultAction() {
  Watchdog_SetTripAction(nullptr);
}

LTE_TEST(Watchdog_TripsOnDeadline) {
  InstallCountingAction();

  Watchdog_Arm(0.05, "test-hang");
  std::this_thread::sleep_for(std::chrono::milliseconds(300));

  LTE_CHECK_EQ((int)tripCount, 1);
  LTE_CHECK(lastContext == "test-hang");

  RestoreDefaultAction();
}

LTE_TEST(Watchdog_DisarmCancelsPending) {
  InstallCountingAction();
  size_t before = tripCount.load();

  Watchdog_Arm(0.3, "test-cancel");
  /* A legitimately fast section: arm, work, disarm — never trip. */
  std::this_thread::sleep_for(std::chrono::milliseconds(50));
  Watchdog_Disarm();
  std::this_thread::sleep_for(std::chrono::milliseconds(400));

  LTE_CHECK_EQ((int)tripCount.load(), (int)before);

  RestoreDefaultAction();
}

LTE_TEST(Watchdog_RearmAfterTripWorks) {
  InstallCountingAction();
  int before = tripCount.load();

  Watchdog_Arm(0.05, "test-again");
  std::this_thread::sleep_for(std::chrono::milliseconds(300));
  LTE_CHECK_EQ((int)tripCount.load(), before + 1);

  /* A fresh arm after the trip window must work normally. */
  Watchdog_Arm(10.0, "test-post-trip");
  std::this_thread::sleep_for(std::chrono::milliseconds(150));
  Watchdog_Disarm();
  LTE_CHECK_EQ((int)tripCount.load(), before + 1);

  RestoreDefaultAction();
}
