#include "../SDFs.h"

#include "LTE/Bound.h"
#include "LTE/Math.h"
#include "LTE/FunctionBind.h"

namespace {
  AutoClassDerived(SDFMultiply, SDFT,
    SDF, source,
    float, value)
    DERIVED_TYPE_EX(SDFMultiply)

    SDFMultiply() = default;

    float Evaluate(V3 const& p) const override {
      return source->Evaluate(p) * value;
    }

    Bound3 GetBound() const override {
      return source->GetBound();
    }

    String GetCode(String const& p) const override {
      return Stringize()
        | "(" | value | " * max(" | source->GetCode(p) | ", 0.))";
    }
  };

  DERIVED_IMPLEMENT(SDFMultiply)
}

SDF SDF_Multiply(SDF const& source, float const& value) {
  return new SDFMultiply(source, value);
}
static Function const SDF_Multiply_Registration = Function_Bind(
  "SDF_Multiply",
  "None",
  &SDF_Multiply,
  "source", "value");


