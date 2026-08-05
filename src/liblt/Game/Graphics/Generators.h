#ifndef Generators_h__
#define Generators_h__

#include "Game/Common.h"
#include "LTE/Color.h"
#include "LTE/Common.h"
#include "LTE/Generic.h"
#include "LTE/V3.h"
#include "LTE/V4.h"
#include "LTE/AutoClass.h"

/* CubeMap. */
AutoClass(Generator_Blur_Args,
  Generic<CubeMap>, source,
  float, radius,
  size_t, resolution,
  size_t, samples)
  Generator_Blur_Args() {}
};

LT_API Generic<CubeMap> Generator_Blur(Generator_Blur_Args const& args);
inline Generic<CubeMap> Generator_Blur(
  Generic<CubeMap> const& source, float const& radius, size_t const& resolution,
  size_t const& samples) {
  return Generator_Blur(Generator_Blur_Args(source, radius, resolution, samples));
}

AutoClass(Generator_IRMap_Args,
  Generic<CubeMap>, source,
  size_t, samples)
  Generator_IRMap_Args() {}
};

LT_API Generic<CubeMap> Generator_IRMap(Generator_IRMap_Args const& args);
inline Generic<CubeMap> Generator_IRMap(
  Generic<CubeMap> const& source, size_t const& samples) {
  return Generator_IRMap(Generator_IRMap_Args(source, samples));
}

AutoClass(Generator_Nebula_Args,
  float, roughness,
  float, seed,
  Color, color1,
  Color, color2,
  V3, starDir,
  V4, offset)
  Generator_Nebula_Args() {}
};

LT_API Generic<CubeMap> Generator_Nebula(Generator_Nebula_Args const& args);
inline Generic<CubeMap> Generator_Nebula(
  float const& roughness, float const& seed, Color const& color1, Color const& color2,
  V3 const& starDir, V4 const& offset) {
  return Generator_Nebula(Generator_Nebula_Args(roughness, seed, color1, color2, starDir, offset));
}

LT_API Generic<CubeMap> Generator_PlanetSkybox(Planet const* planet);

LT_API Generic<CubeMap> Generator_PlanetSurface(uint seed);

/* Texture2D. */
LT_API Generic<Texture2D> Generator_LoadTexture2D(Location const& src);

LT_API Generic<Texture2D> Generator_NormalMap(
    Generic<Texture2D> const& source,
    float normalStrength = 0.1f);

LT_API Generic<Texture2D> Generator_ShaderInstance(
    size_t width,
    size_t height,
    ShaderInstance const& shader);

#endif
