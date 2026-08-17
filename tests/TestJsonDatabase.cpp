// Copyright (C) 2025  darkoned12000
// SPDX-License-Identifier: GPL-3.0-or-later
// Part of the ltheory-old-test modernization effort (Revamp Work).
// See NOTICE and LICENSE.GPL. Original engine (c) Josh Parnell, public domain.

#include "Harness.h"
#include "Game/DatabaseManager.h"
#include "Game/JsonDatabase.h"
#include "LTE/OS.h"

#include <cstdio>

static String WriteTestJson(const char* filename, const char* content) {
  String path = String(OS_GetUserDataPath()) + filename;
  FILE* f = fopen(path.c_str(), "w");
  if (!f)
    return "";
  fprintf(f, "%s", content);
  fclose(f);
  return path;
}

static void CleanupTestFile(const char* filename) {
  String path = String(OS_GetUserDataPath()) + filename;
  remove(path.c_str());
}

LTE_TEST(JsonDatabase_LoadValidJson) {
  String path = WriteTestJson("test_db.json",
    R"({"version": 1, "name": "test", "value": 42})");
  JsonDatabase db;
  LTE_CHECK(db.Load(path));
  LTE_CHECK(db.IsLoaded());
  LTE_CHECK_EQ(db.Version(), 1);
  CleanupTestFile("test_db.json");
}

LTE_TEST(JsonDatabase_FindExistingKey) {
  String path = WriteTestJson("test_db2.json",
    R"({"version": 1, "ships": {"fighter": {"hp": 100}}})");
  JsonDatabase db;
  db.Load(path);
  json const* val = db.Find("ships");
  LTE_CHECK(val != nullptr);
  LTE_CHECK(val->is_object());
  CleanupTestFile("test_db2.json");
}

LTE_TEST(JsonDatabase_FindMissingKey) {
  String path = WriteTestJson("test_db3.json",
    R"({"version": 1, "ships": {}})");
  JsonDatabase db;
  db.Load(path);
  json const* val = db.Find("nonexistent");
  LTE_CHECK(val == nullptr);
  CleanupTestFile("test_db3.json");
}

LTE_TEST(JsonDatabase_FindPath) {
  String path = WriteTestJson("test_db4.json",
    R"({"version": 1, "ships": {"fighter": {"hull": {"hp": 100}}}})");
  JsonDatabase db;
  db.Load(path);
  json const* val = db.FindPath("ships.fighter.hull.hp");
  LTE_CHECK(val != nullptr);
  LTE_CHECK(val->is_number());
  LTE_CHECK_EQ(val->get<int>(), 100);
  CleanupTestFile("test_db4.json");
}

LTE_TEST(JsonDatabase_FindPathMissing) {
  String path = WriteTestJson("test_db5.json",
    R"({"version": 1, "ships": {"fighter": {}}})");
  JsonDatabase db;
  db.Load(path);
  json const* val = db.FindPath("ships.fighter.nonexistent");
  LTE_CHECK(val == nullptr);
  CleanupTestFile("test_db5.json");
}

LTE_TEST(JsonDatabase_Keys) {
  String path = WriteTestJson("test_db6.json",
    R"({"version": 1, "alpha": 1, "beta": 2, "gamma": 3})");
  JsonDatabase db;
  db.Load(path);
  Vector<String> keys = db.Keys();
  LTE_CHECK_EQ(keys.size(), 4);
  CleanupTestFile("test_db6.json");
}

LTE_TEST(JsonDatabase_InvalidJson) {
  String path = WriteTestJson("test_db7.json",
    R"({invalid json here)");
  JsonDatabase db;
  LTE_CHECK(!db.Load(path));
  LTE_CHECK(!db.IsLoaded());
  CleanupTestFile("test_db7.json");
}

LTE_TEST(JsonDatabase_MissingFile) {
  JsonDatabase db;
  LTE_CHECK(!db.Load("nonexistent_path.json"));
  LTE_CHECK(!db.IsLoaded());
}

LTE_TEST(JsonDatabase_VersionMissing) {
  String path = WriteTestJson("test_db8.json",
    R"({"name": "no version field"})");
  JsonDatabase db;
  db.Load(path);
  LTE_CHECK_EQ(db.Version(), 0);
  CleanupTestFile("test_db8.json");
}

LTE_TEST(JsonDatabase_ArrayAccess) {
  String path = WriteTestJson("test_db9.json",
    R"({"version": 1, "biomes": ["desert", "terran", "ice"]})");
  JsonDatabase db;
  db.Load(path);
  json const* val = db.Find("biomes");
  LTE_CHECK(val != nullptr);
  LTE_CHECK(val->is_array());
  LTE_CHECK_EQ((int)val->size(), 3);
  CleanupTestFile("test_db9.json");
}

LTE_TEST(DatabaseManager_LoadAndFind) {
  String path = WriteTestJson("test_mgr.json",
    R"({"version": 1, "ships": {"fighter": {"hp": 100}}})");
  DatabaseManager& mgr = DatabaseManager_Get();
  LTE_CHECK(mgr.Load("test_ships_db", path));
  LTE_CHECK(mgr.HasDatabase("test_ships_db"));
  json const* val = mgr.Find("test_ships_db", "ships");
  LTE_CHECK(val != nullptr);
  mgr.Reload("test_ships_db");
  mgr.Erase("test_ships_db");
  CleanupTestFile("test_mgr.json");
}

LTE_TEST(DatabaseManager_FindMissingDb) {
  json const* val = DatabaseManager_Get().Find("no_such_db", "key");
  LTE_CHECK(val == nullptr);
}

LTE_TEST(DatabaseManager_Keys) {
  String path = WriteTestJson("test_mgr2.json",
    R"({"version": 1, "a": 1, "b": 2})");
  DatabaseManager& mgr = DatabaseManager_Get();
  mgr.Load("test_keys_db", path);
  Vector<String> keys = mgr.Keys("test_keys_db");
  LTE_CHECK_EQ(keys.size(), 3);
  mgr.Erase("test_keys_db");
  CleanupTestFile("test_mgr2.json");
}
