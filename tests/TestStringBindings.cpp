// Copyright (C) 2025  darkoned12000
// SPDX-License-Identifier: GPL-3.0-or-later
// Part of the ltheory-old-test modernization effort (Revamp Work).
// See NOTICE and LICENSE.GPL. Original engine (c) Josh Parnell, public domain.
//
// String helper + script-binding regression tests.
//
// The engine's LTSL string surface had silent-corruption bugs:
//   - FromString<T> returned uninitialized memory on a failed parse (so
//     ToInt("") produced garbage instead of 0);
//   - the String_Substring binding ignored its 'start' argument
//     (substr(0, length)), breaking every config parser that sliced lines.
// Both are fixed; these tests keep them from regressing.
//
// These tests exercise the C++ helpers directly and the registered script
// functions through Function_Find/call so a regression in a binding's
// argument handling (like Substring's hardcoded 0) is caught.

#include "Harness.h"
#include "LTE/Function.h"
#include "LTE/String.h"
#include "LTE/Type.h"
#include "LTE/Vector.h"

using namespace LTE;

// ── FromString<T> garbage-in protection ─────────────────────────────────

LTE_TEST(FromString_FailedParseReturnsZero) {
  LTE_CHECK_EQ(FromString<int>(""), 0);
  LTE_CHECK_EQ(FromString<int>("not a number"), 0);
  LTE_CHECK_EQ(FromString<long>("abc"), 0L);
  LTE_CHECK_EQ(FromString<float>(""), 0.0f);
  LTE_CHECK_EQ(FromString<double>("garbage"), 0.0);
}

LTE_TEST(FromString_ValidParseUnaffected) {
  LTE_CHECK_EQ(FromString<int>("42"), 42);
  LTE_CHECK_EQ(FromString<int>("-7"), -7);
  LTE_CHECK_EQ(FromString<float>("3.5"), 3.5f);
  LTE_CHECK_EQ(FromString<double>("12.25"), 12.25);
  LTE_CHECK_EQ(FromString<String>("hello"), String("hello"));
}

// ── String_Split helper ─────────────────────────────────────────────────

static void CheckSplit(char const* label, Vector<String> const& parts, size_t expected) {
  (void)label;
  LTE_CHECK_EQ(parts.size(), expected);
}

LTE_TEST(String_Split_CharDelimiter) {
  Vector<String> parts;
  String_Split(parts, "a,b,c", ',');
  CheckSplit("char split", parts, 3);
  LTE_CHECK_EQ(parts[0], String("a"));
  LTE_CHECK_EQ(parts[1], String("b"));
  LTE_CHECK_EQ(parts[2], String("c"));
}

LTE_TEST(String_Split_PreservesEmptyFields) {
  Vector<String> parts;
  String_Split(parts, "a,,c", ',');
  CheckSplit("empty middle", parts, 3);
  LTE_CHECK_EQ(parts[0], String("a"));
  LTE_CHECK_EQ(parts[1], String(""));
  LTE_CHECK_EQ(parts[2], String("c"));
}

LTE_TEST(String_Split_NoDelimiterReturnsWholeString) {
  Vector<String> parts;
  String_Split(parts, "shipHull:20000", 'x');
  CheckSplit("no match", parts, 1);
  LTE_CHECK_EQ(parts[0], String("shipHull:20000"));

  String_Split(parts, "anything", ',');
  CheckSplit("no comma", parts, 1);
  LTE_CHECK_EQ(parts[0], String("anything"));
}

// ── Script-binding regression tests (via Function_Find/call) ────────────

static Function FindBinding(String const& name, size_t paramCount) {
  Vector<Function> const& funcs = Function_Find(name);
  for (size_t i = 0; i < funcs.size(); ++i)
    if (funcs[i]->paramCount == paramCount)
      return funcs[i];
  return nullptr;
}

LTE_TEST(Binding_SubstringRespectsStart) {
  // Regression: String_Substring used to hardcode substr(0, length),
  // silently ignoring 'start'. This broke config parsing ("shipHull:20000"
  // sliced with start=9 returned "shipH" instead of "20000").
  Function fn = FindBinding("Substring", 3);
  LTE_CHECK(fn);
  if (!fn)
    return;

  String s = "shipHull:20000";
  int start = 9;
  int length = 5;
  void* args[] = { &s, &start, &length };
  String result;
  fn->call(fn->binding, args, &result);
  LTE_CHECK_EQ(result, String("20000"));

  int start2 = 0;
  void* args2[] = { &s, &start2, &length };
  fn->call(fn->binding, args2, &result);
  LTE_CHECK_EQ(result, String("shipH"));
}

LTE_TEST(Binding_SplitLinesExists) {
  // SplitLines is the entry point the config parsers actually use.
  String name = "SplitLines";
  Vector<Function> const& funcs = Function_Find(name);
  LTE_CHECK_EQ(funcs.size(), size_t(1));
  if (funcs.size() != 1)
    return;

  String s = "a\nb\nc";
  void* args[] = { &s };
  Vector<String> result;
  funcs[0]->call(funcs[0]->binding, args, &result);
  LTE_CHECK_EQ(result.size(), size_t(3));
  if (result.size() == 3) {
    LTE_CHECK_EQ(result[0], String("a"));
    LTE_CHECK_EQ(result[1], String("b"));
    LTE_CHECK_EQ(result[2], String("c"));
  }
}

LTE_TEST(Binding_ToIntParsesDigits) {
  String name = "ToInt";
  Vector<Function> const& funcs = Function_Find(name);
  LTE_CHECK_EQ(funcs.size(), size_t(1));
  if (funcs.size() != 1)
    return;

  String s = "42";
  void* args[] = { &s };
  int result = 0;
  funcs[0]->call(funcs[0]->binding, args, &result);
  LTE_CHECK_EQ(result, 42);
}
