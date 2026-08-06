#include "LTE/Function.h"
#include "LTE/FunctionBind.h"
#include "LTE/Grammar.h"
#include "LTE/RNG.h"

static Function const Grammar_Get_Registration = Function_Bind(
  "Grammar_Get",
  "Return the result of running the global grammar on 'text' using 'rng'",
  [](String const& text, RNG const& rng) -> String
  {
  return Grammar_Get()->Generate(rng, text, "");
  },
  "text", "rng");
