#include "Cursor.h"
#include "LTE/AutoClass.h"
#include "LTE/Vector.h"
#include "LTE/FunctionBind.h"

namespace {
  AutoClass(Cursor,
    V2, pos,
    V2, last)
    Cursor() = default;
  };

  Vector<Cursor>& GetStack() {
    static Vector<Cursor> stack;
    return stack;
  }
}

V2 Cursor_Get() {
  return GetStack().back().pos;
}
static Function const Cursor_Get_Registration = Function_Bind(
  "Cursor_Get",
  "None",
  &Cursor_Get);



V2 Cursor_GetDelta() {
  Cursor const& cursor = GetStack().back();
  return cursor.pos - cursor.last;
}
static Function const Cursor_GetDelta_Registration = Function_Bind(
  "Cursor_GetDelta",
  "None",
  &Cursor_GetDelta);



V2 Cursor_GetLast() {
  return GetStack().back().last;
}
static Function const Cursor_GetLast_Registration = Function_Bind(
  "Cursor_GetLast",
  "None",
  &Cursor_GetLast);



void Cursor_Pop() {
  GetStack().pop();
}
static Function const Cursor_Pop_Registration = Function_Bind(
  "Cursor_Pop",
  "None",
  &Cursor_Pop);



void Cursor_Push(V2 const& pos, V2 const& posLast) {
  GetStack().push(Cursor(pos, posLast));
}
static Function const Cursor_Push_Registration = Function_Bind(
  "Cursor_Push",
  "None",
  &Cursor_Push,
  "pos", "posLast");


