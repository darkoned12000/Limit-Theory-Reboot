#include "WeaponType.h"

#include "Game/Beam.h"
#include "Game/Constants.h"
#include "Game/Icons.h"
#include "Game/NLP.h"
#include "Game/Objects.h"
#include "Game/DatabaseManager.h"
#include "Game/JsonHelpers.h"

#include "Component/Motion.h"
#include "Component/Pilotable.h"

#include "Module/SoundEngine.h"

#include "LTE/Grammar.h"
#include "LTE/Math.h"
#include "LTE/Meshes.h"
#include "LTE/RNG.h"
#include "LTE/Script.h"
#include "LTE/V4.h"
#include "LTE/VectorMap.h"

#include "UI/Glyphs.h"
#include "LTE/FunctionBind.h"

DERIVED_IMPLEMENT(WeaponType)

/* Ensure ships.json is loaded once (shared with ShipType.cpp via singleton). */
static bool EnsureShipsDb() {
  static bool loaded = false;
  static bool available = false;
  if (loaded)
    return available;
  loaded = true;
  available = DatabaseManager_Get().Load(
    "ships", "resource/gamedata/ships.json");
  return available;
}

const Icon kWeaponIcon[WeaponClass_SIZE] = {
  Icon_Crosshair(),
  Icon_Crosshair(),
  Icon_Crosshair(),
  Icon_Crosshair(),
};

/* ---- Weapon class multiplier tables ----
 * Indexed by WeaponClass: Beam=0, Missile=1, Pulse=2, Rail=3.
 * Loaded from ships.json weaponClasses on first call; falls back to
 * hardcoded defaults if JSON is unavailable. */
static float kWeaponMagazineSizeMult[WeaponClass_SIZE];
static float kWeaponMagazineProbability[WeaponClass_SIZE];
static float kWeaponPowerDrainMult[WeaponClass_SIZE];
static float kWeaponRateMult[WeaponClass_SIZE];
static float kWeaponSpreadMult[WeaponClass_SIZE];
static float kWeaponWeightMult[WeaponClass_SIZE];
static float kAmmoDamageMult[WeaponClass_SIZE];
static float kAmmoLifeMult[WeaponClass_SIZE];
static float kAmmoProbabilityMult[WeaponClass_SIZE];
static float kAmmoSpeedMult[WeaponClass_SIZE];

/* Mapping from WeaponClass enum index to JSON key name. */
static const char* kWeaponClassKeys[WeaponClass_SIZE] = {
  "beam", "missile", "pulse", "rail"
};

/* Optional weapon class override (set via LTSL WeaponType_SetOverride).
 * When non-empty, forces all new weapons to this class regardless of
 * probability tables. "none" or empty clears the override. */
static String sWeaponOverride;

void WeaponType_SetOverride(String const& className) {
  if (className == "none" || className.size() == 0)
    sWeaponOverride = String();
  else
    sWeaponOverride = className;
}

/* Load multiplier tables from ships.json weaponClasses section.
 * Each class's multipliers override the hardcoded defaults. */
