// Copyright (C) 2025  darkoned12000
// SPDX-License-Identifier: GPL-3.0-or-later
// Part of the ltheory-old-test modernization effort (Revamp Work).
// See NOTICE and LICENSE.GPL. Original engine (c) Josh Parnell, public domain.

#include "../Items.h"

#include "Game/Objects.h"
#include "Game/Object/Planet.h"
#include "Game/Attribute/Docks.h"
#include "Game/Attribute/Icon.h"
#include "Game/Attribute/Name.h"
#include "Game/Attribute/Renderable.h"
#include "Game/Attribute/Scale.h"
#include "Game/Attribute/Seed.h"
#include "Game/Graphics/Generators.h"
#include "Game/JsonDatabase.h"
#include "Game/DatabaseManager.h"
#include "Game/JsonHelpers.h"

#include "LTE/CubeMap.h"
#include "LTE/DrawState.h"
#include "LTE/Meshes.h"
#include "LTE/Model.h"
#include "LTE/RNG.h"
#include "LTE/Script.h"
#include "LTE/ShaderInstance.h"
#include "LTE/StackFrame.h"
#include "LTE/Texture2D.h"

#include "UI/Glyphs.h"
#include "LTE/FunctionBind.h"

const float kOuterScale = 1.025f;
const uint kMeshQuality = 50;

using PlanetTypeBase =
    Attribute_Docks
  < Attribute_Icon
  < Attribute_Name
  < Attribute_Renderable
  < Attribute_Scale
  < Attribute_Seed
  < ItemWrapper<ItemType_PlanetType>
  > > > > > >;

AutoClassDerived(PlanetType, PlanetTypeBase,
  float, atmoDensity,
  V3, atmoTint,
  float, cloudLevel,
  float, oceanLevel,
  Color, color1,
  Color, color2,
  Color, color3,
  Color, color4,
  V3, wavelength,
  bool, hasRings)

  DERIVED_TYPE_EX(PlanetType)
  PlanetType() = default;

  Object Instantiate(ObjectT* parent) override {
    return Object_Planet(this);
  }
};

DERIVED_IMPLEMENT(PlanetType)

Renderable Generate(PlanetType const& type) {
  static Mesh planetMesh;
  static Mesh atmoMesh;
  static Mesh ringMesh;
  static Shader planetShader = Shader_Create("npm.jsl", "planet.jsl");
  static Shader atmoShader = Shader_Create("npm.jsl", "atmosphere.jsl");
  static Shader ringShader = Shader_Create("npm.jsl", "planetring.jsl");

  if (!planetMesh) {
    planetMesh = Mesh_BoxSphere(kMeshQuality, true)->SetU(1);
    atmoMesh = Mesh_BoxSphere(kMeshQuality, true)
      ->Scale(kOuterScale)
      ->ReverseWinding()
      ->SetU(1);
    ringMesh = Mesh_Quad()
      ->Rotate(V3(0, kPi2, 0))
      ->Scale(3.0f);
  }

  uint seed = type.GetSeed();
  RNG rg = RNG_MTG(seed);
  Model model = Model_Create();

  /* Planet. */
  {
    ShaderInstance planetShaderInstance = ShaderInstance_Create(planetShader);
    float heightMult = 1;

    (*planetShaderInstance)
      ("atmoDensity", type.atmoDensity)
      ("atmoTint", type.atmoTint)
      ("cloudLevel", type.cloudLevel)
      ("color1", type.color1)
      ("color2", type.color2)
      ("color3", type.color3)
      ("color4", type.color4)
      ("colorSeed", rg->GetFloat(1, 1000))
      ("heightMult", heightMult)
      ("oceanLevel", type.oceanLevel)
      ("planetMap",
        DiskCached(Generator_PlanetSurface(seed),
          Stringize() | "planetsurface_" | seed))
      ("wavelength", type.wavelength);
    DrawState_Link(planetShaderInstance);
    model->Add(planetMesh, planetShaderInstance);
  }

  /* Atmosphere. */
  {
    ShaderInstance atmoShaderInstance = ShaderInstance_Create(atmoShader);
    (*atmoShaderInstance)
      (RenderStateSwitch_BlendModeAdditive)
      ("atmoDensity", type.atmoDensity)
      ("atmoTint", type.atmoTint)
      ("wavelength", type.wavelength);
    DrawState_Link(atmoShaderInstance);
    model->Add(atmoMesh, atmoShaderInstance, false);
  }

  /* Rings. */
  {
    if (type.hasRings) {
      static Shader generate =
        Shader_Create("identity.jsl", "gen/planetring.jsl");
      (*generate)("seed", rg->GetFloat());

      Texture2D ringTexture =
        Texture_Create(1024, 1, GL_TextureFormat::R32F);
      Texture_Generate(ringTexture, generate);

      ShaderInstance ringShaderInstance =
        ShaderInstance_Create(ringShader);
      (*ringShaderInstance)
        (RenderStateSwitch_BlendModeAlpha)
        (RenderStateSwitch_CullModeDisabled)
        ("rings", ringTexture);
      DrawState_Link(ringShaderInstance);
      model->Add(ringMesh, ringShaderInstance, false);
    }
  }

  return model;
}

