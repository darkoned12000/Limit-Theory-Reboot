#include "../RenderPasses.h"

#include "Component/Interior.h"

#include "Game/Object.h"
#include "Game/Graphics/RenderStyles.h"

#include "LTE/DrawState.h"
#include "LTE/ParticleSystem.h"
#include "LTE/RenderStyle.h"
#include "LTE/FunctionBind.h"

namespace {
  struct Particles : public RenderPassT {
    RenderStyle style;
    DERIVED_TYPE_EX(Particles)

    Particles() :
      style(RenderStyle_Default(true))
      {}

    char const* GetName() const override {
      return "Particles";
    }

    void OnRender(DrawState* state) override {
      RenderStyle_Push(style);
      ObjectT* container = (ObjectT*)state->visible[0];
      container->GetInterior()->particles->Draw(state);
      RenderStyle_Pop();
    }
  };
}

RenderPass RenderPass_Particles() {
  return new Particles;
}
static Function const RenderPass_Particles_Registration = Function_Bind(
  "RenderPass_Particles",
  "None",
  &RenderPass_Particles);


