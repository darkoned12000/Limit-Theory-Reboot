#include "LTE/Function.h"
#include "LTE/FunctionBind.h"
#include "LTE/Math.h"

TypeAlias(float, Float);

static void double_to_float_Impl(double const& src, float& dest) {
  dest = (float)src;
}
static int const double_to_float_Registration = Conversion_Bind<&double_to_float_Impl>();

static void float_to_int_Impl(float const& src, int& dest) {
  dest = (int)src;
}
static int const float_to_int_Registration = Conversion_Bind<&float_to_int_Impl>();

static Function const Float_Abs_Registration = Function_Bind(
  "Float_Abs",
  "Return the absolute value of 't'",
  [](float const& t) -> float
  {
  return Abs(t);
  },
  "t");
static int const Float_Abs_Alias = Function_Alias("Float_Abs", "Abs");

static Function const Float_Add_Registration = Function_Bind(
  "Float_Add",
  "Return the sum of 'a' and 'b'",
  [](float const& a, float const& b) -> float
  {
  return a + b;
  },
  "a", "b");
static int const Float_Add_Alias = Function_Alias("Float_Add", "+");

static Function const Float_AddInPlace_Registration = Function_Bind(
  "Float_AddInPlace",
  "Add 'b' to 'a'",
  [](float const& a, float const& b)
  {
  (float&)a += b;
  },
  "a", "b");
static int const Float_AddInPlace_Alias = Function_Alias("Float_AddInPlace", "+=");

static Function const Float_Atan_Registration = Function_Bind(
  "Float_Atan",
  "Return the quadrant-correct atan of 'y' / 'x'",
  [](float const& y, float const& x) -> float
  {
  return Atan(y, x);
  },
  "y", "x");
static int const Float_Atan_Alias = Function_Alias("Float_Atan", "Atan");

static Function const Float_Ceil_Registration = Function_Bind(
  "Float_Ceil",
  "Return 'f' rounded up to the nearest integer",
  [](float const& f) -> float
  {
  return Ceil(f);
  },
  "f");
static int const Float_Ceil_Alias = Function_Alias("Float_Ceil", "Ceil");

static Function const Float_Clamp_Registration = Function_Bind(
  "Float_Clamp",
  "Return 'f' clamped to the range [lower, upper]",
  [](float const& f, float const& lower, float const& upper) -> float
  {
  return Clamp(f, lower, upper);
  },
  "f", "lower", "upper");
static int const Float_Clamp_Alias = Function_Alias("Float_Clamp", "Clamp");

static Function const Float_Cos_Registration = Function_Bind(
  "Float_Cos",
  "Return the cosine of 'angle' (radians)",
  [](float const& angle) -> float
  {
  return Cos(angle);
  },
  "angle");
static int const Float_Cos_Alias = Function_Alias("Float_Cos", "Cos");

static Function const Float_Divide_Registration = Function_Bind(
  "Float_Divide",
  "Return the dividend of 'a' and 'b'",
  [](float const& a, float const& b) -> float
  {
  return a / b;
  },
  "a", "b");
static int const Float_Divide_Alias = Function_Alias("Float_Divide", "/");

static Function const Float_DivideInPlace_Registration = Function_Bind(
  "Float_DivideInPlace",
  "Divide 'b' by 'a'",
  [](float const& a, float const& b)
  {
  (float&)a /= b;
  },
  "a", "b");
static int const Float_DivideInPlace_Alias = Function_Alias("Float_DivideInPlace", "/=");

static Function const Float_Exp_Registration = Function_Bind(
  "Float_Exp",
  "Return the e raised to the 't' power",
  [](float const& t) -> float
  {
  return Exp(t);
  },
  "t");
static int const Float_Exp_Alias = Function_Alias("Float_Exp", "Exp");

static Function const Float_ExpDecay_Registration = Function_Bind(
  "Float_ExpDecay",
  "Return the e raised to the '-t / rate' power",
  [](float const& t, float const& rate) -> float
  {
  return Exp(-t / rate);
  },
  "t", "rate");
static int const Float_ExpDecay_Alias = Function_Alias("Float_ExpDecay", "ExpDecay");

static Function const Float_Fract_Registration = Function_Bind(
  "Float_Fract",
  "Return the fractional part of 't'",
  [](float const& t) -> float
  {
  return Fract(t);
  },
  "t");
static int const Float_Fract_Alias = Function_Alias("Float_Fract", "Fract");

static Function const Float_Float_Registration = Function_Bind(
  "Float_Float",
  "Construct a float from 'f'",
  [](float const& f) -> float
  {
  return f;
  },
  "f");
