#include "LTE/Data.h"
#include "LTE/Function.h"
#include "LTE/FunctionBind.h"
#include "LTE/Math.h"
#include "LTE/V3.h"

TypeAlias(V3F, Vec3);
TypeAlias(V3F, Vec3f);
TypeAlias(V3D, Vec3d);

DefineConversion(V3F_to_V3D, V3F, V3D) {
  dest = V3D(src);
}

DefineConversion(float_to_V3F, float, V3F) {
  dest = V3F(src);
}

DefineConversion(int_to_V3F, int, V3F) {
  dest = V3F(src);
}

static Function const Vec3_Create_Registration = Function_Bind(
  "Vec3_Create",
  "Create a 3D, single-precision vector ('x', 'y', 'z')",
  [](float const& x, float const& y, float const& z) -> V3
  {
  return V3(x, y, z);
  },
  "x", "y", "z");
static int const Vec3_Create_Alias = Function_Alias("Vec3_Create", "Vec3");

static Function const Vec3d_Create_Registration = Function_Bind(
  "Vec3d_Create",
  "Create a 3D, double-precision vector ('x', 'y', 'z')",
  [](float const& x, float const& y, float const& z) -> V3
  {
  return V3(x, y, z);
  },
  "x", "y", "z");
static int const Vec3d_Create_Alias = Function_Alias("Vec3d_Create", "Vec3d");

static Function const Vec3f_Abs_Registration = Function_Bind(
  "Vec3f_Abs",
  "Return the component-wise absolute value of 'v'",
  [](V3F const& v) -> V3F
  {
  return Abs(v);
  },
  "v");
static int const Vec3f_Abs_Alias = Function_Alias("Vec3f_Abs", "Abs");

static Function const Vec3d_Abs_Registration = Function_Bind(
  "Vec3d_Abs",
  "Return the component-wise absolute value of 'v'",
  [](V3D const& v) -> V3D
  {
  return Abs(v);
  },
  "v");
static int const Vec3d_Abs_Alias = Function_Alias("Vec3d_Abs", "Abs");

static Function const Vec3f_Add_Registration = Function_Bind(
  "Vec3f_Add",
  "Return the sum of 'a' and 'b'",
  [](V3F const& a, V3F const& b) -> V3F
  {
  return a + b;
  },
  "a", "b");
static int const Vec3f_Add_Alias = Function_Alias("Vec3f_Add", "+");

static Function const Vec3f_AddInPlace_Registration = Function_Bind(
  "Vec3f_AddInPlace",
  "Add 'b' to 'a'",
  [](V3F const& a, V3F const& b)
  {
  Mutable(a) += b;
  },
  "a", "b");
static int const Vec3f_AddInPlace_Alias = Function_Alias("Vec3f_AddInPlace", "+=");

static Function const Vec3d_Add_Registration = Function_Bind(
  "Vec3d_Add",
  "Return the sum of 'a' and 'b'",
  [](V3D const& a, V3D const& b) -> V3D
  {
  return a + b;
  },
  "a", "b");
static int const Vec3d_Add_Alias = Function_Alias("Vec3d_Add", "+");

static Function const Vec3d_AddInPlace_Registration = Function_Bind(
  "Vec3d_AddInPlace",
  "Add 'b' to 'a'",
  [](V3D const& a, V3D const& b)
  {
  Mutable(a) += b;
  },
  "a", "b");
static int const Vec3d_AddInPlace_Alias = Function_Alias("Vec3d_AddInPlace", "+=");

static Function const Vec3_Clamp_Registration = Function_Bind(
  "Vec3_Clamp",
  "Return the component-wise clamp of 'v' and ['lower', 'upper']",
  [](V3 const& v, V3 const& lower, V3 const& upper) -> V3
  {
  return Clamp(v, lower, upper);
  },
  "v", "lower", "upper");
static int const Vec3_Clamp_Alias = Function_Alias("Vec3_Clamp", "Clamp");

static Function const Vec3_Cross_Registration = Function_Bind(
  "Vec3_Cross",
  "Return the cross-product of 'a' and 'b'",
  [](V3 const& a, V3 const& b) -> V3
  {
  return Cross(a, b);
  },
  "a", "b");
static int const Vec3_Cross_Alias = Function_Alias("Vec3_Cross", "Cross");

static Function const Vec3_Cylinder_Registration = Function_Bind(
  "Vec3_Cylinder",
  "Return the cylindrical vector (r, theta, height) in cartesian coordinates",
  [](float const& radius, float const& angle, float const& height) -> V3
  {
  return V3(radius * Cos(angle), height, radius * Sin(angle));
  },
  "radius", "angle", "height");

