#ifndef LTE_SDFs_h__
#define LTE_SDFs_h__

#include "SDF.h"

LT_API SDF SDF_Add(
  SDF const& a, SDF const& b);

LT_API SDF SDF_Box(
  V3 const& center, V3 const& sides);

LT_API SDF SDF_Capsule(
  V3 const& p1, V3 const& p2, float const& radius);

LT_API SDF SDF_Cylinder(
  V3 const& center, V3 const& axis, float const& radius);

LT_API SDF SDF_FractalPerlin(
  V3 const& center, int const& octaves, float const& lac);

LT_API SDF SDF_FractalWorley(
  float const& seed, int const& octaves, float const& lac);

LT_API SDF SDF_Multiply(
  SDF const& source, float const& value);

LT_API SDF SDF_Polyhedron(
  Vector<Plane> const& planes, int const& shape);

LT_API SDF SDF_Radial(
  SDF const& source, float const& rMin, float const& rMax);

LT_API SDF SDF_Repeat(
  SDF const& source, V3 const& frequency, V3 const& spacing);

LT_API SDF SDF_Ring(
  V3 const& center, float const& radius, float const& thickness);

LT_API SDF SDF_RoundBox(
  V3 const& center, V3 const& sides, float const& radius);

LT_API SDF SDF_Scale(
  SDF const& source, V3 const& scale);

LT_API SDF SDF_Shell(
  V3 const& center, float const& radius, float const& thickness);

LT_API SDF SDF_Sphere(
  V3 const& center, float const& radius);

LT_API SDF SDF_Subtract(
  SDF const& a, SDF const& b, float const& sharpness);

LT_API SDF SDF_Torus(
  V3 const& center, float const& radius, float const& thickness);

LT_API SDF SDF_Translate(
  SDF const& source, V3 const& offset);

LT_API SDF SDF_Union(
  SDF const& a, SDF const& b, float const& sharpness);

LT_API SDF SDF_Wedge(
  V3 const& center, float const& angle, float const& angularExtent, float const& radius,
  float const& radialExtent, float const& height);

#endif