/* Ensure planets.json is loaded once. Returns true if available. */
static bool EnsurePlanetsDb() {
  static bool loaded = false;
  static bool available = false;
  if (loaded)
    return available;
  loaded = true;
  available = DatabaseManager_Get().Load(
    "planets", "resource/gamedata/planets.json");
  if (available) {
    json const* biomes = DatabaseManager_Get().Find("planets", "biomes");
    int biomeCount = biomes ? (int)biomes->size() : 0;
    printf("Loaded planets.json (%d biomes)\n", biomeCount);
  } else {
    printf("WARNING: planets.json not found "
           "— using fallback planet generation\n");
  }
  return available;
}

/* Pick a biome name deterministically from the seed.
 * Uses biomeOrder array if present (stable across schema edits),
 * otherwise falls back to map iteration order. */
static String PickBiome(RNG& rg) {
  json const* biomesObj =
    DatabaseManager_Get().Find("planets", "biomes");
  if (!biomesObj || !biomesObj->is_object())
    return "terran";

  /* Check for explicit biomeOrder array (§3.1 of review). */
  json const* orderVal =
    DatabaseManager_Get().Find("planets", "biomeOrder");
  Vector<String> biomeNames;
  if (orderVal && orderVal->is_array()) {
    for (size_t i = 0; i < orderVal->size(); i++) {
      json const* entry = &(*orderVal)[i];
      if (entry->is_string())
        biomeNames.push(String(entry->get<std::string>().c_str()));
    }
  } else {
    /* Fallback: map iteration order (unstable across edits). */
    for (auto it = biomesObj->begin(); it != biomesObj->end(); ++it)
      biomeNames.push(String(it.key().c_str()));
  }

  if (biomeNames.size() == 0)
    return "terran";

  int idx = (int)(rg->GetInt() % (unsigned)biomeNames.size());
  printf("  -> biome: %s\n", biomeNames[idx].c_str());
  return biomeNames[idx];
}

