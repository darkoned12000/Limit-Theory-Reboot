#include "Projects.h"

#include "LTE/Function.h"
#include "LTE/FunctionBind.h"

AutoClass(ProjectsIterator,
  Object, object,
  uint, index)
  ProjectsIterator() = default;
};

static Function const Object_GetProjects_Registration = Function_Bind(
  "Object_GetProjects",
  "Return an iterator to the projects managed by 'object'",
  [](Object const& object) -> ProjectsIterator
  {
  return ProjectsIterator(object, 0);
  },
  "object");
static int const Object_GetProjects_Alias = Function_Alias("Object_GetProjects", "GetProjects");

static Function const ProjectsIterator_Access_Registration = Function_Bind(
  "ProjectsIterator_Access",
  "Return the contents of 'iterator'",
  [](ProjectsIterator const& iterator) -> Project
  {
  return iterator.object->GetProjects()->elements[iterator.index];
  },
  "iterator");
static int const ProjectsIterator_Access_Alias = Function_Alias("ProjectsIterator_Access", "Get");

static Function const ProjectsIterator_Advance_Registration = Function_Bind(
  "ProjectsIterator_Advance",
  "Advance 'iterator'",
  [](ProjectsIterator const& iterator)
  {
  Mutable(iterator).index++;
  },
  "iterator");
static int const ProjectsIterator_Advance_Alias = Function_Alias("ProjectsIterator_Advance", "Advance");

static Function const ProjectsIterator_HasMore_Registration = Function_Bind(
  "ProjectsIterator_HasMore",
  "Return whether 'iterator' has more elements",
  [](ProjectsIterator const& iterator) -> bool
  {
  return 
    iterator.object->GetProjects() &&
    iterator.index < iterator.object->GetProjects()->elements.size();
  },
  "iterator");
static int const ProjectsIterator_HasMore_Alias = Function_Alias("ProjectsIterator_HasMore", "HasMore");

static Function const ProjectsIterator_Size_Registration = Function_Bind(
  "ProjectsIterator_Size",
  "Return the total number of elements in 'iterator'",
  [](ProjectsIterator const& iterator) -> int
  {
  return static_cast<int>(iterator.object->GetProjects()->elements.size());
  },
  "iterator");
static int const ProjectsIterator_Size_Alias = Function_Alias("ProjectsIterator_Size", "Size");
