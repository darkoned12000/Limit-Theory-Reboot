// Copyright (C) 2025  darkoned12000
// SPDX-License-Identifier: GPL-3.0-or-later
// Part of the ltheory-old-test modernization effort (Revamp Work).
// See NOTICE and LICENSE.GPL. Original engine (c) Josh Parnell, public domain.

#include "Game/Player.h"
#include "Game/SaveGame.h"

#include "LTE/Function.h"
#include "LTE/FunctionBind.h"
#include "LTE/Location.h"
#include "LTE/OS.h"
#include "LTE/Serializer.h"

namespace LTE {
  const char* kSaveGameFile = "savegame.bin";

  static Function const SaveGame_Create_Registration = Function_Bind(
  "SaveGame_Create",
  "Write the current game state (player credits, ship position/look, and the"
    " universe seed) to the persistent save file. Returns true on success.",
  [](Player const& player, Object const& root) -> bool
  {
    Object ship = player->piloting;
    SaveGameData d;
    d.version = kSaveGameVersion;
    d.playerName = player->GetName();
    d.playerCredits = player->GetCredits();
    d.universeSeed = root->GetSeed();

    if (ship) {
      d.shipHull = ship->GetSupertype() ? ship->GetSupertype()->GetID() : 0;
      d.playerPos = ship->GetPos();
      d.playerLook = V3D(ship->GetLook());
    }

    SaveTo(d, Location_File(OS_GetUserDataPath() + kSaveGameFile), kSaveGameVersion);
    fprintf(stderr, "TEMPDEBUG credits=%lld hull=%lld seed=%u pos=(%f,%f,%f) look=(%f,%f,%f) ship=%p\n",
      (long long)d.playerCredits, (long long)d.shipHull, d.universeSeed,
      d.playerPos.x, d.playerPos.y, d.playerPos.z,
      d.playerLook.x, d.playerLook.y, d.playerLook.z, (void*)&*ship);
    return true;
  
  },
  "player", "root");
static int const SaveGame_Create_Alias = Function_Alias("SaveGame_Create", "SaveGame");

  static Function const SaveGame_Load_Registration = Function_Bind(
  "SaveGame_Load",
  "Read the saved game state from the persistent save file. Returns a"
    " SaveGameData whose 'version' is 0 when no save exists (or it is corrupt"
    " / from an incompatible version).",
  []() -> SaveGameData
  {
    SaveGameData d;
    Location location = Location_File(OS_GetUserDataPath() + kSaveGameFile);
    if (!LoadFrom(d, location, kSaveGameVersion, kSaveGameVersion))
      return SaveGameData();
    return d;
  
  });
static int const SaveGame_Load_Alias = Function_Alias("SaveGame_Load", "LoadGame");
}
