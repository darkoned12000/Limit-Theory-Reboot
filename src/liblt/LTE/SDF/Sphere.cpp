#include "../SDFs.h"

#include "LTE/Bound.h"
#include "LTE/Math.h"
#include "LTE/FunctionBind.h"

namespace {
  AutoClassDerived(SDFSphere, SDFT,
    V3, center,
    float, radius)
    DERIVED_TYPE_EX(SDFSphere)

    SDFSphere() = default;

    float Evaluate(V3 const& p) const override {
      return Length(p - center) - radius;
    }

    Bound3 GetBound() const override {
      return Bound3(center - V3(radius), center + V3(radius));
    }

    String GetCode(String const& p) const override {
      return Stringize()
        | "(length(" | p | " - " | center | ") - " | radius | ")";
    }
  };

  DERIVED_IMPLEMENT(SDFSphere)
}

SDF SDF_Sphere(V3 const& center, float const& radius) {
  return new SDFSphere(center, radius);
}
static Function const SDF_Sphere_Registration = Function_Bind(
  "SDF_Sphere",
  "None",
  &SDF_Sphere,
  "center", "radius");


