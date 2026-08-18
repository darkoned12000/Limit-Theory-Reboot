// Copyright (C) 2025  darkoned12000
// SPDX-License-Identifier: GPL-3.0-or-later
// Part of the ltheory-old-test modernization effort (Revamp Work).
// See NOTICE and LICENSE.GPL. Original engine (c) Josh Parnell, public domain.

// Unit tests for planets.json schema and loading logic.
// Tests the actual resource/gamedata/planets.json file and verifies
// the JSON helpers work correctly with planet-specific data.

#include "Harness.h"
#include "Game/DatabaseManager.h"
#include "Game/JsonHelpers.h"

#include <cstdio>

// Load the real planets.json once for all tests.
static bool EnsurePlanetsLoaded() {
  static bool loaded = false;
  static bool ok = false;
  if (loaded) return ok;
  loaded = true;
  ok = DatabaseManager_Get().Load("planets_test", "resource/gamedata/planets.json");
  return ok;
}

// ---- Schema validation ----

LTE_TEST(Planets_LoadsSuccessfully) {
  LTE_CHECK(EnsurePlanetsLoaded());
}

LTE_TEST(Planets_HasVersion) {
  json const* v = DatabaseManager_Get().Find("planets_test", "version");
  LTE_CHECK(v != nullptr);
  LTE_CHECK(v->is_number());
  LTE_CHECK(v->get<int>() >= 1);
}

LTE_TEST(Planets_HasBiomeOrder) {
  json const* order = DatabaseManager_Get().Find("planets_test", "biomeOrder");
  LTE_CHECK(order != nullptr);
  LTE_CHECK(order->is_array());
  LTE_CHECK(order->size() >= 2);
}

LTE_TEST(Planets_BiomeOrderMatchesBiomes) {
  json const* order = DatabaseManager_Get().Find("planets_test", "biomeOrder");
  json const* biomes = DatabaseManager_Get().Find("planets_test", "biomes");
  LTE_CHECK(order != nullptr && biomes != nullptr);
  for (size_t i = 0; i < order->size(); i++) {
    std::string name = (*order)[i].get<std::string>();
    LTE_CHECK(biomes->contains(name));
  }
}

LTE_TEST(Planets_HasDefaults) {
  json const* d = DatabaseManager_Get().Find("planets_test", "defaults");
  LTE_CHECK(d != nullptr);
  LTE_CHECK(d->is_object());
}

LTE_TEST(Planets_AllBiomesHaveRequiredFields) {
  json const* biomes = DatabaseManager_Get().Find("planets_test", "biomes");
  LTE_CHECK(biomes != nullptr);
  for (auto it = biomes->begin(); it != biomes->end(); ++it) {
    json const* biome = &(*it);
    // Must have colorPalette with 4 entries
    json const* pal = JGet(biome, "colorPalette");
    LTE_CHECK(pal != nullptr);
    LTE_CHECK(pal->is_array());
    LTE_CHECK(pal->size() >= 4);
    // Must have atmoDensityRange as array
    json const* atmo = JGet(biome, "atmoDensityRange");
    LTE_CHECK(atmo != nullptr);
    LTE_CHECK(atmo->is_array());
    LTE_CHECK(atmo->size() >= 2);
    // Must have a name
    json const* name = JGet(biome, "name");
    LTE_CHECK(name != nullptr);
    LTE_CHECK(name->is_string());
  }
}

// ---- Defaults fields ----

LTE_TEST(Planets_DefaultsHasScaleRange) {
  json const* d = DatabaseManager_Get().Find("planets_test", "defaults");
  float mn, mx;
  LTE_CHECK(JRange(JGet(d, "scaleRange"), "test", mn, mx));
  LTE_CHECK(mn > 0.0f);
  LTE_CHECK(mx >= mn);
}

LTE_TEST(Planets_DefaultsHasDesaturationRange) {
  json const* d = DatabaseManager_Get().Find("planets_test", "defaults");
  float mn, mx;
  LTE_CHECK(JRange(JGet(d, "desaturationRange"), "test", mn, mx));
  LTE_CHECK(mn >= 0.0f);
  LTE_CHECK(mx <= 1.0f);
  LTE_CHECK(mx >= mn);
}

LTE_TEST(Planets_DefaultsHasBlendStrength) {
  json const* d = DatabaseManager_Get().Find("planets_test", "defaults");
  float val = -1.0f;
  LTE_CHECK(JFloat(JGet(d, "blendStrength"), val));
  LTE_CHECK(val >= 0.0f);
  LTE_CHECK(val <= 1.0f);
}

LTE_TEST(Planets_DefaultsHasRingProbability) {
  json const* d = DatabaseManager_Get().Find("planets_test", "defaults");
  float val = -1.0f;
  LTE_CHECK(JFloat(JGet(d, "ringProbability"), val));
  LTE_CHECK(val >= 0.0f);
  LTE_CHECK(val <= 1.0f);
}

LTE_TEST(Planets_DefaultsHasWavelengthBase) {
  json const* d = DatabaseManager_Get().Find("planets_test", "defaults");
  V3 out;
  LTE_CHECK(JColor(JGet(d, "wavelengthBase"), "test", out));
  LTE_CHECK(out.x > 0.0f);
  LTE_CHECK(out.y > 0.0f);
  LTE_CHECK(out.z > 0.0f);
}

