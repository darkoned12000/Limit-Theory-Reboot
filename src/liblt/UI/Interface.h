#ifndef UI_Interface_h__
#define UI_Interface_h__

#include "Common.h"
#include "LTE/RenderPass.h"

struct InterfaceT : public RefCounted {
  virtual ~InterfaceT() = default;

  virtual void Add(Widget const& widget) = 0;
  virtual void Clear() = 0;
  virtual void Draw() = 0;
  virtual void Update() = 0;

  /* InterfaceT is an abstract RefCounted base, only ever used through
     Reference<InterfaceT>. Reflected so Type_Get<InterfaceT>() resolves to a
     real (named) type instead of the generic "unknown type" fallback, which
     otherwise left Reference<InterfaceT> as "Reference<unknown type>" (null
     function pointers) and crashed any Data -> Interface conversion. */
  DeclareMetadata(InterfaceT)
};

LT_API Interface Interface_Create(
  String const& name);

LT_API RenderPass RenderPass_Interface(
  Interface const& interface);

#endif
