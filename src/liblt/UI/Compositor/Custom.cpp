#include "../Compositors.h"

#include "LTE/Script.h"
#include "LTE/Texture2D.h"
#include "LTE/FunctionBind.h"

namespace {
  AutoClassDerived(CompositorCustom, CompositorT,
    Data, instance,
    ScriptFunction, composite,
    Compositor, base)

    CompositorCustom() = default;

    void Composite(Texture2D const& layer, Mesh const& surface) override {
      Texture2D result;
      composite->VoidCall(&result, instance, layer);
      base->Composite(result, surface);
    }

    void Update() override {
      base->Update();
    }
  };
}

Compositor Compositor_Custom(Compositor const& base, Data const& data) {
  Reference<CompositorCustom> self = new CompositorCustom;
  ScriptType type = data.type->GetAux().Convert<ScriptType>();
  self->instance = data;
  self->composite = type->GetFunction("Composite");
  self->base = base;
  return self;
}
static Function const Compositor_Custom_Registration = Function_Bind(
  "Compositor_Custom",
  "None",
  &Compositor_Custom,
  "base", "data");