static void LoadWeaponClassTables() {
  /* Hardcoded defaults (must match ships.json). */
  static const float kDefMagSize[4] =        { 0, 1, 6, 10 };
  static const float kDefMagProb[4] =        { 0, 1, 0.1f, 0.9f };
  static const float kDefPowerDrain[4] =     { 5, 0, 2, 1 };
  static const float kDefRate[4] =           { 1, 0.01f, 1, 1 };
  static const float kDefSpread[4] =         { 0, 1, 2, 5 };
  static const float kDefWeight[4] =         { 5, 3, 2, 1 };
  static const float kDefAmmoDamage[4] =     { 5, 20, 2, 1 };
  static const float kDefAmmoLife[4] =       { 2.5f, 10, 1.25f, 1 };
  static const float kDefAmmoProb[4] =       { 0.0f, 0.0f, 1.0f, 0.0f };
  static const float kDefAmmoSpeed[4] =      { 1e10f, 1, 1, 1e10f };

  /* Copy defaults into the mutable arrays. */
  for (int i = 0; i < WeaponClass_SIZE; ++i) {
    kWeaponMagazineSizeMult[i] = kDefMagSize[i];
    kWeaponMagazineProbability[i] = kDefMagProb[i];
    kWeaponPowerDrainMult[i] = kDefPowerDrain[i];
    kWeaponRateMult[i] = kDefRate[i];
    kWeaponSpreadMult[i] = kDefSpread[i];
    kWeaponWeightMult[i] = kDefWeight[i];
    kAmmoDamageMult[i] = kDefAmmoDamage[i];
    kAmmoLifeMult[i] = kDefAmmoLife[i];
    kAmmoProbabilityMult[i] = kDefAmmoProb[i];
    kAmmoSpeedMult[i] = kDefAmmoSpeed[i];
  }

  bool haveDb = EnsureShipsDb();
  if (!haveDb)
    return;

  json const* weaponClasses =
    DatabaseManager_Get().Find("ships", "weaponClasses");
  if (!weaponClasses || !weaponClasses->is_object())
    return;

  for (int i = 0; i < WeaponClass_SIZE; ++i) {
    json const* cls = JGet(weaponClasses, kWeaponClassKeys[i]);
    if (!cls)
      continue;
    String wp = Stringize() | "ships.json: weaponClasses." | kWeaponClassKeys[i];

    JFloat(cls, "magazineSizeMult", kWeaponMagazineSizeMult[i], kDefMagSize[i]);
    JFloat(cls, "magazineProbability", kWeaponMagazineProbability[i], kDefMagProb[i]);
    JFloat(cls, "powerDrainMult", kWeaponPowerDrainMult[i], kDefPowerDrain[i]);
    JFloat(cls, "rateMult", kWeaponRateMult[i], kDefRate[i]);
    JFloat(cls, "spreadMult", kWeaponSpreadMult[i], kDefSpread[i]);
    JFloat(cls, "weightMult", kWeaponWeightMult[i], kDefWeight[i]);
    JFloat(cls, "ammoDamageMult", kAmmoDamageMult[i], kDefAmmoDamage[i]);
    JFloat(cls, "ammoLifeMult", kAmmoLifeMult[i], kDefAmmoLife[i]);
    JFloat(cls, "ammoProbabilityMult", kAmmoProbabilityMult[i], kDefAmmoProb[i]);
    JFloat(cls, "ammoSpeedMult", kAmmoSpeedMult[i], kDefAmmoSpeed[i]);
  }
  printf("Loaded weapon class tables from ships.json\n");
}

Object WeaponType::Fire(
  ObjectT* w,
  Position const& origin,
  V3 const& heading,
  Object const& target)
{
  V3 myVelocity = w->GetRoot()->GetVelocity();
  Object object;
  WeaponType const* wtype = (WeaponType*)(ItemT*)w->GetSupertype();

  if (type == WeaponClass_Pulse) {
    object = Object_Pulse(
      (heading + wtype->spread * SampleSphere()) * speed,
      myVelocity,
      w->GetScale().GetMax());

    Sound_Play3D(GetSound(), w, wtype->offset, 0.1f)
      ->SetPitch(Rand(0.75f, 1.25f));
  }

  else if (type == WeaponClass_Missile) {
    V3 offset = target ? target->GetDrawable()->renderable()->Sample() : 0;
    object = Object_Missile(heading * speed, myVelocity, target, offset);
    object->SetPos(origin);
    Sound_Play3D("weapon/missile1_fire.ogg", w, wtype->offset, 0.25f)
      ->SetPitch(Rand(0.75f, 1.25f));
  }

  else if (type == WeaponClass_Beam) {
    /* Beam is created/updated by Weapon::Fire() which owns the beam field. */
  }

  else if (type == WeaponClass_Rail) {
    object = Object_Rail(origin, Normalize(heading + wtype->spread * SampleSphere()), myVelocity);
    Sound_Play3D("weapon/rail1_fire.ogg", w, wtype->offset, 0.25f)
      ->SetPitch(Rand(0.7f, 1.3f));
  }

  if (object) {
    object->GetDamager()->type = (WeaponType*)w->GetSupertype();
    object->GetDamager()->source = w;
    /* Rail sets its position in Object_Rail() — it has no Orientation
     * component, so SetPos is not implemented. */
    if (type != WeaponClass_Rail)
      object->SetPos(origin);
  }

  return object;
}

float WeaponType::GetDPS() const {
  float damage = static_cast<float>(this->damage);
  if (uses)
    damage /= 1.0f / rate + magazineTime / static_cast<float>(uses);

  if (type != WeaponClass_Beam)
    damage *= rate;
  return damage;
}

