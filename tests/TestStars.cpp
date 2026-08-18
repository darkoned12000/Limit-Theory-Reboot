// Copyright (C) 2025  darkoned12000
// SPDX-License-Identifier: GPL-3.0-or-later
// Part of the ltheory-old-test modernization effort (Revamp Work).
// See NOTICE and LICENSE.GPL. Original engine (c) Josh Parnell, public domain.

// Unit tests for stars.json schema and loading logic.
// Tests the actual resource/gamedata/stars.json file and verifies
// the JSON helpers work correctly with star-specific data.

#include "Harness.h"
#include "Game/DatabaseManager.h"
#include "Game/JsonHelpers.h"

#include <cstdio>

// Load the real stars.json once for all tests.
static bool EnsureStarsLoaded() {
  static bool loaded = false;
  static bool ok = false;
  if (loaded) return ok;
  loaded = true;
  ok = DatabaseManager_Get().Load("stars_test", "resource/gamedata/stars.json");
  return ok;
}

// ---- Schema validation ----

LTE_TEST(Stars_LoadsSuccessfully) {
  LTE_CHECK(EnsureStarsLoaded());
}

LTE_TEST(Stars_HasVersion) {
  json const* v = DatabaseManager_Get().Find("stars_test", "version");
  LTE_CHECK(v != nullptr);
  LTE_CHECK(v->is_number());
  LTE_CHECK(v->get<int>() >= 1);
}

LTE_TEST(Stars_HasStarClasses) {
  json const* classes = DatabaseManager_Get().Find("stars_test", "starClasses");
  LTE_CHECK(classes != nullptr);
  LTE_CHECK(classes->is_object());
  LTE_CHECK(classes->size() == 7);
}

LTE_TEST(Stars_HasDefaults) {
  json const* d = DatabaseManager_Get().Find("stars_test", "defaults");
  LTE_CHECK(d != nullptr);
  LTE_CHECK(d->is_object());
}

// ---- Star class structure ----

LTE_TEST(Stars_AllClassesHaveColor) {
  json const* classes = DatabaseManager_Get().Find("stars_test", "starClasses");
  for (auto it = classes->begin(); it != classes->end(); ++it) {
    V3 color;
    bool ok = JColor(JGet(&(*it), "color"), "test", color);
    LTE_CHECK(ok);
    LTE_CHECK(color.x >= 0.0f && color.x <= 1.0f);
    LTE_CHECK(color.y >= 0.0f && color.y <= 1.0f);
    LTE_CHECK(color.z >= 0.0f && color.z <= 1.0f);
  }
}

LTE_TEST(Stars_AllClassesHaveBrightness) {
  json const* classes = DatabaseManager_Get().Find("stars_test", "starClasses");
  for (auto it = classes->begin(); it != classes->end(); ++it) {
    float brightness = -1.0f;
    LTE_CHECK(JFloat(JGet(&(*it), "brightness"), brightness));
    LTE_CHECK(brightness > 0.0f);
  }
}

LTE_TEST(Stars_AllClassesHaveRadius) {
  json const* classes = DatabaseManager_Get().Find("stars_test", "starClasses");
  for (auto it = classes->begin(); it != classes->end(); ++it) {
    float radius = -1.0f;
    LTE_CHECK(JFloat(JGet(&(*it), "radius"), radius));
    LTE_CHECK(radius > 0.0f);
  }
}

// ---- Class ordering (O<B<A<F<G<K<M by brightness) ----

LTE_TEST(Stars_BrightnessOrdering) {
  json const* classes = DatabaseManager_Get().Find("stars_test", "starClasses");
  const char* order[] = { "O", "B", "A", "F", "G", "K", "M" };
  float prev = 1e10f;
  for (int i = 0; i < 7; i++) {
    float brightness = -1.0f;
    LTE_CHECK(JFloat(JGet(JGet(classes, order[i]), "brightness"), brightness));
    LTE_CHECK(brightness < prev);
    prev = brightness;
  }
}

LTE_TEST(Stars_RadiusOrdering) {
  json const* classes = DatabaseManager_Get().Find("stars_test", "starClasses");
  const char* order[] = { "O", "B", "A", "F", "G", "K", "M" };
  float prev = 1e10f;
  for (int i = 0; i < 7; i++) {
    float radius = -1.0f;
    LTE_CHECK(JFloat(JGet(JGet(classes, order[i]), "radius"), radius));
    LTE_CHECK(radius < prev);
    prev = radius;
  }
}

// ---- Defaults fields ----

LTE_TEST(Stars_DefaultshasClassWeights) {
  json const* d = DatabaseManager_Get().Find("stars_test", "defaults");
  json const* w = JGet(d, "classWeights");
  LTE_CHECK(w != nullptr);
  LTE_CHECK(w->is_array());
  LTE_CHECK(w->size() == 7);
}

LTE_TEST(Stars_ClassWeightsArePositive) {
  json const* w = DatabaseManager_Get().FindPath("stars_test", "defaults.classWeights");
  LTE_CHECK(w != nullptr);
  float total = 0.0f;
  for (size_t i = 0; i < w->size(); i++) {
    float val = (*w)[i].get<float>();
    LTE_CHECK(val > 0.0f);
    total += val;
  }
  LTE_CHECK(total > 0.0f);
}

LTE_TEST(Stars_DefaultsHasBrightnessRange) {
  json const* d = DatabaseManager_Get().Find("stars_test", "defaults");
  float mn, mx;
  LTE_CHECK(JRange(JGet(d, "brightnessRange"), "test", mn, mx));
  LTE_CHECK(mn > 0.0f);
  LTE_CHECK(mx >= mn);
}

LTE_TEST(Stars_DefaultsHasRadiusRange) {
  json const* d = DatabaseManager_Get().Find("stars_test", "defaults");
  float mn, mx;
  LTE_CHECK(JRange(JGet(d, "radiusRange"), "test", mn, mx));
  LTE_CHECK(mn > 0.0f);
  LTE_CHECK(mx >= mn);
}

// ---- Hex color parsing ----

LTE_TEST(Stars_HexColorParsing) {
  json hexStr = json::parse(R"("#9bb0ff")");
  V3 color;
  LTE_CHECK(JColor(&hexStr, "test", color));
  LTE_CHECK(color.x > 0.5f);
  LTE_CHECK(color.y > 0.5f);
  LTE_CHECK(color.z > 0.9f);
}

// ---- Range helper edge cases ----

LTE_TEST(Stars_RangeFallbackWhenMissing) {
  float mn = -1.0f, mx = -1.0f;
  bool ok = JRange(nullptr, "test", mn, mx, 0.8f, 1.2f);
  LTE_CHECK(!ok);
  LTE_CHECK(mn == 0.8f);
  LTE_CHECK(mx == 1.2f);
}

LTE_TEST(Stars_RangeAcceptsSingleNumber) {
  json single = json::parse(R"(1.0)");
  float mn, mx;
  LTE_CHECK(JRange(&single, "test", mn, mx));
  LTE_CHECK(mn == 1.0f);
  LTE_CHECK(mx == 1.0f);
}

// Cleanup: remove test database entry
static struct StarsTestCleanup {
  ~StarsTestCleanup() {
    DatabaseManager_Get().Erase("stars_test");
  }
} starsCleanup;
