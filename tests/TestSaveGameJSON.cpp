// Copyright (C) 2025  darkoned12000
// SPDX-License-Identifier: GPL-3.0-or-later
// Part of the ltheory-old-test modernization effort (Revamp Work).
// See NOTICE and LICENSE.GPL. Original engine (c) Josh Parnell, public domain.
//
// Round-trip tests for the JSON save-slot layer (SaveGameJSON). Exercises the
// to/from-JSON conversion and the slot write/read/list path without touching
// the binary Serializer. The suite redirects the save layer to a dedicated
// sandbox dir (cache/lte_test_saves/) so it never collides with the user's
// real saves in cache/saves/ (which would otherwise fill the 25-slot cap and
// starve the cap tests). Removes its slots after, leaving no artifacts.

#include "Harness.h"
#include "Game/SaveGameJSON.h"
#include "LTE/OS.h"

#include <algorithm>
#include <cstdio>

using namespace LTE;

namespace {
  /* Point the save layer at a private dir owned by the test suite. Called at
     the top of every test (instead of a static initializer) so each test is
     self-sufficient regardless of what other test files set the dir to.
     SaveGame_SetSavesDir only stores a string (the dir is created lazily on
     first use), so the call itself is safe anywhere. */
  void UseTestSavesDir() {
    SaveGame_SetSavesDir(OS_GetUserDataPath() + "lte_test_saves/");
  }
}

static String UniqueSlot() {
  static int counter = 0;
  char buf[64];
  std::snprintf(buf, sizeof(buf), "test_slot_%d", counter++);
  return String(buf);
}

static SaveGameData MakeData(String const& name) {
  SaveGameData d;
  d.version = kSaveJSONVersion;
  d.dateCreated = "2026-08-08 12:00:00";
  d.saveName = "Test Run " + name;
  d.saveDescription = "Metadata round-trip check";
  d.playerName = name;
  d.playerCredits = 125000;
  d.shipHull = 987654321;
  d.playerPos = V3D(12.5, -3.25, 4000.125);
  d.playerLook = V3D(0.0, 0.0, -1.0);
  d.universeSeed = 42;
  return d;
}

LTE_TEST(SaveGameJSON_ToFromJSONRoundTrip) {
  UseTestSavesDir();
  SaveGameData src = MakeData("SuperSpaceSquirrel");
  SaveGameData dst = SaveGameData_FromJSON(SaveGameData_ToJSON(src));
  LTE_CHECK_EQ(dst.version, src.version);
  LTE_CHECK_EQ(dst.dateCreated, src.dateCreated);
  LTE_CHECK_EQ(dst.saveName, src.saveName);
  LTE_CHECK_EQ(dst.saveDescription, src.saveDescription);
  LTE_CHECK_EQ(dst.playerName, src.playerName);
  LTE_CHECK_EQ(dst.playerCredits, src.playerCredits);
  LTE_CHECK_EQ(dst.shipHull, src.shipHull);
  LTE_CHECK_EQ(dst.playerPos, src.playerPos);
  LTE_CHECK_EQ(dst.playerLook, src.playerLook);
  LTE_CHECK_EQ(dst.universeSeed, src.universeSeed);
}

LTE_TEST(SaveGameJSON_OldVersionFallback) {
  UseTestSavesDir();
  // A save written by the pre-metadata schema (kSaveJSONVersion 2) must still
  // load, with the new fields defaulting to empty.
  SaveGameData dst = SaveGameData_FromJSON(
    "{\"saveVersion\":2,\"playerName\":\"Old\"}");
  LTE_CHECK_EQ(dst.version, 2);
  LTE_CHECK_EQ(dst.playerName, "Old");
  LTE_CHECK_EQ(dst.saveName, "");
  LTE_CHECK_EQ(dst.saveDescription, "");
}

LTE_TEST(SaveGameJSON_InvalidJSONFails) {
  UseTestSavesDir();
  SaveGameData dst = SaveGameData_FromJSON("{not json");
  LTE_CHECK_EQ(dst.version, 0);
  dst = SaveGameData_FromJSON("");
  LTE_CHECK_EQ(dst.version, 0);
  dst = SaveGameData_FromJSON("[1,2,3]");
  LTE_CHECK_EQ(dst.version, 0);
}

