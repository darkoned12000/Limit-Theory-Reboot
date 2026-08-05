#include "../SDFs.h"

#include "LTE/Bound.h"
#include "LTE/Math.h"
#include "LTE/FunctionBind.h"

namespace {
  AutoClassDerived(SDFRing, SDFT,
    V3, center,
    float, radius,
    float, thickness)
    DERIVED_TYPE_EX(SDFRing)

    SDFRing() = default;

    float Evaluate(V3 const& p) const override {
      return Abs(Length(p.GetXZ() - center.GetXZ()) - radius) - thickness;
    }

    Bound3 GetBound() const override {
      return Bound3(center - V3(radius + thickness),
                  center + V3(radius + thickness));
    }

    String GetCode(String const& p) const override {
      return Stringize()
        | "(abs(length(" | p | ".xz - " | center | ".xz) - " | radius | ") - "
        | thickness | ")";
    }
  };

  DERIVED_IMPLEMENT(SDFRing)
}

SDF SDF_Ring(V3 const& center, float const& radius, float const& thickness) {
  return new SDFRing(center, radius, thickness);
}
static Function const SDF_Ring_Registration = Function_Bind(
  "SDF_Ring",
  "None",
  &SDF_Ring,
  "center", "radius", "thickness");


