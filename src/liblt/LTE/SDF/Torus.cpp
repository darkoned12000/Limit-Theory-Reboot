#include "../SDFs.h"

#include "LTE/Bound.h"
#include "LTE/Math.h"
#include "LTE/FunctionBind.h"

namespace {
  AutoClassDerived(SDFTorus, SDFT,
    V3, center,
    float, radius,
    float, thickness)
    DERIVED_TYPE_EX(SDFTorus)

    SDFTorus() = default;

    float Evaluate(V3 const& p) const override {
      V3 o = p - center;
      return Length(o - radius * Normalize(V3(o.x, 0.0f, o.z))) - thickness;
    }

    Bound3 GetBound() const override {
      return Bound3(
        center - V3(radius + thickness),
        center + V3(radius + thickness));
    }

    String GetCode(String const& p) const override {
      return Stringize()
        | "torus(" | p | ", " | center | ", " | radius | ", " | thickness | ")";
    }
  };

  DERIVED_IMPLEMENT(SDFTorus)
}

SDF SDF_Torus(V3 const& center, float const& radius, float const& thickness) {
  return new SDFTorus(center, radius, thickness);
}
static Function const SDF_Torus_Registration = Function_Bind(
  "SDF_Torus",
  "None",
  &SDF_Torus,
  "center", "radius", "thickness");


