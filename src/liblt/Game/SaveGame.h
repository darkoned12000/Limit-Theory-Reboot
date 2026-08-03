// Copyright (C) 2025  darkoned12000
// SPDX-License-Identifier: GPL-3.0-or-later
// Part of the ltheory-old-test modernization effort (Revamp Work).
// See NOTICE and LICENSE.GPL. Original engine (c) Josh Parnell, public domain.

#ifndef Game_SaveGame_h__
#define Game_SaveGame_h__

#include "Game/Common.h"
#include "LTE/AutoClass.h"
#include "LTE/String.h"
#include "LTE/V3.h"

const int kSaveGameVersion = 1;

/* Binary snapshot of the player + universe state, persisted via the engine's
 * reflection-based Serializer (SaveTo/LoadFrom). All fields are AutoClass
 * value types (no raw pointers), so a single Process pass round-trips them. */
AutoClass(SaveGameData,
  int, version,
  String, playerName,
  Quantity, playerCredits,
  ItemID, shipHull,
  V3D, playerPos,
  V3D, playerLook,
  uint, universeSeed)

  SaveGameData() :
    version(0),
    playerCredits(0),
    shipHull(0),
    playerPos(V3D(0)),
    playerLook(V3D(0)),
    universeSeed(0)
    {}
};

#endif
