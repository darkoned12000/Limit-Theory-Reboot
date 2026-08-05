#include "../RenderPasses.h"

#include "LTE/Data.h"
#include "LTE/DrawState.h"
#include "LTE/Renderer.h"
#include "LTE/Script.h"
#include "LTE/ShaderInstance.h"
#include "LTE/Texture2D.h"
#include "LTE/FunctionBind.h"

namespace {
  AutoClassDerived(CustomFilter, RenderPassT,
    Data, instance,
    ScriptFunction, render)
    DERIVED_TYPE_EX(CustomFilter)

    CustomFilter() = default;

    char const* GetName() const override {
      return "Custom Filter";
    }

    void OnRender(DrawState* state) override {
      Texture2D const& input = state->primary;
      Texture2D result;
      render->VoidCall(&result, instance, input);
      state->primary = result;
    }
  };
}

RenderPass RenderPass_CustomFilter(Data const& data) {
  Reference<CustomFilter> self = new CustomFilter;
  ScriptType type = data.type->GetAux().Convert<ScriptType>();
  self->instance = data;
  self->render = type->GetFunction("Render");
  return self;
}
static Function const RenderPass_CustomFilter_Registration = Function_Bind(
  "RenderPass_CustomFilter",
  "None",
  &RenderPass_CustomFilter,
  "data");


