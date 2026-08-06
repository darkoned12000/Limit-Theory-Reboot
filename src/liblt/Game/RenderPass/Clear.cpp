#include "../RenderPasses.h"

#include "LTE/DrawState.h"
#include "LTE/Renderer.h"
#include "LTE/Texture2D.h"
#include "LTE/FunctionBind.h"

namespace {
  struct Clear : public RenderPassT {
    V4 value;
    DERIVED_TYPE_EX(Clear)

    Clear() = default;

    Clear(V4 const& value) :
      value(value)
      {}

    char const* GetName() const override {
      return "Clear";
    }

    void OnRender(DrawState* state) override {
      Renderer_ResetCounters();
      state->primary->Bind(0);
      Renderer_Clear(value);
      state->primary->Unbind();

      for (uint i = 0; i < 2; ++i) {
        state->smallColor[i]->Bind(0);
        Renderer_Clear();
        state->smallColor[i]->Unbind();
      }
    }
  };

  struct ClearDepth : public RenderPassT {
    DERIVED_TYPE_EX(ClearDepth)

    char const* GetName() const override {
      return "Clear Depth";
    }

    void OnRender(DrawState* state) override {
      state->primary->Bind(0);
      Renderer_ClearDepth();
      state->primary->Unbind();
    }
  };
}

RenderPass RenderPass_Clear(V4 const& value) {
  return new Clear(value);
}
static Function const RenderPass_Clear_Registration = Function_Bind(
  "RenderPass_Clear",
  "None",
  &RenderPass_Clear,
  "value");



RenderPass RenderPass_ClearDepth() {
  return new ClearDepth;
}
static Function const RenderPass_ClearDepth_Registration = Function_Bind(
  "RenderPass_ClearDepth",
  "None",
  &RenderPass_ClearDepth);