LTE_TEST(Planets_DefaultsHasDockCapacity) {
  json const* d = DatabaseManager_Get().Find("planets_test", "defaults");
  int val = 999;
  LTE_CHECK(JInt(JGet(d, "dockCapacity"), val));
  LTE_CHECK(val == -1);
}

// ---- Hex color parsing in palette context ----

LTE_TEST(Planets_HexPaletteColorsParse) {
  json const* biomes = DatabaseManager_Get().Find("planets_test", "biomes");
  for (auto it = biomes->begin(); it != biomes->end(); ++it) {
    json const* pal = JGet(&(*it), "colorPalette");
    if (!pal || !pal->is_array()) continue;
    for (size_t i = 0; i < pal->size() && i < 4; i++) {
      V3 color;
      bool ok = JColor(JGet(pal, (int)i), "test", color);
      LTE_CHECK(ok);
      LTE_CHECK(color.x >= 0.0f && color.x <= 1.0f);
      LTE_CHECK(color.y >= 0.0f && color.y <= 1.0f);
      LTE_CHECK(color.z >= 0.0f && color.z <= 1.0f);
    }
  }
}

// ---- Biome-specific fields ----

LTE_TEST(Planets_GasGiantHasScaleRange) {
  json const* biome =
    DatabaseManager_Get().FindPath("planets_test", "biomes.gas_giant");
  LTE_CHECK(biome != nullptr);
  float mn, mx;
  LTE_CHECK(JRange(JGet(biome, "scaleRange"), "gas_giant", mn, mx));
  LTE_CHECK(mn >= 100000.0f);
  LTE_CHECK(mx >= mn);
}

LTE_TEST(Planets_GasGiantHasBlendStrength) {
  json const* biome =
    DatabaseManager_Get().FindPath("planets_test", "biomes.gas_giant");
  LTE_CHECK(biome != nullptr);
  float val = -1.0f;
  LTE_CHECK(JFloat(JGet(biome, "blendStrength"), val));
  LTE_CHECK(val >= 0.0f);
  LTE_CHECK(val <= 1.0f);
}

LTE_TEST(Planets_AllBiomesHaveAtmoDensityRange) {
  json const* biomes = DatabaseManager_Get().Find("planets_test", "biomes");
  for (auto it = biomes->begin(); it != biomes->end(); ++it) {
    float mn, mx;
    bool ok = JRange(JGet(&(*it), "atmoDensityRange"), it.key().c_str(), mn, mx);
    LTE_CHECK(ok);
    LTE_CHECK(mn >= 0.0f);
    LTE_CHECK(mx >= mn);
  }
}

// ---- Surface tint parsing ----

LTE_TEST(Planets_SurfaceTintsAreValidColors) {
  json const* biomes = DatabaseManager_Get().FindPath("planets_test", "biomes");
  LTE_CHECK(biomes != nullptr);
  for (auto it = biomes->begin(); it != biomes->end(); ++it) {
    V3 tint;
    bool ok = JColor(JGet(&(*it), "surfaceTint"), "test", tint);
    LTE_CHECK(ok);
    LTE_CHECK(tint.x >= 0.0f && tint.x <= 1.0f);
    LTE_CHECK(tint.y >= 0.0f && tint.y <= 1.0f);
    LTE_CHECK(tint.z >= 0.0f && tint.z <= 1.0f);
  }
}

// ---- Range helper edge cases ----

LTE_TEST(Planets_RangeFallbackWhenMissing) {
  float mn = -1.0f, mx = -1.0f;
  bool ok = JRange(nullptr, "test", mn, mx, 0.5f, 0.8f);
  LTE_CHECK(!ok);
  LTE_CHECK(mn == 0.5f);
  LTE_CHECK(mx == 0.8f);
}

LTE_TEST(Planets_RangeAcceptsSingleNumber) {
  json single = json::parse(R"(0.7)");
  float mn, mx;
  LTE_CHECK(JRange(&single, "test", mn, mx));
  LTE_CHECK(mn == 0.7f);
  LTE_CHECK(mx == 0.7f);
}

LTE_TEST(Planets_HexAndArrayPaletteMix) {
  json palette = json::parse(R"(["#ff0000", [0.0, 1.0, 0.0], "#0000ff", [1.0, 1.0, 0.0]])");
  V3 c0, c1, c2, c3;
  LTE_CHECK(JColor(JGet(&palette, 0), "test", c0));
  LTE_CHECK(JColor(JGet(&palette, 1), "test", c1));
  LTE_CHECK(JColor(JGet(&palette, 2), "test", c2));
  LTE_CHECK(JColor(JGet(&palette, 3), "test", c3));
  LTE_CHECK(c0.x > 0.99f);
  LTE_CHECK(c0.y < 0.01f);
  LTE_CHECK(c0.z < 0.01f);
  LTE_CHECK(c1.x < 0.01f);
  LTE_CHECK(c1.y > 0.99f);
  LTE_CHECK(c1.z < 0.01f);
  LTE_CHECK(c2.x < 0.01f);
  LTE_CHECK(c2.y < 0.01f);
  LTE_CHECK(c2.z > 0.99f);
}

LTE_TEST(Planets_MoonsSectionExists) {
  json const* moons = DatabaseManager_Get().Find("planets_test", "moons");
  LTE_CHECK(moons != nullptr);
  LTE_CHECK(moons->is_object());
}

// Cleanup: remove test database entry
static struct PlanetsTestCleanup {
  ~PlanetsTestCleanup() {
    DatabaseManager_Get().Erase("planets_test");
  }
} planestCleanup;
