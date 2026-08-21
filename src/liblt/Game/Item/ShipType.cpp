#include "../Items.h"

#include "Component/Integrity.h"
#include "Game/Constants.h"
#include "Game/Materials.h"
#include "Game/Objects.h"
#include "Game/Attribute/Capability.h"
#include "Game/Attribute/Icon.h"
#include "Game/Attribute/Integrity.h"
#include "Game/Attribute/Mass.h"
#include "Game/Attribute/Metatype.h"
#include "Game/Attribute/Name.h"
#include "Game/Attribute/Renderable.h"
#include "Game/Attribute/Scale.h"
#include "Game/Attribute/Sockets.h"
#include "Game/Attribute/Value.h"
#include "Game/DatabaseManager.h"
#include "Game/JsonHelpers.h"

#include "LTE/Math.h"
#include "LTE/Meshes.h"
#include "LTE/Model.h"
#include "LTE/PlateMesh.h"
#include "LTE/RNG.h"
#include "LTE/Ray.h"
#include "LTE/Script.h"
#include "LTE/SDFs.h"
#include "LTE/SDFMesh.h"
#include "LTE/StackFrame.h"

#include "UI/Glyphs.h"

#include "Game/Renderables.h"
#include "LTE/FunctionBind.h"

/* Balance knobs — loaded from ships.json "balance" section.
 * C++ defaults match ships.json; only used if JSON is missing. */
static uint kThrusterAttempts = 10;
static uint kTurretAttempts = 100;
static float kThrusterTolerance = 0.8f;

/* Ensure ships.json is loaded once. Returns true if available. */
static bool EnsureShipsDb() {
  static bool loaded = false;
  static bool available = false;
  if (loaded)
    return available;
  loaded = true;
  available = DatabaseManager_Get().Load(
    "ships", "resource/gamedata/ships.json");
  if (available) {
    json const* archetypes = DatabaseManager_Get().Find("ships", "shipArcheTypes");
    int archetypeCount = archetypes ? (int)archetypes->size() : 0;
    printf("Loaded ships.json (%d archetypes)\n", archetypeCount);

    /* Read balance knobs from JSON. */
    json const* balance = DatabaseManager_Get().Find("ships", "balance");
    if (balance) {
      int tmp;
      JInt(balance, "thrusterAttempts", tmp, (int)kThrusterAttempts);
      kThrusterAttempts = (uint)tmp;
      JInt(balance, "turretAttempts", tmp, (int)kTurretAttempts);
      kTurretAttempts = (uint)tmp;
      JFloat(balance, "thrusterTolerance", kThrusterTolerance, 0.8f);
    }
  } else {
    printf("WARNING: ships.json not found "
           "— using hardcoded ship defaults\n");
  }
  return available;
}

/* Match a value budget to a ship archetype by valueRange.
 * Returns the archetype name, or empty string if none matched. */
static String PickArchetype(double value) {
  json const* archetypes =
    DatabaseManager_Get().Find("ships", "shipArcheTypes");
  if (!archetypes || !archetypes->is_object())
    return "";

  for (auto it = archetypes->begin(); it != archetypes->end(); ++it) {
    json const* rangeVal = JGet(&it.value(), "valueRange");
    float minVal = 0, maxVal = 0;
    if (JRange(rangeVal, "ships.json: shipArcheTypes",
              minVal, maxVal, 0, 0)) {
      if (value >= minVal && value <= maxVal)
        return String(it.key().c_str());
    }
  }
  return "";
}

using ShipTypeBase = 
    Attribute_Capability
  < Attribute_Icon
  < Attribute_Integrity
  < Attribute_Mass
  < Attribute_Metatype
  < Attribute_Name
  < Attribute_Renderable
  < Attribute_Scale
  < Attribute_Sockets
  < Attribute_Value
  < ItemWrapper<ItemType_ShipType>
  > > > > > > > > > >;

