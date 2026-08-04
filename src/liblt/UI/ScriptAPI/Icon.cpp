#include "LTE/Function.h"
#include "LTE/FunctionBind.h"

#include "UI/Icon.h"

TypeAlias(Reference<GlyphT>, Glyph);
TypeAlias(Reference<IconT>, Icon);

static Function const Icon_AddGlyph_Registration = Function_Bind(
  "Icon_AddGlyph",
  "Add 'glyph' to 'icon'",
  [](Icon const& icon, Glyph const& glyph)
  {
  icon->Add(glyph);
  },
  "icon", "glyph");
static int const Icon_AddGlyph_Alias = Function_Alias("Icon_AddGlyph", "+=");

static Function const Icon_AddIcon_Registration = Function_Bind(
  "Icon_AddIcon",
  "Add a copy of 'source' to 'icon' with translation 'offset' and scale 'scale'",
  [](Icon const& icon, Icon const& source, V2 const& offset, V2 const& scale)
  {
  icon->Add(source, offset, scale);
  },
  "icon", "source", "offset", "scale");
static int const Icon_AddIcon_Alias = Function_Alias("Icon_AddIcon", "+=");

static Function const Icon_Draw_Registration = Function_Bind(
  "Icon_Draw",
  "Draw 'icon' to the screen at 'center' with 'scale,' 'color,' and 'alpha'",
  [](Icon const& icon, V2 const& center, V2 const& scale, Color const& color, float const& alpha)
  {
  icon->Draw(GlyphState(center, scale, color, alpha));
  },
  "icon", "center", "scale", "color", "alpha");
static int const Icon_Draw_Alias = Function_Alias("Icon_Draw", "Draw");

static Function const Icon_Transform_Registration = Function_Bind(
  "Icon_Transform",
  "Scale 'icon' by 'scale,' then move by 'offset'",
  [](Icon const& icon, V2 const& offset, V2 const& scale)
  {
  icon->Transform(offset, scale);
  },
  "icon", "offset", "scale");
static int const Icon_Transform_Alias = Function_Alias("Icon_Transform", "Transform");
