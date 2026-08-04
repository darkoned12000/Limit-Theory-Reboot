#include "Interior.h"
#include "Queryable.h"
#include "LTE/DrawState.h"
#include "LTE/Function.h"
#include "LTE/FunctionBind.h"
#include "LTE/Iterator.h"
#include "LTE/RenderStyle.h"
#include "LTE/StackFrame.h"
#include "LTE/Transform.h"

ComponentInterior::~ComponentInterior() {
  for (size_t i = 0; i < objects.size(); ++i)
    objects[i]->Delete();
  objects.clear();
  objectMap.clear();
}

void ComponentInterior::Add(ObjectT* self, Object const& object) {
  if (object->container == self)
    return;

  if (object->container)
    object->container->RemoveInterior(object);
  object->container = self;

  objects.push(object);
  objectMap[object->GetType()].push(object);
  object->OnCreate();
}

void ComponentInterior::Draw(ObjectT* self, DrawState* state) {
  if (state->visible[0] == self) {
    RenderStyle_Get()->SetTransform(Transform_Identity());
    self->OnDrawInterior(state);
  }
}

void ComponentInterior::Remove(ObjectT* self, Object const& object) {
  if (object->container == self) {
    object->container = nullptr;
    ComponentQueryable* qb = self->GetQueryable();
    if (qb)
      qb->Remove(object);
    objects.remove(object);
    objectMap[object->GetType()].remove(object);
    object->OnDestroy();
  }
}

void ComponentInterior::Run(ObjectT* self, UpdateState& state) {
  AUTO_FRAME;
  ParticleSystem_Push(particles);

  FRAME("Particle Update")
    particles->Run(state.dt);

  for (ObjectType type = 0; type < ObjectType_SIZE; ++type) {
    FRAME(ObjectType_String[type]) {
      if (!objectMap.contains(type))
        continue;

      Vector<Object>& objects = objectMap[type];
      for (int i = 0; i < static_cast<int>(objects.size()); ++i) {
        Object object = objects[i];
        if (!object->IsDeleted())
          object->Update(state);
        if (object->IsDeleted()) {
          objects.removeIndex(i);
          i--;
          continue;
        }
      }
    }
  }

  /* Cleanup deleted objects from global list. */
  for (int i = 0; i < static_cast<int>(objects.size()); ++i) {
    Object object = objects[i];
    if (object->IsDeleted()) {
      ComponentQueryable* qb = self->GetQueryable();
      if (qb)
        qb->Remove(object);
      objects.removeIndex(i);
      i--;
      continue;
    }
  }
  
  ParticleSystem_Pop(particles);
}

namespace {
  AutoClass(InteriorIterator,
    Object, object,
    size_t, index)
    InteriorIterator() = default;
  };

  static Function const Object_AddInterior_Registration = Function_Bind(
  "Object_AddInterior",
  "Add 'object' to the interior of 'interior'",
  [](Object const& interior, Object const& object)
  {
    interior->AddInterior(object);
  
  },
  "interior", "object");
static int const Object_AddInterior_Alias = Function_Alias("Object_AddInterior", "AddInterior");

  static Function const Object_GetInteriorObjects_Registration = Function_Bind(
  "Object_GetInteriorObjects",
  "Return an iterator to the objects inside 'container'",
  [](Object const& container) -> InteriorIterator
  {
    return InteriorIterator(container, 0);
  
  },
  "container");
static int const Object_GetInteriorObjects_Alias = Function_Alias("Object_GetInteriorObjects", "GetInteriorObjects");

  static Function const InteriorIterator_Access_Registration = Function_Bind(
  "InteriorIterator_Access",
  "Return the contents of 'iterator'",
  [](InteriorIterator const& iterator) -> Object
  {
    return iterator.object->GetInterior()->objects[iterator.index];
  
  },
  "iterator");
static int const InteriorIterator_Access_Alias = Function_Alias("InteriorIterator_Access", "Get");

  static Function const InteriorIterator_Advance_Registration = Function_Bind(
  "InteriorIterator_Advance",
  "Advance 'iterator'",
  [](InteriorIterator const& iterator)
  {
    Mutable(iterator).index++;
  
  },
  "iterator");
static int const InteriorIterator_Advance_Alias = Function_Alias("InteriorIterator_Advance", "Advance");

  static Function const InteriorIterator_HasMore_Registration = Function_Bind(
  "InteriorIterator_HasMore",
  "Return whether 'iterator' has more elements",
  [](InteriorIterator const& iterator) -> bool
  {
    return iterator.index < iterator.object->GetInterior()->objects.size();
  
  },
  "iterator");
static int const InteriorIterator_HasMore_Alias = Function_Alias("InteriorIterator_HasMore", "HasMore");
}
