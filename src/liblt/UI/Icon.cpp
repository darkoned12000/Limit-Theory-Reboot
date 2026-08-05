#include "Icon.h"
#include "LTE/FunctionBind.h"

Icon Icon_Create() {
  return new IconT;
}
static Function const Icon_Create_Registration = Function_Bind(
  "Icon_Create",
  "None",
  &Icon_Create);
static int const Icon_Create_Alias = Function_Alias("Icon_Create", "Icon");


