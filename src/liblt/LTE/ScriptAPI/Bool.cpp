#include "LTE/Function.h"
#include "LTE/FunctionBind.h"

static void int_to_bool_Impl(int const& src, bool& dest) {
  dest = (src != 0);
}
static int const int_to_bool_Registration = Conversion_Bind<&int_to_bool_Impl>();

static void uint_to_bool_Impl(int const& src, bool& dest) {
  dest = (src != 0);
}
static int const uint_to_bool_Registration = Conversion_Bind<&uint_to_bool_Impl>();

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
