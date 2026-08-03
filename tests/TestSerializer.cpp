// Copyright (C) 2025  darkoned12000
// SPDX-License-Identifier: GPL-3.0-or-later
// Part of the ltheory-old-test modernization effort (Revamp Work).
// See NOTICE and LICENSE.GPL. Original engine (c) Josh Parnell, public domain.
//
// Round-trip test for the reflection-based Serializer backend. This was the
// historical gap: Data_LoadFrom/Data_SaveTo worked against Location_File (the
// Settings precedent) but no test ever exercised SaveTo/LoadFrom on a custom
// AutoClass value type. Uses Location_Memory so the test stays headless (no
// disk IO, no display/audio device).

#include "Harness.h"
#include "LTE/Array.h"
#include "LTE/Location.h"
#include "LTE/Serializer.h"
#include "Game/SaveGame.h"

#include "LTE/Common.h"

using namespace LTE;

static void RunRoundTrip(SaveGameData src, int version) {
  Location mem = Location_Memory(new Array<uchar>());
  SaveTo(src, mem, version);
  LTE_CHECK(mem->Exists());

  SaveGameData dst;
  bool ok = LoadFrom(dst, mem, version, version);
  LTE_CHECK(ok);
  if (ok) {
    LTE_CHECK_EQ(dst.version, src.version);
    LTE_CHECK_EQ(dst.playerName, src.playerName);
    LTE_CHECK_EQ(dst.playerCredits, src.playerCredits);
    LTE_CHECK_EQ(dst.shipHull, src.shipHull);
    LTE_CHECK_EQ(dst.playerPos, src.playerPos);
    LTE_CHECK_EQ(dst.playerLook, src.playerLook);
    LTE_CHECK_EQ(dst.universeSeed, src.universeSeed);
  }
}

LTE_TEST(Serializer_SaveGameRoundTrip) {
  SaveGameData src;
  src.version = kSaveGameVersion;
  src.playerName = "SuperSpaceSquirrel";
  src.playerCredits = 123456789;
  src.shipHull = 987654321;
  src.playerPos = V3D(12.5, -3.25, 4000.125);
  src.playerLook = V3D(0.0, 0.0, -1.0);
  src.universeSeed = 42;
  RunRoundTrip(src, kSaveGameVersion);
}

LTE_TEST(Serializer_SaveGameDefaultsRoundTrip) {
  // An all-default struct (version 0) must also survive a round trip so a
  // fresh player without a save produces a valid file.
  SaveGameData src;
  RunRoundTrip(src, kSaveGameVersion);
}

LTE_TEST(Serializer_SaveGameVersionMismatchRejected) {
  // A save written at a different version must not load as the current one.
  SaveGameData src;
  src.version = kSaveGameVersion;
  src.universeSeed = 7;
  Location mem = Location_Memory(new Array<uchar>());
  SaveTo(src, mem, kSaveGameVersion);

  SaveGameData dst;
  bool ok = LoadFrom(dst, mem, kSaveGameVersion + 1, kSaveGameVersion + 1);
  LTE_CHECK(!ok);
}

LTE_TEST(Serializer_SaveGameMissingLocationFails) {
  // A missing/corrupt location must fail cleanly (LoadFrom returns false),
  // mirroring how SaveGame_Load reports "no save exists".
  SaveGameData dst;
  bool ok = LoadFrom(dst, Location_Memory(new Array<uchar>()), 0, kSaveGameVersion);
  LTE_CHECK(!ok);
}

LTE_TEST(Serializer_LoadRealAppSaveTEMP) {
  // Depends on an app run having written ./cache/savegame.bin; skip headless.
  if (!Location_File("./cache/savegame.bin")->Exists()) return;
  SaveGameData d;
  bool ok = LoadFrom(d, Location_File("./cache/savegame.bin"), kSaveGameVersion, kSaveGameVersion);
  std::printf("  TEMP ok=%d version=%d name=%s credits=%lld hull=%lld seed=%u pos=(%f,%f,%f) look=(%f,%f,%f)\n",
    ok, d.version, d.playerName.c_str(), (long long)d.playerCredits, (long long)d.shipHull,
    d.universeSeed, d.playerPos.x, d.playerPos.y, d.playerPos.z, d.playerLook.x, d.playerLook.y, d.playerLook.z);
  LTE_CHECK(ok);
}

LTE_TEST(Serializer_ReSaveRealAppSaveTEMP) {
  // Depends on an app run having written ./cache/savegame.bin; skip headless.
  if (!Location_File("./cache/savegame.bin")->Exists()) return;
  SaveGameData d;
  bool ok = LoadFrom(d, Location_File("./cache/savegame.bin"), kSaveGameVersion, kSaveGameVersion);
  if (ok) {
    SaveTo(d, Location_File("./cache/savegame2.bin"), kSaveGameVersion);
  }
  std::printf("  TEMP2 ok=%d\n", ok);
  LTE_CHECK(ok);
}

LTE_TEST(Serializer_StructLayoutTEMP) {
  std::printf("  TEMP3 sizeof(SaveGameData)=%zu sizeof(String)=%zu sizeof(V3D)=%zu sizeof(Quantity)=%zu\n",
    sizeof(SaveGameData), sizeof(String), sizeof(V3D), sizeof(Quantity));
  std::printf("  TEMP3 offsetof: version=%zu name=%zu credits=%zu hull=%zu pos=%zu look=%zu seed=%zu\n",
    offsetof(SaveGameData, version), offsetof(SaveGameData, playerName),
    offsetof(SaveGameData, playerCredits), offsetof(SaveGameData, shipHull),
    offsetof(SaveGameData, playerPos), offsetof(SaveGameData, playerLook),
    offsetof(SaveGameData, universeSeed));
}
