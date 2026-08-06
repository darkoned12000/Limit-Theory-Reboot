#include "WidgetRenderer.h"
#include "Glyph.h"

#include "LTE/AutoClass.h"
#include "LTE/DrawState.h"
#include "LTE/Font.h"
#include "LTE/Map.h"
#include "LTE/Math.h"
#include "LTE/Profiler.h"
#include "LTE/Renderer.h"
#include "LTE/Shader.h"
#include "LTE/ShaderInstance.h"
#include "LTE/StackFrame.h"
#include "LTE/Vector.h"
#include "LTE/Viewport.h"

#include "LTE/Debug.h"
#include "LTE/FunctionBind.h"

const float kPanelShadowSize = 32;

namespace {
  AutoClass(GlyphInstance,
    Glyph, glyph,
    GlyphState, state)
    GlyphInstance() = default;
  };

  AutoClass(PanelVertex,
    V3, p,
    V4, uvsize,
    V4, color,
    V2, innerAlphaBevel)
    PanelVertex() = default;
  };

  AutoClass(RadialPanelVertex,
    V3, p,
    V4, uvr1r2,
    V4, color,
    V4, innerAlphaBevelPhaseAngle)
    RadialPanelVertex() = default;
  };

  using GlyphMapT = Map<Type, Vector<GlyphInstance> >;

  struct WidgetRenderer {
    GlyphMapT glyphs;

    Vector<PanelVertex> panelBuffer;
    Vector<RadialPanelVertex> radialPanelBuffer;
    Vector<uchar> vertexBuffer;
    Vector<uint> indexBuffer;

    Shader panelShader;
    Shader radialPanelShader;
    Shader textureShader;
    Shader textureShaderAdditive;

    Map<Type, Shader> shaderCache;
  } renderer;

  void BindShaderInputs(ShaderT* shader) {
    shader->BindInput(1, "vert_attrib1");
    shader->BindInput(2, "vert_attrib2");
    shader->BindInput(3, "vert_attrib3");
    shader->BindInput(4, "vert_attrib4");
    shader->Relink();
  }

  void WidgetRenderer_Initialize() {
    if (!renderer.panelShader) {
      renderer.panelShader = Shader_Create("widget.jsl", "ui/panel.jsl");
      renderer.radialPanelShader = Shader_Create("widget.jsl", "ui/radialpanel.jsl");
      renderer.textureShader = Shader_Create("widgetTexture.jsl", "ui/texture.jsl");
      renderer.textureShaderAdditive = Shader_Create("widgetTexture.jsl", "ui/textureadditive.jsl");
      BindShaderInputs(renderer.panelShader);
    }
  }

  void PopulateIndices(size_t count) {
    for (uint i = renderer.indexBuffer.size() / 6; i < count; ++i) {
      uint offset = i * 4;
      renderer.indexBuffer.push(offset + 0);
      renderer.indexBuffer.push(offset + 1);
      renderer.indexBuffer.push(offset + 2);
      renderer.indexBuffer.push(offset + 0);
      renderer.indexBuffer.push(offset + 2);
      renderer.indexBuffer.push(offset + 3);
    }
  }

  template <class T>
  void Render(Shader const& shader, Vector<T> const& vertexBuffer) {
    DrawState_Link(shader);
    Renderer_SetShader(*shader);
    Renderer_DrawVertices(
      vertexBuffer.data(),
      Type_Get<T>(),
      renderer.indexBuffer.data(),
      6 * (vertexBuffer.size() / 4),
      GL_IndexFormat::Int);
    Profiler_Flush();
  }

  void RenderGlyphs(
    Type const& glyphType,
    Vector<GlyphInstance> const& glyphs)
  {
    if (!glyphs.size())
      return;

    Shader& shader = renderer.shaderCache[glyphType];
    if (!shader) {
      shader = glyphs[0].glyph->GetShader();
      BindShaderInputs(shader);
    }

    PopulateIndices(glyphs.size());

    Type const& vertexFormat = glyphs[0].glyph->GetVertexFormat();
    renderer.vertexBuffer.clear();
    renderer.vertexBuffer.reserve(4 * glyphs.size() * vertexFormat->size);

    for (size_t i = 0; i < glyphs.size(); ++i) {
      uchar* pBuffer = renderer.vertexBuffer.data() + 4 * i * vertexFormat->size;
      glyphs[i].glyph->Submit((void*)pBuffer, glyphs[i].state);
    }

    DrawState_Link(shader);
    Renderer_SetShader(*shader);
    Renderer_DrawVertices(
      renderer.vertexBuffer.data(),
      vertexFormat,
      renderer.indexBuffer.data(),
      6 * glyphs.size(),
      GL_IndexFormat::Int);
    Profiler_Flush();
  }
}

void WidgetRenderer_DrawGlyph(Glyph const& glyph, GlyphState const& state) {
  renderer.glyphs[glyph->GetDerivedTypeInfo()]
    .push(GlyphInstance(glyph, state));
}
static Function const WidgetRenderer_DrawGlyph_Registration = Function_Bind(
  "WidgetRenderer_DrawGlyph",
  "None",
  &WidgetRenderer_DrawGlyph,
  "glyph", "state");