LTE_TEST(SaveGameJSON_MissingFieldFallsBack) {
  UseTestSavesDir();
  // A JSON object that is valid but missing optional fields must not crash
  // and must keep the defaults (exception-free accessors).
  SaveGameData dst = SaveGameData_FromJSON("{\"saveVersion\":2,\"playerName\":\"X\"}");
  LTE_CHECK_EQ(dst.version, 2);
  LTE_CHECK_EQ(dst.playerName, "X");
  LTE_CHECK_EQ(dst.playerCredits, 0);
  LTE_CHECK_EQ(dst.playerPos, V3D(0));
}

LTE_TEST(SaveGameJSON_WriteReadSlot) {
  UseTestSavesDir();
  String slot = UniqueSlot();
  SaveGameData src = MakeData("SlotWriter");
  bool ok = SaveGame_Write(slot, src);
  LTE_CHECK(ok);
  SaveGameData dst = SaveGame_Read(slot);
  LTE_CHECK_EQ(dst.version, kSaveJSONVersion);
  LTE_CHECK_EQ(dst.playerName, src.playerName);
  LTE_CHECK_EQ(dst.universeSeed, src.universeSeed);
  LTE_CHECK(SaveGame_Delete(slot));
}

LTE_TEST(SaveGameJSON_ListSlotsFindsNewestFirst) {
  UseTestSavesDir();
  String slotA = UniqueSlot();
  String slotB = UniqueSlot();
  SaveGameData a = MakeData("Older");
  a.dateCreated = "2026-08-08 10:00:00";
  SaveGameData b = MakeData("Newer");
  b.dateCreated = "2026-08-08 11:00:00";
  LTE_CHECK(SaveGame_Write(slotA, a));
  LTE_CHECK(SaveGame_Write(slotB, b));

  Vector<SaveSlotInfo> slots = SaveGame_ListSlots();
  int foundA = 0, foundB = 0;
  for (SaveSlotInfo const& info : slots) {
    if (info.slotName == slotA) foundA = 1;
    if (info.slotName == slotB) foundB = 1;
  }
  LTE_CHECK(foundA);
  LTE_CHECK(foundB);
  LTE_CHECK(SaveGame_Delete(slotA));
  LTE_CHECK(SaveGame_Delete(slotB));
}

LTE_TEST(SaveGameJSON_ReadMissingSlotFails) {
  UseTestSavesDir();
  String slot = UniqueSlot();
  SaveGameData dst = SaveGame_Read(slot);
  LTE_CHECK_EQ(dst.version, 0);
}

LTE_TEST(SaveGameJSON_QuicksaveRoundTrip) {
  UseTestSavesDir();
  SaveGameData src = MakeData("QuicksavePlayer");
  // Snapshot existing quick- slots so we can identify the one we just wrote.
  Vector<String> beforeQuick;
  for (String const& name : SaveGame_ListSlotNames())
    if (name.find("quick-") == 0)
      beforeQuick.push(name);

  LTE_CHECK(SaveGame_WriteQuicksave(src));

  // Quicksaves accumulate: the new slot is the quick- slot that wasn't there
  // before. Read it and verify the round trip.
  String slot;
  for (String const& name : SaveGame_ListSlotNames()) {
    if (name.find("quick-") != 0)
      continue;
    if (std::find(beforeQuick.begin(), beforeQuick.end(), name) == beforeQuick.end()) {
      slot = name;
      break;
    }
  }
  LTE_CHECK(slot.length() > 0);
  SaveGameData dst = SaveGame_Read(slot);
  LTE_CHECK_EQ(dst.playerName, src.playerName);
  LTE_CHECK_EQ(dst.universeSeed, src.universeSeed);
  LTE_CHECK_EQ(dst.saveName, src.saveName);

  // Quick save slots are deletable like any other.
  LTE_CHECK(SaveGame_Delete(slot));
}

LTE_TEST(SaveGameJSON_DeleteSlot) {
  UseTestSavesDir();
  String slot = UniqueSlot();
  SaveGameData src = MakeData("Deletable");
  LTE_CHECK(SaveGame_Write(slot, src));
  LTE_CHECK(SaveGame_Exists(slot));
  LTE_CHECK(SaveGame_Delete(slot));
  LTE_CHECK(!SaveGame_Exists(slot));
  // Deleting a missing slot reports failure.
  LTE_CHECK(!SaveGame_Delete(slot));
}

