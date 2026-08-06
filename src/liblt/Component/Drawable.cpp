#include "Drawable.h"
#include "Cullable.h"

#include "Game/Object.h"

#include "LTE/DrawState.h"
#include "LTE/Function.h"
#include "LTE/FunctionBind.h"
#include "LTE/RenderStyle.h"

void ComponentDrawable::Draw(ObjectT* self, DrawState* state) {
  if (!renderable)
    return;

  RenderStyle_Get()->SetTransform(self->GetTransform());
  state->lodLevel = lodLevel;
  renderable()->Render(state);
}

static Function const Object_SetRenderable_Registration = Function_Bind(
  "Object_SetRenderable",
  "Set 'object's renderable to 'renderable'",
  [](Object const& object, Renderable const& renderable)
  {
  object->GetDrawable()->renderable = renderable;
  },
  "object", "renderable");
static int const Object_SetRenderable_Alias = Function_Alias("Object_SetRenderable", "SetRenderable");
