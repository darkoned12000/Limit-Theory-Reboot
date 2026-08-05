/* TODO : Unify V2, V3, V4 APIs */

#include "LTE/Data.h"
#include "LTE/Function.h"
#include "LTE/FunctionBind.h"
#include "LTE/V4.h"

TypeAlias(V4F, Vec4);
TypeAlias(V4F, Vec4f);
TypeAlias(V4D, Vec4d);

static void V4F_to_V4D_Impl(V4F const& src, V4D& dest) {
  dest = V4D(src);
}
static int const V4F_to_V4D_Registration = Conversion_Bind<&V4F_to_V4D_Impl>();

static void float_to_V4F_Impl(float const& src, V4F& dest) {
  dest = V4F(src);
}
static int const float_to_V4F_Registration = Conversion_Bind<&float_to_V4F_Impl>();

static void int_to_V4F_Impl(int const& src, V4F& dest) {
  dest = V4F(src);
}
static int const int_to_V4F_Registration = Conversion_Bind<&int_to_V4F_Impl>();

static Function const Vec4_Registration = Function_Bind(
  "Vec4",
  "Create a 4D vector ('x', 'y', 'z', 'w')",
  [](float const& x, float const& y, float const& z, float const& w) -> V4
  {
  return V4(x, y, z, w);
  },
  "x", "y", "z", "w");

static Function const Vec4f_Abs_Registration = Function_Bind(
  "Vec4f_Abs",
  "Return the component-wise absolute value of 'v'",
  [](V4F const& v) -> V4F
  {
  return Abs(v);
  },
  "v");
static int const Vec4f_Abs_Alias = Function_Alias("Vec4f_Abs", "Abs");

static Function const Vec4d_Abs_Registration = Function_Bind(
  "Vec4d_Abs",
  "Return the component-wise absolute value of 'v'",
  [](V4D const& v) -> V4D
  {
  return Abs(v);
  },
  "v");
static int const Vec4d_Abs_Alias = Function_Alias("Vec4d_Abs", "Abs");

static Function const Vec4f_Add_Registration = Function_Bind(
  "Vec4f_Add",
  "Return the sum of 'a' and 'b'",
  [](V4F const& a, V4F const& b) -> V4F
  {
  return a + b;
  },
  "a", "b");
static int const Vec4f_Add_Alias = Function_Alias("Vec4f_Add", "+");

static Function const Vec4f_AddInPlace_Registration = Function_Bind(
  "Vec4f_AddInPlace",
  "Add 'b' to 'a'",
  [](V4F const& a, V4F const& b)
  {
  Mutable(a) += b;
  },
  "a", "b");
static int const Vec4f_AddInPlace_Alias = Function_Alias("Vec4f_AddInPlace", "+=");

static Function const Vec4d_Add_Registration = Function_Bind(
  "Vec4d_Add",
  "Return the sum of 'a' and 'b'",
  [](V4D const& a, V4D const& b) -> V4D
  {
  return a + b;
  },
  "a", "b");
static int const Vec4d_Add_Alias = Function_Alias("Vec4d_Add", "+");

static Function const Vec4d_AddInPlace_Registration = Function_Bind(
  "Vec4d_AddInPlace",
  "Add 'b' to 'a'",
  [](V4D const& a, V4D const& b)
  {
  Mutable(a) += b;
  },
  "a", "b");
static int const Vec4d_AddInPlace_Alias = Function_Alias("Vec4d_AddInPlace", "+=");

static Function const Vec4_Clamp_Registration = Function_Bind(
  "Vec4_Clamp",
  "Return the component-wise clamp of 'v' and ['lower', 'upper']",
  [](V4 const& v, V4 const& lower, V4 const& upper) -> V4
  {
  return Clamp(v, lower, upper);
  },
  "v", "lower", "upper");
static int const Vec4_Clamp_Alias = Function_Alias("Vec4_Clamp", "Clamp");

static Function const Vec4_Distance_Registration = Function_Bind(
  "Vec4_Distance",
  "Return the distance between a 'a' and 'b'",
  [](V4 const& a, V4 const& b) -> float
  {
  return Length(a - b);
  },
  "a", "b");
static int const Vec4_Distance_Alias = Function_Alias("Vec4_Distance", "Distance");

static Function const Vec4f_Dot_Registration = Function_Bind(
  "Vec4f_Dot",
  "Return the dot product of 'a' and 'b'",
  [](V4F const& a, V4F const& b) -> float
  {
  return Dot(a, b);
  },
  "a", "b");
static int const Vec4f_Dot_Alias = Function_Alias("Vec4f_Dot", "Dot");

static Function const Vec4d_Dot_Registration = Function_Bind(
  "Vec4d_Dot",
  "Return the dot product of 'a' and 'b'",
  [](V4D const& a, V4D const& b) -> double
  {
  return Dot(a, b);
  },
  "a", "b");
static int const Vec4d_Dot_Alias = Function_Alias("Vec4d_Dot", "Dot");

static Function const Vec4_Floor_Registration = Function_Bind(
  "Vec4_Floor",
  "Return the component-wise floor of 'v'",
  [](V4 const& v) -> V4
  {
  return Floor(v);
  },
  "v");
static int const Vec4_Floor_Alias = Function_Alias("Vec4_Floor", "Floor");

static Function const Vec4f_Length_Registration = Function_Bind(
  "Vec4f_Length",
  "Return the length of 'v'",
  [](V4F const& v) -> float
  {
  return Length(v);
  },
  "v");
