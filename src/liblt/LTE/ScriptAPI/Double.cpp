#include "LTE/Function.h"
#include "LTE/FunctionBind.h"
#include "LTE/Math.h"

TypeAlias(double, Double);

static void float_to_double_Impl(float const& src, double& dest) {
  dest = (double)src;
}
static int const float_to_double_Registration = Conversion_Bind<&float_to_double_Impl>();

#if 0
static void double_to_int_Impl(double const& src, int& dest) {
  dest = (int)src;
}
static int const double_to_int_Registration = Conversion_Bind<&double_to_int_Impl>();

static Function const Double_Abs_Registration = Function_Bind(
  "Double_Abs",
  "Return the absolute value of 't'",
  [](double const& t) -> double
  {
  return Abs(t);
  },
  "t");
static int const Double_Abs_Alias = Function_Alias("Double_Abs", "Abs");

static Function const Double_Add_Registration = Function_Bind(
  "Double_Add",
  "Return the sum of 'a' and 'b'",
  [](double const& a, double const& b) -> double
  {
  return a + b;
  },
  "a", "b");
static int const Double_Add_Alias = Function_Alias("Double_Add", "+");

static Function const Double_AddInPlace_Registration = Function_Bind(
  "Double_AddInPlace",
  "Add 'b' to 'a'",
  [](double const& a, double const& b)
  {
  (double&)a += b;
  },
  "a", "b");
static int const Double_AddInPlace_Alias = Function_Alias("Double_AddInPlace", "+=");

static Function const Double_Atan_Registration = Function_Bind(
  "Double_Atan",
  "Return the quadrant-correct of 'y' / 'x'",
  [](double const& y, double const& x) -> double
  {
  return Atan(y, x);
  },
  "y", "x");
static int const Double_Atan_Alias = Function_Alias("Double_Atan", "Atan");

static Function const Double_Ceil_Registration = Function_Bind(
  "Double_Ceil",
  "Return 'f' rounded up to the nearest integer",
  [](double const& f) -> double
  {
  return Ceil(f);
  },
  "f");
static int const Double_Ceil_Alias = Function_Alias("Double_Ceil", "Ceil");

static Function const Double_Clamp_Registration = Function_Bind(
  "Double_Clamp",
  "Return 'f' clamped to the range [lower, upper]",
  [](double const& f, double const& lower, double const& upper) -> double
  {
  return Clamp(f, lower, upper);
  },
  "f", "lower", "upper");
static int const Double_Clamp_Alias = Function_Alias("Double_Clamp", "Clamp");

static Function const Double_Cos_Registration = Function_Bind(
  "Double_Cos",
  "Return the cosine of 'angle' (radians)",
  [](double const& angle) -> double
  {
  return Cos(angle);
  },
  "angle");
static int const Double_Cos_Alias = Function_Alias("Double_Cos", "Cos");

static Function const Double_Divide_Registration = Function_Bind(
  "Double_Divide",
  "Return the dividend of 'a' and 'b'",
  [](double const& a, double const& b) -> double
  {
  return a / b;
  },
  "a", "b");
static int const Double_Divide_Alias = Function_Alias("Double_Divide", "/");

static Function const Double_DivideInPlace_Registration = Function_Bind(
  "Double_DivideInPlace",
  "Divide 'b' by 'a'",
  [](double const& a, double const& b)
  {
  (double&)a /= b;
  },
  "a", "b");
static int const Double_DivideInPlace_Alias = Function_Alias("Double_DivideInPlace", "/=");

static Function const Double_Exp_Registration = Function_Bind(
  "Double_Exp",
  "Return the e raised to the 't' power",
  [](double const& t) -> double
  {
  return Exp(t);
  },
  "t");
static int const Double_Exp_Alias = Function_Alias("Double_Exp", "Exp");

static Function const Double_ExpDecay_Registration = Function_Bind(
  "Double_ExpDecay",
  "Return the e raised to the '-t / rate' power",
  [](double const& t, double const& rate) -> double
  {
  return Exp(-t / rate);
  },
  "t", "rate");
static int const Double_ExpDecay_Alias = Function_Alias("Double_ExpDecay", "ExpDecay");

