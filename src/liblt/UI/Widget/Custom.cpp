#include "../Widgets.h"

#include "LTE/Pool.h"
#include "LTE/Script.h" 
#include "LTE/VectorNP.h"

#include "UI/Icon.h"
#include "UI/Widget.h"
#include "LTE/FunctionBind.h"

namespace {
  AutoClassDerived(WidgetCustom, WidgetComponentT,
    Data, instance,
    ScriptFunction, captureFocus,
    ScriptFunction, create,
    ScriptFunction, createChildren,
    ScriptFunction, destroy,
    ScriptFunction, getHash,
    ScriptFunction, getName,
    ScriptFunction, getPadding,
    ScriptFunction, postDraw,
    ScriptFunction, postPosition,
    ScriptFunction, postUpdate,
    ScriptFunction, preDraw,
    ScriptFunction, prePosition,
    ScriptFunction, preUpdate,
    ScriptFunction, receive)
    DERIVED_TYPE_EX(WidgetCustom)
    POOLED_TYPE

    WidgetCustom() = default;

    void CaptureFocus(Widget const& self) override {
      if (captureFocus)
        captureFocus->VoidCall(0, instance, self);
    }

    void Create(Widget const& self) override {
      if (create)
        create->VoidCall(0, instance, self);
    }

    void CreateChildren(Widget const& self, Vector<Widget>& children) override {
      if (createChildren) {
        ListNP childList;
        createChildren->VoidCall(&childList, instance, self);
        for (size_t i = 0; i < childList->size; ++i)
          children.push((*childList)[i].Convert<Widget>());
      }
    }

    void Destroy(Widget const& self) override {
      if (destroy)
        destroy->VoidCall(0, instance, self);
    }

    void GetHash(Widget const& self, HashT& out) override {
      if (getHash)
        getHash->VoidCall(&out, instance);
    }

    void GetName(Widget const& self, String& out) override {
      if (getName)
        getName->VoidCall(&out, instance);
    }

    void GetPadding(Widget const& self, V4& out) override {
      if (getPadding)
        getPadding->VoidCall(&out, instance);
    }

    void PostDraw(Widget const& self) override {
      if (postDraw)
        postDraw->VoidCall(0, instance, self);
    }

    void PostPosition(Widget const& self) override {
      if (postPosition)
        postPosition->VoidCall(0, instance, self);
    }

    void PostUpdate(Widget const& self) override {
      if (postUpdate)
        postUpdate->VoidCall(0, instance, self);
    }

    void PreDraw(Widget const& self) override {
      if (preDraw)
        preDraw->VoidCall(0, instance, self);
    }

    void PrePosition(Widget const& self) override {
      if (prePosition)
        prePosition->VoidCall(0, instance, self);
    }

    void PreUpdate(Widget const& self) override {
      if (preUpdate)
        preUpdate->VoidCall(0, instance, self);
    }

    void Receive(Widget const& self, Data const& data) override {
      if (receive) {
        receive->VoidCall(
          0, instance, self,
          DataRef(Type_Get<Data>(), (void*)&data));
      }
    }
  };
}

Widget Widget_Custom(Widget const& widget, Data const& data) {
  Reference<WidgetCustom> self = new WidgetCustom;
  ScriptType type = data.type->GetAux().Convert<ScriptType>();
  self->instance = data;
  self->captureFocus = type->GetFunction("CaptureFocus");
  self->create = type->GetFunction("Create");
  self->createChildren = type->GetFunction("CreateChildren");
  self->destroy = type->GetFunction("Destroy");
  self->getHash = type->GetFunction("GetHash");
  self->getName = type->GetFunction("GetName");
  self->getPadding = type->GetFunction("GetPadding");
  self->postDraw = type->GetFunction("PostDraw");
  self->postPosition = type->GetFunction("PostPosition");
  self->postUpdate = type->GetFunction("PostUpdate");
  self->preDraw = type->GetFunction("PreDraw");
  self->prePosition = type->GetFunction("PrePosition");
  self->preUpdate = type->GetFunction("PreUpdate");
  self->receive = type->GetFunction("Receive");
  widget->Add(self);
  return widget;
}
static Function const Widget_Custom_Registration = Function_Bind(
  "Widget_Custom",
  "None",
  &Widget_Custom,
  "widget", "data");
static int const Widget_Custom_Alias = Function_Alias("Widget_Custom", "Custom");


