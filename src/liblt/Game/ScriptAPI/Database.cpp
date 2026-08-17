// Copyright (C) 2025  darkoned12000
// SPDX-License-Identifier: GPL-3.0-or-later
// Part of the ltheory-old-test modernization effort (Revamp Work).
// See NOTICE and LICENSE.GPL. Original engine (c) Josh Parnell, public domain.

#include "Game/DatabaseManager.h"

#include "LTE/Function.h"
#include "LTE/FunctionBind.h"
#include "LTE/Script.h"

namespace LTE {

static Function const Database_Load_Registration = Function_Bind(
  "Database_Load",
  "Load a JSON file as a named database. Returns true on success.",
  [](String const& name, String const& path) -> bool
  {
    return DatabaseManager_Get().Load(name, path);
  },
  "name", "path");

static Function const Database_Get_Registration = Function_Bind(
  "Database_Get",
  "Get a top-level value from a named database by key. Returns the raw JSON string.",
  [](String const& db, String const& key) -> String
  {
    json const* val = DatabaseManager_Get().Find(db, key);
    if (!val)
      return "";
    return String(val->dump().c_str());
  },
  "db", "key");

static Function const Database_GetPath_Registration = Function_Bind(
  "Database_GetPath",
  "Get a nested value via dot notation from a named database.",
  [](String const& db, String const& dotPath) -> String
  {
    json const* val = DatabaseManager_Get().FindPath(db, dotPath);
    if (!val)
      return "";
    return String(val->dump().c_str());
  },
  "db", "dotPath");

static Function const Database_Has_Registration = Function_Bind(
  "Database_Has",
  "Check if a key exists in a named database.",
  [](String const& db, String const& key) -> bool
  {
    return DatabaseManager_Get().Has(db, key);
  },
  "db", "key");

static Function const Database_HasDatabase_Registration = Function_Bind(
  "Database_HasDatabase",
  "Check if a named database is loaded.",
  [](String const& db) -> bool
  {
    return DatabaseManager_Get().HasDatabase(db);
  },
  "db");

static Function const Database_Keys_Registration = Function_Bind(
  "Database_Keys",
  "List all top-level keys in a named database.",
  [](String const& db) -> Vector<String>
  {
    return DatabaseManager_Get().Keys(db);
  },
  "db");

static Function const Database_Reload_Registration = Function_Bind(
  "Database_Reload",
  "Reload a named database from its original file path.",
  [](String const& db) -> bool
  {
    return DatabaseManager_Get().Reload(db);
  },
  "db");

}