AutoClassDerived(ShipType, ShipTypeBase,
  Item, standardGenerator,
  Item, standardScanner,
  Item, standardThruster)
  DERIVED_TYPE_EX(ShipType)

  ShipType() = default;

  Object Instantiate(ObjectT* parent) override {
    Object ship = Object_Ship(this);
    while (ship->Plug(standardThruster)) {}
    ship->Plug(standardGenerator);
    ship->Plug(standardScanner);

    /* Create shield and set armor from ships.json config.
     * Re-derives values from the original total budget stored in
     * metatype, since we can't add fields to AutoClassDerived. */
    if (EnsureShipsDb()) {
      float shieldValueRatio = 0.3f;
      float hullValueRatio = 0.6f;
      float shieldIntegrityMult = 1.0f;
      int armorRating = 0;

      json const* defaults =
        DatabaseManager_Get().Find("ships", "defaults");
      if (defaults) {
        JFloat(defaults, "shieldValueRatio", shieldValueRatio, 0.3f);
        JFloat(defaults, "hullValueRatio", hullValueRatio, 0.6f);
      }

      /* Re-derive archetype from original total budget. */
      Item_ShipType_Args const& args =
        this->metatype.Convert<Item_ShipType_Args>();
      String archetypeName = PickArchetype(args.value);
      json const* archetype = nullptr;
      if (archetypeName.size() > 0) {
        String archetypePath =
          Stringize() | "shipArcheTypes." | archetypeName;
        archetype = DatabaseManager_Get().FindPath("ships", archetypePath);
        if (archetype) {
          JFloat(archetype, "shieldIntegrityMult",
                 shieldIntegrityMult, 1.0f);
          JInt(archetype, "armorRating", armorRating, 0);
        }
      }

      /* Mirror the budget split from Item_ShipType(). */
      double valueRemaining = args.value;
      double hullValue = hullValueRatio * valueRemaining;
      valueRemaining -= hullValue;
      double thrusterValue =
        Saturate(0.5 / Sqrt(Sqrt(valueRemaining / 10000.0)))
        * valueRemaining;
      valueRemaining -= thrusterValue;
      double shieldValue = shieldValueRatio * valueRemaining;

      if (shieldValue > 0.0) {
        RNG rng = RNG_MTG(args.seed);
        Item shield = Item_ShieldType(
          shieldValue, rng->GetInt(),
          1.0f, 1.0f, shieldIntegrityMult, 1.0f);
        ship->Plug(shield);
      }

      /* Set armor rating on ship's Integrity component. */
      if (armorRating > 0) {
        ComponentIntegrity* integrity = ship->GetIntegrity();
        if (integrity)
          integrity->armorRating = armorRating;
      }
    }

    return ship;
  }
};

DERIVED_IMPLEMENT(ShipType)

Icon GetIcon(float scale) {
  if (scale <= 2)
    return Icon_Create()->Add(Glyph_Circle(0, 1, 1, 1));

  if (scale <= 5)
    return Icon_Create()->Add(Glyph_Arc(0, 0.10f, 0, 1, 1, 0, 1));

  if (scale <= 10)
    return Icon_Create()
      ->Add(Glyph_Circle(0, 1, 1, 1))
      ->Add(Glyph_Arc(0, 0.10f, 0, 1, 1, 0, 1));

  if (scale <= 20)
    return Icon_Create()
      ->Add(Glyph_Arc(0, 0.5f, 0.1f, 1, 1, 0.25f, 0.1f))
      ->Add(Glyph_Arc(0, 0.05f, 0.05f, 1, 1, 0, 1));

  if (scale <= 40)
    return Icon_Create()
      ->Add(Glyph_Arc(0, 0.5f, 0.05f, 1, 1, 0.25f, 0.08f))
      ->Add(Glyph_Arc(0, 0.5f, 0.05f, 1, 1, 0.75f, 0.08f))
      ->Add(Glyph_Arc(0, 0.05f, 0.05f, 1, 1, 0, 1));
  
  return Icon_Create()
    ->Add(Glyph_Arc(0, 0.5f, 0.05f, 1, 1, 0.00f, 0.08f))
    ->Add(Glyph_Arc(0, 0.5f, 0.05f, 1, 1, 0.25f, 0.08f))
    ->Add(Glyph_Arc(0, 0.5f, 0.05f, 1, 1, 0.50f, 0.08f))
    ->Add(Glyph_Arc(0, 0.5f, 0.05f, 1, 1, 0.75f, 0.08f))
    ->Add(Glyph_Arc(0, 0.05f, 0.05f, 1, 1, 0, 1));
}

V3 GetIntersection(
  Renderable const& renderable,
  V3 const& origin,
  V3 const& target,
  V3* normal = nullptr)
{
  Ray r(origin, target - origin);
  float t;
  return renderable->Intersects(r, &t, normal)
    ? r(t) :
    V3(FLT_MAX);
}

