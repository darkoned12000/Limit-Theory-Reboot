#include "UI/Widget.h"

#include "LTE/Color.h"
#include "LTE/Data.h"
#include "LTE/Function.h"
#include "LTE/FunctionBind.h"
#include "LTE/VectorNP.h"

#include "LTE/Debug.h"

TypeAlias(Reference<WidgetT>, Widget);

AutoClass(WidgetChildIterator,
  Widget, widget,
  size_t, index)
  WidgetChildIterator() = default;
};

namespace Children {
  static Function const Widget_GetChildren_Registration = Function_Bind(
  "Widget_GetChildren",
  "Return an iterator to the children of 'widget'",
  [](Widget const& widget) -> WidgetChildIterator
  {
    return WidgetChildIterator(widget, 0);
  
  },
  "widget");
static int const Widget_GetChildren_Alias = Function_Alias("Widget_GetChildren", "GetChildren");

  static Function const WidgetChildIterator_Access_Registration = Function_Bind(
  "WidgetChildIterator_Access",
  "Return the value of 'iterator'",
  [](WidgetChildIterator const& iterator) -> Widget
  {
    return iterator.widget->children[iterator.index];
  
  },
  "iterator");
static int const WidgetChildIterator_Access_Alias = Function_Alias("WidgetChildIterator_Access", "Get");

  static Function const WidgetChildIterator_Advance_Registration = Function_Bind(
  "WidgetChildIterator_Advance",
  "Advance 'iterator",
  [](WidgetChildIterator const& iterator)
  {
    Mutable(iterator).index++;
  
  },
  "iterator");
static int const WidgetChildIterator_Advance_Alias = Function_Alias("WidgetChildIterator_Advance", "Advance");

  static Function const WidgetChildIterator_HasMore_Registration = Function_Bind(
  "WidgetChildIterator_HasMore",
  "Return whether 'iterator' has more elements",
  [](WidgetChildIterator const& iterator) -> bool
  {
    return iterator.index < iterator.widget->children.size();
  
  },
  "iterator");
static int const WidgetChildIterator_HasMore_Alias = Function_Alias("WidgetChildIterator_HasMore", "HasMore");

  static Function const WidgetChildIterator_Size_Registration = Function_Bind(
  "WidgetChildIterator_Size",
  "Return the total number of elements in 'iterator'",
  [](WidgetChildIterator const& iterator) -> int
  {
    return static_cast<int>(iterator.widget->children.size());
  
  },
  "iterator");
static int const WidgetChildIterator_Size_Alias = Function_Alias("WidgetChildIterator_Size", "Size");
}

static Function const Widget_AddChild_Registration = Function_Bind(
  "Widget_AddChild",
  "Add 'child' to 'widget'",
  [](Widget const& widget, Widget const& child)
  {
  widget->AddChild(child);
  },
  "widget", "child");
static int const Widget_AddChild_Alias = Function_Alias("Widget_AddChild", "AddChild");

static Function const Widget_Center_Registration = Function_Bind(
  "Widget_Center",
  "Return the center of 'widget'",
  [](Widget const& widget) -> V2
  {
  return widget->pos + 0.5f * widget->size;
  },
  "widget");
static int const Widget_Center_Alias = Function_Alias("Widget_Center", "Center");

static Function const Widget_CreateChildren_Registration = Function_Bind(
  "Widget_CreateChildren",
  "Return a list of children created by 'widget'",
  [](Widget const& widget) -> ListNP
  {
  Vector<Widget> children;
  widget->CreateChildren(children);
  ListNP list = new ListNPT(Type_Get<Widget>());
  for (size_t i = 0; i < children.size(); ++i)
    list->Append(children[i]);
  return list;
  },
  "widget");
static int const Widget_CreateChildren_Alias = Function_Alias("Widget_CreateChildren", "CreateChildren");

static Function const Widget_Delete_Registration = Function_Bind(
  "Widget_Delete",
  "Mark 'widget' for deletion",
  [](Widget const& widget)
  {
  widget->deleted = true;
  widget->focusMouse = false;
  widget->focusKey = false;
  widget->focusScroll = false;
  },
  "widget");
static int const Widget_Delete_Alias = Function_Alias("Widget_Delete", "Delete");

