#include "../Warps.h"

#include "LTE/AutoClass.h"
#include "LTE/Script.h"
#include "LTE/FunctionBind.h"

namespace {
  AutoClassDerived(WarpCustom, WarpT,
    Data, instance,
    ScriptFunction, getDelta)
    DERIVED_TYPE_EX(WarpCustom)

    WarpCustom() = default;

    V3 GetDelta(V3 const& p) const override {
      V3 result;
      getDelta->VoidCall(&result, instance, p);
      return result;
    }
  };
}

Warp Warp_Custom(Data const& data) {
  ScriptType type = data.type->GetAux().Convert<ScriptType>();
  Reference<WarpCustom> self = new WarpCustom;
  self->instance = data;
  self->getDelta = type->GetFunction("GetDelta");
  return self;
}
static Function const Warp_Custom_Registration = Function_Bind(
  "Warp_Custom",
  "None",
  &Warp_Custom,
  "data");


