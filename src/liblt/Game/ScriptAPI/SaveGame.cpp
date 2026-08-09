// Copyright (C) 2025  darkoned12000
// SPDX-License-Identifier: GPL-3.0-or-later
// Part of the ltheory-old-test modernization effort (Revamp Work).
// See NOTICE and LICENSE.GPL. Original engine (c) Josh Parnell, public domain.

#include "Game/Player.h"
#include "Game/SaveGame.h"
#include "Game/SaveGameJSON.h"

#include "LTE/Function.h"
#include "LTE/FunctionBind.h"
#include "LTE/Location.h"
#include "LTE/OS.h"
#include "LTE/Serializer.h"

#include <algorithm>

namespace LTE {
  static Function const SaveGame_Create_Registration = Function_Bind(
  "SaveGame_Create",
  "Write the current game state (player credits, ship position/look, and the"
    " universe seed) to the quicksave slot. Returns true on success.",
  [](Player const& player, Object const& root) -> bool
  {
    Object ship = player->piloting;
    SaveGameData d;
    d.version = kSaveJSONVersion;
    d.playerName = player->GetName();
    d.playerCredits = player->GetCredits();
    d.universeSeed = root->GetSeed();

    if (ship) {
      d.shipHull = ship->GetSupertype() ? ship->GetSupertype()->GetID() : 0;
      d.playerPos = ship->GetPos();
      d.playerLook = V3D(ship->GetLook());
    }

    return SaveGame_WriteQuicksave(d);
  
  },
  "player", "root");
static int const SaveGame_Create_Alias = Function_Alias("SaveGame_Create", "SaveGame");

  static Function const SaveGame_Load_Registration = Function_Bind(
  "SaveGame_Load",
  "Read the saved game state from the most recent save slot (quicksave first,"
    " then newest by date). Returns a SaveGameData whose 'version' is 0 when no"
    " save exists (or it is corrupt / from an incompatible version).",
  []() -> SaveGameData
  {
    return SaveGame_ReadLatest();
  
  });
static int const SaveGame_Load_Alias = Function_Alias("SaveGame_Load", "LoadGame");

  static Function const SaveGame_LoadSlot_Registration = Function_Bind(
  "SaveGame_LoadSlot",
  "Read the saved game state from the named save slot. Returns a SaveGameData"
    " whose 'version' is 0 when the slot does not exist (or is corrupt / from"
    " an incompatible version).",
  [](String const& slotName) -> SaveGameData
  {
    return SaveGame_Read(slotName);
  
  },
  "slotName");

  static Function const SaveGame_ListSlots_Registration = Function_Bind(
  "SaveGame_ListSlots",
  "Return metadata (slot name, date created, player name, universe seed) for"
    " every save slot, newest first.",
  []() -> Vector<SaveSlotInfo>
  {
    Vector<SaveSlotInfo> slots = SaveGame_ListSlots();
    std::sort(slots.begin(), slots.end(),
      [](SaveSlotInfo const& a, SaveSlotInfo const& b) {
        return a.dateCreated > b.dateCreated;
      });
    return slots;
  
  });
}