bool AttachThrusterPair(
  Renderable const& renderable,
  Vector<Socket>& sockets,
  V3 const& thrustDir,
  RNG const& rng,
  V3 const& scale)
{ AUTO_FRAME;
  V3 target, origin, normal, inter;
  Bound3 box = renderable->GetBound();

  for (uint i = 0; i < kThrusterAttempts; ++i) {
    target = box.Sample(rng->GetV3(0, 1));
    origin = target + 1000.0f * thrustDir;
    inter = GetIntersection(renderable, origin, target, &normal);
    if (inter.x != FLT_MAX && Dot(normal, thrustDir) >= kThrusterTolerance) {
      V3 look = thrustDir;
      sockets <<
        Socket(Transform_LookUp(inter, look, Cross(look, V3(1, 0, 0))) *
               Transform_Scale(scale), SocketType_Thruster);
      inter.x *= -1;
      normal.x *= -1;
      look.x *= -1;

      sockets <<
        Socket(Transform_LookUp(inter, look, Cross(look, V3(1, 0, 0))) *
               Transform_Scale(scale), SocketType_Thruster);
      return true;
    }
  }

  return false;
}

void AttachTurrets(
  Renderable const& renderable,
  Vector<Socket>& sockets,
  uint num,
  RNG const& rng)
{ AUTO_FRAME;
  /* Affix Turret Sockets. */
  Bound3 bound = renderable->GetBound();
  for (uint i = 0; i < num; ++i) {
    for (uint j = 0; j < kTurretAttempts; ++j) {
      V3 target = bound.Sample(rng->GetV3(0, 1));
      V3 normal = rng->GetFloat() < 0.5f ? V3(0, 1, 0) : V3(0, -1, 0);
      V3 origin = target + 100.0f * normal;
      V3 inter = GetIntersection(renderable, origin, target);

      if (inter.x != FLT_MAX) {
        origin = inter + 0.01f * normal;
        target = origin + V3(0, 0, 100);

        /* Make sure that the line-of-sight doesn't intersect the ship...that
           would be a bad place to put a turret. */
        if (GetIntersection(renderable, origin, target, nullptr).x == FLT_MAX) {
          sockets << Socket(
            Transform_LookUp(inter, V3(0, 0, 1), normal),
            SocketType_Turret,
            JointType::AxisY);

          sockets << Socket(
            Transform_LookUp(inter * V3(-1, 1, 1), V3(0, 0, 1), normal),
            SocketType_Turret,
            JointType::AxisY);
          break;
        }
      }
    }
  }
}

