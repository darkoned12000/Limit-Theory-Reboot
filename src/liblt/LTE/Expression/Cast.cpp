#include "../Expressions.h"

#include "LTE/Environment.h"
#include "LTE/Script.h"
#include "LTE/StringList.h"

#include "LTE/Debug.h"

namespace LTE {
  Expression Expression_Cast(
    StringList const& list,
    CompileEnvironment& env)
  {
    if (list->GetSize() != 3) {
      env.ReportError(list, "'cast' expects 2 arguments (type, expression)");
      return nullptr;
    }

    Type type = env.script->ResolveType(list->Get(1));
    if (!type) {
      env.ReportError(list, Stringize()
        | "'cast' -- unknown type '" | list->Get(1)->GetString() | "'");
      return nullptr;
    }

    Expression e = Expression_Compile(list->Get(2), env);
    if (!e)
      return nullptr;

    return Expression_Conversion(e, type);
  }
}
