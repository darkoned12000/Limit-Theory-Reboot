// Copyright (C) 2025  darkoned12000
// SPDX-License-Identifier: GPL-3.0-or-later
// Part of the ltheory-old-test modernization effort (Revamp Work).
// See NOTICE and LICENSE.GPL. Original engine (c) Josh Parnell, public domain.

#ifndef Game_DatabaseManager_h__
#define Game_DatabaseManager_h__

#include "Game/Common.h"
#include "Game/JsonDatabase.h"
#include "LTE/Map.h"
#include "LTE/String.h"
#include "LTE/Vector.h"

/* Central registry for JSON data databases. Loads and caches JSON files
 * from resource/gamedata/, serves them to LTSL scripts and C++ factories.
 *
 * Singleton accessed via DatabaseManager_Get(). Loaded at Launcher::Launch()
 * before any scripts run.
 *
 * Usage from C++:
 *   DatabaseManager_Get().Load("planets", "resource/gamedata/planets.json");
 *   json const* biome = DatabaseManager_Get().Find("planets", "terran");
 *
 * Usage from LTSL (via bindings):
 *   Database_Load "planets" "resource/gamedata/planets.json"
 *   var biome (Database_Get "planets" "terran") */

class DatabaseManager {
public:
  DatabaseManager();
  ~DatabaseManager();

  /* Load a JSON file as a named database. Overwrites if name already exists.
   * Returns true on success. */
  bool Load(String const& name, String const& path);

  /* Find a top-level key in a named database. Returns nullptr if not found. */
  json const* Find(String const& db, String const& key) const;

  /* Find via dot notation in a named database. */
  json const* FindPath(String const& db, String const& dotPath) const;

  /* List all top-level keys in a named database. */
  Vector<String> Keys(String const& db) const;

  /* Check if a key exists in a named database. */
  bool Has(String const& db, String const& key) const;

  /* Check if a named database is loaded. */
  bool HasDatabase(String const& db) const;

  /* Reload a named database from its original path. */
  bool Reload(String const& db);

  /* List all loaded database names. */
  Vector<String> DatabaseNames() const;

  /* Remove a named database. */
  void Erase(String const& db);

  /* Get a database by name (for direct access). Returns nullptr if not found. */
  JsonDatabase const* GetDatabase(String const& db) const;

private:
  Map<String, JsonDatabase> databases;
  Map<String, String> paths;  /* name → original load path for Reload */
};

/* Global singleton — initialized once at startup. */
DatabaseManager& DatabaseManager_Get();

#endif
