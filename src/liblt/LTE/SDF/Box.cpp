#include "../SDFs.h"

#include "LTE/Bound.h"
#include "LTE/Math.h"
#include "LTE/TypeTraits.h"
#include "LTE/FunctionBind.h"

namespace {
  AutoClassDerived(SDFBox, SDFT,
    V3, center,
    V3, sides)

    DERIVED_TYPE_EX(SDFBox)

    SDFBox() :
      center(0),
      sides(1)
      {}

    float Evaluate(V3 const& p) const override {
      return Length(Max(Abs(p - center) - sides, V3(0)));
    }

    Bound3 GetBound() const override {
      return Bound3(center - sides, center + sides);
    }

    String GetCode(String const& p) const override {
      return Stringize()
        | "box(" | p | ", " | center | ", " | sides | ")";
    }
  };

  DERIVED_IMPLEMENT(SDFBox)

  AutoClassDerived(SDFRoundBox, SDFT,
    V3, center,
    V3, sides,
    float, radius)

    DERIVED_TYPE_EX(SDFRoundBox)

    SDFRoundBox() :
      center(0),
      sides(1),
      radius(.5f)
      {}

    float Evaluate(V3 const& p) const override {
      return Length(Max(Abs(p - center) - sides * (1.f - radius), V3(0))) - radius;
    }

    Bound3 GetBound() const override {
      return Bound3(center - sides, center + sides);
    }

    String GetCode(String const& p) const override {
      return Stringize()
        | "boxr(" | p | ", " | center | ", " | sides | ", "
        | radius | ")";
    }
  };

  DERIVED_IMPLEMENT(SDFRoundBox)
}

SDF SDF_Box(V3 const& center, V3 const& sides) {
  return new SDFBox(center, sides);
}
static Function const SDF_Box_Registration = Function_Bind(
  "SDF_Box",
  "None",
  &SDF_Box,
  "center", "sides");



SDF SDF_RoundBox(V3 const& center, V3 const& sides, float const& radius) {
  return new SDFRoundBox(center, sides, radius);
}
static Function const SDF_RoundBox_Registration = Function_Bind(
  "SDF_RoundBox",
  "None",
  &SDF_RoundBox,
  "center", "sides", "radius");


