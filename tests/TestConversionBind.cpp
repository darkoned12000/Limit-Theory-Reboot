// Copyright (C) 2025  darkoned12000
// SPDX-License-Identifier: GPL-3.0-or-later
// Part of the ltheory-old-test modernization effort (Revamp Work).
// See NOTICE and LICENSE.GPL. Original engine (c) Josh Parnell, public domain.
//
// Unit tests for the DefineConversion replacement (Step 8 of the binding-bridge
// replacement, see ltsl-binding-bridge-replacement.md §6.5):
//   - Conversion_Bind / ConversionTrampoline in FunctionBind.h register a
//     `void (*)(Source const&, Dest&)` impl onto the source type's conversion
//     list via TypeT::AddConversion;
//   - the stored ConversionFn (ConversionTrampoline::Call) is invoked with the
//     engine's call convention `(TypeT*, void const*, void*)`;
//   - the 53 migrated engine sites load in-process (liblt static-init) and are
//     callable — spot-checking a representative sample.
//
// Test bindings use T_-prefixed names and only exist inside the lte_tests
// process (the API-DB dump is a separate binary), so they never leak into the
// language-server database.

#include "Harness.h"
#include "LTE/Common.h"
#include "LTE/Function.h"
#include "LTE/FunctionBind.h"
#include "LTE/String.h"
#include "LTE/Type.h"
#include "LTE/V3.h"
#include "LTE/Vector.h"

using namespace LTE;

// ── test subject ────────────────────────────────────────────────────────

static void T_IntToString_Impl(int const& src, String& dest) {
  dest = ToString<int>(src);
}

static int const T_IntToString_Registration =
  Conversion_Bind<&T_IntToString_Impl>();

// ── helpers ─────────────────────────────────────────────────────────────

static ConversionFn FindConversion(Type const& src, Type const& dst) {
  Vector<ConversionType> const& conversions = src->GetConversions();
  for (size_t i = 0; i < conversions.size(); ++i)
    if (conversions[i].other == dst)
      return conversions[i].fn;
  return nullptr;
}

// ── tests ───────────────────────────────────────────────────────────────

LTE_TEST(Conversion_Bind_RegistersOnSourceType) {
  // Conversion_Bind attaches to the SOURCE type's conversion list with the
  // dest type recorded as 'other' — the same layout the interpreter's cast
  // path (Expression/Conversion.cpp FindConversion) looks up.
  ConversionFn fn = FindConversion(Type_Get<int>(), Type_Get<String>());
  LTE_CHECK(fn);
}

LTE_TEST(Conversion_Trampoline_InvokesImpl) {
  // The stored fn is ConversionTrampoline<Fn>::Call — the exact code the
  // interpreter invokes. Call it with the engine convention (TypeT* ignored,
  // opaque in/out pointers).
  ConversionFn fn = FindConversion(Type_Get<int>(), Type_Get<String>());
  LTE_CHECK(fn);
  if (!fn)
    return;

  int src = 42;
  String dest;
  fn(nullptr, &src, &dest);
  LTE_CHECK_EQ(dest, String("42"));
}

LTE_TEST(Conversion_MissingPairReturnsNull) {
  // No String -> V3F conversion exists; the lookup must fail cleanly.
  LTE_CHECK(!FindConversion(Type_Get<String>(), Type_Get<V3F>()));
}

LTE_TEST(Conversion_EngineMigratedSitesPresent) {
  // Step 8 migrated 53 DefineConversion sites to Conversion_Bind. Those
  // registrations run at liblt static-init, so a representative sample must be
  // present in-process and callable. Failure here means a migrated site was
  // dropped or its trampoline is broken.
  int i = 5;
  double d = 0;
  bool b = true;
  String s;
  unsigned int u = 7;
  V3F v3f(1.0f, 2.0f, 3.0f);
  V3D v3d;
  int iout = 0;

  ConversionFn i2s = FindConversion(Type_Get<int>(), Type_Get<String>());
  LTE_CHECK(i2s);
  if (i2s) { i2s(nullptr, &i, &s); LTE_CHECK_EQ(s, String("5")); }

  ConversionFn i2d = FindConversion(Type_Get<int>(), Type_Get<double>());
  LTE_CHECK(i2d);
  if (i2d) { i2d(nullptr, &i, &d); LTE_CHECK_EQ(d, 5.0); }

  ConversionFn b2s = FindConversion(Type_Get<bool>(), Type_Get<String>());
  LTE_CHECK(b2s);
  if (b2s) { b2s(nullptr, &b, &s); LTE_CHECK_EQ(s, String("true")); }

  ConversionFn u2s = FindConversion(Type_Get<unsigned int>(), Type_Get<String>());
  LTE_CHECK(u2s);
  if (u2s) { u2s(nullptr, &u, &s); LTE_CHECK_EQ(s, String("7")); }

  ConversionFn v2 = FindConversion(Type_Get<V3F>(), Type_Get<V3D>());
  LTE_CHECK(v2);
  if (v2) { v2(nullptr, &v3f, &v3d); LTE_CHECK_EQ(v3d.x, 1.0); }

  ConversionFn f2i = FindConversion(Type_Get<float>(), Type_Get<int>());
  LTE_CHECK(f2i);
  if (f2i) { float g = 2.5f; f2i(nullptr, &g, &iout); LTE_CHECK_EQ(iout, 2); }
}
