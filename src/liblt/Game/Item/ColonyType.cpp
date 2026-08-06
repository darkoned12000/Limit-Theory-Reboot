#include "../Items.h"

#include "Game/Attribute/Icon.h"
#include "Game/Attribute/Name.h"
#include "Game/Attribute/Task.h"
#include "Game/Attribute/Traits.h"

#include "LTE/Math.h"

#include "UI/Glyphs.h"
#include "LTE/FunctionBind.h"

using ColonyTypeBase = 
    Attribute_Icon
  < Attribute_Name
  < Attribute_Task
  < Attribute_Traits
  < ItemWrapper<ItemType_ColonyType>
  > > > >;

AutoClassDerivedEmpty(ColonyType, ColonyTypeBase)
  DERIVED_TYPE_EX(ColonyType)
};

DERIVED_IMPLEMENT(ColonyType)

Item Item_ColonyType(String const& name, Icon const& icon, Task const& task, Traits const& traits) {
  Reference<ColonyType> self = new ColonyType;
  self->name = name;
  self->icon = icon;
  self->task = task;
  self->traits = traits;
  return self;
}
static Function const Item_ColonyType_Registration = Function_Bind(
  "Item_ColonyType",
  "None",
  &Item_ColonyType,
  "name", "icon", "task", "traits");


