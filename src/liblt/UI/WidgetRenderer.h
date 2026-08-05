#ifndef UI_WidgetRenderer_h__
#define UI_WidgetRenderer_h__

#include "Common.h"
#include "UI/Glyph.h"
#include "LTE/AutoClass.h"
#include "LTE/Font.h"
#include "LTE/Renderable.h"
#include "LTE/String.h"
#include "LTE/Texture2D.h"
#include "LTE/Transform.h"
#include "LTE/V2.h"

LT_API void WidgetRenderer_DrawGlyph(
  Glyph const& glyph, GlyphState const& state);

LT_API void WidgetRenderer_DrawPanel(
  V2 const& pos, V2 const& size, Color const& color, float const& innerAlpha,
  float const& alpha, float const& bevel);

LT_API void WidgetRenderer_DrawPanelRadial(
  V2 const& pos, float const& r1, float const& r2, float const& phase, float const& angle,
  Color const& color, float const& innerAlpha, float const& alpha, float const& bevel);

LT_API void WidgetRenderer_DrawRenderable(
  Renderable const& renderable, Transform const& transform, V3 const& camPos,
  V3 const& camLook, V3 const& camUp, float const& camFov, V2 const& pos, V2 const& size,
  Color const& color, float const& alpha, float const& time);

LT_API void WidgetRenderer_DrawTexture(
  Texture2D const& texture, V2 const& pos, V2 const& size, float const& alpha);

LT_API void WidgetRenderer_DrawTextureAdditive(
  Texture2D const& texture, V2 const& pos, V2 const& size, float const& alpha);

AutoClass(WidgetRenderer_DrawText_Args,
  Font, font,
  String, text,
  V2, pos,
  float, size,
  Color, color,
  float, alpha,
  bool, centered)
  WidgetRenderer_DrawText_Args() {}
};

LT_API void WidgetRenderer_DrawText(WidgetRenderer_DrawText_Args const& args);
inline void WidgetRenderer_DrawText(
  Font const& font, String const& text, V2 const& pos, float const& size,
  Color const& color, float const& alpha, bool const& centered) {
  return WidgetRenderer_DrawText(WidgetRenderer_DrawText_Args(font, text, pos, size, color, alpha, centered));
}

AutoClass(WidgetRenderer_DrawTextGlow_Args,
  Font, font,
  String, text,
  V2, pos,
  float, size,
  Color, color,
  float, alpha,
  bool, centered)
  WidgetRenderer_DrawTextGlow_Args() {}
};

LT_API void WidgetRenderer_DrawTextGlow(WidgetRenderer_DrawTextGlow_Args const& args);
inline void WidgetRenderer_DrawTextGlow(
  Font const& font, String const& text, V2 const& pos, float const& size,
  Color const& color, float const& alpha, bool const& centered) {
  return WidgetRenderer_DrawTextGlow(WidgetRenderer_DrawTextGlow_Args(font, text, pos, size, color, alpha, centered));
}

LT_API void WidgetRenderer_Flush();

LT_API V2 WidgetRenderer_GetTextSize(
  Font const& font, String const& text, float const& size);

#endif
