#include "LTE/Function.h"
#include "LTE/FunctionBind.h"
#include "LTE/String.h"
#include "LTE/Vector.h"

DefineConversion(bool_to_string, bool, String) {
  dest = src ? "true" : "false";
}

DefineConversion(float_to_string, float, String) { dest = ToString<float>(src); }
DefineConversion(double_to_string, double, String) { dest = ToString<double>(src); }
DefineConversion(schar_to_string, signed char, String) { dest = ToString<signed char>(src); }
DefineConversion(uchar_to_string, unsigned char, String) { dest = ToString<unsigned char>(src); }
DefineConversion(sshort_to_string, signed short, String) { dest = ToString<signed short>(src); }
DefineConversion(ushort_to_string, unsigned short, String) { dest = ToString<unsigned short>(src); }
DefineConversion(sint_to_string, signed int, String) { dest = ToString<signed int>(src); }
DefineConversion(uint_to_string, unsigned int, String) { dest = ToString<unsigned int>(src); }
DefineConversion(slong_to_string, signed long, String) { dest = ToString<signed long>(src); }
DefineConversion(ulong_to_string, unsigned long, String) { dest = ToString<unsigned long>(src); }
DefineConversion(sllong_to_string, signed long long, String) { dest = ToString<signed long long>(src); }
DefineConversion(ullong_to_string, unsigned long long, String) { dest = ToString<unsigned long long>(src); }
DefineConversion(int32_to_string, int32, String) { dest = ToString<int32>(src); }
DefineConversion(int64_to_string, int64, String) { dest = ToString<int64>(src); }
DefineConversion(uint32_to_string, int32, String) { dest = ToString<uint32>(src); }
DefineConversion(uint64_to_string, int64, String) { dest = ToString<uint64>(src); }

static Function const String_Append_Registration = Function_Bind(
  "String_Append",
  "Append 'b' to 'a'",
  [](String const& a, String const& b)
  {
  (String&)a += b;
  },
  "a", "b");
static int const String_Append_Alias = Function_Alias("String_Append", "+=");

static Function const String_CapitalCase_Registration = Function_Bind(
  "String_CapitalCase",
  "Return a new version of 's' where each word is capitalized",
  [](String const& s) -> String
  {
  return String_Capital(s);
  },
  "s");
static int const String_CapitalCase_Alias = Function_Alias("String_CapitalCase", "CapitalCase");

static Function const String_Concat_Registration = Function_Bind(
  "String_Concat",
  "Return the concatenation of the strings 'a' and 'b'",
  [](String const& a, String const& b) -> String
  {
  return a + b;
  },
  "a", "b");
static int const String_Concat_Alias = Function_Alias("String_Concat", "+");

static Function const String_Contains_Registration = Function_Bind(
  "String_Contains",
  "Return whether 's' contains 'substring'",
  [](String const& s, String const& substring) -> bool
  {
  if (substring.size() == 0)
    return true;
  return s.contains(substring);
  },
  "s", "substring");
static int const String_Contains_Alias = Function_Alias("String_Contains", "Contains");

static Function const String_IsEmpty_Registration = Function_Bind(
  "String_IsEmpty",
  "Return whether 's' is of zero length",
  [](String const& s) -> bool
  {
  return s.size() == 0;
  },
  "s");
static int const String_IsEmpty_Alias = Function_Alias("String_IsEmpty", "IsEmpty");

static Function const String_Equal_Registration = Function_Bind(
  "String_Equal",
  "Return a == b",
  [](String const& a, String const& b) -> bool
  {
  return a == b;
  },
  "a", "b");
static int const String_Equal_Alias = Function_Alias("String_Equal", "==");

static Function const String_Get_Registration = Function_Bind(
  "String_Get",
  "Return the 'i'th character in 's'",
  [](String const& s, int const& i) -> char
  {
  return s[size_t(i)];
  },
  "s", "i");
static int const String_Get_Alias = Function_Alias("String_Get", "Get");

static Function const String_GetHash_Registration = Function_Bind(
  "String_GetHash",
  "Return a hash for 's'",
  [](String const& s) -> HashT
  {
  return String_Hash(s);
  },
  "s");
static int const String_GetHash_Alias = Function_Alias("String_GetHash", "GetHash");

static Function const String_Length_Registration = Function_Bind(
  "String_Length",
  "Return the number of characters in 's'",
  [](String const& s) -> int
  {
  return s.size();
  },
  "s");
static int const String_Length_Alias = Function_Alias("String_Length", "Length");

static Function const String_LowerCase_Registration = Function_Bind(
  "String_LowerCase",
  "Return a new version of 's' where each letter is lower case",
  [](String const& s) -> String
  {
  return String_Lower(s);
  },
  "s");
static int const String_LowerCase_Alias = Function_Alias("String_LowerCase", "LowerCase");

static Function const String_NotEqual_Registration = Function_Bind(
  "String_NotEqual",
  "Return a != b",
  [](String const& a, String const& b) -> bool
  {
  return a != b;
  },
  "a", "b");
static int const String_NotEqual_Alias = Function_Alias("String_NotEqual", "!=");

static Function const String_Pop_Registration = Function_Bind(
  "String_Pop",
  "Delete the last character of 's'",
  [](String const& s)
  {
  Mutable(s).pop();
  },
  "s");
static int const String_Pop_Alias = Function_Alias("String_Pop", "Pop");

static Function const String_Split_Registration = Function_Bind(
  "String_Split",
  "Split 's' into pieces using 'delimiter'",
  [](String const& s, char const& delimiter) -> Vector<String>
  {
  Vector<String> result;
  String_Split(result, s, delimiter);
  return result;
  },
  "s", "delimiter");
static int const String_Split_Alias = Function_Alias("String_Split", "Split");

static Function const String_SplitLines_Registration = Function_Bind(
  "String_SplitLines",
  "Split 's' into pieces using newline as a delimiter",
  [](String const& s) -> Vector<String>
  {
  Vector<String> result;
  String_Split(result, s, '\n');
  return result;
  },
  "s");
static int const String_SplitLines_Alias = Function_Alias("String_SplitLines", "SplitLines");

static Function const String_Substring_Registration = Function_Bind(
  "String_Substring",
  "Return the substring of 'length' from 's' starting at 'start'",
  [](String const& s, int const& start, int const& length) -> String
  {
  return s.substr(0, length);
  },
  "s", "start", "length");
static int const String_Substring_Alias = Function_Alias("String_Substring", "Substring");

static Function const String_ToFloat_Registration = Function_Bind(
  "String_ToFloat",
  "Return 's' converted to a float",
  [](String const& s) -> float
  {
  return FromString<float>(s);
  },
  "s");
static int const String_ToFloat_Alias = Function_Alias("String_ToFloat", "ToFloat");

static Function const String_ToInt_Registration = Function_Bind(
  "String_ToInt",
  "Return 's' converted to an int",
  [](String const& s) -> int
  {
  return FromString<int>(s);
  },
  "s");
static int const String_ToInt_Alias = Function_Alias("String_ToInt", "ToInt");

static Function const String_UpperCase_Registration = Function_Bind(
  "String_UpperCase",
  "Return a new version of 's' where each letter is upper case",
  [](String const& s) -> String
  {
  return String_Upper(s);
  },
  "s");
static int const String_UpperCase_Alias = Function_Alias("String_UpperCase", "UpperCase");
