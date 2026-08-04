#include "LTE/Renderable.h"
#include "LTE/Function.h"
#include "LTE/FunctionBind.h"

static Function const Renderable_GetBound_Registration = Function_Bind(
  "Renderable_GetBound",
  "Return the local bounding box of 'renderable'",
  [](Renderable const& renderable) -> Bound3
  {
  return renderable->GetBound();
  },
  "renderable");
static int const Renderable_GetBound_Alias = Function_Alias("Renderable_GetBound", "GetBound");
