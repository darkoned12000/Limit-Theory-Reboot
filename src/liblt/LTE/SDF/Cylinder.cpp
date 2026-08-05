#include "../SDFs.h"

#include "LTE/Bound.h"
#include "LTE/Math.h"
#include "LTE/FunctionBind.h"

namespace {
  AutoClassDerived(SDFCylinder, SDFT,
    V3, center,
    V3, axis,
    float, radius)
    DERIVED_TYPE_EX(SDFCylinder)

    SDFCylinder() = default;

    float Evaluate(V3 const& p) const override {
      NOT_IMPLEMENTED
      return 1;
    }

    Bound3 GetBound() const override {
      return Bound3(-FLT_MAX, FLT_MAX);
    }

    String GetCode(String const& p) const override {
      return Stringize()
        | "cylinder(" | p | ", " | center | ", " | axis
        | ", " | radius | ")";
    }
  };

  DERIVED_IMPLEMENT(SDFCylinder)
}

SDF SDF_Cylinder(V3 const& center, V3 const& axis, float const& radius) {
  return new SDFCylinder(center, Normalize(axis), radius);
}
static Function const SDF_Cylinder_Registration = Function_Bind(
  "SDF_Cylinder",
  "None",
  &SDF_Cylinder,
  "center", "axis", "radius");


