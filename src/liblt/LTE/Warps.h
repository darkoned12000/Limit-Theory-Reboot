#ifndef LTE_Warps_h__
#define LTE_Warps_h__

#include "V3.h"
#include "Warp.h"

LT_API Warp Warp_AttractorPlane(
  V3 const& center, V3 const& normal, float const& strength);

LT_API Warp Warp_AttractorPoint(
  V3 const& center, float const& strength);

LT_API Warp Warp_Custom(
  Data const& data);

#endif
