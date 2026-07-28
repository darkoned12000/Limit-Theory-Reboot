#include "../Objects.h"

#include "Component/BoundingBox.h"
#include "Component/Cullable.h"
#include "Component/Drawable.h"
#include "Component/Motion.h"
#include "Component/Nameable.h"
#include "Component/Orientation.h"
#include "Component/Scriptable.h"

#include "Game/Player.h"

#include "LTE/Script.h"

#include "UI/Widget.h"

using CustomBaseT = ObjectWrapper
  < Component_BoundingBox
  < Component_Drawable
  < Component_Cullable
  < Component_Motion
  < Component_Nameable
  < Component_Orientation
  < Component_Scriptable
  < ObjectWrapperTail<ObjectType_Custom>
  > > > > > > > >;

AutoClassDerived(ObjectCustom, CustomBaseT,
  Data, instance,
  ScriptFunction, getWidget,
  ScriptFunction, onCreate,
  ScriptFunction, onDestroy,
  ScriptFunction, onUpdate)
  DERIVED_TYPE_EX(ObjectCustom)

  ObjectCustom() = default;

  Widget GetWidget(Player const& self) override {
    if (getWidget) {
      Widget widget;
      getWidget->VoidCall(&widget, instance, (Object)this, self);
      return widget;
    }

    return BaseType::GetWidget(self);
  }

  void OnCreate() override {
    BaseType::OnCreate();
    if (onCreate)
      onCreate->VoidCall(0, instance, (Object)this);
  }

  void OnDestroy() override {
    BaseType::OnDestroy();
    if (onDestroy)
      onDestroy->VoidCall(0, instance, (Object)this);
  }

  void OnUpdate(UpdateState& state) override {
    BaseType::OnUpdate(state);
    if (onUpdate)
      onUpdate->VoidCall(0, instance, (Object)this);
  }
};

DERIVED_IMPLEMENT(ObjectCustom)

DefineFunction(Object_Custom) {
  Reference<ObjectCustom> self = new ObjectCustom;
  ScriptType const& type = args.data.type->GetAux().Convert<ScriptType>();
  self->instance = args.data;
  self->getWidget = type->GetFunction("GetWidget");
  self->onCreate = type->GetFunction("Create");
  self->onDestroy = type->GetFunction("Destroy");
  self->onUpdate = type->GetFunction("Update");
  return self;
}
