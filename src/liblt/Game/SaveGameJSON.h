// Copyright (C) 2025  darkoned12000
// SPDX-License-Identifier: GPL-3.0-or-later
// Part of the ltheory-old-test modernization effort (Revamp Work).
// See NOTICE and LICENSE.GPL. Original engine (c) Josh Parnell, public domain.

#ifndef Game_SaveGameJSON_h__
#define Game_SaveGameJSON_h__

#include "Game/SaveGame.h"
#include "LTE/String.h"
#include "LTE/Vector.h"

/* JSON save-slot layer. Each slot is a standalone JSON file at
 *   cache/saves/<slot>.json
 * carrying the full SaveGameData plus a dateCreated timestamp and a
 * saveVersion for forward-migration (see ROADMAP §2.1 / SAVE-LOAD Pt2).
 *
 * The engine builds with -fno-exceptions, so the nlohmann/json single header
 * is used exception-free: parse(str, nullptr, false) returns a discarded
 * value on bad input and every field read goes through contains()/value(),
 * so a corrupt or foreign save degrades to "no save exists" instead of
 * crashing. See SaveGameJSON.cpp for the JSON_THROW_USER configuration. */

const int kSaveJSONVersion = 2;

AutoClass(SaveSlotInfo,
  String, slotName,
  String, dateCreated,
  String, playerName,
  uint, universeSeed)

  SaveSlotInfo() : universeSeed(0) {}
};

namespace LTE {
  /* Slot management. `slotName` is the file base name (no extension, no path
     separators). All slots live under OS_GetUserDataPath() + "saves/". */
  LT_API bool SaveGame_Write(String const& slotName, SaveGameData const& data);
  LT_API bool SaveGame_WriteQuicksave(SaveGameData const& data);
  LT_API SaveGameData SaveGame_Read(String const& slotName);
  /* Reads the quicksave slot if present, else the newest slot by dateCreated. */
  LT_API SaveGameData SaveGame_ReadLatest();
  LT_API Vector<String> SaveGame_ListSlotNames();
  LT_API Vector<SaveSlotInfo> SaveGame_ListSlots();
  LT_API String SaveGame_GetSavesDir();
  LT_API String SaveGame_DefaultSlotName();

  /* JSON <-> SaveGameData conversion (exposed for tests). */
  LT_API String SaveGameData_ToJSON(SaveGameData const& data);
  LT_API SaveGameData SaveGameData_FromJSON(String const& contents);
}

#endif
