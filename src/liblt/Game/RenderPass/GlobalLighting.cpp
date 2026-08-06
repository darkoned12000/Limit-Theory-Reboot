#include "../RenderPasses.h"

#include "LTE/DrawState.h"
#include "LTE/Renderer.h"
#include "LTE/RenderStyle.h"
#include "LTE/Shader.h"
#include "LTE/Texture2D.h"
#include "LTE/FunctionBind.h"

namespace {
  struct GlobalLighting : public RenderPassT {
    Shader shader;
    DERIVED_TYPE_EX(GlobalLighting)

    GlobalLighting() :
      shader(Shader_Create("worldray.jsl", "light/global.jsl"))
      {}

    char const* GetName() const override {
      return "Global Lighting Pass";
    }

    void OnRender(DrawState* state) override {
      Texture2D const& lightBuffer = state->color[2];
      // Texture2D lightBuffer = state->smallColor[0];

      /* Global light accumulation. */ {
        lightBuffer->Bind(0);

        DrawState_Link(shader);
        (*shader)("normalBuffer", state->color[1]);

        Renderer_Clear();
        Renderer_SetShader(*shader);
        Renderer_PushZBuffer(false);
        Renderer_DrawQuad();
        Renderer_PopZBuffer();

        lightBuffer->Unbind();
      }
    }
  };
}

RenderPass RenderPass_GlobalLighting() {
  return new GlobalLighting;
}
static Function const RenderPass_GlobalLighting_Registration = Function_Bind(
  "RenderPass_GlobalLighting",
  "None",
  &RenderPass_GlobalLighting);


