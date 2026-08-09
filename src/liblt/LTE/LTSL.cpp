#include "LTSL.h"
#include "StringList.h"

namespace {
  void RewriteDot(StringList& list) {
    String const& value = list->GetValue();
    if (!String_IsNumeric(value) &&
         value.contains('.') &&
        !value.contains('"'))
    {
      Vector<String> parts;
      String_Split(parts, value, '.');
      StringList newList = new StringListAtom(parts[0]);
      for (size_t i = 1; i < parts.size(); ++i)
        newList = new StringListList(Vector<StringList>(
          new StringListAtom(parts[i]),
          newList));

      list = newList;
    }
  }

  bool IsBinaryOp(StringList const& list, Vector<String> const& ops) {
    String const& value = list->GetValue();
    for (size_t i = 0; i < ops.size(); ++i)
      if (value == ops[i])
        return true;
    return false;
  }

  void RewriteBinaryOp(StringList& list, Vector<String> const& ops) {
    StringListList* l = (StringListList*)list.t;
    for (int i = 0; i + 2 < (int)l->elements.size(); ++i) {
      if (IsBinaryOp(l->elements[i + 1], ops)) {
        l->elements[i] = new StringListList(Vector<StringList>(
          l->elements[i + 1],
          l->elements[i],
          l->elements[i + 2]));
        l->elements.eraseIndex(i + 1);
        l->elements.eraseIndex(i + 1);
        i--;
      }
    }
  }

  /* `else` is not a head keyword in Expression_Compile, so the parser's
     sibling `(if ...)` / `(else ...)` lists would otherwise compile the
     `else` branch as a bogus function call. Merge each `(else ...)` list
     into the `(if ...)` list that precedes it, yielding a single flat
     `(if pred body... else stmt...)` list that Expression_If consumes.
     Repeated-until-fixed scans also collapse `else if` chains. */
  void RewriteElse(StringList& list) {
    StringListList* l = (StringListList*)list.t;
    for (int i = 0; i < (int)l->elements.size(); ++i) {
      if (i + 1 >= (int)l->elements.size())
        break;

      StringList cur = l->elements[i];
      StringList next = l->elements[i + 1];
      if (cur->IsAtom() || next->IsAtom())
        continue;
      if (cur->GetSize() == 0 || cur->Get(0)->GetValue() != "if")
        continue;
      if (next->GetSize() == 0 || next->Get(0)->GetValue() != "else")
        continue;

      StringListList* cl = (StringListList*)cur.t;
      StringListList* nl = (StringListList*)next.t;
      /* Append every `(else ...)` element — its head `else` atom first, so
         an `(else if b ...)` chain keeps its `else` marker in the merged
         `(if ...)` list. */
      for (size_t j = 0; j < nl->elements.size(); ++j)
        cl->elements.push(nl->elements[j]);
      l->elements.eraseIndex(i + 1);
      i--;
    }
  }

  void RewriteAtom(StringList& list) {
    RewriteDot(list);
  }

  void RewriteList(StringList& list) {
    static Vector<String> precedence[] = {
      Vector<String>() << "^",
      Vector<String>() << "*" << "/",
      Vector<String>() << "+" << "-",
      Vector<String>() << "<" << ">" << "<=" << ">=",
      Vector<String>() << "==" << "!=",
      Vector<String>() << "&&",
      Vector<String>() << "||",
      Vector<String>() << "=" << "+=" << "-=" << "*=" << "/="
    };

    for (uint i = 0; i < sizeof(precedence) / sizeof(*precedence); ++i)
      RewriteBinaryOp(list, precedence[i]);

    RewriteElse(list);
  }

  void Rewrite(StringList& list) {
    StringListList* l = (StringListList*)list.t;
    for (size_t i = 0; i < l->elements.size(); ++i) {
      if (l->elements[i]->IsAtom())
        RewriteAtom(l->elements[i]);
      else
        Rewrite(l->elements[i]);
    }
    RewriteList(list);
  }
}

StringList LTSL_ApplyRewrites(StringList const& list) {
  StringList newList = list;
  Rewrite(newList);
  return newList;
}
