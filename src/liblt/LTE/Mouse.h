#ifndef LTE_Mouse_h__
#define LTE_Mouse_h__

#include "Common.h"
#include "Enum.h"
#include "V2.h"

namespace LTE {
LT_API bool Mouse_DoubleClicked();
  LT_API bool Mouse_Down(MouseButton button);
  LT_API bool Mouse_Pressed(MouseButton button);
  LT_API bool Mouse_Released(MouseButton button);

LT_API V2 Mouse_GetDP();
LT_API int Mouse_GetDX();
LT_API int Mouse_GetDY();
LT_API float Mouse_GetIdleTime();
LT_API V2 Mouse_GetPos();
LT_API V2 Mouse_GetPosImmediate();
LT_API V2 Mouse_GetPosLast();
LT_API int Mouse_GetX();
LT_API int Mouse_GetY();

  LT_API float Mouse_GetDownTime(MouseButton button);
LT_API float Mouse_GetScrollDelta();

  LT_API void Mouse_SetPos(V2 const& v);
  LT_API void Mouse_SetPressed(MouseButton button, bool pressed);
  LT_API void Mouse_SetScrollDelta(float ds);

  LT_API void Mouse_Update();
  LT_API void Mouse_UpdatePos(V2I const& p);

LT_API bool Mouse_LeftDown();
LT_API bool Mouse_LeftPressed();
LT_API bool Mouse_LeftReleased();
LT_API bool Mouse_RightDown();
LT_API bool Mouse_RightPressed();
LT_API bool Mouse_RightReleased();
}

#endif