static Function const Double_Fract_Registration = Function_Bind(
  "Double_Fract",
  "Return the fractional part of 't'",
  [](double const& t) -> double
  {
  return Fract(t);
  },
  "t");
static int const Double_Fract_Alias = Function_Alias("Double_Fract", "Fract");

static Function const Double_Double_Registration = Function_Bind(
  "Double_Double",
  "Construct a double from 'f'",
  [](double const& f) -> double
  {
  return f;
  },
  "f");
static int const Double_Double_Alias = Function_Alias("Double_Double", "Double");

static Function const Double_Int_Registration = Function_Bind(
  "Double_Int",
  "Convert 'i' into a double",
  [](int const& i) -> double
  {
  return (double)i;
  },
  "i");
static int const Double_Int_Alias = Function_Alias("Double_Int", "Double");

static Function const Double_Floor_Registration = Function_Bind(
  "Double_Floor",
  "Return 'f' rounded down to the nearest integer",
  [](double const& f) -> double
  {
  return Floor(f);
  },
  "f");
static int const Double_Floor_Alias = Function_Alias("Double_Floor", "Floor");

static Function const Double_Greater_Registration = Function_Bind(
  "Double_Greater",
  "Return a > b",
  [](double const& a, double const& b) -> bool
  {
  return a > b;
  },
  "a", "b");
static int const Double_Greater_Alias = Function_Alias("Double_Greater", ">");

static Function const Double_GreaterEqual_Registration = Function_Bind(
  "Double_GreaterEqual",
  "Return a >= b",
  [](double const& a, double const& b) -> bool
  {
  return a >= b;
  },
  "a", "b");
static int const Double_GreaterEqual_Alias = Function_Alias("Double_GreaterEqual", ">=");

static Function const Double_Less_Registration = Function_Bind(
  "Double_Less",
  "Return a < b",
  [](double const& a, double const& b) -> bool
  {
  return a < b;
  },
  "a", "b");
static int const Double_Less_Alias = Function_Alias("Double_Less", "<");

static Function const Double_LessEqual_Registration = Function_Bind(
  "Double_LessEqual",
  "Return a <= b",
  [](double const& a, double const& b) -> bool
  {
  return a <= b;
  },
  "a", "b");
static int const Double_LessEqual_Alias = Function_Alias("Double_LessEqual", "<=");

static Function const Double_Max_Registration = Function_Bind(
  "Double_Max",
  "Return the maximum of 'a' and 'b'",
  [](double const& a, double const& b) -> double
  {
  return Max(a, b);
  },
  "a", "b");
static int const Double_Max_Alias = Function_Alias("Double_Max", "Max");

static Function const Double_Mix_Registration = Function_Bind(
  "Double_Mix",
  "Return a linear interpolation 'a' and 'b' with interpolant 't'",
  [](double const& a, double const& b, double const& t) -> double
  {
  return Mix(a, b, t);
  },
  "a", "b", "t");
static int const Double_Mix_Alias = Function_Alias("Double_Mix", "Mix");

static Function const Double_Log_Registration = Function_Bind(
  "Double_Log",
  "Return the logarithm of 'f'",
  [](double const& f) -> double
  {
  return Log(f);
  },
  "f");
static int const Double_Log_Alias = Function_Alias("Double_Log", "Log");

static Function const Double_Min_Registration = Function_Bind(
  "Double_Min",
  "Return the minimum of 'a' and 'b'",
  [](double const& a, double const& b) -> double
  {
  return Min(a, b);
  },
  "a", "b");
static int const Double_Min_Alias = Function_Alias("Double_Min", "Min");

static Function const Double_Mod_Registration = Function_Bind(
  "Double_Mod",
  "Return the remaind of 'a' / 'b'",
  [](double const& a, double const& b) -> double
  {
  return Mod(a, b);
  },
  "a", "b");
static int const Double_Mod_Alias = Function_Alias("Double_Mod", "Mod");

static Function const Double_Mult_Registration = Function_Bind(
  "Double_Mult",
  "Return the product of 'a' and 'b'",
  [](double const& a, double const& b) -> double
  {
  return a * b;
  },
  "a", "b");
