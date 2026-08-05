#include "../SDFs.h"

#include "LTE/Bound.h"
#include "LTE/Math.h"
#include "LTE/FunctionBind.h"

namespace {
  AutoClassDerived(SDFCapsule, SDFT,
    V3, o,
    V3, n,
    V3, nNorm,
    float, length,
    float, radius)
    DERIVED_TYPE_EX(SDFCapsule)

    SDFCapsule() = default;

    SDFCapsule(V3 const& p1, V3 const& p2, float radius) :
      o(p1),
      n(p2 - p1),
      radius(radius)
    {
      length = Length(n);
      nNorm = Normalize(n);
    }

    float Evaluate(V3 const& p) const override {
      V3 v = p - o;
      float proj = Clamp(Dot(v, nNorm), 0.f, length);
      return Length(v - nNorm * proj) - radius;
    }

    Bound3 GetBound() const override {
      Bound3 box = Bound3::FromPoints(o, o + n);
      return Bound3(box.lower - V3(radius), box.upper + V3(radius));
    }

    String GetCode(String const& p) const override {
      return Stringize()
        | "capsule(" | p | ", " | o | ", " | nNorm | ", " | length
        | ", " | radius | ")";
    }
  };

  DERIVED_IMPLEMENT(SDFCapsule)
}

SDF SDF_Capsule(V3 const& p1, V3 const& p2, float const& radius) {
  return new SDFCapsule(p1, p2, radius);
}
static Function const SDF_Capsule_Registration = Function_Bind(
  "SDF_Capsule",
  "None",
  &SDF_Capsule,
  "p1", "p2", "radius");


