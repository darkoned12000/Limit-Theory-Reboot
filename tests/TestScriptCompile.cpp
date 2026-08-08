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

// ── literal-atom probe-silencing regression (ltsl-hardening §5.1) ─────

LTE_TEST(Expression_Compile_LiteralAtomNoProbeNoise) {
  // Compiling a bare literal (number/string/bool) must NOT report the
  // spurious Variable/Reference/FunctionCall/Constructor probe-chain errors
  // ("unknown variable '1'", ...) — the Constant factory is the only probe
  // that can accept literals, so the earlier probes are skipped entirely.
  char const* literals[] = {
    "1", "3.5", "-7", "0.25", "\"hello\"", "'x'", "true", "false"
  };

  for (char const* lit : literals) {
    StringList atom = new StringListAtom(lit);
    CompileEnvironment env;
    env.script = new ScriptT;
    env.script->name = "testScript";

    Expression expr = Expression_Compile(atom, env);
    LTE_CHECK(expr);
    LTE_CHECK(!env.hasErrors);
  }
}

LTE_TEST(Expression_Compile_HashCommentAtomNoNoise) {
  // A bare '#' atom is a comment marker — it must compile silently, not feed
  // the probe chain (observed as "line 7" errors in App/ui.lts).
  StringList atom = new StringListAtom("#");
  CompileEnvironment env;
  env.script = new ScriptT;
  env.script->name = "testScript";

  Expression expr = Expression_Compile(atom, env);
  LTE_CHECK(!expr);
  LTE_CHECK(!env.hasErrors);
}

LTE_TEST(StringList_Create_StripsCommentLines) {
  // Comment lines whose text contains a binary operator (e.g. `# * foo`)
  // previously leaked into the parser: RewriteBinaryOp turned `# * foo`
  // into `(* # foo)` BEFORE the head-`#` comment check in Expression_Compile,
  // so the comment text compiled as code and produced spurious diagnostics
  // ("unknown variable 'geometry'", "no function named ''", ...). Strip `#`
  // lines at parse time so any comment text is safe, and line numbers for
  // later diagnostics still count comment lines.
  //
  // NOTE: we assert that NO diagnostic mentions comment content, not that
  // env.hasErrors is false — single-line StringList_Create input double-wraps
  // and triggers the known spurious wrapper-probe errors (see the
  // Expression_While_ReturnInLoopBreaks comment below).
  String const source =
    "# * Cursor-driven focus (the HitTest/CaptureMouse family) tracks the\n"
    "# 0.5 * size and a dot.foo reference\n"
    "5\n";

  StringList list = StringList_Create(source);
  CompileEnvironment env;
  env.script = new ScriptT;
  env.script->name = "testScript";

  Expression expr = Expression_Compile(list, env);
  LTE_CHECK(expr);
  for (size_t i = 0; i < env.errors.size(); ++i) {
    LTE_CHECK(env.errors[i].find("Cursor") == String::npos);
    LTE_CHECK(env.errors[i].find("geometry") == String::npos);
    LTE_CHECK(env.errors[i].find("family") == String::npos);
  }
}

// ── return-in-loop regression (While honors returnSignal) ──────────────
LTE_TEST(Expression_While_ReturnInLoopBreaks) {
  // Regression guard for the ltheory-main hang: ExpressionWhile never
  // honored returnSignal, so a `return` inside a loop body (e.g. the
  // Config_Get early-exit) spun forever on the same iteration. The body
  // here returns 42 while the predicate stays true — before the fix this
  // test hung; now Evaluate returns with returnSignal set.
  //
  // The list is built programmatically: StringList_Create double-wraps
  // single-line input, which triggers spurious first-pass errors in
  // Expression_Compile (constructor/field-access probes on the wrapper).
  Vector<StringList> returnElements;
  returnElements.push(new StringListAtom("return"));
  returnElements.push(new StringListAtom("42"));
  StringList returnExpr = new StringListList(returnElements);

  Vector<StringList> elements;
  elements.push(new StringListAtom("while"));
  elements.push(new StringListAtom("1"));
  elements.push(returnExpr);
  StringList list = new StringListList(elements);

  CompileEnvironment env;
  env.script = new ScriptT;
  env.script->name = "testScript";

  Expression expr = Expression_Compile(list, env);
  /* NOTE: env.hasErrors is NOT asserted here — Expression_Compile's probe
     chain (Variable/Reference/FunctionCall/Constructor) reports spurious
     "unknown variable '1'"-style errors for literal atoms before the
     Constant factory succeeds (pre-existing A.8 noise, non-fatal in the
     engine). The compile still yields the ExpressionWhile via the
     single-element recursion; the regression under test is the runtime
     behavior below. */
  LTE_CHECK(expr);
  LTE_CHECK(expr);

  Environment runtimeEnv;
  int result = 0;
  runtimeEnv.returnValue = &result;
  expr->Evaluate(&result, runtimeEnv);

  LTE_CHECK(runtimeEnv.returnSignal);
  LTE_CHECK_EQ(result, 42);
}

// ── function-body compile errors must surface (ltsl-hardening §5.x) ────
// A function body compiles into its own sub-environment; before the fix,
// Expression_Function never propagated subEnv errors, so a compile error
// inside a body silently no-opped the offending statement while the app
// "ran fine". Here a mixed Float/Int comparison (ambiguous: both
// Float_Greater and Int_Greater match with one implicit conversion) must
// produce a visible diagnostic in the OUTER environment.
LTE_TEST(FunctionBody_CompileErrorsSurface) {
  Vector<StringList> body;
  Vector<StringList> gt;
  gt.push(new StringListAtom(">"));
  gt.push(new StringListAtom("x"));
  gt.push(new StringListAtom("0"));
  body.push(new StringListList(gt));

  Vector<StringList> params;
  params.push(new StringListAtom("Float"));
  params.push(new StringListAtom("x"));

  Vector<StringList> fn;
  fn.push(new StringListAtom("function"));
  fn.push(new StringListAtom("Bool"));
  fn.push(new StringListAtom("F"));
  fn.push(new StringListList(params));
  fn.push(new StringListList(body));
  StringList fnList = new StringListList(fn);

  CompileEnvironment env;
  env.script = new ScriptT;
  env.script->name = "testScript";

  Expression_Compile(fnList, env);
  LTE_CHECK(env.hasErrors);
  bool foundAmbiguous = false;
  for (size_t i = 0; i < env.errors.size(); ++i) {
    if (env.errors[i].find("ambiguous call to '>'") != String::npos)
      foundAmbiguous = true;
  }
  LTE_CHECK(foundAmbiguous);
}
