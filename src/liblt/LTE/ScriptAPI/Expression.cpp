#include "LTE/Expression.h"
#include "LTE/Function.h"
#include "LTE/FunctionBind.h"
#include "LTE/StringList.h"

TypeAlias(Reference<ExpressionT>, Expression);

static Function const Expression_Compile_Registration = Function_Bind(
  "Expression_Compile",
  "Compile an expression from 'list'",
  [](StringList const& list) -> Expression
  {
  return Expression_Compile(list);
  },
  "list");
static int const Expression_Compile_Alias = Function_Alias("Expression_Compile", "Compile");
