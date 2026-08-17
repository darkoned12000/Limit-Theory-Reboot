// Copyright (C) 2025  darkoned12000
// SPDX-License-Identifier: GPL-3.0-or-later
// Part of the ltheory-old-test modernization effort (Revamp Work).
// See NOTICE and LICENSE.GPL. Original engine (c) Josh Parnell, public domain.

#include "Game/JsonDatabase.h"
#include "LTE/Location.h"

#include <string>
#include <sstream>

JsonDatabase::JsonDatabase() : loaded(false) {}
JsonDatabase::~JsonDatabase() {}

bool JsonDatabase::Load(String const& filePath) {
  loaded = false;
  error = "";
  data = json();
  path = filePath;

  Location location = Location_File(filePath);
  if (!location || !location->Exists()) {
    error = String("File not found: ") + filePath;
    return false;
  }

  String contents = location->ReadAscii();
  if (contents.size() == 0) {
    error = String("Empty file: ") + filePath;
    return false;
  }

  /* Non-throwing parse — bad JSON yields a discarded value, not a crash. */
  data = json::parse(contents.c_str(), nullptr, false);
  if (data.is_discarded() || !data.is_object()) {
    error = String("Invalid JSON: ") + filePath;
    data = json();
    return false;
  }

  loaded = true;
  return true;
}

json const* JsonDatabase::Find(String const& key) const {
  if (!loaded)
    return nullptr;
  auto it = data.find(key.c_str());
  if (it == data.end())
    return nullptr;
  return &(*it);
}

json const* JsonDatabase::FindPath(String const& dotPath) const {
  if (!loaded)
    return nullptr;

  /* Split on '.' and traverse using std::string for convenience. */
  std::string remaining(dotPath.c_str());
  json const* current = &data;

  while (remaining.size() > 0) {
    size_t dotPos = remaining.find('.');
    std::string segment;
    if (dotPos != std::string::npos) {
      segment = remaining.substr(0, dotPos);
      remaining = remaining.substr(dotPos + 1);
    } else {
      segment = remaining;
      remaining = "";
    }

    if (segment.empty())
      return nullptr;

    auto it = current->find(segment.c_str());
    if (it == current->end())
      return nullptr;
    current = &(*it);
  }

  return current;
}

Vector<String> JsonDatabase::Keys() const {
  Vector<String> keys;
  if (!loaded || !data.is_object())
    return keys;
  for (auto it = data.begin(); it != data.end(); ++it)
    keys.push(String(it.key().c_str()));
  return keys;
}

int JsonDatabase::Size() const {
  if (!loaded || !data.is_object())
    return 0;
  return (int)data.size();
}

int JsonDatabase::Version() const {
  if (!loaded)
    return 0;
  auto it = data.find("version");
  if (it == data.end())
    return 0;
  if (it->is_number_integer())
    return it->get<int>();
  return 0;
}

bool JsonDatabase::IsLoaded() const {
  return loaded;
}

String const& JsonDatabase::GetError() const {
  return error;
}

String const& JsonDatabase::GetPath() const {
  return path;
}

json const& JsonDatabase::Raw() const {
  return data;
}
