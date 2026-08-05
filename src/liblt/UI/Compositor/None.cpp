#include "../Compositors.h"

#include "LTE/Math.h"
#include "LTE/Mesh.h"
#include "LTE/Renderer.h"
#include "LTE/Shader.h"
#include "LTE/Texture2D.h"
#include "LTE/FunctionBind.h"

namespace {
  AutoClassDerivedEmpty(CompositorNone, CompositorT)
    Shader shader;

    CompositorNone() {
      shader = Shader_Create("identity.jsl", "ui/none.jsl");
    }

    void Composite(Texture2D const& layer, Mesh const& surface) override {
      RendererState s(BlendMode::Alpha, CullMode::Backface, false, false);
      (*shader)("layer", layer);
      Renderer_SetShader(*shader);
      surface->Draw();
    }
  };
}

Compositor Compositor_None() {
  return new CompositorNone;
}
static Function const Compositor_None_Registration = Function_Bind(
  "Compositor_None",
  "None",
  &Compositor_None);


