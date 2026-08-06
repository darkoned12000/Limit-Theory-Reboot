#include "ClipRegion.h"
#include "WidgetRenderer.h"

#include "LTE/Renderer.h"
#include "LTE/Vector.h"
#include "LTE/Viewport.h"
#include "LTE/FunctionBind.h"

namespace {
  AutoClass(ClipRegion,
    V2, lower,
    V2, upper)
    ClipRegion() = default;
  };

  Vector<ClipRegion>& GetStack() {
    static Vector<ClipRegion> stack;
    return stack;
  }
}

V2 ClipRegion_GetMin() {
  return GetStack().back().lower;
}
static Function const ClipRegion_GetMin_Registration = Function_Bind(
  "ClipRegion_GetMin",
  "None",
  &ClipRegion_GetMin);



V2 ClipRegion_GetMax() {
  return GetStack().back().upper;
}
static Function const ClipRegion_GetMax_Registration = Function_Bind(
  "ClipRegion_GetMax",
  "None",
  &ClipRegion_GetMax);



void ClipRegion_Pop() {
  WidgetRenderer_Flush();
  Renderer_PopScissor();
  GetStack().pop();
}
static Function const ClipRegion_Pop_Registration = Function_Bind(
  "ClipRegion_Pop",
  "None",
  &ClipRegion_Pop);



void ClipRegion_Push(V2 const& pos, V2 const& size) {
  WidgetRenderer_Flush();
  Viewport const& vp = Viewport_Get();
  V2 newPos = pos;
  V2 newSize = size;

  Vector<ClipRegion>& stack = GetStack();
  if (stack.size()) {
    V2 p1 = Max(stack.back().lower, newPos);
    V2 p2 = Min(stack.back().upper, newPos + newSize);
    newPos = p1;
    newSize = p2 - p1;
  }

  V2 posGL(newPos.x, vp->size.y - (newPos.y + newSize.y));
  Renderer_PushScissorOn(posGL * vp->resolution, newSize * vp->resolution);
  GetStack().push(ClipRegion(newPos, newPos + newSize));
}
static Function const ClipRegion_Push_Registration = Function_Bind(
  "ClipRegion_Push",
  "None",
  &ClipRegion_Push,
  "pos", "size");



void ClipRegion_PushNoClip() {
  WidgetRenderer_Flush();
  Renderer_PushScissorOff();
  Viewport const& vp = Viewport_Get();
  GetStack().push(ClipRegion(vp->position, vp->position + vp->size));
}
static Function const ClipRegion_PushNoClip_Registration = Function_Bind(
  "ClipRegion_PushNoClip",
  "None",
  &ClipRegion_PushNoClip);


