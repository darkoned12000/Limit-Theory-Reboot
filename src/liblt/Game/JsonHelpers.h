// Copyright (C) 2025  darkoned12000
// SPDX-License-Identifier: GPL-3.0-or-later
// Part of the ltheory-old-test modernization effort (Revamp Work).
// See NOTICE and LICENSE.GPL. Original engine (c) Josh Parnell, public domain.

#ifndef Game_JsonHelpers_h__
#define Game_JsonHelpers_h__

#include "LTE/V3.h"
#include "json/json.hpp"

using json = nlohmann::json;

/* Shared JSON helpers for gamedata loaders. Used by planets, weapons, ships,
 * stations, graphics, etc. All functions are non-throwing and report errors
 * via the path parameter (for "file: path expected type" messages).
 *
 * Convention: J* prefix = JSON helper, avoids collision with engine names. */

/* ---- Safe field access ---- */

/* Get a named field from a JSON object. Returns nullptr if missing. */
inline json const* JGet(json const* obj, const char* key) {
  if (!obj || !obj->is_object())
    return nullptr;
  auto it = obj->find(key);
  if (it == obj->end())
    return nullptr;
  return &(*it);
}

/* Get a named field from a JSON reference. */
inline json const* JGet(json const& obj, const char* key) {
  return JGet(&obj, key);
}

/* Get an array element by index. Returns nullptr if out of bounds. */
inline json const* JGet(json const* arr, int idx) {
  if (!arr || !arr->is_array() || idx < 0 || idx >= (int)arr->size())
    return nullptr;
  return &(*arr)[idx];
}

/* ---- Color parsing ---- */

/* Parse a color from JSON. Accepts:
 *   "#RRGGBB"       → V3 (0..1)
 *   "#RRGGBBAA"     → V3 (0..1), alpha ignored
 *   [r, g, b]       → V3 (0..1 floats)
 *   [r, g, b, a]    → V3 (0..1 floats), alpha ignored
 *
 * Returns true on success. On failure, sets out to fallback and returns false.
 * path is used for error messages (e.g. "planets.json: biomes.lava.surfaceTint"). */
inline bool JColor(json const* v, String const& path, V3& out,
                   V3 const& fallback = V3(0.5f))
{
  if (!v) {
    out = fallback;
    return false;
  }

  /* Hex string: "#RRGGBB" or "#RRGGBBAA" */
  if (v->is_string()) {
    std::string s = v->get<std::string>();
    if (s.size() >= 7 && s[0] == '#') {
      /* Parse hex digits. */
      auto hex = [&](int start, int len) -> float {
        unsigned int val = 0;
        for (int i = start; i < start + len && i < (int)s.size(); i++) {
          char c = s[i];
          if (c >= '0' && c <= '9') val = val * 16 + (c - '0');
          else if (c >= 'A' && c <= 'F') val = val * 16 + (c - 'A' + 10);
          else if (c >= 'a' && c <= 'f') val = val * 16 + (c - 'a' + 10);
          else { out = fallback; return false; }
        }
        return (float)val / 255.0f;
      };
      out = V3(hex(1, 2), hex(3, 2), hex(5, 2));
      return true;
    }
    out = fallback;
    return false;
  }

  /* Float array: [r, g, b] or [r, g, b, a] */
  if (v->is_array() && v->size() >= 3) {
    if ((*v)[0].is_number() && (*v)[1].is_number() && (*v)[2].is_number()) {
      out = V3((*v)[0].get<float>(), (*v)[1].get<float>(), (*v)[2].get<float>());
      return true;
    }
  }

  out = fallback;
  return false;
}

/* Parse a color from a named field. Convenience wrapper. */
inline bool JColor(json const* obj, const char* key, String const& path,
                   V3& out, V3 const& fallback = V3(0.5f))
{
  return JColor(JGet(obj, key), path, out, fallback);
}

/* ---- Range parsing ---- */

/* Parse a [min, max] range from JSON. Returns true on success.
 * Single number → min=max=value (fixed). Missing → fallback. */
inline bool JRange(json const* v, String const& path,
                   float& minOut, float& maxOut,
                   float fallbackMin = 0.0f, float fallbackMax = 1.0f)
{
  if (!v) {
    minOut = fallbackMin;
    maxOut = fallbackMax;
    return false;
  }

  /* Single number → fixed value */
  if (v->is_number()) {
    float val = v->get<float>();
    minOut = val;
    maxOut = val;
    return true;
  }

  /* Array [min, max] */
  if (v->is_array() && v->size() >= 2 && (*v)[0].is_number() && (*v)[1].is_number()) {
    minOut = (*v)[0].get<float>();
    maxOut = (*v)[1].get<float>();
    return true;
  }

  minOut = fallbackMin;
  maxOut = fallbackMax;
  return false;
}

/* Parse a range from a named field. Convenience wrapper. */
inline bool JRange(json const* obj, const char* key, String const& path,
                   float& minOut, float& maxOut,
                   float fallbackMin = 0.0f, float fallbackMax = 1.0f)
{
  return JRange(JGet(obj, key), path, minOut, maxOut, fallbackMin, fallbackMax);
}

/* ---- Scalar helpers ---- */

/* Read a float from JSON. Returns true on success. */
inline bool JFloat(json const* v, float& out, float fallback = 0.0f) {
  if (v && v->is_number()) {
    out = v->get<float>();
    return true;
  }
  out = fallback;
  return false;
}

inline bool JFloat(json const* obj, const char* key, float& out,
                   float fallback = 0.0f)
{
  return JFloat(JGet(obj, key), out, fallback);
}

/* Read a bool from JSON. Returns true on success. */
inline bool JBool(json const* v, bool& out, bool fallback = false) {
  if (v && v->is_boolean()) {
    out = v->get<bool>();
    return true;
  }
  out = fallback;
  return false;
}

inline bool JBool(json const* obj, const char* key, bool& out,
                  bool fallback = false)
{
  return JBool(JGet(obj, key), out, fallback);
}

/* Read an int from JSON. Returns true on success. */
inline bool JInt(json const* v, int& out, int fallback = 0) {
  if (v && v->is_number_integer()) {
    out = v->get<int>();
    return true;
  }
  out = fallback;
  return false;
}

inline bool JInt(json const* obj, const char* key, int& out,
                 int fallback = 0)
{
  return JInt(JGet(obj, key), out, fallback);
}

/* ---- Vec3 helpers ---- */

/* Read a Vec3 from a JSON array [x, y, z]. */
inline V3 JVec3(json const* v, V3 const& fallback = V3(0.5f)) {
  if (v && v->is_array() && v->size() >= 3
      && (*v)[0].is_number() && (*v)[1].is_number() && (*v)[2].is_number())
    return V3((*v)[0].get<float>(), (*v)[1].get<float>(), (*v)[2].get<float>());
  return fallback;
}

inline V3 JVec3(json const* obj, const char* key,
                V3 const& fallback = V3(0.5f))
{
  return JVec3(JGet(obj, key), fallback);
}

#endif
