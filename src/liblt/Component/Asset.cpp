#include "Asset.h"
#include "Assets.h"
#include "Info.h"

#include "LTE/StackFrame.h"
#include "LTE/FunctionBind.h"

void ComponentAsset::Run(ObjectT* self, UpdateState& state) { AUTO_FRAME;
  if (owner) {
    /* An asset should always be fully visible to its owner. */
    ComponentInfo* info = owner->GetInfo();
    if (info)
      info->Add(self, InfoLevel_Scan);

    /* Notify the owner if the asset is destroyed. */
    if (self->GetIntegrity() && !self->IsAlive()) {
      owner->AddLogMessage("Your " + self->GetName() + " has been destroyed");
      owner->RemoveAsset(self);
    }
  }
}

static Function const Object_GetOwner_Registration = Function_Bind(
  "Object_GetOwner",
  "Return the owner of 'object'",
  [](Object const& object) -> Player
  {
  return object->GetOwner();
  },
  "object");
static int const Object_GetOwner_Alias = Function_Alias("Object_GetOwner", "GetOwner");