Item Item_PlanetType(uint const& seed) { AUTO_FRAME;
  RNG rg = RNG_MTG(seed);

  Reference<PlanetType> self = new PlanetType;
  self->docks.push(Bound3(V3(-1), V3(1)));
  self->dockCapacity = -1;  /* -1 = no docks (open interior) */
  ScriptFunction_Load("Icons:Planet")->Call(self->icon);
  self->name = "Planet";
  self->seed = seed;

  /* Try to load biome data from planets.json. */
  bool haveDb = EnsurePlanetsDb();
  String biomeName = haveDb ? PickBiome(rg) : "";

  /* ---- C++ emergency defaults (only used if JSON is missing/corrupt) ----
   * These MUST match the defaults section in planets.json. The JSON
   * "defaults" object is the canonical source; these exist only so the
   * engine can boot without the data file. */
  float atmoDensityMin = 0.0f, atmoDensityMax = 2.0f;
  float cloudLevelMin = -0.2f, cloudLevelMax = 0.15f;
  float oceanLevelMin = 0.0f, oceanLevelMax = 0.0f;
  float desatMin = 0.4f, desatMax = 1.0f;
  float atmoSatMin = 0.5f, atmoSatMax = 1.0f;
  float ringProb = 0.6f;
  float blendStrength = 0.4f;
  float planetScale = 100000.0f;
  int dockCap = -1;
  V3 wavelengthBase(0.66f, 0.53f, 0.4f);
  float wavelengthJitter = 0.1f;
  V3 surfaceTint(0.5f);

  /* Default palette: surfaceTint + 3 random (used if JSON unavailable). */
  V3 palette[4];
  palette[0] = surfaceTint;
  palette[1] = rg->GetV3(0, 1.0f);
  palette[2] = rg->GetV3(0, 1.0f);
  palette[3] = rg->GetV3(0, 1.0f);

  /* ---- Read from JSON ---- */
  if (haveDb) {
    /* Load global defaults first. */
    json const* defaults =
      DatabaseManager_Get().Find("planets", "defaults");
    if (defaults) {
      JRange(defaults, "atmoDensityRange", "planets.json: defaults",
             atmoDensityMin, atmoDensityMax, 0.0f, 2.0f);
      JRange(defaults, "cloudLevelRange", "planets.json: defaults",
             cloudLevelMin, cloudLevelMax, -0.2f, 0.15f);
      JRange(defaults, "oceanLevelRange", "planets.json: defaults",
             oceanLevelMin, oceanLevelMax, 0.0f, 0.0f);
      JRange(defaults, "desaturationRange", "planets.json: defaults",
             desatMin, desatMax, 0.4f, 1.0f);
      JRange(defaults, "atmoTintSaturationRange", "planets.json: defaults",
             atmoSatMin, atmoSatMax, 0.5f, 1.0f);
      JFloat(defaults, "blendStrength", blendStrength, 0.4f);
      JFloat(defaults, "ringProbability", ringProb, 0.6f);
      JFloat(defaults, "scale", planetScale, 100000.0f);
      JInt(defaults, "dockCapacity", dockCap, -1);
      JColor(defaults, "wavelengthBase", "planets.json: defaults",
             wavelengthBase, V3(0.66f, 0.53f, 0.4f));
      JFloat(defaults, "wavelengthJitter", wavelengthJitter, 0.1f);
    }

    /* Load biome-specific values (override defaults). */
    String biomePath = Stringize() | "biomes." | biomeName;
    json const* biome =
      DatabaseManager_Get().FindPath("planets", biomePath);
    if (biome) {
      String bp = Stringize() | "planets.json: biomes." | biomeName;

      JRange(biome, "atmoDensityRange", bp, atmoDensityMin, atmoDensityMax);
      JRange(biome, "cloudLevelRange", bp, cloudLevelMin, cloudLevelMax);
      JRange(biome, "oceanLevelRange", bp, oceanLevelMin, oceanLevelMax);
      JRange(biome, "desaturationRange", bp, desatMin, desatMax);
      JRange(biome, "atmoTintSaturationRange", bp, atmoSatMin, atmoSatMax);
      JFloat(biome, "blendStrength", blendStrength, blendStrength);
      JFloat(biome, "ringProbability", ringProb, ringProb);

      bool hasRings = false;
      if (JBool(biome, "hasRings", hasRings))
        ringProb = hasRings ? 1.0f : 0.0f;

      /* Color palette: 4 colors for terrain gradient. */
      json const* palVal = JGet(biome, "colorPalette");
      if (palVal && palVal->is_array() && palVal->size() >= 4) {
        for (int i = 0; i < 4; i++) {
          String cp = Stringize() | bp | ".colorPalette[" | i | "]";
          JColor(JGet(palVal, i), cp, palette[i], palette[i]);
        }
      }

      /* Surface tint (used as palette[0] fallback). */
      JColor(biome, "surfaceTint", bp, surfaceTint, surfaceTint);
      if (!palVal || !palVal->is_array() || palVal->size() < 4)
        palette[0] = surfaceTint;
    }
  }

  /* ---- Apply values to the planet ---- */
  self->scale = planetScale;
  self->dockCapacity = dockCap;
  self->atmoDensity = rg->GetFloat(atmoDensityMin, atmoDensityMax);
  self->atmoTint = Desaturate(
    rg->GetV3(0, 1.0f),
    rg->GetFloat(atmoSatMin, atmoSatMax));
  self->cloudLevel = rg->GetFloat(cloudLevelMin, cloudLevelMax);
  self->oceanLevel = rg->GetFloat(oceanLevelMin, oceanLevelMax);

  /* Blend each palette color with seed-driven noise. */
  float desat = rg->GetFloat(desatMin, desatMax);
  float blend = blendStrength;
  self->color1 = Desaturate(
    palette[0] * (1.0f - blend) + rg->GetV3(0, 1.0f) * blend, desat);
  self->color2 = Desaturate(
    palette[1] * (1.0f - blend) + rg->GetV3(0, 1.0f) * blend, desat);
  self->color3 = Desaturate(
    palette[2] * (1.0f - blend) + rg->GetV3(0, 1.0f) * blend, desat);
  self->color4 = Desaturate(
    palette[3] * (1.0f - blend) + rg->GetV3(0, 1.0f) * blend, desat);

  self->wavelength = V3(1) / Pow(
    wavelengthBase + rg->GetV3(-wavelengthJitter, wavelengthJitter),
    4.0f);

  self->hasRings = rg->GetFloat() < ringProb;

  printf("Item_PlanetType(seed=%u) biome=%s\n"
    "  color1=(%.3f,%.3f,%.3f) color2=(%.3f,%.3f,%.3f)\n"
    "  color3=(%.3f,%.3f,%.3f) color4=(%.3f,%.3f,%.3f)\n"
    "  desat=%.2f blend=%.2f ocean=%.2f\n",
    seed, biomeName.c_str(),
    self->color1.x, self->color1.y, self->color1.z,
    self->color2.x, self->color2.y, self->color2.z,
    self->color3.x, self->color3.y, self->color3.z,
    self->color4.x, self->color4.y, self->color4.z,
    desat, blendStrength, self->oceanLevel);
  self->renderable = Generate(*self);
  return self;
}

static Function const Item_PlanetType_Registration = Function_Bind(
  "Item_PlanetType",
  "None",
  &Item_PlanetType,
  "seed");