static int const Double_Mult_Alias = Function_Alias("Double_Mult", "*");

static Function const Double_MultInPlace_Registration = Function_Bind(
  "Double_MultInPlace",
  "Multiply 'a' by 'b'",
  [](double const& a, double const& b)
  {
  (double&)a *= b;
  },
  "a", "b");
static int const Double_MultInPlace_Alias = Function_Alias("Double_MultInPlace", "*=");

static Function const Double_Pow_Registration = Function_Bind(
  "Double_Pow",
  "Return 'a' raised to the 'b'",
  [](double const& a, double const& b) -> double
  {
  return Pow(a, b);
  },
  "a", "b");
static int const Double_Pow_Alias = Function_Alias("Double_Pow", "^");

static Function const Double_Pow2_Registration = Function_Bind(
  "Double_Pow2",
  "Return f to the 2nd power",
  [](double const& f) -> double
  {
  return f * f;
  },
  "f");
static int const Double_Pow2_Alias = Function_Alias("Double_Pow2", "Pow2");

static Function const Double_Pow4_Registration = Function_Bind(
  "Double_Pow4",
  "Return f to the 4th power",
  [](double const& f) -> double
  {
  double s = f * f;
  return s * s;
  },
  "f");
static int const Double_Pow4_Alias = Function_Alias("Double_Pow4", "Pow4");

static Function const Double_Random_Registration = Function_Bind(
  "Double_Random",
  "Return a random between 0 (inclusive) and 1 (exclusive)",
  []() -> double
  {
  return Rand();
  });

static Function const Double_Round_Registration = Function_Bind(
  "Double_Round",
  "Return 'f' rounded to the nearest integer",
  [](double const& f) -> double
  {
  return Round(f);
  },
  "f");
static int const Double_Round_Alias = Function_Alias("Double_Round", "Round");

static Function const Double_Saturate_Registration = Function_Bind(
  "Double_Saturate",
  "Return 'f' clamped to the range [0, 1]",
  [](double const& f) -> double
  {
  return Saturate(f);
  },
  "f");
static int const Double_Saturate_Alias = Function_Alias("Double_Saturate", "Saturate");

static Function const Double_Sign_Registration = Function_Bind(
  "Double_Sign",
  "Return the sign (-1, 0, or +1) of 'f'",
  [](double const& f) -> double
  {
  return Sign(f);
  },
  "f");
static int const Double_Sign_Alias = Function_Alias("Double_Sign", "Sign");

static Function const Double_Sin_Registration = Function_Bind(
  "Double_Sin",
  "Return the sine of 'angle' (radians)",
  [](double const& angle) -> double
  {
  return Sin(angle);
  },
  "angle");
static int const Double_Sin_Alias = Function_Alias("Double_Sin", "Sin");

static Function const Double_Sqrt_Registration = Function_Bind(
  "Double_Sqrt",
  "Return the square root of 'f'",
  [](double const& f) -> double
  {
  return Sqrt(f);
  },
  "f");
static int const Double_Sqrt_Alias = Function_Alias("Double_Sqrt", "Sqrt");

static Function const Double_Subtract_Registration = Function_Bind(
  "Double_Subtract",
  "Return the difference of 'a' and 'b'",
  [](double const& a, double const& b) -> double
  {
  return a - b;
  },
  "a", "b");
static int const Double_Subtract_Alias = Function_Alias("Double_Subtract", "-");

static Function const Double_SubtractInPlace_Registration = Function_Bind(
  "Double_SubtractInPlace",
  "Subtract 'b' from 'a'",
  [](double const& a, double const& b)
  {
  (double&)a -= b;
  },
  "a", "b");
static int const Double_SubtractInPlace_Alias = Function_Alias("Double_SubtractInPlace", "-=");

static Function const Double_Tan_Registration = Function_Bind(
  "Double_Tan",
  "Return the tangent of 'angle' (radians)",
  [](double const& angle) -> double
  {
  return Tan(angle);
  },
  "angle");
static int const Double_Tan_Alias = Function_Alias("Double_Tan", "Tan");
#endif
