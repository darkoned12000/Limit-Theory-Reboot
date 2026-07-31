#include "Drawable.h"
#include "Cullable.h"

#include "Game/Object.h"

#include "LTE/DrawState.h"
#include "LTE/Function.h"
#include "LTE/RenderStyle.h"

void ComponentDrawable::Draw(ObjectT* self, DrawState* state) {
  if (!renderable)
    return;

  RenderStyle_Get()->SetTransform(self->GetTransform());
  state->lodLevel = lodLevel;
  renderable()->Render(state);
}

VoidFreeFunction(Object_SetRenderable,
  "Set 'object's renderable to 'renderable'",
  Object, object,
  Renderable, renderable)
{
  object->GetDrawable()->renderable = renderable;
} FunctionAlias(Object_SetRenderable, SetRenderable);