static Function const Vec3_Distance_Registration = Function_Bind(
  "Vec3_Distance",
  "Return the distance between a 'a' and 'b'",
  [](V3 const& a, V3 const& b) -> float
  {
  return Length(a - b);
  },
  "a", "b");
static int const Vec3_Distance_Alias = Function_Alias("Vec3_Distance", "Distance");

namespace Priv1 {
  static Function const Vec3f_Div_Registration = Function_Bind(
  "Vec3f_Div",
  "Return v / s",
  [](V3F const& v, float const& s) -> V3F
  {
    return v / s;
  
  },
  "v", "s");

  static Function const Vec3d_Div_Registration = Function_Bind(
  "Vec3d_Div",
  "Return v / s",
  [](V3D const& v, double const& s) -> V3D
  {
    return v / s;
  
  },
  "v", "s");
}

namespace Priv2 {
  static Function const Vec3f_Div_Registration = Function_Bind(
  "Vec3f_Div",
  "Return the component-wise division of 'a' and 'b'",
  [](V3F const& a, V3F const& b) -> V3F
  {
    return a / b;
  
  },
  "a", "b");

  static Function const Vec3d_Div_Registration = Function_Bind(
  "Vec3d_Div",
  "Return the component-wise division of 'a' and 'b'",
  [](V3D const& a, V3D const& b) -> V3D
  {
    return a / b;
  
  },
  "a", "b");
}


static int const Vec3f_Div_Alias = Function_Alias("Vec3f_Div", "/");

static int const Vec3d_Div_Alias = Function_Alias("Vec3d_Div", "/");

static Function const Vec3f_Dot_Registration = Function_Bind(
  "Vec3f_Dot",
  "Return the dot product of 'a' and 'b'",
  [](V3F const& a, V3F const& b) -> float
  {
  return Dot(a, b);
  },
  "a", "b");
static int const Vec3f_Dot_Alias = Function_Alias("Vec3f_Dot", "Dot");

static Function const Vec3d_Dot_Registration = Function_Bind(
  "Vec3d_Dot",
  "Return the dot product of 'a' and 'b'",
  [](V3D const& a, V3D const& b) -> double
  {
  return Dot(a, b);
  },
  "a", "b");
static int const Vec3d_Dot_Alias = Function_Alias("Vec3d_Dot", "Dot");


static Function const Vec3_Floor_Registration = Function_Bind(
  "Vec3_Floor",
  "Return the component-wise floor of 'v'",
  [](V3 const& v) -> V3
  {
  return Floor(v);
  },
  "v");
static int const Vec3_Floor_Alias = Function_Alias("Vec3_Floor", "Floor");

static Function const Vec3f_Length_Registration = Function_Bind(
  "Vec3f_Length",
  "Return the length of 'v'",
  [](V3F const& v) -> float
  {
  return Length(v);
  },
  "v");
static int const Vec3f_Length_Alias = Function_Alias("Vec3f_Length", "Length");

static Function const Vec3d_Length_Registration = Function_Bind(
  "Vec3d_Length",
  "Return the length of 'v'",
  [](V3D const& v) -> double
  {
  return Length(v);
  },
  "v");
static int const Vec3d_Length_Alias = Function_Alias("Vec3d_Length", "Length");

static Function const Vec3_Max_Registration = Function_Bind(
  "Vec3_Max",
  "Return the component-wise max of 'a' and 'b'",
  [](V3 const& a, V3 const& b) -> V3
  {
  return Max(a, b);
  },
  "a", "b");
static int const Vec3_Max_Alias = Function_Alias("Vec3_Max", "Max");

static Function const Vec3_Min_Registration = Function_Bind(
  "Vec3_Min",
  "Return the component-wise min of 'a' and 'b'",
  [](V3 const& a, V3 const& b) -> V3
  {
  return Min(a, b);
  },
  "a", "b");
static int const Vec3_Min_Alias = Function_Alias("Vec3_Min", "Min");

static Function const Vec3f_Mix_Registration = Function_Bind(
  "Vec3f_Mix",
  "Return a linear interpolation of 'a' and 'b' with interpolant 't'",
  [](V3F const& a, V3F const& b, float const& t) -> V3F
  {
  return Mix(a, b, t);
  },
  "a", "b", "t");
static int const Vec3f_Mix_Alias = Function_Alias("Vec3f_Mix", "Mix");

static Function const Vec3d_Mix_Registration = Function_Bind(
  "Vec3d_Mix",
  "Return a linear interpolation of 'a' and 'b' with interpolant 't'",
  [](V3D const& a, V3D const& b, double const& t) -> V3D
  {
  return Mix(a, b, t);
  },
  "a", "b", "t");
