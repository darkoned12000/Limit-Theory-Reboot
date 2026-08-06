#include "UI/Glyph.h"

#include "LTE/Function.h"
#include "LTE/FunctionBind.h"

static Function const Glyph_Draw_Registration = Function_Bind(
  "Glyph_Draw",
  "Draw 'glyph' at 'center' with 'scale,' 'color,' and 'alpha'",
  [](Glyph const& glyph, V2 const& center, V2 const& scale, Color const& color, float const& alpha)
  {
  glyph->Draw(GlyphState(center, scale, color, alpha));
  },
  "glyph", "center", "scale", "color", "alpha");
static int const Glyph_Draw_Alias = Function_Alias("Glyph_Draw", "Draw");