Object WeaponType::Instantiate(ObjectT* parent) {
  Object turret = Item_TurretType(1, kPi)->Instantiate();
  turret->Plug(Object_Weapon(this));
  return turret;
}

Item Item_WeaponType(int const& id) {
  static Renderable renderable;
  if (!renderable)
    ScriptFunction_Load("Item/WeaponType:Generate")->Call(renderable);

  /* Ensure multiplier tables are loaded from JSON (once). */
  static bool tablesLoaded = false;
  if (!tablesLoaded) {
    LoadWeaponClassTables();
    tablesLoaded = true;
  }

  RNG rng = RNG_MTG(id);
  Reference<WeaponType> self = new WeaponType;

  /* Check for weapon class override from gameConfig. */
  self->type = WeaponClass_Pulse;
  if (sWeaponOverride.size() > 0) {
    for (int i = 0; i < WeaponClass_SIZE; ++i) {
      if (sWeaponOverride == kWeaponClassKeys[i]) {
        self->type = i;
        break;
      }
    }
  } else {
    float typeValue = rng->GetFloat();
    float thisValue = 0;
    for (int i = 0; i < WeaponClass_SIZE; ++i) {
      thisValue += kAmmoProbabilityMult[i];
      if (typeValue <= thisValue) {
        self->type = i;
        break;
      }
    }
  }

  self->color = 0.25f * Color_White + ToRGB(Color(
    rng->GetFloat(),
    rng->GetFloat(0.6f, 0.99f),
    rng->GetFloat(0.2f, 0.6f)));

  self->damage = (Damage)
    (Constant_AmmoDamageMult() * kAmmoDamageMult[self->type] *
     Sigfigs(1 + 15 * rng->GetExp(), 2));

  self->properties = rng->GetV3(0, 1);

  float life = Constant_AmmoLifeMult() * kAmmoLifeMult[self->type] *
    (1.0f + rng->GetExp());

  self->speed = Constant_AmmoSpeedMult() * kAmmoSpeedMult[self->type] *
    (1.0 + 0.5f * rng->GetExp());

  self->range = life * self->speed;

  if (self->type == WeaponClass_Beam) {
  }

  else if (self->type == WeaponClass_Missile) {
  }

  else if (self->type == WeaponClass_Pulse) {
    const String table[] = {
      "weapon/pulse/1/Pulse1.1.ogg",
      "weapon/pulse/2/Pulse2.1short.ogg",
      "weapon/pulse/3/Pulse3.2.ogg",
      "weapon/pulse/4/Pulse4.1short.ogg",
      "weapon/pulse/5/Pulse5.1.ogg",
      "weapon/pulse/5/Pulse5.1.ogg"
    };

    self->sound = table[rng->GetInt(0, sizeof(table) / sizeof(*table) - 1)];
  }

  else if (self->type == WeaponClass_Rail) {
  }

  self->uses = rng->GetFloat() < kWeaponMagazineProbability[self->type]
    ? 4 * static_cast<int>(kWeaponMagazineSizeMult[self->type] * rng->GetFloat(1, 2)) : 0;

  self->icon = kWeaponIcon[self->type];

  self->mass = self->damage * kWeaponWeightMult[self->type];

  self->name = String_Capital(Grammar_Get()->Generate(rng, "$weapon",
    WeaponClass_String[self->type]));

  self->offset = V3(0, 0.5f, 4);

  self->powerDrain = kWeaponPowerDrainMult[self->type];

  self->renderable = renderable;

  self->magazineTime = self->uses ? Round(rng->GetFloat(6, 10)) : 0.0f;

  self->scale = 0.5f;

  self->spread =
    Constant_WeaponSpreadMult() * kWeaponSpreadMult[self->type] * rng->GetErlang(2);

  self->rate = Constant_WeaponRateMult() * kWeaponRateMult[self->type] *
    (1 + Floor(6 * rng->GetErlang(2)));

  if (self->uses)
    self->rate *= 1.5f;

  self->integrity = 100;

  self->capability = Capability_Attack(self->GetDPS());

  return self;
}
static Function const Item_WeaponType_Registration = Function_Bind(
  "Item_WeaponType",
  "None",
  &Item_WeaponType,
  "id");

static Function const WeaponType_SetOverride_Registration = Function_Bind(
  "WeaponType_SetOverride",
  "None",
  &WeaponType_SetOverride,
  "className");