static int const Float_Float_Alias = Function_Alias("Float_Float", "Float");

static Function const Float_Int_Registration = Function_Bind(
  "Float_Int",
  "Convert 'i' into a float",
  [](int const& i) -> float
  {
  return (float)i;
  },
  "i");
static int const Float_Int_Alias = Function_Alias("Float_Int", "Float");

static Function const Float_Floor_Registration = Function_Bind(
  "Float_Floor",
  "Return 'f' rounded down to the nearest integer",
  [](float const& f) -> float
  {
  return Floor(f);
  },
  "f");
static int const Float_Floor_Alias = Function_Alias("Float_Floor", "Floor");

static Function const Float_Greater_Registration = Function_Bind(
  "Float_Greater",
  "Return a > b",
  [](float const& a, float const& b) -> bool
  {
  return a > b;
  },
  "a", "b");
static int const Float_Greater_Alias = Function_Alias("Float_Greater", ">");

static Function const Float_GreaterEqual_Registration = Function_Bind(
  "Float_GreaterEqual",
  "Return a >= b",
  [](float const& a, float const& b) -> bool
  {
  return a >= b;
  },
  "a", "b");
static int const Float_GreaterEqual_Alias = Function_Alias("Float_GreaterEqual", ">=");

static Function const Float_Less_Registration = Function_Bind(
  "Float_Less",
  "Return a < b",
  [](float const& a, float const& b) -> bool
  {
  return a < b;
  },
  "a", "b");
static int const Float_Less_Alias = Function_Alias("Float_Less", "<");

static Function const Float_LessEqual_Registration = Function_Bind(
  "Float_LessEqual",
  "Return a <= b",
  [](float const& a, float const& b) -> bool
  {
  return a <= b;
  },
  "a", "b");
static int const Float_LessEqual_Alias = Function_Alias("Float_LessEqual", "<=");

static Function const Float_Max_Registration = Function_Bind(
  "Float_Max",
  "Return the maximum of 'a' and 'b'",
  [](float const& a, float const& b) -> float
  {
  return Max(a, b);
  },
  "a", "b");
static int const Float_Max_Alias = Function_Alias("Float_Max", "Max");

static Function const Float_Mix_Registration = Function_Bind(
  "Float_Mix",
  "Return a linear interpolation 'a' and 'b' with interpolant 't'",
  [](float const& a, float const& b, float const& t) -> float
  {
  return Mix(a, b, t);
  },
  "a", "b", "t");
static int const Float_Mix_Alias = Function_Alias("Float_Mix", "Mix");

static Function const Float_Log_Registration = Function_Bind(
  "Float_Log",
  "Return the logarithm of 'f'",
  [](float const& f) -> float
  {
  return Log(f);
  },
  "f");
static int const Float_Log_Alias = Function_Alias("Float_Log", "Log");

static Function const Float_Min_Registration = Function_Bind(
  "Float_Min",
  "Return the minimum of 'a' and 'b'",
  [](float const& a, float const& b) -> float
  {
  return Min(a, b);
  },
  "a", "b");
static int const Float_Min_Alias = Function_Alias("Float_Min", "Min");

static Function const Float_Mod_Registration = Function_Bind(
  "Float_Mod",
  "Return the remaind of 'a' / 'b'",
  [](float const& a, float const& b) -> float
  {
  return Mod(a, b);
  },
  "a", "b");
static int const Float_Mod_Alias = Function_Alias("Float_Mod", "Mod");

static Function const Float_Mult_Registration = Function_Bind(
  "Float_Mult",
  "Return the product of 'a' and 'b'",
  [](float const& a, float const& b) -> float
  {
  return a * b;
  },
  "a", "b");
static int const Float_Mult_Alias = Function_Alias("Float_Mult", "*");

static Function const Float_MultInPlace_Registration = Function_Bind(
  "Float_MultInPlace",
  "Multiply 'a' by 'b'",
  [](float const& a, float const& b)
  {
  (float&)a *= b;
  },
  "a", "b");
static int const Float_MultInPlace_Alias = Function_Alias("Float_MultInPlace", "*=");

static Function const Float_2Pi_Registration = Function_Bind(
  "Float_2Pi",
  "Return the value of 2 * pi",
  []() -> float
  {
  return kTau;
  });
static int const Float_2Pi_Alias = Function_Alias("Float_2Pi", "2Pi");

static Function const Float_Pi_Registration = Function_Bind(
  "Float_Pi",
  "Return the value of pi",
  []() -> float
  {
  return kPi;
  });
