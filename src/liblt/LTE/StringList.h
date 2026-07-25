#ifndef LTE_StringList_h__
#define LTE_StringList_h__

#include "AutoClass.h"
#include "BaseType.h"
#include "Pool.h"
#include "Reference.h"
#include "Vector.h"

struct StringListT : public RefCounted {
  BASE_TYPE(StringListT)

  virtual StringList Clone() const = 0;

  virtual StringList Get(size_t index) const {
    return nullptr;
  }

  virtual size_t GetSize() const {
    return 0;
  }

  virtual String GetString() const = 0;

  virtual String GetValue() const {
    return "";
  }

  virtual bool IsAtom() const = 0;
};

AutoClassDerived(StringListAtom, StringListT,
  String, value,
  uint32_t, line)
  DERIVED_TYPE_EX(StringListAtom)
  POOLED_TYPE

  StringListAtom() = default;

  StringListAtom(String const& value) :
    value(value),
    line(0) {}

  StringList Clone() const override {
    return new StringListAtom(*this);
  }

  size_t GetSize() const override {
    return 1;
  }

  String GetString() const override {
    return value;
  }

  String GetValue() const override {
    return value;
  }

  bool IsAtom() const override {
    return true;
  }
};

AutoClassDerived(StringListList, StringListT,
  Vector<StringList>, elements)
  DERIVED_TYPE_EX(StringListList)
  POOLED_TYPE
  
  StringListList() = default;

  StringList Clone() const override {
    return new StringListList(*this);
  }

  StringList Get(size_t index) const override {
    return elements[index];
  }

  size_t GetSize() const override {
    return elements.size();
  }

  String GetString() const override {
    String str = "(";
    for (size_t i = 0; i < elements.size(); ++i) {
      if (i)
        str += " ";
      str += elements[i]->GetString();
    }

    return str + ")";
  }
  
  bool IsAtom() const override {
    return false;
  }
};

LT_API StringList StringList_Create(String const& data);
LT_API StringList StringList_Load(Location const& location);
LT_API void StringList_Print(StringList const& list);

inline uint32_t StringList_GetLine(StringList const& list) {
  if (!list) return 0;
  if (list->IsAtom())
    return ((StringListAtom*)list.t)->line;
  if (list->GetSize() > 0 && list->Get(0)->IsAtom())
    return ((StringListAtom*)list->Get(0).t)->line;
  return 0;
}

#endif
