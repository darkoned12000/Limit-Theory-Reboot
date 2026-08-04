#include "LTE/Function.h"
#include "LTE/FunctionBind.h"
#include "LTE/V2.h"

DefineConversion(float_to_V2F, float, V2F) {
  dest = V2F(src);
}

DefineConversion(int_to_V2F, int, V2F) {
  dest = V2F(src);
}

TypeAlias(V2, Vec2);

static Function const Vec2_Registration = Function_Bind(
  "Vec2",
  "Create a 2D vector ('x', 'y')",
  [](float const& x, float const& y) -> V2
  {
  return V2(x, y);
  },
  "x", "y");

static Function const Vec3_Distance_Registration = Function_Bind(
  "Vec3_Distance",
  "Return the distance between a 'a' and 'b'",
  [](V2 const& a, V2 const& b) -> float
  {
  return Length(a - b);
  },
  "a", "b");
static int const Vec2_Distance_Alias = Function_Alias("Vec2_Distance", "Distance");

static Function const Vec2_Float_Registration = Function_Bind(
  "Vec2_Float",
  "Create a 2D vector ('f', 'f')",
  [](float const& f) -> V2
  {
  return V2(f, f);
  },
  "f");
static int const Vec2_Float_Alias = Function_Alias("Vec2_Float", "Vec2");

static Function const Vec2_Abs_Registration = Function_Bind(
  "Vec2_Abs",
  "Return the component-wise absolute value of 'v'",
  [](V2 const& v) -> V2
  {
  return Abs(v);
  },
  "v");
static int const Vec2_Abs_Alias = Function_Alias("Vec2_Abs", "Abs");

static Function const Vec2_Add_Registration = Function_Bind(
  "Vec2_Add",
  "Return the sum of 'a' and 'b'",
  [](V2 const& a, V2 const& b) -> V2
  {
  return a + b;
  },
  "a", "b");
static int const Vec2_Add_Alias = Function_Alias("Vec2_Add", "+");

static Function const Vec2_AddInPlace_Registration = Function_Bind(
  "Vec2_AddInPlace",
  "Add 'b' to 'a'",
  [](V2 const& a, V2 const& b)
  {
  Mutable(a) += b;
  },
  "a", "b");
static int const Vec2_AddInPlace_Alias = Function_Alias("Vec2_AddInPlace", "+=");

static Function const Vec2_Clamp_Registration = Function_Bind(
  "Vec2_Clamp",
  "Return the component-wise clamp of 'v' and ['lower', 'upper']",
  [](V2 const& v, V2 const& lower, V2 const& upper) -> V2
  {
  return Clamp(v, lower, upper);
  },
  "v", "lower", "upper");
static int const Vec2_Clamp_Alias = Function_Alias("Vec2_Clamp", "Clamp");

static Function const Vec2_Dot_Registration = Function_Bind(
  "Vec2_Dot",
  "Return the dot product of 'a' and 'b'",
  [](V2 const& a, V2 const& b) -> float
  {
  return Dot(a, b);
  },
  "a", "b");
static int const Vec2_Dot_Alias = Function_Alias("Vec2_Dot", "Dot");

static Function const Vec2_Floor_Registration = Function_Bind(
  "Vec2_Floor",
  "Return the component-wise floor of 'v'",
  [](V2 const& v) -> V2
  {
  return Floor(v);
  },
  "v");
static int const Vec2_Floor_Alias = Function_Alias("Vec2_Floor", "Floor");

static Function const Vec2_Greater_Registration = Function_Bind(
  "Vec2_Greater",
  "Return whether each component of 'a' is greater than the component of 'b'",
  [](V2 const& a, V2 const& b) -> bool
  {
  return a > b;
  },
  "a", "b");
static int const Vec2_Greater_Alias = Function_Alias("Vec2_Greater", ">");