static int const Vec3d_Mix_Alias = Function_Alias("Vec3d_Mix", "Mix");

namespace Priv1 {
  static Function const Vec3f_Mult_Registration = Function_Bind(
  "Vec3f_Mult",
  "Return the product of 's' and 'vec'",
  [](float const& s, V3F const& v) -> V3F
  {
    return s * v;
  
  },
  "s", "v");

  static Function const Vec3d_Mult_Registration = Function_Bind(
  "Vec3d_Mult",
  "Return the product of 's' and 'vec'",
  [](double const& s, V3D const& v) -> V3D
  {
    return s * v;
  
  },
  "s", "v");
}

namespace Priv2 {
  static Function const Vec3f_Mult_Registration = Function_Bind(
  "Vec3f_Mult",
  "Return the product of 'a' and 'b'",
  [](V3F const& a, V3F const& b) -> V3F
  {
    return a * b;
  
  },
  "a", "b");

  static Function const Vec3d_Mult_Registration = Function_Bind(
  "Vec3d_Mult",
  "Return the product of 'a' and 'b'",
  [](V3D const& a, V3D const& b) -> V3D
  {
    return a * b;
  
  },
  "a", "b");
}


static int const Vec3f_Mult_Alias = Function_Alias("Vec3f_Mult", "*");

static int const Vec3d_Mult_Alias = Function_Alias("Vec3d_Mult", "*");

static Function const Vec3f_Normalize_Registration = Function_Bind(
  "Vec3f_Normalize",
  "Return a unit-length vector pointing in the same direction as 'vec'",
  [](V3F const& v) -> V3
  {
  return Normalize(v);
  },
  "v");
static int const Vec3f_Normalize_Alias = Function_Alias("Vec3f_Normalize", "Normalize");

static Function const Vec3d_Normalize_Registration = Function_Bind(
  "Vec3d_Normalize",
  "Return a unit-length vector pointing in the same direction as 'vec'",
  [](V3D const& v) -> V3D
  {
  return Normalize(v);
  },
  "v");
static int const Vec3d_Normalize_Alias = Function_Alias("Vec3d_Normalize", "Normalize");

static Function const Vec3f_Pow_Registration = Function_Bind(
  "Vec3f_Pow",
  "Return the component-wise absolute value of 'v'",
  [](V3F const& v, V3F const& p) -> V3F
  {
  return Pow(v, p);
  },
  "v", "p");
static int const Vec3f_Pow_Alias = Function_Alias("Vec3f_Pow", "^");

static Function const Vec3_Spherical_Registration = Function_Bind(
  "Vec3_Spherical",
  "Return the cartesian coordinates of the spherical vector (radius, pitch, yaw) in cartesian coordinates",
  [](float const& radius, float const& pitch, float const& yaw) -> V3
  {
  return Spherical(radius, yaw, pitch);
  },
  "radius", "pitch", "yaw");
static int const Vec3_Spherical_Alias = Function_Alias("Vec3_Spherical", "Spherical");

static Function const Vec3f_Subtract_Registration = Function_Bind(
  "Vec3f_Subtract",
  "Return the difference of 'a' and 'b'",
  [](V3F const& a, V3F const& b) -> V3
  {
  return a - b;
  },
  "a", "b");
static int const Vec3f_Subtract_Alias = Function_Alias("Vec3f_Subtract", "-");

static Function const Vec3f_SubtractInPlace_Registration = Function_Bind(
  "Vec3f_SubtractInPlace",
  "Subtract 'b' from 'a'",
  [](V3F const& a, V3F const& b)
  {
  Mutable(a) -= b;
  },
  "a", "b");
static int const Vec3f_SubtractInPlace_Alias = Function_Alias("Vec3f_SubtractInPlace", "-=");

static Function const Vec3d_Subtract_Registration = Function_Bind(
  "Vec3d_Subtract",
  "Return the difference of 'a' and 'b'",
  [](V3D const& a, V3D const& b) -> V3D
  {
  return a - b;
  },
  "a", "b");
static int const Vec3d_Subtract_Alias = Function_Alias("Vec3d_Subtract", "-");

static Function const Vec3d_SubtractInPlace_Registration = Function_Bind(
  "Vec3d_SubtractInPlace",
  "Subtract 'b' from 'a'",
  [](V3D const& a, V3D const& b)
  {
  Mutable(a) -= b;
  },
  "a", "b");
static int const Vec3d_SubtractInPlace_Alias = Function_Alias("Vec3d_SubtractInPlace", "-=");
