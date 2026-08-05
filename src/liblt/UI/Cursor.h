#ifndef UI_Cursor_h__
#define UI_Cursor_h__

#include "LTE/V2.h"

LT_API V2 Cursor_Get();
LT_API V2 Cursor_GetDelta();
LT_API V2 Cursor_GetLast();

LT_API void Cursor_Pop();

LT_API void Cursor_Push(
  V2 const& pos, V2 const& posLast);

#endif
