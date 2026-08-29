#ifndef LTE_Function_h__
#define LTE_Function_h__

#include "Type.h"
#include "Parameter.h"
#include "Reference.h"
#include "String.h"

struct BindingBase;   // defined in FunctionBind.h

struct FunctionT : public RefCounted {
  String name;
  String description;
  void (*call)(void* binding, void** in, void* out);
  BindingBase* binding;   // owned; null for manual handlers
  uint paramCount;
  Parameter const* params;
  Type returnType;

  FunctionT() = default;

  LT_API ~FunctionT();

  LT_API Data& GetAux(); 

  LT_API String GetSignature() const;

  template <class StreamT>
  friend void _ToStream(StreamT& s, FunctionT const& t) {
    if (t.description)
      s << "/* " << t.description << " */\n";
    s << t.GetSignature();
  }
};

LT_API Function Function_Create(String const& name);

LT_API void Function_AddAlias(String const& source, String const& alias);
LT_API Vector<Function> const& Function_Find(String const& name);
LT_API bool Function_Exists(String const& name);
LT_API Vector<Function> const& Function_GetList();
LT_API void Function_ForEach(void* user, void (*callback)(
  void* user, String const& name, Vector<Function> const& functions));

#endif
