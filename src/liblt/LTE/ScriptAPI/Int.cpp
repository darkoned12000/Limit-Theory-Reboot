#include "LTE/Function.h"
#include "LTE/FunctionBind.h"
#include "LTE/Math.h"

TypeAlias(signed int, int);
TypeAlias(signed int, Int);
TypeAlias(unsigned int, uint);
TypeAlias(unsigned int, Uint);
TypeAlias(int32, Int32);
TypeAlias(uint32, Uint32);
TypeAlias(int64, Int64);
TypeAlias(uint64, Uint64);

static void int_to_double_Impl(int const& src, double& dest) {
  dest = (double)src;
}
static int const int_to_double_Registration = Conversion_Bind<&int_to_double_Impl>();

static void int_to_float_Impl(int const& src, float& dest) {
  dest = (float)src;
}
static int const int_to_float_Registration = Conversion_Bind<&int_to_float_Impl>();

static void int_to_uint_Impl(int const& src, unsigned int& dest) {
  dest = (unsigned int)src;
}
static int const int_to_uint_Registration = Conversion_Bind<&int_to_uint_Impl>();

static void uint_to_int_Impl(unsigned int const& src, int& dest) {
  dest = (int)src;
}
static int const uint_to_int_Registration = Conversion_Bind<&uint_to_int_Impl>();

static void int_to_long_Impl(int const& src, long& dest) {
  dest = (long)src;
}
static int const int_to_long_Registration = Conversion_Bind<&int_to_long_Impl>();

static void int_to_llong_Impl(int const& src, long long& dest) {
  dest = (long long)src;
}
static int const int_to_llong_Registration = Conversion_Bind<&int_to_llong_Impl>();

static void int32_to_int_Impl(int32 const& src, int& dest) {
  dest = (int)src;
}
static int const int32_to_int_Registration = Conversion_Bind<&int32_to_int_Impl>();

static void int64_to_int_Impl(int64 const& src, int& dest) {
  dest = (int)src;
}
static int const int64_to_int_Registration = Conversion_Bind<&int64_to_int_Impl>();

static void int64_to_float_Impl(int64 const& src, float& dest) {
  dest = (float)src;
}
static int const int64_to_float_Registration = Conversion_Bind<&int64_to_float_Impl>();

static void int64_to_double_Impl(int64 const& src, double& dest) {
  dest = (double)src;
}
static int const int64_to_double_Registration = Conversion_Bind<&int64_to_double_Impl>();

static void int64_to_uint_Impl(int64 const& src, unsigned int& dest) {
  dest = (unsigned int)src;
}
static int const int64_to_uint_Registration = Conversion_Bind<&int64_to_uint_Impl>();

static Function const Int_Abs_Registration = Function_Bind(
  "Int_Abs",
  "Return the absolute value of 'i'",
  [](int const& i) -> int
  {
  return Abs(i);
  },
  "i");
static int const Int_Abs_Alias = Function_Alias("Int_Abs", "Abs");

static Function const Int_Add_Registration = Function_Bind(
  "Int_Add",
  "Return the sum of 'a' and 'b'",
  [](int const& a, int const& b) -> int
  {
  return a + b;
  },
  "a", "b");
static int const Int_Add_Alias = Function_Alias("Int_Add", "+");

static Function const Int_AddInPlace_Registration = Function_Bind(
  "Int_AddInPlace",
  "Add 'b' to 'a'",
  [](int const& a, int const& b)
  {
  (int&)a += b;
  },
  "a", "b");
static int const Int_AddInPlace_Alias = Function_Alias("Int_AddInPlace", "+=");

static Function const Int_Clamp_Registration = Function_Bind(
  "Int_Clamp",
  "Return 'i' clamped to the range [a, b]",
  [](int const& i, int const& a, int const& b) -> int
  {
  return Clamp(i, a, b);
  },
  "i", "a", "b");
static int const Int_Clamp_Alias = Function_Alias("Int_Clamp", "Clamp");

static Function const Int_Decrement_Registration = Function_Bind(
  "Int_Decrement",
  "Decrement the value of 'i' by 1",
  [](int const& i)
  {
  ((int&)i)--;
  },
  "i");
static int const Int_Decrement_Alias = Function_Alias("Int_Decrement", "--");

static Function const Int_Divide_Registration = Function_Bind(
  "Int_Divide",
  "Return 'a' divided by 'b' (as an int)",
  [](int const& a, int const& b) -> int
  {
  return a / b;
  },
  "a", "b");

static Function const Int_DivideFloat_Registration = Function_Bind(
  "Int_DivideFloat",
  "Return 'a' divided by 'b' (as a float)",
  [](int const& a, int const& b) -> float
  {
  return (float)((double)a / b);
  },
  "a", "b");
static int const Int_DivideFloat_Alias = Function_Alias("Int_DivideFloat", "/");

static Function const Int_Equal_Registration = Function_Bind(
  "Int_Equal",
  "Return whether 'a' is equal to 'b'",
  [](int const& a, int const& b) -> bool
  {
  return a == b;
  },
  "a", "b");
static int const Int_Equal_Alias = Function_Alias("Int_Equal", "==");