static int const Float_Pi_Alias = Function_Alias("Float_Pi", "Pi");

static Function const Float_Pi2_Registration = Function_Bind(
  "Float_Pi2",
  "Return the value of pi / 2",
  []() -> float
  {
  return kPi2;
  });
static int const Float_Pi2_Alias = Function_Alias("Float_Pi2", "Pi2");

static Function const Float_Pi4_Registration = Function_Bind(
  "Float_Pi4",
  "Return the value of pi / 4",
  []() -> float
  {
  return kPi4;
  });
static int const Float_Pi4_Alias = Function_Alias("Float_Pi4", "Pi4");

static Function const Float_Pi6_Registration = Function_Bind(
  "Float_Pi6",
  "Return the value of pi / 6",
  []() -> float
  {
  return kPi6;
  });
static int const Float_Pi6_Alias = Function_Alias("Float_Pi6", "Pi6");

static Function const Float_Pow_Registration = Function_Bind(
  "Float_Pow",
  "Return 'a' raised to the 'b'",
  [](float const& a, float const& b) -> float
  {
  return Pow(a, b);
  },
  "a", "b");
static int const Float_Pow_Alias = Function_Alias("Float_Pow", "^");

static Function const Float_Pow2_Registration = Function_Bind(
  "Float_Pow2",
  "Return f to the 2nd power",
  [](float const& f) -> float
  {
  return f * f;
  },
  "f");
static int const Float_Pow2_Alias = Function_Alias("Float_Pow2", "Pow2");

static Function const Float_Pow4_Registration = Function_Bind(
  "Float_Pow4",
  "Return f to the 4th power",
  [](float const& f) -> float
  {
  float s = f * f;
  return s * s;
  },
  "f");
static int const Float_Pow4_Alias = Function_Alias("Float_Pow4", "Pow4");

static Function const Float_Random_Registration = Function_Bind(
  "Float_Random",
  "Return a random between 0 (inclusive) and 1 (exclusive)",
  []() -> float
  {
  return Rand();
  });

static Function const Float_Round_Registration = Function_Bind(
  "Float_Round",
  "Return 'f' rounded to the nearest integer",
  [](float const& f) -> float
  {
  return Round(f);
  },
  "f");
static int const Float_Round_Alias = Function_Alias("Float_Round", "Round");

static Function const Float_Saturate_Registration = Function_Bind(
  "Float_Saturate",
  "Return 'f' clamped to the range [0, 1]",
  [](float const& f) -> float
  {
  return Saturate(f);
  },
  "f");
static int const Float_Saturate_Alias = Function_Alias("Float_Saturate", "Saturate");

static Function const Float_Sign_Registration = Function_Bind(
  "Float_Sign",
  "Return the sign (-1, 0, or +1) of 'f'",
  [](float const& f) -> float
  {
  return Sign(f);
  },
  "f");
static int const Float_Sign_Alias = Function_Alias("Float_Sign", "Sign");

static Function const Float_Sin_Registration = Function_Bind(
  "Float_Sin",
  "Return the sine of 'angle' (radians)",
  [](float const& angle) -> float
  {
  return Sin(angle);
  },
  "angle");
static int const Float_Sin_Alias = Function_Alias("Float_Sin", "Sin");

static Function const Float_Sqrt_Registration = Function_Bind(
  "Float_Sqrt",
  "Return the square root of 'f'",
  [](float const& f) -> float
  {
  return Sqrt(f);
  },
  "f");
static int const Float_Sqrt_Alias = Function_Alias("Float_Sqrt", "Sqrt");

static Function const Float_Subtract_Registration = Function_Bind(
  "Float_Subtract",
  "Return the difference of 'a' and 'b'",
  [](float const& a, float const& b) -> float
  {
  return a - b;
  },
  "a", "b");
static int const Float_Subtract_Alias = Function_Alias("Float_Subtract", "-");

static Function const Float_SubtractInPlace_Registration = Function_Bind(
  "Float_SubtractInPlace",
  "Subtract 'b' from 'a'",
  [](float const& a, float const& b)
  {
  (float&)a -= b;
  },
  "a", "b");
static int const Float_SubtractInPlace_Alias = Function_Alias("Float_SubtractInPlace", "-=");

static Function const Float_Tan_Registration = Function_Bind(
  "Float_Tan",
  "Return the tangent of 'angle' (radians)",
  [](float const& angle) -> float
  {
  return Tan(angle);
  },
  "angle");
static int const Float_Tan_Alias = Function_Alias("Float_Tan", "Tan");
