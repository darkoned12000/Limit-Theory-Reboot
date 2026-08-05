#include "../SDFs.h"

#include "LTE/Bound.h"
#include "LTE/Math.h"
#include "LTE/FunctionBind.h"

namespace {
  AutoClassDerived(SDFScale, SDFT,
    SDF, source,
    V3, scale)
    DERIVED_TYPE_EX(SDFScale)

    SDFScale() = default;

    float Evaluate(V3 const& p) const override {
      return source->Evaluate(p / scale);
    }

    Bound3 GetBound() const override {
      Bound3 bound = source->GetBound();
      return Bound3(bound.lower * scale, bound.upper * scale);
    }

    String GetCode(String const& p) const override {
      return source->GetCode(Stringize() | "(" | p | " / " | scale | ")");
    }
  };

  DERIVED_IMPLEMENT(SDFScale)
}

SDF SDF_Scale(SDF const& source, V3 const& scale) {
  return new SDFScale(source, scale);
}
static Function const SDF_Scale_Registration = Function_Bind(
  "SDF_Scale",
  "None",
  &SDF_Scale,
  "source", "scale");

static int const SDF_Scale_Alias = Function_Alias("SDF_Scale", "*");
