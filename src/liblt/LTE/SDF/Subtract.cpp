#include "../SDFs.h"

#include "LTE/Bound.h"
#include "LTE/Math.h"
#include "LTE/FunctionBind.h"

namespace {
  AutoClassDerived(SDFSubtract, SDFT,
    SDF, a,
    SDF, b,
    float, sharpness)
    DERIVED_TYPE_EX(SDFSubtract)

    SDFSubtract() = default;

    float Evaluate(V3 const& p) const override {
      float at = a->Evaluate(p);
      float bt = -b->Evaluate(p);
      float alpha = Sigmoid((bt - at) * sharpness);
      return Mix(at, bt, alpha);
    }

    Bound3 GetBound() const override {
      return a->GetBound();
    }

    String GetCode(const String& p) const override {
      return Stringize()
        | "intersect(" | a->GetCode(p) | ", -" | b->GetCode(p) | ", "
        | sharpness | ")";
    }
  };

  DERIVED_IMPLEMENT(SDFSubtract)

  AutoClassDerived(SDFSubtract0, SDFT,
    SDF, a,
    SDF, b)
    DERIVED_TYPE_EX(SDFSubtract0)

    SDFSubtract0() = default;

    float Evaluate(V3 const& p) const override {
      return Max(a->Evaluate(p), -b->Evaluate(p));
    }

    Bound3 GetBound() const override {
      return a->GetBound();
    }

    String GetCode(const String& p) const override {
      return Stringize()
        | "max(" | a->GetCode(p) | ", -" | b->GetCode(p) | ")";
    }
  };

  DERIVED_IMPLEMENT(SDFSubtract0)
}

SDF SDF_Subtract(SDF const& a, SDF const& b, float const& sharpness) {
  if (sharpness <= 0)
    return new SDFSubtract0(a, b);
  else
    return new SDFSubtract(a, b, sharpness);
}
static Function const SDF_Subtract_Registration = Function_Bind(
  "SDF_Subtract",
  "None",
  &SDF_Subtract,
  "a", "b", "sharpness");

static int const SDF_Subtract_Alias = Function_Alias("SDF_Subtract", "-");

SDF SDFT::Subtract(SDF const& other, float sharpness) {
  return SDF_Subtract(this, other, sharpness);
}