Item Item_ShipType(Item_ShipType_Args const& args) { AUTO_FRAME;
  RNG rng = RNG_MTG(args.seed);

  bool haveDb = EnsureShipsDb();

  /* ---- C++ emergency defaults (only used if JSON is missing/corrupt) ----
   * These MUST match the defaults section in ships.json. The JSON
   * "defaults" object is the canonical source; these exist only so the
   * engine can boot without the data file. */
  float hullValueRatio = 0.6f;
  float shieldValueRatio = 0.3f;
  float scannerValue = 1000.0f;
  int thrusterCount = 2;
  int turretCount = 4;

  /* Archetype multipliers (identity defaults = no change). */
  float capacityMult = 1.0f;
  float compactnessMult = 1.0f;
  float integrityMult = 1.0f;
  float shieldIntegrityMult = 1.0f;
  V3 hullTint(1.0f, 1.0f, 1.0f);
  String shipName = "Ship";

  /* ---- Read defaults from JSON ---- */
  if (haveDb) {
    json const* defaults =
      DatabaseManager_Get().Find("ships", "defaults");
    if (defaults) {
      JFloat(defaults, "hullValueRatio", hullValueRatio, 0.6f);
      JFloat(defaults, "shieldValueRatio", shieldValueRatio, 0.3f);
      JFloat(defaults, "scannerValue", scannerValue, 1000.0f);
      JInt(defaults, "thrusterCount", thrusterCount, 2);
      JInt(defaults, "turretCount", turretCount, 4);
    }

    /* ---- Match archetype by value and apply overrides ---- */
    String archetypeName = PickArchetype(args.value);
    if (archetypeName.size() > 0) {
      String archetypePath = Stringize() | "shipArcheTypes." | archetypeName;
      json const* archetype =
        DatabaseManager_Get().FindPath("ships", archetypePath);
      if (archetype) {
        String ap = Stringize() | "ships.json: " | archetypeName;
        JFloat(archetype, "capacityMult", capacityMult, 1.0f);
        JFloat(archetype, "compactnessMult", compactnessMult, 1.0f);
        JFloat(archetype, "integrityMult", integrityMult, 1.0f);
        JFloat(archetype, "shieldIntegrityMult", shieldIntegrityMult, 1.0f);
        JInt(archetype, "turretCount", turretCount, turretCount);
        JColor(archetype, "hullTint",
               Stringize() | "ships.json: " | archetypeName,
               hullTint, V3(1.0f, 1.0f, 1.0f));

        /* Read ship name from archetype. */
        json const* nameField = JGet(archetype, "name");
        if (nameField && nameField->is_string())
          shipName = String(nameField->get<std::string>().c_str());

        printf("  -> archetype: %s (%s, turrets=%d, compact=%.2f, integrity=%.2f, shield=%.2f)\n",
               archetypeName.c_str(), shipName.c_str(), turretCount, compactnessMult, integrityMult,
               shieldIntegrityMult);
      }
    }
  }

  /* Apply user-supplied multipliers on top of JSON archetype values.
   * JSON archetype sets the base; user args scale from there. */
  float finalCapacity = args.capacity * capacityMult;
  float finalCompactness = args.compactness * compactnessMult;
  float finalIntegrity = args.integrity * integrityMult;

  /* ---- Budget allocation (thruster formula stays in C++) ---- */
  double valueRemaining = args.value;
  double hullValue = hullValueRatio * valueRemaining;
  valueRemaining -= hullValue;
  double thrusterValue = Saturate(0.5 / Sqrt(Sqrt(valueRemaining / 10000.0))) * valueRemaining;
  valueRemaining -= thrusterValue;
  double shieldValue = shieldValueRatio * valueRemaining;
  valueRemaining -= shieldValue;
  double generatorValue = valueRemaining;

  Mass capacity = Constant_ValueToCapacity(hullValue, finalCapacity);
  Health integrity = Constant_ValueToIntegrity(hullValue, finalIntegrity);
  Mass mass = Constant_ValueToMass(hullValue, finalCompactness);

  Reference<ShipType> self = new ShipType;
  self->capability = Capability_Storage(capacity);
  self->integrity = integrity;
  self->mass = mass;
  self->metatype = Item_ShipType_Args(args);
  self->name = shipName;
  self->scale = Constant_MassToScale(mass);
  self->value = hullValue;

  self->icon = GetIcon(self->scale);

  float logScale = Log(self->scale);
  int generatorCount = static_cast<int>(rng->GetFloat(1, 2) + logScale);
  int interiorCount = 2 * static_cast<int>(logScale / Log(10.0f));

  Script_Reload("Item/ShipType/Generate");
  ScriptFunction_Load("Item/ShipType/Generate:Main")
    ->Call(self->renderable, self->scale, static_cast<int>(rng->GetInt()),
           hullTint);

  /* Sockets. */ {
    Vector<Socket> sockets;

    /* Create thruster sockets. */
    int actualThrusterCount = 0;
    for (int i = 0; i < thrusterCount; ++i)
      if (AttachThrusterPair(self->renderable, sockets, V3(0, 0, -1), rng, V3(1)))
        actualThrusterCount += 2;

    if (AttachThrusterPair(self->renderable, sockets, V3(1, 0, 0), rng, V3(0.25f)))
      actualThrusterCount += 2;
    if (AttachThrusterPair(self->renderable, sockets, V3(0, 1, 0), rng, V3(0.25f)))
      actualThrusterCount += 2;
    if (AttachThrusterPair(self->renderable, sockets, V3(0,-1, 0), rng, V3(0.25f)))
      actualThrusterCount += 2;
    if (AttachThrusterPair(self->renderable, sockets, V3(0, 0, 1), rng, V3(0.25f)))
      actualThrusterCount += 2;

    /* Default Power Generator. */
    self->standardGenerator = Item_PowerGeneratorType(generatorValue, rng->GetInt());
    sockets << Socket(Transform_Translation(V3(0, 0, 1)), SocketType_Generator);

    /* Default Scanner. */
    self->standardScanner = Item_ScannerType(scannerValue, rng->GetInt());
    sockets << Socket(Transform_Translation(V3(0, 0, -1)), SocketType_Generator);

    /* Thrusters. */
    thrusterValue /= actualThrusterCount;
    self->standardThruster = Item_ThrusterType(thrusterValue, rng->GetInt());

    /* Empty Slots. */
    AttachTurrets(self->renderable, sockets, turretCount, rng);
    for (int i = 0; i < generatorCount; ++i)
      sockets << Socket(Transform(), SocketType_Generator);
    for (int i = 0; i < interiorCount; ++i)
      sockets << Socket(Transform(), SocketType_Interior);
    self->sockets = Array<Socket>(sockets.size(), sockets.data());
  }

  return self;
}
static Function const Item_ShipType_Registration = Function_Bind(
  "Item_ShipType",
  "None",
  [](double const& value, uint const& seed, float const& capacity, float const& compactness, float const& integrity) -> Item { return Item_ShipType(value, seed, capacity, compactness, integrity); },
  "value", "seed", "capacity", "compactness", "integrity");


