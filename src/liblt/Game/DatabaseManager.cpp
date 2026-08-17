// Copyright (C) 2025  darkoned12000
// SPDX-License-Identifier: GPL-3.0-or-later
// Part of the ltheory-old-test modernization effort (Revamp Work).
// See NOTICE and LICENSE.GPL. Original engine (c) Josh Parnell, public domain.

#include "Game/DatabaseManager.h"

DatabaseManager::DatabaseManager() {}
DatabaseManager::~DatabaseManager() {}

bool DatabaseManager::Load(String const& name, String const& path) {
  JsonDatabase db;
  if (!db.Load(path))
    return false;
  databases[name] = db;
  paths[name] = path;
  return true;
}

json const* DatabaseManager::Find(String const& db, String const& key) const {
  JsonDatabase const* dbPtr = databases.get(db);
  if (!dbPtr)
    return nullptr;
  return dbPtr->Find(key);
}

json const* DatabaseManager::FindPath(
  String const& db,
  String const& dotPath) const
{
  JsonDatabase const* dbPtr = databases.get(db);
  if (!dbPtr)
    return nullptr;
  return dbPtr->FindPath(dotPath);
}

Vector<String> DatabaseManager::Keys(String const& db) const {
  JsonDatabase const* dbPtr = databases.get(db);
  if (!dbPtr)
    return Vector<String>();
  return dbPtr->Keys();
}

bool DatabaseManager::Has(String const& db, String const& key) const {
  return Find(db, key) != nullptr;
}

bool DatabaseManager::HasDatabase(String const& db) const {
  return databases.contains(db);
}

bool DatabaseManager::Reload(String const& db) {
  String const* pathStr = paths.get(db);
  if (!pathStr)
    return false;
  return Load(db, *pathStr);
}

Vector<String> DatabaseManager::DatabaseNames() const {
  Vector<String> names;
  for (auto it = databases.begin(); it != databases.end(); ++it)
    names.push(it->first);
  return names;
}

JsonDatabase const* DatabaseManager::GetDatabase(String const& db) const {
  return databases.get(db);
}

void DatabaseManager::Erase(String const& db) {
  databases.erase(db);
  paths.erase(db);
}

static DatabaseManager sDatabaseManager;

DatabaseManager& DatabaseManager_Get() {
  return sDatabaseManager;
}
