// Copyright (C) 2025  darkoned12000
// SPDX-License-Identifier: GPL-3.0-or-later
// Part of the ltheory-old-test modernization effort (Revamp Work).
// See NOTICE and LICENSE.GPL. Original engine (c) Josh Parnell, public domain.

#ifndef Game_JsonDatabase_h__
#define Game_JsonDatabase_h__

#include "Game/Common.h"
#include "LTE/String.h"
#include "LTE/Vector.h"

#include "json/json.hpp"

using json = nlohmann::json;

/* A single JSON data file loaded into memory with a top-level key index.
 * Used by DatabaseManager to serve data to LTSL scripts and C++ factories.
 *
 * Usage:
 *   JsonDatabase db;
 *   db.Load("resource/gamedata/planets.json");
 *   json const* biome = db.Find("terran");  // top-level key lookup
 *
 * Non-throwing: all parsing uses json::parse(str, nullptr, false) so a
 * corrupt file degrades to an empty database instead of crashing. */

class JsonDatabase {
public:
  JsonDatabase();
  ~JsonDatabase();

  /* Load a JSON file. Returns true on success. On failure, sets error
   * message accessible via GetError(). */
  bool Load(String const& path);

  /* Load from an in-memory string. path is for error messages only. */
  bool LoadFromString(String const& contents, String const& label = "mem");

  /* Look up a top-level key. Returns nullptr if not found. */
  json const* Find(String const& key) const;

  /* Look up a nested path via dot notation: "ships.fighter.hull.valueRatio" */
  json const* FindPath(String const& dotPath) const;

  /* List all top-level keys. */
  Vector<String> Keys() const;

  /* Number of top-level entries. */
  int Size() const;

  /* Schema version from the "version" field, or 0 if absent. */
  int Version() const;

  /* Is the database loaded and valid? */
  bool IsLoaded() const;

  /* Error message from last Load() call, or empty. */
  String const& GetError() const;

  /* Source file path. */
  String const& GetPath() const;

  /* Raw JSON access (for LTSL JsonValue wrapping). */
  json const& Raw() const;

private:
  json data;
  String path;
  String error;
  bool loaded;
};

#endif