static int const WidgetRenderer_DrawGlyph_Alias = Function_Alias("WidgetRenderer_DrawGlyph", "DrawGlyph");



void WidgetRenderer_DrawPanel(V2 const& pos, V2 const& size, Color const& color, float const& innerAlpha, float const& alpha, float const& bevel) {
  WidgetRenderer_Flush();
  SFRAME("Panel");
  RendererState rs(BlendMode::Alpha, CullMode::Disabled, false, false);
  renderer.panelBuffer.clear();
  PopulateIndices(1);

  V2 ss1 = pos;
  V2 ss2 = pos + size;
  V2 sizePx = ss2 - ss1 + 2.0f * V2(kPanelShadowSize);
  V4 color4 = V4(color, alpha);

  renderer.panelBuffer.push(PanelVertex(
    V3(V2(ss1.x - kPanelShadowSize, ss1.y - kPanelShadowSize), 0),
    V4(0, 0, sizePx.x, sizePx.y),
    color4,
    V2(innerAlpha, bevel)));

  renderer.panelBuffer.push(PanelVertex(
    V3(V2(ss2.x + kPanelShadowSize, ss1.y - kPanelShadowSize), 0),
    V4(1, 0, sizePx.x, sizePx.y),
    color4,
    V2(innerAlpha, bevel)));

  renderer.panelBuffer.push(PanelVertex(
    V3(V2(ss2.x + kPanelShadowSize, ss2.y + kPanelShadowSize), 0),
    V4(1, 1, sizePx.x, sizePx.y),
    color4,
    V2(innerAlpha, bevel)));

  renderer.panelBuffer.push(PanelVertex(
    V3(V2(ss1.x - kPanelShadowSize, ss2.y + kPanelShadowSize), 0),
    V4(0, 1, sizePx.x, sizePx.y),
    color4,
    V2(innerAlpha, bevel)));

  (*renderer.panelShader)("shadowSize", kPanelShadowSize);
  Render(renderer.panelShader, renderer.panelBuffer);
}
static Function const WidgetRenderer_DrawPanel_Registration = Function_Bind(
  "WidgetRenderer_DrawPanel",
  "None",
  &WidgetRenderer_DrawPanel,
  "pos", "size", "color", "innerAlpha", "alpha", "bevel");
static int const WidgetRenderer_DrawPanel_Alias = Function_Alias("WidgetRenderer_DrawPanel", "DrawPanel");



void WidgetRenderer_DrawPanelRadial(V2 const& pos, float const& r1, float const& r2, float const& phase, float const& angle, Color const& color, float const& innerAlpha, float const& alpha, float const& bevel) {
  WidgetRenderer_Flush();
  SFRAME("PanelRadial");
  RendererState rs(BlendMode::Alpha, CullMode::Disabled, false, false);
  renderer.radialPanelBuffer.clear();
  PopulateIndices(1);

  V2 ss1 = pos - V2(r2 + kPanelShadowSize);
  V2 ss2 = pos + V2(r2 + kPanelShadowSize);
  V4 color4 = V4(color, alpha);

  renderer.radialPanelBuffer.push(RadialPanelVertex(
    V3(V2(ss1.x - kPanelShadowSize, ss1.y - kPanelShadowSize), 0),
    V4(0, 0, r1, r2),
    color4,
    V4(innerAlpha, bevel, phase, angle)));

  renderer.radialPanelBuffer.push(RadialPanelVertex(
    V3(V2(ss2.x + kPanelShadowSize, ss1.y - kPanelShadowSize), 0),
    V4(1, 0, r1, r2),
    color4,
    V4(innerAlpha, bevel, phase, angle)));

  renderer.radialPanelBuffer.push(RadialPanelVertex(
    V3(V2(ss2.x + kPanelShadowSize, ss2.y + kPanelShadowSize), 0),
    V4(1, 1, r1, r2),
    color4,
    V4(innerAlpha, bevel, phase, angle)));

  renderer.radialPanelBuffer.push(RadialPanelVertex(
    V3(V2(ss1.x - kPanelShadowSize, ss2.y + kPanelShadowSize), 0),
    V4(0, 1, r1, r2),
    color4,
    V4(innerAlpha, bevel, phase, angle)));

  (*renderer.radialPanelShader)("shadowSize", kPanelShadowSize);
  Render(renderer.radialPanelShader, renderer.radialPanelBuffer);
}
static Function const WidgetRenderer_DrawPanelRadial_Registration = Function_Bind(
  "WidgetRenderer_DrawPanelRadial",
  "None",
  &WidgetRenderer_DrawPanelRadial,
  "pos", "r1", "r2", "phase", "angle", "color", "innerAlpha", "alpha", "bevel");
static int const WidgetRenderer_DrawPanelRadial_Alias = Function_Alias("WidgetRenderer_DrawPanelRadial", "DrawPanelRadial");