static Function const Vec2_GreaterEqual_Registration = Function_Bind(
  "Vec2_GreaterEqual",
  "Return whether each component of 'a' is greater or equal to the component of 'b'",
  [](V2 const& a, V2 const& b) -> bool
  {
  return a >= b;
  },
  "a", "b");
static int const Vec2_GreaterEqual_Alias = Function_Alias("Vec2_GreaterEqual", ">=");

static Function const Vec2_Length_Registration = Function_Bind(
  "Vec2_Length",
  "Return the length of 'v'",
  [](V2 const& v) -> float
  {
  return Length(v);
  },
  "v");
static int const Vec2_Length_Alias = Function_Alias("Vec2_Length", "Length");

static Function const Vec2_Less_Registration = Function_Bind(
  "Vec2_Less",
  "Return whether each component of 'a' is less than the component of 'b'",
  [](V2 const& a, V2 const& b) -> bool
  {
  return a < b;
  },
  "a", "b");
static int const Vec2_Less_Alias = Function_Alias("Vec2_Less", "<");

static Function const Vec2_LessEqual_Registration = Function_Bind(
  "Vec2_LessEqual",
  "Return whether each component of 'a' is less or equal to the component of 'b'",
  [](V2 const& a, V2 const& b) -> bool
  {
  return a <= b;
  },
  "a", "b");
static int const Vec2_LessEqual_Alias = Function_Alias("Vec2_LessEqual", "<=");

static Function const Vec2_Max_Registration = Function_Bind(
  "Vec2_Max",
  "Return the component-wise max of 'a' and 'b'",
  [](V2 const& a, V2 const& b) -> V2
  {
  return Max(a, b);
  },
  "a", "b");
static int const Vec2_Max_Alias = Function_Alias("Vec2_Max", "Max");

static Function const Vec2_Min_Registration = Function_Bind(
  "Vec2_Min",
  "Return the component-wise min of 'a' and 'b'",
  [](V2 const& a, V2 const& b) -> V2
  {
  return Min(a, b);
  },
  "a", "b");
static int const Vec2_Min_Alias = Function_Alias("Vec2_Min", "Min");

static Function const Vec2_MinComponent_Registration = Function_Bind(
  "Vec2_MinComponent",
  "Return the minimum component of 'v'",
  [](V2 const& v) -> float
  {
  return Min(v.x, v.y);
  },
  "v");
static int const Vec2_MinComponent_Alias = Function_Alias("Vec2_MinComponent", "MinComponent");

namespace Priv1 {
  static Function const Vec2_Divide_Registration = Function_Bind(
  "Vec2_Divide",
  "Return the 'vec' divided by 'f'",
  [](V2 const& v, float const& f) -> V2
  {
    return v / f;
  
  },
  "v", "f");

  static Function const Vec2_DivideInPlace_Registration = Function_Bind(
  "Vec2_DivideInPlace",
  "Divide 'v' by 'f'",
  [](V2 const& v, float const& f)
  {
    Mutable(v) /= f;
  
  },
  "v", "f");

  static Function const Vec2_Mix_Registration = Function_Bind(
  "Vec2_Mix",
  "Return the component-wise linear interpolation of 'a' and 'b' with interpolant 't'",
  [](V2 const& a, V2 const& b, float const& t) -> V2
  {
    return Mix(a, b, t);
  
  },
  "a", "b", "t");

  static Function const Vec2_Mult_Registration = Function_Bind(
  "Vec2_Mult",
  "Return the product of 'f' and 'v'",
  [](float const& f, V2 const& v) -> V2
  {
    return f * v;
  
  },
  "f", "v");

  static Function const Vec2_MultInPlace_Registration = Function_Bind(
  "Vec2_MultInPlace",
  "Multiply 'v' by 'f'",
  [](V2 const& v, float const& f)
  {
    Mutable(v) *= f;
  
  },
  "v", "f");
}

