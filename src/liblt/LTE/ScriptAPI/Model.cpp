#include "LTE/Function.h"
#include "LTE/FunctionBind.h"
#include "LTE/Model.h"
#include "LTE/ShaderInstance.h"

TypeAlias(Reference<ModelT>, Model);

static void model_to_renderable_Impl(Model const& src, Renderable& dest) {
  dest = (Renderable)src;
}
static int const model_to_renderable_Registration = Conversion_Bind<&model_to_renderable_Impl>();

static Function const Model_Create_Registration = Function_Bind(
  "Model_Create",
  "Create a new, empty model",
  []() -> Model
  {
  return Model_Create();
  });

static Function const Model_Add_Registration = Function_Bind(
  "Model_Add",
  "Add 'geometry' to 'model' with shader 'shaderInstance'",
  [](Model const& model, Geometry const& geometry, ShaderInstance const& shaderInstance)
  {
  model->Add(geometry, shaderInstance);
  },
  "model", "geometry", "shaderInstance");
static int const Model_Add_Alias = Function_Alias("Model_Add", "Add");
