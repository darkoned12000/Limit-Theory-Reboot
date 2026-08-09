// Copyright (C) 2025  darkoned12000
// SPDX-License-Identifier: GPL-3.0-or-later
// Part of the ltheory-old-test modernization effort (Revamp Work).
// See NOTICE and LICENSE.GPL. Original engine (c) Josh Parnell, public domain.
//
// Round-trip tests for the JSON save-slot layer (SaveGameJSON). Exercises the
// to/from-JSON conversion and the slot write/read/list path without touching
// the binary Serializer. Uses distinct slot names in the user-data dir
// (cache/saves/) and removes them after, so the suite stays headless and
// leaves no artifacts behind.

#include "Harness.h"
#include "Game/SaveGameJSON.h"

#include <cstdio>

using namespace LTE;

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
  d.playerName = name;
  d.playerCredits = 125000;
  d.shipHull = 987654321;
  d.playerPos = V3D(12.5, -3.25, 4000.125);
  d.playerLook = V3D(0.0, 0.0, -1.0);
  d.universeSeed = 42;
  return d;
}

LTE_TEST(SaveGameJSON_ToFromJSONRoundTrip) {
  SaveGameData src = MakeData("SuperSpaceSquirrel");
  SaveGameData dst = SaveGameData_FromJSON(SaveGameData_ToJSON(src));
  LTE_CHECK_EQ(dst.version, src.version);
  LTE_CHECK_EQ(dst.dateCreated, src.dateCreated);
  LTE_CHECK_EQ(dst.playerName, src.playerName);
  LTE_CHECK_EQ(dst.playerCredits, src.playerCredits);
  LTE_CHECK_EQ(dst.shipHull, src.shipHull);
  LTE_CHECK_EQ(dst.playerPos, src.playerPos);
  LTE_CHECK_EQ(dst.playerLook, src.playerLook);
  LTE_CHECK_EQ(dst.universeSeed, src.universeSeed);
}

LTE_TEST(SaveGameJSON_InvalidJSONFails) {
  SaveGameData dst = SaveGameData_FromJSON("{not json");
  LTE_CHECK_EQ(dst.version, 0);
  dst = SaveGameData_FromJSON("");
  LTE_CHECK_EQ(dst.version, 0);
  dst = SaveGameData_FromJSON("[1,2,3]");
  LTE_CHECK_EQ(dst.version, 0);
}

LTE_TEST(SaveGameJSON_MissingFieldFallsBack) {
  // A JSON object that is valid but missing optional fields must not crash
  // and must keep the defaults (exception-free accessors).
  SaveGameData dst = SaveGameData_FromJSON("{\"saveVersion\":2,\"playerName\":\"X\"}");
  LTE_CHECK_EQ(dst.version, 2);
  LTE_CHECK_EQ(dst.playerName, "X");
  LTE_CHECK_EQ(dst.playerCredits, 0);
  LTE_CHECK_EQ(dst.playerPos, V3D(0));
}

LTE_TEST(SaveGameJSON_WriteReadSlot) {
  String slot = UniqueSlot();
  SaveGameData src = MakeData("SlotWriter");
  bool ok = SaveGame_Write(slot, src);
  LTE_CHECK(ok);
  SaveGameData dst = SaveGame_Read(slot);
  LTE_CHECK_EQ(dst.version, kSaveJSONVersion);
  LTE_CHECK_EQ(dst.playerName, src.playerName);
  LTE_CHECK_EQ(dst.universeSeed, src.universeSeed);
}

LTE_TEST(SaveGameJSON_ListSlotsFindsNewestFirst) {
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
}

LTE_TEST(SaveGameJSON_ReadMissingSlotFails) {
  String slot = UniqueSlot();
  SaveGameData dst = SaveGame_Read(slot);
  LTE_CHECK_EQ(dst.version, 0);
}

LTE_TEST(SaveGameJSON_QuicksaveRoundTrip) {
  SaveGameData src = MakeData("QuicksavePlayer");
  LTE_CHECK(SaveGame_WriteQuicksave(src));
  SaveGameData dst = SaveGame_ReadLatest();
  LTE_CHECK_EQ(dst.playerName, src.playerName);
  LTE_CHECK_EQ(dst.universeSeed, src.universeSeed);
}
