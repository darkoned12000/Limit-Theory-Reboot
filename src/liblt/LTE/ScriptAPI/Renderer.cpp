#include "LTE/Function.h"
#include "LTE/FunctionBind.h"
#include "LTE/Renderer.h"

static Function const Renderer_GetDrawCallCount_Registration = Function_Bind(
  "Renderer_GetDrawCallCount",
  "Return the number of renderer draw calls dispatched this frame",
  []() -> int
  {
  return Renderer_GetDrawCallCount();
  });

static Function const Renderer_GetPolyCount_Registration = Function_Bind(
  "Renderer_GetPolyCount",
  "Return the number of polygons drawn this frame",
  []() -> int
  {
  return Renderer_GetPolyCount();
  });
