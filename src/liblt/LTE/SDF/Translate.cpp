#include "../SDFs.h"

#include "LTE/Bound.h"
#include "LTE/Math.h"
#include "LTE/FunctionBind.h"

namespace {
  AutoClassDerived(SDFTranslate, SDFT,
    SDF, source,
    V3, offset)
    DERIVED_TYPE_EX(SDFTranslate)

    SDFTranslate() = default;

    float Evaluate(V3 const& p) const override {
      return source->Evaluate(p - offset);
    }

    Bound3 GetBound() const override {
      Bound3 bound = source->GetBound();
      return Bound3(bound.lower + offset, bound.upper + offset);
    }

    String GetCode(String const& p) const override {
      return source->GetCode(Stringize() | "(" | p | " - " | offset | ")");
    }
  };

  DERIVED_IMPLEMENT(SDFTranslate)
}

SDF SDF_Translate(SDF const& source, V3 const& offset) {
  return new SDFTranslate(source, offset);
}
static Function const SDF_Translate_Registration = Function_Bind(
  "SDF_Translate",
  "None",
  &SDF_Translate,
  "source", "offset");

static int const SDF_Translate_Alias = Function_Alias("SDF_Translate", "+");
