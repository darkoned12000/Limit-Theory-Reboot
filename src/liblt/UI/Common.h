#ifndef UI_Common_h__
#define UI_Common_h__

#include "LTE/Common.h"

struct CompositorT;
struct GlyphState;
struct GlyphT;
struct IconT;
struct InterfaceT;
struct WidgetT;
struct WidgetComponentT;

using Compositor = Reference<CompositorT>;
using Glyph = Reference<GlyphT>;
using Icon = Reference<IconT>;
using Interface = Reference<InterfaceT>;
using Widget = Reference<WidgetT>;
using WidgetComponent = Reference<WidgetComponentT>;

#define WIDGET_FOCUS_X                                                         \
  X(Key)                                                                       \
  X(Mouse)                                                                     \
  X(Scroll)

#define XTYPE FocusType
#define XLIST WIDGET_FOCUS_X
#include "LTE/XEnum.h"
#undef XTYPE
#undef XLIST

#endif
