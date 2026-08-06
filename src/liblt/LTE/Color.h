#ifndef LTE_Color_h__
#define LTE_Color_h__

#include "V3.h"

struct Color : public V3F {
  using BaseType = V3T<float>;

  Color() : V3F(0) {}
  Color(float rgb) : V3F(rgb) {}
  Color(float r, float g, float b) : V3F(r, g, b) {}
  Color(V3F const& rgb) : V3F(rgb) {}

  DefineMetadataInline(Color)
};

LT_API Color Desaturate(
  Color const& color, float const& amount);

LT_API float Luminance(
  Color const& color);

LT_API Color ToHSL(
  Color const& color);

LT_API Color ToRGB(
  Color const& color);

LT_API Color Color_FromWavelength(
  float const& wavelength);

const Color Color_Black = Color(0.0f);
const Color Color_White = Color(1.0f);

#endif
