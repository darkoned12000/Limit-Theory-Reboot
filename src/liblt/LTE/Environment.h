// Copyright (C) 2025  darkoned12000
// SPDX-License-Identifier: GPL-3.0-or-later
// Part of the ltheory-old-test modernization effort (Revamp Work).
// See NOTICE and LICENSE.GPL. Original engine (c) Josh Parnell, public domain.

#ifndef LTE_Environment_h__
#define LTE_Environment_h__

#include "AutoClass.h"
#include "Data.h"
#include "Expression.h"
#include "Map.h"
#include "ScriptType.h"
#include "Stack.h"
#include "StringList.h"
#include "Vector.h"

#include <algorithm>
#include <iostream>
#include <limits>

namespace LTE {
  struct Environment {
    uint base;
    Vector<void*> registers;

    /* Set by a `return` expression; signals enclosing blocks to stop
       evaluating further expressions and propagate the return value up
       to the calling function. Checked by ExpressionBlock::Evaluate. */
    bool returnSignal;

    /* Canonical return-value slot for the innermost enclosing function
       call. Set by ScriptFunctionT::Call (and ExpressionT::Evaluate) so a
       `return` anywhere in the body writes directly here, regardless of how
       deeply nested it is. Null for void / unknown return slots. */
    void* returnValue;

    Environment() : base(0), returnSignal(false), returnValue(nullptr) {}

    inline void* Allocate(Type const& type) {
      return type->Allocate();
    }

    inline void Free(Type const& type, void* ptr) {
      type->Deallocate(ptr);
    }
  };

  AutoClass(Variable,
    String, name,
    uint, registerIndex,
    int, offset,
    Type, type,
    bool, constant,
    bool, reference)

    Variable() = default;
  };

  /* Levenshtein edit distance — used for "did you mean?" suggestions. */
  inline size_t EditDistance(String const& a, String const& b) {
    size_t m = a.size();
    size_t n = b.size();
    std::vector<size_t> prev(n + 1, 0);
    std::vector<size_t> curr(n + 1, 0);

    for (size_t j = 0; j <= n; ++j)
      prev[j] = j;

    for (size_t i = 1; i <= m; ++i) {
      curr[0] = i;
      for (size_t j = 1; j <= n; ++j) {
        size_t cost = (a[i - 1] == b[j - 1]) ? 0 : 1;
        curr[j] = std::min({
          prev[j] + 1,
          curr[j - 1] + 1,
          prev[j - 1] + cost
        });
      }
      std::swap(prev, curr);
    }
    return prev[n];
  }

  /* Find the closest matching name from a list of candidates. Returns empty
     string if no candidate is within edit-distance threshold. */
  inline String BestMatch(
    String const& target,
    Vector<String> const& candidates,
    size_t maxDistance = 3)
  {
    String best;
    size_t bestDist = maxDistance + 1;
    for (size_t i = 0; i < candidates.size(); ++i) {
      size_t d = EditDistance(target, candidates[i]);
      if (d < bestDist) {
        bestDist = d;
        best = candidates[i];
      }
    }
    return best;
  }

  struct CompileEnvironment {
    Pointer<ScriptT> script;
    Vector<ScriptType> context;
    Map<String, Stack<Variable> > variables;
    int registers;
    bool hasErrors;
    Vector<String> errors;

    CompileEnvironment() :
      registers(0),
      hasErrors(false)
      {}

    void ReportError(StringList const& list, String const& message) {
      uint32_t line = StringList_GetLine(list);
      String formatted;
      if (line > 0)
        formatted = Stringize() | "  line " | line | ": " | message;
      else
        formatted = Stringize() | "  " | message;
      errors.push(formatted);
      hasErrors = true;
    }

    void PrintErrors(String const& scriptName) const {
      if (errors.size() == 0) return;
      std::cout << "'" << scriptName << "' -- " << errors.size()
                << " compilation error(s):" << std::endl;
      for (size_t i = 0; i < errors.size(); ++i)
        std::cout << errors[i] << std::endl;
    }

    uint Allocate(
      String const& name,
      Type const& type,
      bool constant,
      bool reference)
    {
      uint registerIndex = registers++;
      variables[name].push(Variable(name, registerIndex, -1, type, constant, reference));
      return registerIndex;
    }

    bool Contains(String const& name) {
      return variables[name].size() > 0;
    }

    void Free(String const& name) {
      variables[name].pop();
      registers--;
    }

    Variable& Get(String const& name) {
      return variables[name].back();
    }

    /* Collect all visible variable names for "did you mean?" suggestions. */
    void CollectVariableNames(Vector<String>& names) const {
      for (auto it = variables.begin(); it != variables.end(); ++it) {
        if (it->second.size() > 0)
          names.push(it->first);
      }
    }
  };
}

#endif
