#include "../SDFs.h"

#include "LTE/Bound.h"
#include "LTE/Math.h"
#include "LTE/FunctionBind.h"

namespace {
  AutoClassDerived(SDFShell, SDFT,
    V3, center,
    float, radius,
    float, thickness)
    DERIVED_TYPE_EX(SDFShell)

    SDFShell() = default;

    float Evaluate(V3 const& p) const override {
      return Abs(Length(p - center) - radius) - thickness;
    }

    Bound3 GetBound() const override {
      return Bound3(center - V3(radius + thickness),
                  center + V3(radius + thickness));
    }

    String GetCode(String const& p) const override {
      return Stringize()
        | "shell(" | p | ", " | center | ", " | radius | ", " | thickness | ")";
    }
  };

  DERIVED_IMPLEMENT(SDFShell)
}

SDF SDF_Shell(V3 const& center, float const& radius, float const& thickness) {
  return new SDFShell(center, radius, thickness);
}
static Function const SDF_Shell_Registration = Function_Bind(
  "SDF_Shell",
  "None",
  &SDF_Shell,
  "center", "radius", "thickness");


