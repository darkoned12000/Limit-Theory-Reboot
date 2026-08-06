#include "../SDFs.h"

#include "LTE/Bound.h"
#include "LTE/Math.h"
#include "LTE/FunctionBind.h"

namespace {
  AutoClassDerived(SDFWedge, SDFT,
    V3, center,
    float, angle,
    float, angularExtent,
    float, radius,
    float, radialExtent,
    float, height)
    DERIVED_TYPE_EX(SDFWedge)

    SDFWedge() = default;

    float Evaluate(V3 const& p) const override {
      NOT_IMPLEMENTED
      return 0;
    }

    Bound3 GetBound() const override {
      float r = radius + radialExtent;
      return Bound3(center - V3(r, height, r), center + V3(r, height, r));
    }

    String GetCode(const String& p) const override {
      return Stringize()
        | "wedge(" | p | ", " | center | ", " | angle | ", "
        | angularExtent | ", " | radius | ", " | radialExtent | ", "
        | height | ")";
    }
  };

  DERIVED_IMPLEMENT(SDFWedge)
}

SDF SDF_Wedge(V3 const& center, float const& angle, float const& angularExtent, float const& radius, float const& radialExtent, float const& height) {
  return new SDFWedge(
    center,
    angle,
    angularExtent,
    radius,
    radialExtent,
    height);
}
static Function const SDF_Wedge_Registration = Function_Bind(
  "SDF_Wedge",
  "None",
  &SDF_Wedge,
  "center", "angle", "angularExtent", "radius", "radialExtent", "height");


