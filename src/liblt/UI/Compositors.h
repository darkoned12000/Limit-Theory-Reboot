#ifndef UI_Compositors_h__
#define UI_Compositors_h__

#include "Common.h"
#include "Compositor.h"

LT_API Compositor Compositor_None();

LT_API Compositor Compositor_Basic(
  float const& noise, float const& lines, V3 const& gradeBlue);

LT_API Compositor Compositor_Custom(
  Compositor const& base, Data const& data);

#endif
