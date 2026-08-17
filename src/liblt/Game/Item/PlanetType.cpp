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
  Color, color1,
  Color, color2,
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

  /* Planet. */ {
    ShaderInstance planetShaderInstance = ShaderInstance_Create(planetShader);
    float heightMult = 1;
    float oceanLevel = Pow(rg->GetFloat(), 1.5f);

    (*planetShaderInstance)
      ("atmoDensity", type.atmoDensity)
      ("atmoTint", type.atmoTint)
      ("cloudLevel", type.cloudLevel)
      ("color1", type.color1)
      ("color2", rg->GetV3(0.5f, 0.75f))
      ("color3", rg->GetV3(0.5f, 0.75f))
      ("color4", type.color2)
      ("colorSeed", rg->GetFloat(1, 1000))
      ("heightMult", heightMult)
      ("oceanLevel", oceanLevel)
      ("planetMap",
        DiskCached(Generator_PlanetSurface(seed), Stringize() | "planetsurface_" | seed))
      ("wavelength", type.wavelength);
    DrawState_Link(planetShaderInstance);
    model->Add(planetMesh, planetShaderInstance);
  }

  /* Atmosphere. */ {
    ShaderInstance atmoShaderInstance = ShaderInstance_Create(atmoShader);
    (*atmoShaderInstance)
      (RenderStateSwitch_BlendModeAdditive)
      ("atmoDensity", type.atmoDensity)
      ("atmoTint", type.atmoTint)
      ("wavelength", type.wavelength);
    DrawState_Link(atmoShaderInstance);
    model->Add(atmoMesh, atmoShaderInstance, false);
  }

  /* Rings. */ {
    if (type.hasRings) {
      static Shader generate = Shader_Create("identity.jsl", "gen/planetring.jsl");
      (*generate)("seed", rg->GetFloat());

      Texture2D ringTexture = Texture_Create(1024, 1, GL_TextureFormat::R32F);
      Texture_Generate(ringTexture, generate);

      ShaderInstance ringShaderInstance = ShaderInstance_Create(ringShader);
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
  available = DatabaseManager_Get().Load("planets", "resource/gamedata/planets.json");
  if (available) {
    json const* biomes = DatabaseManager_Get().Find("planets", "biomes");
    int biomeCount = biomes ? (int)biomes->size() : 0;
    printf("Loaded planets.json (%d biomes)\n", biomeCount);
  }
  else
    printf("WARNING: planets.json not found — using fallback planet generation\n");
  return available;
}

/* Pick a biome name deterministically from the seed. */
static String PickBiome(RNG& rg) {
  Vector<String> biomes = DatabaseManager_Get().Keys("planets");
  /* Remove "version" and "defaults" and "moons" keys — only want biome entries.
   * Biomes are in a nested object under "biomes" key, so get those. */
  json const* biomesObj = DatabaseManager_Get().Find("planets", "biomes");
  if (!biomesObj || !biomesObj->is_object())
    return "terran";  /* fallback */

  Vector<String> biomeNames;
  for (auto it = biomesObj->begin(); it != biomesObj->end(); ++it)
    biomeNames.push(String(it.key().c_str()));

  if (biomeNames.size() == 0)
    return "terran";

  int idx = (int)(rg->GetInt() % (unsigned)biomeNames.size());
  printf("  → biome: %s\n", biomeNames[idx].c_str());
  return biomeNames[idx];
}

/* Read a float range from a JSON array [min, max]. */
static void ReadRange(json const* arr, float& minVal, float& maxVal) {
  if (arr && arr->is_array() && arr->size() >= 2) {
    minVal = (*arr)[0].get<float>();
    maxVal = (*arr)[1].get<float>();
  }
}

/* Read a Vec3 from a JSON array [r, g, b]. */
static V3 ReadV3(json const* arr) {
  if (arr && arr->is_array() && arr->size() >= 3)
    return V3((*arr)[0].get<float>(), (*arr)[1].get<float>(), (*arr)[2].get<float>());
  return V3(0.5f);
}

/* Safe JSON field access — returns nullptr if key doesn't exist. */
static json const* JGet(json const* obj, const char* key) {
  if (!obj || !obj->is_object())
    return nullptr;
  auto it = obj->find(key);
  if (it == obj->end())
    return nullptr;
  return &(*it);
}

/* Safe JSON field access on a reference. */
static json const* JGet(json const& obj, const char* key) {
  return JGet(&obj, key);
}

Item Item_PlanetType(uint const& seed) { AUTO_FRAME;
  RNG rg = RNG_MTG(seed);

  Reference<PlanetType> self = new PlanetType;
  self->docks.push(Bound3(V3(-1), V3(1)));
  self->dockCapacity = -1;
  ScriptFunction_Load("Icons:Planet")->Call(self->icon);
  self->name = "Planet";
  self->seed = seed;

  /* Try to load biome data from planets.json. */
  bool haveDb = EnsurePlanetsDb();
  String biomeName = haveDb ? PickBiome(rg) : "";

  /* Defaults (used if JSON unavailable or biome missing). */
  float atmoDensityMin = 0.0f, atmoDensityMax = 2.0f;
  float cloudLevelMin = -0.2f, cloudLevelMax = 0.15f;
  float desatMin = 0.4f, desatMax = 1.0f;
  float atmoSatMin = 0.5f, atmoSatMax = 1.0f;
  float ringProb = 0.6f;
  V3 wavelengthBase(0.66f, 0.53f, 0.4f);
  V3 wavelengthJitter(-0.1f, 0.1f, 0.1f);
  V3 surfaceTint(0.5f);

  /* Override defaults from JSON biome data. */
  if (haveDb) {
    String biomePath = Stringize() | "biomes." | biomeName;
    json const* biome = DatabaseManager_Get().FindPath("planets", biomePath);
    if (biome) {
      /* Biome-specific ranges. */
      json const* atmoRange = JGet(biome, "atmoDensityRange");
      ReadRange(atmoRange, atmoDensityMin, atmoDensityMax);
      json const* cloudRange = JGet(biome, "cloudLevelRange");
      if (!cloudRange) {
        /* Derive from cloudLevel if present. */
        json const* cl = JGet(biome, "cloudLevel");
        if (cl && cl->is_number()) {
          float clv = cl->get<float>();
          cloudLevelMin = clv;
          cloudLevelMax = clv;
        }
      } else {
        ReadRange(cloudRange, cloudLevelMin, cloudLevelMax);
      }

      json const* ringVal = JGet(biome, "hasRings");
      if (ringVal && ringVal->is_boolean())
        ringProb = ringVal->get<bool>() ? 1.0f : 0.0f;

      json const* tintVal = JGet(biome, "surfaceTint");
      surfaceTint = ReadV3(tintVal);

      json const* oceanVal = JGet(biome, "oceanLevel");
      (void)oceanVal;  /* TODO: wire into oceanLevel shader param */
    }

    /* Also read global defaults. */
    json const* defaults = DatabaseManager_Get().Find("planets", "defaults");
    if (defaults) {
      json const* desatRange = JGet(defaults, "desaturationRange");
      ReadRange(desatRange, desatMin, desatMax);
      json const* atmoSatRange = JGet(defaults, "atmoTintSaturationRange");
      ReadRange(atmoSatRange, atmoSatMin, atmoSatMax);
      /* Don't override biome-specific ring setting. */
      json const* wlBase = JGet(defaults, "wavelengthBase");
      wavelengthBase = ReadV3(wlBase);
      json const* wlJitter = JGet(defaults, "wavelengthJitter");
      wavelengthJitter = ReadV3(wlJitter);
    }
  }

  /* Apply values: biome-tinted colors + seed-driven noise for variety. */
  self->scale = 100000;
  self->atmoDensity = rg->GetFloat(atmoDensityMin, atmoDensityMax);
  self->atmoTint = Desaturate(
    rg->GetV3(0, 1.0f),
    rg->GetFloat(atmoSatMin, atmoSatMax));
  self->cloudLevel = rg->GetFloat(cloudLevelMin, cloudLevelMax);

  /* Blend biome surface tint with seed-driven variation. */
  float desat = rg->GetFloat(desatMin, desatMax);
  float blend = rg->GetFloat(0.2f, 0.6f);
  V3 blended = surfaceTint * (1.0f - blend) + rg->GetV3(0, 1.0f) * blend;
  self->color1 = Desaturate(blended, desat);
  self->color2 = Desaturate(rg->GetV3(0, 1.0f), desat);

  self->wavelength = V3(1) / Pow(
    wavelengthBase + rg->GetV3(-wavelengthJitter.x, wavelengthJitter.x),
    4.0f);

  self->hasRings = rg->GetFloat() < ringProb;

  printf("Item_PlanetType(seed=%u) biome=%s\n", seed, biomeName.c_str());
  self->renderable = Generate(*self);
  return self;
}
static Function const Item_PlanetType_Registration = Function_Bind(
  "Item_PlanetType",
  "None",
  &Item_PlanetType,
  "seed");