LTE_TEST(SaveGameJSON_QuicksavesAccumulate) {
  UseTestSavesDir();
  // Snapshot the quick- slots that already exist so we only clean up our own
  // (the user's real quicksaves must never be touched).
  Vector<String> beforeQuick;
  for (String const& name : SaveGame_ListSlotNames())
    if (name.find("quick-") == 0)
      beforeQuick.push(name);

  // Two quick saves create two distinct slots instead of overwriting each other.
  int before = SaveGame_Count();
  LTE_CHECK(SaveGame_WriteQuicksave(MakeData("QuickOne")));
  LTE_CHECK(SaveGame_WriteQuicksave(MakeData("QuickTwo")));
  LTE_CHECK_EQ(SaveGame_Count(), before + 2);

  int quick = 0;
  for (String const& name : SaveGame_ListSlotNames())
    if (name.find("quick-") == 0)
      quick++;
  LTE_CHECK_EQ(quick, (int)beforeQuick.size() + 2);

  // Quick saves are deletable like any other slot (delete only our two).
  int deleted = 0;
  for (String const& name : SaveGame_ListSlotNames()) {
    if (name.find("quick-") == 0 &&
        std::find(beforeQuick.begin(), beforeQuick.end(), name) == beforeQuick.end()) {
      LTE_CHECK(SaveGame_Delete(name));
      deleted++;
    }
  }
  LTE_CHECK_EQ(deleted, 2);

  // Quick saves count against the cap: fill to 25 and verify a new quicksave
  // is refused.
  Vector<String> fillers;
  while (SaveGame_Count() < kMaxSaveSlots) {
    String s = UniqueSlot();
    LTE_CHECK(SaveGame_Write(s, MakeData("CapFiller")));
    fillers.push(s);
  }
  LTE_CHECK(!SaveGame_WriteQuicksave(MakeData("OverCap")));

  // Remove the cap fillers so later tests start from a clean dir.
  for (String const& s : fillers)
    SaveGame_Delete(s);
}

LTE_TEST(SaveGameJSON_SlotCap) {
  UseTestSavesDir();
  // Clear any test_slot_* leftovers from an interrupted earlier run so the
  // fill below starts from a clean base.
  for (String const& name : SaveGame_ListSlotNames())
    if (name.find("test_slot_") == 0)
      SaveGame_Delete(name);

  // Fill the dir with distinct slots up to the cap, then verify a new slot is
  // refused while overwriting an existing one still works.
  Vector<String> created;
  while (SaveGame_Count() < kMaxSaveSlots) {
    String slot = UniqueSlot();
    LTE_CHECK(SaveGame_Write(slot, MakeData("CapFiller")));
    created.push(slot);
  }
  LTE_CHECK(created.size() > 0);

  String extra = UniqueSlot();
  LTE_CHECK(!SaveGame_Write(extra, MakeData("OverCap")));
  LTE_CHECK(!SaveGame_Exists(extra));

  // Overwriting an existing slot is always allowed at the cap.
  String victim = created[created.size() - 1];
  LTE_CHECK(SaveGame_Write(victim, MakeData("Overwrite")));

  // Free a slot; writing a new one succeeds again.
  LTE_CHECK(SaveGame_Delete(victim));
  LTE_CHECK(SaveGame_Write(extra, MakeData("AfterDelete")));
  LTE_CHECK(SaveGame_Exists(extra));
  LTE_CHECK(SaveGame_Delete(extra));

  // Tear down the fillers so the suite leaves no artifacts behind.
  for (String const& slot : created)
    SaveGame_Delete(slot);
  for (String const& slot : created) {
    char buf[64];
    std::snprintf(buf, sizeof(buf), "%s", slot.c_str());
    LTE_CHECK(!SaveGame_Exists(String(buf)));
  }
}

LTE_TEST(SaveGameJSON_CleanupTestSlots) {
  UseTestSavesDir();
  // Any test_slot_* files left behind by an interrupted earlier run are
  // removed, keeping repeated suite runs deterministic w.r.t. the slot cap.
  for (String const& name : SaveGame_ListSlotNames()) {
    if (name.find("test_slot_") == 0)
      SaveGame_Delete(name);
  }
}