namespace Priv2 {
  static Function const Vec2_Divide_Registration = Function_Bind(
  "Vec2_Divide",
  "Return the dividend of 'a' and 'b'",
  [](V2 const& a, V2 const& b) -> V2
  {
    return a / b;
  
  },
  "a", "b");

  static Function const Vec2_DivideInPlace_Registration = Function_Bind(
  "Vec2_DivideInPlace",
  "Divide 'a' by 'b'",
  [](V2 const& a, V2 const& b)
  {
    Mutable(a) /= b;
  
  },
  "a", "b");

  static Function const Vec2_Mix_Registration = Function_Bind(
  "Vec2_Mix",
  "Return the component-wise linear interpolation of 'a' and 'b' with interpolant 'v'",
  [](V2 const& a, V2 const& b, V2 const& v) -> V2
  {
    return Mix(a, b, v);
  
  },
  "a", "b", "v"); 

  static Function const Vec2_Mult_Registration = Function_Bind(
  "Vec2_Mult",
  "Return the product of 'a' and 'b'",
  [](V2 const& a, V2 const& b) -> V2
  {
    return a * b;
  
  },
  "a", "b");

  static Function const Vec2_MultInPlace_Registration = Function_Bind(
  "Vec2_MultInPlace",
  "Multiply 'a' by 'b'",
  [](V2 const& a, V2 const& b)
  {
    Mutable(a) *= b;
  
  },
  "a", "b");
}


static int const Vec2_Divide_Alias = Function_Alias("Vec2_Divide", "/");

static int const Vec2_DivideInPlace_Alias = Function_Alias("Vec2_DivideInPlace", "/=");

static int const Vec2_Mix_Alias = Function_Alias("Vec2_Mix", "Mix");

static int const Vec2_Mult_Alias = Function_Alias("Vec2_Mult", "*");

static int const Vec2_MultInPlace_Alias = Function_Alias("Vec2_MultInPlace", "*=");

static Function const Vec2_Normalize_Registration = Function_Bind(
  "Vec2_Normalize",
  "Return a unit-length vector pointing in the same direction as 'vec'",
  [](V2 const& v) -> V2
  {
  return Normalize(v);
  },
  "v");
static int const Vec2_Normalize_Alias = Function_Alias("Vec2_Normalize", "Normalize");

static Function const Vec2_Round_Registration = Function_Bind(
  "Vec2_Round",
  "Return the component-wise round of 'v'",
  [](V2 const& v) -> V2
  {
  return Round(v);
  },
  "v");
static int const Vec2_Round_Alias = Function_Alias("Vec2_Round", "Round");

static Function const Vec2_Polar_Registration = Function_Bind(
  "Vec2_Polar",
  "Return the cartesian coordinates of the unit vector with 'angle'",
  [](float const& angle) -> V2
  {
  return V2(Cos(angle), Sin(angle));
  },
  "angle");
static int const Vec2_Polar_Alias = Function_Alias("Vec2_Polar", "Polar");

static Function const Vec2_Sign_Registration = Function_Bind(
  "Vec2_Sign",
  "Return the component-wise sign of 'v'",
  [](V2 const& v) -> V2
  {
  return Sign(v);
  },
  "v");
static int const Vec2_Sign_Alias = Function_Alias("Vec2_Sign", "Sign");

static Function const Vec2_Subtract_Registration = Function_Bind(
  "Vec2_Subtract",
  "Return the difference of 'a' and 'b'",
  [](V2 const& a, V2 const& b) -> V2
  {
  return a - b;
  },
  "a", "b");
static int const Vec2_Subtract_Alias = Function_Alias("Vec2_Subtract", "-");

static Function const Vec2_SubtractInPlace_Registration = Function_Bind(
  "Vec2_SubtractInPlace",
  "Subtract 'b' from 'a'",
  [](V2 const& a, V2 const& b)
  {
  Mutable(a) -= b;
  },
  "a", "b");
static int const Vec2_SubtractInPlace_Alias = Function_Alias("Vec2_SubtractInPlace", "-=");

