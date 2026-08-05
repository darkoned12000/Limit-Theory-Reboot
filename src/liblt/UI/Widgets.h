#ifndef UI_Widgets_h__
#define UI_Widgets_h__

#include "WidgetComponent.h"


LT_API Widget Widget_Custom(
  Widget const& widget, Data const& data);

LT_API Widget Widget_Dynamic(
  Widget const& widget);

LT_API Widget Widget_Layer(
  Compositor const& compositor, Mesh const& surface, float const& resolution,
  Widget const& widget);

LT_API Widget Widget_ListHorizontal(
  float const& spacing, Widget const& widget);

LT_API Widget Widget_ListVertical(
  float const& spacing, Widget const& widget);

LT_API Widget Widget_Rendered(
  Vector<RenderPass> const& passes);

LT_API Widget Widget_Stack(
  Widget const& widget);

#endif
