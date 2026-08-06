#include "LTE/Function.h"
#include "LTE/FunctionBind.h"
#include "UI/Interface.h"
#include "UI/Widget.h"

TypeAlias(Reference<InterfaceT>, Interface);

static Function const Interface_Add_Registration = Function_Bind(
  "Interface_Add",
  "Add 'widget' to 'interface'",
  [](Interface const& interface, Widget const& widget)
  {
  interface->Add(widget);
  },
  "interface", "widget");
static int const Interface_Add_Alias = Function_Alias("Interface_Add", "Add");

static Function const Interface_Clear_Registration = Function_Bind(
  "Interface_Clear",
  "Clear all widgets from 'interface'",
  [](Interface const& interface)
  {
  interface->Clear();
  },
  "interface");
static int const Interface_Clear_Alias = Function_Alias("Interface_Clear", "Clear");

static Function const Interface_Draw_Registration = Function_Bind(
  "Interface_Draw",
  "Draw all widgets in 'interface' to the screen",
  [](Interface const& interface)
  {
  interface->Draw();
  },
  "interface");
static int const Interface_Draw_Alias = Function_Alias("Interface_Draw", "Draw");

static Function const Interface_Update_Registration = Function_Bind(
  "Interface_Update",
  "Update all widgets in 'interface'",
  [](Interface const& interface)
  {
  interface->Update();
  },
  "interface");
static int const Interface_Update_Alias = Function_Alias("Interface_Update", "Update");
