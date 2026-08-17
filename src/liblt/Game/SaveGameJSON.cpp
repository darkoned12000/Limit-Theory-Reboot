// Copyright (C) 2025  darkoned12000
// SPDX-License-Identifier: GPL-3.0-or-later
// Part of the ltheory-old-test modernization effort (Revamp Work).
// See NOTICE and LICENSE.GPL. Original engine (c) Josh Parnell, public domain.

#include "Game/SaveGameJSON.h"

#include "LTE/Array.h"
#include "LTE/Location.h"
#include "LTE/OS.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <fstream>
#include <sstream>

/* The engine builds with -fno-exceptions, so nlohmann/json's default
 * `throw`-based error path won't compile. We route errors to the engine's
 * crash-aware LTE_ASSERT_FAILURE (which logs + raises SIGABRT, landing in the
 * CrashHandler) instead of a bare std::abort(): a JSON failure inside engine
 * code is a genuine bug and deserves a crash log, while ordinary bad-save
 * handling uses the non-throwing API (parse(..., false) + contains()/value())
 * and never reaches these macros. */
#define JSON_THROW_USER(exception)                                               \
  (LTE_ASSERT_FAILURE(__FILE__, __LINE__, #exception), std::abort())
#define JSON_TRY_USER if (true)
#define JSON_CATCH_USER(exception) if (false)
#define JSON_INTERNAL_CATCH_USER(exception) if (false)

#include "json/json.hpp"

using json = nlohmann::json;

namespace LTE {
  /* Test-only override: redirects the whole layer to another directory. Only
     a string store here (no filesystem work), so the unit tests can set it
     from a static initializer without engine-init ordering concerns. */
  static String savesDirOverride;

  static String const& GetSavesDir() {
    static String defaultDir = OS_GetUserDataPath() + "saves/";
    static String createdDir;
    String const& dir = savesDirOverride.length() ? savesDirOverride : defaultDir;
    if (dir != createdDir) {
      createdDir = dir;
      OS_CreateDir(dir);
    }
    return dir;
  }

  void SaveGame_SetSavesDir(String const& dir) {
    savesDirOverride = dir;
  }

  /* Strip any path separators so a slot name can't escape the saves dir. */
  static String SanitizeSlotName(String const& slotName) {
    String clean;
    for (char c : slotName) {
      if (c == '/' || c == '\\' || c == ':' || c == '.')
        continue;
      clean += c;
    }
    return clean.length() ? clean : SaveGame_DefaultSlotName();
  }

  static String SlotPath(String const& slotName) {
    return GetSavesDir() + SanitizeSlotName(slotName) + ".json";
  }

  String SaveGame_GetSavesDir() {
    return GetSavesDir();
  }

  String SaveGame_DefaultSlotName() {
    return "quicksave";
  }

  static void WritePadded(std::stringstream& stream, int value, int width) {
    stream.fill('0');
    stream.width(width);
    stream << value;
    stream.fill(' ');
  }

  static String FormatTimestamp() {
    std::time_t t = std::time(nullptr);
    std::tm tm;
    std::memset(&tm, 0, sizeof(tm));
#if defined(_WIN32)
    localtime_s(&tm, &t);
#else
    localtime_r(&t, &tm);
#endif
    std::stringstream stream;
    WritePadded(stream, tm.tm_year + 1900, 4);
    stream << "-";
    WritePadded(stream, tm.tm_mon + 1, 2);
    stream << "-";
    WritePadded(stream, tm.tm_mday, 2);
    stream << " ";
    WritePadded(stream, tm.tm_hour, 2);
    stream << ":";
    WritePadded(stream, tm.tm_min, 2);
    stream << ":";
    WritePadded(stream, tm.tm_sec, 2);
    return stream.str();
  }

  static json VectorToJSON(V3D const& v) {
    json j;
    j["x"] = v.x;
    j["y"] = v.y;
    j["z"] = v.z;
    return j;
  }

  static V3D VectorFromJSON(json const& j) {
    return V3D(
      j.value("x", 0.0),
      j.value("y", 0.0),
      j.value("z", 0.0));
  }

  String SaveGameData_ToJSON(SaveGameData const& data) {
    json j;
    j["saveVersion"]     = kSaveJSONVersion;
    j["dateCreated"]     = data.dateCreated;
    j["saveName"]        = data.saveName.c_str();
    j["saveDescription"] = data.saveDescription.c_str();
    j["playerName"]      = data.playerName.c_str();
    j["systemName"]      = data.systemName.c_str();
    j["playerCredits"]   = data.playerCredits;
    j["shipHull"]        = data.shipHull;
    j["playerPos"]       = VectorToJSON(data.playerPos);
    j["playerLook"]      = VectorToJSON(data.playerLook);
    j["universeSeed"]    = data.universeSeed;
    return String(j.dump(2).c_str());
  }

  SaveGameData SaveGameData_FromJSON(String const& contents) {
    json j = json::parse(contents.c_str(), nullptr, false);
    if (j.is_discarded() || !j.is_object())
      return SaveGameData();

    SaveGameData d;
    d.version        = j.value("saveVersion", 0);
    d.dateCreated    = String(j.value("dateCreated", "").c_str());
    d.saveName       = String(j.value("saveName", "").c_str());
    d.saveDescription = String(j.value("saveDescription", "").c_str());
    d.playerName     = String(j.value("playerName", "").c_str());
    d.systemName     = String(j.value("systemName", "").c_str());
    d.playerCredits  = j.value("playerCredits", (long long)0);
    d.shipHull       = j.value("shipHull", (long long)0);
    if (j.contains("playerPos") && j["playerPos"].is_object())
      d.playerPos = VectorFromJSON(j["playerPos"]);
    if (j.contains("playerLook") && j["playerLook"].is_object())
      d.playerLook = VectorFromJSON(j["playerLook"]);
    d.universeSeed   = j.value("universeSeed", (unsigned int)0);
    return d;
  }

  bool SaveGame_Write(String const& slotName, SaveGameData const& data) {
    String clean = SanitizeSlotName(slotName);
    /* Enforce the slot cap when creating a NEW slot. Overwriting an existing
       slot is always allowed. Quicksaves count against the cap too. */
    if (!SaveGame_Exists(clean) &&
        SaveGame_Count() >= kMaxSaveSlots)
      return false;

    json j = json::parse(SaveGameData_ToJSON(data).c_str());
    if (j.value("dateCreated", "").empty())
      j["dateCreated"] = FormatTimestamp().c_str();
    String contents = String(j.dump(2).c_str());

    Array<uchar> bytes((size_t)contents.length(), (uchar const*)contents.c_str());
    return Location_File(SlotPath(clean))->Write(bytes);
  }

  /* Compact timestamp "YYYYMMDD-HHMMSS" used to give each quicksave its own
     slot name. */
  static String FormatTimestampCompact() {
    std::time_t t = std::time(nullptr);
    std::tm tm;
    std::memset(&tm, 0, sizeof(tm));
#if defined(_WIN32)
    localtime_s(&tm, &t);
#else
    localtime_r(&t, &tm);
#endif
    std::stringstream stream;
    WritePadded(stream, tm.tm_year + 1900, 4);
    WritePadded(stream, tm.tm_mon + 1, 2);
    WritePadded(stream, tm.tm_mday, 2);
    stream << "-";
    WritePadded(stream, tm.tm_hour, 2);
    WritePadded(stream, tm.tm_min, 2);
    WritePadded(stream, tm.tm_sec, 2);
    return stream.str();
  }

  String SaveGame_QuicksaveSlotName() {
    String base = "quick-" + FormatTimestampCompact();
    String candidate = base;
    int n = 1;
    while (SaveGame_Exists(candidate)) {
      candidate = base + "-" + String(std::to_string(n).c_str());
      ++n;
    }
    return candidate;
  }

  bool SaveGame_WriteQuicksave(SaveGameData const& data) {
    /* Each quicksave accumulates into its own timestamped slot instead of
       overwriting the previous one. The slot cap applies, so a new quicksave
       is refused once 25 slots exist. */
    return SaveGame_Write(SaveGame_QuicksaveSlotName(), data);
  }

  SaveGameData SaveGame_Read(String const& slotName) {
    Location loc = Location_File(SlotPath(slotName));
    if (!loc->Exists())
      return SaveGameData();
    return SaveGameData_FromJSON(loc->ReadAscii());
  }

  Vector<String> SaveGame_ListSlotNames() {
    Vector<String> names;
    Vector<String> entries = OS_ListDir(GetSavesDir());
    for (String const& entry : entries) {
      if (entry.length() >= 5 && entry.substr(entry.length() - 5) == ".json")
        names.push(entry.substr(0, entry.length() - 5));
    }
    return names;
  }

  Vector<SaveSlotInfo> SaveGame_ListSlots() {
    Vector<SaveSlotInfo> slots;
    for (String const& name : SaveGame_ListSlotNames()) {
      SaveGameData d = SaveGame_Read(name);
      if (d.version != kSaveJSONVersion)
        continue;
      SaveSlotInfo info;
      info.slotName = name;
      info.dateCreated = d.dateCreated;
      info.saveName = d.saveName;
      info.saveDescription = d.saveDescription;
      info.playerName = d.playerName;
      info.systemName = d.systemName;
      info.playerCredits = d.playerCredits;
      info.playerPos = d.playerPos;
      info.universeSeed = d.universeSeed;
      slots.push(info);
    }
    return slots;
  }

  bool SaveGame_Delete(String const& slotName) {
    String clean = SanitizeSlotName(slotName);
    String path = SlotPath(clean);
    if (!OS_FileExists(path))
      return false;
    return std::remove(path.c_str()) == 0;
  }

  bool SaveGame_Exists(String const& slotName) {
    return SaveGame_Read(slotName).version == kSaveJSONVersion;
  }

  int SaveGame_Count() {
    /* Counts every slot file present -- quicksaves and named saves together. */
    return (int)SaveGame_ListSlotNames().size();
  }

  SaveGameData SaveGame_ReadLatest() {
    /* Newest slot by dateCreated. Quicksaves are ordinary slots now, so there
       is no dedicated quicksave slot to prefer. */
    SaveGameData latest;
    String latestDate;
    for (String const& name : SaveGame_ListSlotNames()) {
      SaveGameData d = SaveGame_Read(name);
      if (d.version != kSaveJSONVersion)
        continue;
      if (latestDate.empty() || d.dateCreated > latestDate) {
        latestDate = d.dateCreated;
        latest = d;
      }
    }
    return latest;
  }
}
