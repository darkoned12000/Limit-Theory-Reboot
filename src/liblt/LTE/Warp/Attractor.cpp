#include "../Warps.h"

#include "LTE/AutoClass.h"
#include "LTE/FunctionBind.h"

namespace {
  AutoClassDerived(WarpAttractorPoint, WarpT,
    V3, center,
    float, strength)
    DERIVED_TYPE_EX(WarpAttractorPoint)

    WarpAttractorPoint() = default;

    V3 GetDelta(V3 const& p) const override {
      V3 toCenter = center - p;
      float dist = Length(toCenter);
      return Mix(p, center, Exp(-dist / strength)) - p;
    }
  };

  AutoClassDerived(WarpAttractorPlane, WarpT,
    V3, center,
    V3, normal,
    float, strength)
    DERIVED_TYPE_EX(WarpAttractorPlane)

    WarpAttractorPlane() = default;

    V3 GetDelta(V3 const& p) const override {
      V3 proj = p + normal * Dot(center - p, normal);
      V3 toCenter = proj - p;
      float dist = Length(toCenter);
      return Mix(p, proj, Exp(-dist / strength)) - p;
    }
  };
}

Warp Warp_AttractorPlane(V3 const& center, V3 const& normal, float const& strength) {
  return new WarpAttractorPlane(center, normal, strength);
}
static Function const Warp_AttractorPlane_Registration = Function_Bind(
  "Warp_AttractorPlane",
  "None",
  &Warp_AttractorPlane,
  "center", "normal", "strength");



Warp Warp_AttractorPoint(V3 const& center, float const& strength) {
  return new WarpAttractorPoint(center, strength);
}
static Function const Warp_AttractorPoint_Registration = Function_Bind(
  "Warp_AttractorPoint",
  "None",
  &Warp_AttractorPoint,
  "center", "strength");


