#include "../SDFs.h"

#include "LTE/Bound.h"
#include "LTE/Math.h"
#include "LTE/FunctionBind.h"

namespace {
  AutoClassDerived(SDFRadial, SDFT,
    SDF, source,
    float, rMin,
    float, rMax)
    DERIVED_TYPE_EX(SDFRadial)

    SDFRadial() = default;

    float Evaluate(V3 const& p) const override {
      return Length(p) - Mix(rMin, rMax, source->Evaluate(p));
    }

    Bound3 GetBound() const override {
      return Bound3(V3(-rMax), V3(rMax));
    }

    String GetCode(String const& p) const override {
      return Stringize()
        | "(length(" | p | ") - mix(" | rMin | ", " | rMax | ", "
        | source->GetCode(p) | "))";
    }
  };

  DERIVED_IMPLEMENT(SDFRadial)
}

SDF SDF_Radial(SDF const& source, float const& rMin, float const& rMax) {
  return new SDFRadial(source, rMin, rMax);
}
static Function const SDF_Radial_Registration = Function_Bind(
  "SDF_Radial",
  "None",
  &SDF_Radial,
  "source", "rMin", "rMax");


