#include "LTE/Function.h"
#include "LTE/FunctionBind.h"

DefineConversion(int_to_bool, int, bool) {
  dest = (src != 0);
}

DefineConversion(uint_to_bool, int, bool) {
  dest = (src != 0);
}

TypeAlias(bool, Bool);

static Function const Bool_And_Registration = Function_Bind(
  "Bool_And",
  "Return 'a' && 'b'",
  [](bool const& a, bool const& b) -> bool
  {
  return a && b;
  },
  "a", "b");
static int const Bool_And_Alias = Function_Alias("Bool_And", "&&");

static Function const Bool_Equal_Registration = Function_Bind(
  "Bool_Equal",
  "Return 'a' == 'b'",
  [](bool const& a, bool const& b) -> bool
  {
  return a == b;
  },
  "a", "b");
static int const Bool_Equal_Alias = Function_Alias("Bool_Equal", "==");

static Function const Bool_Not_Registration = Function_Bind(
  "Bool_Not",
  "Return !'b'",
  [](bool const& b) -> bool
  {
  return !b;
  },
  "b");
static int const Bool_Not_Alias = Function_Alias("Bool_Not", "!");

static Function const Bool_NotEqual_Registration = Function_Bind(
  "Bool_NotEqual",
  "Return 'a' != 'b'",
  [](bool const& a, bool const& b) -> bool
  {
  return a != b;
  },
  "a", "b");
static int const Bool_NotEqual_Alias = Function_Alias("Bool_NotEqual", "!=");

static Function const Bool_Or_Registration = Function_Bind(
  "Bool_Or",
  "Return 'a' || 'b'",
  [](bool const& a, bool const& b) -> bool
  {
  return a || b;
  },
  "a", "b");
static int const Bool_Or_Alias = Function_Alias("Bool_Or", "||");
