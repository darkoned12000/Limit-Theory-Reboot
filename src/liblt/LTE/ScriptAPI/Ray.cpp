#include "LTE/Ray.h"
#include "LTE/Function.h"
#include "LTE/FunctionBind.h"

TypeAlias(RayD, Ray);

static Function const Ray_GetPoint_Registration = Function_Bind(
  "Ray_GetPoint",
  "Return the point at 't' units along 'ray'",
  [](RayD const& ray, double const& t) -> V3D
  {
  return ray(t);
  },
  "ray", "t");
static int const Ray_GetPoint_Alias = Function_Alias("Ray_GetPoint", "GetPoint");