template <class T>
void WidgetRenderer_DrawTextGeneric(T const& args, bool additive) {
  V2 textSize = args.font->GetTextSize(args.text, args.size);
  V2 offset = V2(0, 0.25f * textSize.y);
  if (args.centered)
    offset.x = -0.5f * textSize.x;

  args.font->Draw(
    args.text,
    args.pos + offset,
    args.size,
    args.color,
    args.alpha,
    additive);
}

void WidgetRenderer_DrawText(WidgetRenderer_DrawText_Args const& args) {
  WidgetRenderer_Flush();
  RendererState rs(BlendMode::Alpha, CullMode::Disabled, false, false);
  WidgetRenderer_DrawTextGeneric(args, false);
  WidgetRenderer_Flush();
}
static Function const WidgetRenderer_DrawText_Registration = Function_Bind(
  "WidgetRenderer_DrawText",
  "None",
  [](Font const& font, String const& text, V2 const& pos, float const& size, Color const& color, float const& alpha, bool const& centered) -> void { return WidgetRenderer_DrawText(font, text, pos, size, color, alpha, centered); },
  "font", "text", "pos", "size", "color", "alpha", "centered");
static int const WidgetRenderer_DrawText_Alias = Function_Alias("WidgetRenderer_DrawText", "DrawText");



void WidgetRenderer_DrawTextGlow(WidgetRenderer_DrawTextGlow_Args const& args) {
  WidgetRenderer_Flush();
  RendererState rs(BlendMode::Additive, CullMode::Disabled, false, false);
  WidgetRenderer_DrawTextGeneric(args, true);
  WidgetRenderer_Flush();
}
static Function const WidgetRenderer_DrawTextGlow_Registration = Function_Bind(
  "WidgetRenderer_DrawTextGlow",
  "None",
  [](Font const& font, String const& text, V2 const& pos, float const& size, Color const& color, float const& alpha, bool const& centered) -> void { return WidgetRenderer_DrawTextGlow(font, text, pos, size, color, alpha, centered); },
  "font", "text", "pos", "size", "color", "alpha", "centered");
static int const WidgetRenderer_DrawTextGlow_Alias = Function_Alias("WidgetRenderer_DrawTextGlow", "DrawTextGlow");



void WidgetRenderer_DrawTexture(Texture2D const& texture, V2 const& pos, V2 const& size, float const& alpha) {
  WidgetRenderer_Flush();
  RendererState rs(BlendMode::Alpha, CullMode::Disabled, false, false);
  (*renderer.textureShader)
    ("texture", texture)
    ("alpha", alpha);
  Renderer_SetShader(*renderer.textureShader);
  Renderer_DrawQuad(pos, pos + size);
}
static Function const WidgetRenderer_DrawTexture_Registration = Function_Bind(
  "WidgetRenderer_DrawTexture",
  "None",
  &WidgetRenderer_DrawTexture,
  "texture", "pos", "size", "alpha");
static int const WidgetRenderer_DrawTexture_Alias = Function_Alias("WidgetRenderer_DrawTexture", "Draw");



void WidgetRenderer_DrawTextureAdditive(Texture2D const& texture, V2 const& pos, V2 const& size, float const& alpha) {
  WidgetRenderer_Flush();
  RendererState rs(BlendMode::Additive, CullMode::Disabled, false, false);
  (*renderer.textureShaderAdditive)
    ("texture", texture)
    ("alpha", alpha);
  Renderer_SetShader(*renderer.textureShaderAdditive);
  Renderer_DrawQuad(pos, pos + size);
}
static Function const WidgetRenderer_DrawTextureAdditive_Registration = Function_Bind(
  "WidgetRenderer_DrawTextureAdditive",
  "None",
  &WidgetRenderer_DrawTextureAdditive,
  "texture", "pos", "size", "alpha");
static int const WidgetRenderer_DrawTextureAdditive_Alias = Function_Alias("WidgetRenderer_DrawTextureAdditive", "DrawAdditive");



void WidgetRenderer_Flush() {
  WidgetRenderer_Initialize();
  RendererState rs(
    BlendMode::Additive,
    CullMode::Disabled,
    false,
    false);

  for (GlyphMapT::iterator it = renderer.glyphs.begin();
       it != renderer.glyphs.end(); ++it)
  {
    FRAME(it->first->name) {
      RenderGlyphs(it->first, it->second);
      it->second.clear();
    }
  }
}
static Function const WidgetRenderer_Flush_Registration = Function_Bind(
  "WidgetRenderer_Flush",
  "None",
  &WidgetRenderer_Flush);
static int const WidgetRenderer_Flush_Alias = Function_Alias("WidgetRenderer_Flush", "Flush");



V2 WidgetRenderer_GetTextSize(Font const& font, String const& text, float const& size) {
  return font->GetTextSize(text, size);
}
static Function const WidgetRenderer_GetTextSize_Registration = Function_Bind(
  "WidgetRenderer_GetTextSize",
  "None",
  &WidgetRenderer_GetTextSize,
  "font", "text", "size");
static int const WidgetRenderer_GetTextSize_Alias = Function_Alias("WidgetRenderer_GetTextSize", "GetTextSize");


