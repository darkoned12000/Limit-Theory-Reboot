#include "LTE/Function.h"
#include "LTE/FunctionBind.h"
#include "LTE/StringTree.h"

TypeAlias(Reference<StringTreeT>, StringTree);

static Function const StringTree_Add_Registration = Function_Bind(
  "StringTree_Add",
  "Add 'child' to 'tree'",
  [](StringTree const& tree, StringTree const& child) -> StringTree
  {
  ((StringTree&)tree)->children.push(child);
  return tree;
  },
  "tree", "child");
static int const StringTree_Add_Alias = Function_Alias("StringTree_Add", "Add");

static Function const StringTree_Child_Registration = Function_Bind(
  "StringTree_Child",
  "Get the child at 'index' in 'tree'",
  [](StringTree const& tree, int const& index) -> StringTree
  {
  return tree->children[index];
  },
  "tree", "index");
static int const StringTree_Child_Alias = Function_Alias("StringTree_Child", "Child");

static Function const StringTree_Children_Registration = Function_Bind(
  "StringTree_Children",
  "Return the number of children in 'tree'",
  [](StringTree const& tree) -> int
  {
  return tree->children.size();
  },
  "tree");
static int const StringTree_Children_Alias = Function_Alias("StringTree_Children", "Children");

static Function const StringTree_Create_Registration = Function_Bind(
  "StringTree_Create",
  "Create a new StringTree by parsing 'contents'",
  [](String const& contents) -> StringTree
  {
  return StringTree_Create(contents);
  },
  "contents");

static Function const StringTree_Value_Registration = Function_Bind(
  "StringTree_Value",
  "Return the value contained in 'tree'",
  [](StringTree const& tree) -> String
  {
  return tree->value;
  },
  "tree");
static int const StringTree_Value_Alias = Function_Alias("StringTree_Value", "Value");
