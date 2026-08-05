#include "../Compositors.h"

#include "LTE/Math.h"
#include "LTE/Mesh.h"
#include "LTE/Renderer.h"
#include "LTE/Shader.h"
#include "LTE/Texture2D.h"

#include "Module/FrameTimer.h"
#include "LTE/FunctionBind.h"

namespace {
  AutoClassDerived(CompositorBasic, CompositorT,
    float, lines,
    float, noise,
    V3, gradeBlue,
    float, age)
    Shader shader;

    CompositorBasic() = default;

    DefineInitializer {
      shader = Shader_Create("identity.jsl", "ui/basic.jsl");
    }

    void Composite(Texture2D const& layer, Mesh const& surface) override {
      RendererState s(BlendMode::Alpha, CullMode::Backface, false, false);
      (*shader)
        ("age", age)
        ("layer", layer)
        ("linesMag", lines)
        ("gradeBlue", gradeBlue)
        ("noiseMag", noise)
        ("seed", Rand())
        ("size", V2(layer->GetWidth(), layer->GetHeight()));
      Renderer_SetShader(*shader);
      surface->Draw();
    }

    void Update() override {
      age += FrameTimer_Get();
    }
  };
}

Compositor Compositor_Basic(float const& noise, float const& lines, V3 const& gradeBlue) {
  return new CompositorBasic(lines, noise, gradeBlue, 0);
}
static Function const Compositor_Basic_Registration = Function_Bind(
  "Compositor_Basic",
  "None",
  &Compositor_Basic,
  "noise", "lines", "gradeBlue");


