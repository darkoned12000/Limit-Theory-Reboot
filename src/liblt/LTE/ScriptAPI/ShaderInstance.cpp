#include "LTE/Function.h"
#include "LTE/FunctionBind.h"
#include "LTE/ShaderInstance.h"
#include "LTE/Texture2D.h"

TypeAlias(Reference<ShaderInstanceT>, ShaderInstance);

static Function const ShaderInstance_Clone_Registration = Function_Bind(
  "ShaderInstance_Clone",
  "Return a new copy of 'instance'",
  [](ShaderInstance const& instance) -> ShaderInstance
  {
  return instance->Clone();
  },
  "instance");
static int const ShaderInstance_Clone_Alias = Function_Alias("ShaderInstance_Clone", "Clone");

static Function const ShaderInstance_SetFloat_Registration = Function_Bind(
  "ShaderInstance_SetFloat",
  "Set the float variable 'name' in 'instance' to 'value'",
  [](ShaderInstance const& instance, String const& name, float const& value)
  {
  instance->SetFloat(name, value);
  },
  "instance", "name", "value");
static int const ShaderInstance_SetFloat_Alias = Function_Alias("ShaderInstance_SetFloat", "SetFloat");

static Function const ShaderInstance_SetInt_Registration = Function_Bind(
  "ShaderInstance_SetInt",
  "Set the int variable 'name' in 'instance' to 'value'",
  [](ShaderInstance const& instance, String const& name, int const& value)
  {
  instance->SetInt(name, value);
  },
  "instance", "name", "value");
static int const ShaderInstance_SetInt_Alias = Function_Alias("ShaderInstance_SetInt", "SetInt");

static Function const ShaderInstance_SetTexture2D_Registration = Function_Bind(
  "ShaderInstance_SetTexture2D",
  "Set the texture2d variable 'name' in 'instance' to 'value'",
  [](ShaderInstance const& instance, String const& name, Texture2D const& value)
  {
  instance->SetTexture2D(name, value);
  },
  "instance", "name", "value");
static int const ShaderInstance_SetTexture2D_Alias = Function_Alias("ShaderInstance_SetTexture2D", "SetTexture2D");

static Function const ShaderInstance_SetVec2_Registration = Function_Bind(
  "ShaderInstance_SetVec2",
  "Set the vec2 variable 'name' in 'instance' to 'value'",
  [](ShaderInstance const& instance, String const& name, V2 const& value)
  {
  instance->SetFloat2(name, value);
  },
  "instance", "name", "value");
static int const ShaderInstance_SetVec2_Alias = Function_Alias("ShaderInstance_SetVec2", "SetVec2");

static Function const ShaderInstance_SetVec3_Registration = Function_Bind(
  "ShaderInstance_SetVec3",
  "Set the vec3 variable 'name' in 'instance' to 'value'",
  [](ShaderInstance const& instance, String const& name, V3 const& value)
  {
  instance->SetFloat3(name, value);
  },
  "instance", "name", "value");
static int const ShaderInstance_SetVec3_Alias = Function_Alias("ShaderInstance_SetVec3", "SetVec3");

static Function const ShaderInstance_SetVec4_Registration = Function_Bind(
  "ShaderInstance_SetVec4",
  "Set the vec4 variable 'name' in 'instance' to 'value'",
  [](ShaderInstance const& instance, String const& name, V4 const& value)
  {
  instance->SetFloat4(name, value);
  },
  "instance", "name", "value");
static int const ShaderInstance_SetVec4_Alias = Function_Alias("ShaderInstance_SetVec4", "SetVec4");

namespace {
  static int const ShaderInstance_SetFloat_BlockAlias = Function_Alias(
    "ShaderInstance_SetFloat", "Set");
  static int const ShaderInstance_SetInt_BlockAlias = Function_Alias(
    "ShaderInstance_SetInt", "Set");
  static int const ShaderInstance_SetTexture2D_BlockAlias = Function_Alias(
    "ShaderInstance_SetTexture2D", "Set");
  static int const ShaderInstance_SetVec2_BlockAlias = Function_Alias(
    "ShaderInstance_SetVec2", "Set");
  static int const ShaderInstance_SetVec3_BlockAlias = Function_Alias(
    "ShaderInstance_SetVec3", "Set");
  static int const ShaderInstance_SetVec4_BlockAlias = Function_Alias(
    "ShaderInstance_SetVec4", "Set");
}
