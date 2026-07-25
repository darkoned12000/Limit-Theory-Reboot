// Copyright (C) 2025  darkoned12000
// SPDX-License-Identifier: GPL-3.0-or-later
// Part of the ltheory-old-test modernization effort (Revamp Work).
// See NOTICE and LICENSE.GPL. Original engine (c) Josh Parnell, public domain.
//
// Unit tests for the LTSL compilation error-reporting infrastructure:
//   - EditDistance / BestMatch helpers
//   - CompileEnvironment error collection and line-number tracking
//   - Expression_Compile error output for various bad-input scenarios

#include "Harness.h"
#include "LTE/Environment.h"
#include "LTE/Expression.h"
#include "LTE/Expressions.h"
#include "LTE/Script.h"
#include "LTE/StringList.h"

using namespace LTE;

// ── EditDistance tests ────────────────────────────────────────────────

LTE_TEST(EditDistance_IdenticalStrings) {
  LTE_CHECK_EQ(EditDistance("hello", "hello"), size_t(0));
}

LTE_TEST(EditDistance_EmptyStrings) {
  LTE_CHECK_EQ(EditDistance("", ""), size_t(0));
}

LTE_TEST(EditDistance_OneInsertion) {
  LTE_CHECK_EQ(EditDistance("cat", "cats"), size_t(1));
}

LTE_TEST(EditDistance_OneDeletion) {
  LTE_CHECK_EQ(EditDistance("cats", "cat"), size_t(1));
}

LTE_TEST(EditDistance_OneSubstitution) {
  LTE_CHECK_EQ(EditDistance("cat", "car"), size_t(1));
}

LTE_TEST(EditDistance_CompletelyDifferent) {
  LTE_CHECK_EQ(EditDistance("abc", "xyz"), size_t(3));
}

LTE_TEST(EditDistance_OneEmpty) {
  LTE_CHECK_EQ(EditDistance("abc", ""), size_t(3));
  LTE_CHECK_EQ(EditDistance("", "abc"), size_t(3));
}

// ── BestMatch tests ───────────────────────────────────────────────────

LTE_TEST(BestMatch_ExactMatch) {
  Vector<String> candidates;
  candidates.push("foo");
  candidates.push("bar");
  candidates.push("baz");
  LTE_CHECK_EQ(BestMatch("foo", candidates), String("foo"));
}

LTE_TEST(BestMatch_CloseMatch) {
  Vector<String> candidates;
  candidates.push("playerPosition");
  candidates.push("playerHealth");
  candidates.push("playerVelocity");
  // "playerPostion" is one edit away from "playerPosition"
  LTE_CHECK_EQ(BestMatch("playerPostion", candidates), String("playerPosition"));
}

LTE_TEST(BestMatch_NoMatch) {
  Vector<String> candidates;
  candidates.push("foo");
  candidates.push("bar");
  // "xyz" is 3 edits from "foo" and 3 from "bar", threshold is 3 by default
  // but since it's within threshold, it returns the closest
  // Use a tighter threshold to get empty
  String result = BestMatch("completelyDifferent", candidates, 2);
  LTE_CHECK(result.size() == 0);
}

LTE_TEST(BestMatch_EmptyCandidates) {
  Vector<String> candidates;
  String result = BestMatch("anything", candidates);
  LTE_CHECK(result.size() == 0);
}

// ── CompileEnvironment error reporting tests ──────────────────────────

LTE_TEST(CompileEnvironment_CollectVariableNames) {
  CompileEnvironment env;
  env.Allocate("myVar", Type_Get<int>(), false, false);
  env.Allocate("otherVar", Type_Get<float>(), false, false);

  Vector<String> names;
  env.CollectVariableNames(names);
  LTE_CHECK(names.size() >= 2);

  // Check that our variables are in the list
  bool foundMyVar = false;
  bool foundOtherVar = false;
  for (size_t i = 0; i < names.size(); ++i) {
    if (names[i] == "myVar") foundMyVar = true;
    if (names[i] == "otherVar") foundOtherVar = true;
  }
  LTE_CHECK(foundMyVar);
  LTE_CHECK(foundOtherVar);
}

LTE_TEST(CompileEnvironment_ReportErrorCollectsErrors) {
  CompileEnvironment env;
  LTE_CHECK(!env.hasErrors);
  LTE_CHECK_EQ(env.errors.size(), size_t(0));

  // Create a minimal StringList for testing
  StringList list = StringList_Create("testToken");

  env.ReportError(list, "test error message");
  LTE_CHECK(env.hasErrors);
  LTE_CHECK_EQ(env.errors.size(), size_t(1));
}

LTE_TEST(CompileEnvironment_LineNumberInError) {
  CompileEnvironment env;
  // StringList_Create doesn't set line numbers (they're set by the file loader),
  // but ReportError should handle line == 0 gracefully
  StringList list = StringList_Create("(some expression)");
  env.ReportError(list, "error without line number");

  // The error should contain the message but not "line 0"
  LTE_CHECK(env.errors.size() == 1);
  // Since line is 0, the formatted string should just be "  error without line number"
  LTE_CHECK(env.errors[0].find("error without line number") != String::npos);
}

