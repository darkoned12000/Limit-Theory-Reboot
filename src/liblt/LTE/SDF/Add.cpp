#include "../SDFs.h"

#include "LTE/Bound.h"
#include "LTE/Math.h"
#include "LTE/FunctionBind.h"

namespace {
  AutoClassDerived(SDFAdd, SDFT,
    SDF, a,
    SDF, b)
    DERIVED_TYPE_EX(SDFAdd)

    SDFAdd() = default;

    float Evaluate(V3 const& p) const override {
      return a->Evaluate(p) + b->Evaluate(p);
    }

    Bound3 GetBound() const override {
      /* NOTE : There is no good way to
                get a bound on arbitrary density addition. */
      return a->GetBound();
    }

    String GetCode(String const& p) const override {
      return "(" + a->GetCode(p) + " + " + b->GetCode(p) + ")";
    }
  };

  DERIVED_IMPLEMENT(SDFAdd)
}

SDF SDF_Add(SDF const& a, SDF const& b) {
  return new SDFAdd(a, b);
}
static Function const SDF_Add_Registration = Function_Bind(
  "SDF_Add",
  "None",
  &SDF_Add,
  "a", "b");

static int const SDF_Add_Alias = Function_Alias("SDF_Add", "+");
