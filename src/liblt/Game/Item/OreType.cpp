#include "../Items.h"

#include "Game/Constants.h"
#include "Game/Attribute/Icon.h"
#include "Game/Attribute/Mass.h"
#include "Game/Attribute/Name.h"
#include "Game/Attribute/Value.h"

#include "LTE/Grammar.h"
#include "LTE/Math.h"
#include "LTE/RNG.h"
#include "LTE/Renderable.h"

#include "UI/Glyphs.h"
#include "LTE/FunctionBind.h"

namespace {
  Icon GenerateIcon(RNG const& rg) {
    Icon icon = Icon_Create();
    float seed = rg->GetFloat();

    V2 last = 0;
    V2 first = 0;

    uint points = rg->GetInt(6, 12);
    for (uint i = 0; i < points; ++i) {
      float t = static_cast<float>(i) / static_cast<float>(points);
      V2 point = 0.5f * Polar(kTau * t);
      point *= Clamp(0.5f - 0.75f * (Log(Fractal(ValueNoise3D, 4.0f * V3(point, 16.0f * seed), 4, 2))), 0.0f, 2.0f);

      if (i)
        icon->Add(Glyph_Line(last, point, 1, 1));
      else
        first = point;

      last = point;
    }

    icon->Add(Glyph_Line(first, last, 1, 1));
    return icon;
  }
}

using OreTypeBase = 
    Attribute_Icon
  < Attribute_Mass
  < Attribute_Name
  < Attribute_Value
  < ItemWrapper<ItemType_OreType>
  > > > >;

AutoClassDerivedEmpty(OreType, OreTypeBase)
  DERIVED_TYPE_EX(OreType)
};

DERIVED_IMPLEMENT(OreType)

Item Item_OreType(Item_OreType_Args const& args) {
  RNG rg = RNG_MTG(args.seed);

  Reference<OreType> self = new OreType;
  self->icon = GenerateIcon(rg);
  self->mass = Constant_ValueToMass(args.value);
  self->name = String_Capital(Grammar_Get()->Generate(rg, "$oretype", ""));
  self->value = args.value;
  return self;
}
static Function const Item_OreType_Registration = Function_Bind(
  "Item_OreType",
  "None",
  [](double const& value, uint const& seed) -> Item { return Item_OreType(value, seed); },
  "value", "seed");


