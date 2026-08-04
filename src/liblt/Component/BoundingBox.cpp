#include "BoundingBox.h"
#include "Orientation.h"
#include "Cullable.h"
#include "Drawable.h"

#include "Game/Object.h"

#include "LTE/Matrix.h"
#include "LTE/Model.h"
#include "LTE/FunctionBind.h"

void ComponentBoundingBox::Recompute(ObjectT const* self) {
  ComponentOrientation const& orientation = *self->GetOrientation();
  ComponentDrawable const& drawable = *self->GetDrawable();

  if (drawable.renderable) {
    short newVersion = drawable.renderable()->GetVersion();
    if (modelVersion != newVersion) {
      modelVersion = newVersion;
      orientationVersion = -1;
    }
  }

  if (orientation.version != orientationVersion) {
    orientationVersion = orientation.version;

    /* TODO : Cleaner way than extracting matrix from frame? */
    worldBox = self->GetLocalBound();
    worldBox = worldBox.GetTransformed(orientation.transform.GetMatrix());
    radius = worldBox.GetRadius();

    /* Recompute the cull distance if necessary. */
    ComponentCullable const* cullable = self->GetCullable();
    if (cullable)
      cullable->Recompute(self);
  }
}

static Function const Object_GetBound_Registration = Function_Bind(
  "Object_GetBound",
  "Return the world-space bounding box of 'object'",
  [](Object const& object) -> Bound3D
  {
  return object->GetBoundingBox()->worldBox;
  },
  "object");
static int const Object_GetBound_Alias = Function_Alias("Object_GetBound", "GetBound");
