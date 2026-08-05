#ifndef UI_ClipRegion_h__
#define UI_ClipRegion_h__

#include "Common.h"
#include "LTE/V2.h"

LT_API V2 ClipRegion_GetMin();
LT_API V2 ClipRegion_GetMax();

LT_API void ClipRegion_Pop();

LT_API void ClipRegion_Push(
  V2 const& pos, V2 const& size);

LT_API void ClipRegion_PushNoClip();

#endif
