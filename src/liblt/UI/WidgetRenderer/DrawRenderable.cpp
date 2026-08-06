#include "UI/WidgetRenderer.h"

#include "LTE/Color.h"
#include "LTE/Data.h"
#include "LTE/DrawState.h"
#include "LTE/Geometry.h"
#include "LTE/Math.h"
#include "LTE/Renderable.h"
#include "LTE/Renderer.h"
#include "LTE/RenderStyle.h"
#include "LTE/ShaderInstance.h"
#include "LTE/Transform.h"
#include "LTE/View.h"
#include "LTE/Viewport.h"
#include "LTE/FunctionBind.h"

namespace {

  struct HoloStyle : public RenderStyleT {
    ShaderInstance currentShader;
    ShaderInstance holoShader;

    HoloStyle() :
      holoShader(ShaderInstance_Create("npm.jsl", "hologram.jsl"))
      {}

    Shader const& GetShader () const {
      return holoShader->GetShader();
    }

    void OnBegin() override {
      Renderer_PushBlendMode(BlendMode::Additive);
      Renderer_PushZBuffer(true);
      Renderer_PushZWritable(true);
      (*holoShader)("seed", Rand());
    }

    void OnEnd() override {
      Renderer_PopBlendMode();
      Renderer_PopZBuffer();
      Renderer_PopZWritable();
    }

    void Render(Geometry const& geometry) override {
      currentShader->Begin();
      geometry->Draw();
      currentShader->End();
    }

    void SetShader(ShaderInstanceT* shader) override {
      bool hasBlending = shader->HasBlending();
      if (hasBlending)
        currentShader = shader;
      else
        currentShader = holoShader;

      int prepassLoc = currentShader->GetShader()->QueryUniformLocation("prepass");
      if (prepassLoc >= 0)
        currentShader->GetShader()->SetInt(prepassLoc, 0);
      DrawState_Inject(currentShader->GetShader());
    }

    void SetTransform(Transform const& transform) override {
      Renderer_SetWorldTransform(transform);
    }

    bool WillRender() const override {
      return true;
    }
  };

  Reference<HoloStyle> holoStyle;

  void Initialize() {
    static bool init = false;
    if (!init) {
      init = true;
      holoStyle = new HoloStyle;
    }
  }
}

void WidgetRenderer_DrawRenderable(Renderable const& renderable, Transform const& transform, V3 const& camPos, V3 const& camLook, V3 const& camUp, float const& camFov, V2 const& pos, V2 const& size, Color const& color, float const& alpha, float const& time) {
  Initialize();
  WidgetRenderer_Flush();

  Viewport const& currentVp = Viewport_Get();
  Viewport vp = Viewport_Create(pos, size, 1, currentVp->windowSpace);
  Viewport_Push(vp);
  DrawState state;
  state.Push();

#if 0
  float angle = Radians(60.0f);
  float h = 15.0f;
  float d = 0.5f * h / tan(0.5f * angle);
#endif
  View view(
    Transform_LookUp(camPos, camLook, camUp),
    camFov,
    size.x / size.y,
    0.05f,
    1.0e6f);

  Renderer_PushScissorOff();
  Renderer_SetViewTransform(view.transform);
  Renderer_SetProjMatrix(view.proj);

  Shader const& shader = holoStyle->GetShader();
  (*shader)
    ("baseAlpha", alpha)
    ("baseColor", V3(color))
    ("time", time);

  RenderStyle_Push(holoStyle);

  holoStyle->OnBegin();
  holoStyle->SetTransform(transform);
  renderable->Render(&state);
  holoStyle->OnEnd();

  RenderStyle_Pop();

  state.Pop();
  Viewport_Pop();
  Renderer_PopScissor();
}
static Function const WidgetRenderer_DrawRenderable_Registration = Function_Bind(
  "WidgetRenderer_DrawRenderable",
  "None",
  &WidgetRenderer_DrawRenderable,
  "renderable", "transform", "camPos", "camLook", "camUp", "camFov", "pos", "size", "color", "alpha", "time");
static int const WidgetRenderer_DrawRenderable_Alias = Function_Alias("WidgetRenderer_DrawRenderable", "DrawRenderable");