static Function const Int_Greater_Registration = Function_Bind(
  "Int_Greater",
  "Return whether 'a' is greater than 'b'",
  [](int const& a, int const& b) -> bool
  {
  return a > b;
  },
  "a", "b");
static int const Int_Greater_Alias = Function_Alias("Int_Greater", ">");

static Function const Int_GreaterEqual_Registration = Function_Bind(
  "Int_GreaterEqual",
  "Return whether 'a' is greater than or equal to 'b'",
  [](int const& a, int const& b) -> bool
  {
  return a >= b;
  },
  "a", "b");
static int const Int_GreaterEqual_Alias = Function_Alias("Int_GreaterEqual", ">=");

static Function const Int_Increment_Registration = Function_Bind(
  "Int_Increment",
  "Increment the value of 'i' by 1",
  [](int const& i)
  {
  ((int&)i)++;
  },
  "i");
static int const Int_Increment_Alias = Function_Alias("Int_Increment", "++");

static Function const Int_Less_Registration = Function_Bind(
  "Int_Less",
  "Return whether 'a' is less than 'b'",
  [](int const& a, int const& b) -> bool
  {
  return a < b;
  },
  "a", "b");
static int const Int_Less_Alias = Function_Alias("Int_Less", "<");

static Function const Int_LessEqual_Registration = Function_Bind(
  "Int_LessEqual",
  "Return whether 'a' is less than or equal to 'b'",
  [](int const& a, int const& b) -> bool
  {
  return a <= b;
  },
  "a", "b");
static int const Int_LessEqual_Alias = Function_Alias("Int_LessEqual", "<=");

static Function const Int_Max_Registration = Function_Bind(
  "Int_Max",
  "Return the max of 'a' and 'b'",
  [](int const& a, int const& b) -> int
  {
  return Max(a, b);
  },
  "a", "b");
static int const Int_Max_Alias = Function_Alias("Int_Max", "Max");

static Function const Int_Min_Registration = Function_Bind(
  "Int_Min",
  "Return the min of 'a' and 'b'",
  [](int const& a, int const& b) -> int
  {
  return Min(a, b);
  },
  "a", "b");
static int const Int_Min_Alias = Function_Alias("Int_Min", "Min");

static Function const Int_Mod_Registration = Function_Bind(
  "Int_Mod",
  "Return a modulo b",
  [](int const& a, int const& b) -> int
  {
  return a % b;
  },
  "a", "b");
static int const Int_Mod_Alias = Function_Alias("Int_Mod", "Mod");

static Function const Int_Mult_Registration = Function_Bind(
  "Int_Mult",
  "Return the product of 'a' and 'b'",
  [](int const& a, int const& b) -> int
  {
  return a * b;
  },
  "a", "b");
static int const Int_Mult_Alias = Function_Alias("Int_Mult", "*");

static Function const Int_MultInPlace_Registration = Function_Bind(
  "Int_MultInPlace",
  "Multiply 'a' by 'b'",
  [](int const& a, int const& b)
  {
  (int&)a *= b;
  },
  "a", "b");
static int const Int_MultInPlace_Alias = Function_Alias("Int_MultInPlace", "*=");

static Function const Int_NotEqual_Registration = Function_Bind(
  "Int_NotEqual",
  "Return whether 'a' is not equal to 'b'",
  [](int const& a, int const& b) -> bool
  {
  return a != b;
  },
  "a", "b");
static int const Int_NotEqual_Alias = Function_Alias("Int_NotEqual", "!=");

namespace Priv1 {
  static Function const Int_Random_Registration = Function_Bind(
  "Int_Random",
  "Return a random integer between 0 and the maximal integer value",
  []() -> int
  {
    return Rand32();
  
  });
}

namespace Priv2 {
  static Function const Int_Random_Registration = Function_Bind(
  "Int_Random",
  "Return a random integer from 0 to 'upper' - 1",
  [](int const& upper) -> int
  {
    return RandI(upper);
  
  },
  "upper");
}

namespace Priv3 {
  static Function const Int_Random_Registration = Function_Bind(
  "Int_Random",
  "Return a random integer from 'lower' to 'upper'",
  [](int const& lower, int const& upper) -> int
  {
    return RandI(lower, upper);
  
  },
  "lower", "upper");
}

static Function const Int_Sign_Registration = Function_Bind(
  "Int_Sign",
  "Return the sign of 'i'",
  [](int const& i) -> int
  {
  return i > 0 ? 1 : i == 0 ? 0 : -1;
  },
  "i");
static int const Int_Sign_Alias = Function_Alias("Int_Sign", "Sign");

static Function const Int_Subtract_Registration = Function_Bind(
  "Int_Subtract",
  "Return the difference of 'a' and 'b'",
  [](int const& a, int const& b) -> int
  {
  return a - b;
  },
  "a", "b");
static int const Int_Subtract_Alias = Function_Alias("Int_Subtract", "-");

static Function const Int_SubtractInPlace_Registration = Function_Bind(
  "Int_SubtractInPlace",
  "Subtract 'b' from 'a'",
  [](int const& a, int const& b)
  {
  (int&)a -= b;
  },
  "a", "b");
static int const Int_SubtractInPlace_Alias = Function_Alias("Int_SubtractInPlace", "-=");