static int const Vec4f_Length_Alias = Function_Alias("Vec4f_Length", "Length");

static Function const Vec4d_Length_Registration = Function_Bind(
  "Vec4d_Length",
  "Return the length of 'v'",
  [](V4D const& v) -> double
  {
  return Length(v);
  },
  "v");
static int const Vec4d_Length_Alias = Function_Alias("Vec4d_Length", "Length");

static Function const Vec4_Max_Registration = Function_Bind(
  "Vec4_Max",
  "Return the component-wise max of 'a' and 'b'",
  [](V4 const& a, V4 const& b) -> V4
  {
  return Max(a, b);
  },
  "a", "b");
static int const Vec4_Max_Alias = Function_Alias("Vec4_Max", "Max");

static Function const Vec4_Min_Registration = Function_Bind(
  "Vec4_Min",
  "Return the component-wise min of 'a' and 'b'",
  [](V4 const& a, V4 const& b) -> V4
  {
  return Min(a, b);
  },
  "a", "b");
static int const Vec4_Min_Alias = Function_Alias("Vec4_Min", "Min");

static Function const Vec4_Mix_Registration = Function_Bind(
  "Vec4_Mix",
  "Return a linear interpolation of 'a' and 'b' with interpolant 't'",
  [](V4 const& a, V4 const& b, float const& t) -> V4
  {
  return Mix(a, b, t);
  },
  "a", "b", "t");
static int const Vec4_Mix_Alias = Function_Alias("Vec4_Mix", "Mix");

namespace Priv1 {
  static Function const Vec4f_Mult_Registration = Function_Bind(
  "Vec4f_Mult",
  "Return the product of 's' and 'vec'",
  [](float const& s, V4F const& v) -> V4F
  {
    return s * v;
  
  },
  "s", "v");

  static Function const Vec4d_Mult_Registration = Function_Bind(
  "Vec4d_Mult",
  "Return the product of 's' and 'vec'",
  [](double const& s, V4D const& v) -> V4D
  {
    return s * v;
  
  },
  "s", "v");
}

namespace Priv2 {
  static Function const Vec4f_Mult_Registration = Function_Bind(
  "Vec4f_Mult",
  "Return the product of 'a' and 'b'",
  [](V4F const& a, V4F const& b) -> V4F
  {
    return a * b;
  
  },
  "a", "b");

  static Function const Vec4d_Mult_Registration = Function_Bind(
  "Vec4d_Mult",
  "Return the product of 'a' and 'b'",
  [](V4D const& a, V4D const& b) -> V4D
  {
    return a * b;
  
  },
  "a", "b");
}

static int const Vec4f_Mult_Alias = Function_Alias("Vec4f_Mult", "*");

static int const Vec4d_Mult_Alias = Function_Alias("Vec4d_Mult", "*");

static Function const Vec4f_Normalize_Registration = Function_Bind(
  "Vec4f_Normalize",
  "Return a unit-length vector pointing in the same direction as 'vec'",
  [](V4F const& v) -> V4
  {
  return Normalize(v);
  },
  "v");
static int const Vec4f_Normalize_Alias = Function_Alias("Vec4f_Normalize", "Normalize");

static Function const Vec4d_Normalize_Registration = Function_Bind(
  "Vec4d_Normalize",
  "Return a unit-length vector pointing in the same direction as 'vec'",
  [](V4D const& v) -> V4
  {
  return Normalize(v);
  },
  "v");
static int const Vec4d_Normalize_Alias = Function_Alias("Vec4d_Normalize", "Normalize");

static Function const Vec4f_Pow_Registration = Function_Bind(
  "Vec4f_Pow",
  "Return the component-wise absolute value of 'v'",
  [](V4F const& v, V4F const& p) -> V4F
  {
  return Pow(v, p);
  },
  "v", "p");
static int const Vec4f_Pow_Alias = Function_Alias("Vec4f_Pow", "^");

static Function const Vec4f_Subtract_Registration = Function_Bind(
  "Vec4f_Subtract",
  "Return the difference of 'a' and 'b'",
  [](V4F const& a, V4F const& b) -> V4
  {
  return a - b;
  },
  "a", "b");
static int const Vec4f_Subtract_Alias = Function_Alias("Vec4f_Subtract", "-");

static Function const Vec4f_SubtractInPlace_Registration = Function_Bind(
  "Vec4f_SubtractInPlace",
  "Subtract 'b' from 'a'",
  [](V4F const& a, V4F const& b)
  {
  Mutable(a) -= b;
  },
  "a", "b");
static int const Vec4f_SubtractInPlace_Alias = Function_Alias("Vec4f_SubtractInPlace", "-=");

static Function const Vec4d_Subtract_Registration = Function_Bind(
  "Vec4d_Subtract",
  "Return the difference of 'a' and 'b'",
  [](V4D const& a, V4D const& b) -> V4D
  {
  return a - b;
  },
  "a", "b");
static int const Vec4d_Subtract_Alias = Function_Alias("Vec4d_Subtract", "-");

static Function const Vec4d_SubtractInPlace_Registration = Function_Bind(
  "Vec4d_SubtractInPlace",
  "Subtract 'b' from 'a'",
  [](V4D const& a, V4D const& b)
  {
  Mutable(a) -= b;
  },
  "a", "b");
static int const Vec4d_SubtractInPlace_Alias = Function_Alias("Vec4d_SubtractInPlace", "-=");