LTE_TEST(CompileEnvironment_MultipleErrorsAccumulate) {
  CompileEnvironment env;
  StringList list1 = StringList_Create("token1");
  StringList list2 = StringList_Create("token2");
  StringList list3 = StringList_Create("token3");

  env.ReportError(list1, "first error");
  env.ReportError(list2, "second error");
  env.ReportError(list3, "third error");

  LTE_CHECK_EQ(env.errors.size(), size_t(3));
  LTE_CHECK(env.hasErrors);
}

// ── Expression_Compile error output tests ─────────────────────────────

LTE_TEST(Expression_Compile_UnknownVariableReportsError) {
  // Compile an unknown variable name — should produce an error
  StringList list = StringList_Create("someUndefinedVariable");
  CompileEnvironment env;
  env.script = new ScriptT;
  env.script->name = "testScript";

  Expression_Compile(list, env);
  LTE_CHECK(env.hasErrors);
  LTE_CHECK(env.errors.size() > 0);

  // Should contain the variable name in the error
  bool foundName = false;
  for (size_t i = 0; i < env.errors.size(); ++i) {
    if (env.errors[i].find("someUndefinedVariable") != String::npos)
      foundName = true;
  }
  LTE_CHECK(foundName);
}

LTE_TEST(Expression_Compile_BadIfArgCount) {
  // (if) with no arguments should report error about missing arguments
  StringList list = StringList_Create("(if)");
  CompileEnvironment env;
  env.script = new ScriptT;
  env.script->name = "testScript";

  Expression_Compile(list, env);
  LTE_CHECK(env.hasErrors);
  bool foundIfError = false;
  for (size_t i = 0; i < env.errors.size(); ++i) {
    if (env.errors[i].find("'if'") != String::npos)
      foundIfError = true;
  }
  LTE_CHECK(foundIfError);
}

LTE_TEST(Expression_Compile_BadWhileArgCount) {
  StringList list = StringList_Create("(while)");
  CompileEnvironment env;
  env.script = new ScriptT;
  env.script->name = "testScript";

  Expression_Compile(list, env);
  LTE_CHECK(env.hasErrors);
  bool foundWhileError = false;
  for (size_t i = 0; i < env.errors.size(); ++i) {
    if (env.errors[i].find("'while'") != String::npos)
      foundWhileError = true;
  }
  LTE_CHECK(foundWhileError);
}

LTE_TEST(Expression_Compile_BadAssignArgCount) {
  // (set x) — only 1 argument instead of 2
  StringList list = StringList_Create("(set x)");
  CompileEnvironment env;
  env.script = new ScriptT;
  env.script->name = "testScript";

  Expression_Compile(list, env);
  LTE_CHECK(env.hasErrors);
  bool foundSetError = false;
  for (size_t i = 0; i < env.errors.size(); ++i) {
    if (env.errors[i].find("'set'") != String::npos)
      foundSetError = true;
  }
  LTE_CHECK(foundSetError);
}

LTE_TEST(Expression_Compile_BadCastArgCount) {
  StringList list = StringList_Create("(cast)");
  CompileEnvironment env;
  env.script = new ScriptT;
  env.script->name = "testScript";

  Expression_Compile(list, env);
  LTE_CHECK(env.hasErrors);
  bool foundCastError = false;
  for (size_t i = 0; i < env.errors.size(); ++i) {
    if (env.errors[i].find("'cast'") != String::npos)
      foundCastError = true;
  }
  LTE_CHECK(foundCastError);
}

LTE_TEST(Expression_Compile_BadVarArgCount) {
  // (var x) — only 1 argument instead of 2
  StringList list = StringList_Create("(var x)");
  CompileEnvironment env;
  env.script = new ScriptT;
  env.script->name = "testScript";
  Vector<String> locals;

  Expression_Compile(list, env, &locals);
  LTE_CHECK(env.hasErrors);
  bool foundVarError = false;
  for (size_t i = 0; i < env.errors.size(); ++i) {
    if (env.errors[i].find("'var'") != String::npos)
      foundVarError = true;
  }
  LTE_CHECK(foundVarError);
}

LTE_TEST(Expression_Compile_BadReturnArgCount) {
  // (return x y) — too many arguments
  StringList list = StringList_Create("(return x y)");
  CompileEnvironment env;
  env.script = new ScriptT;
  env.script->name = "testScript";

  Expression_Compile(list, env);
  // Should produce an error about return
  LTE_CHECK(env.hasErrors);
}

LTE_TEST(Expression_Compile_DidYouMean_Variable) {
  // Create a script with a known variable, then reference a typo
  StringList list = StringList_Create("positon");
  CompileEnvironment env;
  env.script = new ScriptT;
  env.script->name = "testScript";

  // Pre-populate a similar variable name
  env.Allocate("position", Type_Get<float>(), false, false);

  Expression_Compile(list, env);
  LTE_CHECK(env.hasErrors);
  bool foundSuggestion = false;
  for (size_t i = 0; i < env.errors.size(); ++i) {
    if (env.errors[i].find("did you mean") != String::npos)
      foundSuggestion = true;
  }
  LTE_CHECK(foundSuggestion);
}