static Function const Widget_DeleteAncestors_Registration = Function_Bind(
  "Widget_DeleteAncestors",
  "Delete 'widget' and all ancestors up to root",
  [](Widget const& widget)
  {
  WidgetT* w = widget.t;
  while (w && w->parent.t) {
    WidgetT* next = w->parent.t;
    w->deleted = true;
    w->focusMouse = false;
    w->focusKey = false;
    w->focusScroll = false;
    w = next;
  }
  },
  "widget");
static int const Widget_DeleteAncestors_Alias = Function_Alias("Widget_DeleteAncestors", "DeleteAncestors");

static Function const Widget_GetName_Registration = Function_Bind(
  "Widget_GetName",
  "Return the name of 'widget'",
  [](Widget const& widget) -> String
  {
  return widget->GetName();
  },
  "widget");
static int const Widget_GetName_Alias = Function_Alias("Widget_GetName", "GetName");

static Function const Widget_GetParent_Registration = Function_Bind(
  "Widget_GetParent",
  "Return the parent of 'widget'",
  [](Widget const& widget) -> Widget
  {
  return widget->parent.t;
  },
  "widget");
static int const Widget_GetParent_Alias = Function_Alias("Widget_GetParent", "GetParent");

#define X(x)                                                                   \
  static Function const Widget_HasFocus##x##Recursive_Registration =           \
    Function_Bind(                                                             \
      "Widget_HasFocus" #x "Recursive",                                        \
      "Return whether 'widget' or any of its children have " #x " focus",      \
      [](Widget const& widget) -> bool { return widget->HasFocus##x(); },      \
      "widget");                                                               \
  static int const Widget_HasFocus##x##Recursive_Alias = Function_Alias(       \
    "Widget_HasFocus" #x "Recursive", "HasFocus" #x "Recursive");
  WIDGET_FOCUS_X
#undef X

static Function const Widget_HasParent_Registration = Function_Bind(
  "Widget_HasParent",
  "Return whether 'widget' has a parent",
  [](Widget const& widget) -> bool
  {
  return widget->parent != nullptr;
  },
  "widget");
static int const Widget_HasParent_Alias = Function_Alias("Widget_HasParent", "HasParent");

static Function const Widget_Rebuild_Registration = Function_Bind(
  "Widget_Rebuild",
  "Force 'widget' to clear and re-create all children",
  [](Widget const& widget)
  {
  widget->Rebuild();
  },
  "widget");
static int const Widget_Rebuild_Alias = Function_Alias("Widget_Rebuild", "Rebuild");

static Function const Widget_Send_Registration = Function_Bind(
  "Widget_Send",
  "Send 'message' to all components of 'widget'",
  [](Widget const& widget, Data const& message)
  {
  widget->Send(message);
  },
  "widget", "message");
static int const Widget_Send_Alias = Function_Alias("Widget_Send", "Send");

static Function const Widget_SendDown_Registration = Function_Bind(
  "Widget_SendDown",
  "Send 'message' recursively to all children of 'widget'",
  [](Widget const& widget, Data const& message)
  {
  widget->SendDown(message);
  },
  "widget", "message");
static int const Widget_SendDown_Alias = Function_Alias("Widget_SendDown", "SendDown");

static Function const Widget_SendSiblings_Registration = Function_Bind(
  "Widget_SendSiblings",
  "Send 'message' to all siblings of 'widget'",
  [](Widget const& widget, Data const& message)
  {
  if (widget->parent) {
    for (size_t i = 0; i < widget->children.size(); ++i) {
      Widget const& child = widget->children[i];
      if (child != widget)
        child->Send(message);
    }
  }
  },
  "widget", "message");
static int const Widget_SendSiblings_Alias = Function_Alias("Widget_SendSiblings", "SendSiblings");

static Function const Widget_SendUp_Registration = Function_Bind(
  "Widget_SendUp",
  "Send 'message' recursively to all parents of 'widget'",
  [](Widget const& widget, Data const& message)
  {
  widget->SendUp(message);
  },
  "widget", "message");
static int const Widget_SendUp_Alias = Function_Alias("Widget_SendUp", "SendUp");

static Function const Widget_BottomCenter_Registration = Function_Bind(
  "Widget_BottomCenter",
  "Return the position of the bottom center of 'widget'",
  [](Widget const& widget) -> V2
  {
  return widget->pos + V2(0.5f * widget->size.x, widget->size.y);
  },
  "widget");
static int const Widget_BottomCenter_Alias = Function_Alias("Widget_BottomCenter", "BottomCenter");

static Function const Widget_BottomLeft_Registration = Function_Bind(
  "Widget_BottomLeft",
  "Return the position of the bottom left of 'widget'",
  [](Widget const& widget) -> V2
  {
  return widget->pos + V2(0, widget->size.y);
  },
  "widget");
static int const Widget_BottomLeft_Alias = Function_Alias("Widget_BottomLeft", "BottomLeft");

static Function const Widget_BottomRight_Registration = Function_Bind(
  "Widget_BottomRight",
  "Return the position of the bottom right of 'widget'",
  [](Widget const& widget) -> V2
  {
  return widget->pos + widget->size;
  },
  "widget");
static int const Widget_BottomRight_Alias = Function_Alias("Widget_BottomRight", "BottomRight");

static Function const Widget_GetPoint_Registration = Function_Bind(
  "Widget_GetPoint",
  "Return the position of the interpolant on 'widget'",
  [](Widget const& widget, float const& x, float const& y) -> V2
  {
  return widget->pos + V2(x, y) * widget->size;
  },
  "widget", "x", "y");
static int const Widget_GetPoint_Alias = Function_Alias("Widget_GetPoint", "GetPoint");

static Function const Widget_LeftCenter_Registration = Function_Bind(
  "Widget_LeftCenter",
  "Return the position of the left center of 'widget'",
  [](Widget const& widget) -> V2
  {
  return widget->pos + V2(0, 0.5f * widget->size.y);
  },
  "widget");
static int const Widget_LeftCenter_Alias = Function_Alias("Widget_LeftCenter", "LeftCenter");

static Function const Widget_RemoveChild_Registration = Function_Bind(
  "Widget_RemoveChild",
  "Remove 'child' from 'widget'",
  [](Widget const& widget, Widget const& child)
  {
  widget->RemoveChild(child);
  },
  "widget", "child");
static int const Widget_RemoveChild_Alias = Function_Alias("Widget_RemoveChild", "RemoveChild");

static Function const Widget_RightCenter_Registration = Function_Bind(
  "Widget_RightCenter",
  "Return the position of the right center of 'widget'",
  [](Widget const& widget) -> V2
  {
  return widget->pos + V2(widget->size.x, 0.5f * widget->size.y);
  },
  "widget");
static int const Widget_RightCenter_Alias = Function_Alias("Widget_RightCenter", "RightCenter");

static Function const Widget_TopCenter_Registration = Function_Bind(
  "Widget_TopCenter",
  "Return the position of the top center of 'widget'",
  [](Widget const& widget) -> V2
  {
  return widget->pos + V2(0.5f * widget->size.x, 0);
  },
  "widget");
static int const Widget_TopCenter_Alias = Function_Alias("Widget_TopCenter", "TopCenter");

static Function const Widget_TopLeft_Registration = Function_Bind(
  "Widget_TopLeft",
  "Return the position of the top left of 'widget'",
  [](Widget const& widget) -> V2
  {
  return widget->pos;
  },
  "widget");
static int const Widget_TopLeft_Alias = Function_Alias("Widget_TopLeft", "TopLeft");

static Function const Widget_TopRight_Registration = Function_Bind(
  "Widget_TopRight",
  "Return the position of the top right of 'widget'",
  [](Widget const& widget) -> V2
  {
  return widget->pos + V2(widget->size.x, 0);
  },
  "widget");
static int const Widget_TopRight_Alias = Function_Alias("Widget_TopRight", "TopRight");

static Function const Widget_Equal_Registration = Function_Bind(
  "Widget_Equal",
  "Return 'a' == 'b'",
  [](Widget const& a, Widget const& b) -> bool
  {
  return a == b;
  },
  "a", "b");
static int const Widget_Equal_Alias = Function_Alias("Widget_Equal", "==");

static Function const Widget_NotEqual_Registration = Function_Bind(
  "Widget_NotEqual",
  "Return 'a' != 'b'",
  [](Widget const& a, Widget const& b) -> bool
  {
  return a != b;
  },
  "a", "b");
static int const Widget_NotEqual_Alias = Function_Alias("Widget_NotEqual", "!=");
