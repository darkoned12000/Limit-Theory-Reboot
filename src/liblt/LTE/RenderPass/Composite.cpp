#include "../RenderPasses.h"

#include "LTE/Vector.h"
#include "LTE/FunctionBind.h"

namespace {
  struct Composite : public RenderPassT {
    Vector<RenderPass> passes;
    DERIVED_TYPE_EX(Composite)
  
    Composite() = default;

    Composite(Vector<RenderPass> const& passes) : passes(passes) {}

    char const* GetName() const override {
      return "Composite";
    }

    void OnRender(DrawState* state) override {
      for (size_t i = 0; i < passes.size(); ++i)
        passes[i]->Render(state);
    }
  };
}

RenderPass RenderPass_Composite(Vector<RenderPass> const& passes) {
  return new Composite(passes);
}
static Function const RenderPass_Composite_Registration = Function_Bind(
  "RenderPass_Composite",
  "None",
  &RenderPass_Composite,
  "passes");


