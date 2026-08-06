#include "../RenderPasses.h"

#include "LTE/DrawState.h"
#include "LTE/Renderer.h"
#include "LTE/ShaderInstance.h"
#include "LTE/Texture2D.h"
#include "LTE/FunctionBind.h"

namespace {
  struct RadialBlur : public RenderPassT {
    Shader shader;
    V2 center;
    float radius;
    float strength;
    float falloff;
    DERIVED_TYPE_EX(RadialBlur)

    RadialBlur() = default;

    RadialBlur(
        V2 const& center,
        float radius,
        float strength,
        float falloff) :
      shader(Shader_Create("identity.jsl", "post/radialblur.jsl")),
      center(center),
      radius(radius),
      strength(strength),
      falloff(falloff)
      {}

    char const* GetName() const override {
      return "Radial Blur";
    }

    void OnRender(DrawState* state) override {
      Renderer_SetShader(*shader);
      (*shader)
        ("texture", state->primary)
        ("center", center)
        ("radius", radius)
        ("strength", strength)
        ("falloff", falloff);

      state->secondary->Bind(0);
      Renderer_DrawFSQ();
      state->secondary->Unbind();
      state->Flip();
    }
  };
}

RenderPass RenderPass_RadialBlur(V2 const& center, float const& radius, float const& strength, float const& falloff) {
  return new RadialBlur(center, radius, strength, falloff);
}
static Function const RenderPass_RadialBlur_Registration = Function_Bind(
  "RenderPass_RadialBlur",
  "None",
  &RenderPass_RadialBlur,
  "center", "radius", "strength", "falloff");


