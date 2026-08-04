#include "LTE/Data.h"
#include "LTE/Function.h"
#include "LTE/FunctionBind.h"
#include "LTE/Location.h"
#include "LTE/Serializer.h"

static Function const Data_IsType_Registration = Function_Bind(
  "Data_IsType",
  "Return whether 'data' is of type 'type'",
  [](Data const& data, String const& type) -> bool
  {
  return data.type && data.type->name == type;
  },
  "data", "type");
static int const Data_IsType_Alias = Function_Alias("Data_IsType", "IsType");

static Function const Data_None_Registration = Function_Bind(
  "Data_None",
  "Return an empty piece of data",
  []() -> Data
  {
  return Data();
  });

static Function const Data_NotEmpty_Registration = Function_Bind(
  "Data_NotEmpty",
  "Return whether 'data' contains something",
  [](Data const& data) -> bool
  {
  return data.type;
  },
  "data");
static int const Data_NotEmpty_Alias = Function_Alias("Data_NotEmpty", "NotEmpty");

static Function const Data_IsNotNull_Registration = Function_Bind(
  "Data_IsNotNull",
  "Return whether 'data' is an object rather than a null pointer",
  [](Data const& data) -> bool
  {
  if (!data.type)
    return false;
  return !data.type->GetPointeeType() || *(void**)data.data;
  },
  "data");
static int const Data_IsNotNull_Alias = Function_Alias("Data_IsNotNull", "IsNotNull");

static Function const Data_IsNull_Registration = Function_Bind(
  "Data_IsNull",
  "Return whether 'data' is a null pointer",
  [](Data const& data) -> bool
  {
  if (!data.type)
    return true;
  return data.type->GetPointeeType() && *(void**)data.data == nullptr;
  },
  "data");
static int const Data_IsNull_Alias = Function_Alias("Data_IsNull", "IsNull");

static Function const Data_LoadFrom_Registration = Function_Bind(
  "Data_LoadFrom",
  "Load binary data from 'path'",
  [](String const& path) -> Data
  {
  Data data;
  LoadFrom(data, Location_File(path));
  return data;
  },
  "path");
static int const Data_LoadFrom_Alias = Function_Alias("Data_LoadFrom", "LoadFrom");

static Function const Data_SaveTo_Registration = Function_Bind(
  "Data_SaveTo",
  "Save 'data' in binary form to 'path'",
  [](Data const& data, String const& path)
  {
  SaveTo((Data&)data, Location_File(path), 0);
  },
  "data", "path");
static int const Data_SaveTo_Alias = Function_Alias("Data_SaveTo", "SaveTo");
