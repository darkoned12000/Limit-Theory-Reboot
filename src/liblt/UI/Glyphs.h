#ifndef UI_Glyphs_h__
#define UI_Glyphs_h__

#include "Glyph.h"

#include "LTE/V3.h"
#include "LTE/AutoClass.h"

AutoClass(Glyph_Arc_Args,
  V2, position,
  float, radius,
  float, radiusS,
  Color, color,
  float, alpha,
  float, angle,
  float, angleS)
  Glyph_Arc_Args() {}
};

LT_API Glyph Glyph_Arc(Glyph_Arc_Args const& args);
inline Glyph Glyph_Arc(
  V2 const& position, float const& radius, float const& radiusS, Color const& color,
  float const& alpha, float const& angle, float const& angleS) {
  return Glyph_Arc(Glyph_Arc_Args(position, radius, radiusS, color, alpha, angle, angleS));
}

AutoClass(Glyph_Box_Args,
  V2, position,
  V2, size,
  Color, color,
  float, alpha)
  Glyph_Box_Args() {}
};

LT_API Glyph Glyph_Box(Glyph_Box_Args const& args);
inline Glyph Glyph_Box(
  V2 const& position, V2 const& size, Color const& color, float const& alpha) {
  return Glyph_Box(Glyph_Box_Args(position, size, color, alpha));
}

AutoClass(Glyph_Circle_Args,
  V2, position,
  float, radius,
  Color, color,
  float, alpha)
  Glyph_Circle_Args() {}
};

LT_API Glyph Glyph_Circle(Glyph_Circle_Args const& args);
inline Glyph Glyph_Circle(
  V2 const& position, float const& radius, Color const& color, float const& alpha) {
  return Glyph_Circle(Glyph_Circle_Args(position, radius, color, alpha));
}

AutoClass(Glyph_Line_Args,
  V2, p1,
  V2, p2,
  Color, color,
  float, alpha)
  Glyph_Line_Args() {}
};

LT_API Glyph Glyph_Line(Glyph_Line_Args const& args);
inline Glyph Glyph_Line(
  V2 const& p1, V2 const& p2, Color const& color, float const& alpha) {
  return Glyph_Line(Glyph_Line_Args(p1, p2, color, alpha));
}

AutoClass(Glyph_LineFade_Args,
  V2, p1,
  V2, p2,
  Color, color,
  float, alpha)
  Glyph_LineFade_Args() {}
};

LT_API Glyph Glyph_LineFade(Glyph_LineFade_Args const& args);
inline Glyph Glyph_LineFade(
  V2 const& p1, V2 const& p2, Color const& color, float const& alpha) {
  return Glyph_LineFade(Glyph_LineFade_Args(p1, p2, color, alpha));
}

AutoClass(Glyph_Gradient_Args,
  V2, position,
  V2, size,
  Color, color1,
  float, alpha1,
  Color, color2,
  float, alpha2)
  Glyph_Gradient_Args() {}
};

LT_API Glyph Glyph_Gradient(Glyph_Gradient_Args const& args);
inline Glyph Glyph_Gradient(
  V2 const& position, V2 const& size, Color const& color1, float const& alpha1,
  Color const& color2, float const& alpha2) {
  return Glyph_Gradient(Glyph_Gradient_Args(position, size, color1, alpha1, color2, alpha2));
}

AutoClass(Glyph_Grid_Args,
  V2, p1,
  V2, p2,
  Color, color,
  float, alpha,
  V2, offset,
  V2, scale)
  Glyph_Grid_Args() {}
};

LT_API Glyph Glyph_Grid(Glyph_Grid_Args const& args);
inline Glyph Glyph_Grid(
  V2 const& p1, V2 const& p2, Color const& color, float const& alpha, V2 const& offset,
  V2 const& scale) {
  return Glyph_Grid(Glyph_Grid_Args(p1, p2, color, alpha, offset, scale));
}

AutoClass(Glyph_Rect_Args,
  V2, position,
  V2, size,
  Color, color,
  float, alpha,
  float, bevel,
  float, variance)
  Glyph_Rect_Args() {}
};

LT_API Glyph Glyph_Rect(Glyph_Rect_Args const& args);
inline Glyph Glyph_Rect(
  V2 const& position, V2 const& size, Color const& color, float const& alpha,
  float const& bevel, float const& variance) {
  return Glyph_Rect(Glyph_Rect_Args(position, size, color, alpha, bevel, variance));
}

AutoClass(Glyph_Ring_Args,
  V2, position,
  float, radius,
  Color, color,
  float, alpha,
  float, angle)
  Glyph_Ring_Args() {}
};

LT_API Glyph Glyph_Ring(Glyph_Ring_Args const& args);
inline Glyph Glyph_Ring(
  V2 const& position, float const& radius, Color const& color, float const& alpha,
  float const& angle) {
  return Glyph_Ring(Glyph_Ring_Args(position, radius, color, alpha, angle));
}

AutoClass(Glyph_Triangle_Args,
  V2, p1,
  V2, p2,
  V2, p3,
  Color, color,
  float, alpha)
  Glyph_Triangle_Args() {}
};

LT_API Glyph Glyph_Triangle(Glyph_Triangle_Args const& args);
inline Glyph Glyph_Triangle(
  V2 const& p1, V2 const& p2, V2 const& p3, Color const& color, float const& alpha) {
  return Glyph_Triangle(Glyph_Triangle_Args(p1, p2, p3, color, alpha));
}

#endif
