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
  /* Build the SaveGameData snapshot of the player's state. Shared by the
     quicksave and named-slot save bindings. */
  static SaveGameData BuildSaveData(Player const& player, Object const& root) {
    Object ship = player->piloting;
    SaveGameData d;
    d.version = kSaveJSONVersion;
    d.playerName = player->GetName();
    d.systemName = root->GetName();
    d.playerCredits = player->GetCredits();
    d.universeSeed = root->GetSeed();

    if (ship) {
      d.shipHull = ship->GetSupertype() ? ship->GetSupertype()->GetID() : 0;
      d.playerPos = ship->GetPos();
      d.playerLook = V3D(ship->GetLook());
    }

    return d;
  }

  static Function const SaveGame_Create_Registration = Function_Bind(
  "SaveGame_Create",
  "Write the current game state (player credits, ship position/look, and the"
    " universe seed) to a new quicksave slot. Each quick save accumulates its"
    " own timestamped slot instead of overwriting, and counts against the 25"
    " save-slot cap. Returns true on success.",
  [](Player const& player, Object const& root) -> bool
  {
    return SaveGame_WriteQuicksave(BuildSaveData(player, root));
  
  },
  "player", "root");
static int const SaveGame_Create_Alias = Function_Alias("SaveGame_Create", "SaveGame");

  static Function const SaveGame_SaveSlot_Registration = Function_Bind(
  "SaveGame_SaveSlot",
  "Write the current game state to the named save slot, stamping it with a"
    " custom save name and description. Overwrites the slot if it already"
    " exists; fails (returns false) if the slot is new and the save cap"
    " (25 slots, quicksaves included) has been reached.",
  [](Player const& player, Object const& root, String const& slotName,
     String const& saveName, String const& saveDescription) -> bool
  {
    SaveGameData d = BuildSaveData(player, root);
    d.saveName = saveName;
    d.saveDescription = saveDescription;
    return SaveGame_Write(slotName, d);
  
  },
  "player", "root", "slotName", "saveName", "saveDescription");

  static Function const SaveGame_Delete_Registration = Function_Bind(
  "SaveGame_Delete",
  "Permanently delete the named save slot (quicksave slots included). Returns"
    " true if a slot was removed.",
  [](String const& slotName) -> bool
  {
    return SaveGame_Delete(slotName);
  
  },
  "slotName");

  static Function const SaveGame_Exists_Registration = Function_Bind(
  "SaveGame_Exists",
  "Return whether a readable save exists in the named slot.",
  [](String const& slotName) -> bool
  {
    return SaveGame_Exists(slotName);
  
  },
  "slotName");

  static Function const SaveGame_Count_Registration = Function_Bind(
  "SaveGame_Count",
  "Return the number of save slots currently present (quicksaves and named"
    " saves together).",
  []() -> int
  {
    return SaveGame_Count();
  
  });

  static Function const SaveGame_Load_Registration = Function_Bind(
  "SaveGame_Load",
  "Read the saved game state from the most recent save slot (newest by date)."
    " Returns a SaveGameData whose 'version' is 0 when no save exists (or it is"
    " corrupt / from an incompatible version).",
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
  "Return metadata (slot name, date created, custom save name/description,"
    " player name, credits, player position, universe seed) for every save"
    " slot, newest first.",
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
